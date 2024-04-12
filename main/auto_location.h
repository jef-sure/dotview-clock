#ifndef _AUTO_LOCATION_H_
#define _AUTO_LOCATION_H_

#include "str.h"

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

typedef struct {
    str_t *country;
    str_t *countryCode;
    str_t *region;
    str_t *regionName;
    str_t *city;
    str_t *zip;
    str_t *timezone;
    str_t *isp;
    str_t *ip;
    str_t *location;
    float lat, lon;
} WLocation_t;

WLocation_t auto_location_ipinfo(const char *ipinfo_token);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif

#endif /* _AUTO_LOCATION_H_ */
