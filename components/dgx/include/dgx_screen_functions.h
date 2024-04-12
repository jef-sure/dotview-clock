/*
 * dgx_screen_functions.h
 *
 *  Created on: 16.12.2022
 *      Author: KYMJ
 */

#ifndef _DGX_INCLUDE_DGX_SCREEN_FUNCTIONS_H_
#define _DGX_INCLUDE_DGX_SCREEN_FUNCTIONS_H_

#include "dgx_screen.h"

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

void dgx_scr_init_slow_bus_optimized_funcs(dgx_screen_t *scr);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif

#endif /* _DGX_INCLUDE_DGX_SCREEN_FUNCTIONS_H_ */
