//display_app.c
#include "display_app.h"
#include "../lib/GUI/GUI_Paint.h"
#include "../lib/GUI/GUI_BMPfile.h"
#include "../lib/Config/Debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <time.h>
#include "config.h"

time_t last_image_display_time = 0;
DisplayImageType current_image_type = IMAGE_DEFAULT;
//static UBYTE *Refresh_Frame_Buf = NULL;

extern IT8951_Dev_Info global_dev_info;
extern UDOUBLE Init_Target_Memory_Addr;

/* Helper to calculate aligned width and image buffer size */
static void computeAlignedWidthAndBufferSize(IT8951_Dev_Info dev_info, UWORD *aligned_width, UDOUBLE *buffer_size) {
    UWORD width = dev_info.Panel_W;
    UWORD height = dev_info.Panel_H;
    *aligned_width = width - (width % 32);
    *buffer_size = (((*aligned_width) * 4 + 7) / 8) * height;
}

static inline const char* getDefaultImageFilename() {
    const char *slash = strrchr(globalConfig.defaultImagePath, '/');
    return slash ? slash + 1 : globalConfig.defaultImagePath;
}

/* Generic function to load and display an image.
   If imagePath is non-empty, it attempts to load it.
   If that fails, it attempts to load the fallback image (NO_IMAGE_AVAILABLE_PATH).
   Finally, the display is refreshed using the given memory address.
*/
static void loadAndDisplayImage(const char *imagePath, IT8951_Dev_Info dev_info, UDOUBLE mem_addr) {
    UWORD aligned_width;
    UDOUBLE buffer_size;
    computeAlignedWidthAndBufferSize(dev_info, &aligned_width, &buffer_size);

    UBYTE *buffer = (UBYTE *)malloc(buffer_size);
    if (!buffer) {
        Debug("loadAndDisplayImage: Memory allocation failed.\n");
        return;
    }
    
    Paint_NewImage(buffer, aligned_width, dev_info.Panel_H, 0, BLACK);
    Paint_SelectImage(buffer);
    Paint_SetRotate(ROTATE_0);
    Paint_SetMirroring(MIRROR_NONE);
    Paint_SetBitsPerPixel(4);
    Paint_Clear(WHITE);

    if (imagePath && strlen(imagePath) > 0) {
        int ret = GUI_ReadBmp(imagePath, 0, 0);
        if (ret != 0) {
            Debug("loadAndDisplayImage: Failed to load image %s, error code %d. Attempting fallback.\n", imagePath, ret);
            if (strcmp(imagePath, globalConfig.noImageAvailablePath) != 0) {
                ret = GUI_ReadBmp(globalConfig.noImageAvailablePath, 0, 0);
                if (ret != 0) {
                    Debug("loadAndDisplayImage: Fallback image %s also failed, error code %d.\n", globalConfig.noImageAvailablePath, ret);
                }
            }
        }
    }
    
    EPD_IT8951_4bp_Refresh(buffer, 0, 0, aligned_width, dev_info.Panel_H, false, mem_addr, true);
    free(buffer);
    last_image_display_time = time(NULL);
}

/* Clears the display by loading a blank image */
void Display_Clear(IT8951_Dev_Info dev_info, UDOUBLE init_target_memory_addr) {
    loadAndDisplayImage("", dev_info, init_target_memory_addr);
}

/* Displays a special image (such as default or disconnected) */
void Display_ShowSpecialImage(const char *imagePath, IT8951_Dev_Info dev_info, UDOUBLE init_target_memory_addr) {
    loadAndDisplayImage(imagePath, dev_info, init_target_memory_addr);
    current_image_type = IMAGE_DEFAULT;
}

/* Process an incoming MQTT message */
void Process_MQTT_Message(const char *message) {
    if (!message || strlen(message) == 0) {
        Debug("Process_MQTT_Message: Received empty message.\n");
        return;
    }
    cJSON *json = cJSON_Parse(message);
    if (!json) {
        Debug("Process_MQTT_Message: Error parsing JSON.\n");
        return;
    }
    cJSON *filename_item = cJSON_GetObjectItemCaseSensitive(json, "Filename");
    if (!cJSON_IsString(filename_item) || !filename_item->valuestring) {
        Debug("Process_MQTT_Message: Invalid or missing 'Filename' in JSON.\n");
        cJSON_Delete(json);
        return;
    }
    char filename[128];
    strncpy(filename, filename_item->valuestring, sizeof(filename));
    filename[sizeof(filename)-1] = '\0';
    cJSON_Delete(json);
    
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "./pic/%s", filename);
    Debug("Process_MQTT_Message: Displaying BMP file: %s\n", filepath);

    loadAndDisplayImage(filepath, global_dev_info, Init_Target_Memory_Addr);
    
    // Update the image type flag. For example, if a specific filename indicates default.
    if (strcmp(filename, getDefaultImageFilename()) == 0) {
        current_image_type = IMAGE_DEFAULT;
    } else {
        current_image_type = IMAGE_CUSTOM;
    }
}

/* Factory test routine (unchanged for now) */
void Display_FactoryTest(IT8951_Dev_Info dev_info, UDOUBLE init_target_memory_addr) {
    Debug("Display_FactoryTest: Starting factory test...\n");
    Display_Clear(dev_info, init_target_memory_addr);
    DEV_Delay_ms(2000);
    Debug("Display_FactoryTest: Running pattern test...\n");
    Debug("Display_FactoryTest: Factory test completed.\n");
}