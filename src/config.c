#include "config.h"
#include "../lib/Config/Debug.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

Config globalConfig;

int loadConfig(const char *filename, Config *config) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        Debug("loadConfig: Failed to open config file %s\n", filename);
        return -1;
    }
    
    // Determine file size.
    fseek(fp, 0, SEEK_END);
    long filesize = ftell(fp);
    rewind(fp);
    
    char *data = (char *)malloc(filesize + 1);
    if (!data) {
        Debug("loadConfig: Memory allocation failed\n");
        fclose(fp);
        return -1;
    }
    fread(data, 1, filesize, fp);
    data[filesize] = '\0';
    fclose(fp);
    
    cJSON *json = cJSON_Parse(data);
    free(data);
    if (!json) {
        Debug("loadConfig: Error parsing JSON.\n");
        return -1;
    }
    
    // Extract values from the JSON.
    cJSON *item = cJSON_GetObjectItemCaseSensitive(json, "DEFAULT_IMAGE_PATH");
    if (cJSON_IsString(item) && (item->valuestring != NULL)) {
        strncpy(config->defaultImagePath, item->valuestring, MAX_STR_LEN - 1);
    }
    
    item = cJSON_GetObjectItemCaseSensitive(json, "DISCONNECTED_IMAGE_PATH");
    if (cJSON_IsString(item) && (item->valuestring != NULL)) {
        strncpy(config->disconnectedImagePath, item->valuestring, MAX_STR_LEN - 1);
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "NO_IMAGE_AVAILABLE_PATH");
    if (cJSON_IsString(item) && (item->valuestring != NULL)) {
        strncpy(config->noImageAvailablePath, item->valuestring, MAX_STR_LEN - 1);
    }
    
    item = cJSON_GetObjectItemCaseSensitive(json, "MQTT_ADDRESS");
    if (cJSON_IsString(item) && (item->valuestring != NULL)) {
        strncpy(config->mqttAddress, item->valuestring, MAX_STR_LEN - 1);
    }
    
    item = cJSON_GetObjectItemCaseSensitive(json, "MQTT_CLIENTID");
    if (cJSON_IsString(item) && (item->valuestring != NULL)) {
        strncpy(config->mqttClientID, item->valuestring, MAX_STR_LEN - 1);
    }
    
    item = cJSON_GetObjectItemCaseSensitive(json, "MQTT_TOPIC");
    if (cJSON_IsString(item) && (item->valuestring != NULL)) {
        strncpy(config->mqttTopic, item->valuestring, MAX_STR_LEN - 1);
    }
    
    item = cJSON_GetObjectItemCaseSensitive(json, "MQTT_QOS");
    if (cJSON_IsNumber(item)) {
        config->mqttQos = item->valueint;
    }
    
    item = cJSON_GetObjectItemCaseSensitive(json, "DEFAULT_IMAGE_TIMEOUT");
    if (cJSON_IsNumber(item)) {
        config->defaultImageTimeout = item->valueint;
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "INITIAL_RECONNECT_TIMEOUT");
    if (cJSON_IsNumber(item)) {
        config->initialReconnectTimeout = item->valueint;
    }

    item = cJSON_GetObjectItemCaseSensitive(json, "MAX_RECONNECT_TIMEOUT");
    if (cJSON_IsNumber(item)) {
        config->maxReconnectTimeout = item->valueint;
    }
    
    cJSON_Delete(json);
    return 0;
}
