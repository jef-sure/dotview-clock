/*
 * dgx_spi_esp32.h
 *
 *  Created on: 28.10.2022
 *      Author: KYMJ
 */

#ifndef _DGX_INCLUDE_BUS_DGX_I2C_ESP32_H_
#define _DGX_INCLUDE_BUS_DGX_I2C_ESP32_H_

#include "bus/dgx_bus_protocols.h"

#include "driver/i2c.h"
#include "driver/gpio.h"

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

enum {
    DGX_I2C_AREA_FUNC_STD = 0/* default */, //
};

dgx_bus_protocols_t* dgx_i2c_init(
        i2c_port_t i2c_num, //
        uint8_t i2c_address,//
        gpio_num_t sda,//
        gpio_num_t sclk,//
        gpio_num_t rst,//
        int clock_speed_hz//
);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif

#endif /* _DGX_INCLUDE_BUS_DGX_I2C_ESP32_H_ */
