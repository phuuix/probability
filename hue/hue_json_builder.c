/*
 * A JSON builder for HUE
 */
#include <stdio.h>
#include <string.h>
#include "siprintf.h"
#include "hue.h"
#include "cJSON.h"
#include "hue_json_builder.h"

#define JSON_SPRINTF siprintf

const static char *strHueSuccess = "success";
const static char *strHueFailure = "failure";
const static char *strHueAlert[] = {"none", "select", "lselect"};
const static char *strHueEffect[] = {"none", "colorloop"};
const static char *strHueColormode[] = {"hs", "xy", "ct"};
//FIXME:   strHueType
const static char *strHueType[] = {"Extended color light", "Living Colors"};   

uint32_t json_build_char(char *out_buf, uint32_t in_len, char c)
{
	uint32_t length = 0;
	
	if(in_len)
	{
		out_buf[length++] = c;
	}

	return length;
}

uint32_t json_build_object_start(char *out_buf, uint32_t in_len)
{
	return json_build_char(out_buf, in_len, '{');
}

uint32_t json_build_object_end(char *out_buf, uint32_t in_len)
{
	return json_build_char(out_buf, in_len, '}');
}

uint32_t json_build_array_start(char *out_buf, uint32_t in_len)
{
	return json_build_char(out_buf, in_len, '[');
}

uint32_t json_build_array_end(char *out_buf, uint32_t in_len)
{
	return json_build_char(out_buf, in_len, ']');
}

uint32_t json_build_string(char *out_buf, uint32_t in_len, const char *str)
{
	const char *ptr;char *ptr2;
	unsigned char token;	
	int printed = 0;	
	int len;		
	if (!out_buf || in_len<2) return 0;	
	ptr2=out_buf;
	ptr=str;	
	INJECT_CHAR(ptr2, '\"', printed);	
	while (*ptr && printed<in_len)	
	{		
		if ((unsigned char)*ptr>31 && *ptr!='\"' && *ptr!='\\') 		
		{			
			INJECT_CHAR(ptr2, *ptr++, printed);		
		}		
		else		
		{
			INJECT_CHAR(ptr2, '\\', printed);			
			switch (token=*ptr++)
			{
				case '\\':	INJECT_CHAR(ptr2, '\\', printed);	
				break;				
				case '\"':	INJECT_CHAR(ptr2, '\"', printed);	
				break;				
				case '\b':	INJECT_CHAR(ptr2, 'b', printed);	
				break;				
				case '\f':	INJECT_CHAR(ptr2, 'f', printed);	
				break;				
				case '\n':	INJECT_CHAR(ptr2, 'n', printed);	
				break;				
				case '\r':	INJECT_CHAR(ptr2, 'r', printed);	
				break;				
				case '\t':	INJECT_CHAR(ptr2, 't', printed);	
				break;				
				default: 
					len=JSON_SPRINTF(ptr2, in_len-printed, "u%04x",token);
					ptr2+=len; 
					printed+=len;	
				break;	/* escape and print */			
			}		
		}	
	}
	if(printed<in_len)	
	{		
		INJECT_CHAR(ptr2, '\"', printed);		
		*ptr2++=0;	
	}	
	
	return printed;
}

uint32_t json_build_int(char *out_buf, uint32_t in_len, uint32_t v)
{
	uint32_t length = 0;

	length = JSON_SPRINTF(out_buf, in_len, "%d", v);

	return length;
}

// double in Qx.y
uint32_t json_build_Qxy(char *out_buf, uint32_t in_len, uint8_t x, uint8_t y, uint32_t v)
{
	uint32_t length = 0;

	//TODO
	return length;
}

uint32_t json_build_bool(char *out_buf, uint32_t in_len, uint8_t v)
{
	uint32_t length = 0;

	if(in_len > 5)
	{
		length = JSON_SPRINTF(out_buf, in_len, v?"true":"false");
	}

	return length;
}

uint32_t json_build_null(char *out_buf, uint32_t in_len)
{
	uint32_t length = 0;
	
	if(in_len > 4)
	{
		length = JSON_SPRINTF(out_buf, in_len, "null");
	}

	return length;
}

// double in xy
uint32_t hue_json_build_xy(char *out_buf, uint32_t in_len, uint16_t v)
{
	uint32_t length = 0;

	if(in_len > 2+5)
	{
		out_buf[0] = '0';
		out_buf[1] = '.';
		length = 2 + JSON_SPRINTF(&out_buf[2], in_len-2, "%d", (v*10000)>>16);
	}

	return length;
}

uint32_t hue_json_build_light_state(char *out_buf, uint32_t in_len, hue_light_t *light)
{
	uint32_t offset = 0;
	
	// light state is an object
	offset += json_build_object_start(&out_buf[offset], in_len-offset);

	// build on
	offset += json_build_string(&out_buf[offset], in_len-offset, "on");
	offset += json_build_char(&out_buf[offset], in_len-offset, ':');
	offset += json_build_bool(&out_buf[offset], in_len-offset, light->on);
	offset += json_build_char(&out_buf[offset], in_len-offset, ',');

	// build bri
	offset += json_build_string(&out_buf[offset], in_len-offset, "bri");
	offset += json_build_char(&out_buf[offset], in_len-offset, ':');
	offset += json_build_int(&out_buf[offset], in_len-offset, light->bri);
	offset += json_build_char(&out_buf[offset], in_len-offset, ',');

	// build hue
	offset += json_build_string(&out_buf[offset], in_len-offset, "hue");
	offset += json_build_char(&out_buf[offset], in_len-offset, ':');
	offset += json_build_int(&out_buf[offset], in_len-offset, light->hue);
	offset += json_build_char(&out_buf[offset], in_len-offset, ',');

	// build sat
	offset += json_build_string(&out_buf[offset], in_len-offset, "sat");
	offset += json_build_char(&out_buf[offset], in_len-offset, ':');
	offset += json_build_int(&out_buf[offset], in_len-offset, light->sat);
	offset += json_build_char(&out_buf[offset], in_len-offset, ',');

	// build xy
	offset += json_build_string(&out_buf[offset], in_len-offset, "xy");
	offset += json_build_char(&out_buf[offset], in_len-offset, ':');
	offset += json_build_array_start(&out_buf[offset], in_len-offset);			//xy is an array
	offset += hue_json_build_xy(&out_buf[offset], in_len-offset, light->x);
	offset += json_build_char(&out_buf[offset], in_len-offset, ',');
	offset += hue_json_build_xy(&out_buf[offset], in_len-offset, light->y);
	offset += json_build_array_end(&out_buf[offset], in_len-offset);
	offset += json_build_char(&out_buf[offset], in_len-offset, ',');

	// build ct
	offset += json_build_string(&out_buf[offset], in_len-offset, "ct");
	offset += json_build_char(&out_buf[offset], in_len-offset, ':');
	offset += json_build_int(&out_buf[offset], in_len-offset, light->ct);
	offset += json_build_char(&out_buf[offset], in_len-offset, ',');

	//build alert
	offset += json_build_string(&out_buf[offset], in_len-offset, "alert");
	offset += json_build_char(&out_buf[offset], in_len-offset, ':');
	offset += json_build_string(&out_buf[offset], in_len-offset, strHueAlert[light->alert]);
	offset += json_build_char(&out_buf[offset], in_len-offset, ',');

	// build effect
	offset += json_build_string(&out_buf[offset], in_len-offset, "effect");
	offset += json_build_char(&out_buf[offset], in_len-offset, ':');
	offset += json_build_string(&out_buf[offset], in_len-offset, strHueEffect[light->effect]);
	offset += json_build_char(&out_buf[offset], in_len-offset, ',');

	// build colormode
	offset += json_build_string(&out_buf[offset], in_len-offset, "colormode");
	offset += json_build_char(&out_buf[offset], in_len-offset, ':');
	offset += json_build_string(&out_buf[offset], in_len-offset, strHueColormode[light->colormode]);
	offset += json_build_char(&out_buf[offset], in_len-offset, ',');

	// build reachable
	offset += json_build_string(&out_buf[offset], in_len-offset, "on");
	offset += json_build_char(&out_buf[offset], in_len-offset, ':');
	offset += json_build_bool(&out_buf[offset], in_len-offset, light->reachable);
	
	// end of light state object
	offset += json_build_object_end(&out_buf[offset], in_len-offset);

	return offset;
}

uint32_t hue_json_build_light(char *out_buf, uint32_t in_len, hue_light_t *light)
{
	uint32_t offset = 0;
	
	// light is an object
	offset += json_build_object_start(&out_buf[offset], in_len-offset);

	// build state
	offset += json_build_string(&out_buf[offset], in_len-offset, "state");
	offset += json_build_char(&out_buf[offset], in_len-offset, ':');
	offset += hue_json_build_light_state(&out_buf[offset], in_len-offset, light);
	offset += json_build_char(&out_buf[offset], in_len-offset, ',');

	// build type
	offset += json_build_string(&out_buf[offset], in_len-offset, "type");
	offset += json_build_char(&out_buf[offset], in_len-offset, ':');
	offset += json_build_string(&out_buf[offset], in_len-offset, strHueType[light->type]);
	offset += json_build_char(&out_buf[offset], in_len-offset, ',');

	// build name
	offset += json_build_string(&out_buf[offset], in_len-offset, "name");
	offset += json_build_char(&out_buf[offset], in_len-offset, ':');
	offset += json_build_string(&out_buf[offset], in_len-offset, (const char *)light->name);
	offset += json_build_char(&out_buf[offset], in_len-offset, ',');

	// build modelid
	offset += json_build_string(&out_buf[offset], in_len-offset, "modelid");
	offset += json_build_char(&out_buf[offset], in_len-offset, ':');
	offset += json_build_string(&out_buf[offset], in_len-offset, (const char *)light->modelId);
	offset += json_build_char(&out_buf[offset], in_len-offset, ',');

	// build swversion
	offset += json_build_string(&out_buf[offset], in_len-offset, "swversion");
	offset += json_build_char(&out_buf[offset], in_len-offset, ':');
	offset += json_build_string(&out_buf[offset], in_len-offset, (const char *)light->swversion);
	offset += json_build_char(&out_buf[offset], in_len-offset, ',');

	// build pointsymbol
	offset += json_build_string(&out_buf[offset], in_len-offset, "pointsymbol");
	offset += json_build_char(&out_buf[offset], in_len-offset, ':');
	offset += json_build_char(&out_buf[offset], in_len-offset, '{');
	offset += json_build_char(&out_buf[offset], in_len-offset, '}');

	// end of light object
	offset += json_build_object_end(&out_buf[offset], in_len-offset);

	return offset;
}

uint32_t hue_json_build_all_lights(char *out_buf, uint32_t in_len)
{
	uint16_t n;
	uint32_t offset;
	char idString[10];

	offset = 0;
	offset += json_build_object_start(&out_buf[offset], in_len-offset);
	
	for(n=0; n<gNumHueLight; n++)
	{
		if(n!=0)
		{
			// build ','
			offset += json_build_char(&out_buf[offset], in_len-offset, ',');
		}

		siprintf(idString, 10, "%d", gHueLight[n].id);
		offset += json_build_string(&out_buf[offset], in_len-offset, idString);

		offset += json_build_char(&out_buf[offset], in_len-offset, ':');
		
		offset += hue_json_build_light(&out_buf[offset], in_len-offset, &gHueLight[n]);
	}

	offset += json_build_object_end(&out_buf[offset], in_len-offset);
	return offset;
}

uint32_t hue_json_build_create_user(char *out_buf, uint32_t in_len, uint8_t success, uint8_t *username)
{
	uint32_t offset;

	offset = 0;
	offset += json_build_array_start(&out_buf[offset], in_len-offset);
	offset += json_build_object_start(&out_buf[offset], in_len-offset);

	offset += json_build_string(out_buf, in_len-offset, success?strHueSuccess:strHueFailure);
	offset += json_build_char(out_buf, in_len-offset, ':');

	// username object
	offset += json_build_object_start(&out_buf[offset], in_len-offset);
	offset += json_build_string(out_buf, in_len-offset, "username");
	offset += json_build_char(out_buf, in_len-offset, ':');
	offset += json_build_string(out_buf, in_len-offset, (const char *)username);
	offset += json_build_object_end(&out_buf[offset], in_len-offset);
		
	offset += json_build_object_end(&out_buf[offset], in_len-offset);
	offset += json_build_array_end(&out_buf[offset], in_len-offset);
	return offset;
}

