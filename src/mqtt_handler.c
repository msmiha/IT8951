#include "mqtt_handler.h"
#include "../lib/Config/Debug.h"    // For logging/debugging functions
#include "display_app.h"            // For calling Process_MQTT_Message() when a message arrives
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"             // Paho MQTT Client header

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

// Flag indicating connection status.
static int connected = 0;

/**
 * @brief Callback function invoked when a message arrives.
 *
 * This callback simply logs the received message and calls the display module to process it.
 */
static int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    Debug("MQTT: Message received on topic \"%s\": %s\n", topicName, (char *)message->payload);

    // Call the display module to process the message (e.g., to display an image).
    Process_MQTT_Message((char *)message->payload);

    // Free the message and topic memory.
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    
    return 1;  // Return 1 to indicate successful processing.
}

int MQTT_Init(const char *address, const char *clientID, const char *topic) {
    if (address == NULL || clientID == NULL || topic == NULL) {
        Debug("MQTT_Init: Invalid parameters provided.\n");
        return -1;
    }
    
    // Copy parameters into static storage.
    strncpy(mqtt_address, address, MAX_ADDR_LEN - 1);
    mqtt_address[MAX_ADDR_LEN - 1] = '\0';
    
    strncpy(mqtt_clientID, clientID, MAX_CLIENTID_LEN - 1);
    mqtt_clientID[MAX_CLIENTID_LEN - 1] = '\0';
    
    strncpy(mqtt_topic, topic, MAX_TOPIC_LEN - 1);
    mqtt_topic[MAX_TOPIC_LEN - 1] = '\0';

    // Create the MQTT client.
    int rc = MQTTClient_create(&client, mqtt_address, mqtt_clientID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        Debug("MQTT_Init: Failed to create MQTT client, return code %d.\n", rc);
        return rc;
    }

    // Set up callbacks.
    MQTTClient_setCallbacks(client, NULL, NULL, messageArrived, NULL);
    return 0;
}

int MQTT_Connect(void) {
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    
    int rc = MQTTClient_connect(client, &conn_opts);
    if (rc == MQTTCLIENT_SUCCESS) {
        connected = 1;
        Debug("MQTT_Connect: Connected to broker at %s\n", mqtt_address);
    } else {
        Debug("MQTT_Connect: Failed to connect, return code %d\n", rc);
    }
    return rc;
}

int MQTT_Subscribe(const char *topic, int qos) {
    // If topic is NULL, use the default stored topic.
    const char *sub_topic = topic ? topic : mqtt_topic;
    
    // If qos is negative, use the default mqtt_qos; otherwise, use the provided qos.
    int sub_qos = (qos < 0 ? mqtt_qos : qos);

    int rc = MQTTClient_subscribe(client, sub_topic, sub_qos);
    if (rc != MQTTCLIENT_SUCCESS) {
        Debug("MQTT_Subscribe: Failed to subscribe to topic \"%s\", return code %d\n", sub_topic, rc);
    } else {
        Debug("MQTT_Subscribe: Subscribed to topic \"%s\"\n", sub_topic);
    }
    return rc;
}

void MQTT_Process(void) {
    // If using asynchronous callbacks, the client processes messages in its own thread.
    // However, if needed, call yield to allow the MQTT client to dispatch messages.
    MQTTClient_yield();
}

void MQTT_Disconnect(void) {
    MQTTClient_disconnect(client, 1000);
    connected = 0;
    Debug("MQTT_Disconnect: Disconnected from broker.\n");
}

void MQTT_Cleanup(void) {
    MQTTClient_destroy(&client);
}
