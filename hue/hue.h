/* hue.h */

#ifndef _HUE_H_
#define _HUE_H_

#include <inttypes.h>

#define UPRINT_BLK_HUE 6

typedef struct zigbee_addr
{
    uint16_t network_addr;
    uint8_t endpoint;
}zigbee_addr_t;

typedef struct hue_light
{
    zigbee_addr_t zig_addr; // address of light

	uint8_t id;
	uint8_t on:1;			//On/Off state of the light
	uint8_t reachable:1;	//Indicates if a light can be reached by the bridge.
/*
The alert effect, which is a temporary change to the bulb's state. This can take one of the following values:
"none" - The light is not performing an alert effect.
"select" - The light is performing one breathe cycle.
"lselect" - The light is performing breathe cycles for 30 seconds or until an "alert": "none" command is received.
*/
 #define HUE_LIGHT_ALERT_NONE 0
 #define HUE_LIGHT_ALERT_SELECT 1
 #define HUE_LIGHT_ALERT_LSELECT 2
	uint8_t alert:2;
/*
The dynamic effect of the light, can either be "none" or "colorloop".
If set to colorloop, the light will cycle through all hues using the current brightness and saturation settings.
*/
#define HUE_LIGHT_EFFECT_NONE 0
#define HUE_LIGHT_EFFECT_COLORLOOP 1
	uint8_t effect:1;
/*
Indicates the color mode in which the light is working, this is the last command type it received. 
Values are "hs" for Hue and Saturation, "xy" for XY and "ct" for Color Temperature. 
This parameter is only present when the light supports at least one of the values.
*/
#define HUE_LIGHT_COLORMODE_NONE 0
#define HUE_LIGHT_COLORMODE_HS 1
#define HUE_LIGHT_COLORMODE_XY 2
#define HUE_LIGHT_COLORMODE_CT 3
	uint8_t colormode:2;
	
	uint8_t bri;			//Brightness of the light. Note a brightness of 0 is not off
	uint8_t sat;			//Saturation of the light. 255 is the most saturated (colored) and 0 is the least saturated (white).
	uint16_t hue;			//Hue of the light. This is a wrapping value between 0 and 65535. Both 0 and 65535 are red, 25500 is green and 46920 is blue.
	uint16_t ct;			//The Mired Color temperature?of the light. 2012 connected lights are capable of 153 (6500K) to 500 (2000K)
	uint16_t x;				//Q0.15, The x and y coordinates of a color in CIE color space.
	uint16_t y;				//Q0.15

	uint8_t type;			//A fixed name describing the type of light e.g. "Extended color light"
	uint8_t name[32];		//A unique, editable name given to the light
	uint8_t modelId[6];		//The hardware model of the light
	uint8_t swversion[8];	//An identifier for the software version running on the light
	
}hue_light_t;

typedef struct hue_user
{
	uint8_t id;
	uint8_t name[40];
	uint8_t devType[40];
	uint32_t createDate;
	uint32_t lastUseDate;
}hue_user_t;

enum
{
    HUE_STATE_INIT,
    HUE_STATE_CONNECTED,               // connected to Soc
    HUE_STATE_NETSETUP,                // network is setup
};

#define SOC_MT_CMD_BUF_SIZ 256
typedef struct hue_s
{
	uint16_t state;

    uint8_t transport_rev;              // transport protocol revision
    uint8_t product_id;                 // Product Id
    uint8_t major_rel;                  // Software major release number
    uint8_t minor_rel;                  // Software minor release number
    uint8_t maint_rel;                  // Software maintenance release number

	uint8_t ieee_addr[8];
	uint16_t short_addr;
	uint8_t device_type;
	uint8_t device_state;
	uint8_t num_assoc_dev;
	
    uint8_t socbuf[SOC_MT_CMD_BUF_SIZ];    // store the MT response and indication
}hue_t;

typedef struct hue_mail_s
{
	uint8_t event;
    uint8_t flags;
    uint16_t length;
	uint8_t *data;
}hue_mail_t;

typedef struct zll_mail_s
{
	uint8_t event;
    uint8_t flags;
    uint16_t length;
	uint8_t *data;
}zll_mail_t;

#define HUE_MAX_USERS 8
#define HUE_MAX_LIGHTS 32

extern hue_light_t gHueLight[HUE_MAX_LIGHTS];
extern hue_user_t gHueUser[HUE_MAX_USERS];
extern uint16_t gNumHueLight;
extern uint16_t gNumHueUser;
#endif //_HUE_H_