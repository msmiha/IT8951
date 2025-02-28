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
#include "image_cache.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


time_t last_image_display_time = 0;
DisplayImageType current_image_type = IMAGE_DEFAULT;
volatile int loading_image = 0;  // Flag indicating an image load is in progress

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

// Generic function to load and display an image with caching.
// If imagePath is non-empty, it attempts to load a pre-decoded image
// from the cache. If not present (or size mismatch), it decodes the BMP
// and then caches the result. Finally, the display is refreshed.
static void loadAndDisplayImage(const char *imagePath, IT8951_Dev_Info dev_info, UDOUBLE mem_addr) {
    struct timespec start, mid, end;
    double elapsed_load_ms, elapsed_refresh_ms;

    // Record start time (for image loading)
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (loading_image) {
        Debug("loadAndDisplayImage: Another image load is in progress. Skipping this request.\n");
        return;
    }
    loading_image = 1;

    UWORD aligned_width;
    UDOUBLE expected_buffer_size;
    computeAlignedWidthAndBufferSize(dev_info, &aligned_width, &expected_buffer_size);

    // Build paths for the BMP and the cache file.
    char bmpPath[256] = {0};
    char cachePath[256] = {0};
    if (imagePath && strlen(imagePath) > 0) {
        // Extract the base filename from imagePath.
        const char *base = strrchr(imagePath, '/');
        if (base)
            base++;  // Skip the '/' character.
        else
            base = imagePath;
        
        // Construct the BMP file path in the new location (./pic/bmp/).
        snprintf(bmpPath, sizeof(bmpPath), "./pic/bmp/%s", base);

        // Find the dot in the base filename to remove the extension.
        const char *dot = strrchr(base, '.');
        int nameLen = dot ? (dot - base) : strlen(base);

        // Ensure the cache directory exists.
        struct stat st = {0};
        if (stat("./pic/raw", &st) == -1) {
            if (mkdir("./pic/raw", 0777) != 0) {
                Debug("loadAndDisplayImage: Failed to create directory ./pic/raw/.\n");
            }
        }
        // Construct the cache file path based on the requested image.
        snprintf(cachePath, sizeof(cachePath), "./pic/raw/%.*s.raw", nameLen, base);
    }

    UBYTE *buffer = NULL;
    if (imagePath && strlen(imagePath) > 0) {
        UDOUBLE cached_size = 0;
        buffer = loadPreDecodedImage(cachePath, &cached_size);
        if (buffer && (cached_size != expected_buffer_size)) {
            Debug("loadAndDisplayImage: Cached image size (%u) does not match expected (%u). Re-decoding image.\n",
                  cached_size, expected_buffer_size);
            free(buffer);
            buffer = NULL;
        }
    }

    if (!buffer) {
        // Allocate buffer for decoding.
        buffer = (UBYTE *)malloc(expected_buffer_size);
        if (!buffer) {
            Debug("loadAndDisplayImage: Memory allocation failed.\n");
            loading_image = 0;
            return;
        }
        
        Paint_NewImage(buffer, aligned_width, dev_info.Panel_H, 0, BLACK);
        Paint_SelectImage(buffer);
        Paint_SetRotate(ROTATE_0);
        Paint_SetMirroring(MIRROR_NONE);
        Paint_SetBitsPerPixel(4);
        Paint_Clear(WHITE);

        if (imagePath && strlen(imagePath) > 0) {
            int ret = GUI_ReadBmp(bmpPath, 0, 0);
            if (ret != 0) {
                Debug("loadAndDisplayImage: Failed to load image %s, error code %d. Attempting fallback.\n", bmpPath, ret);
                // Only attempt fallback if we're not already trying to load the fallback image.
                if (strcmp(bmpPath, globalConfig.noImageAvailablePath) != 0) {
                    // Build fallback paths.
                    char fallbackBmpPath[256] = {0};
                    char fallbackCachePath[256] = {0};
                    int maxFilenameLen = sizeof(fallbackBmpPath) - strlen("./pic/bmp/") - 1;
                    
                    // Build fallback BMP path.
                    if (strchr(globalConfig.noImageAvailablePath, '/') == NULL) {
                        snprintf(fallbackBmpPath, sizeof(fallbackBmpPath), "./pic/bmp/%.*s", maxFilenameLen, globalConfig.noImageAvailablePath);
                    } else {
                        const char *fallbackBase = strrchr(globalConfig.noImageAvailablePath, '/');
                        fallbackBase = fallbackBase ? fallbackBase + 1 : globalConfig.noImageAvailablePath;
                        snprintf(fallbackBmpPath, sizeof(fallbackBmpPath), "./pic/bmp/%.*s", maxFilenameLen, fallbackBase);
                    }
                    // Build fallback cache path based on the fallback image's base name.
                    const char *fallbackBase = strrchr(fallbackBmpPath, '/');
                    fallbackBase = fallbackBase ? fallbackBase + 1 : fallbackBmpPath;
                    const char *dot = strrchr(fallbackBase, '.');
                    int fbNameLen = dot ? (dot - fallbackBase) : strlen(fallbackBase);
                    snprintf(fallbackCachePath, sizeof(fallbackCachePath), "./pic/raw/%.*s.raw", fbNameLen, fallbackBase);
                    
                    // First, try to load the fallback image from its cache.
                    UDOUBLE fallbackCachedSize = 0;
                    UBYTE *fallbackBuffer = loadPreDecodedImage(fallbackCachePath, &fallbackCachedSize);
                    if (fallbackBuffer && (fallbackCachedSize == expected_buffer_size)) {
                        Debug("loadAndDisplayImage: Loaded fallback image from cache: %s\n", fallbackCachePath);
                        free(buffer);
                        buffer = fallbackBuffer;
                        // Update cachePath to fallbackCachePath for consistency.
                        strncpy(cachePath, fallbackCachePath, sizeof(cachePath));
                    } else {
                        // Either cache not available or size mismatch; decode the fallback image.
                        ret = GUI_ReadBmp(fallbackBmpPath, 0, 0);
                        if (ret != 0) {
                            Debug("loadAndDisplayImage: Fallback image %s also failed, error code %d.\n", fallbackBmpPath, ret);
                        } else {
                            Debug("loadAndDisplayImage: Successfully loaded fallback image %s from file.\n", fallbackBmpPath);
                            // Cache the fallback image.
                            if (cachePreDecodedImage(fallbackCachePath, buffer, expected_buffer_size) != 0) {
                                Debug("loadAndDisplayImage: Failed to cache fallback pre-decoded image to %s.\n", fallbackCachePath);
                            }
                            // Update cachePath for consistency.
                            strncpy(cachePath, fallbackCachePath, sizeof(cachePath));
                        }
                    }
                }
            }
        }
        // Cache the decoded image (whether primary or fallback).
        if (imagePath && strlen(imagePath) > 0) {
            if (cachePreDecodedImage(cachePath, buffer, expected_buffer_size) != 0) {
                Debug("loadAndDisplayImage: Failed to cache pre-decoded image to %s.\n", cachePath);
            }
        }
    }

    // Record mid time after image loading/decoding.
    clock_gettime(CLOCK_MONOTONIC, &mid);
    elapsed_load_ms = (mid.tv_sec - start.tv_sec) * 1000.0 +
                      (mid.tv_nsec - start.tv_nsec) / 1000000.0;
    printf("Elapsed time Image Loading: %f ms\n", elapsed_load_ms);

    // Perform the display refresh.
    EPD_IT8951_4bp_Refresh(buffer, 0, 0, aligned_width, dev_info.Panel_H, false, mem_addr, true);

    // Record end time after refresh.
    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed_refresh_ms = (end.tv_sec - mid.tv_sec) * 1000.0 +
                         (end.tv_nsec - mid.tv_nsec) / 1000000.0;
    printf("Elapsed time Display Refresh: %f ms\n", elapsed_refresh_ms);

    free(buffer);
    last_image_display_time = time(NULL);
    loading_image = 0;
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
    
    // If the requested file is the default image and it's already loaded, do nothing.
    // (Use strcasecmp for case-insensitive comparison if needed)
    if ((strcmp(filename, getDefaultImageFilename()) == 0) && (current_image_type == IMAGE_DEFAULT)) {
        Debug("Default image already loaded.\n");
        return;
    }
    
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "./pic/%s", filename);
    Debug("Process_MQTT_Message: Displaying BMP file: %s\n", filepath);

    loadAndDisplayImage(filepath, global_dev_info, Init_Target_Memory_Addr);
    
    // Update the image type flag based on the filename.
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