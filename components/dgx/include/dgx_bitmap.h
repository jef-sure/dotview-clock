#ifndef _DGX_BITMAP_H_
#define _DGX_BITMAP_H_

#include <stddef.h>
#include <stdint.h>

#include "dgx_screen.h"
#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

typedef struct {
    uint8_t *bitmap;
    int width;
    int height;
    bool is_stream;
} dgx_bw_bitmap_t;

bool dgx_bw_bitmap_get_pixel(dgx_bw_bitmap_t *bmap, int x, int y);
static inline dgx_bw_bitmap_t dgx_bw_bitmap_make_of(uint8_t *bitmap, int width, int height, bool is_stream) {
    dgx_bw_bitmap_t ret = {
        .bitmap = bitmap,       //
        .width = width,         //
        .height = height,       //
        .is_stream = is_stream  //
    };
    return ret;
}

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif
#endif /* _DGX_BITMAP_H_ */
