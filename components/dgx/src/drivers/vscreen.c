#include <string.h>

#include "dgx_arch.h"
#include "dgx_bits.h"
#include "dgx_screen_functions.h"
#include "drivers/dgx_lcd_init.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

static const char TAG[] = "DGX V-Screen";

static void dgx_vscreen_set_area(dgx_screen_t *_scr, uint16_t left, uint16_t right, uint16_t top, uint16_t bottom) {
    dgx_vscreen_t *scr = (dgx_vscreen_t *)_scr;
    if (left >= scr->base.width || top >= scr->base.height || right >= scr->base.width || bottom >= scr->base.height) {
        ESP_LOGE(TAG, "Set area failed");
        return;
    }
    scr->area.left = left;
    scr->area.right = right;
    scr->area.top = top;
    scr->area.bottom = bottom;
    scr->area.x_offset = left;
    scr->area.y_offset = top;
}

static inline void _dgx_move_to_next_area_pixel_v(dgx_screen_t *_scr) {
    dgx_vscreen_t *scr = (dgx_vscreen_t *)_scr;
    if (scr->area.x_offset >= scr->area.right) {
        scr->area.x_offset = scr->area.left;
        if (scr->area.y_offset >= scr->area.bottom) {
            scr->area.y_offset = scr->area.top;
        } else {
            scr->area.y_offset++;
        }
    } else {
        scr->area.x_offset++;
    }
}

#define DGX_VSCR_OFFSET(vscr, x, y) ((x) + (y) * (vscr)->base.width)
#define DGX_VSCR_PTR(vscr, offset) ((vscr)->v_array + ((offset) * (vscr)->base.color_bits + 7u) / 8u)

static void dgx_vscreen_write_area(dgx_screen_t *_scr, uint8_t *data, uint32_t lenbits) {
    dgx_vscreen_t *scr = (dgx_vscreen_t *)_scr;
    int idx = 0;
    while (lenbits >= _scr->color_bits) {
        uint32_t offset = DGX_VSCR_OFFSET(scr, scr->area.x_offset, scr->area.y_offset);
        uint8_t *lp = DGX_VSCR_PTR(scr, offset);
        uint32_t color = dgx_read_buf_value(_scr->color_bits, &data, idx);
        ++idx;
        dgx_fill_buf_value(_scr->color_bits, lp, offset, color);
        _dgx_move_to_next_area_pixel_v(_scr);
        lenbits -= _scr->color_bits;
    }
}
static uint32_t dgx_vscreen_read_area(dgx_screen_t *_scr, uint8_t *data, uint32_t lenbits) {
    uint32_t ret = 0;
    dgx_vscreen_t *scr = (dgx_vscreen_t *)_scr;
    int idx = 0;
    while (lenbits >= _scr->color_bits) {
        uint32_t offset = DGX_VSCR_OFFSET(scr, scr->area.x_offset, scr->area.y_offset);
        uint8_t *lp = DGX_VSCR_PTR(scr, offset);
        uint32_t color = dgx_read_buf_value(_scr->color_bits, &lp, offset);
        data = dgx_fill_buf_value(_scr->color_bits, data, idx, color);
        ++idx;
        _dgx_move_to_next_area_pixel_v(_scr);
        ret += _scr->color_bits;
        lenbits -= _scr->color_bits;
    }
    return ret;
}

static void dgx_vscreen_set_pixel(dgx_screen_t *_scr, int x, int y, uint32_t color) {
    dgx_vscreen_t *scr = (dgx_vscreen_t *)_scr;
    if (x < 0 || y < 0 || x >= _scr->width || y >= _scr->height) return;
    uint32_t offset = DGX_VSCR_OFFSET(scr, x, y);
    uint8_t *lp = DGX_VSCR_PTR(scr, offset);
    dgx_fill_buf_value(_scr->color_bits, lp, offset, color);
    if (!_scr->in_progress) _scr->update_screen(_scr, x, x, y, y);
}

static uint32_t dgx_vscreen_get_pixel(dgx_screen_t *_scr, int x, int y) {
    dgx_vscreen_t *scr = (dgx_vscreen_t *)_scr;
    if (x < 0 || y < 0 || x >= _scr->width || y >= _scr->height) return 0;
    uint32_t offset = DGX_VSCR_OFFSET(scr, x, y);
    uint8_t *lp = DGX_VSCR_PTR(scr, offset);
    uint32_t color = dgx_read_buf_value(_scr->color_bits, &lp, offset);
    return color;
}

static void dgx_vscreen_update_area(dgx_screen_t *_scr, int left, int right, int top, int bottom) {}
static void dgx_vscreen_wait_buffer_dummy(dgx_screen_t *_scr) {}
static void dgx_vscreen_screen_destroy(dgx_screen_t **_pscr) {
    if (*_pscr) {
        dgx_vscreen_t *scr = (dgx_vscreen_t *)(*_pscr);
        free(scr->v_array);
        free(*_pscr);
        *_pscr = 0;
    }
}

static void dgx_vscreen_fill_rectangle(dgx_screen_t *_scr, int x, int y, int w, int h, uint32_t color) {
    dgx_vscreen_t *scr = (dgx_vscreen_t *)_scr;
    if (w < 0 || x + w < 0 || x >= _scr->width || h < 0 || y + h < 0 || y >= _scr->height) return;
    if (x < 0) {
        w = x + w;
        x = 0;
    }
    if (x + w > _scr->width) {
        w = _scr->width - x;
    }
    if (y < 0) {
        h = y + h;
        y = 0;
    }
    if (y + h > _scr->height) {
        h = _scr->height - y;
    }
    if (h <= 0 || w <= 0) return;
    if (x == 0 && w == _scr->width && y == 0 && y == _scr->height && color == 0) {
        uint32_t asize = dgx_color_points_to_bytes(_scr->color_bits, _scr->width * _scr->height);
        memset(scr->v_array, 0, asize);
    } else {
        for (int i = 0; i < h; ++i) {
            uint32_t offset = DGX_VSCR_OFFSET(scr, x, y + i);
            uint8_t *lp = DGX_VSCR_PTR(scr, offset);
            DGX_FILL_BUFFER(_scr->color_bits, lp, w, color);
        }
    }
    if (!_scr->in_progress) _scr->update_screen(_scr, x, x + w - 1, y, y + h - 1);
}
static void dgx_vscreen_line(dgx_screen_t *_scr, int x1, int y1, int x2, int y2, uint32_t color) {
    int dx, dy;
    dx = x2 - x1;
    dy = DGX_ABS(y2 - y1);
    int steep = dy > DGX_ABS(dx);
    if (steep) {
        DGX_INT_SWAP(x1, y1);
        DGX_INT_SWAP(x2, y2);
    }

    if (x1 > x2) {
        DGX_INT_SWAP(x1, x2);
        DGX_INT_SWAP(y1, y2);
        dx = -dx;
    }

    int err = dx / 2;
    int ystep;

    if (y1 < y2) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    ++_scr->in_progress;
    int x0 = x1;
    int y0 = y1;
    for (; x0 <= x2; x0++) {
        if (steep) {
            _scr->set_pixel(_scr, y0, x0, color);
        } else {
            _scr->set_pixel(_scr, x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
    if (!--_scr->in_progress) _scr->update_screen(_scr, x1, x2, DGX_MIN(y1, y2), DGX_MAX(y1, y2));
}

static void dgx_vscreen_circle(dgx_screen_t *_scr, int x0, int y0, int r, uint32_t color) {
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0;
    int y = r;
    int area_left = x0 - r;
    int area_right = x0 + r;
    int area_top = y0 - r;
    int area_bottom = y0 + r;
    if (area_bottom < 0 || area_top >= _scr->height || area_left >= _scr->width || area_right < 0) return;
    if (r == 0) {
        _scr->set_pixel(_scr, x, y, color);
        // set_pixel has its own call to update_screen
        return;
    }

    ++_scr->in_progress;

    _scr->set_pixel(_scr, x0, y0 + r, color);
    _scr->set_pixel(_scr, x0, y0 - r, color);
    _scr->set_pixel(_scr, x0 + r, y0, color);
    _scr->set_pixel(_scr, x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        _scr->set_pixel(_scr, x0 + x, y0 + y, color);
        _scr->set_pixel(_scr, x0 - x, y0 + y, color);
        _scr->set_pixel(_scr, x0 + x, y0 - y, color);
        _scr->set_pixel(_scr, x0 - x, y0 - y, color);
        _scr->set_pixel(_scr, x0 + y, y0 + x, color);
        _scr->set_pixel(_scr, x0 - y, y0 + x, color);
        _scr->set_pixel(_scr, x0 + y, y0 - x, color);
        _scr->set_pixel(_scr, x0 - y, y0 - x, color);
    }
    if (!--_scr->in_progress) _scr->update_screen(_scr, area_left, area_right, area_top, area_bottom);
}

static void dgx_vscreen_solid_circle(dgx_screen_t *_scr, int x0, int y0, int r, uint32_t color) {
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0;
    int y = r;
    int area_left = x0 - r;
    int area_right = x0 + r;
    int area_top = y0 - r;
    int area_bottom = y0 + r;
    if (area_bottom < 0 || area_top >= _scr->height || area_left >= _scr->width || area_right < 0) return;
    if (r == 0) {
        _scr->set_pixel(_scr, x, y, color);
        // set_pixel has its own call to update_screen
        return;
    }

    ++_scr->in_progress;

    _scr->fill_rectangle(_scr, x0 - r, y0, 2 * r + 1, 1, color);
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        _scr->fill_rectangle(_scr, x0 - x, y0 + y, 2 * x + 1, 1, color);
        _scr->fill_rectangle(_scr, x0 - x, y0 - y, 2 * x + 1, 1, color);
        _scr->fill_rectangle(_scr, x0 - y, y0 + x, 2 * y + 1, 1, color);
        _scr->fill_rectangle(_scr, x0 - y, y0 - x, 2 * y + 1, 1, color);
    }
    if (!--_scr->in_progress) _scr->update_screen(_scr, area_left, area_right, area_top, area_bottom);
}

// Initialize the display
dgx_screen_t *dgx_vscreen_init(int width, int height, uint8_t color_bits, dgx_color_order_t cbo) {
    // Attach the LCD to the SPI bus
    dgx_vscreen_t *scr = (dgx_vscreen_t *)calloc(1, sizeof(dgx_vscreen_t));
    if (!scr) {
        ESP_LOGE(TAG, "Screen structure memory allocation failed");
        return 0;
    }
    scr->base.cg_row_shift = 0;
    scr->base.cg_col_shift = 0;
    scr->base.width = width;
    scr->base.height = height;
    scr->base.color_bits = color_bits;
    scr->base.rgb_order = cbo;
    scr->base.screen_name = "V-SCREEN";
    scr->base.screen_submodel = 0;
    scr->base.screen_subtype = DgxVirtualScreen;
    scr->base.circle = dgx_vscreen_circle;
    scr->base.destroy = dgx_vscreen_screen_destroy;
    scr->base.set_pixel = dgx_vscreen_set_pixel;
    scr->base.fill_rectangle = dgx_vscreen_fill_rectangle;
    scr->base.draw_line = dgx_vscreen_line;
    scr->base.get_pixel = dgx_vscreen_get_pixel;
    scr->base.set_area = dgx_vscreen_set_area;
    scr->base.read_area = dgx_vscreen_read_area;
    scr->base.write_area = dgx_vscreen_write_area;
    scr->base.wait_buffer = dgx_vscreen_wait_buffer_dummy;
    scr->base.solid_circle = dgx_vscreen_solid_circle;
    scr->base.update_screen = dgx_vscreen_update_area;

    uint32_t asize = dgx_color_points_to_bytes(color_bits, width * height);
    scr->v_array = calloc(asize, 1);
    if (!scr->v_array) {
        ESP_LOGE(TAG, "Impossible to allocation memory");
        free(scr);
        return 0;
    }
    return (dgx_screen_t *)scr;
}

/* Interscreen operatiopns */

// big endian
#define _DGX_READ_COLOR(bits, ptr)                                                                                   \
    (((bits) == 8    ? (uint32_t) * (ptr)                                                                            \
      : (bits) == 16 ? ((uint32_t)(ptr)[1] | ((uint32_t)(ptr)[0] << 8))                                              \
                     : ((bits) == 24 ? ((uint32_t)(ptr)[2] | ((uint32_t)(ptr)[1] << 8) | ((uint32_t)(ptr)[0] << 16)) \
                                     : ((uint32_t)(ptr)[3] | ((uint32_t)(ptr)[2] << 8) | ((uint32_t)(ptr)[1] << 16) | ((uint32_t)(ptr)[0] << 24)))))

// big endian
#define _DGX_WRITE_COLOR(bits, ptr, color)            \
    do {                                              \
        switch (bits) {                               \
            case 8:                                   \
                (ptr)[0] = (uint8_t)(color);          \
                break;                                \
            case 16:                                  \
                (ptr)[1] = (uint8_t)(color);          \
                (ptr)[0] = (uint8_t)((color) >> 8u);  \
                break;                                \
            case 24:                                  \
                (ptr)[2] = (uint8_t)(color);          \
                (ptr)[1] = (uint8_t)((color) >> 8u);  \
                (ptr)[0] = (uint8_t)((color) >> 16u); \
                break;                                \
            default:                                  \
                (ptr)[3] = (uint8_t)(color);          \
                (ptr)[2] = (uint8_t)((color) >> 8u);  \
                (ptr)[1] = (uint8_t)((color) >> 16u); \
                (ptr)[0] = (uint8_t)((color) >> 24u); \
                break;                                \
        }                                             \
    } while (0)

void dgx_vscreen_to_vscreen(dgx_screen_t *_scr_dst, int x_dst, int y_dst, dgx_screen_t *_scr_src, bool has_transparency) {
    if (_scr_src->screen_subtype != DgxVirtualScreen && _scr_src->screen_subtype != DgxVirtualBackScreen) {
        ESP_LOGE(TAG, "dgx_vscreen_to_vscreen: source is not virtual");
        return;
    }
    if (_scr_dst->screen_subtype != DgxVirtualScreen && _scr_dst->screen_subtype != DgxVirtualBackScreen) {
        ESP_LOGE(TAG, "dgx_vscreen_to_vscreen: destination is not virtual");
        return;
    }
    dgx_vscreen_t *scr_dst = (dgx_vscreen_t *)_scr_dst;
    dgx_vscreen_t *scr_src = (dgx_vscreen_t *)_scr_src;
    if (_scr_dst->color_bits != _scr_src->color_bits) {
        ESP_LOGE(TAG, "vscreen_to_vscreen: color bits for source and destination arn't equal");
        return;
    }
    int x1 = x_dst;
    int y1 = y_dst;
    int bw = _scr_src->width;
    int bh = _scr_src->height;
    int skip_x = 0;
    int skip_y = 0;
    if (x1 < 0) {
        skip_x = -x1;
        bw += x1;
        x1 = 0;
    }
    if (y1 < 0) {
        skip_y = -y1;
        bh += y1;
        y1 = 0;
    }
    if (x1 + bw <= 0 || y1 + bh <= 0) return;
    if (x1 + bw > _scr_dst->width) {
        bw = _scr_dst->width - x1;
    }
    if (y1 + bh > _scr_dst->height) {
        bh = _scr_dst->height - y1;
    }
    if (bh <= 0 || bw <= 0) return;
    ++_scr_dst->in_progress;
    uint32_t csize = dgx_color_points_to_bytes(_scr_dst->color_bits, bw);
    for (int br = 0; br < bh; ++br) {
        if (_scr_dst->color_bits % 8u == 0) {
            uint32_t offset_src = DGX_VSCR_OFFSET(scr_src, skip_x, skip_y + br);
            uint8_t *lp_src = DGX_VSCR_PTR(scr_src, offset_src);
            uint32_t offset_dst = DGX_VSCR_OFFSET(scr_dst, x1, y1 + br);
            uint8_t *lp_dst = DGX_VSCR_PTR(scr_dst, offset_dst);
            if (!has_transparency) {
                memcpy(lp_dst, lp_src, csize);
            } else {
                uint32_t psize = dgx_color_points_to_bytes(_scr_dst->color_bits, 1);
                for (int bc = 0; bc < bw; ++bc) {
                    uint8_t *src_ptr = lp_src + bc * psize;
                    uint32_t color = _DGX_READ_COLOR(_scr_dst->color_bits, src_ptr);
                    if (color || !has_transparency) {
                        uint8_t *dst_ptr = lp_dst + bc * psize;
                        _DGX_WRITE_COLOR(_scr_dst->color_bits, dst_ptr, color);
                    }
                }
            }
        } else {
            for (int bc = 0; bc < bw; ++bc) {
                uint32_t color = _scr_src->get_pixel(_scr_src, skip_x + bc, skip_y + br);
                if (color || !has_transparency) _scr_dst->set_pixel(_scr_dst, x1 + bc, y1 + br, color);
            }
        }
    }
    if (!--_scr_dst->in_progress) _scr_dst->update_screen(_scr_dst, x1, x1 + bw - 1, y1, y1 + bh - 1);
}

bool dgx_vscreen_copy(dgx_screen_t *_scr_dst, dgx_screen_t *_scr_src) {
    if (_scr_src->height != _scr_dst->height || _scr_src->width != _scr_dst->width || _scr_src->color_bits != _scr_dst->color_bits) return false;
    if (_scr_src->screen_subtype != DgxVirtualScreen && _scr_src->screen_subtype != DgxVirtualBackScreen) {
        ESP_LOGE(TAG, "dgx_vscreen_copy: source is not virtual");
        return false;
    }
    if (_scr_dst->screen_subtype != DgxVirtualScreen && _scr_dst->screen_subtype != DgxVirtualBackScreen) {
        ESP_LOGE(TAG, "dgx_vscreen_copy: destination is not virtual");
        return false;
    }
    dgx_vscreen_t *scr_dst = (dgx_vscreen_t *)_scr_dst;
    dgx_vscreen_t *scr_src = (dgx_vscreen_t *)_scr_src;
    if (!scr_dst->v_array || !scr_src->v_array) return false;
    uint32_t asize = dgx_color_points_to_bytes(_scr_src->color_bits, _scr_src->width * _scr_src->height);
    memcpy(scr_dst->v_array, scr_src->v_array, asize);
    return true;
}

#define DGX_INTERSECT_RECTANGLES(xo, yo, wo, ho, xb, yb, wb, hb) \
    do {                                                         \
        int xc = DGX_MAX(xo, xb);                                \
        int yc = DGX_MAX(yo, yb);                                \
        wo += xo - xc;                                           \
        ho += yo - yc;                                           \
        int rx = DGX_MIN(xc + wo, xb + wb);                      \
        int ry = DGX_MIN(yc + ho, yb + hb);                      \
        wo = rx - xc;                                            \
        ho = ry - yc;                                            \
        xo = xc;                                                 \
        yo = yc;                                                 \
    } while (0)

void dgx_vscreen_to_screen(dgx_screen_t *_scr_dst, int x_dst, int y_dst, dgx_screen_t *_scr_src) {
    dgx_vscreen_t *scr_src = (dgx_vscreen_t *)_scr_src;
    if (_scr_dst->color_bits != _scr_src->color_bits) {
        ESP_LOGE(TAG, "dgx_vscreen_to_screen: source color bits %d != destination %d", _scr_src->color_bits, _scr_dst->color_bits);
        return;
    }
    if (_scr_src->screen_subtype != DgxVirtualScreen && _scr_src->screen_subtype != DgxVirtualBackScreen) {
        ESP_LOGE(TAG, "dgx_vscreen_to_screen: source is not virtual");
        return;
    }
    int x1 = x_dst;
    int y1 = y_dst;
    int bw = _scr_src->width;
    int bh = _scr_src->height;
    DGX_INTERSECT_RECTANGLES(x1, y1, bw, bh, 0, 0, _scr_dst->width, _scr_dst->height);
    if (bh <= 0 || bw <= 0) return;
    int skip_x = x1 - x_dst;
    int skip_y = y1 - y_dst;
    if (_scr_dst->screen_subtype == DgxPhysicalScreenWithBus) {
        dgx_bus_protocols_t *bus = ((dgx_screen_with_bus_t *)_scr_dst)->bus;
        uint8_t *draw_buffer = bus->draw_buffer;
        size_t draw_buffer_len = bus->draw_buffer_len;
        _scr_dst->set_area(_scr_dst, x1, x1 + bw - 1, y1, y1 + bh - 1);
        _scr_dst->wait_buffer(_scr_dst);
        uint8_t *cbuf = draw_buffer;
        uint32_t bip = dgx_color_points_to_bytes(_scr_src->color_bits, 1);
        for (int br = 0; br < bh; ++br) {
            if (_scr_dst->color_bits % 8u == 0) {
                uint32_t offset_src = DGX_VSCR_OFFSET(scr_src, skip_x, skip_y + br);
                uint8_t *lp_src = DGX_VSCR_PTR(scr_src, offset_src);
                for (int bc = 0; bc < bw; ++bc) {
                    uint32_t color = _DGX_READ_COLOR(_scr_src->color_bits, lp_src);
                    _DGX_WRITE_COLOR(_scr_src->color_bits, cbuf, color);
                    lp_src += bip;
                    cbuf += bip;
                    if (cbuf - draw_buffer >= draw_buffer_len - 4) {
                        _scr_dst->write_area(_scr_dst, draw_buffer, 8u * (cbuf - draw_buffer));
                        _scr_dst->wait_buffer(_scr_dst);
                        cbuf = draw_buffer;
                    }
                }
            } else {
                for (int bc = 0; bc < bw; ++bc) {
                    uint32_t color = _scr_src->get_pixel(_scr_src, skip_x + bc, skip_y + br);
                    _scr_dst->set_pixel(_scr_dst, x1 + bc, y1 + br, color);
                }
            }
        }
        if (cbuf != draw_buffer) {
            _scr_dst->write_area(_scr_dst, draw_buffer, 8u * (cbuf - draw_buffer));
            _scr_dst->wait_buffer(_scr_dst);
        }
    } else if (_scr_dst->screen_subtype == DgxVirtualScreen || _scr_dst->screen_subtype == DgxVirtualBackScreen) {
        dgx_vscreen_t *scr_dst = (dgx_vscreen_t *)_scr_dst;
        uint32_t bip = dgx_color_points_to_bytes(_scr_src->color_bits, 1);
        for (int br = 0; br < bh; ++br) {
            if (_scr_dst->color_bits % 8u == 0) {
                uint32_t offset_src = DGX_VSCR_OFFSET(scr_src, skip_x, skip_y + br);
                uint8_t *lp_src = DGX_VSCR_PTR(scr_src, offset_src);
                uint32_t offset_dst = DGX_VSCR_OFFSET(scr_dst, x1, y1 + br);
                uint8_t *lp_dst = DGX_VSCR_PTR(scr_dst, offset_dst);
                for (int bc = 0; bc < bw; ++bc) {
                    uint32_t color = _DGX_READ_COLOR(_scr_src->color_bits, lp_src);
                    _DGX_WRITE_COLOR(_scr_dst->color_bits, lp_dst, color);
                    lp_src += bip;
                    lp_dst += bip;
                }
            } else {
                for (int bc = 0; bc < bw; ++bc) {
                    uint32_t color = _scr_src->get_pixel(_scr_src, bc + skip_x, br + skip_y);
                    _scr_dst->set_pixel(_scr_dst, x1 + bc, y1 + br, color);
                }
            }
        }
    }
    if (!_scr_dst->in_progress) _scr_dst->update_screen(_scr_dst, x1, x1 + bw - 1, y1, y1 + bh - 1);
}

void dgx_vscreen_region_to_screen(dgx_screen_t *_scr_dst, int x_dst, int y_dst, dgx_screen_t *_scr_src, int x_src, int y_src, int width, int height) {
    dgx_vscreen_t *scr_src = (dgx_vscreen_t *)_scr_src;
    if (_scr_dst->color_bits != _scr_src->color_bits) {
        ESP_LOGE(TAG, "dgx_vscreen_to_screen: source color bits %d != destination %d", _scr_src->color_bits, _scr_dst->color_bits);
        return;
    }
    if (_scr_src->screen_subtype != DgxVirtualScreen && _scr_src->screen_subtype != DgxVirtualBackScreen) {
        ESP_LOGE(TAG, "dgx_vscreen_to_screen: source is not virtual");
        return;
    }
    DGX_INTERSECT_RECTANGLES(x_src, y_src, width, height, 0, 0, _scr_src->width, _scr_src->height);
    if (width <= 0 || height <= 0) return;
    DGX_INTERSECT_RECTANGLES(x_dst, y_dst, width, height, 0, 0, _scr_dst->width, _scr_dst->height);
    if (width <= 0 || height <= 0) return;
    uint8_t *draw_buffer;
    size_t draw_buffer_len;
    if (_scr_dst->screen_subtype == DgxPhysicalScreenWithBus) {
        dgx_bus_protocols_t *bus = ((dgx_screen_with_bus_t *)_scr_dst)->bus;
        draw_buffer_len = bus->draw_buffer_len;
        draw_buffer = bus->draw_buffer;
    } else {
        draw_buffer_len = 4000;  // arbitrary
        draw_buffer = calloc(draw_buffer_len, 1);
        if (!draw_buffer) {
            ESP_LOGE(TAG, "dgx_vscreen_to_screen: v-v requires %u free ram but it's impossible to allocate", draw_buffer_len);
            return;
        }
    }
    _scr_dst->set_area(_scr_dst, x_dst, x_dst + width - 1, y_dst, y_dst + height - 1);
    _scr_dst->wait_buffer(_scr_dst);
    uint8_t *cbuf = draw_buffer;
    uint32_t bip = dgx_color_points_to_bytes(_scr_src->color_bits, 1);
    for (int br = 0; br < height; ++br) {
        if (_scr_dst->color_bits % 8u == 0) {
            uint32_t offset_src = DGX_VSCR_OFFSET(scr_src, x_src, y_src + br);
            uint8_t *lp_src = DGX_VSCR_PTR(scr_src, offset_src);
            for (int bc = 0; bc < width; ++bc) {
                uint32_t color = _DGX_READ_COLOR(_scr_src->color_bits, lp_src);
                _DGX_WRITE_COLOR(_scr_src->color_bits, cbuf, color);
                lp_src += bip;
                cbuf += bip;
                if (cbuf - draw_buffer >= draw_buffer_len - 4u) {
                    _scr_dst->write_area(_scr_dst, draw_buffer, 8u * (cbuf - draw_buffer));
                    _scr_dst->wait_buffer(_scr_dst);
                    cbuf = draw_buffer;
                }
            }
        } else {
            for (int bc = 0; bc < width; ++bc) {
                uint32_t color = _scr_src->get_pixel(_scr_src, x_src + bc, y_src + br);
                _scr_dst->set_pixel(_scr_dst, x_dst + bc, y_dst + br, color);
            }
        }
    }
    if (cbuf != draw_buffer) {
        _scr_dst->write_area(_scr_dst, draw_buffer, 8u * (cbuf - draw_buffer));
        _scr_dst->wait_buffer(_scr_dst);
    }
    if (_scr_dst->screen_subtype != DgxPhysicalScreenWithBus) {
        free(draw_buffer);
    }
    if (!_scr_dst->in_progress) _scr_dst->update_screen(_scr_dst, x_dst, x_dst + width - 1, y_dst, y_dst + height - 1);
}

void dgx_vscreen8_to_screen16(dgx_screen_t *_scr_dst, int x_dst, int y_dst, dgx_screen_t *_scr_src, uint16_t *lut, bool has_transparency) {
    dgx_vscreen_t *scr_src = (dgx_vscreen_t *)_scr_src;
    if (_scr_dst->color_bits != 16 || _scr_src->color_bits != 8) {
        ESP_LOGE(TAG, "vscreen8_to_screen16: source color bits %d != 8 or destination %d != 16", _scr_src->color_bits, _scr_dst->color_bits);
        return;
    }
    if (_scr_src->screen_subtype != DgxVirtualScreen && _scr_src->screen_subtype != DgxVirtualBackScreen) {
        ESP_LOGE(TAG, "vscreen8_to_screen16: source is not virtual");
        return;
    }
    int bw = _scr_src->width;
    int bh = _scr_src->height;
    int rxd = x_dst;
    int ryd = y_dst;
    DGX_INTERSECT_RECTANGLES(rxd, ryd, bw, bh, 0, 0, _scr_dst->width, _scr_dst->height);
    if (bh <= 0 || bw <= 0) return;
    int skip_x = rxd - x_dst;
    int skip_y = ryd - y_dst;
    if (_scr_dst->screen_subtype == DgxPhysicalScreenWithBus) {
        dgx_bus_protocols_t *bus = ((dgx_screen_with_bus_t *)_scr_dst)->bus;
        uint8_t *draw_buffer = bus->draw_buffer;
        size_t draw_buffer_len =bus->draw_buffer_len ;
        if (!has_transparency) {
            _scr_dst->set_area(_scr_dst, rxd, rxd + bw - 1, ryd, ryd + bh - 1);
            _scr_dst->wait_buffer(_scr_dst);
            uint8_t *cbuf = draw_buffer;
            uint32_t sw = _scr_src->width;
            for (int br = 0; br < bh; ++br) {
                for (int bc = 0; bc < bw; ++bc) {
                    uint8_t cidx = scr_src->v_array[skip_x + bc + (skip_y + br) * sw];
                    uint16_t color = lut[cidx];
                    *cbuf++ = (uint8_t)(color >> 8);
                    *cbuf++ = (uint8_t)color;
                    if (cbuf - draw_buffer >= draw_buffer_len - 4) {
                        _scr_dst->write_area(_scr_dst, draw_buffer, 8u * (cbuf - draw_buffer));
                        _scr_dst->wait_buffer(_scr_dst);
                        cbuf = draw_buffer;
                    }
                }
            }
            if (cbuf != draw_buffer) {
                _scr_dst->write_area(_scr_dst, draw_buffer, 8u * (cbuf - draw_buffer));
                _scr_dst->wait_buffer(_scr_dst);
            }
        } else {
            for (int br = 0; br < bh; ++br) {
                for (int bc = 0; bc < bw; ++bc) {
                    uint32_t color = _scr_src->get_pixel(_scr_src, bc + skip_x, br + skip_y);
                    if (color || !has_transparency) {
                        _scr_dst->set_pixel(_scr_dst, rxd + bc, ryd + br, color);
                    }
                }
            }
        }
    } else if (_scr_dst->screen_subtype == DgxVirtualScreen || _scr_dst->screen_subtype == DgxVirtualBackScreen) {
        dgx_vscreen_t *scr_dst = (dgx_vscreen_t *)_scr_dst;
        for (int br = 0; br < bh; ++br) {
            if (_scr_dst->color_bits % 8u == 0) {
                uint32_t offset_src = DGX_VSCR_OFFSET(scr_src, skip_x, skip_y + br);
                uint8_t *lp_src = DGX_VSCR_PTR(scr_src, offset_src);
                uint32_t offset_dst = DGX_VSCR_OFFSET(scr_dst, rxd, ryd + br);
                uint8_t *lp_dst = DGX_VSCR_PTR(scr_dst, offset_dst);
                for (int bc = 0; bc < bw; ++bc) {
                    uint32_t color = lut[*lp_src];
                    ++lp_src;
                    if (color || !has_transparency) {
                        uint8_t *dst_ptr = lp_dst + bc * 2u;
                        dst_ptr[0] = (uint8_t)(color >> 8);
                        dst_ptr[1] = (uint8_t)color;
                    }
                }
            } else {
                for (int bc = 0; bc < bw; ++bc) {
                    uint32_t color = _scr_src->get_pixel(_scr_src, bc + skip_x, br + skip_y);
                    if (color || !has_transparency) {
                        _scr_dst->set_pixel(_scr_dst, rxd + bc, ryd + br, color);
                    }
                }
            }
        }
    }
    if (!_scr_dst->in_progress) _scr_dst->update_screen(_scr_dst, rxd, rxd + bw - 1, ryd, ryd + bh - 1);
}

dgx_screen_t *dgx_vscreen_clone(dgx_screen_t *_scr_src) {
    if (_scr_src->screen_subtype != DgxVirtualScreen && _scr_src->screen_subtype != DgxVirtualBackScreen) {
        ESP_LOGE(TAG, "dgx_vscreen_clone: source is not virtual");
        return 0;
    }
    dgx_screen_t *_scr = dgx_vscreen_init(_scr_src->width, _scr_src->height, _scr_src->color_bits, _scr_src->rgb_order);
    if (!_scr) return 0;
    dgx_vscreen_t *scr_src = (dgx_vscreen_t *)_scr_src;
    dgx_vscreen_t *scr = (dgx_vscreen_t *)_scr;
    uint32_t csize = dgx_color_points_to_bytes(_scr_src->color_bits, _scr_src->width * _scr_src->height);
    memcpy(scr->v_array, scr_src->v_array, csize);
    return _scr;
}
