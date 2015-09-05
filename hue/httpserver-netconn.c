#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"

#include "memory.h"
#include "httpserver-netconn.h"
#include "http_parser.h"
#include "cJSON.h"
#include "Hue.h"

#if LWIP_NETCONN

#ifndef HTTPD_DEBUG
#define HTTPD_DEBUG         LWIP_DBG_OFF
#endif

const static char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
const static char http_index_html[] = "<html><head><title>Probability</title></head><body><h1>Welcome to Probability HTTP server!</h1><p>Please use HUE JSON API to access.</body></html>";
const static char http_html_hdr_err[] = "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\n\r\n";

const static char http_json_hdr[] = "HTTP/1.1 200 OK\r\nCatch-Control: no-store, no-cache, must-revalidate, post-check=0, pre-check=0\r\nPragma: no-cache\r\nConnection: close\r\nContent-type: application/json\r\n\r\n";

#define JSON_RESP_BUFF_SIZE 1024
static char json_response_buf[JSON_RESP_BUFF_SIZE];

/** Serve one HTTP connection accepted in the http thread */
static void
http_server_netconn_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  char *buf;
  u16_t buflen = 0;
  err_t err;
  http_parser_t http_parser;
  char *responseBuf = json_response_buf;
  int responseBuf_len;
  cJSON *json_root = NULL;

  /* Read the data from the port, blocking if nothing yet there. 
   We assume the request (the part we care about) is in one netbuf */
  err = netconn_recv(conn, &inbuf);
  
  if (err == ERR_OK) {
    netbuf_data(inbuf, (void**)&buf, &buflen);
    buf[buflen] = '\0';
    uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "http packet: %s\n", (char *)buf);

    memset(&http_parser, 0, sizeof(http_parser));
    http_parse_request(buf, buflen, &http_parser);

	memset(responseBuf,0,JSON_RESP_BUFF_SIZE);

    //task_delay(1); // wait for uprintf to finish
	//dump_buffer((uint8_t *)http_parser.content, http_parser.content_len);

    if(http_parser.req_type == HTTP_REQTYPE_GET )
    {
      if(strcmp(http_parser.url, "/") == 0)
      {
        /* Send the HTML header 
             * subtract 1 from the size, since we dont send the \0 in the string
             * NETCONN_NOCOPY: our data is const static, so no need to copy it
        */
        netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);

        /* Send our HTML page */
        netconn_write(conn, http_index_html, sizeof(http_index_html)-1, NETCONN_NOCOPY);
      }
      else if(strncmp(http_parser.url, "/api",4) == 0)
      {
        /* get all lights commands */
		responseBuf_len = process_hue_api_get_all_lights(responseBuf, JSON_RESP_BUFF_SIZE);

		uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "response_alllight:%s\n", responseBuf);

		netconn_write(conn, http_json_hdr, sizeof(http_json_hdr)-1, NETCONN_NOCOPY);

		netconn_write(conn,responseBuf,responseBuf_len,NETCONN_NOCOPY);
      }
      else
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "unknown GET command\n");
	}
    else if(http_parser.req_type == HTTP_REQTYPE_POST && http_parser.content_type == HTTP_CONTENTTYPE_JSON)
    {
      json_root = cJSON_Parse(http_parser.content);
      assert(json_root);

      if(strcmp(http_parser.url, "/api")==0)
      {
        cJSON *json_devtype;
        cJSON *json_username;

        /* this is a create user command */
        json_devtype = cJSON_GetObjectItem(json_root, "devicetype");
        assert(json_devtype);
        json_username = cJSON_GetObjectItem(json_root, "username");
        assert(json_username);
        uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "json_devtype:%s,json_username: %s\n",json_devtype->valuestring,json_username->valuestring);

        responseBuf_len = process_hue_api_create_user(json_devtype->valuestring,json_username->valuestring,responseBuf,JSON_RESP_BUFF_SIZE);

        uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "responsebuf:%s\n",responseBuf);

        netconn_write(conn, http_json_hdr, sizeof(http_json_hdr)-1, NETCONN_NOCOPY);

        netconn_write(conn,responseBuf,responseBuf_len,NETCONN_NOCOPY);
      }
      else if(strstr(http_parser.url, "lights"))
      {
        /* assume this is a search new lights command */
        uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "search for new lights\n");
        responseBuf_len = process_hue_api_search_new_lights(responseBuf, JSON_RESP_BUFF_SIZE);
        uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "responsebuf:%s\n",responseBuf);
        netconn_write(conn, http_json_hdr, sizeof(http_json_hdr)-1, NETCONN_NOCOPY);
        netconn_write(conn,responseBuf,responseBuf_len,NETCONN_NOCOPY);
      }
      else
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "unknown POST command\n");
    }
	else if(http_parser.req_type == HTTP_REQTYPE_PUT && http_parser.content_type == HTTP_CONTENTTYPE_JSON)
    {
      if(strcmp(http_parser.url,"/api/pzz/lights/1")==0)
      {		
        cJSON *json_light_name;

        /* this is rename light command */
        json_root = cJSON_Parse(http_parser.content);
        assert(json_root);
        json_light_name = cJSON_GetObjectItem(json_root,"name");
        if(!json_light_name)
        	uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "not found!\n");
        else
        	uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "json_light_name:%s\n",json_light_name->valuestring);

        responseBuf_len = process_hue_api_rename_light(1,json_light_name->valuestring,responseBuf,JSON_RESP_BUFF_SIZE);

        uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "respons_put:%s\n",responseBuf);

        netconn_write(conn,http_json_hdr,sizeof(http_json_hdr)-1,NETCONN_NOCOPY);

        netconn_write(conn,responseBuf,responseBuf_len,NETCONN_NOCOPY);
      }
	  else if(strcmp(http_parser.url,"/api/pzz/lights/1/state")==0)
      {
        uint32_t bitmap=0;
        cJSON *json_state;
        hue_light_t *in_light;

        // in_light will be freed by hue task
        in_light = malloc(sizeof(hue_light_t));
        assert(in_light);

        json_root = cJSON_Parse(http_parser.content);
        assert(json_root);

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

        responseBuf_len= process_hue_api_set_light_state(1,in_light,bitmap,responseBuf,JSON_RESP_BUFF_SIZE);

        uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "state response:%s\n",responseBuf);

        netconn_write(conn,http_json_hdr,sizeof(http_json_hdr)-1,NETCONN_NOCOPY);

        netconn_write(conn,responseBuf,responseBuf_len,NETCONN_NOCOPY);
      }
	  else
        uprintf(UPRINT_WARNING, UPRINT_BLK_HUE, "unknown PUT command\n");
    }
    else
    {
      /* Send the HTML header */
      netconn_write(conn, http_html_hdr_err, sizeof(http_html_hdr_err)-1, NETCONN_NOCOPY);
      
      /* Send our HTML page */
      netconn_write(conn, http_index_html, sizeof(http_index_html)-1, NETCONN_NOCOPY);
    }
  }

  /* free json object */
  cJSON_Delete(json_root);

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
  sys_thread_new("thttpd", http_server_netconn_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
}

#endif /* LWIP_NETCONN*/
