#include "http_server.h"

#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <unistd.h>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_config.h"
#include "nvs_flash.h"
#include "str.h"
#include "tzones.h"

static const char *TAG = "http_client_request";

#define COUNT_OF(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

static str_t *http_data_is_auto_location_checked(httpd_req_t *req) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    int8_t is_auto_location_checked = 0;  // value will default to 0, if not set yet in NVS
    err = nvs_get_i8(my_handle, "is_auto_loc", &is_auto_location_checked);
    nvs_close(my_handle);
    str_t *ret = str_new_pc("checked");
    if (err == ESP_OK) {
        if (!is_auto_location_checked) {
            str_clear(ret);
        }
    } else {
        ESP_LOGE(TAG, "Error reading is_auto_location_checked: %s", esp_err_to_name(err));
    }
    return ret;
}

static str_t *http_data_ip_info_key(httpd_req_t *req) {  //
    return get_nvs_str_key("ip_info_key");               //
}

static str_t *http_data_location(httpd_req_t *req) {  //
    return get_nvs_str_key("location");               //
}

static str_t *http_data_timezone_options(httpd_req_t *req) {
    static str_t *option = 0;
    if (!option) option = str_new_ln(256);
    str_t *tz = get_nvs_str_key("timezone");
    if (str_is_empty(tz) && getenv("TZ")) {
        str_destroy(&tz);
        ESP_LOGI(TAG, "Get TZ from env: %s", getenv("TZ"));
        TZName_t const *tZone = findTZbyValue(getenv("TZ"));
        if (tZone) tz = str_new_pc(tZone->name);
        ESP_LOGI(TAG, "Got TZ from env: %s", str_c(tz));
    } else {
        if (!getenv("TZ")) {
            ESP_LOGI(TAG, "No TZ from NVS and ENV is empty");
        } else {
            ESP_LOGI(TAG, "TZ from NVS: %s", str_c(tz));
        }
    }
    for (size_t i = 0; i < TimeZonesNumer; ++i) {
        str_clear(option);
        str_append_pc(&option, "<option value=\"");
        str_append_pc(&option, TimeZones[i].name);
        str_append_pc(&option, "\"");
        if (tz && str_cmp_pc(tz, TimeZones[i].name) == 0) {
            str_append_pc(&option, " selected");
        }
        str_append_pc(&option, ">");
        str_append_pc(&option, TimeZones[i].name);
        str_append_pc(&option, "</option>");
        httpd_resp_send_chunk(req, option->data, str_length(option));
    }
    return 0;
}

static str_t *http_data_openweather_key(httpd_req_t *req) {  //
    return get_nvs_str_key("openweather_key");               //
}

static str_t *http_data_weather_timestamp(httpd_req_t *req) {
    int64_t dt = get_nvs_bint_key("weather_ts");
    char tsbuf[32];
    memset(tsbuf, 0, sizeof(tsbuf));
    snprintf(tsbuf, sizeof(tsbuf), "%" PRId64, dt);
    return str_new_pc(tsbuf);
}
static str_t *http_data_weather_station(httpd_req_t *req) {
    str_t *wb = get_nvs_str_key("weather_base");
    str_t *ueq = 0;
    for (size_t i = 0; i < str_length(wb); i++) {
        if (wb->data[i] == '"') {
            if (!ueq) {
                ueq = str_new_pc("\\\"");
            }
            str_replace_str(&wb, i, 1, ueq);
        }
    }
    str_destroy(&ueq);
    return wb;
}

struct _http_answer {
    const char *const_html;
    str_t *(*handler)();
} template[] = {
    {
        "<html lang=\"en\"><head><title>DotView Clock Settings</title><link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/boots"
        "trap.min.css\" rel=\"stylesheet\" integrity=\"sha384-T3c6CoIi6uLrA9TneNEoa7RxnatzjcDSCmG1MXxSR1GAsXEV/Dwwykc2MPK8M2HN\" crossorigin=\"an"
        "onymous\"><style>"
        " input#clear_wifi:checked[type=checkbox] {"
        " background-color: crimson;"
        " }"
        " label.form-label {"
        " user-select: none;"
        ""
        " }"
        " .form-check-input {"
        " border-width: medium;"
        " }"
        " </style></head><body onload=\"on_load()\" class=\"p-3\"><form action=\"/\" method=\"p"
        "ost\"><div class=\"mb-3\"><header class=\"d-flex justify-content-left py-3\"> DotView Clock configuration </header> <button type=\"submi"
        "t\" name=\"cancel\" value=\"cancel\" class=\"btn btn-warning\">Cancel</button> </div><div class=\"mb-3\"> <input type=\"checkbox\" "
        "name=\"is_a"
        "uto_location\" id=\"is_auto_location\" class=\"form-check-input\"",  //
        http_data_is_auto_location_checked                                    //
    },
    {
        " onchange=\"on_is_auto_location(this.checked)\"> <label for=\"is_auto_location\" class=\"form-label\"> Guess location from IP Info </lab"
        "el> </div><div class=\"mb-3\" id=\"div_ip_info_key\"> <label for=\"ip_info_key\" class=\"form-label\"> IP Info API key </label> <input typ"
        "e=\"text\" name=\"ip_info_key\" id=\"ip_info_key\" class=\"form-control\" value=\"",  //
        http_data_ip_info_key                                                                  //
    },
    {
        "\"> </div><div class=\"div_location mb-3\"> <label for=\"location\" class=\"form-label\"> Manual location </label> <input type=\"text\" nam"
        "e=\"location\" id=\"location\" class=\"form-control\" value=\"",  //
        http_data_location                                                 //
    },
    {
        "\"> </div><div class=\"div_location mb-3\"> <label for=\"timezone\" class=\"form-label\"> Manual timezone </label> <select name=\"timezone"
        "\" id=\"timezone\" class=\"form-select\">",  //
        http_data_timezone_options                    //
    },
    {
        " </select> </div><div id=\"div_weather\" class=\"mb-3\"> <label for=\"openweather_key\" class=\"form-label\"> Openweathermap.org API key <"
        "/label> <input type=\"text\" name=\"openweather_key\" id=\"openweather_key\" class=\"form-control\" value=\"",  //
        http_data_openweather_key                                                                                        //
    },
    {
        "\"> <div id=\"last_weather_station\" class=\"form-text\"></div></div><div class=\"mb-3\"> <input type=\"checkbox\" name=\"clear_wifi\" "
        "class="
        "\"form-check-input\" id=\"clear_wifi\"> <label for=\"clear_wifi\" class=\"form-label\"> Clear WiFi data </label> </div> <button type=\"subm"
        "it\" class=\"btn btn-success\">Submit</button> </form><script>"
        " var WeatherTimestamp = \"",  //
        http_data_weather_timestamp    //
    },
    {
        "\";"
        " var WeatherStation = \"",  //
        http_data_weather_station    //
    },
    {
        "\";"
        " function date_from_dt(dt) {"
        " return (new Date(dt * 1000)).toLocaleString();"
        " }"
        " function on_is_auto_location(checked) {"
        " if (c"
        "hecked) {"
        " document.getElementById(\"div_ip_info_key\").style.display = null;"
        " document.querySelectorAll(\".div_location\").forEach(e"
        "=> e.style.display = 'none');"
        " } else {"
        " document.getElementById(\"div_ip_info_key\").style.display = 'none';"
        " document.querySelecto"
        "rAll(\".div_location\").forEach(e => e.style.display = null);"
        " }"
        " }"
        " function on_load() {"
        " on_is_auto_location(document.getElementBy"
        "Id('is_auto_location').checked);"
        " if (parseInt(WeatherTimestamp) && WeatherStation) {"
        " document.getElementById(\"last_weather_stati"
        "on\").textContent = 'Last weather station: ' + date_from_dt(parseInt(WeatherTimestamp)) + ' - ' + WeatherStation;"
        " }"
        " }"
        " </script><"
        "/body></html>",  //
        0                 //
    },
};

/* Our URI handler function to be called during GET /uri request */
esp_err_t http_get_index(httpd_req_t *req) {
    for (int ti = 0; ti < COUNT_OF(template); ++ti) {
        httpd_resp_send_chunk(req, template[ti].const_html, strlen(template[ti].const_html));
        if (template[ti].handler) {
            str_t *chunk = template[ti].handler(req);
            if (str_length(chunk)) {
                httpd_resp_send_chunk(req, chunk->data, str_length(chunk));
                str_destroy(&chunk);
            }
        } else {
            httpd_resp_send_chunk(req, NULL, 0);
            break;
        }
    }
    return ESP_OK;
}

esp_err_t http_get_thankyou(httpd_req_t *req) {
    httpd_resp_sendstr(req, "<html><head><title>Configured</title></head><body><p>Clock configured");
    return ESP_OK;
}

esp_err_t http_get_nochanges(httpd_req_t *req) {
    httpd_resp_sendstr(req, "<html><head><title>No changes</title></head><body><p>Configuration not changed");
    return ESP_OK;
}

#define DECODE_NIMBLE(c) \
    ((uint8_t)((c) >= '0' && (c) <= '9' ? (c) - '0' : (c) >= 'A' && (c) <= 'F' ? (c) - 'A' + 10 : (c) >= 'a' && (c) <= 'f' ? (c) - 'a' + 10 : 0))

static uint8_t decode_hex(const char *h) {
    uint8_t ret = (DECODE_NIMBLE(h[0]) << 4) | DECODE_NIMBLE(h[1]);
    return ret;
}

str_t *decode_uri_component_in_place(str_t *uri_str) {
    size_t npos = 0, fpos;
    do {
        fpos = str_find_c(uri_str, '%', npos);
        if (fpos != str_npos) {
            size_t len_rest = str_length(uri_str) - fpos;
            if (len_rest > 2u) {
                uri_str->data[fpos] = (char)decode_hex(uri_str->data + fpos + 1);
                memmove(uri_str->data + fpos + 1, uri_str->data + fpos + 3, len_rest - 3u);
                uri_str->length -= 2u;
            }
        } else {
            break;
        }
        npos = fpos + 1;
    } while (1);
    for (size_t i = 0; i < str_length(uri_str); i++) {
        if (uri_str->data[i] == '+') uri_str->data[i] = ' ';
    }

    return uri_str;
}

/*

*/
esp_err_t http_post_settings(httpd_req_t *req) {
    str_t *recv_buf = str_new_ln(req->content_len);
    if (!recv_buf) {
        ESP_LOGI(TAG, "Cannot allocate recv_buf(%u)", req->content_len);
        return ESP_FAIL;
    }
    size_t recv_offset = 0;
    while (recv_offset < req->content_len) {
        /* Read data received in the request */
        int ret = httpd_req_recv(req, recv_buf->data + recv_offset, req->content_len - recv_offset);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            str_destroy(&recv_buf);
            return ESP_FAIL;
        }
        recv_offset += ret;
        ESP_LOGI(TAG, "post_handler recv length %d", ret);
    }
    ESP_LOGI(TAG, "POST received '%s'", str_c(recv_buf));
    size_t npos = 0, fpos;
    bool is_auto_location_checked = false;
    bool clear_wifi = false;
    bool cancel = false;
    do {
        fpos = str_find_c(recv_buf, '&', npos);
        if (fpos == str_npos) fpos = recv_buf->length;
        size_t eqpos = str_find_c(recv_buf, '=', npos);
        if (eqpos != str_npos && eqpos < fpos) {
            str_t *key = str_new_pcln(recv_buf->data + npos, eqpos - npos);
            str_t *value = str_new_pcln(recv_buf->data + eqpos + 1, fpos - eqpos - 1);
            decode_uri_component_in_place(key);
            decode_uri_component_in_place(value);
            ESP_LOGI(TAG, "POST parsed '%s' => '%s'", str_c(key), str_c(value));
            if (str_cmp_pc(key, "cancel") == 0) {
                cancel = true;
            }
            if (!cancel) {
                if (str_cmp_pc(key, "timezone") == 0 || str_cmp_pc(key, "location") == 0 || str_cmp_pc(key, "ip_info_key") == 0 ||
                    str_cmp_pc(key, "openweather_key") == 0) {
                    set_nvs_str_key(str_c(key), value);
                }
                if (str_cmp_pc(key, "is_auto_location") == 0) {
                    is_auto_location_checked = str_cmp_pc(value, "on") == 0;
                }
                if (str_cmp_pc(key, "clear_wifi") == 0) {
                    clear_wifi = str_cmp_pc(value, "on") == 0;
                }
            }
            str_destroy(&key);
            str_destroy(&value);
        }
        npos = fpos + 1;
    } while (npos < str_length(recv_buf));
    str_destroy(&recv_buf);
    if (!cancel) {
        set_nvs_tint_key("is_auto_loc", is_auto_location_checked);
        set_nvs_tint_key("clear_wifi", clear_wifi);
    }
    set_nvs_tint_key("got_data", 1);
    /* Send a simple response */
    {
        httpd_resp_set_status(req, "302 Found");
        char myUrl[36] = {0};
        esp_netif_ip_info_t ip_infoMyIf;
        esp_netif_get_ip_info(esp_netif_get_default_netif(), &ip_infoMyIf);
        snprintf(myUrl, sizeof(myUrl), "http://%s/%s", inet_ntoa(ip_infoMyIf.ip), (cancel ? "nochanges" : "thankyou"));
        httpd_resp_set_hdr(req, "Location", myUrl);
        httpd_resp_send(req, NULL, 0);
    }
    return ESP_OK;
}

/* URI handler structure for GET /uri */
httpd_uri_t uri_get = {
    .uri = "/",                 //
    .method = HTTP_GET,         //
    .handler = http_get_index,  //
    .user_ctx = NULL            //
};

httpd_uri_t uri_get_thanku = {
    .uri = "/thankyou",            //
    .method = HTTP_GET,            //
    .handler = http_get_thankyou,  //
    .user_ctx = NULL               //
};

httpd_uri_t uri_get_nochanges = {
    .uri = "/nochanges",            //
    .method = HTTP_GET,             //
    .handler = http_get_nochanges,  //
    .user_ctx = NULL                //
};

/* URI handler structure for POST /uri */
httpd_uri_t uri_post = {
    .uri = "/",                     //
    .method = HTTP_POST,            //
    .handler = http_post_settings,  //
    .user_ctx = NULL                //
};

httpd_handle_t WebConfigServer = NULL;

/* Function for starting the webserver */
bool start_webserver(void) {
    /* Generate default configuration */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (WebConfigServer) stop_webserver();

    /* Start the httpd server */
    if (httpd_start(&WebConfigServer, &config) == ESP_OK) {
        /* Register URI handlers */
        httpd_register_uri_handler(WebConfigServer, &uri_get);
        httpd_register_uri_handler(WebConfigServer, &uri_get_thanku);
        httpd_register_uri_handler(WebConfigServer, &uri_get_nochanges);
        httpd_register_uri_handler(WebConfigServer, &uri_post);
    }
    /* If server failed to start, handle will be NULL */
    return WebConfigServer != NULL;
}

/* Function for stopping the webserver */
void stop_webserver(void) {
    if (WebConfigServer) {
        /* Stop the httpd server */
        httpd_stop(WebConfigServer);
        WebConfigServer = 0;
    }
}