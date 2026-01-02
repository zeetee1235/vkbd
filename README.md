# vkbd

Keyboard interception library for Linux using uinput.

## Why

Wayland's security model blocks direct evdev access. This library works around it by reading `/dev/input/event*` directly and forwarding through a virtual keyboard.

## Requirements

- Linux + uinput module
- GCC
- Root privileges

## Build

```bash
make
sudo ./vkbd
```

## Usage

```c
#include "vkbd.h"
#include "event_listener.h"

vkbd_context_t vkbd;
vkbd_init(&vkbd, "Virtual Keyboard");
vkbd_register_callback(&vkbd, my_callback, NULL);

event_listener_t listener;
event_listener_init(&listener, &vkbd);
event_listener_auto_detect(&listener);
event_listener_run(&listener);
```

## API

### vkbd.h

```c
int vkbd_init(vkbd_context_t *ctx, const char *device_name);
```
Initialize virtual keyboard device. Returns 0 on success, -1 on error.

```c
void vkbd_destroy(vkbd_context_t *ctx);
```
Cleanup and destroy virtual keyboard device.

```c
int vkbd_send_key(vkbd_context_t *ctx, uint16_t key_code, int32_t value);
```
Send key event. `value`: 0=release, 1=press, 2=repeat. Returns 0 on success.

```c
int vkbd_sync(vkbd_context_t *ctx);
```
Send synchronization event (EV_SYN).

```c
int vkbd_register_callback(vkbd_context_t *ctx, vkbd_callback_t callback, void *user_data);
```
Register event handler. Max 16 callbacks. Returns handler ID on success.

```c
int vkbd_unregister_callback(vkbd_context_t *ctx, int handler_id);
```
Unregister callback by handler ID.

```c
int vkbd_process_key(vkbd_context_t *ctx, uint16_t key_code, int32_t value);
```
Process key through all callbacks then forward to virtual device.

### event_listener.h

```c
int event_listener_init(event_listener_t *listener, vkbd_context_t *vkbd_ctx);
```
Initialize event listener. Links listener to virtual keyboard context.

```c
int event_listener_auto_detect(event_listener_t *listener);
```
Auto-detect and add all keyboard devices from `/dev/input/event*`.

```c
int event_listener_add_device(event_listener_t *listener, const char *device_path);
```
Add specific device to monitor (e.g., `/dev/input/event0`).

```c
int event_listener_run(event_listener_t *listener);
```
Start listening for events. Blocking call using epoll.

```c
void event_listener_stop(event_listener_t *listener);
```
Stop the event listener.

```c
void event_listener_destroy(event_listener_t *listener);
```
Cleanup and free resources.

## Setup

```bash
# Load uinput module
sudo modprobe uinput

# Optional: auto-load on boot
echo uinput | sudo tee /etc/modules-load.d/uinput.conf

# Run setup script
sudo sh setup.sh
```

## Examples

See `examples/` directory:
- `simple_logger.c` - Basic key logging
- `key_remapper.c` - Key remapping (Caps Lock → Escape)

## License

MIT
