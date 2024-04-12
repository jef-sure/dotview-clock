#ifndef _dgx_ST7735_H_
#define _dgx_ST7735_H_
#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

#include "dgx_screen.h"
#include "bus/dgx_bus_protocols.h"

dgx_screen_t* dgx_st7735_init(dgx_bus_protocols_t *bus, gpio_num_t rst, uint8_t color_bits, dgx_color_order_t cbo);
void dgx_st7735_orientation(dgx_screen_t *scr, dgx_orientation_t dir_x, dgx_orientation_t dir_y, bool swap_xy);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif
#endif /* _dgx_ST7735_H_ */
