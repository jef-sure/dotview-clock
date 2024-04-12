#ifndef __DGX_SCREEN_H__
#define __DGX_SCREEN_H__
#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bus/dgx_bus_protocols.h"

struct _dgx_screen_t;

typedef void (*dgx_scr_set_area_func)(struct _dgx_screen_t *scr, uint16_t left, uint16_t right, uint16_t top, uint16_t bottom);
typedef void (*dgx_scr_update_area_func)(struct _dgx_screen_t *scr, int left, int right, int top, int bottom);
typedef void (*dgx_scr_write_area_func)(struct _dgx_screen_t *scr, uint8_t *data, uint32_t lenbits);
typedef uint32_t (*dgx_scr_read_area_func)(struct _dgx_screen_t *scr, uint8_t *data, uint32_t lenbits);
typedef void (*dgx_scr_draw_pixel_func)(struct _dgx_screen_t *scr, int x, int y, uint32_t color);
typedef uint32_t (*dgx_scr_read_pixel_func)(struct _dgx_screen_t *scr, int x, int y);
typedef void (*dgx_scr_screen_wait_buffer_func)(struct _dgx_screen_t *scr);
typedef void (*dgx_scr_screen_destroy_func)(struct _dgx_screen_t **pscr);
// typedef void (*dgx_scr_wait_buffer_func)(struct _dgx_screen_t *scr);

/* optimized functions <!-- */
typedef void (*dgx_scr_fill_rectangle_func)(struct _dgx_screen_t *scr, int x, int y, int w, int h, uint32_t color);
typedef void (*dgx_scr_line_func)(struct _dgx_screen_t *scr, int x1, int y1, int x2, int y2, uint32_t color);
typedef void (*dgx_scr_circle_func)(struct _dgx_screen_t *scr, int x, int y, int r, uint32_t color);
typedef void (*dgx_scr_solid_circle_func)(struct _dgx_screen_t *scr, int x, int y, int r, uint32_t color);
/* optimized functions --> */

typedef enum {                //
    DgxScreenBottomTop = -1,  //
    DgxScreenTopBottom = 1,   //
    DgxScreenLeftRight = 1,   //
    DgxScreenRightLeft = -1   //
} dgx_orientation_t;

typedef enum {     //
    DgxScreenRGB,  //
    DgxScreenBGR   //
} dgx_color_order_t;

typedef enum {                 //
    DgxPhysicalScreenWithBus,  //
    DgxVirtualScreen,          //
    DgxVirtualBackScreen       //
} dgx_screen_subtype_t;

typedef struct _dgx_screen_t {
    const char *screen_name;
    int screen_submodel;
    dgx_screen_subtype_t screen_subtype;
    dgx_orientation_t dir_x;
    dgx_orientation_t dir_y;
    bool swap_xy;
    int width;
    int height;
    uint8_t color_bits;
    dgx_color_order_t rgb_order;
    int32_t in_progress;
    uint16_t cg_row_shift;
    uint16_t cg_col_shift;
    dgx_scr_set_area_func set_area;
    dgx_scr_write_area_func write_area;
    dgx_scr_read_area_func read_area;
    dgx_scr_screen_wait_buffer_func wait_buffer;
    dgx_scr_draw_pixel_func set_pixel;
    dgx_scr_read_pixel_func get_pixel;
    dgx_scr_update_area_func update_screen;
    dgx_scr_screen_destroy_func destroy;
    /* optimized functions */
    dgx_scr_fill_rectangle_func fill_rectangle;
    dgx_scr_line_func draw_line;
    dgx_scr_circle_func circle;
    dgx_scr_solid_circle_func solid_circle;
} dgx_screen_t;

typedef struct dgx_point_2d {
    int x;
    int y;
} dgx_point_2d_t;

dgx_point_2d_t _dgx_start_area_pixel(  //
    int left,                          //
    int right,                         //
    int top,                           //
    int bottom,                        //
    dgx_orientation_t dir_x,           //
    dgx_orientation_t dir_y            //
);

dgx_point_2d_t _dgx_move_to_next_area_pixel(  //
    dgx_point_2d_t current_point,            //
    int left,                                //
    int right,                               //
    int top,                                 //
    int bottom,                              //
    dgx_orientation_t dir_x,                 //
    dgx_orientation_t dir_y,                 //
    bool swap_xy                             //
);

static inline void dgx_screen_destroy(dgx_screen_t **_pscr) {  //
    if (*_pscr && (*_pscr)->destroy) {                         //
        (*_pscr)->destroy(_pscr);                              //
    }                                                          //
}

static inline int dgx_screen_progress_up(dgx_screen_t *_scr) {  //
    return ++_scr->in_progress;                                 //
}

static inline int dgx_screen_progress_down(dgx_screen_t *_scr) {  //
    return --_scr->in_progress;                                   //
}

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on

#endif

#endif /* __DGX_SCREEN_H__ */
