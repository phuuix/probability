#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/netif.h"

#include "memory.h"
#include "siprintf.h"
#include "httpserver-netconn.h"
#include "http_parser.h"
#include "cJSON.h"
#include "Hue.h"

#if LWIP_NETCONN

#ifndef HTTPD_DEBUG
#define HTTPD_DEBUG         LWIP_DBG_OFF
#endif

#define HTTP_THREAD_STACKSIZE 0x800
#define HTTP_THREAD_PRIO      0x4

const static char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
const static char http_index_html[] = "<html><head><title>Probability</title></head><body><h1>Welcome to Probability HTTP server!</h1><p>Please use HUE JSON API to access.</body></html>";
const static char http_html_hdr_err[] = "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\n\r\n";

const static char http_json_hdr[] = "HTTP/1.1 200 OK\r\nCatch-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\nPragma: no-cache\r\nConnection: close\r\nContent-type: application/json\r\n\r\n";
const static char http_xml_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/xml\r\nConnection: close\r\n\r\n";

const static char http_description_xml1[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \
<root xmlns=\"urn:schemas-upnp-org:device-1-0\"> \
<specVersion> \
<major>1</major> \
<minor>0</minor> \
</specVersion>";

const static char http_description_xml2[] = "<device> \
<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType> \
<friendlyName>My bridge </friendlyName> \
<manufacturer>Royal Philips Electronics</manufacturer> \
<manufacturerURL>http://www.philips.com</manufacturerURL> \
<modelDescription>Philips hue Personal Wireless Lighting</modelDescription> \
<modelName>Philips hue bridge 2012</modelName> \
<modelNumber>929000226503</modelNumber> \
<modelURL>http://www.meethue.com</modelURL> \
<serialNumber>0017881733aa</serialNumber>";

const static char http_description_xml3[] = "<presentationURL>index.html</presentationURL> \
</device></root>";

/*
const static char http_description_xml3[] = "<presentationURL>index.html</presentationURL> \
<iconList> \
<icon> \
<mimetype>image/png</mimetype> \
<height>48</height> \
<width>48</width> \
<depth>24</depth> \
<url>hue_logo_0.png</url> \
</icon> \
<icon> \
<mimetype>image/png</mimetype> \
<height>120</height> \
<width>120</width> \
<depth>24</depth> \
<url>hue_logo_3.png</url> \
</icon> \
</iconList> \
</device></root>";
*/
#define HTTP_REQ_BUFF_SIZE 1024
#define HTTP_RESP_BUFF_SIZE 2048
static char http_request_buf[HTTP_REQ_BUFF_SIZE];
static char http_response_buf[HTTP_RESP_BUFF_SIZE];

/* build location string from ip address */
uint32_t http_build_ipaddr(char *outstr, uint32_t size)
{
    uint32_t offset = 0, ip;
    uint8_t *pIp;

    *outstr = '\0';
    if(netif_default)
    {
        /* build location */
        ip = (netif_default->ip_addr.addr);
        pIp = (uint8_t *)&ip;
        offset += siprintf(outstr, size, "%d.%d.%d.%d", pIp[0], pIp[1], pIp[2], pIp[3]);
    }

    return offset;
}

/* build UUID from MAC address in form of 2f402f80-da50-1111-9933-00118877ff55 */
uint32_t http_build_uuid(char *outstr, uint32_t size)
{
    uint32_t offset = 0;
    uint8_t *mac;

    *outstr = '\0';
    if(netif_default)
    {
        /* build location */
        mac = netif_default->hwaddr;
        offset += siprintf(outstr, size, "2f402f80-da50-1111-9933-%02x%02x%02x%02x%02x%02x", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    return offset;
}

/* build hue bridge ID in form of mac0:mac1:mac2:fffe:mac3:mac4:mac5 */
uint32_t http_build_bridgeid(char *outstr, int32_t size)
{
	uint32_t offset = 0;
	uint8_t *mac;

	*outstr = '\0';
	if(netif_default)
	{
		mac = netif_default->hwaddr;
		offset += siprintf(outstr, size, "%02x%02x%02xFFFE%02x%02x%02x", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}

	return offset;
}

static void http_serv_send_err_page(struct netconn *conn)
{
    /* Send the HTML header */
    netconn_write(conn, http_html_hdr_err, sizeof(http_html_hdr_err)-1, NETCONN_NOCOPY);

    /* Send our HTML page */
    netconn_write(conn, http_index_html, sizeof(http_index_html)-1, NETCONN_NOCOPY);
}

/* return length of response */
static int http_serv_get(struct netconn *conn, http_parser_t *http_parser, char *responseBuf, uint16_t max_resp_size)
{
    int responseBuf_len = 0;
    uint16_t len;
    char tmpbuf[64];

    if(http_parser->num_token == 0)
    {
        /* url = "/" */
        /* Send the HTML header 
             * subtract 1 from the size, since we dont send the \0 in the string
             * NETCONN_NOCOPY: our data is const static, so no need to copy it
        */
        memcpy(responseBuf, http_html_hdr, sizeof(http_html_hdr)-1);
        responseBuf_len += sizeof(http_html_hdr)-1;

        /* Send our HTML page */
        memcpy(responseBuf, http_index_html, sizeof(http_index_html)-1);
        responseBuf_len += sizeof(http_index_html)-1;
    }
    else if(http_parser->val_token[0] == HUE_URL_TOKEN_DESCRIPTION_XML)
    {
        responseBuf_len = 0;
        memcpy(&responseBuf[responseBuf_len], http_xml_hdr, sizeof(http_xml_hdr)-1);
        responseBuf_len += sizeof(http_xml_hdr)-1;
        memcpy(&responseBuf[responseBuf_len], http_description_xml1, sizeof(http_description_xml1)-1);
        responseBuf_len += sizeof(http_description_xml1)-1;
        /* <URLBase>http://192.168.0.106:80/</URLBase> */
        len = http_build_ipaddr(tmpbuf, 64);
        len = siprintf(&responseBuf[responseBuf_len], max_resp_size-responseBuf_len, "<URLBase>http://%s:80/</URLBase>", tmpbuf);
        responseBuf_len += len;
        memcpy(&responseBuf[responseBuf_len], http_description_xml2, sizeof(http_description_xml2)-1);
        responseBuf_len += sizeof(http_description_xml2)-1;
        /* <UDN>uuid:2f402f80-da50-11e1-9b23-0017881733fb</UDN> */
        len = http_build_uuid(tmpbuf, 64);
        len = siprintf(&responseBuf[responseBuf_len], max_resp_size-responseBuf_len, "<UDN>uuid:%s</UDN>", tmpbuf);
        responseBuf_len += len;
        memcpy(&responseBuf[responseBuf_len], http_description_xml3, sizeof(http_description_xml3)-1);
        responseBuf_len += sizeof(http_description_xml3)-1;
    }
    else if(http_parser->val_token[0] == HUE_URL_TOKEN_API)
    {
        if((http_parser->val_token[2] == HUE_URL_TOKEN_LIGHTS) && (http_parser->val_token[3] == HUE_URL_TOKEN_NEW))
        {
            /* Get new lights: /api/<username>/lights/new */
        }
        else if((http_parser->val_token[2] == HUE_URL_TOKEN_LIGHTS) && (http_parser->num_token == 3))
        {
            memcpy(&responseBuf[responseBuf_len], http_json_hdr, sizeof(http_json_hdr)-1);
            responseBuf_len += sizeof(http_json_hdr)-1;

            /* get all lights: /api/<username>/lights */
            responseBuf_len += process_hue_api_get_all_lights(&responseBuf[responseBuf_len], max_resp_size-responseBuf_len);
        }
        else if((http_parser->val_token[2] == HUE_URL_TOKEN_LIGHTS) && (http_parser->num_token == 4))
        {
            uint16_t light_id;

            light_id = strtol(http_parser->url_token[3], NULL, 10);

            memcpy(&responseBuf[responseBuf_len], http_json_hdr, sizeof(http_json_hdr)-1);
            responseBuf_len += sizeof(http_json_hdr)-1;

            /* Get light attributes and state: /api/<username>/lights/<id> */
            responseBuf_len += process_hue_api_get_light_attr_state(light_id, &responseBuf[responseBuf_len], max_resp_size-responseBuf_len);
        }
        else if((http_parser->val_token[2] == HUE_URL_TOKEN_CONFIG))
        {
            memcpy(&responseBuf[responseBuf_len], http_json_hdr, sizeof(http_json_hdr)-1);
            responseBuf_len += sizeof(http_json_hdr)-1;

            /* get configuration: /api/<username>/config */
            responseBuf_len += process_hue_api_get_configuration(&responseBuf[responseBuf_len], max_resp_size-responseBuf_len);
        }
        else if((http_parser->val_token[2] == HUE_URL_TOKEN_GROUPS) && (http_parser->num_token == 3))
        {
            memcpy(&responseBuf[responseBuf_len], http_json_hdr, sizeof(http_json_hdr)-1);
            responseBuf_len += sizeof(http_json_hdr)-1;

            /* Get all groups: /api/<username>/groups */
            responseBuf_len += process_hue_api_get_all_groups(&responseBuf[responseBuf_len], max_resp_size-responseBuf_len);
        }
		else if((http_parser->val_token[2] == HUE_URL_TOKEN_SCHEDULES) && (http_parser->num_token == 3))
		{
			memcpy(&responseBuf[responseBuf_len], http_json_hdr, sizeof(http_json_hdr)-1);
            responseBuf_len += sizeof(http_json_hdr)-1;

            /* Get all schedules: /api/<username>/schedules */
            responseBuf_len += process_hue_api_get_all_schedules(&responseBuf[responseBuf_len], max_resp_size-responseBuf_len);
		}
		else if((http_parser->val_token[2] == HUE_URL_TOKEN_SCENES) && (http_parser->num_token == 3))
		{
			memcpy(&responseBuf[responseBuf_len], http_json_hdr, sizeof(http_json_hdr)-1);
            responseBuf_len += sizeof(http_json_hdr)-1;

            /* Get all scenes: /api/<username>/scenes */
            responseBuf_len += process_hue_api_get_all_scenes(&responseBuf[responseBuf_len], max_resp_size-responseBuf_len);
		}
		else if((http_parser->val_token[2] == HUE_URL_TOKEN_SENSORS) && (http_parser->num_token == 3))
		{
			memcpy(&responseBuf[responseBuf_len], http_json_hdr, sizeof(http_json_hdr)-1);
            responseBuf_len += sizeof(http_json_hdr)-1;

            /* Get all sensors: /api/<username>/sensors */
            responseBuf_len += process_hue_api_get_all_sensors(&responseBuf[responseBuf_len], max_resp_size-responseBuf_len);
		}
		else if((http_parser->val_token[2] == HUE_URL_TOKEN_RULES) && (http_parser->num_token == 3))
		{
			memcpy(&responseBuf[responseBuf_len], http_json_hdr, sizeof(http_json_hdr)-1);
            responseBuf_len += sizeof(http_json_hdr)-1;

            /* Get all sensors: /api/<username>/rules */
            responseBuf_len += process_hue_api_get_all_rules(&responseBuf[responseBuf_len], max_resp_size-responseBuf_len);
		}
        else if((http_parser->val_token[2] == HUE_URL_TOKEN_GROUPS) && (http_parser->num_token == 4))
        {
            uint32_t group_id;

            memcpy(&responseBuf[responseBuf_len], http_json_hdr, sizeof(http_json_hdr)-1);
            responseBuf_len += sizeof(http_json_hdr)-1;

            /* Get group attributes: /api/<username>/groups/<id> */
            group_id = strtol(http_parser->url_token[3], NULL, 10);
            responseBuf_len += process_hue_api_get_group_attr(group_id, &responseBuf[responseBuf_len], max_resp_size-responseBuf_len);
        }
        else if(http_parser->num_token == 2)
        {
            /* Get full state (datastore): /api/<username> */
            memcpy(&responseBuf[responseBuf_len], http_json_hdr, sizeof(http_json_hdr)-1);
            responseBuf_len += sizeof(http_json_hdr)-1;

            responseBuf_len += process_hue_api_get_full_state(&responseBuf[responseBuf_len], max_resp_size-responseBuf_len);
        }
    }

    assert(responseBuf_len< max_resp_size);
    return responseBuf_len;
}

static int http_serv_post(struct netconn *conn, http_parser_t *http_parser, char *responseBuf, uint16_t max_resp_size)
{
    int responseBuf_len = 0;
    cJSON *json_root = NULL;

    json_root = cJSON_Parse(http_parser->content);

    if(http_parser->val_token[0] != HUE_URL_TOKEN_API)
    {
        cJSON_Delete(json_root);
        return 0;
    }

    if(http_parser->num_token == 1)
    {
        cJSON *json_devtype = NULL;
        cJSON *json_username = NULL;
        char *devtype = NULL;
        char *username = NULL;

        /* this is a create user command */
        if(json_root)
        {
            json_devtype = cJSON_GetObjectItem(json_root, "devicetype");
            if(json_devtype) devtype = json_devtype->valuestring;
            json_username = cJSON_GetObjectItem(json_root, "username");
            if(json_username) username = json_username->valuestring;
        }

        memcpy(responseBuf, http_json_hdr, sizeof(http_json_hdr)-1);
        responseBuf_len += sizeof(http_json_hdr)-1;
        responseBuf_len += process_hue_api_create_user(devtype, username,&responseBuf[responseBuf_len], max_resp_size-responseBuf_len);
    }
    else if(http_parser->val_token[2] == HUE_URL_TOKEN_LIGHTS && http_parser->num_token == 3)
    {
        /* this is a search new lights command */
        memcpy(responseBuf, http_json_hdr, sizeof(http_json_hdr)-1);
        responseBuf_len += sizeof(http_json_hdr)-1;
        responseBuf_len += process_hue_api_search_new_lights(&responseBuf[responseBuf_len], max_resp_size-responseBuf_len);
    }

    /* free json object */
    cJSON_Delete(json_root);

    return responseBuf_len;
}

static int http_serv_put(struct netconn *conn, http_parser_t *http_parser, char *responseBuf, uint16_t max_resp_size)
{
    int responseBuf_len = 0;
    cJSON *json_root = NULL;

    json_root = cJSON_Parse(http_parser->content);

    if(http_parser->num_token == 4 && http_parser->val_token[2] == HUE_URL_TOKEN_LIGHTS)
    {		
        cJSON *json_light_name;
        uint16_t light_id;
        
        /* this is rename light command */
        light_id = strtol(http_parser->url_token[3], NULL, 10);

		if(json_root)
		{
	        json_light_name = cJSON_GetObjectItem(json_root,"name");
	        if(!json_light_name)
	        	uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "rename light: not found light name!\n");
	        else
	        {
	        	uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "rename light %d:%s\n", light_id, json_light_name->valuestring);

	            memcpy(responseBuf, http_json_hdr, sizeof(http_json_hdr)-1);
	            responseBuf_len += sizeof(http_json_hdr)-1;
	            responseBuf_len += process_hue_api_rename_light(light_id, json_light_name->valuestring, &responseBuf[responseBuf_len], max_resp_size-responseBuf_len);
	        }
		}
    }
    else if(http_parser->val_token[2] == HUE_URL_TOKEN_LIGHTS && http_parser->val_token[4] == HUE_URL_TOKEN_STATE)
    {
        uint32_t bitmap=0;
        cJSON *json_state;
        hue_light_t *in_light;
        uint16_t light_id;
        
        light_id = strtol(http_parser->url_token[3], NULL, 10);

        /* set light state command */
        // in_light will be freed by hue task
        in_light = malloc(sizeof(hue_light_t));
        assert(in_light);

        in_light->id = light_id;
        if(json_root)
        {
            json_state = cJSON_GetObjectItem(json_root, "on");
            if(json_state)
            {
                bitmap |= (1<<HUE_STATE_BIT_ON);
                in_light->on = json_state->valueint;
            }

            json_state = cJSON_GetObjectItem(json_root, "bri");
            if(json_state)
            {
                bitmap |= (1<<HUE_STATE_BIT_BRI);
                in_light->bri= json_state->valueint;
            }

            json_state = cJSON_GetObjectItem(json_root, "hue");
            if(json_state)
            {
                bitmap |= (1<<HUE_STATE_BIT_HUE);
                in_light->hue = (json_state->valueint >> 8);
            }

            json_state = cJSON_GetObjectItem(json_root, "sat");
            if(json_state)
            {
                bitmap |= (1<<HUE_STATE_BIT_SAT);
                in_light->sat= json_state->valueint;
            }

            json_state = cJSON_GetObjectItem(json_root, "xy");
            if(json_state && (json_state->type == cJSON_Array))
            {
                cJSON *json_x, *json_y;

                json_x = cJSON_GetArrayItem(json_state, 0);
                json_y = cJSON_GetArrayItem(json_state, 1);
                if(json_x && json_y)
                {
                    bitmap |= (1<<HUE_STATE_BIT_XY);
                    in_light->x = json_x->valuedouble * (1<<16); // xy in Q0.16
                    in_light->y = json_y->valuedouble * (1<<16);
                    uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "set xy [%d %d]\n", in_light->x, in_light->y);
                }
            }

            json_state = cJSON_GetObjectItem(json_root, "ct");
            if(json_state)
            {
                bitmap |= (1<<HUE_STATE_BIT_CT);
                in_light->ct= json_state->valueint;
            }
        }

        uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "set light %d bitmap: 0x%x\n", light_id, bitmap);

        memcpy(responseBuf, http_json_hdr, sizeof(http_json_hdr)-1);
        responseBuf_len += sizeof(http_json_hdr)-1;

        responseBuf_len += process_hue_api_set_light_state(light_id,in_light,bitmap, &responseBuf[responseBuf_len], max_resp_size-responseBuf_len);
    }
    else
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "unknown PUT command\n");

    /* free json object */
    cJSON_Delete(json_root);

    return responseBuf_len;
}

static void http_dump_string(char *title, char *str)
{
	char c;
	uint16_t len;
	int offset;
	
	len = strlen(str);

	/* uprintf can only support about 200 bytes for each print */
	for(offset=0; offset<len; offset += 200)
	{
		if(offset + 200 < len)
		{
			c = str[offset+200];
			str[offset+200] = 0;
		}
		uprintf(UPRINT_DEBUG, UPRINT_BLK_HUE, "%s(%04d): %s\n", title, offset, &str[offset]);
		if(offset + 200 < len)
		{
			str[offset+200] = c;
		}
	}
}

/** Serve one HTTP connection accepted in the http thread */
static void
http_server_netconn_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  char *requestBuf = http_request_buf;
  u16_t requestBuf_len = 0;
  err_t err;
  http_parser_t http_parser;
  char *responseBuf = http_response_buf;
  int responseBuf_len = 0;

  /* Read the data from the port, blocking if nothing yet there. 
   We assume the request (the part we care about) is in one netbuf */
  err = netconn_recv(conn, &inbuf);
  
  if (err == ERR_OK) {
    // FIXME: handle the case one HTTP packet is splitted into more than one TCP packet
    requestBuf_len = netbuf_copy(inbuf, requestBuf, HTTP_REQ_BUFF_SIZE);
    requestBuf[requestBuf_len] = '\0';

    // for debug
    http_dump_string("http request", requestBuf);

    memset(&http_parser, 0, sizeof(http_parser));
    http_parse_request(requestBuf, requestBuf_len, &http_parser);

	/* if request contains "Expect: 100-continue", there will be no the body part */ 
	if(http_parser.content_len <= strlen(http_parser.content))
	{
		memset(responseBuf,0,HTTP_RESP_BUFF_SIZE);

	    if(http_parser.req_type == HTTP_REQTYPE_GET )
	    {
	      responseBuf_len = http_serv_get(conn, &http_parser, responseBuf, HTTP_RESP_BUFF_SIZE);
		}
	    else if(http_parser.req_type == HTTP_REQTYPE_POST && http_parser.content_type == HTTP_CONTENTTYPE_JSON)
	    {
	      uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "http content: %s\n", http_parser.content);
	      responseBuf_len = http_serv_post(conn, &http_parser, responseBuf, HTTP_RESP_BUFF_SIZE);
	    }
		else if(http_parser.req_type == HTTP_REQTYPE_PUT && http_parser.content_type == HTTP_CONTENTTYPE_JSON)
	    {
	      uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "http content: %s\n", http_parser.content);
	      responseBuf_len = http_serv_put(conn, &http_parser, responseBuf, HTTP_RESP_BUFF_SIZE);
	    }
	}

    if(responseBuf_len)
    {
      netconn_write(conn,responseBuf,responseBuf_len,NETCONN_NOCOPY);

	  // for debug
      http_dump_string("http response", responseBuf);
    }
    else
    {
      http_serv_send_err_page(conn);
      uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "unknown HTTP request\n");
    }
  }

  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);
  
  /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
}

/** The main function, never returns! */
static void
http_server_netconn_thread(void *arg)
{
  struct netconn *conn, *newconn;
  err_t err;
  LWIP_UNUSED_ARG(arg);

  task_delay(100);
  
  /* Create a new TCP connection handle */
  conn = netconn_new(NETCONN_TCP);
  LWIP_ERROR("http_server: invalid conn", (conn != NULL), return;);
  
  /* Bind to port 80 (HTTP) with default IP address */
  netconn_bind(conn, NULL, 80);
  
  /* Put the connection into LISTEN state */
  netconn_listen(conn);
  
  do {
    err = netconn_accept(conn, &newconn);
    if (err == ERR_OK) {
      http_server_netconn_serve(newconn);
      netconn_delete(newconn);
    }
  } while(err == ERR_OK);
  LWIP_DEBUGF(HTTPD_DEBUG,
    ("http_server_netconn_thread: netconn_accept received error %d, shutting down",
    err));
  netconn_close(conn);
  netconn_delete(conn);
}

/** Initialize the HTTP server (start its thread) */
void
http_server_netconn_init()
{
  sys_thread_new("thttpd", http_server_netconn_thread, NULL, HTTP_THREAD_STACKSIZE, HTTP_THREAD_PRIO);
}

#endif /* LWIP_NETCONN*/
