#ifndef _SWITCH_WIFICONFIGWEB_H_
#define _SWITCH_WIFICONFIGWEB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "softAP.h"

#define G_REBOOT_TIME 6000 // 6s

char* _doGenWiFiConfigPage() {
    char* strWebPage = malloc(4096);
    char* strHtmlTemplate = "\
<html> \
<head> \
    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/> \
    <title>WiFi Config Page For SmartOnOff</title> \
    <style> \
        html { \
            font-family: Arial; \
            display: inline-block; \
            margin: 0px auto; \
            text-align: center; \
        } \
    </style> \
</head> \
\
<body> \
    <br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/> \
    <div style=\"text-align: center; margin-top: 60px;\"> \
        <h1 style=\"font-size:39px\">WiFi Config Page :></h1> \
        <div class=\"text\"> \
            <input type=\"text\" placeholder=\"SSID ( 支持中文; 区分大小写; 最多 30 个字符 )\" id=\"ssid\" required maxlength=30 style=\"width: 626px; height: 80px; font-size: 21pt; margin-top: 20px;\"> \
        </div> \
        <div class=\"text\"> \
            <input type=\"text\" placeholder=\"Password ( Case sensitive; Max 30 characters )\" id=\"pwd\" maxlength=30 style=\"width: 626px; height: 80px; font-size: 21pt; margin-top: 20px;\"> \
        </div> \
        <button onclick=\"save()\" style=\"margin-top: 50px; width: 200px; height: 80px; font-size: 25pt;\">Save</button> \
        <br/><br/><br/><br/> \
        <button onclick=\"reboot()\" style=\"margin-top: 50px; width: 200px; height: 80px; font-size: 25pt;\">Reboot</button> \
        <br/><br/><br/><br/><br/><br/><br/> \
        <a href=\"/\" style=\"font-size: 30px\">Back</a> \
        <br/><br/><br/><br/><br/><br/><br/><br/><br/><br/><br/> \
        <a href=\"https://github.com/palmtoy\" style=\"position: absolute; left: 6px; font-size: 16px\">Powered by LZG</a> \
        <br/> \
    </div> \
    <script type=\"text/javascript\"> \
        function reboot() { \
            fetch(\"http://%s/?reboot\"); \
            alert(\"Reboot OK.\"); \
            setTimeout(() => { window.location.href = \"http://%s/\"; }, %d); \
        } \
        function save() { \
            const ssid = document.getElementById(\"ssid\").value.trim(); \
            const password = document.getElementById(\"pwd\").value.trim(); \
            console.log(`ssid = ${ssid}, pwd = ${password}`); \
            if (ssid === \"\") { \
                alert(\"Please input WiFi SSID ...\"); \
            } else { \
                fetch(\"http://%s/wifi_config\", { \
                    method: \"POST\", \
                    headers: { \
                        \"Accept\": \"application/json\", \
                        \"Content-Type\": \"application/json\" \
                    }, \
                    body: JSON.stringify({ ssid, pwd: password }) \
                }); \
                alert(\"Save OK.\"); \
                setTimeout(() => { window.location.href = \"http://%s/\"; }, %d); \
            } \
        } \
    </script> \
</body> \
</html> \
";
    char* domainName = getDomainName();
    sprintf(strWebPage, strHtmlTemplate, domainName, domainName, G_REBOOT_TIME, domainName, domainName, G_REBOOT_TIME);
    return strWebPage;
}

char* getWiFiConfigPage() {
    return _doGenWiFiConfigPage();
}

#ifdef __cplusplus
}
#endif

#endif /* _SWITCH_WIFICONFIGWEB_H_ */
