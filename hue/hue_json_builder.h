/* hue_json_builder.h */

#ifndef _HUE_JSON_BUILDER_H_
#define _HUE_JSON_BUILDER_H_

uint32_t hue_json_build_all_lights(char *out_buf, uint32_t in_len);
uint32_t hue_json_build_light(char * out_buf,uint32_t in_len,hue_light_t * light);
uint32_t hue_json_build_create_user(char *out_buf, uint32_t in_len, uint8_t success, uint8_t *username);

#endif //_HUE_JSON_BUILDER_H_

