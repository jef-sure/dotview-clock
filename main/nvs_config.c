#include "nvs_config.h"

#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#include "esp_check.h"
#include "esp_log.h"
#include "nvs_flash.h"

const char TAG[] = "NVS CONFIG";

str_t *get_nvs_str_key(const char *key) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    size_t out_len;
    str_t *ret = str_new_ln(0);
    err = nvs_get_str(my_handle, key, NULL, &out_len);
    ESP_LOGI(TAG, "nvs_get_str '%s' out len %u", key, out_len);
    if (err == ESP_OK) {
        if (out_len) {
            if (!str_resize(&ret, out_len)) {
                ESP_LOGE(TAG, "Error allocating %u bytes for %s", out_len, key);
            } else {
                ret->length = out_len - 1;
                err = nvs_get_str(my_handle, key, ret->data, &out_len);
                ESP_LOGI(TAG, "nvs_get_str '%s' returns '%s' (%u) ", key, str_c(ret), str_length(ret));
            }
        } else {
            ESP_LOGI(TAG, "nvs_get_str '%s' is empty", key);
        }
    } else {
        ESP_LOGE(TAG, "Error reading %s: %s", key, esp_err_to_name(err));
    }
    nvs_close(my_handle);
    return ret;
}

void set_nvs_str_key(const char *key, str_t *value) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    err = nvs_set_str(my_handle, key, str_c(value));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error writing %s: %s", key, esp_err_to_name(err));
    }
    nvs_close(my_handle);
}

void set_nvs_tint_key(const char *key, int8_t value) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    err = nvs_set_i8(my_handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error writing %s: %s", key, esp_err_to_name(err));
    }
    nvs_close(my_handle);
}

int8_t get_nvs_tint_key(const char *key) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    int8_t ret = 0;
    err = nvs_get_i8(my_handle, key, &ret);
    nvs_close(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading %s: %s", key, esp_err_to_name(err));
        ret = 0;
    }
    return ret;
}

void set_nvs_bint_key(const char *key, int64_t value) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    err = nvs_set_i64(my_handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error writing %s: %s", key, esp_err_to_name(err));
    }
    nvs_close(my_handle);
}

int64_t get_nvs_bint_key(const char *key) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    int64_t ret = 0;
    err = nvs_get_i64(my_handle, key, &ret);
    nvs_close(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading %s: %s", key, esp_err_to_name(err));
        ret = 0;
    }
    return ret;
}
