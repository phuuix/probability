/* zll_controller.h */

#ifndef _ZLL_CONTROLLER_H_
#define _ZLL_CONTROLLER_H_

#define TZLL_STACK_SIZE 0x400
#define TZLL_PRIORITY 5

#define HUE_MBOX_SIZE 16
#define ZLL_MBOX_SIZE 16


/* timeout before response is back: 500ms */
#define ZLL_RESP_DEFAULT_TIMEOUT 50

hue_t *zllctrl_get_hue();

int zllctrl_process_soc_message(hue_t *hue, uint8_t *data, uint16_t length);
int zllctrl_on_get_light_sat(uint16_t nwkAddr, uint8_t endpoint, uint8_t sat);
int zllctrl_on_get_light_hue(uint16_t nwkAddr, uint8_t endpoint, uint8_t hue);
int zllctrl_on_get_light_level(uint16_t nwkAddr, uint8_t endpoint, uint8_t level);
int zllctrl_on_get_light_on_off(uint16_t nwkAddr, uint8_t endpoint, uint8_t state);
uint8_t zllctrl_process_touchlink_indication(epInfo_t *epInfo);
uint8_t zllctrl_process_newdev_indication(epInfo_t *epInfo);
hue_light_t *zllctrl_create_light(epInfo_t *epInfo);
hue_light_t *zllctrl_find_light_by_addr(uint16_t nwkAddr, uint8_t endpoint);

#endif
