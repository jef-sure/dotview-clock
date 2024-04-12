#ifndef _NVS_CONFIG_H_
#define _NVS_CONFIG_H_

#include "str.h"

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

str_t *get_nvs_str_key(const char *key);
int8_t get_nvs_tint_key(const char *key);
int64_t get_nvs_bint_key(const char *key);
void set_nvs_str_key(const char *key, str_t *value);
void set_nvs_tint_key(const char *key, int8_t value);
void set_nvs_bint_key(const char *key, int64_t value);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif

#endif /* _NVS_CONFIG_H_ */
