/* http_parser.c 
 * parser for hue http server
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <strings.h>

#include "http_parser.h"
#include "uprintf.h"
#include "hue.h"

const char *hue_url_tokens[] = 
{
    "unknow", //HUE_URL_TOKEN_UNKNOW,
    "description.xml", //HUE_URL_TOKEN_DESCRIPTION_XML,
    "api", //HUE_URL_TOKEN_API,
    "lights", //HUE_URL_TOKEN_LIGHTS,
    "new", //HUE_URL_TOKEN_NEW,
    "state", //HUE_URL_TOKEN_STATE,
    "groups", //HUE_URL_TOKEN_GROUPS,
    "action", //HUE_URL_TOKEN_ACTION,
    "schedules", //HUE_URL_TOKEN_SCHEDULES,
    "scenes", //HUE_URL_TOKEN_SCENES,
    "sensors", //HUE_URL_TOKEN_SENSORS,
    "config", //HUE_URL_TOKEN_CONFIG,
    "rules", //HUE_URL_TOKEN_RULES,
    "whitelist", //HUE_URL_WHITELIST,
    "timezones", //HUE_URL_TIMEZONES,
};

/* return the first token by parsing a string with separator character 
 * str: input -- string; output -- after the first token 
 * return: first token
 */
char *strtoken(char **str, char separator)
{
	char *rstr;

	if(*str == NULL)
		return NULL;

	while(**str == separator)
	{
		**str = '\0';
		(*str)++;
	}

	if(**str == '\0')
		return NULL;

	rstr = *str;

	while(**str != '\0')
	{
		if(**str == separator)
		{
			**str = '\0';
			(*str)++;
			break;
		}
		(*str)++;
	}
	return rstr;
}

// strip the leading characters which is in set of string c
// ie http_strip_chars("aabbccdd", "ab") return 4 ("ccdd");
int http_strip_chars(char *in_buf, char *c)
{
	int offset = 0;
	char *delimiter = c;

	do{
		delimiter = c;
		while(*delimiter && in_buf[offset] != *delimiter)
			delimiter ++;
		if(*delimiter) 
			offset ++;
	}while(*delimiter);

	return offset;
}

int http_parse_content_type(char *in_buf, uint16_t length)
{
	int offset = 0;
	int type = HTTP_CONTENTTYPE_JSON;

	// strip leading blanks
	offset = http_strip_chars(in_buf, " ");
	in_buf += offset;
	
	if(strcmp(in_buf, HTTP_STR_CONTENT_TYPE_JSON) == 0)
		type = HTTP_CONTENTTYPE_JSON;
	else if(strcmp(in_buf, HTTP_STR_CONTENT_TYPE_HTML) == 0)
		type = HTTP_CONTENTTYPE_HTML;

	return type;
}

/**
 * parse a HTTP URL
 */
void http_parse_url(char *in_buf, uint16_t length, http_parser_t *parser)
{
    char *line=in_buf;
    char *tmpptr;
    int num_token = 0, i;

    if(in_buf == NULL || length == 0)
        return;

    do
    {
        tmpptr = strtoken(&line, '/');
        parser->url_token[num_token ++] = tmpptr;
    }while(tmpptr && (num_token <= HTTP_URL_TOKEN_MAX_NUM));

    parser->num_token = num_token-1;

    for(num_token=0; num_token<parser->num_token; num_token++)
    {
        for(i=1; i<=HUE_URL_TOKEN_TIMEZONES; i++)
        {
            if(strcmp(hue_url_tokens[i], parser->url_token[num_token]) == 0)
            {
                /* save the token value */
                parser->val_token[num_token] = i;
                break;
            }
        }
    }

    uprintf(UPRINT_DEBUG, UPRINT_BLK_HUE, "HTTP URL token number: %d\n", parser->num_token);
}


/* parse HTTP request line
 * Request-Line = Method SP Request-URI SP HTTP-Version CRLF
 * return: the offset to Content-Type
 */
int http_parse_request_line(char *in_buf, uint16_t length, http_parser_t *parser)
{
	int offset = 0;
	char *line=in_buf;
	char *tmpptr;

	// get the first token "Method", current support "get", "post" and "put"
	tmpptr = strtoken(&line, ' ');
	if(strcmp(tmpptr, HTTP_STR_GET) == 0)
	{
		parser->req_type = HTTP_REQTYPE_GET;
	}
	else if(strcmp(tmpptr, HTTP_STR_PUT) == 0)
	{
		parser->req_type = HTTP_REQTYPE_PUT;
	}
	else if(strcmp(tmpptr, HTTP_STR_POST) == 0)
	{
		parser->req_type = HTTP_REQTYPE_POST;
	}
	else
		offset = -1;

	if(offset >= 0)
	{
		// req type is OK, get and terminatel url
		tmpptr = strtoken(&line, ' ');
        uprintf(UPRINT_INFO, UPRINT_BLK_HUE, "http url -- %s\n", tmpptr);
        http_parse_url(tmpptr, strlen(tmpptr), parser);

		// go to next line
		tmpptr = strstr(line, HTTP_STR_LF);
		if(tmpptr == NULL)
			offset = length;
		else
			offset = tmpptr + 2 - in_buf; // 2="\r\n"
	}
	
	return offset;
}

/* parse http request headers: only interested in content-type/content-length/connection */
int http_parse_request_headers(char *in_buf, uint16_t length, http_parser_t *parser)
{
	int offset = 0;
	char *next_line = NULL, *line, *header, *value;

	parser->content_len = 0;
	parser->content_type = HTTP_CONTENTTYPE_JSON;

	line = in_buf;
	/* parser one line every loop */
	while(offset < length)
	{
		if(line[0] == '\r' && line[1] == '\n')
		{
			// HTTP body start
			offset += 2;
			break;
		}

		// get next line first
		next_line = strstr(line, HTTP_STR_LF);
		if(next_line)
		{
			next_line[0] = '\0';
			next_line[1] = '\0';
		}
		
		// get header field
		header = line + http_strip_chars(line, " "); //strip leading blanks
		strtoken(&line, ':');
		value = line + http_strip_chars(line, " ");
		
		if(strcasecmp(header, HTTP_STR_CONTENT_TYPE) == 0)
		{
			parser->content_type = http_parse_content_type(value, length-offset);
		}
		else if(strcasecmp(header, HTTP_STR_CONTENT_LENGTH) == 0)
		{
			parser->content_len = strtol(value, NULL, 10);
		}
		else if(strcasecmp(header, HTTP_STR_CONNECTION) == 0)
		{
			parser->keep_live = !strcasecmp(value, HTTP_STR_KEEPALIVE);
		}

		// update offset
		if(next_line)
		{
			offset = next_line + 2 - in_buf;
			line = next_line + 2;
		}
		else
			offset = length;

        parser->num_request_header ++;
		uprintf(UPRINT_DEBUG, UPRINT_BLK_HUE, "request header -- %s:%s\n", header, value);
	}
	
	return offset;
}

/* parse http request body: only interested in json */
int http_parse_request_body(char *in_buf, uint16_t length, http_parser_t *parser)
{
	if(length)
	{
		parser->content = in_buf;
        parser->content[length] = 0; // terminate string
        if(parser->content_len == 0)
            parser->content_len = length;
	}
	else
		parser->content = NULL;
	
	return length;
}

/**
 * parse a HTTP request message.
 */
int http_parse_request(char *in_buf, uint16_t length, http_parser_t *parser)
{
	int offset = 0;
	
	/* parse request line */
	offset  += http_parse_request_line(in_buf, length, parser);
	if(offset < 0)
		return -1;

	if(parser->req_type == HTTP_REQTYPE_GET)
	{
		/* parse http headers */
		offset += http_parse_request_headers(&in_buf[offset], length - offset, parser);
	}
	else if(parser->req_type == HTTP_REQTYPE_POST)
	{
		/* parse http headers */
		offset += http_parse_request_headers(&in_buf[offset], length - offset, parser);

		/* parse http body if this is a post method */
		offset += http_parse_request_body(&in_buf[offset], length - offset, parser);
	}
	else if(parser->req_type == HTTP_REQTYPE_PUT)
	{
		/* parse http headers */
		offset += http_parse_request_headers(&in_buf[offset], length - offset, parser);

		/* parse http body if this is a post method */
		offset += http_parse_request_body(&in_buf[offset], length - offset, parser);
	}
	else
	{
		uprintf_default("unknow HTTP request type: %s\n", in_buf);
	}

    uprintf(UPRINT_DEBUG, UPRINT_BLK_HUE, "HTTP parser result: reqType=%d nURLToken=%d nHeader=%d contentLen=%d\n",
        parser->req_type, parser->num_token, parser->num_request_header, parser->content_len);

	return offset;
}

#ifdef HTTP_HOST_TEST
int main(int argc, char *argv[])
{
	http_parser_t http_parser;
	int i;
	char msgbuf[1000];
	char *msgs[] = {
		"GET / HTTP/1.1 \r\n",
		"GET /api/whoru/lights HTTP/1.1 \r\n        Content-Type: text/html\r\nContent-Length: 2\r\n\r\n{}",
		"GET /ntab.gif?c=ntab&t=timing&a=1-f-newssites&d=1297&f=&r=0.6531566826163131&cid=stub.firefox.others HTTP/1.1\r\n Host: addons.g-fox.cn\r\nConnection: keep-alive\r\n",
		"POST /api/whoru/lights HTTP/1.1 \r\n Content-Type: application/json\r\n Content-Length: 50\r\n\r\n{\"deviceid\":[\"45AF34\",\"543636\",\"34AFBE\"]}",
		"PUT /api/whoru/lights/1 HTTP/1.1 \r\n Content-Type: application/json\r\n Content-Length: 30\r\n\r\n{\"name\":\"Bedroom Light\"}",
		"PUT /api/whoru/lights/2 HTTP/1.1 \r\n Content-Type: application/json\r\n Content-Length: 40\r\n\r\n{\"hue\": 50000,\"on\": true,\"bri\": 200}",
	};

	for(i=0; i<6; i++)
	{
		strcpy(msgbuf, msgs[i]);
		printf("========= %d =========\n", i);
		memset(&http_parser, 0, sizeof(http_parser));

		http_parse_request(msgbuf, strlen(msgbuf), &http_parser);

	}

	return 0;
}

#endif

