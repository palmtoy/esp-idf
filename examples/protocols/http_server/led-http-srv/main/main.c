// LED HTTP Server
#include "genweb.h"

static const char *TAG = "LED-HTTP-SRV";

#define BLINK_GPIO_POWER 2
#define BLINK_GPIO_R 3
#define BLINK_GPIO_G 4
#define BLINK_GPIO_B 5

#define MAX_RDM_NUM 30

static void initLEDs(void) {
    gpio_reset_pin(BLINK_GPIO_POWER);
    gpio_set_direction(BLINK_GPIO_POWER, GPIO_MODE_OUTPUT);
    gpio_reset_pin(BLINK_GPIO_R); // red LED
    gpio_set_direction(BLINK_GPIO_R, GPIO_MODE_OUTPUT);
    gpio_reset_pin(BLINK_GPIO_G); // green LED
    gpio_set_direction(BLINK_GPIO_G, GPIO_MODE_OUTPUT);
    gpio_reset_pin(BLINK_GPIO_B); // blue LED
    gpio_set_direction(BLINK_GPIO_B, GPIO_MODE_OUTPUT);
}

static void switchOnRandomLED() {
    srand(time(NULL));
    uint rdm = (uint)(rand() % MAX_RDM_NUM);
    // printf("MAX_RDM_NUM = %u, rdm = %u\n", MAX_RDM_NUM, rdm);
    if (rdm < MAX_RDM_NUM / 3) {
        gpio_set_level(BLINK_GPIO_R, 1);
        gpio_set_level(BLINK_GPIO_G, 0);
        gpio_set_level(BLINK_GPIO_B, 0);
        // printf("Turn on the red LED\n");
    } else if (rdm < MAX_RDM_NUM * 2 / 3) {
        gpio_set_level(BLINK_GPIO_R, 0);
        gpio_set_level(BLINK_GPIO_G, 1);
        gpio_set_level(BLINK_GPIO_B, 0);
        // printf("Turn on the green LED\n");
    } else {
        gpio_set_level(BLINK_GPIO_R, 0);
        gpio_set_level(BLINK_GPIO_G, 0);
        gpio_set_level(BLINK_GPIO_B, 1);
        // printf("Turn on the blue LED\n");
    }
}

static void switchLEDs(bool bSwitchOn)
{
    if (bSwitchOn) {
        gpio_set_level(BLINK_GPIO_POWER, 1);
        switchOnRandomLED();
        ESP_LOGI(TAG, "Turn on a random LED");
    } else {
        gpio_set_level(BLINK_GPIO_POWER, 0);
        gpio_set_level(BLINK_GPIO_R, 0);
        gpio_set_level(BLINK_GPIO_G, 0);
        gpio_set_level(BLINK_GPIO_B, 0);
        ESP_LOGI(TAG, "Turn off the LED");
    }
}

// An HTTP GET handler
static esp_err_t root_get_handler(httpd_req_t *req)
{
    bool bStatusChange = false;
    bool switchStatus = false;
    char*  buf;
    size_t buf_len;
    // Read URL query string length and allocate memory for length + 1, extra byte for null termination
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            if (strcmp(buf, "switch_on") == 0) {
                bStatusChange = true;
                switchStatus = true;
                switchLEDs(switchStatus);
            } else if (strcmp(buf, "switch_off") == 0) {
                bStatusChange = true;
                switchStatus = false;
                switchLEDs(switchStatus);
            }
        }
        free(buf);
    }
    // Send response as the string passed in user context
    char* resp_str = getWebPage(bStatusChange, switchStatus);
    httpd_resp_send(req, (const char*)resp_str, HTTPD_RESP_USE_STRLEN);
    free(resp_str);
    return ESP_OK;
}

static const httpd_uri_t rootUri = {
    .uri      = "/",
    .method   = HTTP_GET,
    .handler  = root_get_handler,
    // pass response string in user context to demonstrate it's usage
    .user_ctx = "<html><body><h1 style=\"color:black; font-size:30px\">ESP32C3 Relay Controller :> </h1><p><a href=\"?switch_on\"><button>Switch ON</button></a></p><p><a href=\"?switch_off\"><button>Switch OFF</button></a></p></body></html>"
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers /");
        httpd_register_uri_handler(server, &rootUri);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

static void stop_webserver(httpd_handle_t server)
{
    httpd_stop(server); // Stop the httpd server
}

static void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}

static void initialise_mdns(void)
{
    char* hostname = "esp32c3";
    //initialize mDNS
    ESP_ERROR_CHECK( mdns_init() );
    //set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK( mdns_hostname_set(hostname) );
    ESP_LOGI(TAG, "mDNS hostname set to: [%s.local]", hostname);
}

void app_main(void)
{
    static httpd_handle_t server = NULL;
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
       Read "Establishing Wi-Fi or Ethernet Connection" section in
       examples/protocols/README.md for more information about this function. */
    ESP_ERROR_CHECK(example_connect());
    /* Register event handlers to stop the server when Wi-Fi or Ethernet is disconnected,
       and re-start it upon connection. */
    #ifdef CONFIG_EXAMPLE_CONNECT_WIFI
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
    #endif // CONFIG_EXAMPLE_CONNECT_WIFI
    initialise_mdns();
    initLEDs();
    // Start the server for the first time
    server = start_webserver();
}
