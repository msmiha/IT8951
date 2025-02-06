#ifndef DISPLAY_APP_H
#define DISPLAY_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../lib/e-Paper/EPD_IT8951.h"  // Adjust path as needed (contains IT8951_Dev_Info, etc.)
#include "../lib/Config/DEV_Config.h"  // For DEV_Module functions and any global definitions

/**
 * @brief Clear the e-Paper display.
 *
 * This function refreshes the display with a blank (white) image.
 *
 * @param dev_info The IT8951 device info structure (contains panel width, height, etc.).
 * @param init_target_memory_addr The initial target memory address for display refresh.
 */
void Display_Clear(IT8951_Dev_Info dev_info, UDOUBLE init_target_memory_addr);

/**
 * @brief Process an incoming MQTT message to display a BMP image.
 *
 * Expects a JSON string with a "Filename" field. The BMP file is read
 * from the "./pic" folder.
 *
 * @param message The JSON message received via MQTT.
 */
void Process_MQTT_Message(const char *message);

/**
 * @brief Run a simple factory test routine.
 *
 * This function cycles through test patterns and/or images to verify the
 * display functionality.
 *
 * @param dev_info The IT8951 device info structure.
 * @param init_target_memory_addr The initial target memory address.
 */
void Display_FactoryTest(IT8951_Dev_Info dev_info, UDOUBLE init_target_memory_addr);

#ifdef __cplusplus
}
#endif

#endif  // DISPLAY_APP_H
