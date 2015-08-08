/* zll_controller.h */

#ifndef _ZLL_CONTROLLER_H_
#define _ZLL_CONTROLLER_H_

#define TZLL_STACK_SIZE 0x400
#define TZLL_PRIORITY 5

#define HUE_MBOX_SIZE 16
#define ZLL_MBOX_SIZE 16

/* event type definition */
#define ZLL_EVENT_JSON 1
#define ZLL_EVENT_CONSOLE 2
#define ZLL_EVENT_SOC 3

/* timeout before response is back: 500ms */
#define ZLL_RESP_DEFAULT_TIMEOUT 50

hue_t *zllctrl_get_hue();

int zllctrl_process_soc_message(hue_t *hue, uint8_t *data, uint16_t length);
int zllctrl_on_get_light_sat(uint16_t nwkAddr, uint8_t endpoint, uint8_t sat);
int zllctrl_on_get_light_hue(uint16_t nwkAddr, uint8_t endpoint, uint8_t hue);
int zllctrl_on_get_light_level(uint16_t nwkAddr, uint8_t endpoint, uint8_t level);
int zllctrl_on_get_light_on_off(uint16_t nwkAddr, uint8_t endpoint, uint8_t state);

#endif
