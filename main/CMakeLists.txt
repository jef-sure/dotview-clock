FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.c)

idf_component_register(
    SRCS ${app_sources}
    REQUIRES            dgx mjson qrcode esp_event esp_wifi nvs_flash protocomm wifi_provisioning esp_event mbedtls esp_http_client esp_http_server esp_adc
    INCLUDE_DIRS ""
)
