/**
 * Virtual Keyboard Library - Implementation
 * 
 * Fast, extensible uinput-based virtual keyboard for Linux
 */

#include "vkbd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <linux/input.h>

/* Write event with retry on EINTR */
static inline int write_event(int fd, const struct input_event *ev) {
    ssize_t ret;
    do {
        ret = write(fd, ev, sizeof(*ev));
    } while (ret < 0 && errno == EINTR);
    
    return (ret == sizeof(*ev)) ? 0 : -1;
}

/* Initialize virtual keyboard device */
int vkbd_init(vkbd_context_t *ctx, const char *device_name) {
    if (!ctx) {
        fprintf(stderr, "vkbd_init: NULL context\n");
        return -1;
    }

    /* Initialize context */
    memset(ctx, 0, sizeof(vkbd_context_t));
    ctx->handler_count = 0;
    ctx->device.fd = -1;

    /* Open uinput device */
    ctx->device.fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (ctx->device.fd < 0) {
        perror("vkbd_init: Failed to open /dev/uinput");
        fprintf(stderr, "Hint: Make sure uinput module is loaded (modprobe uinput)\n");
        fprintf(stderr, "Hint: You may need root privileges (sudo)\n");
        return -1;
    }

    /* Enable key events */
    if (ioctl(ctx->device.fd, UI_SET_EVBIT, EV_KEY) < 0) {
        perror("vkbd_init: Failed to set EV_KEY");
        close(ctx->device.fd);
        return -1;
    }

    /* Enable synchronization events */
    if (ioctl(ctx->device.fd, UI_SET_EVBIT, EV_SYN) < 0) {
        perror("vkbd_init: Failed to set EV_SYN");
        close(ctx->device.fd);
        return -1;
    }

    /* Enable all keyboard keys (0-255 covers most keyboard keys) */
    for (int i = 0; i < MAX_KEY_CODES; i++) {
        if (ioctl(ctx->device.fd, UI_SET_KEYBIT, i) < 0) {
            /* Some keys may not be supported, continue anyway */
        }
    }

    /* Set up device info */
    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;   /* Dummy vendor ID */
    usetup.id.product = 0x5678;  /* Dummy product ID */
    usetup.id.version = 1;
    
    strncpy(usetup.name, device_name ? device_name : "Virtual Keyboard", 
            UINPUT_MAX_NAME_SIZE - 1);
    strncpy(ctx->device.name, usetup.name, UINPUT_MAX_NAME_SIZE - 1);

    if (ioctl(ctx->device.fd, UI_DEV_SETUP, &usetup) < 0) {
        perror("vkbd_init: Failed to setup device");
        close(ctx->device.fd);
        return -1;
    }

    /* Create the device */
    if (ioctl(ctx->device.fd, UI_DEV_CREATE) < 0) {
        perror("vkbd_init: Failed to create device");
        close(ctx->device.fd);
        return -1;
    }

    ctx->device.initialized = true;
    
    /* Give kernel time to create the device */
    usleep(100000); /* 100ms */

    printf("Virtual keyboard '%s' created successfully\n", ctx->device.name);
    return 0;
}

/* Destroy virtual keyboard device */
void vkbd_destroy(vkbd_context_t *ctx) {
    if (!ctx || !ctx->device.initialized) {
        return;
    }

    if (ctx->device.fd >= 0) {
        ioctl(ctx->device.fd, UI_DEV_DESTROY);
        close(ctx->device.fd);
        ctx->device.fd = -1;
    }

    ctx->device.initialized = false;
    printf("Virtual keyboard destroyed\n");
}

/* Send key event to virtual keyboard */
int vkbd_send_key(vkbd_context_t *ctx, uint16_t key_code, int32_t value) {
    if (!ctx || !ctx->device.initialized) {
        fprintf(stderr, "vkbd_send_key: Device not initialized\n");
        return -1;
    }

    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    
    ev.type = EV_KEY;
    ev.code = key_code;
    ev.value = value;
    
    /* Get current time for event timestamp */
    gettimeofday(&ev.time, NULL);

    if (write_event(ctx->device.fd, &ev) < 0) {
        perror("vkbd_send_key: Failed to write key event");
        return -1;
    }

    return 0;
}

/* Send synchronization event */
int vkbd_sync(vkbd_context_t *ctx) {
    if (!ctx || !ctx->device.initialized) {
        fprintf(stderr, "vkbd_sync: Device not initialized\n");
        return -1;
    }

    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    
    gettimeofday(&ev.time, NULL);

    if (write_event(ctx->device.fd, &ev) < 0) {
        perror("vkbd_sync: Failed to write sync event");
        return -1;
    }

    return 0;
}

/* Register a callback for key events */
int vkbd_register_callback(vkbd_context_t *ctx, vkbd_callback_t callback, void *user_data) {
    if (!ctx) {
        fprintf(stderr, "vkbd_register_callback: NULL context\n");
        return -1;
    }

    if (!callback) {
        fprintf(stderr, "vkbd_register_callback: NULL callback\n");
        return -1;
    }

    if (ctx->handler_count >= MAX_CALLBACKS) {
        fprintf(stderr, "vkbd_register_callback: Too many callbacks\n");
        return -1;
    }

    int handler_id = ctx->handler_count;
    ctx->handlers[handler_id].callback = callback;
    ctx->handlers[handler_id].user_data = user_data;
    ctx->handlers[handler_id].active = true;
    ctx->handler_count++;

    return handler_id;
}

/* Unregister a callback */
int vkbd_unregister_callback(vkbd_context_t *ctx, int handler_id) {
    if (!ctx) {
        fprintf(stderr, "vkbd_unregister_callback: NULL context\n");
        return -1;
    }

    if (handler_id < 0 || handler_id >= ctx->handler_count) {
        fprintf(stderr, "vkbd_unregister_callback: Invalid handler ID\n");
        return -1;
    }

    ctx->handlers[handler_id].active = false;
    return 0;
}

/* Process and forward key event - Maximum speed with robust error handling */
int vkbd_process_key(vkbd_context_t *ctx, uint16_t key_code, int32_t value) {
    /* Fast path: assume valid context (hot path optimization) */
    if (__builtin_expect(!ctx || !ctx->device.initialized, 0)) {
        return -1;
    }

    /* Inline callback processing - no function calls */
    const int count = ctx->handler_count;
    for (int i = 0; i < count; i++) {
        if (ctx->handlers[i].active) {
            ctx->handlers[i].callback(key_code, value, ctx->handlers[i].user_data);
        }
    }

    /* Pre-allocated events on stack - no heap allocation */
    struct input_event events[2];
    struct timeval now;
    gettimeofday(&now, NULL);
    
    /* Key event */
    events[0].time = now;
    events[0].type = EV_KEY;
    events[0].code = key_code;
    events[0].value = value;
    
    /* Sync event */
    events[1].time = now;
    events[1].type = EV_SYN;
    events[1].code = SYN_REPORT;
    events[1].value = 0;
    
    /* Single atomic write - retry on EINTR or EAGAIN */
    ssize_t ret;
    int retry = 0;
    do {
        ret = write(ctx->device.fd, events, sizeof(events));
        if (ret == sizeof(events)) {
            return 0;  /* Success */
        }
        /* Retry on interrupt or would-block */
        if (ret < 0 && (errno == EINTR || errno == EAGAIN)) {
            if (++retry > 10) {
                /* Too many retries - buffer might be full from key repeat */
                usleep(100);  /* 0.1ms backoff */
                if (retry > 100) {
                    break;  /* Give up after 100 retries */
                }
            }
            continue;
        }
        /* Other errors - fail immediately */
        break;
    } while (1);
    
    /* Log error only if write completely failed */
    if (__builtin_expect(ret < 0, 0)) {
        static int error_logged = 0;
        if (!error_logged) {
            perror("vkbd_process_key: write failed");
            error_logged = 1;  /* Prevent log spam */
        }
        return -1;
    }
    
    return 0;
}

/* Get device file descriptor */
int vkbd_get_fd(vkbd_context_t *ctx) {
    if (!ctx || !ctx->device.initialized) {
        return -1;
    }
    return ctx->device.fd;
}
