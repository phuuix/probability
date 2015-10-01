/*
 * hue.c
 * The below HUE APIs are supported and each API has one processing function.
 * The processing function is called directly by http task.
========================================
1. Lights API
1.1. Get all lights
1.2. Get new lights
1.3. Search for new lights
1.4. Get light attributes and state
1.5. Set light attributes (rename)
1.6. Set light state
1.7. Delete lights
2. Groups API
2.1. Get all groups
2.2. Create group
2.3. Get group attributes
2.4. Set group attributes
2.5. Set group state
2.6. Delete Group
7 Configuration API
7.1. Create user
7.2. Get configuration
7.3. Modify configuration
7.4. Delete user from whitelist
7.5. Get full state (datastore)
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lwip/netif.h"

#include "ipc.h"
#include "siprintf.h"
#include "uprintf.h"
#include "clock.h"
#include "hue.h"
#include "hue_json_builder.h"

hue_light_t gHueLight[HUE_MAX_LIGHTS];
hue_user_t gHueUser[HUE_MAX_USERS];
uint16_t gNumHueLight;
uint16_t gNumHueUser;

extern mbox_t g_hue_mbox;

static char hue_tmpstr[64];

char *hue_get_ip_string()
{
    uint32_t ip;
    uint8_t *pIp;
    char *outstr = hue_tmpstr;

    *outstr = '\0';
    if(netif_default)
    {
        /* build location */
        ip = (netif_default->ip_addr.addr);
        pIp = (uint8_t *)&ip;
        siprintf(outstr, 16, "%d.%d.%d.%d", pIp[0], pIp[1], pIp[2], pIp[3]);
    }

    return outstr;
}

char *hue_get_netmask_string()
{
    char *outstr = hue_tmpstr;

    siprintf(outstr, 16, "255.255.255.0");

    return outstr;
}

char *hue_get_mac_string()
{
    uint8_t *pMac;
    char *outstr = hue_tmpstr;

    *outstr = '\0';
    if(netif_default)
    {
        /* build location */
        pMac = (netif_default->hwaddr);
        siprintf(outstr, 64, "%02x:%02x:%02x:%02x:%02x:%02x", pMac[0], pMac[1], pMac[2], pMac[3], pMac[4], pMac[5]);
    }

    return outstr;
}

char *hue_get_gateway_string()
{
    uint32_t ip;
    uint8_t *pIp;
    char *outstr = hue_tmpstr;

    *outstr = '\0';
    if(netif_default)
    {
        /* build location */
        ip = (netif_default->ip_addr.addr);
        pIp = (uint8_t *)&ip;
        siprintf(outstr, 16, "%d.%d.%d.%d", pIp[0], pIp[1], pIp[2], 1);
    }

    return outstr;
}

hue_light_t *hue_find_light_by_id(uint8_t in_light_id)
{
    int i;

    for(i=0; i<gNumHueLight; i++)
    {
        if(in_light_id == gHueLight[i].id)
            break;
    }

    if(i < gNumHueLight)
        return &gHueLight[i];
    else
        return NULL;
}

/*
1. Lights API
1.1. Get all lights
1.2. Get new lights
1.3. Search for new lights
1.4. Get light attributes and state
1.5. Set light attributes (rename)
1.6. Set light state
*/

int process_hue_api_get_all_lights(char *out_responseBuf, uint32_t in_max_resp_size)
{
	return hue_json_build_all_lights(out_responseBuf, in_max_resp_size);
}

int process_hue_api_get_new_lights(char *out_responseBuf, uint32_t in_max_resp_size)
{
	return hue_json_build_new_lights(out_responseBuf, in_max_resp_size);
}

int process_hue_api_search_new_lights(char *out_responseBuf, uint32_t in_max_resp_size)
{
    hue_mail_t huemail;

    // start a new search
    huemail.search_new_lights.cmd = HUE_MAIL_CMD_SEACH_NEW_LIGHTS;
    huemail.search_new_lights.username = NULL;
    huemail.search_new_lights.unused = NULL;
    mbox_post(&g_hue_mbox, (uint32_t *)&huemail);

	return hue_json_build_search_new_lights(out_responseBuf, in_max_resp_size);
}

int process_hue_api_get_light_attr_state(uint16_t in_light_id, char *out_responseBuf, uint32_t in_max_resp_size)
{
	hue_light_t *light;
	
	// find the light
    light = hue_find_light_by_id(in_light_id);
    if(light == NULL)
        return hue_json_build_error(out_responseBuf, in_max_resp_size, HUE_ERROR_ID_003);
	
	return hue_json_build_light(out_responseBuf, in_max_resp_size, light);
}

int process_hue_api_rename_light(uint16_t in_light_id, char *newname, char *out_responseBuf, uint32_t in_max_resp_size)
{
	hue_light_t *light;
	
	// find the light
    light = hue_find_light_by_id(in_light_id);
    if(light == NULL)
        return hue_json_build_error(out_responseBuf, in_max_resp_size, HUE_ERROR_ID_003);
	
    strncpy((char *)light->name, newname, 32);
	return hue_json_build_rename_light(out_responseBuf, in_max_resp_size, light);
}

int process_hue_api_set_light_state(uint16_t in_light_id, hue_light_t *in_light, uint32_t in_bitmap, char *out_responseBuf, uint32_t in_max_resp_size)
{
	hue_light_t *light;
    hue_mail_t huemail;
	
	// find the light
    light = hue_find_light_by_id(in_light_id);
    if(light == NULL)
        return hue_json_build_error(out_responseBuf, in_max_resp_size, HUE_ERROR_ID_003);

    // issue a command to set light state
    huemail.set_one_light.cmd = HUE_MAIL_CMD_SET_ONE_LIGHT;
    huemail.set_one_light.light_id = in_light_id;
    huemail.set_one_light.flags = in_bitmap;
    huemail.set_one_light.username = NULL;
    huemail.set_one_light.light = in_light;
    mbox_post(&g_hue_mbox, (uint32_t *)&huemail);

	return hue_json_build_set_light_state(out_responseBuf, in_max_resp_size, in_light, in_bitmap);
}

/*
2. Groups API
2.1. Get all groups
2.2. Create group
2.3. Get group attributes
2.4. Set group attributes
2.5. Set group state
2.6. Delete Group
*/
int process_hue_api_get_all_groups(char *responseBuf, uint32_t size)
{
	return hue_json_build_all_groups(responseBuf, size, &g_hue);
}

int process_hue_api_create_group(char *responseBuf, uint32_t size)
{
	//TODO
	return 0;
}

int process_hue_api_get_group_attr(uint32_t group_id, char *responseBuf, uint32_t size)
{
	return hue_json_build_group_attr(responseBuf, size, group_id);
}

int process_hue_api_set_group_attr(char *responseBuf, uint32_t size)
{
	//TODO
	return 0;
}

int process_hue_api_set_group_state(char *responseBuf, uint32_t size)
{
	//TODO
	return 0;
}

int process_hue_api_delete_group(char *responseBuf, uint32_t size)
{
	//TODO
	return 0;
}

/*
7 Configuration API
7.1. Create user
7.2. Get configuration
7.3. Modify configuration
7.4. Delete user from whitelist
7.5. Get full state (datastore)
*/
int process_hue_api_create_user(char *devType, char *userName, char *responseBuf, uint32_t size)
{
	uint16_t i, success = 0;
	hue_user_t *user;
	
    if(userName != NULL)
    {
    	for(i=0; i<gNumHueUser; i++)
    	{
    		if(strcmp(userName, (const char *)gHueUser[i].name) == 0)
    		{
    			success = 1;
    			break;
    		}
    	}

    	if(i<gNumHueUser)
    	{
    		//If the requested username already exists then the response will report a success.
    		uprintf_default("Hue user already existed: %s\n", userName);
    		return hue_json_build_create_user(responseBuf, size, success, (uint8_t *)userName);
    	}
    }

	if(gNumHueUser < HUE_MAX_USERS)
	{
		success = 1;
		user = &gHueUser[gNumHueUser];
		gNumHueUser ++;
		user->id = gNumHueUser;
        if(devType == NULL)
            strcpy((char *)user->devType, "unknownDev");
        else
		    strncpy((char *)user->devType, devType, 40);
        if(userName == NULL)
            siprintf((char *)user->name, 40, "Huser%d", user->id);
        else
		    strncpy((char *)user->name, userName, 40);
		user->createDate = time(NULL);
		user->lastUseDate = user->createDate;

        return hue_json_build_create_user(responseBuf, size, success, (uint8_t *)user->name);
	}

	return hue_json_build_create_user(responseBuf, size, success, (uint8_t *)userName);
}

int process_hue_api_get_configuration(char *responseBuf, uint32_t size)
{
	return hue_json_build_get_config(responseBuf, size, &g_hue);
}

int process_hue_api_modify_configuration(char *responseBuf, uint32_t size)
{
	// TODO
	return 0;
}

int process_hue_api_delete_user(char *responseBuf, uint32_t size)
{
	//TODO
	return 0;
}

int process_hue_api_get_full_state(char *responseBuf, uint32_t size)
{
	return hue_json_build_fullstate(responseBuf, size, &g_hue);
}

void hue_data_init(hue_t *hue)
{
	uint8_t myname[] = "HueLamp1";
	uint8_t mymodelId[] = "LCT001";
	uint8_t myswversion[] = "01150905";
	
    strcpy(hue->name, "HUE0");
    strcpy(hue->apiversion, "1.3.0");
    strcpy(hue->swversion, "0.9.0");

    /* create two lights for debug */
    gNumHueLight = 1;

	gHueLight[0].id = 1;
	gHueLight[0].on = 0;
	gHueLight[0].bri = 100;
	gHueLight[0].hue = 13088;
	gHueLight[0].sat = 100;
	gHueLight[0].x = 32768;
	gHueLight[0].y = 32768;
	gHueLight[0].ct = 500;
	gHueLight[0].alert = HUE_LIGHT_ALERT_NONE;
	gHueLight[0].effect = HUE_LIGHT_EFFECT_NONE;
	gHueLight[0].colormode = HUE_LIGHT_COLORMODE_HS;
	gHueLight[0].reachable = 1;
	gHueLight[0].type = 0;
	strcpy((char *)gHueLight[0].name, (char *)myname);
	strcpy((char *)gHueLight[0].modelId, (char *)mymodelId);
	strcpy((char *)gHueLight[0].swversion, (char *)myswversion);
    gHueLight[0].ep_info.nwkAddr = 0x02;
    gHueLight[0].ep_info.endpoint = 0x0b;

    gHueLight[1].id = 2;
	gHueLight[1].on = 0;
	gHueLight[1].bri = 100;
	gHueLight[1].hue = 13088;
	gHueLight[1].sat = 100;
	gHueLight[1].x = 32768;
	gHueLight[1].y = 32768;
	gHueLight[1].ct = 500;
	gHueLight[1].alert = HUE_LIGHT_ALERT_NONE;
	gHueLight[1].effect = HUE_LIGHT_EFFECT_NONE;
	gHueLight[1].colormode = HUE_LIGHT_COLORMODE_HS;
	gHueLight[1].reachable = 0;
	gHueLight[1].type = 0;
	strcpy((char *)gHueLight[1].name, (char *)myname);
    gHueLight[1].name[7] = '2';
	strcpy((char *)gHueLight[1].modelId, (char *)mymodelId);
	strcpy((char *)gHueLight[1].swversion, (char *)myswversion);
    gHueLight[1].ep_info.nwkAddr = 0x03;
    gHueLight[1].ep_info.endpoint = 0x0b;

    /* create one user (in whitelist) for debug */
    gNumHueUser = 1;
    gHueUser[0].id = 0;
    strcpy((char *)gHueUser[0].name, "Huser1");
    strcpy((char *)gHueUser[0].devType, "Hue#Android");
    gHueUser[0].createDate = time(NULL);
    gHueUser[0].lastUseDate = time(NULL);
}

void hue_localtime(uint32_t t, struct hue_tm *tm)
{
    memset(tm, 0, sizeof(struct hue_tm));

    tm->tm_yday = t/(3600*24);
    t -= tm->tm_yday * 3600*24;

    tm->tm_hour = t/3600;
    t -= tm->tm_hour*3600;
    tm->tm_min = t/60;
    tm->tm_sec = t - tm->tm_min*60;
}

