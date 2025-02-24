#ifndef CONFIG_H
#define CONFIG_H

#define MAX_STR_LEN 256

typedef struct {
    char defaultImagePath[MAX_STR_LEN];
    char noImageAvailablePath[MAX_STR_LEN];
    char mqttAddress[MAX_STR_LEN];
    char mqttClientID[MAX_STR_LEN];
    char mqttTopic[MAX_STR_LEN];
    int  mqttQos;
    int  defaultImageTimeout;
    int  initialReconnectTimeout;
    int  maxReconnectTimeout;
    // Add other settings as needed.
} Config;

// Loads configuration from a file; returns 0 on success.
int loadConfig(const char *filename, Config *config);

// Optionally, provide a getter for a global configuration.
extern Config globalConfig;

#endif // CONFIG_H