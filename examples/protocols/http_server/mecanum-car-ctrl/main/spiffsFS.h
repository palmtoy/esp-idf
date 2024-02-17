#ifndef _SWITCH_SPIFFSFS_H_
#define _SWITCH_SPIFFSFS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "cJSON.h"

#define LINE_MAX_CHAR 256
#define PARAM_MAX_CHAR (LINE_MAX_CHAR / 2)

static const char *SPIFFS_FS_TAG = "SPIFFS-FS";

struct S_JSON_CONFIG {
    cJSON* jRoot;
    cJSON* jSSID;
    cJSON* jPwd;
} G_JSON_DICT;

struct S_WIFI_CONFIG
{
    char ssid[PARAM_MAX_CHAR];
    char password[PARAM_MAX_CHAR];
} G_WIFI_DICT;


void initSpiffs()
{
    ESP_LOGI(SPIFFS_FS_TAG, "Initializing SPIFFS");
    
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 3,
      .format_if_mount_failed = true
    };
    
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(SPIFFS_FS_TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(SPIFFS_FS_TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(SPIFFS_FS_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
    
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(SPIFFS_FS_TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(SPIFFS_FS_TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

esp_err_t jsonParser(char* buf) {
    G_JSON_DICT.jRoot = cJSON_Parse(buf);
    if (G_JSON_DICT.jRoot == NULL) {
        const char* errPtr = cJSON_GetErrorPtr();
        if (errPtr != NULL) {
            ESP_LOGE(SPIFFS_FS_TAG, "cJSON_Parse failed! [%s]\n", errPtr);
        }
        G_JSON_DICT.jSSID = NULL;
        G_JSON_DICT.jPwd = NULL;
        return ESP_FAIL;
    }
    G_JSON_DICT.jSSID = cJSON_GetObjectItem(G_JSON_DICT.jRoot, "ssid");
    if (!(cJSON_IsString(G_JSON_DICT.jSSID) && (G_JSON_DICT.jSSID->valuestring != NULL)
        && (strlen(G_JSON_DICT.jSSID->valuestring) < PARAM_MAX_CHAR))) {
        G_JSON_DICT.jSSID = NULL;
        G_JSON_DICT.jPwd = NULL;
        cJSON_Delete(G_JSON_DICT.jRoot);
        G_JSON_DICT.jRoot = NULL;
        return ESP_FAIL;
    }
    G_JSON_DICT.jPwd = cJSON_GetObjectItem(G_JSON_DICT.jRoot, "pwd");
    if (!(cJSON_IsString(G_JSON_DICT.jPwd) && (G_JSON_DICT.jPwd->valuestring != NULL)
        && (strlen(G_JSON_DICT.jPwd->valuestring) < PARAM_MAX_CHAR))) {
        G_JSON_DICT.jSSID = NULL;
        G_JSON_DICT.jPwd = NULL;
        cJSON_Delete(G_JSON_DICT.jRoot);
        G_JSON_DICT.jRoot = NULL;
        return ESP_FAIL;
    }
    return ESP_OK;
}

struct S_JSON_CONFIG* getJsonDict() {
    return &G_JSON_DICT;
}

esp_err_t saveWiFiConfig(char* buf, const char* wifiSSID)
{
    if (strlen(wifiSSID) <= 0) {
        free(buf);
        return ESP_FAIL;
    }
    const char* pFileName = "/spiffs/wifi_config.txt";
    // Use POSIX and C standard library functions to work with files, first create a file.
    ESP_LOGI(SPIFFS_FS_TAG, "Opening file ...");
    FILE* f = fopen(pFileName, "w");
    if (f == NULL) {
        free(buf);
        ESP_LOGE(SPIFFS_FS_TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, "%s\n", buf);
    free(buf);
    fclose(f);
    ESP_LOGI(SPIFFS_FS_TAG, "File written.");

    // All done, unmount partition and disable SPIFFS
    esp_vfs_spiffs_unregister(NULL);
    ESP_LOGI(SPIFFS_FS_TAG, "SPIFFS unmounted.");

    esp_restart();
    return ESP_OK;
}

struct S_WIFI_CONFIG* loadWiFiConfig()
{
    const char* pFileName = "/spiffs/wifi_config.txt";
    // Open renamed file for reading
    ESP_LOGI(SPIFFS_FS_TAG, "Reading file ...");
    FILE* f = fopen(pFileName, "r");
    if (f == NULL) {
        ESP_LOGE(SPIFFS_FS_TAG, "Failed to open file for reading.");
        return NULL;
    }
    char lineBuf[LINE_MAX_CHAR];
    fgets(lineBuf, sizeof(lineBuf), f);
    fclose(f);
    // strip newline
    char* pos = strchr(lineBuf, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(SPIFFS_FS_TAG, "Read from file: '%s'.", lineBuf);

    // All done, unmount partition and disable SPIFFS
    esp_vfs_spiffs_unregister(NULL);
    ESP_LOGI(SPIFFS_FS_TAG, "SPIFFS unmounted.");

    if (jsonParser(lineBuf) == ESP_OK) {
        sprintf(G_WIFI_DICT.ssid, "%s", G_JSON_DICT.jSSID->valuestring);
        sprintf(G_WIFI_DICT.password, "%s", G_JSON_DICT.jPwd->valuestring);
        cJSON_Delete(G_JSON_DICT.jRoot);
        return &G_WIFI_DICT;
    }
    return NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* _SWITCH_SPIFFSFS_H_ */
