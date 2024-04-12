/*
 * dgx_screen_with_bus.h
 *
 *  Created on: 02.01.2023
 *      Author: KYMJ
 */

#ifndef COMPONENTS_DGX_INCLUDE_DGX_SCREEN_WITH_BUS_H_
#define COMPONENTS_DGX_INCLUDE_DGX_SCREEN_WITH_BUS_H_

#include "dgx_screen.h"
#include "bus/dgx_bus_protocols.h"

typedef struct _dgx_screen_with_bus_t {
    dgx_screen_t scr;
    dgx_bus_protocols_t *bus;
} dgx_screen_with_bus_t;

#endif /* COMPONENTS_DGX_INCLUDE_DGX_SCREEN_WITH_BUS_H_ */
