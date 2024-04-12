#ifndef _dgx_VSCREEN_H_
#define _dgx_VSCREEN_H_
#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

#include "dgx_screen.h"

typedef struct _dgx_vscreen_t {
    dgx_screen_t base;
    uint8_t *v_array;
    struct {
        uint16_t left;
        uint16_t right;
        uint16_t top;
        uint16_t bottom;
        uint16_t x_offset;
        uint16_t y_offset;
    } area;
} dgx_vscreen_t;

dgx_screen_t* dgx_vscreen_init(int width, int height, uint8_t color_bits, dgx_color_order_t cbo);
dgx_screen_t *dgx_vscreen_clone(dgx_screen_t *_scr_src);
void dgx_vscreen8_to_screen16(dgx_screen_t *_scr_dst, int x_dst, int y_dst, dgx_screen_t *_scr_src, uint16_t *lut, bool has_transparency);
bool dgx_vscreen_copy(dgx_screen_t *_scr_dst, dgx_screen_t *_scr_src);
void dgx_vscreen_to_vscreen(dgx_screen_t *_scr_dst, int x_dst, int y_dst, dgx_screen_t *_scr_src, bool has_transparency);
void dgx_vscreen_to_screen(dgx_screen_t *_scr_dst, int x_dst, int y_dst, dgx_screen_t *_scr_src);
void dgx_vscreen_region_to_screen(dgx_screen_t *_scr_dst, int x_dst, int y_dst, dgx_screen_t *_scr_src, int x_src, int y_src, int width, int height);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif
#endif /* _dgx_VSCREEN_H_ */
