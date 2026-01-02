# vkbd

Keyboard interception library. Linux + uinput. Latency ~0.2ms.

Wayland blocks `/dev/input` access. This reads `/dev/input/event*` directly, forwards via uinput.

## Quick Start

```bash
# Install
sudo modprobe uinput
make

# Run
sudo ./vkbd

# Test (10s auto-stop)
sudo sh test.sh
```

**Requires:** Linux, uinput, GCC, root

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

| Function | Description |
|----------|-------------|
| `vkbd_init(ctx, name)` | Initialize. Returns 0/-1 |
| `vkbd_destroy(ctx)` | Cleanup |
| `vkbd_register_callback(ctx, cb, data)` | Add handler (max 16). Returns ID/-1 |
| `vkbd_unregister_callback(ctx, id)` | Remove handler. Returns 0/-1 |
| `vkbd_process_key(ctx, code, val)` | Process key. val: 0=release, 1=press, 2=repeat |
| `vkbd_send_key(ctx, code, val)` | Send key directly |
| `vkbd_sync(ctx)` | Send EV_SYN |

### event_listener.h

| Function | Description |
|----------|-------------|
| `event_listener_init(listener, vkbd)` | Initialize |
| `event_listener_auto_detect(listener)` | Find keyboards. Returns count/-1 |
| `event_listener_add_device(listener, path)` | Add device manually |
| `event_listener_run(listener)` | Start (blocking) |
| `event_listener_stop(listener)` | Stop |
| `event_listener_destroy(listener)` | Cleanup |

## Examples

```bash
make examples
cd examples
sudo ./simple_logger    # Log keys
sudo ./key_remapper     # Remap Caps→Esc
```

Source: `examples/simple_logger.c`, `examples/key_remapper.c`

## Setup

Load uinput on boot:
```bash
echo uinput | sudo tee /etc/modules-load.d/uinput.conf
```

## License

MIT
