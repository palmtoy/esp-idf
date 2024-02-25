#ifndef _JOYSTICK_HTTPSRV_H_
#define _JOYSTICK_HTTPSRV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/param.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <esp_http_server.h>
#include "driver/gpio.h"
#include "spiffsFS.h"
#include "wifiConfigWeb.h"

#define REBOOT_DELAY_TIME 1000

static const char *HTTP_SRV_TAG = "HTTP-SRV";
static httpd_handle_t httpSrvObj = NULL;
static gpio_num_t BLINK_GPIO_BLUE = GPIO_NUM_2;

static void initLEDs(void) {
  gpio_reset_pin(BLINK_GPIO_BLUE);
  gpio_set_direction(BLINK_GPIO_BLUE, GPIO_MODE_OUTPUT);
  gpio_set_level(BLINK_GPIO_BLUE, 0);
}

static void setLEDStatus(bool bTurnOn) {
  if (bTurnOn) {
    gpio_set_level(BLINK_GPIO_BLUE, 1);
  }
  else {
    gpio_set_level(BLINK_GPIO_BLUE, 0);
  }
}

/* HTTP GET handler for route / */
esp_err_t root_get_handler(httpd_req_t *req) {
  bool bReboot = false;
  char *buf;
  size_t buf_len;
  // Read URL query string length and allocate memory for length + 1, extra byte for null termination
  buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    buf = (char*)malloc(buf_len);
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      ESP_LOGI(HTTP_SRV_TAG, "Found URL query => %s", buf);
      if (strcmp(buf, "reboot") == 0) {
        bReboot = true;
      }
    }
    free(buf);
  }
  char *resp_str = NULL;
  if (bReboot) {
    char tmpStr[] = "200 OK";
    resp_str = (char*)malloc(strlen(tmpStr) + 1);
    sprintf(resp_str, "%s", tmpStr);
  } else {
    resp_str = getWiFiConfigPage();
  }
  httpd_resp_send(req, (const char *)resp_str, strlen(resp_str));
  free(resp_str);
  if (bReboot) {
    vTaskDelay(REBOOT_DELAY_TIME / portTICK_PERIOD_MS);
    esp_restart();
  }
  return ESP_OK;
}

httpd_uri_t rootUri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler,
    .user_ctx = NULL
};

/* HTTP POST handler for route /wifi_config */
esp_err_t wifiConfigUriHandler(httpd_req_t *req) {
  int remaining = req->content_len;
  int bufSize = remaining + 1;
  if (bufSize > 3072) {
    ESP_LOGE(HTTP_SRV_TAG, "_wifiConfigUriHandler ~ POST request content too long: remaining=%d", remaining);
    const char *resp_str = "Message: WiFi config failed.";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_FAIL;
  }
  char *buf = (char*)malloc(bufSize);
  memset(buf, '\0', bufSize);
  int bufPos = 0;

  while (remaining > 0) {
    int mtuSize = 1500 + 1;
    char mtuBuf[mtuSize];
    int cnt = 0; // Read the data for the request
    if ((cnt = httpd_req_recv(req, mtuBuf, MIN(remaining, mtuSize))) <= 0) {
      if (cnt == HTTPD_SOCK_ERR_TIMEOUT) {
        continue; // Retry receiving if timeout occurred
      }
      free(buf);
      const char *resp_str = "Message: WiFi config failed.";
      httpd_resp_send(req, resp_str, strlen(resp_str));
      return ESP_FAIL;
    }
    if (cnt < bufSize) {
      strncpy(buf + bufPos, mtuBuf, cnt);
      bufPos += cnt;
      bufSize -= cnt;
    } else {
      ESP_LOGE(HTTP_SRV_TAG,
                "_wifiConfigUriHandler ~ Received too much data from POST request: cnt=%d, bufSize=%d, buf=%s, mtuBuf=%s",
                cnt, bufSize, buf, mtuBuf);
      free(buf);
      const char *resp_str = "Message: WiFi config failed.";
      httpd_resp_send(req, resp_str, strlen(resp_str));
      return ESP_FAIL;
    }
    remaining -= cnt;
  }
  ESP_LOGI(HTTP_SRV_TAG, "Received data from POST request => %s", buf);
  if (jsonParser(buf) == ESP_OK) {
    char wifiSSID[PARAM_MAX_CHAR] = {0};
    char wifiPwd[PARAM_MAX_CHAR] = {0};
    struct S_JSON_CONFIG *pG_JSON_DICT = getJsonDict();
    sprintf(wifiSSID, "%s", pG_JSON_DICT->jSSID->valuestring);
    sprintf(wifiPwd, "%s", pG_JSON_DICT->jPwd->valuestring);
    cJSON_Delete(pG_JSON_DICT->jRoot);
    ESP_LOGI(HTTP_SRV_TAG, "Found POST request parameters => ssid=%s, wifiPwd=%s", wifiSSID, wifiPwd);
    const char *resp_str = "200 OK";
    httpd_resp_send(req, resp_str, strlen(resp_str));
    initSpiffs();
    return saveWiFiConfig(buf, wifiSSID);
  }
  free(buf);
  const char *resp_str = "Message: WiFi config failed.";
  httpd_resp_send(req, resp_str, strlen(resp_str));
  return ESP_OK;
}

httpd_uri_t wifiConfigUri = {
    .uri = "/wifi_config",
    .method = HTTP_POST,
    .handler = wifiConfigUriHandler,
    .user_ctx = NULL
};

void stopHttpSrv() {
  if (httpSrvObj) {
    ESP_LOGI(HTTP_SRV_TAG, "Stopping http webserver");
    httpd_stop(httpSrvObj);
    httpSrvObj = NULL;
  }
}

void start_webserver(void) {
  stopHttpSrv();
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  // Start the httpd server
  ESP_LOGI(HTTP_SRV_TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&httpSrvObj, &config) == ESP_OK) {
    setLEDStatus(true);
    // Set URI handlers
    ESP_LOGI(HTTP_SRV_TAG, "Registering URI handlers / and wifi_config");
    httpd_register_uri_handler(httpSrvObj, &rootUri);
    httpd_register_uri_handler(httpSrvObj, &wifiConfigUri);
    return;
  }
  ESP_LOGI(HTTP_SRV_TAG, "Error starting server!");
}

#ifdef __cplusplus
}
#endif

#endif /* _JOYSTICK_HTTPSRV_H_ */
