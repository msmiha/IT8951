// mqtt_handler.c
#include "mqtt_handler.h"
#include "../lib/Config/Debug.h"    // For logging/debugging functions
#include "display_app.h"            // For calling Process_MQTT_Message() when a message arrives
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"             // Paho MQTT Client header
#include "config.h"

// Maximum sizes for internal storage.
#define MAX_ADDR_LEN     256
#define MAX_CLIENTID_LEN 128
#define MAX_TOPIC_LEN    256

// Static variables to hold connection parameters and the client.
static MQTTClient client;
static char mqtt_address[MAX_ADDR_LEN];
static char mqtt_clientID[MAX_CLIENTID_LEN];
static char mqtt_topic[MAX_TOPIC_LEN];
static int mqtt_qos = 0;  // Default QoS level

// Default reconnect timeout (in seconds); mutable for exponential backoff.
static int mqtt_reconnect_timeout;

// Flag indicating connection status.
static int connected = 0;

extern IT8951_Dev_Info global_dev_info;
extern UDOUBLE Init_Target_Memory_Addr;

/**
 * @brief Callback function invoked when a message arrives.
 *
 * Logs the received message and delegates processing to the display module.
 */
static int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    // Allocate a buffer for the payload plus a null terminator.
    char *payloadStr = malloc(message->payloadlen + 1);
    if (!payloadStr) {
        Debug("MQTT: Failed to allocate memory for payload.\n");
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        return 1;
    }
    memcpy(payloadStr, message->payload, message->payloadlen);
    payloadStr[message->payloadlen] = '\0';  // Null-terminate the string

    Debug("MQTT: Message received on topic \"%s\": %s\n", topicName, payloadStr);
    Process_MQTT_Message(payloadStr);

    free(payloadStr);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    
    return 1;
}

/**
 * @brief Callback invoked when the MQTT connection is lost.
 *
 * Sets the connection flag to false and displays a "disconnected" image to provide immediate visual feedback.
 */
static void connectionLost(void *context, char *cause) {
    Debug("MQTT: Connection lost, cause: %s\n", cause);
    connected = 0;
    Display_ShowSpecialImage(globalConfig.disconnectedImagePath, global_dev_info, Init_Target_Memory_Addr);
}

/**
 * @brief Initialize the MQTT client.
 *
 * Copies connection parameters into static storage, creates the client,
 * and sets up callbacks.
 */
int MQTT_Init(const char *address, const char *clientID, const char *topic) {
    if (address == NULL || clientID == NULL || topic == NULL) {
        Debug("MQTT_Init: Invalid parameters provided.\n");
        return -1;
    }

    mqtt_reconnect_timeout = globalConfig.initialReconnectTimeout;
    Debug("MQTT_Init: Using reconnect timeout: %d seconds\n", mqtt_reconnect_timeout);

    strncpy(mqtt_address, address, MAX_ADDR_LEN - 1);
    mqtt_address[MAX_ADDR_LEN - 1] = '\0';
    
    strncpy(mqtt_clientID, clientID, MAX_CLIENTID_LEN - 1);
    mqtt_clientID[MAX_CLIENTID_LEN - 1] = '\0';
    
    strncpy(mqtt_topic, topic, MAX_TOPIC_LEN - 1);
    mqtt_topic[MAX_TOPIC_LEN - 1] = '\0';

    int rc = MQTTClient_create(&client, mqtt_address, mqtt_clientID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        Debug("MQTT_Init: Failed to create MQTT client, return code %d.\n", rc);
        return rc;
    }

    MQTTClient_setCallbacks(client, NULL, connectionLost, messageArrived, NULL);
    return 0;
}

/**
 * @brief Helper function to encapsulate reconnection.
 *
 * Attempts to reconnect and re-subscribe.
 */
static void mqttReconnect(void) {
    Debug("MQTT_Process: Detected lost connection. Attempting to reconnect...\n");
    MQTT_Connect();
    MQTT_Subscribe(NULL, globalConfig.mqttQos);
}

/**
 * @brief Connect to the MQTT broker.
 *
 * Uses an exponential backoff strategy until the connection is successful.
 * Upon connection, the default image is displayed.
 */
int MQTT_Connect(void) {
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 3;   // Send PINGREQ every 3 seconds
    conn_opts.cleansession = 1;         // Start with a clean session
    int rc;
    
    // Attempt connection with exponential backoff.
    while ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        Debug("MQTT_Connect: Failed to connect, return code %d. Reconnecting in %d seconds...\n", 
              rc, mqtt_reconnect_timeout);
        sleep(mqtt_reconnect_timeout);
        if (mqtt_reconnect_timeout * 2 < globalConfig.maxReconnectTimeout) {
            mqtt_reconnect_timeout *= 2;  // Exponential backoff.
        }
        else {
            mqtt_reconnect_timeout = globalConfig.maxReconnectTimeout;
        }        
    }
    
    // Reset reconnect timeout after a successful connection.
    mqtt_reconnect_timeout = globalConfig.initialReconnectTimeout;
    connected = 1;
    Debug("MQTT_Connect: Connected to broker at %s\n", mqtt_address);
    
    // Display the default image upon connection.    
    Display_ShowSpecialImage(globalConfig.defaultImagePath, global_dev_info, Init_Target_Memory_Addr);
    return rc;
}

/**
 * @brief Subscribe to an MQTT topic.
 *
 * Uses the provided topic or defaults to the stored topic if NULL.
 */
int MQTT_Subscribe(const char *topic, int qos) {
    const char *sub_topic = topic ? topic : mqtt_topic;
    int sub_qos = (qos < 0 ? mqtt_qos : qos);

    int rc = MQTTClient_subscribe(client, sub_topic, sub_qos);
    if (rc != MQTTCLIENT_SUCCESS) {
        Debug("MQTT_Subscribe: Failed to subscribe to topic \"%s\", return code %d\n", sub_topic, rc);
    } else {
        Debug("MQTT_Subscribe: Subscribed to topic \"%s\"\n", sub_topic);
    }
    return rc;
}

/**
 * @brief Process incoming MQTT messages.
 *
 * If the connection is lost, attempts to reconnect.
 */
void MQTT_Process(void) {
    if (!connected) {
        mqttReconnect();
    }
    MQTTClient_yield();
}

/**
 * @brief Disconnect from the MQTT broker.
 */
void MQTT_Disconnect(void) {
    MQTTClient_disconnect(client, 1000);
    connected = 0;
    Debug("MQTT_Disconnect: Disconnected from broker.\n");
}

/**
 * @brief Clean up the MQTT client.
 */
void MQTT_Cleanup(void) {
    MQTTClient_destroy(&client);
}

/**
 * @brief Set the reconnect timeout in seconds.
 *
 * @param timeout The desired timeout in seconds for reconnect attempts.
 */
void MQTT_SetReconnectTimeout(int timeout) {
    if (timeout > 0) {
        mqtt_reconnect_timeout = timeout;
    }
}