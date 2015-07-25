
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"

#include "httpserver-netconn.h"
#include "http_parser.h"

#if LWIP_NETCONN

#ifndef HTTPD_DEBUG
#define HTTPD_DEBUG         LWIP_DBG_OFF
#endif

const static char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
const static char http_index_html[] = "<html><head><title>Probability</title></head><body><h1>Welcome to Probability HTTP server!</h1><p>Please use HUE JSON API to access.</body></html>";
const static char http_html_hdr_err[] = "HTTP/1.1 404 Not Found\r\nContent-type: text/html\r\n\r\n";


/** Serve one HTTP connection accepted in the http thread */
static void
http_server_netconn_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  char *buf;
  u16_t buflen;
  err_t err;
  http_parser_t http_parser;
  
  /* Read the data from the port, blocking if nothing yet there. 
   We assume the request (the part we care about) is in one netbuf */
  err = netconn_recv(conn, &inbuf);
  
  if (err == ERR_OK) {
    netbuf_data(inbuf, (void**)&buf, &buflen);
    
    memset(&http_parser, 0, sizeof(http_parser));
    http_parse_request(buf, buflen, &http_parser);

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
      else
      {
        /* build request and send it to hue task */
      }
    }
    else if(http_parser.req_type == HTTP_REQTYPE_POST && http_parser.content_type == HTTP_CONTENTTYPE_JSON)
    {
      /* call json parser: cJSON_ParseWithOpts 
       * return a cJSON object
       */
      
      /* build a C structure from cJSON object and send to hue task */

      /* wait for response from hue task with timeout */

      /* send HTML header */
      netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);

      /* build HTML page based on response of hue */

      /* send HTML response */
    }
    else if(http_parser.req_type == HTTP_REQTYPE_PUT && http_parser.content_type == HTTP_CONTENTTYPE_JSON)
    {
      // similar as POST request
    }
    else
    {
      /* Send the HTML header */
      netconn_write(conn, http_html_hdr_err, sizeof(http_html_hdr_err)-1, NETCONN_NOCOPY);
      
      /* Send our HTML page */
      netconn_write(conn, http_index_html, sizeof(http_index_html)-1, NETCONN_NOCOPY);
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
  sys_thread_new("thttpd", http_server_netconn_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
}

#endif /* LWIP_NETCONN*/
