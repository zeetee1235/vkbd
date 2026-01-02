/**
 * Virtual Keyboard Library - Header
 * 
 * Fast, extensible uinput-based virtual keyboard for Linux
 * Allows intercepting keyboard input, processing it, and forwarding to games/applications
 */

#ifndef VKBD_H
#define VKBD_H

#include <linux/uinput.h>
#include <stdint.h>
#include <stdbool.h>

/* Maximum number of callback handlers */
#define MAX_CALLBACKS 16

/* Maximum number of key codes to enable */
#define MAX_KEY_CODES 256

/* Virtual keyboard device structure */
typedef struct {
    int fd;                          /* uinput device file descriptor */
    char name[UINPUT_MAX_NAME_SIZE]; /* Device name */
    bool initialized;                /* Initialization status */
} vkbd_device_t;

/* Key event callback function type */
typedef void (*vkbd_callback_t)(uint16_t key_code, int32_t value, void *user_data);

/* Callback handler structure */
typedef struct {
    vkbd_callback_t callback;
    void *user_data;
    bool active;
} vkbd_handler_t;

/* Virtual keyboard context */
typedef struct {
    vkbd_device_t device;
    vkbd_handler_t handlers[MAX_CALLBACKS];
    int handler_count;
} vkbd_context_t;

/**
 * Initialize virtual keyboard device
 * 
 * @param ctx Pointer to vkbd_context_t structure
 * @param device_name Name of the virtual device (shown in /dev/input)
 * @return 0 on success, -1 on error
 */
int vkbd_init(vkbd_context_t *ctx, const char *device_name);

/**
 * Destroy virtual keyboard device
 * 
 * @param ctx Pointer to vkbd_context_t structure
 */
void vkbd_destroy(vkbd_context_t *ctx);

/**
 * Send key event to virtual keyboard
 * 
 * @param ctx Pointer to vkbd_context_t structure
 * @param key_code Linux key code (from linux/input-event-codes.h)
 * @param value 0=release, 1=press, 2=repeat
 * @return 0 on success, -1 on error
 */
int vkbd_send_key(vkbd_context_t *ctx, uint16_t key_code, int32_t value);

/**
 * Send synchronization event (EV_SYN)
 * 
 * @param ctx Pointer to vkbd_context_t structure
 * @return 0 on success, -1 on error
 */
int vkbd_sync(vkbd_context_t *ctx);

/**
 * Register a callback for key events
 * 
 * @param ctx Pointer to vkbd_context_t structure
 * @param callback Callback function to be called on key events
 * @param user_data User data passed to callback
 * @return Handler ID (>= 0) on success, -1 on error
 */
int vkbd_register_callback(vkbd_context_t *ctx, vkbd_callback_t callback, void *user_data);

/**
 * Unregister a callback
 * 
 * @param ctx Pointer to vkbd_context_t structure
 * @param handler_id Handler ID returned by vkbd_register_callback
 * @return 0 on success, -1 on error
 */
int vkbd_unregister_callback(vkbd_context_t *ctx, int handler_id);

/**
 * Process and forward key event (calls callbacks then sends to virtual device)
 * 
 * @param ctx Pointer to vkbd_context_t structure
 * @param key_code Linux key code
 * @param value Key state (0=release, 1=press, 2=repeat)
 * @return 0 on success, -1 on error
 */
int vkbd_process_key(vkbd_context_t *ctx, uint16_t key_code, int32_t value) __attribute__((hot));

/**
 * Get device file descriptor (for epoll/select integration)
 * 
 * @param ctx Pointer to vkbd_context_t structure
 * @return File descriptor or -1 if not initialized
 */
int vkbd_get_fd(vkbd_context_t *ctx);

#endif /* VKBD_H */
