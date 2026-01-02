/**
 * Event Listener Module
 * 
 * Captures input from real keyboard devices and forwards to virtual keyboard
 * Uses epoll for efficient event monitoring
 */

#ifndef EVENT_LISTENER_H
#define EVENT_LISTENER_H

#include "vkbd.h"
#include <stdbool.h>

/* Maximum number of input devices to monitor */
#define MAX_INPUT_DEVICES 16

/* Input device structure */
typedef struct {
    int fd;
    char path[256];
    char name[256];
    bool active;
} input_device_t;

/* Event listener context */
typedef struct {
    input_device_t devices[MAX_INPUT_DEVICES];
    int device_count;
    int epoll_fd;
    bool running;
    vkbd_context_t *vkbd_ctx;
} event_listener_t;

/**
 * Initialize event listener
 * 
 * @param listener Pointer to event_listener_t structure
 * @param vkbd_ctx Pointer to virtual keyboard context
 * @return 0 on success, -1 on error
 */
int event_listener_init(event_listener_t *listener, vkbd_context_t *vkbd_ctx);

/**
 * Add input device to monitor
 * 
 * @param listener Pointer to event_listener_t structure
 * @param device_path Path to input device (e.g., /dev/input/event0)
 * @return 0 on success, -1 on error
 */
int event_listener_add_device(event_listener_t *listener, const char *device_path);

/**
 * Auto-detect and add all keyboard devices
 * 
 * @param listener Pointer to event_listener_t structure
 * @return Number of devices added, -1 on error
 */
int event_listener_auto_detect(event_listener_t *listener);

/**
 * Start listening for events (blocking)
 * 
 * @param listener Pointer to event_listener_t structure
 * @return 0 on success, -1 on error
 */
int event_listener_run(event_listener_t *listener);

/**
 * Stop listening for events
 * 
 * @param listener Pointer to event_listener_t structure
 */
void event_listener_stop(event_listener_t *listener);

/**
 * Destroy event listener and clean up resources
 * 
 * @param listener Pointer to event_listener_t structure
 */
void event_listener_destroy(event_listener_t *listener);

#endif /* EVENT_LISTENER_H */
