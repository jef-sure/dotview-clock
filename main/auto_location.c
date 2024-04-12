#include "auto_location.h"

#include "esp_system.h"
#include "esp_log.h"
#include "http_client_request.h"
#include "mjson.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-const-variable"
static const char TAG[] = "AUTOLOCATION";
#pragma GCC diagnostic pop

#define cpjs(json_body, field)                                                                                                    \
    do {                                                                                                                          \
        int slen = mjson_get_string(json_body->data, str_length(json_body), "$." #field, json_value_buf, sizeof(json_value_buf)); \
        if (slen >= 0) {                                                                                                          \
            ret.field = str_new_pc(json_value_buf);                                                                               \
        } else {                                                                                                                  \
            ret.field = str_new_ln(0);                                                                                            \
        }                                                                                                                         \
    } while (0)

#define cpjss(json_body, field, source)                                                                                            \
    do {                                                                                                                           \
        int slen = mjson_get_string(json_body->data, str_length(json_body), "$." #source, json_value_buf, sizeof(json_value_buf)); \
        if (slen >= 0) {                                                                                                           \
            ret.field = str_new_pc(json_value_buf);                                                                                \
        } else {                                                                                                                   \
            ret.field = str_new_ln(0);                                                                                             \
        }                                                                                                                          \
    } while (0)

WLocation_t auto_location_ipinfo(const char *ipinfo_token) {
    WLocation_t ret;
    char json_value_buf[256];
    memset(&ret, 0, sizeof(ret));
    http_client_response_t location_json;
    location_json.body = NULL;
    str_t *ipinfo_url = str_new_pc("https://ipinfo.io?token=");
    str_append_pc(&ipinfo_url, ipinfo_token);
    bool rc = http_client_request_get(str_c(ipinfo_url), &location_json);
    str_destroy(&ipinfo_url);
// I (11184) AUTOLOCATION: got IP Info: {
//   "ip": "62.158.75.3",
//   "hostname": "p3e9e4b03.dip0.t-ipconnect.de",
//   "city": "Neustadt an der Weinstraße",
//   "region": "Rheinland-Pfalz",
//   "country": "DE",
//   "loc": "49.3501,8.1389",
//   "org": "AS3320 Deutsche Telekom AG",
//   "postal": "67433",
//   "timezone": "Europe/Berlin"
// }
    if (rc && location_json.body && !str_is_empty(location_json.body)) {
        ESP_LOGI(TAG, "got IP Info: %s", str_c(location_json.body));
        cpjs(location_json.body, country);
        cpjss(location_json.body, countryCode, country);
        cpjss(location_json.body, regionName, region);
        cpjs(location_json.body, city);
        cpjss(location_json.body, zip, postal);
        cpjs(location_json.body, timezone);
        cpjss(location_json.body, isp, org);
        cpjs(location_json.body, ip);
    }
    if (location_json.body) {
        str_destroy(&location_json.body);
    }
    return ret;
}
