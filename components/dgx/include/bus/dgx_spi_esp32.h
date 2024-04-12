/*
 * dgx_spi_esp32.h
 *
 *  Created on: 28.10.2022
 *      Author: KYMJ
 */

#ifndef _DGX_INCLUDE_BUS_DGX_SPI_ESP32_H_
#define _DGX_INCLUDE_BUS_DGX_SPI_ESP32_H_

#include "bus/dgx_bus_protocols.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "hal/gpio_ll.h"


#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

dgx_bus_protocols_t* dgx_spi_init(          //
    spi_host_device_t host_id,              //
    spi_dma_chan_t dma_chan,                //
    gpio_num_t mosi,                        //
    gpio_num_t miso,                        //
    gpio_num_t sclk,                        //
    gpio_num_t cs,                          //
    gpio_num_t dc,                          //
    int clock_speed_hz,                     //
    uint8_t /* default = 0 */ cpolpha_mode  //
);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif

#endif /* _DGX_INCLUDE_BUS_DGX_SPI_ESP32_H_ */
