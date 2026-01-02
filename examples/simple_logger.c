/**
 * Simple Key Logger Example
 * 
 * Minimal example showing how to log all keyboard input
 * Compile: gcc simple_logger.c ../vkbd.c ../event_listener.c -o simple_logger
 * Run: sudo ./simple_logger
 */

#include "../vkbd.h"
#include "../event_listener.h"
#include <stdio.h>
#include <signal.h>
#include <linux/input-event-codes.h>

static event_listener_t *g_listener = NULL;

void signal_handler(int sig) {
    (void)sig;
    if (g_listener) {
        event_listener_stop(g_listener);
    }
}

void log_callback(uint16_t key_code, int32_t value, void *user_data) {
    (void)user_data;
    if (value == 1) {  /* Only log key press, not release or repeat */
        printf("Key pressed: code=%d\n", key_code);
    }
}

int main() {
    vkbd_context_t vkbd;
    event_listener_t listener;
    
    g_listener = &listener;
    signal(SIGINT, signal_handler);
    
    printf("Simple Key Logger\n");
    printf("Press Ctrl+C to exit\n\n");
    
    if (vkbd_init(&vkbd, "Logger Virtual Keyboard") < 0) {
        return 1;
    }
    
    vkbd_register_callback(&vkbd, log_callback, NULL);
    
    if (event_listener_init(&listener, &vkbd) < 0) {
        vkbd_destroy(&vkbd);
        return 1;
    }
    
    if (event_listener_auto_detect(&listener) < 0) {
        event_listener_destroy(&listener);
        vkbd_destroy(&vkbd);
        return 1;
    }
    
    printf("Logging started...\n");
    event_listener_run(&listener);
    
    event_listener_destroy(&listener);
    vkbd_destroy(&vkbd);
    
    return 0;
}
