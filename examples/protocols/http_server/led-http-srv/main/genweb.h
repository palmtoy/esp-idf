#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <esp_http_server.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "mdns.h"
#include "driver/gpio.h"
#include "protocol_examples_common.h"


struct
{
    char on[3];
    char off[4];
} G_SWITCH_DICT = {"ON", "OFF"};

struct S_SWITCH_ON
{
    char color[7];
    char disabled[9];
} G_SWITCH_ON_1 = { "gray;", "disabled" }, G_SWITCH_ON_0 = { "green;", "" };

struct S_SWITCH_OFF
{
    char color[6];
    char disabled[9];
} G_SWITCH_OFF_1 = { "red;", "" }, G_SWITCH_OFF_0 = { "gray;", "disabled" };

static bool G_SWITCH_STATUE = false;

struct S_SWITCH_ON* getSwitchOnStatus() {
    if (G_SWITCH_STATUE) {
        return &G_SWITCH_ON_1;
    } else {
        return &G_SWITCH_ON_0;
    }
}

struct S_SWITCH_OFF* getSwitchOffStatus() {
    if (G_SWITCH_STATUE) {
        return &G_SWITCH_OFF_1;
    } else {
        return &G_SWITCH_OFF_0;
    }
}

char* getSwitchStatusColor() {
    if (G_SWITCH_STATUE) {
        return "green";
    } else {
        return "red";
    }
}

char* getSwitchStatusText() {
    if (G_SWITCH_STATUE) {
        return G_SWITCH_DICT.on;
    } else {
        return G_SWITCH_DICT.off;
    }
}

char* _doGenWebPage() {
    char* strWebPage = malloc(3 * 1024);
    char* strHtmlTemplate = "\
<html> \
<head> \
    <style> \
        html { \
            font-family: Arial; \
            display: inline-block; \
            margin: 0px auto; \
            text-align: center; \
        } \
        .buttonOn { \
            background-color: %s \
            color: white; \
            padding: 80px 200px; \
            text-align: center; \
            display: inline-block; \
            font-size: 80px; \
            margin: 8px 2px; \
            cursor: pointer; \
        } \
        .buttonOff { \
            background-color: %s \
            color: white; \
            padding: 80px 180px; \
            text-align: center; \
            display: inline-block; \
            font-size: 80px; \
            margin: 12px 2px; \
            cursor: pointer; \
        } \
    </style> \
</head> \
\
<body> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <br/> \
    <h1 style=\"font-size:39px\">ESP32C3 Relay Controller :> </h1> \
    <h2 style=\"font-size:36px;color: %s\">Switch Status: <strong>%s</strong></h2> \
    <p> \
        <a href=\"?switch_on\"><button %s class=\"buttonOn\">Switch ON</button></a> \
    </p> \
    <p> \
        <a href=\"?switch_off\"><button %s class=\"buttonOff\">Switch OFF</button></a> \
    </p> \
</body> \
</html> \
";
    sprintf(strWebPage, strHtmlTemplate,
        getSwitchOnStatus()->color, getSwitchOffStatus()->color,
        getSwitchStatusColor(), getSwitchStatusText(),
        getSwitchOnStatus()->disabled, getSwitchOffStatus()->disabled);
    return strWebPage;
}

char* getWebPage(bool bStatusChange, bool switchStatus) {
    if (bStatusChange) {
        G_SWITCH_STATUE = switchStatus;
    }
    return _doGenWebPage();
}
