/*
 * dgx_lcd_init.c
 *
 *  Created on: 16.12.2022
 *      Author: Anton
 */

#include "drivers/dgx_lcd_init.h"
#include "dgx_arch.h"
//#include "esp_log.h"
//
//static const char TAG[] = "DGX LCD INIT";

void dgx_lcd_init(dgx_screen_with_bus_t *scr, const dgx_lcd_init_cmd_t *lcd_init_cmds) {
    dgx_bus_protocols_t *bus = scr->bus;
    uint8_t new_data[1];
    for (uint8_t cidx = 0; lcd_init_cmds[cidx].databytes != 0xff; ++cidx) {
        uint8_t cmd = lcd_init_cmds[cidx].cmd;
        bus->write_command(bus, cmd);
        uint8_t const *data = lcd_init_cmds[cidx].data;
        if (lcd_init_cmds[cidx].adjust_func) {
            new_data[0] = lcd_init_cmds[cidx].adjust_func(scr, &lcd_init_cmds[cidx]);
            data = new_data;
        }
        if (lcd_init_cmds[cidx].databytes) bus->write_data(bus, data, lcd_init_cmds[cidx].databytes * 8);
        if (lcd_init_cmds[cidx].delay) {
            int ms = lcd_init_cmds[cidx].delay == 255 ? 500 : lcd_init_cmds[cidx].delay;
            dgx_delay(ms);
        }
    }
}

