//display_app.h
#ifndef DISPLAY_APP_H
#define DISPLAY_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../lib/e-Paper/EPD_IT8951.h"  // Contains IT8951_Dev_Info and UDOUBLE.
#include "../lib/Config/DEV_Config.h"   // Hardware initialization routines.
#include <time.h>                       // For time_t

/**
 * @brief Enumerates the type of image currently displayed.
 */
typedef enum {
    IMAGE_DEFAULT,  /**< The default image is displayed. */
    IMAGE_CUSTOM    /**< A custom image (from an MQTT message) is displayed. */
} DisplayImageType;

/* Global state updated by display functions. */
extern DisplayImageType current_image_type;
extern time_t last_image_display_time;

/**
 * @brief Clears the e-Paper display.
 *
 * This function shows a blank (white) image on the display.
 *
 * @param dev_info The device information containing panel dimensions.
 * @param init_target_memory_addr The target memory address for refresh.
 */
void Display_Clear(IT8951_Dev_Info dev_info, UDOUBLE init_target_memory_addr);

/**
 * @brief Processes an incoming MQTT message to display an image.
 *
 * The JSON message must include a "Filename" key. The image is loaded from the
 * "./pic" folder. The current image type flag is updated accordingly.
 *
 * @param message The JSON message received via MQTT.
 */
void Process_MQTT_Message(const char *message);

/**
 * @brief Runs a factory test routine for the display.
 *
 * This function cycles through test patterns to verify display functionality.
 *
 * @param dev_info The device information.
 * @param init_target_memory_addr The target memory address for refresh.
 */
void Display_FactoryTest(IT8951_Dev_Info dev_info, UDOUBLE init_target_memory_addr);

/**
 * @brief Displays a special image (e.g., default or disconnected).
 *
 * This function loads an image from the specified path and updates the display.
 * It also resets the current image type to IMAGE_DEFAULT.
 *
 * @param imagePath The file path of the image.
 * @param dev_info The device information.
 * @param init_target_memory_addr The target memory address for refresh.
 */
void Display_ShowSpecialImage(const char *imagePath, IT8951_Dev_Info dev_info, UDOUBLE init_target_memory_addr);

#ifdef __cplusplus
}
#endif

#endif  // DISPLAY_APP_H