#ifndef _DGX_FONT_H_
#define _DGX_FONT_H_

#include <stddef.h>
#include <stdint.h>

#include "dgx_bitmap.h"
#include "dgx_screen.h"

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

typedef struct dgx_font_dot_ {
    int8_t x;
    int8_t y;
} dgx_font_dot_t;

static inline dgx_font_dot_t dgx_font_dot_new(int8_t x, int8_t y) {
    dgx_font_dot_t ret = {
        .x = x,  //
        .y = y   //
    };
    return ret;
}

struct dgx_font_sym8_params_;

typedef void (*dgx_font_dot_func_t)(dgx_screen_t *_scr_dst, int x_dst, int y_dst, struct dgx_font_sym8_params_ *vParam);

typedef struct dgx_font_sym8_params_ {
    dgx_screen_t *vpoint;
    uint16_t *lut;
    dgx_font_dot_func_t dot_func;
} dgx_font_sym8_params_t;

typedef enum {                   //
    DGX_FONT_BITMAP_LINES = 1,   //
    DGX_FONT_BITMAP_STREAM = 2,  //
    DGX_FONT_DOTS = 3            //
} dgx_font_model_t;

typedef struct {
    union {
        const uint8_t *bitmap;  ///< Pointer into GFXfont->bitmap
        struct {
            const dgx_font_dot_t *dots;
            size_t number_of_dots;
        };
    };
    int16_t width;     ///< Bitmap dimensions in pixels
    int16_t height;    ///< Bitmap dimensions in pixels
    int16_t xAdvance;  ///< Distance to advance cursor (x axis)
    int16_t xOffset;
    int16_t yOffset;
} glyph_t;

typedef struct {
    int first, number;
    const glyph_t *glyphs;
} glyph_array_t;

typedef struct {
    uint32_t fromCP;
    uint32_t toCP;
    int number_of_dots;
    dgx_point_2d_t *m_start;
    dgx_point_2d_t *m_end;
    bool is_from_empty;
    bool is_to_empty;
} dgx_font_symbol_morph_t;

struct dgx_font_;
typedef int (*dgx_font_draw_sym_func_t)(  //
    dgx_screen_t *scr,                    //
    int16_t x,                            //
    int16_t y,                            //
    uint32_t codePoint,                   //
    uint32_t color,                       //
    dgx_orientation_t xdir,               //
    dgx_orientation_t ydir,               //
    bool swap_xy,                         //
    int scale,                            //
    struct dgx_font_ *font,               //
    void *param                           //
);

typedef struct dgx_font_ {
    const glyph_array_t *glyph_ranges;
    int16_t yAdvance, yOffsetLowest, xWidest, xWidthAverage;
    dgx_font_model_t f_type;
} dgx_font_t;

uint32_t decodeUTF8next(const char *chr, size_t *idx);
const glyph_t *dgx_font_find_glyph(uint32_t codePoint, dgx_font_t *font, int16_t *xAdvance);
int dgx_font_char_to_screen(             //
    dgx_screen_t *scr,                   //
    int16_t x,                           //
    int16_t y,                           //
    uint32_t codePoint,                  //
    uint32_t color,                      //
    dgx_orientation_t xdir,              //
    dgx_orientation_t ydir,              //
    bool swap_xy,                        //
    int scale,                           //
    struct dgx_font_ *font,              //
    dgx_font_draw_sym_func_t draw_func,  //
    void *param                          //
);
int dgx_font_string_bounds(const char *str, dgx_font_t *font, int16_t *ycorner, int16_t *height);
void dgx_font_string_utf8_screen(        //
    dgx_screen_t *scr,                   //
    int16_t x,                           //
    int16_t y,                           //
    const char *str,                     //
    uint32_t color,                      //
    dgx_orientation_t xdir,              //
    dgx_orientation_t ydir,              //
    bool swap_xy,                        //
    int scale,                           //
    struct dgx_font_ *font,              //
    dgx_font_draw_sym_func_t draw_func,  //
    void *param                          //
);

void dgx_font_make_point8(dgx_screen_t *vpoint);

dgx_font_symbol_morph_t *dgx_font_make_morph_struct(  //
    dgx_font_t *font,                                 //
    uint32_t fromCP,                                  //
    uint32_t toCP,                                    //
    int x,                                            //
    int y,                                            //
    int scale                                         //
);
void dgx_font_make_morph_struct_destroy(dgx_font_symbol_morph_t **ms);

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif

#endif /* _DGX_FONT_H_ */
