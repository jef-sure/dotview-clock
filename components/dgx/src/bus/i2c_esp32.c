/*
 * i2c_esp32.c
 *
 *  Created on: 14.10.2022
 *      Author: Anton Petrusevich
 */

#include <stdbool.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"

#include "bus/dgx_i2c_esp32.h"

#define DGX_I2C_TIMEOUT 10

typedef struct _dgx_i2c_bus_t {
    dgx_bus_protocols_t protocols;
    uint8_t i2c_address;
    i2c_port_t i2c_num;
    gpio_num_t sda;
    gpio_num_t sclk;
    gpio_num_t rst;
    int clock_speed_hz;
    uint8_t cmd_single;
    uint8_t cmd_stream;
    uint8_t data_stream;
} dgx_i2c_bus_t;

static const char TAG[] = "DGX I2C BUS";

#define RCCHECK(exp, err) \
    do {\
        rc = exp;\
        if (rc != ESP_OK) {\
            ESP_LOGE(TAG, err);\
        }\
    } while(0)

static void dgx_i2c_send_command(struct _dgx_bus_protocols_t *_bus, uint8_t cmd) {
    dgx_i2c_bus_t *bus = (dgx_i2c_bus_t*) _bus;
    esp_err_t rc;
    i2c_cmd_handle_t i2c_handle;
    i2c_handle = i2c_cmd_link_create();
    RCCHECK(i2c_master_start(i2c_handle), "dgx_i2c_send_command failed");
    RCCHECK(i2c_master_write_byte(i2c_handle, (bus->i2c_address << 1) | I2C_MASTER_WRITE, true), "dgx_i2c_send_command failed");
    RCCHECK(i2c_master_write_byte(i2c_handle, bus->cmd_single, true), "dgx_i2c_send_command failed");
    RCCHECK(i2c_master_write_byte(i2c_handle, cmd, true), "dgx_i2c_send_command failed");
    RCCHECK(i2c_master_stop(i2c_handle), "dgx_i2c_send_command failed");
    RCCHECK(i2c_master_cmd_begin(bus->i2c_num, i2c_handle, pdMS_TO_TICKS(DGX_I2C_TIMEOUT)), "dgx_i2c_send_command failed");
    i2c_cmd_link_delete(i2c_handle);
}

static void dgx_i2c_send_commands(struct _dgx_bus_protocols_t *_bus, const uint8_t *cmds, uint32_t len) {
    dgx_i2c_bus_t *bus = (dgx_i2c_bus_t*) _bus;
    esp_err_t rc;
    i2c_cmd_handle_t i2c_handle;
    len = (len + 7u) / 8u;
    i2c_handle = i2c_cmd_link_create();
    RCCHECK(i2c_master_start(i2c_handle), "dgx_i2c_send_commands_async failed");
    RCCHECK(i2c_master_write_byte(i2c_handle, (bus->i2c_address << 1) | I2C_MASTER_WRITE, true), "dgx_i2c_send_commands_async failed");
    RCCHECK(i2c_master_write_byte(i2c_handle, bus->cmd_stream, true), "dgx_i2c_send_commands_async failed");
    RCCHECK(i2c_master_write(i2c_handle, cmds, len, true), "dgx_i2c_send_commands_async failed");
    RCCHECK(i2c_master_stop(i2c_handle), "dgx_i2c_send_commands_async failed");
    RCCHECK(i2c_master_cmd_begin(bus->i2c_num, i2c_handle, pdMS_TO_TICKS(DGX_I2C_TIMEOUT)), "dgx_i2c_send_commands_async failed");
    i2c_cmd_link_delete(i2c_handle);
}

static void dgx_i2c_send_data(struct _dgx_bus_protocols_t *_bus, const uint8_t *data, uint32_t len) {
    dgx_i2c_bus_t *bus = (dgx_i2c_bus_t*) _bus;
    esp_err_t rc;
    i2c_cmd_handle_t i2c_handle;
    len = (len + 7u) / 8u;
    i2c_handle = i2c_cmd_link_create();
    RCCHECK(i2c_master_start(i2c_handle), "dgx_i2c_send_data failed");
    RCCHECK(i2c_master_write_byte(i2c_handle, bus->data_stream, true), "dgx_i2c_send_commands failed");
    RCCHECK(i2c_master_write(i2c_handle, data, len, true), "dgx_i2c_send_data failed");
    RCCHECK(i2c_master_stop(i2c_handle), "dgx_i2c_send_data failed");
    RCCHECK(i2c_master_cmd_begin(bus->i2c_num, i2c_handle, pdMS_TO_TICKS(DGX_I2C_TIMEOUT)), "dgx_i2c_send_data failed");
    i2c_cmd_link_delete(i2c_handle);
}

static void dgx_i2c_send_data_async(struct _dgx_bus_protocols_t *_bus, const uint8_t *data, uint32_t len) {
    dgx_i2c_send_data(_bus, data, len);
}

static uint32_t dgx_i2c_read_data(struct _dgx_bus_protocols_t *_bus, uint8_t *data, uint32_t len) {
    memset(data, 0, len / 8);
    return len;
}

static void dgx_i2c_set_area_cmd_async(struct _dgx_bus_protocols_t *_bus, uint16_t left, uint16_t right, uint16_t top, uint16_t bottom, bool readWrite) {
    /* Horizontal addressing mode */
    uint8_t crset[] = { //
            _bus->xcmd_set, (uint8_t) left, (uint8_t) right, //
                    _bus->ycmd_set, (uint8_t) top, (uint8_t) bottom //
            };
    dgx_i2c_send_commands(_bus, crset, sizeof(crset) * 8);
}

static void dgx_i2c_dispose_bus_func(struct _dgx_bus_protocols_t *_bus) {
    dgx_i2c_bus_t *bus = (dgx_i2c_bus_t*) _bus;
    i2c_driver_delete(bus->i2c_num);
    heap_caps_free(bus); // for symmetry with heap_caps_calloc
}

void dgx_i2c_set_area_func(struct _dgx_bus_protocols_t *_bus, int area_func_style) {
    _bus->set_area_async = dgx_i2c_set_area_cmd_async;
}

static uint8_t* dgx_i2c_memory_allocator(uint32_t len) {
    return (uint8_t*) malloc(len);
}

static void dgx_i2c_sync_write(struct _dgx_bus_protocols_t *_bus) {
}

dgx_bus_protocols_t* dgx_i2c_init(i2c_port_t i2c_num, uint8_t i2c_address, gpio_num_t sda, gpio_num_t sclk, gpio_num_t rst, int clock_speed_hz) {
    i2c_config_t i2c_config = { //
            .mode = I2C_MODE_MASTER, //
                    .sda_io_num = sda, //
                    .scl_io_num = sclk, //
                    .sda_pullup_en = GPIO_PULLUP_ENABLE, //
                    .scl_pullup_en = GPIO_PULLUP_ENABLE, //
                    .master.clk_speed = clock_speed_hz //
            };
    esp_err_t rc;
    RCCHECK(i2c_param_config(i2c_num, &i2c_config), "dgx_i2c_init failed");
    RCCHECK(i2c_driver_install(i2c_num, I2C_MODE_MASTER, 0, 0, 0), "dgx_i2c_init failed");
    dgx_i2c_bus_t *bus = (dgx_i2c_bus_t*) heap_caps_calloc(1, sizeof(dgx_i2c_bus_t), 0);
    if (bus == 0) {
        ESP_LOGE(TAG, "dgx_i2c_init failed to allocate dgx_i2c_bus_t");
        return (dgx_bus_protocols_t*) bus;
    }
    bus->protocols.bus_type = DGX_BUS_I2C;
    bus->protocols.bus_name = "I2C";
    bus->protocols.write_command = dgx_i2c_send_command;
    bus->protocols.write_commands = dgx_i2c_send_commands;
    bus->protocols.read_data = dgx_i2c_read_data;
    bus->protocols.write_data = dgx_i2c_send_data;
    bus->protocols.write_data_async = dgx_i2c_send_data_async;
    bus->protocols.sync_write = dgx_i2c_sync_write;
    bus->protocols.set_area_async = dgx_i2c_set_area_cmd_async;
    bus->protocols.set_area_protocol = dgx_i2c_set_area_func;
    bus->protocols.dispose = dgx_i2c_dispose_bus_func;
    bus->protocols.memory_allocator = dgx_i2c_memory_allocator;
    bus->protocols.draw_buffer_len = 1024;
    bus->protocols.draw_buffer = bus->protocols.memory_allocator(bus->protocols.draw_buffer_len);
    if (!bus->protocols.draw_buffer) {
        ESP_LOGE(TAG, "dgx_i2c_init buffer allocation failed");
        return 0;
    }
    bus->i2c_address = i2c_address;
    bus->i2c_num = i2c_num;
    bus->sda = sda;
    bus->sclk = sclk;
    bus->rst = rst;
    bus->clock_speed_hz = clock_speed_hz;
    return (dgx_bus_protocols_t*) bus;
}
