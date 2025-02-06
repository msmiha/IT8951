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

// Define VCOM if not defined elsewhere
#define VCOM 2010

#define FUNCTION "tm"
#define DEV_ID   "1"


// Define MQTT parameters
#define MQTT_ADDRESS "mqtt://192.168.1.15:1883"
#define MQTT_CLIENTID "EpaperDisplay_" FUNCTION DEV_ID
#define MQTT_TOPIC  "fontana/"FUNCTION"/"DEV_ID"/epaper/image"
#define MQTT_QOS 0

UDOUBLE Init_Target_Memory_Addr = 0;  // Global definition

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

    // -------------------------------
    // Hardware and Display Initialization
    // -------------------------------

    // Initialize hardware (for example, the BCM2835 device)
    if (DEV_Module_Init() != 0) {
        fprintf(stderr, "Hardware initialization failed.\n");
        return EXIT_FAILURE;
    }

    // Initialize the e-Paper display and get device info.
    IT8951_Dev_Info dev_info = EPD_IT8951_Init(VCOM);
    if (dev_info.Panel_W == 0 || dev_info.Panel_H == 0) {
        fprintf(stderr, "e-Paper display initialization failed.\n");
        DEV_Module_Exit();
        return EXIT_FAILURE;
    }
    UDOUBLE init_value = dev_info.Memory_Addr_L | (dev_info.Memory_Addr_H << 16);
    Init_Target_Memory_Addr = init_value;  // Assign the computed value

    
    // Clear the display before starting.
    // (Display_Clear() would be a wrapper inside display_app that calls EPD_IT8951_Clear_Refresh()
    //  using the proper mode and parameters.)
    Display_Clear(dev_info, Init_Target_Memory_Addr);

    // -------------------------------
    // MQTT Initialization and Setup
    // -------------------------------

    // Initialize the MQTT client.
    // For example, MQTT_Init() may set up the broker address, client ID, and topic.
    if (MQTT_Init(MQTT_ADDRESS, MQTT_CLIENTID, MQTT_TOPIC) != 0) {
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
    MQTT_Subscribe(NULL, MQTT_QOS);

    // -------------------------------
    // Main Loop
    // -------------------------------

    // In this simple loop, we simply let the MQTT handler process incoming messages.
    // In a more sophisticated design, you might use a select() loop or a proper MQTT
    // client library that provides its own event loop.
    while (running) {
        // Process incoming MQTT messages.
        // (This function should call your callback to display images via Process_MQTT_Message().)
        MQTT_Process();

        // Sleep briefly so we don’t busy-wait.
        sleep(1);
    }

    // -------------------------------
    // Cleanup
    // -------------------------------

    // Disconnect and clean up the MQTT connection.
    MQTT_Disconnect();
    MQTT_Cleanup();

    // Optionally refresh the display to a blank state before exit.
    Display_Clear(dev_info, Init_Target_Memory_Addr);

    // Put the e-Paper display into sleep mode.
    EPD_IT8951_Sleep();

    // Clean up any hardware resources.
    DEV_Module_Exit();

    return EXIT_SUCCESS;
}
