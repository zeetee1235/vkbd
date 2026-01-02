#!/bin/sh

# Quick test script for virtual keyboard

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

printf "${GREEN}=== Virtual Keyboard Test ===${NC}\n\n"

# Check if running as root
if [ "$(id -u)" -ne 0 ]; then
    printf "${YELLOW}[WARN] Not running as root. Some features may not work.${NC}\n"
    echo "Consider running: sudo sh test.sh"
    echo ""
fi

# Check uinput
printf "${GREEN}[1/5] Checking uinput...${NC}\n"
if [ ! -e /dev/uinput ]; then
    printf "${YELLOW}Loading uinput module...${NC}\n"
    sudo modprobe uinput
    sleep 1
fi

if [ -e /dev/uinput ]; then
    printf "${GREEN}✓ /dev/uinput available${NC}\n"
else
    printf "${RED}✗ Failed to load uinput${NC}\n"
    exit 1
fi

# Build
echo ""
printf "${GREEN}[2/5] Building...${NC}\n"
if make clean > /dev/null 2>&1 && make > /dev/null 2>&1; then
    printf "${GREEN}✓ Build successful${NC}\n"
else
    printf "${RED}✗ Build failed${NC}\n"
    exit 1
fi

# Check binary
echo ""
printf "${GREEN}[3/5] Checking binary...${NC}\n"
if [ -x ./vkbd ]; then
    size=$(du -h ./vkbd | cut -f1)
    printf "${GREEN}✓ vkbd binary ready ($size)${NC}\n"
else
    printf "${RED}✗ vkbd binary not found${NC}\n"
    exit 1
fi

# List devices
echo ""
printf "${GREEN}[4/5] Input devices...${NC}\n"
count=$(ls /dev/input/event* 2>/dev/null | wc -l)
if [ "$count" -gt 0 ]; then
    printf "${GREEN}✓ Found $count device(s)${NC}\n"
else
    printf "${YELLOW}[WARN] No input devices found${NC}\n"
fi

# Ready
echo ""
printf "${GREEN}[5/5] Ready!${NC}\n\n"
echo "Run: ${YELLOW}sudo ./vkbd${NC}"
echo ""

# Ask to run
printf "Run now? [y/N] "
read -r response
if [ "$response" = "y" ] || [ "$response" = "Y" ]; then
    echo ""
    printf "${GREEN}=== Starting Virtual Keyboard ===${NC}\n"
    printf "${YELLOW}Auto-timeout in 10 seconds${NC}\n"
    echo "Press Ctrl+C to exit early"
    echo ""
    sleep 1
    
    # Run with 10 second timeout
    if [ "$(id -u)" -eq 0 ]; then
        timeout 10 ./vkbd &
        PID=$!
    else
        sudo timeout 10 ./vkbd &
        PID=$!
    fi
    
    # Wait for timeout or manual interrupt
    wait $PID 2>/dev/null
    EXIT_CODE=$?
    
    echo ""
    if [ $EXIT_CODE -eq 124 ]; then
        printf "${GREEN}✓ Test completed (timeout)${NC}\n"
    elif [ $EXIT_CODE -eq 0 ]; then
        printf "${GREEN}✓ Test completed successfully${NC}\n"
    else
        printf "${RED}✗ Test failed (exit code: $EXIT_CODE)${NC}\n"
    fi
else
    echo "Run manually: sudo ./vkbd"
fi
