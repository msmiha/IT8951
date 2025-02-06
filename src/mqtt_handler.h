#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the MQTT client.
 *
 * This function stores the broker address, client ID, and topic, and creates the MQTT client.
 *
 * @param address The MQTT broker address (e.g., "tcp://192.168.1.15:1883").
 * @param clientID The MQTT client ID.
 * @param topic The topic that the client will subscribe to.
 * @return 0 on success, or an error code on failure.
 */
int MQTT_Init(const char *address, const char *clientID, const char *topic);

/**
 * @brief Connect to the MQTT broker.
 *
 * @return 0 on success, or an error code on failure.
 */
int MQTT_Connect(void);

/**
 * @brief Subscribe to an MQTT topic.
 *
 * @param topic The topic to subscribe to.
 * @param qos The Quality of Service level (typically 0, 1, or 2).
 * @return 0 on success, or an error code on failure.
 */
int MQTT_Subscribe(const char *topic, int qos);

/**
 * @brief Process incoming MQTT messages.
 *
 * This function should be called periodically (or inside the main loop) to let the MQTT client
 * process incoming messages. With asynchronous callbacks, it may simply yield control.
 */
void MQTT_Process(void);

/**
 * @brief Disconnect from the MQTT broker.
 */
void MQTT_Disconnect(void);

/**
 * @brief Clean up the MQTT client.
 */
void MQTT_Cleanup(void);

#ifdef __cplusplus
}
#endif

#endif  // MQTT_HANDLER_H
