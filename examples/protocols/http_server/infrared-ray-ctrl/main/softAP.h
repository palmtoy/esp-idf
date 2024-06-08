#ifndef _INFRARED_RAY_SOFTAP_H_
#define _INFRARED_RAY_SOFTAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#define AP_NAME_PREFIX CONFIG_ESP_WIFI_SSID
#define AP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD
#define AP_WIFI_CHANNEL CONFIG_ESP_WIFI_CHANNEL
#define AP_MAX_CONN CONFIG_ESP_MAX_STA_CONN
#define AP_NAME_LEN 32
#define DOMAIN_NAME_LEN (AP_NAME_LEN + 6)

static const char *SOFT_AP_TAG = "SOFT-AP";

char G_SOFT_AP[AP_NAME_LEN] = {0};
char G_DOMAIN_NAME[DOMAIN_NAME_LEN] = {0};

esp_err_t generate_ap_name() {
  if (strlen(G_SOFT_AP) > 0) {
    return ESP_OK;
  }
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  if (sprintf(G_SOFT_AP, "%s-%02x%02x", AP_NAME_PREFIX, mac[4] + 96, mac[5] + 17 ) < 0) {
    memset(G_SOFT_AP, '\0', AP_NAME_LEN);
    memset(G_DOMAIN_NAME, '\0', DOMAIN_NAME_LEN);
    return ESP_FAIL;
  }
  sprintf(G_DOMAIN_NAME, "%s.local", G_SOFT_AP);
  return ESP_OK;
}

char *getSoftApName() {
  if (generate_ap_name() == ESP_OK)
  {
    return G_SOFT_AP;
  }
  return NULL;
}

char *getDomainName() {
  if (generate_ap_name() == ESP_OK)
  {
    return G_DOMAIN_NAME;
  }
  return NULL;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(SOFT_AP_TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
  } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(SOFT_AP_TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
  }
}

void wifi_init_softap() {
  esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

  char *ssidName = getDomainName();
  if (ssidName == NULL) {
    ESP_LOGE(SOFT_AP_TAG, "Gen SSID name FAILED.");
    abort();
    return;
  }
  wifi_config_t wifi_config = {};
  wifi_config.ap.ssid_len = (uint8_t)(strlen(ssidName));
  wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
  wifi_config.ap.channel = AP_WIFI_CHANNEL;
  wifi_config.ap.max_connection = AP_MAX_CONN;
  strlcpy((char *)wifi_config.ap.ssid, ssidName, sizeof(wifi_config.ap.ssid));
  strlcpy((char *)wifi_config.ap.password, AP_WIFI_PASS, sizeof(wifi_config.ap.password));

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(SOFT_AP_TAG, "WIFI_INIT_SOFTAP finished. SSID:%s password:%s channel:%d", ssidName, AP_WIFI_PASS, AP_WIFI_CHANNEL);
}

#ifdef __cplusplus
}
#endif

#endif /* _INFRARED_RAY_SOFTAP_H_ */
