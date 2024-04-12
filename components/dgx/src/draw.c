/*
 * draw.c
 *
 *  Created on: Apr 2, 2023
 *      Author: anton
 */

#include "dgx_draw.h"
#include "esp_log.h"

static const char TAG[] = "DGX DRAW";

void dgx_fill_rectangle(dgx_screen_t *scr, int x, int y, int w, int h, uint32_t color) {
    if (!scr->fill_rectangle) {
        ESP_LOGE(TAG, "fill_rectangle is not implemented");
        return;
    }
    scr->fill_rectangle(scr, x, y, w, h, color);
}

void dgx_draw_circle(dgx_screen_t *scr, int x, int y, int r, uint32_t color) {
    if (!scr->circle) {
        ESP_LOGE(TAG, "circle is not implemented");
        return;
    }
    scr->circle(scr, x, y, r, color);
}

void dgx_draw_line(dgx_screen_t *scr, int x1, int y1, int x2, int y2, uint32_t color) {
    if (!scr->draw_line) {
        ESP_LOGE(TAG, "draw_line is not implemented");
        return;
    }
    scr->draw_line(scr, x1, y1, x2, y2, color);
}

void dgx_solid_circle(dgx_screen_t *scr, int x, int y, int r, uint32_t color) {
    if (!scr->solid_circle) {
        ESP_LOGE(TAG, "solid_circle is not implemented");
        return;
    }
    scr->solid_circle(scr, x, y, r, color);
}

void dgx_set_pixel(dgx_screen_t *scr, int x, int y, uint32_t color) {
    if (!scr->set_pixel) {
        ESP_LOGE(TAG, "set_pixel is not implemented");
        return;
    }
    scr->set_pixel(scr, x, y, color);
}

uint32_t dgx_get_pixel(dgx_screen_t *scr, int x, int y) {
    if (!scr->get_pixel) {
        ESP_LOGE(TAG, "set_pixel is not implemented");
        return 0;
    }
    return scr->get_pixel(scr, x, y);
}
