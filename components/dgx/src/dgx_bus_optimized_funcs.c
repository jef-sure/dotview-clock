/*
 * dgx_slow_bus_optimized_funcs.c
 *
 *  Created on: 16.12.2022
 *      Author: KYMJ
 */

#include "dgx_screen.h"
#include "dgx_bits.h"
#include "dgx_screen_with_bus.h"
#include <malloc.h>

/*
 * Slow bus optimized functions
 */

#include "esp_log.h"

// static const char TAG[] = "DGX SLOW BUS FUNCTIONS";

void dgx_scr_fill_rectangle_sb(dgx_screen_t *scr, int x, int y, int w, int h, uint32_t color)
{
    if (w < 0)
    {
        x += w;
        w = -w;
    }
    if (x < 0)
    {
        w += x;
        x = 0;
    }
    if (x + w > scr->width)
    {
        w = scr->width - x;
    }
    if (h < 0)
    {
        y += h;
        h = -h;
    }
    if (y < 0)
    {
        h += y;
        y = 0;
    }
    if (y + h > scr->height)
    {
        h = scr->height - y;
    }
    if (w <= 0 || x + w <= 0 || x >= scr->width || h <= 0 || y + h <= 0 || y >= scr->height)
        return;
    scr->in_progress++;
    uint32_t fill_scr_size = h * w;
    uint8_t *draw_buffer;
    size_t draw_buffer_len;
    if (scr->screen_subtype == DgxPhysicalScreenWithBus)
    {
        dgx_screen_with_bus_t *_scr = (dgx_screen_with_bus_t *)scr;
        dgx_bus_protocols_t *bus = _scr->bus;
        draw_buffer_len = bus->draw_buffer_len;
        draw_buffer = bus->draw_buffer;
    }
    else
    { // bad idea ...
        draw_buffer_len = dgx_color_points_to_bytes(scr->color_bits, fill_scr_size);
        draw_buffer = malloc(draw_buffer_len);
    }
    uint8_t *lp = draw_buffer;
    uint32_t pic = dgx_bytes_to_color_points(scr->color_bits, draw_buffer_len);
    if (pic > fill_scr_size)
        pic = fill_scr_size;
    if (scr->wait_buffer)
        scr->wait_buffer(scr);
    DGX_FILL_BUFFER(scr->color_bits, lp, pic, color);
    uint32_t pic_lenbits = dgx_color_points_to_bytes(scr->color_bits, pic) * 8u;
    scr->set_area(scr, x, x + w - 1, y, y + h - 1);
    while (fill_scr_size != 0)
    {
        uint32_t lenbits;
        if (fill_scr_size >= pic)
        {
            lenbits = pic_lenbits;
            fill_scr_size -= pic;
        }
        else
        {
            lenbits = dgx_color_points_to_bytes(scr->color_bits, fill_scr_size) * 8u;
            fill_scr_size = 0;
        }
        scr->write_area(scr, draw_buffer, lenbits);
        if (fill_scr_size && scr->wait_buffer)
            scr->wait_buffer(scr);
    }
    if (scr->screen_subtype != DgxPhysicalScreenWithBus)
    {
        free(draw_buffer);
    }
    if (!--scr->in_progress)
        scr->update_screen(scr, x, x + w - 1, y, y + h - 1);
}

void dgx_scr_set_pixel_sb(dgx_screen_t *scr, int x, int y, uint32_t color)
{
    if (x < 0 || x >= scr->width || y < 0 || y >= scr->height)
        return;
    uint8_t draw_buffer[4];
    uint8_t *lp = draw_buffer;
    if (scr->wait_buffer)
        scr->wait_buffer(scr);
    DGX_FILL_BUFFER(scr->color_bits, lp, 1, color);
    scr->set_area(scr, x, x, y, y);
    scr->write_area(scr, draw_buffer, dgx_color_points_to_bytes(scr->color_bits, 1) * 8u);
    if (!scr->in_progress)
        scr->update_screen(scr, x, x, y, y);
}

uint32_t dgx_scr_get_pixel_sb(dgx_screen_t *_scr, int x, int y)
{
    if (_scr->screen_subtype == DgxPhysicalScreenWithBus)
    {
        dgx_screen_with_bus_t *scr = (dgx_screen_with_bus_t *)_scr;
        dgx_bus_protocols_t *bus = scr->bus;
        if (bus->rcmd_send != 0)
        {
            bus->set_area_async(bus, x, x, y, y, 0);
            // somehow we should receive data here
            return 0;
        }
    }
    return 0;
}

// NOP for physical not virtually backed screens

void dgx_scr_update_area_sb(struct _dgx_screen_t *scr, int left, int right, int top, int bottom)
{
}

void dgx_scr_with_bus_destroy(struct _dgx_screen_t **pscr)
{
    if (*pscr)
    {
        if ((*pscr)->screen_subtype == DgxPhysicalScreenWithBus)
        {
            dgx_bus_protocols_t *bus = ((dgx_screen_with_bus_t *)(*pscr))->bus;
            bus->dispose(bus);
        }
        free(*pscr);
        *pscr = 0;
    }
}

void dgx_scr_line_sb(dgx_screen_t *scr, int x1, int y1, int x2, int y2, uint32_t color)
{
    int area_left = DGX_MIN(x1, x2);
    int area_right = DGX_MAX(x1, x2);
    int area_top = DGX_MIN(y1, y2);
    int area_bottom = DGX_MAX(y1, y2);
    if (area_bottom < 0 || area_top >= scr->height || area_left >= scr->width || area_right < 0)
        return;
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (dx == 0 && dy == 0)
    {
        scr->set_pixel(scr, x1, y1, color);
        return;
    }
    if (dx == 0 || dy == 0)
    {
        if (dx < 0)
        {
            x1 = x2;
            dx = -dx;
        }
        if (dy < 0)
        {
            y1 = y2;
            dy = -dy;
        }
        scr->fill_rectangle(scr, x1, y1, dx + 1, dy + 1, color);
        return;
    }
    int sy = dy < 0 ? -1 : 1;
    int sx = dx < 0 ? -1 : 1;
    dx = DGX_ABS(dx);
    dy = DGX_ABS(dy);
    ++dx;
    ++dy;
    scr->in_progress++;
    if (dx == dy)
    {
        while (1)
        {
            scr->set_pixel(scr, x1, y1, color);
            if (x1 == x2)
                break;
            x1 += sx;
            y1 += sy;
        };
        if (!--scr->in_progress)
            scr->update_screen(scr, area_left, area_right, area_top, area_bottom);
        return;
    }
    int err;
    if (dx > dy)
    {
        err = dy;
        dy *= 2;
        dx *= 2;
        int sp = x1;
        while (1)
        {
            err += dy;
            if (err >= dx || x1 == x2)
            {
                scr->fill_rectangle(scr, (x1 > sp ? sp : x1), y1, (x1 > sp ? x1 - sp : sp - x1) + 1, 1, color);
                if (x1 == x2)
                    break;
                sp = x1 + sx;
                err -= dx;
                y1 += sy;
            }
            x1 += sx;
        }
    }
    else
    {
        err = dx;
        dy *= 2;
        dx *= 2;
        int sp = y1;
        while (1)
        {
            err += dx;
            if (err >= dy || y1 == y2)
            {
                scr->fill_rectangle(scr, x1, (y1 > sp ? sp : y1), 1, (y1 > sp ? y1 - sp : sp - y1) + 1, color);
                if (y1 == y2)
                    break;
                sp = y1 + sy;
                err -= dy;
                x1 += sx;
            }
            y1 += sy;
        }
    }
    if (!--scr->in_progress)
        scr->update_screen(scr, area_left, area_right, area_top, area_bottom);
}

void dgx_scr_circle_sb(dgx_screen_t *scr, int x, int y, int r, uint32_t color)
{
    if (r < 0)
        return;
    int area_left = x - r;
    int area_right = x + r;
    int area_top = y - r;
    int area_bottom = y + r;
    if (area_bottom < 0 || area_top >= scr->height || area_left >= scr->width || area_right < 0)
        return;
    if (r == 0)
    {
        scr->set_pixel(scr, x, y, color);
        return;
    }
    int xs = 0;
    int ys = r;
    int dx = 2;
    int dy = 4 * r - 2;
    int error = 0;
    scr->in_progress++;
    scr->set_pixel(scr, x, y + r, color);
    scr->set_pixel(scr, x, y - r, color);
    while (ys > 0)
    {
        if (dx < dy)
        {
            ++xs;
            error -= dx;
            dx += 4;
            if (-error > dy / 2)
            {
                --ys;
                error += dy;
                dy -= 4;
            }
        }
        else if (dy < dx)
        {
            --ys;
            error += dy;
            dy -= 4;
            if (error > dx / 2)
            {
                ++xs;
                error -= dx;
                dx += 4;
            }
        }
        else
        {
            ++xs;
            --ys;
            error += dy - dx;
            dx += 4;
            dy -= 4;
        }
        scr->set_pixel(scr, x + xs, y + ys, color);
        scr->set_pixel(scr, x - xs, y + ys, color);
        if (ys != 0)
        {
            scr->set_pixel(scr, x - xs, y - ys, color);
            scr->set_pixel(scr, x + xs, y - ys, color);
        }
    }
    if (!--scr->in_progress)
        scr->update_screen(scr, area_left, area_right, area_top, area_bottom);
}

void dgx_scr_solid_circle_sb(dgx_screen_t *scr, int x, int y, int r, uint32_t color)
{
    if (r < 0)
        return;
    int area_left = x - r;
    int area_right = x + r;
    int area_top = y - r;
    int area_bottom = y + r;
    if (area_bottom < 0 || area_top >= scr->height || area_left >= scr->width || area_right < 0)
        return;
    if (r == 0)
    {
        scr->set_pixel(scr, x, y, color);
        return;
    }
    int xs = 0;
    int ys = r;
    int dx = 2;
    int dy = 4 * r - 2;
    int error = 0;
    int px = xs;
    scr->in_progress++;
    scr->fill_rectangle(scr, x, y - ys, 1, 2 * ys + 1, color);
    while (ys > 0)
    {
        if (dx < dy)
        {
            ++xs;
            error -= dx;
            dx += 4;
            if (-error > dy / 2)
            {
                --ys;
                error += dy;
                dy -= 4;
            }
        }
        else if (dy < dx)
        {
            --ys;
            error += dy;
            dy -= 4;
            if (error > dx / 2)
            {
                ++xs;
                error -= dx;
                dx += 4;
            }
        }
        else
        {
            ++xs;
            --ys;
            error += dy - dx;
            dx += 4;
            dy -= 4;
        }
        if (px != xs)
        {
            scr->fill_rectangle(scr, x + xs, y - ys, 1, 2 * ys + 1, color);
            scr->fill_rectangle(scr, x - xs, y - ys, 1, 2 * ys + 1, color);
            px = xs;
        }
    }
    if (!--scr->in_progress)
        scr->update_screen(scr, area_left, area_right, area_top, area_bottom);
}

void dgx_scr_set_area_sb(dgx_screen_t *scr, uint16_t left, uint16_t right, uint16_t top, uint16_t bottom)
{
    if (scr->screen_subtype == DgxPhysicalScreenWithBus)
    {
        dgx_bus_protocols_t *bus = ((dgx_screen_with_bus_t *)scr)->bus;
        if (left > right)
            DGX_INT_SWAP(left, right);
        if (top > bottom)
            DGX_INT_SWAP(top, bottom);
        if (left >= scr->width || top >= scr->height)
            return;
        if (right > scr->width - 1)
            right = scr->width - 1;
        if (bottom > scr->height - 1)
            bottom = scr->height - 1;
        left += scr->cg_col_shift;
        right += scr->cg_col_shift;
        top += scr->cg_row_shift;
        bottom += scr->cg_row_shift;
        bus->set_area_async(bus, left, right, top, bottom, true);
    }
}

void dgx_scr_write_area_sb(dgx_screen_t *scr, uint8_t *data, uint32_t lenbits)
{
    if (scr->screen_subtype == DgxPhysicalScreenWithBus)
    {
        dgx_bus_protocols_t *bus = ((dgx_screen_with_bus_t *)scr)->bus;
        bus->write_data_async(bus, data, lenbits);
    }
}

uint32_t dgx_scr_read_area_sb(dgx_screen_t *scr, uint8_t *data, uint32_t lenbits)
{
    if (scr->screen_subtype == DgxPhysicalScreenWithBus)
    {
        dgx_bus_protocols_t *bus = ((dgx_screen_with_bus_t *)scr)->bus;
        if (bus->rcmd_send != 0)
        {
            return bus->read_data(bus, data, lenbits);
        }
    }
    return 0;
}

void dgx_scr_wait_buffer_sb(dgx_screen_t *scr)
{
    if (scr->screen_subtype == DgxPhysicalScreenWithBus)
    {
        dgx_bus_protocols_t *bus = ((dgx_screen_with_bus_t *)scr)->bus;
        bus->sync_write(bus);
    }
}

void dgx_scr_init_slow_bus_optimized_funcs(dgx_screen_t *scr)
{
    scr->circle = dgx_scr_circle_sb;
    scr->destroy = dgx_scr_with_bus_destroy;
    scr->set_pixel = dgx_scr_set_pixel_sb;
    scr->fill_rectangle = dgx_scr_fill_rectangle_sb;
    scr->draw_line = dgx_scr_line_sb;
    scr->get_pixel = dgx_scr_get_pixel_sb;
    scr->set_area = dgx_scr_set_area_sb;
    scr->read_area = dgx_scr_read_area_sb;
    scr->write_area = dgx_scr_write_area_sb;
    scr->wait_buffer = dgx_scr_wait_buffer_sb;
    scr->solid_circle = dgx_scr_solid_circle_sb;
    scr->update_screen = dgx_scr_update_area_sb;
    scr->dir_x = DgxScreenLeftRight;
    scr->dir_y = DgxScreenTopBottom;
    scr->swap_xy = false;
}

/*
 * This algorithm is faster when pixel set function is fast
 *
 * void dgx_scr_line_sb(dgx_screen_t *scr, int x1, int y1, int x2, int y2, uint32_t color) {
 int dx = x2 - x1;
 int dy = y2 - y1;
 if (dx == 0 && dy == 0) {
 dgx_scr_set_pixel_sb(scr, x1, y1, color);
 return;
 }
 if (dx == 0 || dy == 0) {
 if (dx < 0) {
 x1 = x2;
 dx = -dx;
 }
 if (dy < 0) {
 y1 = y2;
 dy = -dy;
 }
 dgx_scr_fill_rectangle_sb(scr, x1, y1, dx + 1, dy + 1, color);
 return;
 }
 int sy = dy < 0 ? -1 : 1;
 int sx = dx < 0 ? -1 : 1;
 dx = DGX_ABS(dx);
 dy = DGX_ABS(dy);
 ++dx;
 ++dy;
 if (dx == dy) {
 while (1) {
 dgx_scr_set_pixel_sb(scr, x1, y1, color);
 if (x1 == x2) break;
 x1 += sx;
 y1 += sy;
 };
 return;
 }
 int err = 0;
 if (dx > dy) {
 while (1) {
 dgx_scr_fill_rectangle_sb(scr, x1, y1, 1, 1, color);
 if (x1 == x2) break;
 x1 += sx;
 err += dy;
 if (err >= dx) {
 err -= dx;
 y1 += sy;
 }
 }
 } else {
 while (1) {
 dgx_scr_fill_rectangle_sb(scr, x1, y1, 1, 1, color);
 if (y1 == y2) break;
 y1 += sy;
 err += dx;
 if (err >= dy) {
 err -= dy;
 x1 += sx;
 }
 }
 }
 }
 *
 */
