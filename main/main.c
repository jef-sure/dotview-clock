#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

//
#include "driver/adc_types_legacy.h"
#include "driver/gpio.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hal/adc_types.h"
#include "nvs_flash.h"
//
#include "auto_location.h"
#include "bus/dgx_spi_esp32.h"
#include "dgx_bits.h"
#include "dgx_colors.h"
#include "dgx_draw.h"
#include "dgx_font.h"
#include "drivers/ssd1351.h"
#include "drivers/vscreen.h"
#include "drivers/vscreen_2h.h"
#include "fonts/CasusDotView.h"
#include "fonts/IBMCGALight8x16Light8x1616.h"
#include "fonts/WeatherIconsRegular13.h"
#include "http_client_request.h"
#include "http_server.h"
#include "mjson.h"
#include "nvs_config.h"
#include "qrcode.h"
#include "str.h"
#include "tzones.h"
#include "weather.h"

static const char TAG[] = "DGX MAIN";

void provisioning_setup(dgx_screen_t *scr, bool force_provisioning);

enum {
    DISPLAY_R_MOSI = GPIO_NUM_23,             //
    DISPLAY_R_SCLK = GPIO_NUM_18,             //
    DISPLAY_R_CS = GPIO_NUM_5,                //
    DISPLAY_R_DC = GPIO_NUM_25,               //
    DISPLAY_R_RST = GPIO_NUM_26,              //
    DISPLAY_R_DMA = GPIO_NUM_2,               //
    DISPLAY_R_CLOCK_SPEED_HZ = 20 * 1000000,  //
    DISPLAY_L_MOSI = GPIO_NUM_13,             //
    DISPLAY_L_SCLK = GPIO_NUM_14,             //
    DISPLAY_L_CS = GPIO_NUM_15,               //
    DISPLAY_L_DC = GPIO_NUM_27,               //
    DISPLAY_L_RST = GPIO_NUM_33,              //
    DISPLAY_L_DMA = GPIO_NUM_1,               //
    DISPLAY_L_CLOCK_SPEED_HZ = 20 * 1000000,  //
    PHOTO_SENSOR = GPIO_NUM_32,               //
    CONFIG_BUTTON = GPIO_NUM_19               //
};

// QUEUE:
// I (1391) DGX MAIN: rectangle fill: 32785
// I (5051) DGX MAIN: draw circles: 3658772
// I (6824) DGX MAIN: fill circles: 1773719

// POLLING:
// 1024
// I (1392) DGX MAIN: rectangle fill: 32664
// I (3398) DGX MAIN: draw circles: 2005647
// I (4611) DGX MAIN: fill circles: 1212547
// 2048
// I (1392) DGX MAIN: rectangle fill: 32681
// I (2902) DGX MAIN: draw circles: 1509184
// I (3999) DGX MAIN: fill circles: 1097047

// CPU 240 MHz
// I (1389) DGX MAIN: rectangle fill: 32025
// I (2449) DGX MAIN: draw circles: 1059667
// I (3317) DGX MAIN: fill circles: 868445

void make_lut_16(uint16_t *dest_lut, uint8_t r, uint8_t g, uint8_t b) {
    for (size_t i = 0; i < 256; i++) {
        dest_lut[i] = dgx_rgb_to_16(i * r / 255u, i * g / 255u, i * b / 255u);
    }
}

dgx_point_2d_t linear_move(float t, const dgx_point_2d_t p0, const dgx_point_2d_t p1) {
    dgx_point_2d_t ret;
    float dX = t * (p1.x - p0.x);
    float dY = t * (p1.y - p0.y);
    ret.x = dX + p0.x;
    ret.y = dY + p0.y;
    return ret;
}

static inline float non_linear_time(float t) {
    // g(x) = x<0.5? x*x*2 : 1-(1-x)*(1-x)*2
    return t < 0.5f ? t * t * 2.0f : 1.0f - (1.0f - t) * (1.0f - t) * 2.0f;
}
static inline float non_linear_time3(float t) {
    // f(x) = x<0.5? x*x*x*4 : 1-(1-x)*(1-x)*(1-x)*4
    float e = t < 0.5f ? t : 1.0f - t;
    float c = e * e * e * 4.0f;
    return t < 0.5f ? c : 1.0f - c;
}

void draw_font_morph8216(dgx_screen_t *vscr, dgx_screen_t *vpoint, uint16_t *lut, dgx_font_symbol_morph_t *ms, float t) {
    for (int i = 0; i < ms->number_of_dots; i++) {
        dgx_point_2d_t pi = linear_move(t, ms->m_start[i], ms->m_end[i]);
        dgx_vscreen8_to_screen16(vscr, pi.x, pi.y, vpoint, lut, true);
    }
}

dgx_screen_t *make_sym_point8(int point_size) {
    dgx_screen_t *vpoint = dgx_vscreen_init(point_size, point_size, 8, DgxScreenRGB);
    dgx_font_make_point8(vpoint);
    return vpoint;
}

void font_draw_dot_func(dgx_screen_t *_scr_dst, int x_dst, int y_dst, dgx_font_sym8_params_t *vParams) {
    if (vParams->vpoint->color_bits == _scr_dst->color_bits) {
        dgx_vscreen_to_vscreen(_scr_dst, x_dst, y_dst, vParams->vpoint, true);
    } else if (vParams->vpoint->color_bits == 8 && _scr_dst->color_bits == 16) {
        dgx_vscreen8_to_screen16(_scr_dst, x_dst, y_dst, vParams->vpoint, vParams->lut, true);
    }
}

typedef enum message_type_ {
    ClockDigits,  //
    CityName,     //
    Weather,      //
    Forecast,     //
    WebConfigure  //
} message_type_t;

typedef enum display_mode_ {
    DisplayNormal,    //
    DisplayWebConfig  //
} display_mode_t;

typedef struct task_message_ {
    message_type_t m_type;
    uint32_t cp_info[6];
    int temperature;
    int64_t sunrise;
    int64_t sunset;
} task_message_t;

SemaphoreHandle_t xSemLocation = NULL;
WLocation_t Location;

typedef struct {
    int64_t display_start_time;
    uint32_t weather_symbol_old;
    uint32_t weather_symbol_new;
    dgx_screen_t *vpoint;
    dgx_font_symbol_morph_t *ws_morph;
    dgx_font_symbol_morph_t *info_morph[4];
    uint32_t cp_info[4];
    uint16_t icon_lut[256];
    uint16_t temp_lut[256];
    int temp_new;
    int temp_old;
} weather_display_info_t;

QueueHandle_t xQueDMS;

typedef struct {
    int64_t display_start_time;
    int64_t sunrise;
    int64_t sunset;
    dgx_font_symbol_morph_t *info_morph[6];
    uint32_t cp_info[6];
    uint16_t clock_lut[256];
    dgx_screen_t *vpoint;
    dgx_screen_t *seconds_vpoint;
} clock_display_info_t;

#define NIBBLE2HEX(b) ((char)((b) > 9 ? 'A' + (b)-10 : '0' + (b)))

str_t *encode_uri_component(const str_t *val) {
    size_t new_len = str_length(val);
    for (size_t i = 0; i < str_length(val); i++) {
        if (!isalnum((uint8_t)val->data[i])) new_len += 2;
    }
    if (new_len == str_length(val)) {
        return str_new_str(val);
    }
    str_t *uri_str = str_new_ln(new_len);
    for (size_t i = 0, ni = 0; i < str_length(val); i++) {
        uint8_t c = val->data[i];
        if (!isalnum(c)) {
            uint8_t hc = c >> 4;
            uint8_t lc = c & 0x0f;
            uri_str->data[ni++] = '%';
            uri_str->data[ni++] = NIBBLE2HEX(hc);
            uri_str->data[ni++] = NIBBLE2HEX(lc);
        } else {
            uri_str->data[ni++] = (char)c;
        }
    }
    return uri_str;
}

void destroyLocation() {
    str_destroy(&Location.country);
    str_destroy(&Location.countryCode);
    str_destroy(&Location.region);
    str_destroy(&Location.regionName);
    str_destroy(&Location.city);
    str_destroy(&Location.zip);
    str_destroy(&Location.timezone);
    str_destroy(&Location.isp);
    str_destroy(&Location.ip);
    str_destroy(&Location.location);
}

void vTaskGetLocation(void *pvParameters) {
    static task_message_t message = {
        .m_type = CityName,  //
        .cp_info = {0}       //
    };
    const long delay = 600000l;
    for (;;) {
        if (get_nvs_tint_key("is_auto_loc")) {
            str_t *ip_info_key = get_nvs_str_key("ip_info_key");
            if (ip_info_key) {
                WLocation_t location = auto_location_ipinfo(str_c(ip_info_key));
                if (location.timezone) {
                    xSemaphoreTake(xSemLocation, 0);
                    if (!Location.timezone || strcmp(str_c(Location.timezone), str_c(location.timezone)) != 0) {
                        setupTZ(str_c(location.timezone));
                    }
                    destroyLocation();
                    Location = location;
                    Location.location = str_new_str(Location.zip);
                    str_append_pc(&Location.location, ",");
                    str_append_str(&Location.location, Location.country);
                    xSemaphoreGive(xSemLocation);
                    xQueueSendToFront(xQueDMS, &message, 0);
                    vTaskDelay(pdMS_TO_TICKS(delay));
                } else {
                    vTaskDelay(pdMS_TO_TICKS(10000));
                }
            } else {
                // wait for configuration
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
            str_destroy(&ip_info_key);
        } else {
            // manual location
            str_t *loc = get_nvs_str_key("location");
            str_t *timezone = get_nvs_str_key("timezone");
            if (!str_is_empty(loc) || !str_is_empty(timezone)) {
                xSemaphoreTake(xSemLocation, 0);
                if (!Location.timezone || strcmp(str_c(Location.timezone), str_c(timezone)) != 0) {
                    setupTZ(str_c(timezone));
                }
                destroyLocation();
                Location.timezone = timezone;
                Location.location = loc;
                xSemaphoreGive(xSemLocation);
                xQueueSendToFront(xQueDMS, &message, 0);
            } else {
                str_destroy(&loc);
                str_destroy(&timezone);
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void vTaskConfigButton(void *pvParameters) {
    static task_message_t message = {
        .m_type = WebConfigure,  //
        .cp_info = {0}           //
    };
    display_mode_t display_mode = DisplayNormal;
    int64_t start_webserver_time = esp_timer_get_time();
    const long min_delay = 1000000l;
    for (;;) {
        int palLevel = gpio_get_level(CONFIG_BUTTON);
        if (palLevel == 0 && !WebConfigServer && start_webserver()) {
            esp_netif_ip_info_t ip_infoMyIf;
            esp_netif_get_ip_info(esp_netif_get_default_netif(), &ip_infoMyIf);
            message.cp_info[0] = ip_infoMyIf.ip.addr;
            display_mode = DisplayWebConfig;
            xQueueSendToFront(xQueDMS, &message, 0);
            start_webserver_time = esp_timer_get_time();
        }
        if (display_mode == DisplayWebConfig && !WebConfigServer) {
            message.cp_info[0] = 0;
            display_mode = DisplayNormal;
            xQueueSendToFront(xQueDMS, &message, 0);
        }
        bool button_to_stop = (palLevel == 0 && esp_timer_get_time() - start_webserver_time > min_delay);
        if (display_mode == DisplayWebConfig && WebConfigServer && (get_nvs_tint_key("got_data") == 1 || button_to_stop)) {
            set_nvs_tint_key("got_data", 0);
            vTaskDelay(button_to_stop ? pdMS_TO_TICKS(100) : pdMS_TO_TICKS(2000));
            stop_webserver();
            if (get_nvs_tint_key("clear_wifi")) {
                esp_restart();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

typedef struct weather_info_ {
    int temperature;
    int weather_id;
    int64_t timestamp;
    int64_t sunrise;
    int64_t sunset;
    uint32_t weather_symbol;
} weather_info_t;

void makeWeatherInfo(int temperature, uint32_t *cp_info) {
    int absTemp = temperature >= 0 ? temperature : -temperature;
    if (absTemp > 9) {
        cp_info[1] = temperature < 0 ? '-' : ' ';
        cp_info[2] = '0' + absTemp / 10;
        cp_info[3] = '0' + absTemp % 10;
    } else {
        cp_info[1] = ' ';
        cp_info[2] = temperature < 0 ? '-' : ' ';
        cp_info[3] = '0' + absTemp;
    }
}

int getOpenweatherCod(str_t *json_body) {
    char json_value_buf[32];
    double dcod;
    int cod = 0;
    if (mjson_get_number(str_c(json_body), str_length(json_body), "$.cod", &dcod)) {
        cod = (int)dcod;
    } else {
        int slen = mjson_get_string(json_body->data, str_length(json_body), "$.cod", json_value_buf, sizeof(json_value_buf));
        if (slen >= 0) {
            cod = atoi(json_value_buf);
        } else {
            ESP_LOGE(TAG, "getOpenweatherCod (no cod): %s\n", str_c(json_body));
        }
    }
    return cod;
}

int64_t getWeatherJsonInt(str_t *json_body, const char *path) {
    double ddt;
    if (mjson_get_number(str_c(json_body), str_length(json_body), path, &ddt)) {
        return ddt;
    } else {
        ESP_LOGE(TAG, "weather json has no path %s", path);
        return 0;
    }
}

float getWeatherJsonFloat(str_t *json_body, const char *path) {
    double ddt;
    if (mjson_get_number(str_c(json_body), str_length(json_body), path, &ddt)) {
        return ddt;
    } else {
        ESP_LOGE(TAG, "weather json has no path %s", path);
        return 0;
    }
}

void vTaskGetWeather(void *pvParameters) {
    const unsigned long delay = 120000lu;
    const unsigned long delay_between = 60000lu;
    static task_message_t message = {.m_type = Weather, .cp_info = {0}};
    static http_client_response_t weather_response;
    static http_client_response_t forecast_response;
    str_t *json_value_buf = str_new_ln(512);
    str_clear(json_value_buf);
    weather_response.body = str_new_ln(620);
    str_clear(weather_response.body);
    forecast_response.body = str_new_ln(20000);
    str_clear(forecast_response.body);
    bool has_got_location = false;
    for (;;) {
        str_t *url_weather = 0;
        str_t *url_forecast = 0;
        weather_info_t weather_now;
        weather_info_t weather_forecast;
        memset(&weather_now, 0, sizeof(weather_info_t));
        memset(&weather_forecast, 0, sizeof(weather_info_t));
        if (xSemaphoreTake(xSemLocation, pdMS_TO_TICKS(100)) == pdTRUE) {
            str_t *weather_api_key = get_nvs_str_key("openweather_key");
            if (Location.location && !str_is_empty(weather_api_key)) {
                str_t *enloc = encode_uri_component(Location.location);
                url_weather = str_new_pc("https://api.openweathermap.org/data/2.5/weather?q=");
                str_append_str(&url_weather, enloc);
                str_append_pc(&url_weather, "&units=metric&appid=");
                str_append_str(&url_weather, weather_api_key);
                ESP_LOGI(TAG, "weather url request: %s\n", str_c(url_weather));
                url_forecast = str_new_pc("https://api.openweathermap.org/data/2.5/forecast?q=");
                str_append_str(&url_forecast, enloc);
                str_append_pc(&url_forecast, "&units=metric&appid=");
                str_append_str(&url_forecast, weather_api_key);
                ESP_LOGI(TAG, "forecast url request: %s\n", str_c(url_forecast));
                str_destroy(&enloc);
                has_got_location = true;
            }
            str_destroy(&weather_api_key);
            xSemaphoreGive(xSemLocation);
        }
        if (url_weather) {
            heap_caps_print_heap_info(MALLOC_CAP_8BIT);
            ESP_LOGI(TAG, "getWeather starting");
            bool rc = http_client_request_get(str_c(url_weather), &weather_response);
            ESP_LOGI(TAG, "getWeather got len = %u; rc = %d", str_length(weather_response.body), rc);
            str_destroy(&url_weather);
            if (rc && !str_is_empty(weather_response.body)) {
                int cod = getOpenweatherCod(weather_response.body);
                if (cod == 200) {
                    weather_now.weather_id = getWeatherJsonInt(weather_response.body, "$.weather[0].id");
                    weather_now.timestamp = getWeatherJsonInt(weather_response.body, "$.dt");
                    weather_now.sunrise = getWeatherJsonInt(weather_response.body, "$.sys.sunrise");
                    weather_now.sunset = getWeatherJsonInt(weather_response.body, "$.sys.sunset");
                    float temperature = getWeatherJsonFloat(weather_response.body, "$.main.temp");
                    weather_now.temperature = (int)(temperature < 0 ? temperature - 0.5f : temperature + 0.5f);
                    uint8_t is_daytime = weather_now.timestamp <= weather_now.sunset && weather_now.timestamp >= weather_now.sunrise;
                    weather_now.weather_symbol = findWeatherSymbol(weather_now.weather_id, is_daytime);
                    ESP_LOGI(TAG, "Weather timestamp: %lld, sunrise: %lld, sunset: %lld; is_daytime: %d", weather_now.timestamp, weather_now.sunrise,
                             weather_now.sunset, is_daytime);
                    message.m_type = Weather;
                    message.cp_info[0] = weather_now.weather_symbol;
                    makeWeatherInfo(weather_now.temperature, message.cp_info);
                    message.temperature = weather_now.temperature;
                    message.sunrise = weather_now.sunrise;
                    message.sunset = weather_now.sunset;
                    set_nvs_bint_key("weather_ts", weather_now.timestamp);
                    {
                        char weather_base[64];
                        int slen = mjson_get_string(weather_response.body->data, str_length(weather_response.body), "$.name", weather_base,
                                                    sizeof(weather_base));
                        if (slen) {
                            str_t *wb = str_new_pc(weather_base);
                            set_nvs_str_key("weather_base", wb);
                            str_destroy(&wb);
                        }
                    }
                    xQueueSendToFront(xQueDMS, &message, 0);
                } else {
                    ESP_LOGE(TAG, "weather returns: %s\n", str_c(weather_response.body));
                }
            }
            vTaskDelay(pdMS_TO_TICKS(delay_between));
            rc = http_client_request_get(str_c(url_forecast), &forecast_response);
            ESP_LOGI(TAG, "getForecast got len = %u; rc = %d", str_length(forecast_response.body), rc);
            str_destroy(&url_forecast);
            if (rc && !str_is_empty(forecast_response.body)) {
                int cod = getOpenweatherCod(forecast_response.body);
                if (cod == 200) {
                    int list_index = 0;
                    double ddt = 0;
                    static const char *ldtpath[] = {
                        "$.list[0].dt",
                        "$.list[1].dt",
                    };
                    mjson_get_number(str_c(forecast_response.body), str_length(forecast_response.body), ldtpath[0], &ddt);
                    if (ddt != 0) {
                        if (ddt - weather_now.timestamp < 3600) ++list_index;
                        static const char *temppath[] = {
                            "$.list[0].main.temp",
                            "$.list[1].main.temp",
                        };
                        static const char *widpath[] = {
                            "$.list[0].weather[0].id",
                            "$.list[1].weather[0].id",
                        };
                        if (mjson_get_number(str_c(forecast_response.body), str_length(forecast_response.body), temppath[list_index], &ddt)) {
                            float temp = ddt;
                            weather_forecast.temperature = (int)(temp < 0 ? temp - 0.5f : temp + 0.5f);
                        } else {
                            ESP_LOGE(TAG, "no path %s", temppath[list_index]);
                            cod = 0;
                        }
                        if (mjson_get_number(str_c(forecast_response.body), str_length(forecast_response.body), widpath[list_index], &ddt)) {
                            weather_forecast.weather_id = ddt;
                        } else {
                            ESP_LOGE(TAG, "no path %s", widpath[list_index]);
                            cod = 0;
                        }
                        if (mjson_get_number(str_c(forecast_response.body), str_length(forecast_response.body), ldtpath[list_index], &ddt)) {
                            weather_forecast.timestamp = ddt;
                        } else {
                            ESP_LOGE(TAG, "no path %s", ldtpath[list_index]);
                            cod = 0;
                        }
                        if (cod == 200) {
                            uint8_t is_daytime =
                                weather_forecast.timestamp <= weather_now.sunset && weather_forecast.timestamp >= weather_now.sunrise;
                            weather_forecast.weather_symbol = findWeatherSymbol(weather_forecast.weather_id, is_daytime);
                            message.m_type = Forecast;
                            message.cp_info[0] = weather_forecast.weather_symbol;
                            makeWeatherInfo(weather_forecast.temperature, message.cp_info);
                            message.temperature = weather_forecast.temperature;
                            xQueueSendToFront(xQueDMS, &message, 0);
                            ESP_LOGI(TAG,
                                     "Forecast timestamp: %lld, list_index: %d, temperature: %d, weather_id: %d, is_daytime: %d, weather_symbol: %lu",
                                     weather_forecast.timestamp, list_index, weather_forecast.temperature, weather_forecast.weather_id, is_daytime,
                                     weather_forecast.weather_symbol);
                        } else {
                            ESP_LOGE(TAG, "forecast returns: %s\n", str_c(forecast_response.body));
                        }
                    } else {
                        ESP_LOGE(TAG, "no timestamp in list");
                        ESP_LOGE(TAG, "forecast returns: %s\n", str_c(forecast_response.body));
                    }
                }
                ESP_LOGI(TAG, "getForecast: cod: %d", cod);
            } else {
                ESP_LOGI(TAG, "Forecast response is empty");
            }
        }
        if (has_got_location) {
            vTaskDelay(pdMS_TO_TICKS(delay));
        } else {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

void vTaskTime(void *pvParameters) {
    static task_message_t message = {.m_type = ClockDigits, .cp_info = {0}};
    for (;;) {
        struct timeval now;
        gettimeofday(&now, 0);
        long delayUsec = 500000 - now.tv_usec;
        if (delayUsec < 0) {
            delayUsec += 1000000;
        }
        if (delayUsec / 1000) {
            vTaskDelay(pdMS_TO_TICKS(delayUsec / 1000));
        }
        gettimeofday(&now, 0);
        now.tv_sec++;
        struct tm timeinfo;
        localtime_r(&now.tv_sec, &timeinfo);
        message.cp_info[5] = '0' + (timeinfo.tm_sec % 10);
        message.cp_info[4] = '0' + (timeinfo.tm_sec / 10);
        message.cp_info[3] = '0' + (timeinfo.tm_min % 10);
        message.cp_info[2] = '0' + (timeinfo.tm_min / 10);
        message.cp_info[1] = '0' + (timeinfo.tm_hour % 10);
        message.cp_info[0] = timeinfo.tm_hour / 10 != 0 ? '0' + (timeinfo.tm_hour / 10) : ' ';
        xQueueSendToFront(xQueDMS, &message, 0);
        vTaskDelay(1);
    }
}

void process_weather(weather_display_info_t *wdi, task_message_t *message, int xscr, int scale) {
    if (wdi->ws_morph) dgx_font_make_morph_struct_destroy(&wdi->ws_morph);
    for (size_t i = 0; i < 4 /* number of wdi.info_morph */; i++) {
        if (wdi->info_morph[i]) dgx_font_make_morph_struct_destroy(&wdi->info_morph[i]);
    }
    wdi->weather_symbol_old = wdi->weather_symbol_new;
    wdi->weather_symbol_new = message->cp_info[0];
    wdi->ws_morph =
        dgx_font_make_morph_struct(WeatherIconsRegular13(), wdi->weather_symbol_old, wdi->weather_symbol_new, xscr, 100, wdi->vpoint->width);
    int fwidth = CasusDotView()->xWidthAverage;
    for (size_t i = 1; i < 4 /* number of wdi.info_morph */; i++) {
        wdi->info_morph[i] = dgx_font_make_morph_struct(CasusDotView(), wdi->cp_info[i], message->cp_info[i], xscr + 128 - (5 - i) * fwidth * scale,
                                                        128 - 2 * 3, scale);
        wdi->cp_info[i] = message->cp_info[i];
    }
    int64_t now = esp_timer_get_time();
    wdi->display_start_time = now;
    wdi->temp_old = wdi->temp_new;
    wdi->temp_new = message->temperature;
    uint32_t wc = weatherTempToRGB(wdi->temp_new);
    uint8_t r;
    uint8_t g;
    uint8_t b;
    WEATHERCOLOR24TORGB(r, g, b, wc);
    make_lut_16(wdi->icon_lut, r / 3u, g / 3u, b / 3u);
    make_lut_16(wdi->temp_lut, r, g, b);
    // ESP_LOGI(TAG, "temp: %d, r: %d, g: %d, b: %d; max lut %04x", wdi.temp_new, r, g, b, wdi.temp_lut[255]);
}

void process_weather_morph(weather_display_info_t *wdi, dgx_screen_t *vscr, dgx_font_sym8_params_t *vParams, int xscr, int scale) {
    int64_t now = esp_timer_get_time();
    int fwidth = CasusDotView()->xWidthAverage;
    if (wdi->ws_morph) {
        float wt = (now - wdi->display_start_time) / 500000.0f;
        if (wt > 1.0f) wt = 1.0f;
        if (!(wt == 0.0f && wdi->ws_morph->is_from_empty) || !(wt == 1.0f && wdi->ws_morph->is_to_empty)) {
            draw_font_morph8216(vscr, wdi->vpoint, wdi->icon_lut, wdi->ws_morph, (wt));
        }
    }
    bool has_degree = false;
    for (size_t i = 0; i < 4 /* number of wdi.info_morph */; i++) {
        if (wdi->info_morph[i]) {
            has_degree = true;
            float wt = (now - wdi->display_start_time) / 500000.0f;
            if (wt > 1.0f) wt = 1.0f;
            if (!(wt == 0.0f && wdi->info_morph[i]->is_from_empty) || !(wt == 1.0f && wdi->info_morph[i]->is_to_empty)) {
                draw_font_morph8216(vscr, wdi->vpoint, wdi->temp_lut, wdi->info_morph[i], (wt));
            }
        }
    }
    if (has_degree) {
        dgx_font_char_to_screen(vscr, xscr + 128 - fwidth * vParams->vpoint->width, 128 - 6, 0xb0, 0, DgxScreenLeftRight, DgxScreenTopBottom, false,
                                vParams->vpoint->width, CasusDotView(), NULL, vParams);
    }
}

void process_digit_clock(clock_display_info_t *cdi, task_message_t *message) {
    uint8_t r, g, b;
#define GETRGB(_r, _g, _b) r = _r, g = _g, b = _b
#define GETRGBN(_r, _g, _b) rn = _r, gn = _g, bn = _b
    struct timeval now_tod;
    gettimeofday(&now_tod, 0);
    bool is_daytime = true;
    if (cdi->sunrise > 0) {
        is_daytime = now_tod.tv_sec >= cdi->sunrise && now_tod.tv_sec < cdi->sunset;
    }
    const int64_t time_threshold = 1800;
    if (DGX_ABS(now_tod.tv_sec - cdi->sunrise) < time_threshold) {
        uint8_t rn, gn, bn;
        DGX_COPPER(GETRGB);
        DGX_SILVER(GETRGBN);
        float tt = (now_tod.tv_sec - (cdi->sunrise - time_threshold)) / (float)time_threshold / 2;
        r = tt * rn + (1 - tt) * r;
        g = tt * gn + (1 - tt) * g;
        b = tt * bn + (1 - tt) * b;
    } else if (DGX_ABS(now_tod.tv_sec - cdi->sunset) < time_threshold) {
        uint8_t rn, gn, bn;
        DGX_SILVER(GETRGB);
        DGX_COPPER(GETRGBN);
        float tt = (now_tod.tv_sec - (cdi->sunset - time_threshold)) / (float)time_threshold / 2;
        r = tt * rn + (1 - tt) * r;
        g = tt * gn + (1 - tt) * g;
        b = tt * bn + (1 - tt) * b;
    } else if (is_daytime) {
        DGX_SILVER(GETRGB);
    } else {
        DGX_COPPER(GETRGB);
    }
    make_lut_16(cdi->clock_lut, r, g, b);
    int fwidth = CasusDotView()->xWidthAverage;
    for (size_t i = 0; i < 6 /* number of info_morph */; i++) {
        if (cdi->info_morph[i]) dgx_font_make_morph_struct_destroy(&cdi->info_morph[i]);
    }
    cdi->info_morph[0] = dgx_font_make_morph_struct(CasusDotView(), cdi->cp_info[0], message->cp_info[0], 128 - 2 * fwidth * cdi->vpoint->width, 80,
                                                    cdi->vpoint->width);
    cdi->info_morph[1] = dgx_font_make_morph_struct(CasusDotView(), cdi->cp_info[1], message->cp_info[1], 128 - 1 * fwidth * cdi->vpoint->width, 80,
                                                    cdi->vpoint->width);
    cdi->info_morph[2] = dgx_font_make_morph_struct(CasusDotView(), cdi->cp_info[2], message->cp_info[2], 128 + 0 * fwidth * cdi->vpoint->width, 80,
                                                    cdi->vpoint->width);
    cdi->info_morph[3] = dgx_font_make_morph_struct(CasusDotView(), cdi->cp_info[3], message->cp_info[3], 128 + 1 * fwidth * cdi->vpoint->width, 80,
                                                    cdi->vpoint->width);
    cdi->info_morph[4] = dgx_font_make_morph_struct(CasusDotView(), cdi->cp_info[4], message->cp_info[4], 256 - 2 * fwidth, 10, 1);
    cdi->info_morph[5] = dgx_font_make_morph_struct(CasusDotView(), cdi->cp_info[5], message->cp_info[5], 256 - 1 * fwidth, 10, 1);
    for (size_t i = 0; i < 6; ++i) cdi->cp_info[i] = message->cp_info[i];
}

void process_digit_clock_morph(clock_display_info_t *cdi, dgx_screen_t *vscr, float wt) {
    if (wt > 1.0f) wt = 1.0f;
    for (size_t i = 0; i < 4 /* number of cdi->info_morph - 2 */; i++) {
        if (cdi->info_morph[i]) {
            if (!(wt == 0.0f && cdi->info_morph[i]->is_from_empty) || !(wt == 1.0f && cdi->info_morph[i]->is_to_empty)) {
                draw_font_morph8216(vscr, cdi->vpoint, cdi->clock_lut, cdi->info_morph[i], (wt));
            }
        }
    }
    for (size_t i = 4; i < 6 /* number of cdi->info_morph */; i++) {
        if (cdi->info_morph[i]) {
            if (!(wt == 0.0f && cdi->info_morph[i]->is_from_empty) || !(wt == 1.0f && cdi->info_morph[i]->is_to_empty)) {
                draw_font_morph8216(vscr, cdi->seconds_vpoint, cdi->clock_lut, cdi->info_morph[i], (wt));
            }
        }
    }
}

void control_brightness(dgx_screen_t *scr_l, dgx_screen_t *scr_r, adc_oneshot_unit_handle_t adc_handle) {
    static uint8_t br_window[10] = {0};
    static uint8_t set_brightness = 0;
    static int64_t timeset_brightness = 0;
    int brightness_sensor;
    adc_oneshot_read(adc_handle, ADC_CHANNEL_4, &brightness_sensor);
    float max_sensor = 1024;
    uint8_t lowest_brightness = 16;
    uint8_t greatest_brightness = 255;
    uint8_t brightness_screen = (greatest_brightness - lowest_brightness) * sqrtf(brightness_sensor / max_sensor) + lowest_brightness;
    int s = 0;
    for (size_t i = 0; i < sizeof(br_window) - 1; i++) {
        br_window[i] = br_window[i + 1];
        s += br_window[i];
    }
    s += brightness_screen;
    br_window[sizeof(br_window) - 1] = brightness_screen;
    s /= sizeof(br_window);
    uint8_t avg_br = s;
    if ((avg_br > set_brightness && avg_br - set_brightness > 1) || (avg_br < set_brightness && set_brightness - avg_br > 1) ||
        esp_timer_get_time() - timeset_brightness > 5000000LL) {
        dgx_ssd1351_brightness(scr_l, avg_br);
        dgx_ssd1351_brightness(scr_r, avg_br);
        set_brightness = avg_br;
        timeset_brightness = esp_timer_get_time();
    }
    // ESP_LOGI(TAG, "brightness sensor: %d; brightness screen: %d; avg_br: %d", brightness_sensor, brightness_screen, avg_br);
}

static dgx_screen_t *virtualMainScreen;

static void draw_web_config_qrcode(esp_qrcode_handle_t code) {
    int qrsize = esp_qrcode_get_size(code);
    int minwh = virtualMainScreen->width > virtualMainScreen->height ? virtualMainScreen->height : virtualMainScreen->width;
    int psize = minwh / qrsize;
    int x_border = (minwh - psize * qrsize) / 2;
    int y_border = x_border;
    dgx_fill_rectangle(virtualMainScreen, 0, 0, minwh, minwh, ~0u);
    for (int x = 0; x < qrsize; ++x)
        for (int y = 0; y < qrsize; ++y) {
            bool q = esp_qrcode_get_module(code, x, y);
            if (q) {
                for (int p = 0; p < psize; p++) {
                    dgx_fill_rectangle(virtualMainScreen, x_border + x * psize, y_border + y * psize, psize, psize, 0u);
                }
            }
        }
}

void app_main(void) {
    xQueDMS = xQueueCreate(10, sizeof(task_message_t));
    assert(xQueDMS != NULL);
    xSemLocation = xSemaphoreCreateMutex();
    assert(xSemLocation != NULL);
    dgx_bus_protocols_t *bus_l =
        dgx_spi_init(SPI2_HOST, DISPLAY_L_DMA, DISPLAY_L_MOSI, -1, DISPLAY_L_SCLK, DISPLAY_L_CS, DISPLAY_L_DC, DISPLAY_L_CLOCK_SPEED_HZ, 0);
    dgx_bus_protocols_t *bus_r =
        dgx_spi_init(SPI3_HOST, DISPLAY_R_DMA, DISPLAY_R_MOSI, -1, DISPLAY_R_SCLK, DISPLAY_R_CS, DISPLAY_R_DC, DISPLAY_R_CLOCK_SPEED_HZ, 0);
    dgx_screen_t *scr_r = dgx_ssd1351_init(bus_r, DISPLAY_R_RST, 16, DgxScreenRGB);
    dgx_screen_t *scr_l = dgx_ssd1351_init(bus_l, DISPLAY_L_RST, 16, DgxScreenRGB);
    if (scr_r == 0) {
        ESP_LOGE(TAG, "scr_r is null!");
        return;
    }
    if (scr_l == 0) {
        ESP_LOGE(TAG, "scr_l is null!");
        return;
    }
    gpio_set_direction(CONFIG_BUTTON, GPIO_MODE_INPUT);
    gpio_pullup_en(CONFIG_BUTTON);
    dgx_ssd1351_orientation(scr_l, DgxScreenLeftRight, DgxScreenTopBottom, true);
    dgx_ssd1351_orientation(scr_r, DgxScreenRightLeft, DgxScreenBottomTop, true);
    dgx_fill_rectangle(scr_r, 0, 0, scr_r->width, scr_r->height, 0);
    dgx_font_string_utf8_screen(scr_r, 0, 40, "ESP Soft AP Prov", DGX_WHITE(DGX_RGB_16), DgxScreenLeftRight, DgxScreenTopBottom, false, 1,
                                IBMCGALight8x16Light8x1616(), NULL, NULL);
    provisioning_setup(scr_l, gpio_get_level(CONFIG_BUTTON) == 0);
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_adc_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_adc_config, &adc_handle));
    adc_oneshot_chan_cfg_t adc_config = {
        .bitwidth = ADC_WIDTH_BIT_10,
        .atten = ADC_ATTEN_DB_11,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_4, &adc_config));
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
    esp_sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    virtualMainScreen = dgx_vscreen_2h_init(scr_l, scr_r);
    int64_t now = esp_timer_get_time();
    int frame_count = 0;
    int64_t fc_start = esp_timer_get_time();
    task_message_t message;
    static weather_display_info_t wdi = {
        .display_start_time = 0,  //
        .weather_symbol_new = 0,  //
        .weather_symbol_old = 0,  //
        .ws_morph = 0,            //
        .info_morph = {0}         //
    };
    static weather_display_info_t fdi = {
        .display_start_time = 0,  //
        .weather_symbol_new = 0,  //
        .weather_symbol_old = 0,  //
        .ws_morph = 0,            //
        .info_morph = {0}         //
    };
    static clock_display_info_t cdi = {
        .cp_info = {'0', '0', '0', '0', '0', '0'},
        .sunrise = -1,            //
        .sunset = -1,             //
        .display_start_time = 0,  //
        .info_morph = {0},        //
        .clock_lut = {0}          //
    };
    fdi.vpoint = wdi.vpoint = make_sym_point8(3);
    cdi.vpoint = make_sym_point8(8);
    cdi.seconds_vpoint = make_sym_point8(1);
    dgx_font_sym8_params_t vParamsWeather = {
        .dot_func = font_draw_dot_func,  //
        .vpoint = make_sym_point8(3)     //
    };
    dgx_font_sym8_params_t vParamsForecast = vParamsWeather;
    xTaskCreate(vTaskTime, "Time", 4096, 0, tskIDLE_PRIORITY + 2, 0);
    xTaskCreate(vTaskGetLocation, "Location", 4096, 0, tskIDLE_PRIORITY + 1, 0);
    xTaskCreate(vTaskGetWeather, "Weather", 4096, 0, tskIDLE_PRIORITY + 1, 0);
    xTaskCreate(vTaskConfigButton, "ConfigButton", 4096, 0, tskIDLE_PRIORITY + 1, 0);
    display_mode_t display_mode = DisplayNormal;
    uint32_t webConfigIp = 0;
    while (1) {
        now = esp_timer_get_time();
        if (now - fc_start > 1000000ll) {
            control_brightness(scr_l, scr_r, adc_handle);
            float fps = 1000000.0f * frame_count / (float)(now - fc_start);
            ESP_LOGI(TAG, "FPS: %f", fps);
            fc_start = now;
            frame_count = 0;
        }
        dgx_screen_progress_up(virtualMainScreen);
        dgx_fill_rectangle(virtualMainScreen, 0, 0, virtualMainScreen->width, virtualMainScreen->height, 0);
        if (xQueueReceive(xQueDMS, &message, (TickType_t)0)) {
            now = esp_timer_get_time();
            if (message.m_type == Weather) {
                process_weather(&wdi, &message, 0, 3);
                vParamsWeather.lut = wdi.temp_lut;
                cdi.sunrise = message.sunrise;
                cdi.sunset = message.sunset;
            } else if (message.m_type == Forecast) {
                process_weather(&fdi, &message, 128, 3);
                vParamsForecast.lut = fdi.temp_lut;
            } else if (message.m_type == ClockDigits) {
                process_digit_clock(&cdi, &message);
                cdi.display_start_time = now;
            } else if (message.m_type == WebConfigure) {
                webConfigIp = message.cp_info[0];
                display_mode = webConfigIp == 0 ? DisplayNormal : DisplayWebConfig;
            }
        }
        if (display_mode == DisplayNormal) {
            float wt = (now - cdi.display_start_time) / 500000.0f;
            process_weather_morph(&wdi, virtualMainScreen, &vParamsWeather, 0, 3);
            process_weather_morph(&fdi, virtualMainScreen, &vParamsForecast, 128, 3);
            process_digit_clock_morph(&cdi, virtualMainScreen, wt);
        } else if (display_mode == DisplayWebConfig) {
            esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
            char payload[150] = {0};
            snprintf(payload, sizeof(payload), "http://%s/", inet_ntoa(webConfigIp));
            cfg.display_func = draw_web_config_qrcode;
            esp_log_level_set("QRCODE", ESP_LOG_NONE);
            esp_qrcode_generate(&cfg, payload);
            esp_log_level_set("QRCODE", ESP_LOG_INFO);
            dgx_font_string_utf8_screen(virtualMainScreen, 128, 40, "http://", DGX_WHITE(DGX_RGB_16), DgxScreenLeftRight, DgxScreenTopBottom, false,
                                        1, IBMCGALight8x16Light8x1616(), NULL, NULL);
            snprintf(payload, sizeof(payload), "%s/", inet_ntoa(webConfigIp));
            dgx_font_string_utf8_screen(virtualMainScreen, 128, 60, payload, DGX_WHITE(DGX_RGB_16), DgxScreenLeftRight, DgxScreenTopBottom, false, 1,
                                        IBMCGALight8x16Light8x1616(), NULL, NULL);
        }
        dgx_screen_progress_down(virtualMainScreen);
        virtualMainScreen->update_screen(virtualMainScreen, 0, 255, 0, virtualMainScreen->height - 1);
        ++frame_count;
    }
}
