#ifndef __DGX_BUS_PROTOCOLS_H__
#define __DGX_BUS_PROTOCOLS_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

enum dgx_bus_type {  //
    DGX_BUS_SPI,     //
    DGX_BUS_I2C,     //
    DGX_BUS_P8       //
};

enum {                 //
    DGX_BUS_READ = 0,  //
    DGX_BUS_WRITE = 1  //
};

enum {
    DGX_AREA_FUNC_STD = 0 /* default */,  //
    DGX_AREA_FUNC_CMD = 1 /* SSD1306 */,
    DGX_AREA_FUNC_8BIT = 2 /* SSD1351 */,
};

struct _dgx_bus_protocols_t;

/*
 * Bus functions
 */
typedef void (*dgx_bus_send_command_func)(struct _dgx_bus_protocols_t *bus, uint8_t cmd);
typedef uint8_t *(*dgx_bus_malloc_func)(uint32_t len);

/*
 * len is always in bits
 */
typedef void (*dgx_bus_write_commands_func)(struct _dgx_bus_protocols_t *bus, const uint8_t *data, uint32_t len);
typedef void (*dgx_bus_write_data_func)(struct _dgx_bus_protocols_t *bus, const uint8_t *data, uint32_t len);
typedef uint32_t (*dgx_bus_read_data_func)(struct _dgx_bus_protocols_t *bus, uint8_t *data, uint32_t len);
typedef void (*dgx_bus_write_data_async_func)(struct _dgx_bus_protocols_t *bus, const uint8_t *data, uint32_t len);
typedef void (*dgx_bus_sync_write_func)(struct _dgx_bus_protocols_t *bus);

/*
 * Special protocol functions
 */
typedef void (*dgx_bus_set_area_async_func)(struct _dgx_bus_protocols_t *bus, uint16_t left, uint16_t right, uint16_t top, uint16_t bottom,
                                            bool readWrite);
typedef void (*dgx_bus_dispose_bus_func)(struct _dgx_bus_protocols_t *bus);
typedef void (*dgx_bus_set_area_protocol_func)(struct _dgx_bus_protocols_t *bus, int protocol);

typedef struct _dgx_bus_protocols_t {
    enum dgx_bus_type bus_type;
    const char *bus_name;
    dgx_bus_send_command_func write_command;
    dgx_bus_write_commands_func write_commands;
    dgx_bus_write_data_func write_data;
    dgx_bus_read_data_func read_data;
    dgx_bus_write_data_async_func write_data_async;
    dgx_bus_sync_write_func sync_write;
    dgx_bus_set_area_async_func set_area_async;
    dgx_bus_set_area_protocol_func set_area_protocol;
    dgx_bus_malloc_func memory_allocator;
    dgx_bus_dispose_bus_func dispose;
    uint8_t xcmd_set;
    uint8_t ycmd_set;
    uint8_t rcmd_send;
    uint8_t wcmd_send;
    size_t draw_buffer_len;
    uint8_t *draw_buffer;
} dgx_bus_protocols_t;

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif

#endif /* __DGX_BUS_PROTOCOLS_H__ */
