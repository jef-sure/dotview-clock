/*
 * dgx_spi_esp32.h
 *
 *  Created on: 28.10.2022
 *      Author: KYMJ
 */

#ifndef COMPONENTS_DGX_INCLUDE_BUS_DGX_P8_ESP32_H_
#define COMPONENTS_DGX_INCLUDE_BUS_DGX_P8_ESP32_H_

#include "bus/dgx_bus_protocols.h"
#include "driver/gpio.h"
#include "hal/gpio_ll.h"
#include "dgx_arch.h"

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

enum {
    DGX_P8_AREA_FUNC_STD = 0/* default */, //
};

void dgx_p8_set_area_func(struct _dgx_bus_protocols_t *_bus, int area_func_style);
dgx_bus_protocols_t* dgx_p8_init(gpio_num_t lcd_d0, gpio_num_t lcd_d1, gpio_num_t lcd_d2, gpio_num_t lcd_d3, gpio_num_t lcd_d4, gpio_num_t lcd_d5,
        gpio_num_t lcd_d6, gpio_num_t lcd_d7, gpio_num_t lcd_wr, gpio_num_t lcd_rd, gpio_num_t cs, gpio_num_t dc);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif

#endif /* COMPONENTS_DGX_INCLUDE_BUS_DGX_P8_ESP32_H_ */
