#ifndef _INFRARED_RAY_WIFISTATION_H_
#define _INFRARED_RAY_WIFISTATION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "httpSrv.h"

#define ROUTER_MAXIMUM_RETRY CONFIG_ROUTER_MAXIMUM_RETRY
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the Router with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;
static const char *STATION_TAG = "WIFI-STATION";
static int s_retry_num = 0;
static bool G_B_CONNECTED = false;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    if (s_retry_num < ROUTER_MAXIMUM_RETRY) {
      esp_wifi_connect();
      s_retry_num++;
      ESP_LOGI(STATION_TAG, "retry to connect to the router");
    } else {
      xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
    }
    ESP_LOGI(STATION_TAG, "connect to the router fail");
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    ESP_LOGI(STATION_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    s_retry_num = 0;
    xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
  }
}

static void connect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  ESP_LOGI(STATION_TAG, "Starting http webserver ...");
  start_webserver();
}

static void disconnect_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (G_B_CONNECTED) {
    ESP_LOGI(STATION_TAG, "Stopping http webserver and restart.");
    G_B_CONNECTED = false;
    stopHttpSrv();
    esp_restart();
  }
}

bool wifi_init_sta(char *wifiSSID, char *wifiPwd, const char *domainName) {
  G_B_CONNECTED = false;
  s_wifi_event_group = xEventGroupCreate();
  esp_netif_t *netif = esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  esp_event_handler_instance_t instance_any_id;
  esp_event_handler_instance_t instance_got_ip;
  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, NULL));

  ESP_LOGI(STATION_TAG, "wifi_init_station: wifiSSID = %s, wifiPwd = %s", wifiSSID, wifiPwd);
  wifi_config_t wifi_config = {
      .sta = {}};
  strlcpy((char *)wifi_config.sta.ssid, wifiSSID, sizeof(wifi_config.sta.ssid));
  strlcpy((char *)wifi_config.sta.password, wifiPwd, sizeof(wifi_config.sta.password));

  /* Setting a password implies station will connect to all security modes including WEP/WPA.
      * However these modes are deprecated and not advisable to be used. Incase your Access point
      * doesn't support WPA2, these mode can be enabled by commenting below line */
  if (strlen((char *)wifi_config.sta.password)) {
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
  }

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  // Set the hostname for the network interface
  esp_err_t err = esp_netif_set_hostname(netif, domainName);
  if (err) {
    ESP_LOGE(STATION_TAG, "Set hostname Failed.");
    return G_B_CONNECTED;
  }

  ESP_LOGI(STATION_TAG, "wifi_init_station finished.");
  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
    * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

  /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
    * happened. */
  if (bits & WIFI_CONNECTED_BIT) {
    ESP_LOGI(STATION_TAG, "Connected to router SSID:%s password:%s", wifiSSID, wifiPwd);
    G_B_CONNECTED = true;
  } else if (bits & WIFI_FAIL_BIT) {
    ESP_LOGI(STATION_TAG, "Failed to connect to SSID:%s, password:%s", wifiSSID, wifiPwd);
  } else {
    ESP_LOGE(STATION_TAG, "UNEXPECTED EVENT");
  }

  /* The event will not be processed after unregister */
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
  ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
  vEventGroupDelete(s_wifi_event_group);
  return G_B_CONNECTED;
}

#ifdef __cplusplus
}
#endif

#endif /* _INFRARED_RAY_WIFISTATION_H_ */
