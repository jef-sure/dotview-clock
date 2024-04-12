/*
 * dgx_lcd_init.h
 *
 *  Created on: 16.12.2022
 *      Author: KYMJ
 */

#ifndef _DGX_INCLUDE_DGX_LCD_INIT_H_
#define _DGX_INCLUDE_DGX_LCD_INIT_H_

#include <stdint.h>

#include "dgx_screen_with_bus.h"

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

struct _dgx_lcd_init_cmd_t;

typedef uint8_t (*dgx_lcd_init_adj_func)(dgx_screen_with_bus_t *scr, const struct _dgx_lcd_init_cmd_t *init_cmd);

typedef struct _dgx_lcd_init_cmd_t {
    uint8_t cmd;
    dgx_lcd_init_adj_func adjust_func;
    uint8_t databytes; //No of data in data; 0xFF = end of cmds.
    uint8_t delay;
    uint8_t data[20];
}dgx_lcd_init_cmd_t;

void dgx_lcd_init(dgx_screen_with_bus_t *scr, const dgx_lcd_init_cmd_t *lcd_init_cmds);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif

#endif /* _DGX_INCLUDE_DGX_LCD_INIT_H_ */
