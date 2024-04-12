#include <stdlib.h>

#include "dgx_draw.h"
#include "dgx_font.h"
#include "esp_log.h"

static const char TAG[] = "DGX FONT";

uint32_t decodeUTF8next(const char *chr, size_t *idx) {
    uint32_t c = chr[*idx];
    if (c < 0x80) {
        if (c) ++*idx;
        return c;
    }
    ++*idx;
    uint8_t len;
    if ((c & 0xE0) == 0xC0) {
        c &= 0x1f;
        len = 2;
    } else if ((c & 0xF0) == 0xE0) {
        c &= 0xf;
        len = 3;
    } else if ((c & 0xF8) == 0xF0) {
        c &= 0x7;
        len = 4;
    } else {
        return c;
    }
    while (--len) {
        uint32_t nc = chr[(*idx)++];
        if ((nc & 0xC0) == 0x80) {
            c <<= 6;
            c |= nc & 0x3f;
        } else {
            break;
        }
    }
    return c;
}

const glyph_t *dgx_font_find_glyph(uint32_t codePoint, dgx_font_t *font, int16_t *xAdvance) {
    const glyph_t *g = 0, *fg = 0;
    for (const glyph_array_t *r = font->glyph_ranges; r->number; ++r) {
        if (!fg) fg = r->glyphs;
        if (codePoint >= r->first && codePoint < r->first + r->number) {
            g = r->glyphs + (codePoint - r->first);
            break;
        }
    }
    if (g == 0 && fg) {
        *xAdvance = fg->xAdvance;
        return 0;
    }
    if (g == 0 && fg == 0) {
        *xAdvance = 0;
        return 0;
    }
    *xAdvance = g->xAdvance;
    return g;
}

void dgx_font_make_point8(dgx_screen_t *vpoint) {
    int point_size = vpoint->width;
    if (point_size == 1) {
        dgx_set_pixel(vpoint, 0, 0, 255);
    } else if (point_size == 2) {
        dgx_set_pixel(vpoint, 0, 0, 255);
        dgx_set_pixel(vpoint, 0, 1, 255);
        dgx_set_pixel(vpoint, 1, 0, 255);
        dgx_set_pixel(vpoint, 1, 1, 255);
    } else if (point_size == 3) {
        dgx_set_pixel(vpoint, 1, 1, 255);
        dgx_set_pixel(vpoint, 1, 0, 128);
        dgx_set_pixel(vpoint, 0, 1, 128);
        dgx_set_pixel(vpoint, 2, 1, 128);
        dgx_set_pixel(vpoint, 1, 2, 128);
        dgx_set_pixel(vpoint, 0, 0, 96);
        dgx_set_pixel(vpoint, 2, 0, 96);
        dgx_set_pixel(vpoint, 0, 2, 96);
        dgx_set_pixel(vpoint, 2, 2, 96);
    } else if (point_size == 4) {
        dgx_set_pixel(vpoint, 1, 1, 255);
        dgx_set_pixel(vpoint, 1, 2, 255);
        dgx_set_pixel(vpoint, 2, 1, 255);
        dgx_set_pixel(vpoint, 2, 2, 255);
        dgx_set_pixel(vpoint, 1, 0, 128);
        dgx_set_pixel(vpoint, 2, 0, 128);
        dgx_set_pixel(vpoint, 1, 3, 128);
        dgx_set_pixel(vpoint, 2, 3, 128);
        dgx_set_pixel(vpoint, 0, 1, 128);
        dgx_set_pixel(vpoint, 0, 2, 128);
        dgx_set_pixel(vpoint, 3, 1, 128);
        dgx_set_pixel(vpoint, 3, 2, 128);
        dgx_set_pixel(vpoint, 0, 0, 96);
        dgx_set_pixel(vpoint, 3, 0, 96);
        dgx_set_pixel(vpoint, 0, 3, 96);
        dgx_set_pixel(vpoint, 3, 3, 96);
    } else {
        if (point_size % 2 == 1) {
            for (int16_t r = vpoint->width / 2; r >= 1; --r) {
                uint8_t v = (vpoint->width / 2 - r + 1) * 510.0f / vpoint->width;
                dgx_solid_circle(vpoint, vpoint->width / 2, vpoint->height / 2, r, v);
            }
        } else {
            for (int16_t r = vpoint->width / 2 - 1; r >= 1; --r) {
                uint8_t v = (vpoint->width / 2 - r) * 255.0f / (vpoint->width / 2 - 1);
                dgx_solid_circle(vpoint, vpoint->width / 2, vpoint->height / 2, r, v);
                dgx_solid_circle(vpoint, vpoint->width / 2 - 1, vpoint->height / 2, r, v);
                dgx_solid_circle(vpoint, vpoint->width / 2, vpoint->height / 2 - 1, r, v);
                dgx_solid_circle(vpoint, vpoint->width / 2 - 1, vpoint->height / 2 - 1, r, v);
            }
        }
    }
}

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
) {
    if (!draw_func) {
        int16_t xAdvance;
        const glyph_t *g = dgx_font_find_glyph(codePoint, font, &xAdvance);
        if (!g) return xAdvance;
        if (font->f_type == DGX_FONT_BITMAP_LINES || font->f_type == DGX_FONT_BITMAP_STREAM) {
            dgx_bw_bitmap_t bmap = dgx_bw_bitmap_make_of((uint8_t *)g->bitmap, g->width, g->height, font->f_type == DGX_FONT_BITMAP_STREAM);
            int left = 0;
            int right = left + g->width - 1;
            int top = 0;
            int bottom = top + g->height - 1;
            if (!scale) scale = 1;
            int x_shift = g->xOffset * scale;
            int y_shift = g->yOffset * scale;
            if (swap_xy) {
                int t = x_shift;
                x_shift = y_shift;
                y_shift = t;
                t = right;
                right = bottom;
                bottom = t;
            }
            dgx_point_2d_t current_point = _dgx_start_area_pixel(left, right, top, bottom, xdir, ydir);
            for (int by = 0; by < g->height; ++by) {
                for (int bx = 0; bx < g->width; bx++) {
                    bool pix = dgx_bw_bitmap_get_pixel(&bmap, bx, by);
                    if (pix) {
                        dgx_fill_rectangle(scr, x + x_shift + current_point.x * scale, y + y_shift + current_point.y * scale, scale, scale, color);
                    }
                    current_point = _dgx_move_to_next_area_pixel(current_point, left, right, top, bottom, xdir, ydir, swap_xy);
                }
            }
        } else {
            int x_shift = g->xOffset * scale;
            int y_shift = g->yOffset * scale;
            if (param != NULL) {
                dgx_font_sym8_params_t *vParams = (dgx_font_sym8_params_t *)param;
                for (int i = 0; i < g->number_of_dots; i++) {
                    int px = g->dots[i].x * scale + x_shift;
                    int py = g->dots[i].y * scale + y_shift;
                    if (swap_xy) {
                        int t = px;
                        px = py;
                        py = t;
                    }
                    vParams->dot_func(scr, x + px, y + py, vParams);
                }
            } else {
                for (int i = 0; i < g->number_of_dots; i++) {
                    int px = g->dots[i].x * scale + x_shift;
                    int py = g->dots[i].y * scale + y_shift;
                    if (swap_xy) {
                        int t = px;
                        px = py;
                        py = t;
                    }
                    dgx_fill_rectangle(scr, x + px, y + py, scale, scale, color);
                }
            }
        }
        return xAdvance * scale;
    } else {
        return draw_func(scr, x, y, codePoint, color, xdir, ydir, swap_xy, scale, font, param);
    }
}

int dgx_font_string_bounds(const char *str, dgx_font_t *font, int16_t *ycorner, int16_t *height) {
    size_t idx = 0;
    int16_t width = 0, yb = 0, yt = 0;
    bool is_first = true;
    while (str[idx]) {
        uint32_t cp = decodeUTF8next(str, &idx);
        int16_t xAdvance;
        const glyph_t *g = dgx_font_find_glyph(cp, font, &xAdvance);
        width += xAdvance;
        if (g) {
            int16_t bl = g->yOffset;
            int16_t bh = g->height + g->yOffset;
            if (is_first || bl < yt) yt = bl;
            if (is_first || bh > yb) yb = bh;
            is_first = false;
        }
    }
    *ycorner = yt;
    *height = yb - yt;
    return width;
}

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
) {
    size_t idx = 0;
    idx = 0;
    while (str[idx]) {
        uint32_t cp = decodeUTF8next(str, &idx);
        int16_t offset = dgx_font_char_to_screen(scr, x, y, cp, color, xdir, ydir, swap_xy, scale, font, draw_func, param);
        if (swap_xy) {
            y += offset * ydir;
        } else {
            x += offset * xdir;
        }
    }
}

#define DGX_MAX(a, b) ((a) < (b) ? b : a)

static dgx_font_dot_t *_dgx_dots_from_bitmap(const glyph_t *g, bool is_stream, int *number_of_dots) {
    if (!g) {
        *number_of_dots = 0;
        return NULL;
    }
    int count = 0;
    dgx_bw_bitmap_t bmap = dgx_bw_bitmap_make_of((uint8_t *)g->bitmap, g->width, g->height, is_stream);
    for (int by = 0; by < g->height; ++by) {
        for (int bx = 0; bx < g->width; bx++) {
            bool pix = dgx_bw_bitmap_get_pixel(&bmap, bx, by);
            count += (int)pix;
        }
    }
    *number_of_dots = count;
    dgx_font_dot_t *ret = calloc(sizeof(dgx_font_dot_t), count);
    if (!ret) {
        ESP_LOGE(TAG, "Memory allocation for temporary dgx_font_dot_t array failed");
        *number_of_dots = 0;
        return NULL;
    }
    count = 0;
    for (int by = 0; by < g->height; ++by) {
        for (int bx = 0; bx < g->width; bx++) {
            bool pix = dgx_bw_bitmap_get_pixel(&bmap, bx, by);
            if (pix) {
                ret[count++] = dgx_font_dot_new(bx, by);
            }
        }
    }
    return ret;
}

dgx_font_symbol_morph_t *dgx_font_make_morph_struct(  //
    dgx_font_t *font,                                 //
    uint32_t fromCP,                                  //
    uint32_t toCP,                                    //
    int x,                                            //
    int y,                                            //
    int scale                                         //
) {
    dgx_font_symbol_morph_t *ret = calloc(sizeof(*ret), 1);
    if (!ret) {
        ESP_LOGE(TAG, "Memory allocation for dgx_font_symbol_morph_t failed");
        return NULL;
    }
    ret->fromCP = fromCP;
    ret->toCP = toCP;
    ret->is_from_empty = true;
    ret->is_to_empty = true;
    int16_t xAdvance;
    const glyph_t *gFrom = dgx_font_find_glyph(fromCP, font, &xAdvance);
    const glyph_t *gTo = dgx_font_find_glyph(toCP, font, &xAdvance);
    int from_number_of_dots;
    int to_number_of_dots;
    const dgx_font_dot_t *from_dots;
    const dgx_font_dot_t *to_dots;
    dgx_font_dot_t *from_dots_tmp = NULL;
    dgx_font_dot_t *to_dots_tmp = NULL;
    int max_sym_height;
    int max_sym_width;
    int x_shift_from = gFrom ? gFrom->xOffset * scale : 0;
    int y_shift_from = gFrom ? gFrom->yOffset * scale : 0;
    int x_shift_to = gTo ? gTo->xOffset * scale : 0;
    int y_shift_to = gTo ? gTo->yOffset * scale : 0;
    max_sym_height = gFrom ? gFrom->height : 0;
    max_sym_width = gFrom ? gFrom->width : 0;
    max_sym_height = gTo ? DGX_MAX(gTo->height, max_sym_height) : 0;
    max_sym_width = gTo ? DGX_MAX(gTo->width, max_sym_width) : 0;
    if (font->f_type == DGX_FONT_DOTS) {
        from_dots = gFrom ? gFrom->dots : NULL;
        from_number_of_dots = gFrom ? gFrom->number_of_dots : 0;
        to_dots = gTo ? gTo->dots : NULL;
        to_number_of_dots = gTo ? gTo->number_of_dots : 0;
    } else {
        from_dots = from_dots_tmp = _dgx_dots_from_bitmap(gFrom, font->f_type == DGX_FONT_BITMAP_STREAM, &from_number_of_dots);
        to_dots = to_dots_tmp = _dgx_dots_from_bitmap(gTo, font->f_type == DGX_FONT_BITMAP_STREAM, &to_number_of_dots);
    }
    if (from_number_of_dots == 0 && to_number_of_dots == 0) return ret;
    if (max_sym_height == 0 || max_sym_width == 0) return ret;
    ret->is_from_empty = from_number_of_dots == 0;
    ret->is_to_empty = to_number_of_dots == 0;
    int maxp = DGX_MAX(from_number_of_dots, to_number_of_dots);
    ret->number_of_dots = maxp;
    ret->m_start = (dgx_point_2d_t *)calloc(sizeof(dgx_point_2d_t), maxp * 2);
    if (!ret->m_start) {
        ESP_LOGE(TAG, "Memory allocation for dots array failed");
        free(ret);
        return NULL;
    }
    ret->m_end = ret->m_start + maxp;
    for (int i = 0; i < maxp; i++) {
        if (i < from_number_of_dots) {
            ret->m_start[i].x = x + x_shift_from + from_dots[i].x * scale;
            ret->m_start[i].y = y + y_shift_from + from_dots[i].y * scale;
        } else if (from_number_of_dots != 0) {
            ret->m_start[i].x = ret->m_start[from_number_of_dots - 1].x;
            ret->m_start[i].y = ret->m_start[from_number_of_dots - 1].y;
        }
        if (i < to_number_of_dots) {
            ret->m_end[i].x = x + x_shift_to + to_dots[i].x * scale;
            ret->m_end[i].y = y + y_shift_to + to_dots[i].y * scale;
        } else if (to_number_of_dots != 0) {
            ret->m_end[i].x = ret->m_end[to_number_of_dots - 1].x;
            ret->m_end[i].y = ret->m_end[to_number_of_dots - 1].y;
        }
    }
    if (from_number_of_dots == 0) {
        for (int i = 0; i < maxp; i++) {
            ret->m_start[i].x = ret->m_end[0].x;
            ret->m_start[i].y = ret->m_end[0].y;
        }
    }
    if (to_number_of_dots == 0) {
        for (int i = 0; i < maxp; i++) {
            ret->m_end[i].x = ret->m_start[from_number_of_dots - 1].x;
            ret->m_end[i].y = ret->m_start[from_number_of_dots - 1].y;
        }
    }
    if (font->f_type != DGX_FONT_DOTS) {
        free(from_dots_tmp);
        free(to_dots_tmp);
    }
    return ret;
}

void dgx_font_make_morph_struct_destroy(dgx_font_symbol_morph_t **ms) {
    free((*ms)->m_start);
    (*ms)->m_start = 0;
    free(*ms);
    *ms = 0;
}