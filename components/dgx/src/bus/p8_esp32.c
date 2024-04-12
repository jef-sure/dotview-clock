/*
 * p8_esp32.c
 *
 *  Created on: 14.10.2022
 *      Author: Anton Petrusevich
 */

#include <stdbool.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"

#include "bus/dgx_p8_esp32.h"

struct _dgx_p8_bus_t;

typedef void (*p8_send_byte_func)(struct _dgx_p8_bus_t *bus, uint8_t cmd, bool dc);

typedef struct _dgx_p8_bus_t {
    dgx_bus_protocols_t protocols;
    gpio_num_t dio[8];
    gpio_num_t wrio;
    gpio_num_t rdio;
    gpio_num_t csio;
    gpio_num_t dcio;
    p8_send_byte_func p8_send_byte;
    struct {
        uint16_t left;
        uint16_t right;
        uint16_t top;
        uint16_t bottom;
    } area;
} dgx_p8_bus_t;

static const char TAG[] = "DGX P8 BUS";

static uint32_t p8_gpio_mask_set[2][256];
static uint32_t p8_gpio_mask_clear[2][256];

static void p8_init_masks(dgx_p8_bus_t *bus) {
    uint16_t i;
    for (i = 0; i < 256; ++i) {
        uint32_t set[2] = { 0, 0 };
        uint32_t clear[2] = { 0, 0 };
        clear[bus->wrio >> 5] = 1 << (bus->wrio & 31);
        for (uint8_t j = 0, mask = 0x1; j < 8u; ++j, mask <<= 1) {
            if (i & mask) {
                set[bus->dio[j] >> 5] |= 1 << (bus->dio[j] & 31);
            } else {
                clear[bus->dio[j] >> 5] |= 1 << (bus->dio[j] & 31);
            }
        }
        p8_gpio_mask_set[0][i] = set[0];
        p8_gpio_mask_set[1][i] = set[1];
        p8_gpio_mask_clear[0][i] = clear[0];
        p8_gpio_mask_clear[1][i] = clear[1];
    }
}

static void p8_send_byte_low_dc(dgx_p8_bus_t *bus, uint8_t cmd, bool dc) {
    uint32_t bm = p8_gpio_mask_clear[0][cmd];
    if (!dc) bm |= 1u << bus->dcio;
    GPIO.out_w1tc = bm;
    bm = p8_gpio_mask_set[0][cmd];
    if (dc) bm |= 1u << bus->dcio;
    GPIO.out_w1ts = bm;
    GPIO.out_w1ts = 1u << bus->wrio;
}

static void p8_send_byte_mix_dc(dgx_p8_bus_t *bus, uint8_t cmd, bool dc) {
    uint32_t bm_low = p8_gpio_mask_clear[0][cmd];
    uint32_t bm_high = p8_gpio_mask_clear[1][cmd];
    if (!dc) {
        if (bus->dcio < 32) {
            bm_low |= 1u << bus->dcio;
        } else {
            bm_high |= 1u << (bus->dcio - (uint16_t) 32);
        }
    }
    if (bm_low) GPIO.out_w1tc = bm_low;
    if (bm_high) GPIO.out1_w1tc.val = bm_high;
    bm_low = p8_gpio_mask_set[0][cmd];
    bm_high = p8_gpio_mask_set[1][cmd];
    if (dc) {
        if (bus->dcio < 32) {
            bm_low |= 1u << bus->dcio;
        } else {
            bm_high |= 1u << (bus->dcio - (uint16_t) 32);
        }
    }
    if (bm_low) GPIO.out_w1ts = bm_low;
    if (bm_high) GPIO.out1_w1ts.val = bm_high;
    if (bus->wrio < 32) {
        GPIO.out_w1ts = 1u << bus->wrio;
    } else {
        GPIO.out1_w1ts.val = 1u << (bus->wrio - (uint16_t) 32);
    }
}

static void dgx_p8_send_command(struct _dgx_bus_protocols_t *_bus, uint8_t cmd) {
    dgx_p8_bus_t *bus = (dgx_p8_bus_t*) _bus;
    bus->p8_send_byte(bus, cmd, 0);
}

static void dgx_p8_send_commands(struct _dgx_bus_protocols_t *_bus, const uint8_t *cmds, uint32_t len) {
    dgx_p8_bus_t *bus = (dgx_p8_bus_t*) _bus;
    len = (len + 7u) / 8u;
    while (len--) {
        bus->p8_send_byte(bus, *cmds++, 0);
    }
}

static void dgx_p8_send_data_low(struct _dgx_bus_protocols_t *_bus, const uint8_t *data, uint32_t len) {
    dgx_p8_bus_t *bus = (dgx_p8_bus_t*) _bus;
    len = (len + 7u) / 8u;
    if (len--) {
        uint8_t cmd = *data++;
        uint32_t bm;
        GPIO.out_w1tc = p8_gpio_mask_clear[0][cmd];
        bm = p8_gpio_mask_set[0][cmd];
        bm |= 1u << bus->dcio;
        if (bm) GPIO.out_w1ts = bm;
        GPIO.out_w1ts = 1u << bus->wrio;
    }
    while (len--) {
        uint8_t cmd = *data++;
        uint32_t bm;
        GPIO.out_w1tc = p8_gpio_mask_clear[0][cmd];
        bm = p8_gpio_mask_set[0][cmd];
        if (bm) GPIO.out_w1ts = bm;
        GPIO.out_w1ts = 1u << bus->wrio;
    }
}

static void dgx_p8_send_data_mix(struct _dgx_bus_protocols_t *_bus, const uint8_t *data, uint32_t len) {
    dgx_p8_bus_t *bus = (dgx_p8_bus_t*) _bus;
    len = (len + 7u) / 8u;
    if (len--) {
        bus->p8_send_byte(bus, *data++, 1);
    }
    while (len--) {
        uint8_t cmd = *data++;
        uint32_t bm_low = p8_gpio_mask_clear[0][cmd];
        uint32_t bm_high = p8_gpio_mask_clear[1][cmd];
        if (bm_low) GPIO.out_w1tc = bm_low;
        if (bm_high) GPIO.out1_w1tc.val = bm_high;
        bm_low = p8_gpio_mask_set[0][cmd];
        bm_high = p8_gpio_mask_set[1][cmd];
        if (bm_low) GPIO.out_w1ts = bm_low;
        if (bm_high) GPIO.out1_w1ts.val = bm_high;
        if (bus->wrio < 32) {
            GPIO.out_w1ts = 1u << bus->wrio;
        } else {
            GPIO.out1_w1ts.val = 1u << (bus->wrio - (uint16_t) 32);
        }
    }
}

static void dgx_p8_set_area_std_async(struct _dgx_bus_protocols_t *_bus, uint16_t left, uint16_t right, uint16_t top, uint16_t bottom, bool readWrite) {
    dgx_p8_bus_t *bus = (dgx_p8_bus_t*) _bus;
    uint8_t bdata[4];
    if (readWrite == false) {
//        ESP_LOGI(TAG, "read support is not implemented");
        return;
    }
    if (bus->area.left != left || bus->area.right != right) {
        _bus->write_command(_bus, _bus->xcmd_set);
        bdata[0] = left >> 8;
        bdata[1] = left & 255;
        bdata[2] = right >> 8;
        bdata[3] = right & 255;
        bus->area.left = left;
        bus->area.right = right;
        _bus->write_data(_bus, bdata, 4 * 8);
    }
    if (bus->area.top != top || bus->area.bottom != bottom) {
        _bus->write_command(_bus, _bus->ycmd_set);
        bdata[0] = top >> 8;
        bdata[1] = top & 255;
        bdata[2] = bottom >> 8;
        bdata[3] = bottom & 255;
        bus->area.top = top;
        bus->area.bottom = bottom;
        _bus->write_data(_bus, bdata, 4 * 8);
    }
    _bus->write_command(_bus, (readWrite == false ? _bus->rcmd_send : _bus->wcmd_send));
}

static void dgx_p8_dispose_bus_func(struct _dgx_bus_protocols_t *_bus) {
    dgx_p8_bus_t *bus = (dgx_p8_bus_t*) _bus;
    heap_caps_free(bus); // for symmetry with heap_caps_calloc
}

void dgx_p8_set_area_func(struct _dgx_bus_protocols_t *_bus, int area_func_style) {
    _bus->set_area_async = dgx_p8_set_area_std_async;
}

static uint8_t* dgx_p8_memory_allocator(uint32_t len) {
    return (uint8_t*) malloc(len);
}

static void dgx_p8_sync_write_func(struct _dgx_bus_protocols_t *_bus) {
}

static uint32_t dgx_p8_read_data(struct _dgx_bus_protocols_t *_bus, uint8_t *data, uint32_t len) {
//    ESP_LOGI(TAG, "read support is not implemented");
    memset(data, 0, len / 8);
    return len;
}

dgx_bus_protocols_t* dgx_p8_init(gpio_num_t lcd_d0, gpio_num_t lcd_d1, gpio_num_t lcd_d2, gpio_num_t lcd_d3, gpio_num_t lcd_d4, gpio_num_t lcd_d5,
        gpio_num_t lcd_d6, gpio_num_t lcd_d7, gpio_num_t lcd_wr, gpio_num_t lcd_rd, gpio_num_t cs, gpio_num_t dc)
{
    dgx_p8_bus_t *bus = (dgx_p8_bus_t*) calloc(1, sizeof(dgx_p8_bus_t));
    if (bus == 0) {
        ESP_LOGE(TAG, "dgx_p8_init failed to allocate dgx_p8_bus_t");
        return (dgx_bus_protocols_t*) bus;
    }
    p8_init_masks(bus);
    bool is_low = dc < 32 && lcd_wr < 32 && lcd_d7 < 32 && lcd_d6 < 32 && lcd_d5 < 32 && lcd_d4 < 32 && lcd_d3 < 32 && lcd_d2 < 32 && lcd_d1 < 32 && lcd_d0;
    bus->p8_send_byte = is_low ? p8_send_byte_low_dc : p8_send_byte_mix_dc;
    bus->protocols.bus_type = DGX_BUS_P8;
    bus->protocols.bus_name = "P8";
    bus->protocols.write_command = dgx_p8_send_command;
    bus->protocols.write_commands = dgx_p8_send_commands;
    bus->protocols.read_data = dgx_p8_read_data;
    bus->protocols.write_data_async = // there's no difference between "sync" and "async"
            bus->protocols.write_data = is_low ? dgx_p8_send_data_low : dgx_p8_send_data_mix;
    bus->protocols.set_area_async = dgx_p8_set_area_std_async;
    bus->protocols.sync_write = dgx_p8_sync_write_func;
    bus->protocols.set_area_protocol = dgx_p8_set_area_func;
    bus->protocols.dispose = dgx_p8_dispose_bus_func;
    bus->protocols.memory_allocator = dgx_p8_memory_allocator;
    bus->protocols.draw_buffer_len = 4000;
    bus->protocols.draw_buffer = bus->protocols.memory_allocator(bus->protocols.draw_buffer_len);
    if (!bus->protocols.draw_buffer) {
        ESP_LOGE(TAG, "dgx_p8_init buffer allocation failed");
        return 0;
    }
    bus->dcio = dc;
    bus->csio = cs;
    bus->wrio = lcd_wr;
    bus->rdio = lcd_rd;
    bus->dio[0] = lcd_d0;
    bus->dio[1] = lcd_d1;
    bus->dio[2] = lcd_d2;
    bus->dio[3] = lcd_d3;
    bus->dio[4] = lcd_d4;
    bus->dio[5] = lcd_d5;
    bus->dio[6] = lcd_d6;
    bus->dio[7] = lcd_d7;
    for (uint8_t j = 0; j < 8u; ++j) {
        dgx_gpio_set_direction(bus->dio[j], GPIO_MODE_OUTPUT);
    }
    dgx_gpio_set_direction(dc, GPIO_MODE_OUTPUT);
    dgx_gpio_set_direction(cs, GPIO_MODE_OUTPUT);
    dgx_gpio_set_direction(lcd_wr, GPIO_MODE_OUTPUT);
    if (lcd_rd >= 0) {
        dgx_gpio_set_direction(lcd_rd, GPIO_MODE_OUTPUT);
        gpio_set_level(lcd_rd, 1);
    }
    dgx_gpio_set_level(cs, 1); // unselect
    dgx_gpio_set_level(lcd_wr, 1); // "normally" high
    dgx_gpio_set_level(cs, 0); // select
    return (dgx_bus_protocols_t*) bus;
}
