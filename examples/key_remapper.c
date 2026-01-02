/**
 * Key Remapper Example
 * 
 * Demonstrates remapping keys (e.g., Caps Lock to Escape)
 * This is a more advanced example showing how to modify key events
 * 
 * Compile: gcc key_remapper.c ../vkbd.c ../event_listener.c -o key_remapper
 * Run: sudo ./key_remapper
 */

#include "../vkbd.h"
#include "../event_listener.h"
#include <stdio.h>
#include <signal.h>
#include <linux/input-event-codes.h>

static event_listener_t *g_listener = NULL;
static vkbd_context_t *g_vkbd = NULL;

void signal_handler(int sig) {
    (void)sig;
    printf("\nShutting down...\n");
    if (g_listener) {
        event_listener_stop(g_listener);
    }
}

/* Custom callback that remaps keys */
void remap_callback(uint16_t key_code, int32_t value, void *user_data) {
    (void)user_data;
    
    /* Example remapping rules */
    uint16_t new_key_code = key_code;
    
    /* Caps Lock -> Escape */
    if (key_code == KEY_CAPSLOCK) {
        new_key_code = KEY_ESC;
        printf("[REMAP] CAPSLOCK -> ESC\n");
    }
    
    /* Right Alt -> Right Ctrl (for ergonomic reasons) */
    else if (key_code == KEY_RIGHTALT) {
        new_key_code = KEY_RIGHTCTRL;
        printf("[REMAP] RIGHTALT -> RIGHTCTRL\n");
    }
    
    /* Send the (possibly remapped) key */
    if (g_vkbd) {
        vkbd_send_key(g_vkbd, new_key_code, value);
        vkbd_sync(g_vkbd);
    }
}

int main() {
    vkbd_context_t vkbd;
    event_listener_t listener;
    
    g_vkbd = &vkbd;
    g_listener = &listener;
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("=== Key Remapper ===\n");
    printf("Active remappings:\n");
    printf("  - Caps Lock  -> Escape\n");
    printf("  - Right Alt  -> Right Ctrl\n");
    printf("\nPress Ctrl+C to exit\n\n");
    
    if (vkbd_init(&vkbd, "Remapper Virtual Keyboard") < 0) {
        fprintf(stderr, "Failed to initialize virtual keyboard\n");
        return 1;
    }
    
    /* Register remapping callback */
    if (vkbd_register_callback(&vkbd, remap_callback, NULL) < 0) {
        fprintf(stderr, "Failed to register callback\n");
        vkbd_destroy(&vkbd);
        return 1;
    }
    
    if (event_listener_init(&listener, &vkbd) < 0) {
        fprintf(stderr, "Failed to initialize event listener\n");
        vkbd_destroy(&vkbd);
        return 1;
    }
    
    if (event_listener_auto_detect(&listener) < 0) {
        fprintf(stderr, "Failed to detect keyboard devices\n");
        event_listener_destroy(&listener);
        vkbd_destroy(&vkbd);
        return 1;
    }
    
    printf("Remapper active! Try pressing Caps Lock or Right Alt\n\n");
    event_listener_run(&listener);
    
    printf("\nCleaning up...\n");
    event_listener_destroy(&listener);
    vkbd_destroy(&vkbd);
    
    printf("Goodbye!\n");
    return 0;
}
