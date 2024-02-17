// Switch HTTP Server
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "mdns.h"

#include "spiffsFS.h"
#include "softAP.h"
#include "wifiStation.h"
#include "wifiConfigWeb.h"
#include "httpSrv.h"

static const char *TAG = "MECNUM-CAR";

static void initialise_mdns(void) {
    char* softApName = getSoftApName();
    //initialize mDNS
    ESP_ERROR_CHECK(mdns_init());
    //set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK(mdns_hostname_set(softApName));
    ESP_LOGI(TAG, "mDNS hostname set to: [ %s ]", getDomainName());
}

void app_main() {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    initSpiffs();
    struct S_WIFI_CONFIG* pWiFiConfig = loadWiFiConfig();
    bool bConnected = false;
    if (pWiFiConfig) {
        const char* domainName = getDomainName();
        if (domainName == NULL) {
            ESP_LOGE(TAG, "Get domain name ( gen softAP name ) FAILED.");
            abort();
            return;
        }
        bConnected = wifi_init_sta(pWiFiConfig->ssid, pWiFiConfig->password, domainName);
    }
    if (!bConnected) {
        wifi_init_softap();
        start_webserver();
    }

    initLEDs();
    initialise_mdns();
}
