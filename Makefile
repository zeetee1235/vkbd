# Virtual Keyboard Makefile
# Optimized for fast compilation and execution

CC = gcc
CFLAGS = -Wall -Wextra -O3 -march=native -flto -ffast-math
LDFLAGS = -flto
LIBS = 

# Enable additional warnings for better code quality
EXTRA_WARNINGS = -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes

# Source files
SOURCES = main.c vkbd.c event_listener.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = vkbd

# Library files for creating static/shared libraries
LIB_SOURCES = vkbd.c event_listener.c
LIB_OBJECTS = $(LIB_SOURCES:.c=.o)
STATIC_LIB = libvkbd.a
SHARED_LIB = libvkbd.so

# Debug build
DEBUG_CFLAGS = -Wall -Wextra -O0 -g -DDEBUG -fsanitize=address -fsanitize=undefined
DEBUG_LDFLAGS = -fsanitize=address -fsanitize=undefined
DEBUG_TARGET = vkbd_debug

.PHONY: all clean debug install library test examples help

# Default target
all: $(TARGET)

# Main executable
$(TARGET): $(OBJECTS)
	@echo "Linking $@..."
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo "Build complete: $@"

# Compile source files
%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Debug build
debug: CFLAGS = $(DEBUG_CFLAGS)
debug: LDFLAGS = $(DEBUG_LDFLAGS)
debug: clean $(DEBUG_TARGET)

$(DEBUG_TARGET): $(OBJECTS)
	@echo "Linking debug build..."
	$(CC) $(DEBUG_LDFLAGS) -o $@ $^ $(LIBS)
	@echo "Debug build complete: $@"

# Static library
$(STATIC_LIB): $(LIB_OBJECTS)
	@echo "Creating static library..."
	ar rcs $@ $^
	@echo "Static library created: $@"

# Shared library
$(SHARED_LIB): CFLAGS += -fPIC
$(SHARED_LIB): $(LIB_SOURCES)
	@echo "Creating shared library..."
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LIBS)
	@echo "Shared library created: $@"

# Build libraries
library: $(STATIC_LIB) $(SHARED_LIB)
	@echo "Libraries built successfully"

# Build examples
examples: $(STATIC_LIB)
	@echo "Building examples..."
	@cd examples && \
	$(CC) $(CFLAGS) simple_logger.c -I.. -L.. -lvkbd -o simple_logger && \
	$(CC) $(CFLAGS) key_remapper.c -I.. -L.. -lvkbd -o key_remapper
	@echo "Examples built successfully"

# Run tests
test: $(TARGET)
	@echo "Running tests..."
	@sh test.sh

# Install (requires root)
install: $(TARGET) $(STATIC_LIB) $(SHARED_LIB)
	@echo "Installing..."
	install -m 755 $(TARGET) /usr/local/bin/
	install -m 644 vkbd.h event_listener.h /usr/local/include/
	install -m 644 $(STATIC_LIB) /usr/local/lib/
	install -m 755 $(SHARED_LIB) /usr/local/lib/
	ldconfig
	@echo "Installation complete"

# Uninstall
uninstall:
	@echo "Uninstalling..."
	rm -f /usr/local/bin/$(TARGET)
	rm -f /usr/local/include/vkbd.h /usr/local/include/event_listener.h
	rm -f /usr/local/lib/$(STATIC_LIB) /usr/local/lib/$(SHARED_LIB)
	ldconfig
	@echo "Uninstall complete"

# Clean build files
clean:
	@echo "Cleaning..."
	rm -f $(OBJECTS) $(TARGET) $(DEBUG_TARGET)
	rm -f $(STATIC_LIB) $(SHARED_LIB)
	rm -f *.o
	@cd examples 2>/dev/null && rm -f simple_logger key_remapper || true
	@echo "Clean complete"

# Dependencies
main.o: main.c vkbd.h event_listener.h
vkbd.o: vkbd.c vkbd.h
event_listener.o: event_listener.c event_listener.h vkbd.h

# Help
help:
	@echo "Virtual Keyboard Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build optimized executable (default)"
	@echo "  debug      - Build debug version with sanitizers"
	@echo "  library    - Build static and shared libraries"
	@echo "  examples   - Build example programs"
	@echo "  test       - Run test suite"
	@echo "  install    - Install to system (requires root)"
	@echo "  uninstall  - Remove from system (requires root)"
	@echo "  clean      - Remove all build files"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make              # Build optimized"
	@echo "  make debug        # Build with debug symbols"
	@echo "  make examples     # Build example programs"
	@echo "  make test         # Run tests"
	@echo "  sudo make install # Install system-wide"
