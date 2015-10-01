/* http_parser.h */

#ifndef _HTTP_PARSER_H_
#define _HTTP_PARSER_H_

//#define HTTP_HOST_TEST
#ifdef HTTP_HOST_TEST
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define uprintf_default printf
#endif

#define HTTP_STR_GET "GET"
#define HTTP_STR_PUT "PUT"
#define HTTP_STR_POST "POST"
#define HTTP_STR_LF "\r\n"
#define HTTP_STR_CONTENT_TYPE "Content-Type"
#define HTTP_STR_CONTENT_LENGTH "Content-Length"
#define HTTP_STR_CONNECTION "Connection"
#define HTTP_STR_KEEPALIVE "Keep-Alive"

#define HTTP_STR_CONTENT_TYPE_JSON "application/json"
#define HTTP_STR_CONTENT_TYPE_APP "application"
#define HTTP_STR_CONTENT_TYPE_HTML "text/html"



#define HTTP_REQTYPE_GET 1
#define HTTP_REQTYPE_POST 2
#define HTTP_REQTYPE_PUT 3

#define HTTP_CONTENTTYPE_UNKNOW 0
#define HTTP_CONTENTTYPE_JSON 1
#define HTTP_CONTENTTYPE_HTML 2
#define HTTP_CONTENTTYPE_APP 3

enum
{
    HUE_URL_TOKEN_UNKNOW,
    HUE_URL_TOKEN_DESCRIPTION_XML,
    HUE_URL_TOKEN_API,
    HUE_URL_TOKEN_LIGHTS,
    HUE_URL_TOKEN_NEW,
    HUE_URL_TOKEN_STATE,
    HUE_URL_TOKEN_GROUPS,
    HUE_URL_TOKEN_ACTION,
    HUE_URL_TOKEN_SCHEDULES,
    HUE_URL_TOKEN_SCENES,
    HUE_URL_TOKEN_SENSORS,
    HUE_URL_TOKEN_CONFIG,
    HUE_URL_TOKEN_RULES,
    HUE_URL_TOKEN_WHITELIST,
    HUE_URL_TOKEN_TIMEZONES,
};

#define HTTP_URL_TOKEN_MAX_NUM 8
typedef struct http_parser 
{
	uint8_t req_type;
	uint8_t content_type;
	uint16_t content_len;
	uint8_t keep_live;
    uint8_t num_request_header;
    uint8_t num_token;
    uint8_t val_token[HTTP_URL_TOKEN_MAX_NUM];
	char *url_token[HTTP_URL_TOKEN_MAX_NUM];
	char *content;
}http_parser_t;


int http_parse_request(char *in_buf, uint16_t length, http_parser_t *parser);

#endif //_HTTP_PARSER_H_

