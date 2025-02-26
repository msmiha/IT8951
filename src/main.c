// main.c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

// Include headers for your application modules.
// Adjust the include paths based on your project structure.
#include "display_app.h"    // Contains functions like Display_Clear(), etc.
#include "mqtt_handler.h"   // Contains MQTT_Init(), MQTT_Connect(), MQTT_Subscribe(), MQTT_Process(), etc.
#include "../lib/Config/DEV_Config.h"  // Hardware initialization routines
#include "../lib/e-Paper/EPD_IT8951.h"  // Ensure that UDOUBLE is defined
#include "config.h"

// Define VCOM if not defined elsewhere
#define VCOM 2010

// Make sure to declare external references to the timestamp and current image type.
extern time_t last_image_display_time;
extern DisplayImageType current_image_type;

UDOUBLE Init_Target_Memory_Addr = 0;  // Global definition
IT8951_Dev_Info global_dev_info;  // Global variable to hold device info.

// Global flag to control the main loop
volatile int running = 1;

// Signal handler for graceful termination (e.g., when pressing Ctrl+C)
void signal_handler(int signo) {
    if (signo == SIGINT) {
        printf("Received SIGINT, shutting down...\n");
        running = 0;
    }
}

int main(int argc, char *argv[]) {
    // Set up signal handling so we can exit gracefully.
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        fprintf(stderr, "Failed to set signal handler\n");
        return EXIT_FAILURE;
    }

    // Load configuration from file.
    if (loadConfig("./config/config.json", &globalConfig) != 0) {
        fprintf(stderr, "Failed to load configuration. Exiting.\n");
        return EXIT_FAILURE;
    }

    // -------------------------------
    // Hardware and Display Initialization
    // -------------------------------

    // Initialize hardware (for example, the BCM2835 device)
    if (DEV_Module_Init() != 0) {
        fprintf(stderr, "Hardware initialization failed.\n");
        return EXIT_FAILURE;
    }

    // Initialize the e-Paper display and get device info.
    global_dev_info = EPD_IT8951_Init(VCOM);
    if (global_dev_info.Panel_W == 0 || global_dev_info.Panel_H == 0) {
        fprintf(stderr, "e-Paper display initialization failed.\n");
        DEV_Module_Exit();
        return EXIT_FAILURE;
    }
    UDOUBLE init_value = global_dev_info.Memory_Addr_L | (global_dev_info.Memory_Addr_H << 16);
    Init_Target_Memory_Addr = init_value;  // Assign the computed value

    
    // Clear the display before starting.
    // (Display_Clear() would be a wrapper inside display_app that calls EPD_IT8951_Clear_Refresh()
    //  using the proper mode and parameters.)
    Display_Clear(global_dev_info, Init_Target_Memory_Addr);

    // -------------------------------
    // MQTT Initialization and Setup
    // -------------------------------

    // Initialize the MQTT client.
    // For example, MQTT_Init() may set up the broker address, client ID, and topic.
    if (MQTT_Init(globalConfig.mqttAddress, globalConfig.mqttClientID, globalConfig.mqttTopic) != 0) {
        fprintf(stderr, "MQTT initialization failed.\n");
        DEV_Module_Exit();
        return EXIT_FAILURE;
    }

    // Connect to the MQTT broker.
    if (MQTT_Connect() != 0) {
        fprintf(stderr, "MQTT connection failed.\n");
        DEV_Module_Exit();
        return EXIT_FAILURE;
    }
    
    // Subscribe to the desired MQTT topic.
    MQTT_Subscribe(NULL, globalConfig.mqttQos);

    // -------------------------------
    // Main Loop
    // -------------------------------

    // In this simple loop, we simply let the MQTT handler process incoming messages.
    // In a more sophisticated design, you might use a select() loop or a proper MQTT
    // client library that provides its own event loop.

    // Initialize the timestamp so that the default image doesn't load immediately.
    last_image_display_time = time(NULL);

    while (running) {
        // Process incoming MQTT messages.
        // (This function should call your callback to display images via Process_MQTT_Message().)
        MQTT_Process();

        // Sleep briefly so we don’t busy-wait.
        sleep(1);
        // Only switch to the default image if a custom image is currently displayed
        // and the timeout has elapsed.
        if (current_image_type == IMAGE_CUSTOM && (time(NULL) - last_image_display_time >= globalConfig.defaultImageTimeout)) {
            
            Display_ShowSpecialImage(globalConfig.defaultImagePath, global_dev_info, Init_Target_Memory_Addr);
            // Set the flag to indicate default image is now displayed.
            current_image_type = IMAGE_DEFAULT;
            // Do not update last_image_display_time here, so it doesn't keep refreshing default repeatedly.
        }
    }

    // -------------------------------
    // Cleanup
    // -------------------------------

    // Disconnect and clean up the MQTT connection.
    MQTT_Disconnect();
    MQTT_Cleanup();

    // Optionally refresh the display to a blank state before exit.
    Display_Clear(global_dev_info, Init_Target_Memory_Addr);

    // Put the e-Paper display into sleep mode.
    EPD_IT8951_Sleep();

    // Clean up any hardware resources.
    DEV_Module_Exit();

    return EXIT_SUCCESS;
}