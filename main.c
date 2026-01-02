/**
 * Virtual Keyboard - Example Usage
 * 
 * Demonstrates how to use the virtual keyboard library
 * This example intercepts keyboard input, adds custom processing, and forwards it
 */

#include "vkbd.h"
#include "event_listener.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <linux/input-event-codes.h>

/* Global variables for signal handling */
static vkbd_context_t *g_vkbd_ctx = NULL;
static event_listener_t *g_listener = NULL;

/* Signal handler for clean shutdown */
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    
    if (g_listener) {
        event_listener_stop(g_listener);
    }
}

/* Example callback: Print key events */
void key_logger_callback(uint16_t key_code, int32_t value, void *user_data) {
    (void)user_data; /* Unused */
    const char *action = NULL;
    
    switch (value) {
        case 0: action = "RELEASE"; break;
        case 1: action = "PRESS  "; break;
        case 2: action = "REPEAT "; break;
        default: action = "UNKNOWN"; break;
    }
    
    printf("[KEY] %s: code=%d\n", action, key_code);
}

/* Example callback: Play sound on keypress (placeholder) */
void key_sound_callback(uint16_t key_code, int32_t value, void *user_data) {
    (void)user_data; /* Unused */
    /* Only on key press (not release or repeat) */
    if (value == 1) {
        /* TODO: Add actual sound playback here
         * This is where you would integrate with a sound library
         * like ALSA, PulseAudio, or SDL_mixer
         * 
         * Example:
         * play_sound("keysound.wav");
         */
        
        /* For demonstration, just print */
        printf("[SOUND] Playing key sound for key %d\n", key_code);
    }
}

/* Example callback: Custom key mapping */
void key_mapper_callback(uint16_t key_code, int32_t value, void *user_data) {
    (void)value;     /* Unused */
    (void)user_data; /* Unused */
    /* Example: Remap Caps Lock to Escape */
    if (key_code == KEY_CAPSLOCK) {
        printf("[MAPPER] Remapping CAPSLOCK to ESC (in callback)\n");
        /* Note: This is just logging - actual remapping would need
         * to modify the key_code before it's sent to the virtual device
         */
    }
}

int main(int argc, char *argv[]) {
    (void)argc; /* Unused */
    (void)argv; /* Unused */
    int ret = 0;
    vkbd_context_t vkbd_ctx;
    event_listener_t listener;
    
    /* Set global pointers for signal handler */
    g_vkbd_ctx = &vkbd_ctx;
    g_listener = &listener;

    printf("=== Virtual Keyboard Example ===\n");
    printf("This program intercepts keyboard input and forwards it through a virtual device\n");
    printf("Press Ctrl+C to exit\n\n");

    /* Check if running with sufficient privileges */
    if (geteuid() != 0) {
        fprintf(stderr, "Warning: Not running as root. You may need sudo for full functionality.\n");
        fprintf(stderr, "Some features (like grabbing devices) may not work.\n\n");
    }

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize virtual keyboard */
    printf("Initializing virtual keyboard...\n");
    if (vkbd_init(&vkbd_ctx, "Virtual Keyboard Example") < 0) {
        fprintf(stderr, "Failed to initialize virtual keyboard\n");
        return 1;
    }

    /* Register callbacks */
    printf("Registering callbacks...\n");
    
    int logger_id = vkbd_register_callback(&vkbd_ctx, key_logger_callback, NULL);
    if (logger_id < 0) {
        fprintf(stderr, "Failed to register logger callback\n");
        goto cleanup;
    }
    
    int sound_id = vkbd_register_callback(&vkbd_ctx, key_sound_callback, NULL);
    if (sound_id < 0) {
        fprintf(stderr, "Failed to register sound callback\n");
        goto cleanup;
    }

    int mapper_id = vkbd_register_callback(&vkbd_ctx, key_mapper_callback, NULL);
    if (mapper_id < 0) {
        fprintf(stderr, "Failed to register mapper callback\n");
        goto cleanup;
    }

    printf("Registered %d callbacks\n", 3);

    /* Initialize event listener */
    printf("Initializing event listener...\n");
    if (event_listener_init(&listener, &vkbd_ctx) < 0) {
        fprintf(stderr, "Failed to initialize event listener\n");
        goto cleanup;
    }

    /* Auto-detect keyboard devices */
    printf("Auto-detecting keyboard devices...\n");
    if (event_listener_auto_detect(&listener) < 0) {
        fprintf(stderr, "Failed to detect keyboard devices\n");
        fprintf(stderr, "Make sure you have permission to access /dev/input/event* devices\n");
        goto cleanup;
    }

    printf("\n=== Virtual keyboard is now active ===\n");
    printf("All keyboard input will be intercepted and forwarded\n");
    printf("Check the output to see key events being processed\n\n");

    /* Start listening (blocking) */
    ret = event_listener_run(&listener);

cleanup:
    printf("\nCleaning up...\n");
    
    /* Destroy listener */
    event_listener_destroy(&listener);
    
    /* Destroy virtual keyboard */
    vkbd_destroy(&vkbd_ctx);
    
    printf("Goodbye!\n");
    return ret;
}
