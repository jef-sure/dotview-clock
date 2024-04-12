/*
 * dgx_arch_esp32.h
 *
 *  Created on: 16.12.2022
 *      Author: KYMJ
 */

#ifndef _DGX_INCLUDE_DGX_ARCH_ESP32_H_
#define _DGX_INCLUDE_DGX_ARCH_ESP32_H_
#ifdef __cplusplus
// @formatter:off
extern "C"
{
// @formatter:on
#endif

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

static const char *ARCH_ESP32_TAG = "ARCH ESP32";
#define RC_CHECK(gpio_call, err_str)                 \
    do                                               \
    {                                                \
        esp_err_t rc = gpio_call;                    \
        if (rc != ESP_OK)                            \
        {                                            \
            ESP_LOGE(ARCH_ESP32_TAG, "%s", err_str); \
            return rc;                               \
        }                                            \
    } while (0)

static inline esp_err_t dgx_gpio_set_direction(gpio_num_t gpio_num, gpio_mode_t mode)
{
    esp_rom_gpio_pad_select_gpio(gpio_num);
    RC_CHECK(gpio_set_direction(gpio_num, mode), "gpio_set_direction failed");
    return ESP_OK;
}

static inline esp_err_t dgx_gpio_set_level(gpio_num_t gpio_num, uint32_t level)
{
    return gpio_set_level(gpio_num, level);
}

static inline void dgx_delay(uint32_t msDelay)
{
    vTaskDelay(msDelay / portTICK_PERIOD_MS);
}

static inline void *dgx_allocate_dma_memory(size_t size)
{
    uint8_t *ptr = heap_caps_malloc(size, MALLOC_CAP_DMA);
    return ptr;
}

#ifdef __cplusplus
    // @formatter:off
}
// @formatter:on
#endif
#endif /* _DGX_INCLUDE_DGX_ARCH_ESP32_H_ */
