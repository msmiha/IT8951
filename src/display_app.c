#include "display_app.h"
#include "../lib/GUI/GUI_Paint.h"      // Contains Paint_NewImage(), Paint_SelectImage(), etc.
#include "../lib/GUI/GUI_BMPfile.h"    // Contains GUI_ReadBmp()
#include "../lib/Config/Debug.h"       // Debug() macro or function for logging
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cjson/cJSON.h>

// Global frame buffer pointer (used during image display)
static UBYTE *Refresh_Frame_Buf = NULL;

/**
 * @brief Clear the e-Paper display.
 *
 * This function creates a blank image and calls the low-level refresh routine.
 */
void Display_Clear(IT8951_Dev_Info dev_info, UDOUBLE init_target_memory_addr) {
    // Determine panel dimensions; you might need to adjust these values.
    UWORD panel_width  = dev_info.Panel_W;
    UWORD panel_height = dev_info.Panel_H;
    // Ensure the width is 32-bit aligned if required.
    UWORD aligned_width = panel_width - (panel_width % 32);

    // Calculate the image size for 4 bits per pixel.
    UDOUBLE image_size = ((aligned_width * 4 + 7) / 8) * panel_height;

    Refresh_Frame_Buf = (UBYTE *)malloc(image_size);
    if (Refresh_Frame_Buf == NULL) {
        Debug("Display_Clear: Failed to allocate memory for frame buffer.\n");
        return;
    }

    // Create a new image (using black as the default color, then clear to white)
    Paint_NewImage(Refresh_Frame_Buf, aligned_width, panel_height, 0, BLACK);
    Paint_SelectImage(Refresh_Frame_Buf);
    Paint_SetRotate(ROTATE_0);
    Paint_SetMirroring(MIRROR_NONE);
    Paint_SetBitsPerPixel(4);
    Paint_Clear(WHITE);

    // Use the low-level refresh function.
    // (INIT_Mode is assumed defined in your project – adjust if needed.)
    EPD_IT8951_4bp_Refresh(Refresh_Frame_Buf, 0, 0, aligned_width, panel_height, false, init_target_memory_addr, true);

    free(Refresh_Frame_Buf);
    Refresh_Frame_Buf = NULL;
}

/**
 * @brief Process an incoming MQTT message to display a BMP image.
 *
 * The JSON message must contain a "Filename" key with the name of the BMP file.
 */
void Process_MQTT_Message(const char *message) {
    if (message == NULL || strlen(message) == 0) {
        Debug("Process_MQTT_Message: Received empty message.\n");
        return;
    }

    // Parse the JSON message.
    cJSON *json = cJSON_Parse(message);
    if (json == NULL) {
        Debug("Process_MQTT_Message: Error parsing JSON.\n");
        return;
    }

    // Extract the filename.
    cJSON *filename_item = cJSON_GetObjectItemCaseSensitive(json, "Filename");
    if (!cJSON_IsString(filename_item) || (filename_item->valuestring == NULL)) {
        Debug("Process_MQTT_Message: Invalid or missing 'Filename' in JSON.\n");
        cJSON_Delete(json);
        return;
    }

    char filename[128];
    strncpy(filename, filename_item->valuestring, sizeof(filename));
    filename[sizeof(filename) - 1] = '\0';
    cJSON_Delete(json);

    // Construct the file path (assuming images are stored in the "pic" folder)
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "./pic/%s", filename);
    Debug("Process_MQTT_Message: Displaying BMP file: %s\n", filepath);

    // Set panel dimensions; adjust these to match your display.
    UWORD panel_width  = 1448;
    UWORD panel_height = 1072;
    UWORD aligned_width = panel_width - (panel_width % 32);
    UDOUBLE image_size = ((aligned_width * 4 + 7) / 8) * panel_height;

    Refresh_Frame_Buf = (UBYTE *)malloc(image_size);
    if (Refresh_Frame_Buf == NULL) {
        Debug("Process_MQTT_Message: Failed to allocate memory for frame buffer.\n");
        return;
    }

    // Create a new image and prepare the drawing environment.
    Paint_NewImage(Refresh_Frame_Buf, aligned_width, panel_height, 0, BLACK);
    Paint_SelectImage(Refresh_Frame_Buf);
    Paint_SetRotate(ROTATE_0);
    Paint_SetMirroring(MIRROR_NONE);
    Paint_SetBitsPerPixel(4);
    Paint_Clear(WHITE);

    // Read and draw the BMP image onto the frame buffer.
    if (GUI_ReadBmp(filepath, 0, 0) != 0) {
        Debug("Process_MQTT_Message: Failed to read BMP file: %s\n", filepath);
    } else {
        // Assume that Init_Target_Memory_Addr is defined elsewhere or passed to this module.
        extern UDOUBLE Init_Target_Memory_Addr;
        EPD_IT8951_4bp_Refresh(Refresh_Frame_Buf, 0, 0, aligned_width, panel_height, false, Init_Target_Memory_Addr, true);
    }

    free(Refresh_Frame_Buf);
    Refresh_Frame_Buf = NULL;
}

/**
 * @brief Run a simple factory test routine.
 *
 * This routine cycles through a few test patterns and images.
 */
void Display_FactoryTest(IT8951_Dev_Info dev_info, UDOUBLE init_target_memory_addr) {
    Debug("Display_FactoryTest: Starting factory test...\n");

    // Clear the display.
    Display_Clear(dev_info, init_target_memory_addr);
    DEV_Delay_ms(2000);

    // You can add more test routines here, e.g.,:
    // - Display test patterns
    // - Run dynamic refresh tests
    // - Display a series of BMP images
    // For demonstration, we simply log the test progress.
    Debug("Display_FactoryTest: Running pattern test...\n");
    // ... (insert additional tests as needed) ...

    Debug("Display_FactoryTest: Factory test completed.\n");
}
