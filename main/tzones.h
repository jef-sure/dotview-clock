/*
 * tzones.h
 *
 *  Created on: Mar 26, 2021
 *      Author: anton
 */

#ifndef _TZONES_H_
#define _TZONES_H_
#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

#include <stdbool.h>
#include <stddef.h>

typedef struct TZName_ {
    const char *name, *tzname;
} TZName_t;

const TZName_t *findTZ(const char *name);
const TZName_t* findTZbyValue(const char *tzname);
bool setupTZ(const char *name);
extern const TZName_t TimeZones[];
extern const size_t TimeZonesNumer;

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif
#endif /* _TZONES_H_ */
