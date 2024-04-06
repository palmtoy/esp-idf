#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "esp_http_client.h"

#define MAX_HTTP_OUTPUT_BUFFER  256
#define HTTP_SRV_HOST "smartonoff-d066"
#define HTTP_SRV_PORT 80

static const char *HTTP_CLI_TAG = "HTTP_CLIENT";

esp_err_t _httpEvtHandler(esp_http_client_event_t *evt) {
  static char *output_buffer; // Buffer to store response of http request from event handler
  static int output_len;      // Stores number of bytes read
  switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
      ESP_LOGD(HTTP_CLI_TAG, "HTTP_EVENT_ERROR");
      break;
    case HTTP_EVENT_REDIRECT:
      ESP_LOGD(HTTP_CLI_TAG, "HTTP_EVENT_REDIRECT");
      break;
    case HTTP_EVENT_ON_CONNECTED:
      ESP_LOGD(HTTP_CLI_TAG, "HTTP_EVENT_ON_CONNECTED");
      break;
    case HTTP_EVENT_HEADER_SENT:
      ESP_LOGD(HTTP_CLI_TAG, "HTTP_EVENT_HEADER_SENT");
      break;
    case HTTP_EVENT_ON_HEADER:
      ESP_LOGD(HTTP_CLI_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
      break;
    case HTTP_EVENT_ON_DATA:
      ESP_LOGD(HTTP_CLI_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
      /*
      *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
      *  However, event handler can also be used in case chunked encoding is used.
      */
      if (!esp_http_client_is_chunked_response(evt->client)) {
        // If user_data buffer is configured, copy the response into the buffer
        if (evt->user_data) {
          memcpy((void*)(u32_t(evt->user_data) + output_len), evt->data, evt->data_len);
        } else {
          if (output_buffer == NULL) {
            output_buffer = (char *)malloc(esp_http_client_get_content_length(evt->client));
            output_len = 0;
            if (output_buffer == NULL) {
              ESP_LOGE(HTTP_CLI_TAG, "Failed to allocate memory for output buffer");
              return ESP_FAIL;
            }
          }
          memcpy(output_buffer + output_len, evt->data, evt->data_len);
        }
        output_len += evt->data_len;
      }
      break;
    case HTTP_EVENT_ON_FINISH:
      ESP_LOGD(HTTP_CLI_TAG, "HTTP_EVENT_ON_FINISH");
      if (output_buffer != NULL)
      {
        // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
        // ESP_LOG_BUFFER_HEX(HTTP_CLI_TAG, output_buffer, output_len);
        free(output_buffer);
        output_buffer = NULL;
      }
      output_len = 0;
      break;
    case HTTP_EVENT_DISCONNECTED:
      ESP_LOGI(HTTP_CLI_TAG, "HTTP_EVENT_DISCONNECTED");
      int mbedtls_err = 0;
      esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)(evt->data), &mbedtls_err, NULL);
      if (err != 0)
      {
        ESP_LOGI(HTTP_CLI_TAG, "Last esp error code: 0x%x", err);
        ESP_LOGI(HTTP_CLI_TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
      }
      if (output_buffer != NULL)
      {
        free(output_buffer);
        output_buffer = NULL;
      }
      output_len = 0;
      break;
  }
  return ESP_OK;
}

void sendHttpRequest(char* pStrQuery) {
  esp_ip4_addr_t ipAddr = {0};
  esp_err_t queryRet = mdns_query_a(HTTP_SRV_HOST, 5000, &ipAddr); // dns query timeout: 5s
  if (ESP_OK != queryRet) {
    ESP_LOGW(HTTP_CLI_TAG, "DNS query %s.local server's ip failed ( ErrorCode = %d )", HTTP_SRV_HOST, queryRet);
    return;
  }
  char strIpAddr[16] = {0};
  sprintf(strIpAddr, "%s", inet_ntoa(ipAddr));
  ESP_LOGI(HTTP_CLI_TAG, "%s.local server's ip: %s", HTTP_SRV_HOST, strIpAddr);
  esp_http_client_config_t config = {};
  config.host = strIpAddr,
  config.port = HTTP_SRV_PORT,
  config.method = HTTP_METHOD_GET;
  config.path = "/";
  config.query = pStrQuery;
  config.transport_type = HTTP_TRANSPORT_OVER_TCP;
  config.event_handler = _httpEvtHandler;
  esp_http_client_handle_t client = esp_http_client_init(&config);
  esp_err_t err = esp_http_client_open(client, 0);
  if (err != ESP_OK) {
    ESP_LOGE(HTTP_CLI_TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
  } else {
    int content_length = esp_http_client_fetch_headers(client);
    if (content_length < 0) {
      ESP_LOGE(HTTP_CLI_TAG, "HTTP client fetch headers failed");
    } else {
      char output_buffer[MAX_HTTP_OUTPUT_BUFFER] = { 0 }; // Buffer to store response of http request
      int data_read = esp_http_client_read_response(client, output_buffer, MAX_HTTP_OUTPUT_BUFFER);
      if (data_read >= 0) {
        ESP_LOGI(HTTP_CLI_TAG, "HTTP GET Status = %d, data_read = %d, content_length = %" PRId64,
          esp_http_client_get_status_code(client), data_read, esp_http_client_get_content_length(client));
        ESP_LOGI(HTTP_CLI_TAG, "HTTP GET output_buffer = %s", output_buffer);
      } else {
        ESP_LOGE(HTTP_CLI_TAG, "Failed to read response");
      }
    }
  }
  esp_http_client_cleanup(client);
}

#ifdef __cplusplus
}
#endif
