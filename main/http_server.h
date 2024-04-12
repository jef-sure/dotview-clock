#ifndef _HTTP_SERVER_
#define _HTTP_SERVER_

#include "esp_http_server.h"

#ifdef __cplusplus
// @formatter:off
extern "C" {
// @formatter:on
#endif

bool start_webserver(void);
void stop_webserver(void);
extern httpd_handle_t WebConfigServer;

#ifdef __cplusplus
// @formatter:off
}
// @formatter:on
#endif

#endif /* _HTTP_SERVER_ */
