/**
 * Event Listener Module - Implementation
 * 
 * Captures input from real keyboard devices and forwards to virtual keyboard
 */

#include "event_listener.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#define INPUT_DIR "/dev/input"
#define MAX_EVENTS 64

/* Bit manipulation macros */
#define NBITS(x) ((((x) - 1) / (sizeof(long) * 8)) + 1)
#define OFFSET(x) ((x) % (sizeof(long) * 8))
#define BIT(x) (1UL << OFFSET(x))
#define LONG(x) ((x) / (sizeof(long) * 8))
#define test_bit(bit, array) ((array[LONG(bit)] >> OFFSET(bit)) & 1)

/* Check if device is a keyboard */
static bool is_keyboard(int fd) {
    unsigned long evbit[NBITS(EV_MAX)] = {0};
    unsigned long keybit[NBITS(KEY_MAX)] = {0};

    /* Get event types supported */
    if (ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), evbit) < 0) {
        return false;
    }

    /* Check if device supports key events */
    if (!test_bit(EV_KEY, evbit)) {
        return false;
    }

    /* Get key codes supported */
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keybit)), keybit) < 0) {
        return false;
    }

    /* Check for typical keyboard keys */
    bool has_keyboard_keys = false;
    for (int i = KEY_Q; i <= KEY_P; i++) {
        if (test_bit(i, keybit)) {
            has_keyboard_keys = true;
            break;
        }
    }

    return has_keyboard_keys;
}

/* Initialize event listener */
int event_listener_init(event_listener_t *listener, vkbd_context_t *vkbd_ctx) {
    if (!listener) {
        fprintf(stderr, "event_listener_init: NULL listener\n");
        return -1;
    }

    if (!vkbd_ctx) {
        fprintf(stderr, "event_listener_init: NULL vkbd_ctx\n");
        return -1;
    }

    memset(listener, 0, sizeof(event_listener_t));
    listener->vkbd_ctx = vkbd_ctx;
    listener->device_count = 0;
    listener->running = false;
    listener->epoll_fd = -1;

    /* Create epoll instance */
    listener->epoll_fd = epoll_create1(0);
    if (listener->epoll_fd < 0) {
        perror("event_listener_init: Failed to create epoll");
        return -1;
    }

    return 0;
}

/* Add input device to monitor */
int event_listener_add_device(event_listener_t *listener, const char *device_path) {
    if (!listener || !device_path) {
        fprintf(stderr, "event_listener_add_device: Invalid arguments\n");
        return -1;
    }

    if (listener->device_count >= MAX_INPUT_DEVICES) {
        fprintf(stderr, "event_listener_add_device: Too many devices\n");
        return -1;
    }

    /* Open device */
    int fd = open(device_path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("event_listener_add_device: Failed to open device");
        return -1;
    }

    /* Check if it's a keyboard */
    if (!is_keyboard(fd)) {
        fprintf(stderr, "Device %s is not a keyboard\n", device_path);
        close(fd);
        return -1;
    }

    /* Get device name */
    char name[256] = "Unknown";
    ioctl(fd, EVIOCGNAME(sizeof(name)), name);

    /* Grab device (exclusive access) */
    /* This prevents the original keyboard from sending events to other apps */
    /* Comment this out if you want to test without exclusive access */
    if (ioctl(fd, EVIOCGRAB, 1) < 0) {
        fprintf(stderr, "Warning: Could not grab device %s (may need root)\n", device_path);
        fprintf(stderr, "         Events will still be captured but also sent to system\n");
        /* Continue anyway - useful for testing without breaking system input */
    }

    /* Add to epoll */
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;

    if (epoll_ctl(listener->epoll_fd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        perror("event_listener_add_device: Failed to add to epoll");
        close(fd);
        return -1;
    }

    /* Store device info */
    int idx = listener->device_count;
    listener->devices[idx].fd = fd;
    strncpy(listener->devices[idx].path, device_path, sizeof(listener->devices[idx].path) - 1);
    strncpy(listener->devices[idx].name, name, sizeof(listener->devices[idx].name) - 1);
    listener->devices[idx].active = true;
    listener->device_count++;

    printf("Added keyboard: %s (%s)\n", name, device_path);
    return 0;
}

/* Auto-detect and add all keyboard devices */
int event_listener_auto_detect(event_listener_t *listener) {
    if (!listener) {
        fprintf(stderr, "event_listener_auto_detect: NULL listener\n");
        return -1;
    }

    DIR *dir = opendir(INPUT_DIR);
    if (!dir) {
        perror("event_listener_auto_detect: Failed to open /dev/input");
        return -1;
    }

    int count = 0;
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        /* Look for event devices */
        if (strncmp(entry->d_name, "event", 5) != 0) {
            continue;
        }

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", INPUT_DIR, entry->d_name);

        /* Try to add device */
        if (event_listener_add_device(listener, path) == 0) {
            count++;
        }
    }

    closedir(dir);
    
    if (count == 0) {
        fprintf(stderr, "No keyboard devices found\n");
        return -1;
    }

    printf("Auto-detected %d keyboard device(s)\n", count);
    return count;
}

/* Start listening for events */
int event_listener_run(event_listener_t *listener) {
    if (!listener) {
        fprintf(stderr, "event_listener_run: NULL listener\n");
        return -1;
    }

    if (listener->device_count == 0) {
        fprintf(stderr, "event_listener_run: No devices to monitor\n");
        return -1;
    }

    listener->running = true;
    printf("Event listener started, monitoring %d device(s)\n", listener->device_count);

    struct epoll_event events[MAX_EVENTS];

    while (listener->running) {
        int nfds = epoll_wait(listener->epoll_fd, events, MAX_EVENTS, 100);
        
        if (nfds < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("event_listener_run: epoll_wait failed");
            return -1;
        }

        /* Process events */
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            struct input_event ev;

            while (read(fd, &ev, sizeof(ev)) == sizeof(ev)) {
                /* Only process key events */
                if (ev.type == EV_KEY) {
                    /* Forward to virtual keyboard */
                    vkbd_process_key(listener->vkbd_ctx, ev.code, ev.value);
                }
                /* Note: EV_SYN is sent by vkbd_process_key, no need to forward */
            }
        }
    }

    printf("Event listener stopped\n");
    return 0;
}

/* Stop listening for events */
void event_listener_stop(event_listener_t *listener) {
    if (listener) {
        listener->running = false;
    }
}

/* Destroy event listener */
void event_listener_destroy(event_listener_t *listener) {
    if (!listener) {
        return;
    }

    listener->running = false;

    /* Close all device file descriptors */
    for (int i = 0; i < listener->device_count; i++) {
        if (listener->devices[i].active && listener->devices[i].fd >= 0) {
            /* Release grab */
            ioctl(listener->devices[i].fd, EVIOCGRAB, 0);
            close(listener->devices[i].fd);
            listener->devices[i].fd = -1;
            listener->devices[i].active = false;
        }
    }

    /* Close epoll fd */
    if (listener->epoll_fd >= 0) {
        close(listener->epoll_fd);
        listener->epoll_fd = -1;
    }

    listener->device_count = 0;
    printf("Event listener destroyed\n");
}
