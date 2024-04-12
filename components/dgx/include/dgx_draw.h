/*
 * dgx_draw.h
 *
 *  Created on: Apr 2, 2023
 *      Author: anton
 */

#ifndef _DGX_INCLUDE_DGX_DRAW_H_
#define _DGX_INCLUDE_DGX_DRAW_H_

#include "dgx_screen.h"

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

void dgx_fill_rectangle(dgx_screen_t *scr, int x, int y, int w, int h, uint32_t color);
void dgx_draw_circle(dgx_screen_t *scr, int x, int y, int r, uint32_t color);
void dgx_draw_line(dgx_screen_t *scr, int x1, int y1, int x2, int y2, uint32_t color);
void dgx_solid_circle(dgx_screen_t *scr, int x, int y, int r, uint32_t color);
void dgx_set_pixel(dgx_screen_t *scr, int x, int y, uint32_t color);
uint32_t dgx_get_pixel(dgx_screen_t *scr, int x, int y);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif

#endif /* _DGX_INCLUDE_DGX_DRAW_H_ */
