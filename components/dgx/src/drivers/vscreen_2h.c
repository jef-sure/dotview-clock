#include <string.h>

#include "dgx_arch.h"
#include "dgx_bits.h"
#include "dgx_screen_functions.h"
#include "drivers/dgx_lcd_init.h"
#include "drivers/vscreen.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct _dgx_vscreen_2h_t {
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
    dgx_screen_t *left_screen;
    dgx_screen_t *right_screen;
} dgx_vscreen_2h_t;

static const char TAG[] = "DGX V-Screen 2H";

// Initialize the display

static void dgx_vscreen_2h_update_area(dgx_screen_t *_scr, int left, int right, int top, int bottom) {
    dgx_vscreen_2h_t *scr = (dgx_vscreen_2h_t *)_scr;
    int ll = DGX_MAX(0, left);
    int lr = DGX_MIN(scr->left_screen->width - 1, right);
    int rl = DGX_MAX(scr->left_screen->width, left);
    int rr = DGX_MIN(scr->left_screen->width + scr->right_screen->width - 1, right);
    int lw = lr - ll + 1;
    int rw = rr - rl + 1;
    if (scr->left_screen->screen_subtype == DgxPhysicalScreenWithBus && scr->right_screen->screen_subtype == DgxPhysicalScreenWithBus &&
        scr->left_screen->color_bits == 16 && lw == rw && lw == scr->left_screen->width && rw == scr->right_screen->width) {
        dgx_bus_protocols_t *lbus = ((dgx_screen_with_bus_t *)scr->left_screen)->bus;
        dgx_bus_protocols_t *rbus = ((dgx_screen_with_bus_t *)scr->right_screen)->bus;
        size_t draw_buffer_len = DGX_MIN(lbus->draw_buffer_len, rbus->draw_buffer_len) / 2;
        top = DGX_MAX(0, top);
        bottom = DGX_MIN(scr->base.height - 1, bottom);
        if (top > bottom) return;
        scr->left_screen->set_area(scr->left_screen, ll, lr, top, bottom);
        scr->right_screen->set_area(scr->right_screen, rl - scr->left_screen->width, rr - scr->left_screen->width, top, bottom);
        scr->left_screen->wait_buffer(scr->left_screen);
        scr->right_screen->wait_buffer(scr->right_screen);
        uint16_t *lbuf = (uint16_t *)lbus->draw_buffer;
        uint16_t *rbuf = (uint16_t *)rbus->draw_buffer;
        for (int br = top; br <= bottom; ++br) {
            uint32_t offset_src = _scr->width * br;
            uint16_t *lp_src = (uint16_t *)(scr->v_array + offset_src * 2);
            uint16_t *rp_src = lp_src + scr->left_screen->width;
            for (int bc = 0; bc < lw; ++bc) {
                *lbuf++ = *lp_src++;
                *rbuf++ = *rp_src++;
                uint32_t filled_size = lbuf - (uint16_t *)lbus->draw_buffer;
                if (filled_size >= draw_buffer_len - 1) {
                    scr->left_screen->write_area(scr->left_screen, lbus->draw_buffer, 16u * filled_size);
                    scr->right_screen->write_area(scr->right_screen, rbus->draw_buffer, 16u * filled_size);
                    scr->left_screen->wait_buffer(scr->left_screen);
                    scr->right_screen->wait_buffer(scr->right_screen);
                    lbuf = (uint16_t *)lbus->draw_buffer;
                    rbuf = (uint16_t *)rbus->draw_buffer;
                }
            }
        }
        if (lbuf != (uint16_t *)lbus->draw_buffer) {
            scr->left_screen->write_area(scr->left_screen, lbus->draw_buffer, 16u * (lbuf - (uint16_t *)lbus->draw_buffer));
            scr->right_screen->write_area(scr->right_screen, rbus->draw_buffer, 16u * (lbuf - (uint16_t *)lbus->draw_buffer));
        }
    } else {
        if (lw > 0) {
            dgx_vscreen_region_to_screen(scr->left_screen, ll, top, _scr, ll, top, lw, bottom - top + 1);
        }
        if (rw > 0) {
            dgx_vscreen_region_to_screen(scr->right_screen, rl - scr->left_screen->width, top, _scr, rl, top, rw, bottom - top + 1);
        }
    }
}

dgx_screen_t *dgx_vscreen_2h_init(dgx_screen_t *left_screen, dgx_screen_t *right_screen) {
    dgx_vscreen_t *vscr = (dgx_vscreen_t *)dgx_vscreen_init(1, 1, left_screen->color_bits, left_screen->rgb_order);
    if (!vscr) {
        ESP_LOGE(TAG, "Screen structure memory allocation failed");
        return 0;
    }
    dgx_vscreen_2h_t *scr = (dgx_vscreen_2h_t *)calloc(1, sizeof(dgx_vscreen_2h_t));
    if (!scr) {
        ESP_LOGE(TAG, "Screen structure memory allocation failed");
        return 0;
    }
    scr->base.cg_row_shift = 0;
    scr->base.cg_col_shift = 0;
    scr->base.width = left_screen->width + right_screen->width;
    scr->base.height = DGX_MAX(left_screen->height, right_screen->height);
    scr->base.color_bits = left_screen->color_bits;
    scr->base.rgb_order = left_screen->rgb_order;
    scr->base.screen_name = "V-SCREEN 2H";
    scr->base.screen_submodel = 0;
    scr->base.screen_subtype = DgxVirtualBackScreen;
    scr->base.circle = vscr->base.circle;
    scr->base.destroy = vscr->base.destroy;
    scr->base.set_pixel = vscr->base.set_pixel;
    scr->base.fill_rectangle = vscr->base.fill_rectangle;
    scr->base.draw_line = vscr->base.draw_line;
    scr->base.get_pixel = vscr->base.get_pixel;
    scr->base.set_area = vscr->base.set_area;
    scr->base.read_area = vscr->base.read_area;
    scr->base.write_area = vscr->base.write_area;
    scr->base.wait_buffer = vscr->base.wait_buffer;
    scr->base.solid_circle = vscr->base.solid_circle;
    scr->base.update_screen = dgx_vscreen_2h_update_area;
    scr->left_screen = left_screen;
    scr->right_screen = right_screen;
    dgx_screen_destroy((dgx_screen_t **)&vscr);
    uint32_t asize = dgx_color_points_to_bytes(scr->base.color_bits, scr->base.width * scr->base.height);

    scr->v_array = calloc(asize, 1);
    if (!scr->v_array) {
        ESP_LOGE(TAG, "Impossible to allocation memory");
        free(scr);
        return 0;
    }
    return (dgx_screen_t *)scr;
}
