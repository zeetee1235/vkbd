# Examples

This directory contains example programs demonstrating various use cases of the vkbd library.

## Building Examples

```bash
# From the examples directory
cd examples

# Build simple logger
gcc simple_logger.c ../vkbd.c ../event_listener.c -o simple_logger

# Build key remapper
gcc key_remapper.c ../vkbd.c ../event_listener.c -o key_remapper

# Or build with optimizations
gcc -O3 -march=native simple_logger.c ../vkbd.c ../event_listener.c -o simple_logger
```

## Running Examples

All examples require root privileges:

```bash
sudo ./simple_logger
sudo ./key_remapper
```

## Available Examples

### 1. simple_logger.c
Basic key logging example. Prints every key press to stdout.

**Use case**: Debug keyboard input, understand key codes

### 2. key_remapper.c
Advanced example showing how to remap keys:
- Caps Lock → Escape
- Right Alt → Right Ctrl

**Use case**: Ergonomic keyboard layouts, custom key mappings

## Creating Your Own

Use these examples as templates for your own projects:

1. Include the headers: `vkbd.h` and `event_listener.h`
2. Initialize vkbd context
3. Register your callback functions
4. Initialize and run event listener
5. Clean up resources

See the main `main.c` in the parent directory for more complex examples including:
- Multiple callbacks
- Sound effects
- Key statistics
