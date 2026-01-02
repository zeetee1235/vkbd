# vkbd

Keyboard interception library for Linux using uinput.

## Requirements

- Linux with uinput
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

**vkbd.h**
- `vkbd_init()` - Initialize
- `vkbd_send_key()` - Send key
- `vkbd_register_callback()` - Add handler
- `vkbd_destroy()` - Cleanup

**event_listener.h**
- `event_listener_init()` - Initialize
- `event_listener_auto_detect()` - Find keyboards
- `event_listener_run()` - Start
- `event_listener_stop()` - Stop

## Setup

```bash
sudo modprobe uinput
sudo sh setup.sh
```
