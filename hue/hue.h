/* hue.h */

#ifndef _HUE_H_
#define _HUE_H_

#include <inttypes.h>
#include "zllSocCmd.h"

#define UPRINT_BLK_HUE 6

/* HUE error ID */
#define HUE_ERROR_ID_001 001        //unauthorized user
#define HUE_ERROR_ID_002 002        //body contains invalid JSON
#define HUE_ERROR_ID_003 003        //resource, <resource>, not available
#define HUE_ERROR_ID_004 004        //method, <method_name>, not available for resource, <resource>
#define HUE_ERROR_ID_005 005        //missing parameters in body
#define HUE_ERROR_ID_006 006        //parameter, <parameter>, not available
#define HUE_ERROR_ID_007 007        //invalid value, <value>, for parameter, <parameter>
#define HUE_ERROR_ID_008 008        //parameter, <parameter>, is not modifiable
#define HUE_ERROR_ID_011 011        //too many items in list
#define HUE_ERROR_ID_012 012        //Portal connection required
#define HUE_ERROR_ID_101 101        //link button not pressed
#define HUE_ERROR_ID_110 110        //DHCP cannot be disabled
#define HUE_ERROR_ID_111 111        //Invalid updatestate
#define HUE_ERROR_ID_201 201        //parameter, <parameter>, is not modifiable. Device is set to off.
#define HUE_ERROR_ID_301 301        //group could not be created. Group table is full.
#define HUE_ERROR_ID_302 302        //device, <id>, could not be added to group. Device's group table is full.
#define HUE_ERROR_ID_304 304        //device, <id>, could not be added to the scene. Device is unreachable.
#define HUE_ERROR_ID_305 305        //It is not allowed to update or delete group of this type
#define HUE_ERROR_ID_401 401        //scene could not be created. Scene creation in progress.
#define HUE_ERROR_ID_402 402        //Scene could not be created. Scene buffer in bridge full
#define HUE_ERROR_ID_501 501        //No allowed to create sensor type
#define HUE_ERROR_ID_502 502        //Sensor list is full
#define HUE_ERROR_ID_601 601        //Rule engine full
#define HUE_ERROR_ID_607 607        //Condition error
#define HUE_ERROR_ID_608 608        //Action error
#define HUE_ERROR_ID_609 609        //Unable to activate
#define HUE_ERROR_ID_701 701        //Schedule list is full
#define HUE_ERROR_ID_702 702        //Schedule time-zone not valid
#define HUE_ERROR_ID_703 703        //Schedule cannot set time and local time
#define HUE_ERROR_ID_704 704        //Cannot create schedule
#define HUE_ERROR_ID_705 705        //Cannot enable schedule, time is in the past
#define HUE_ERROR_ID_901 901        //Internal error, <error code>

/* state bits */
#define HUE_STATE_BIT_ON    0
#define HUE_STATE_BIT_BRI   1
#define HUE_STATE_BIT_HUE   2
#define HUE_STATE_BIT_SAT   3
#define HUE_STATE_BIT_XY    4
#define HUE_STATE_BIT_CT    5
#define HUE_STATE_BIT_ALERT 6
#define HUE_STATE_BIT_EFFECT    7
#define HUE_STATE_BIT_TRANSTIME 8
#define HUE_STATE_BIT_BRIINC    9
#define HUE_STATE_BIT_HUEINC    10
#define HUE_STATE_BIT_CTINC     11
#define HUE_STATE_BIT_XYINC     12

typedef struct zigbee_addr
{
    uint16_t network_addr;
    uint8_t endpoint;
}zigbee_addr_t;

/*
The alert effect, which is a temporary change to the bulb's state. This can take one of the following values:
"none" - The light is not performing an alert effect.
"select" - The light is performing one breathe cycle.
"lselect" - The light is performing breathe cycles for 30 seconds or until an "alert": "none" command is received.
*/
#define HUE_LIGHT_ALERT_NONE 0
#define HUE_LIGHT_ALERT_SELECT 1
#define HUE_LIGHT_ALERT_LSELECT 2

/*
The dynamic effect of the light, can either be "none" or "colorloop".
If set to colorloop, the light will cycle through all hues using the current brightness and saturation settings.
*/
#define HUE_LIGHT_EFFECT_NONE 0
#define HUE_LIGHT_EFFECT_COLORLOOP 1

/*
Indicates the color mode in which the light is working, this is the last command type it received. 
Values are "hs" for Hue and Saturation, "xy" for XY and "ct" for Color Temperature. 
This parameter is only present when the light supports at least one of the values.
*/
#define HUE_LIGHT_COLORMODE_NONE 0
#define HUE_LIGHT_COLORMODE_HS 1
#define HUE_LIGHT_COLORMODE_XY 2
#define HUE_LIGHT_COLORMODE_CT 3

typedef struct hue_light
{
    epInfo_t ep_info;

	uint8_t id;
	uint8_t on:1;			//On/Off state of the light
	uint8_t reachable:1;	//Indicates if a light can be reached by the bridge.
/*
The alert effect, which is a temporary change to the bulb's state. This can take one of the following values:
"none" - The light is not performing an alert effect.
"select" - The light is performing one breathe cycle.
"lselect" - The light is performing breathe cycles for 30 seconds or until an "alert": "none" command is received.
*/
	uint8_t alert:2;
/*
The dynamic effect of the light, can either be "none" or "colorloop".
If set to colorloop, the light will cycle through all hues using the current brightness and saturation settings.
*/
	uint8_t effect:1;
/*
Indicates the color mode in which the light is working, this is the last command type it received. 
Values are "hs" for Hue and Saturation, "xy" for XY and "ct" for Color Temperature. 
This parameter is only present when the light supports at least one of the values.
*/
	uint8_t colormode:2;
	
	uint8_t bri;			//Brightness of the light. Note a brightness of 0 is not off
	uint8_t sat;			//Saturation of the light. 255 is the most saturated (colored) and 0 is the least saturated (white).
	uint16_t hue;			//Hue of the light. This is a wrapping value between 0 and 65535. Both 0 and 65535 are red, 25500 is green and 46920 is blue.
	uint16_t ct;			//The Mired Color temperature?of the light. 2012 connected lights are capable of 153 (6500K) to 500 (2000K)
	uint16_t x;				//Q0.15, The x and y coordinates of a color in CIE color space.
	uint16_t y;				//Q0.15
    uint16_t transitiontime;//in 100ms

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

    char name[16];
    char apiversion[16];
    char swversion[16];

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

struct hue_tm 
{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_yday;
};

/* hue mail macros */
#define HUE_MAIL_CMD_GET_ALL_LIGHTS   0
#define HUE_MAIL_CMD_GET_NEW_LIGHTS   1
#define HUE_MAIL_CMD_SEACH_NEW_LIGHTS 2
#define HUE_MAIL_CMD_GET_ONE_LIGHT    3
#define HUE_MAIL_CMD_RENAME_LIGHT     4
#define HUE_MAIL_CMD_SET_ONE_LIGHT    5
#define HUE_MAIL_CMD_DELETE_LIGHT     6
#define HUE_MAIL_CMD_CONSOLE          64
#define HUE_MAIL_CMD_SSDP             128

/* hue light state bitmap */
#define HUE_LIGHT_STATE_ON            0
#define HUE_LIGHT_STATE_BRI           1
#define HUE_LIGHT_STATE_HUE           2
#define HUE_LIGHT_STATE_SAT           3
#define HUE_LIGHT_STATE_XY            4
#define HUE_LIGHT_STATE_CT            5
#define HUE_LIGHT_STATE_ALERT         6
#define HUE_LIGHT_STATE_EFFECT        7
#define HUE_LIGHT_STATE_TIME          8
#define HUE_LIGHT_STATE_BRIINC        9
#define HUE_LIGHT_STATE_HUEINC        10
#define HUE_LIGHT_STATE_SATINC        11
#define HUE_LIGHT_STATE_XYINC         12
#define HUE_LIGHT_STATE_CTINC         13

typedef union hue_mail_s
{
    struct // hue_mail_search_new_lights
    {
        uint8_t cmd;
        uint8_t filler1;
        uint16_t filler2;
        uint8_t *username;
        uint8_t *unused;        // later will be a device ID list
    } search_new_lights;

    struct // hue_mail_set_one_light
    {
        uint8_t cmd;
        uint8_t light_id;
        uint16_t flags;          // flags indicate which field will be set
        uint8_t *username;
        hue_light_t *light;
    } set_one_light;

    struct // console command
    {
        uint8_t cmd;
        uint8_t filler;
        uint16_t length;
        uint8_t *data;
    } console;

    struct // ssdp command 
    {
        uint8_t cmd;
        uint8_t filler;
        uint16_t interval;
        uint16_t port;
        uint32_t ipaddr;
    } ssdp;

    struct //
    {
        uint8_t cmd;
        uint8_t filler1;
        uint16_t filler2;
        uint8_t *username;
        uint8_t *data;
    } common;
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
extern hue_t g_hue;

char *hue_get_ip_string();
char *hue_get_netmask_string();
char *hue_get_mac_string();
char *hue_get_gateway_string();

hue_light_t *hue_find_light_by_id(uint8_t in_light_id);
int process_hue_api_get_all_lights(char *out_responseBuf, uint32_t in_max_resp_size);
int process_hue_api_get_new_lights(char *out_responseBuf, uint32_t in_max_resp_size);
int process_hue_api_search_new_lights(char *out_responseBuf, uint32_t in_max_resp_size);
int process_hue_api_get_light_attr_state(uint16_t in_light_id, char *out_responseBuf, uint32_t in_max_resp_size);
int process_hue_api_rename_light(uint16_t in_light_id, char *newname, char *out_responseBuf, uint32_t in_max_resp_size);
int process_hue_api_set_light_state(uint16_t in_light_id, hue_light_t *in_light, uint32_t in_bitmap, char *out_responseBuf, uint32_t in_max_resp_size);
int process_hue_api_create_user(char *devType, char *userName, char *responseBuf, uint32_t size);
int process_hue_api_get_configuration(char *responseBuf, uint32_t size);
int process_hue_api_get_full_state(char *responseBuf, uint32_t size);
int process_hue_api_get_group_attr(uint32_t group_id, char *responseBuf, uint32_t size);
int process_hue_api_get_all_groups(char *responseBuf, uint32_t size);


void hue_localtime(uint32_t t, struct hue_tm *tm);
void hue_data_init(hue_t *hue);

#endif //_HUE_H_
