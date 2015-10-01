/* hue_json_builder.h */

#ifndef _HUE_JSON_BUILDER_H_
#define _HUE_JSON_BUILDER_H_

uint32_t hue_json_build_all_lights(char *out_buf, uint32_t in_len);
uint32_t hue_json_build_light(char * out_buf,uint32_t in_len,hue_light_t * light);
uint32_t hue_json_build_new_lights(char *out_buf, uint32_t in_len);
uint32_t hue_json_build_search_new_lights(char *out_buf, uint32_t in_len);
uint32_t hue_json_build_rename_light(char *out_buf, uint32_t in_len, hue_light_t *light);
uint32_t hue_json_build_set_light_state(char *out_buf, uint32_t in_len, hue_light_t *in_light, uint32_t in_bitmap);
uint32_t hue_json_build_error(char *out_buf, uint32_t in_len, uint32_t in_error_id);
uint32_t hue_json_build_fullstate(char *out_buf, uint32_t in_len, hue_t *hue);
uint32_t hue_json_build_get_config(char *out_buf, uint32_t in_len, hue_t *hue);
uint32_t hue_json_build_create_user(char *out_buf, uint32_t in_len, uint8_t success, uint8_t *username);
uint32_t hue_json_build_all_groups(char *out_buf, uint32_t in_len, hue_t *hue);
uint32_t hue_json_build_all_schedules(char *out_buf, uint32_t in_len, hue_t *hue);
uint32_t hue_json_build_all_scenes(char *out_buf, uint32_t in_len, hue_t *hue);
uint32_t hue_json_build_all_rules(char *out_buf, uint32_t in_len, hue_t *hue);
uint32_t hue_json_build_all_sensors(char *out_buf, uint32_t in_len, hue_t *hue);
uint32_t hue_json_build_group_attr(char *out_buf, uint32_t in_len, uint32_t group_id);


#endif //_HUE_JSON_BUILDER_H_

