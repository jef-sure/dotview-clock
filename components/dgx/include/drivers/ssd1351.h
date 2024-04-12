#ifndef _dgx_SSD1351_H_
#define _dgx_SSD1351_H_
#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

#include "bus/dgx_bus_protocols.h"
#include "dgx_screen.h"
#include "driver/gpio.h"

dgx_screen_t *dgx_ssd1351_init(dgx_bus_protocols_t *bus, gpio_num_t rst, uint8_t color_bits, dgx_color_order_t cbo);
void dgx_ssd1351_orientation(dgx_screen_t *_scr, dgx_orientation_t dir_x, dgx_orientation_t dir_y, bool swap_xy);
void dgx_ssd1351_brightness(dgx_screen_t *_scr, uint8_t brightness /* 0 - 255 */);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif
#endif /* _dgx_SSD1351_H_ */
