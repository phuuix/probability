/*
 * hue.c
========================================
1. Lights API
1.1. Get all lights
1.2. Get new lights
1.3. Search for new lights
1.4. Get light attributes and state
1.5. Set light attributes (rename)
1.6. Set light state
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

#include "hue.h"

hue_light_t gHueLight[HUE_MAX_LIGHTS];
hue_user_t gHueUser[HUE_MAX_USERS];
uint16_t gNumHueLight;
uint16_t gNumHueUser;

/*
1. Lights API
1.1. Get all lights
1.2. Get new lights
1.3. Search for new lights
1.4. Get light attributes and state
1.5. Set light attributes (rename)
1.6. Set light state
*/
int process_hue_api_get_all_lights(char *responseBuf, uint32_t size)
{
	return hue_json_build_all_lights(responseBuf, size);
}

int process_hue_api_get_new_lights(char *responseBuf, uint32_t size)
{
	//TODO
	return 0;
}

int process_hue_api_search_new_lights(char *responseBuf, uint32_t size)
{
	//TODO
	return 0;
}

int process_hue_api_get_light_attr_state(char *responseBuf, uint32_t size, uint16_t light_id)
{
	hue_light_t *light;
	int i;
	
	// TODO: find the light
	
	return hue_json_build_light(responseBuf, size, light);
}

int process_hue_api_rename_light(char *responseBuf, uint32_t size)
{
	//TODO
	return 0;
}

int process_hue_api_set_light_state(char *responseBuf, uint32_t size)
{
	//TODO
	return 0;
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
	//TODO
	return 0;
}

int process_hue_api_create_group(char *responseBuf, uint32_t size)
{
	//TODO
	return 0;
}

int process_hue_api_get_group_attr(char *responseBuf, uint32_t size)
{
	//TODO
	return 0;
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
	
	for(i=0; i<gNumHueUser; i++)
	{
		if(strcmp(userName, gHueUser[i].name) == 0)
		{
			success = 1;
			break;
		}
	}

	if(i<gNumHueUser)
	{
		//If the requested username already exists then the response will report a success.
		uprintf_default("Hue user already existed: %s\n", userName);
		return hue_json_build_create_user(responseBuf, size, success, userName);
	}

	if(gNumHueUser < HUE_MAX_USERS)
	{
		success = 1;
		user = &gHueUser[gNumHueUser];
		gNumHueUser ++;
		user->id = gNumHueUser;
		strncpy(user->devType, devType, 40);
		strncpy(user->name, userName, 40);
		user->createDate = time(NULL);
		user->lastUseDate = user->createDate;
	}

	return hue_json_build_create_user(responseBuf, size, success, userName);
}

int process_hue_api_get_configuration(char *responseBuf, uint32_t size)
{
	// TODO
	return 0;
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
	//TODO
	return 0;
}

