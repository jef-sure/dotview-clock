#include <string.h>

#include "dgx_arch.h"
#include "dgx_bits.h"
#include "dgx_screen_functions.h"
#include "drivers/dgx_lcd_init.h"
#include "drivers/ili9341.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct _dgx_vscreen_t {
    dgx_screen_t base;
    uint8_t *v_array;
    dgx_orientation_t dir_x;
    dgx_orientation_t dir_y;
    bool swap_xy;
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

static void dgx_scr_v_set_area(dgx_screen_t *_scr, uint16_t left, uint16_t right, uint16_t top, uint16_t bottom) {
    dgx_vscreen_t *scr = (dgx_vscreen_t *)_scr;
    if (left >= scr->base.width || top >= scr->base.height || right >= scr->base.width || bottom >= scr->base.height) {
        ESP_LOGE(TAG, "Set area failed");
        return;
    }
    if (scr->swap_xy == true) {
        scr->area.top = left;
        scr->area.bottom = right;
        scr->area.left = top;
        scr->area.right = bottom;
        if (scr->dir_x == DgxScreenLeftRight)
            scr->area.y_offset = left;
        else
            scr->area.y_offset = right;
        if (scr->dir_y == DgxScreenTopBottom)
            scr->area.x_offset = top;
        else
            scr->area.x_offset = bottom;
    } else {
        scr->area.left = left;
        scr->area.right = right;
        scr->area.top = top;
        scr->area.bottom = bottom;
        if (scr->dir_x == DgxScreenLeftRight)
            scr->area.x_offset = left;
        else
            scr->area.x_offset = right;
        if (scr->dir_y == DgxScreenTopBottom)
            scr->area.y_offset = top;
        else
            scr->area.y_offset = bottom;
    }
}

static void _dgx_move_to_next_area_pixel(dgx_screen_t *_scr) {
    dgx_vscreen_t *scr = (dgx_vscreen_t *)_scr;
    if (scr->swap_xy == true) {
        if (scr->dir_y == DgxScreenTopBottom) {
            if (scr->area.y_offset >= scr->area.bottom) {
                scr->area.y_offset = scr->area.top;
                if (scr->dir_x == DgxScreenLeftRight)
                    if (scr->area.x_offset >= scr->area.right) {
                        scr->area.x_offset = scr->area.left;
                    } else {
                        scr->area.x_offset++;
                    }
                else {
                    if (scr->area.x_offset <= scr->area.left) {
                        scr->area.x_offset = scr->area.right;
                    } else {
                        scr->area.x_offset--;
                    }
                }
            } else {
                scr->area.y_offset++;
            }
        } else {
            if (scr->area.y_offset <= scr->area.top) {
                scr->area.y_offset = scr->area.bottom;
                if (scr->dir_x == DgxScreenLeftRight)
                    if (scr->area.x_offset >= scr->area.right) {
                        scr->area.x_offset = scr->area.left;
                    } else {
                        scr->area.x_offset++;
                    }
                else {
                    if (scr->area.x_offset <= scr->area.left) {
                        scr->area.x_offset = scr->area.right;
                    } else {
                        scr->area.x_offset--;
                    }
                }
            } else {
                scr->area.y_offset--;
            }
        }
    } else {
        if (scr->dir_x == DgxScreenLeftRight)
            if (scr->area.x_offset >= scr->area.right) {
                scr->area.x_offset = scr->area.left;
                if (scr->dir_y == DgxScreenTopBottom) {
                    if (scr->area.y_offset >= scr->area.bottom) {
                        scr->area.y_offset = scr->area.top;
                    } else {
                        scr->area.y_offset++;
                    }
                } else {
                    if (scr->area.y_offset <= scr->area.top) {
                        scr->area.y_offset = scr->area.bottom;
                    } else {
                        scr->area.y_offset--;
                    }
                }
            } else {
                scr->area.x_offset++;
            }
        else if (scr->area.x_offset <= scr->area.left) {
            scr->area.x_offset = scr->area.right;
            if (scr->dir_y == DgxScreenTopBottom) {
                if (scr->area.y_offset >= scr->area.bottom) {
                    scr->area.y_offset = scr->area.top;
                } else {
                    scr->area.y_offset++;
                }
            } else {
                if (scr->area.y_offset <= scr->area.top) {
                    scr->area.y_offset = scr->area.bottom;
                } else {
                    scr->area.y_offset--;
                }
            }
        } else {
            scr->area.x_offset--;
        }
    }
}

static void dgx_scr_v_write_area(dgx_screen_t *_scr, uint8_t *data, uint32_t lenbits) {
    dgx_vscreen_t *scr = (dgx_vscreen_t *)_scr;
    int idx = 0;
    while (lenbits >= _scr->color_bits) {
        uint32_t offset = scr->area.x_offset + scr->area.y_offset * _scr->width;
        uint8_t *lp = scr->v_array + (offset * _scr->color_bits + 7u) / 8u;
        uint32_t color = dgx_read_buf_value(_scr, &data, idx);
        ++idx;
        dgx_fill_buf_value(_scr, lp, offset, color);
        _dgx_move_to_next_area_pixel(_scr);
        lenbits -= _scr->color_bits;
    }
}

static uint32_t dgx_scr_v_read_area(dgx_screen_t *_scr, uint8_t *data, uint32_t lenbits) {
    uint32_t ret = 0;
    dgx_vscreen_t *scr = (dgx_vscreen_t *)_scr;
    int idx = 0;
    while (lenbits >= _scr->color_bits) {
        uint32_t offset = scr->area.x_offset + scr->area.y_offset * _scr->width;
        uint8_t *lp = scr->v_array + (offset * _scr->color_bits + 7u) / 8u;
        uint32_t color = dgx_read_buf_value(_scr, &lp, offset);
        data = dgx_fill_buf_value(_scr, data, idx, color);
        ++idx;
        _dgx_move_to_next_area_pixel(_scr);
        ret += _scr->color_bits;
        lenbits -= _scr->color_bits;
    }
    return ret;
}

typedef struct _dgx_2d {
    int x;
    int y;
} _dgx_2d_t;

static inline _dgx_2d_t _dgx_pixel_to_ram(dgx_vscreen_t *scr, int x, int y) {
    _dgx_2d_t raw;
    if (scr->swap_xy == true) {
        if (scr->dir_x == DgxScreenLeftRight)
            raw.x = y;
        else
            raw.x = scr->base.width - y - 1;
        if (scr->dir_y == DgxScreenTopBottom)
            raw.y = x;
        else
            raw.y = scr->base.height - x - 1;
    } else {
        if (scr->dir_x == DgxScreenLeftRight)
            raw.x = x;
        else
            raw.x = scr->base.width - x - 1;
        if (scr->dir_y == DgxScreenTopBottom)
            raw.y = y;
        else
            raw.y = scr->base.height - y - 1;
    }
    return raw;
}

static void dgx_scr_v_set_pixel(dgx_screen_t *_scr, int x, int y, uint32_t color) {
    dgx_vscreen_t *scr = (dgx_vscreen_t *)_scr;
    _dgx_2d_t rawPoint = _dgx_pixel_to_ram(scr, x, y);
    if (rawPoint.x < 0 || rawPoint.y < 0 || rawPoint.x >= _scr->width || rawPoint.y >= _scr->height) return;
    uint32_t offset = rawPoint.x + rawPoint.y * _scr->width;
    uint8_t *lp = scr->v_array + (offset * _scr->color_bits + 7u) / 8u;
    dgx_fill_buf_value(_scr, lp, offset, color);
    if (!_scr->in_progress) _scr->update_screen(scr, x, x, y, y);
}

static uint32_t dgx_scr_v_get_pixel(dgx_screen_t *_scr, int x, int y) {
    dgx_vscreen_t *scr = (dgx_vscreen_t *)_scr;
    _dgx_2d_t rawPoint = _dgx_pixel_to_ram(scr, x, y);
    if (rawPoint.x < 0 || rawPoint.y < 0 || rawPoint.x >= _scr->width || rawPoint.y >= _scr->height) return 0;
    uint32_t offset = rawPoint.x + rawPoint.y * _scr->width;
    uint8_t *lp = scr->v_array + (offset * _scr->color_bits + 7u) / 8u;
    uint32_t color = dgx_read_buf_value(_scr, &lp, offset);
    return color;
}

static void dgx_scr_v_update_area(dgx_screen_t *_scr, int left, int right, int top, int bottom) {}
static void dgx_scr_v_wait_buffer_dummy(dgx_screen_t *_scr) {}
static void dgx_scr_v_screen_destroy(dgx_screen_t **_pscr) {
    if (*_pscr) {
        dgx_vscreen_t *scr = (dgx_vscreen_t *)(*_pscr);
        free(scr->v_array);
        free(*_pscr);
        *_pscr = 0;
    }
}

static void dgx_scr_v_fill_rectangle(dgx_screen_t *_scr, int x, int y, int w, int h, uint32_t color) {
    dgx_vscreen_t *scr = (dgx_vscreen_t *)_scr;
    int dw = scr->dir_x == DgxScreenLeftRight ? w - 1 : 1 - w;
    int dh = scr->dir_y == DgxScreenTopBottom ? h - 1 : 1 - h;
    int x2;
    int y2;
    x2 = x + dw;
    y2 = y + dh;
    _dgx_2d_t rawPoint1 = _dgx_pixel_to_ram(scr, x, y);
    _dgx_2d_t rawPoint2 = _dgx_pixel_to_ram(scr, x2, y2);
    int minx = DGX_MIN(rawPoint1.x, rawPoint2.x);
    int maxx = DGX_MAX(rawPoint1.x, rawPoint2.x);
    int miny = DGX_MIN(rawPoint1.y, rawPoint2.y);
    int maxy = DGX_MAX(rawPoint1.y, rawPoint2.y);
    while (maxy >= miny) {
        uint32_t offset = minx + miny * _scr->width;
        uint8_t *lp = scr->v_array + (offset * _scr->color_bits + 7u) / 8u;
        DGX_FILL_BUFFER(_scr->color_bits, lp, (maxx - minx + 1), color);
        miny++;
    }
    if (!_scr->in_progress) _scr->update_screen(_scr, x, x + w - 1, y, y + h - 1);
}

static void dgx_scr_v_line(dgx_screen_t *_scr, int x1, int y1, int x2, int y2, uint32_t color) {
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

static void dgx_scr_v_circle(dgx_screen_t *_scr, int x0, int y0, int r, uint32_t color) {
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
static void dgx_scr_v_solid_circle(dgx_screen_t *_scr, int x0, int y0, int r, uint32_t color) {
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

void dgx_scr_v_destroy(struct _dgx_screen_t **pscr) {
    if (*pscr) {
        dgx_vscreen_t *scr = (dgx_vscreen_t *)*pscr;
        free(scr->v_array);
        free(*pscr);
        *pscr = 0;
    }
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
    scr->dir_x = DgxScreenLeftRight;
    scr->dir_y = DgxScreenTopBottom;
    scr->swap_xy = false;
    scr->base.circle = dgx_scr_v_circle;
    scr->base.destroy = dgx_scr_v_destroy;
    scr->base.set_pixel = dgx_scr_v_set_pixel;
    scr->base.fill_rectangle = dgx_scr_v_fill_rectangle;
    scr->base.draw_line = dgx_scr_v_line;
    scr->base.get_pixel = dgx_scr_v_get_pixel;
    scr->base.set_area = dgx_scr_v_set_area;
    scr->base.read_area = dgx_scr_v_read_area;
    scr->base.write_area = dgx_scr_v_write_area;
    scr->base.wait_buffer = dgx_scr_v_wait_buffer_dummy;
    scr->base.solid_circle = dgx_scr_v_solid_circle;
    scr->base.update_screen = dgx_scr_v_update_area;

    uint32_t asize = dgx_color_points_to_bytes(color_bits, width * height);
    scr->v_array = calloc(asize, 1);
    if (!scr->v_array) {
        ESP_LOGE(TAG, "Impossible to allocation memory");
        free(scr);
        return 0;
    }
    return (dgx_screen_t *)scr;
}
