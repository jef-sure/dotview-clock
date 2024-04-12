#include <stdbool.h>
#include <string.h>

#include "dgx_bitmap.h"

bool dgx_bw_bitmap_get_pixel(dgx_bw_bitmap_t *bmap, int x, int y) {
    if (!bmap->is_stream) { // DGX_FONT_BITMAP_LINES
        int pitch = (bmap->width + 7) / 8;
        int offset = y * pitch + x / 8;
        uint8_t bmask = 0x80 >> (x & 7);
        uint8_t bc = bmap->bitmap[offset];
        bool ret = !!(bc & bmask);
        return ret;
    } else {  // DGX_FONT_BITMAP_STREAM
        int offset = y * bmap->width + x;
        uint8_t bmask = 0x80 >> (offset & 7);
        uint8_t bc = bmap->bitmap[(offset + 7) / 8];
        return !!(bc & bmask);
    }
}
