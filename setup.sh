#!/bin/sh

# Virtual Keyboard Setup Script
# Helps set up permissions and configure the system

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_info() {
    printf "${GREEN}[INFO]${NC} %s\n" "$*"
}

print_warn() {
    printf "${YELLOW}[WARN]${NC} %s\n" "$*"
}

print_error() {
    printf "${RED}[ERROR]${NC} %s\n" "$*"
}

check_uinput() {
    print_info "Checking uinput module..."
    
    if [ -e /dev/uinput ]; then
        print_info "/dev/uinput exists"
        ls -l /dev/uinput
        return 0
    else
        print_warn "/dev/uinput not found, loading module..."
        sudo modprobe uinput
        
        if [ -e /dev/uinput ]; then
            print_info "uinput module loaded successfully"
            return 0
        else
            print_error "Failed to load uinput module"
            return 1
        fi
    fi
}

check_permissions() {
    print_info "Checking permissions..."
    
    if groups | grep -q input; then
        print_info "User is in 'input' group"
    else
        print_warn "User is NOT in 'input' group"
        echo "To add yourself to the input group, run:"
        echo "  sudo usermod -a -G input \$USER"
        echo "Then log out and log back in"
    fi
    
    if [ "$(id -u)" -eq 0 ]; then
        print_info "Running as root"
    else
        print_warn "Not running as root. Some features may not work."
    fi
}

setup_udev_rules() {
    print_info "Setting up udev rules..."
    
    rules_file="/etc/udev/rules.d/99-uinput.rules"
    rules_content='KERNEL=="uinput", MODE="0660", GROUP="input", OPTIONS+="static_node=uinput"'
    
    if [ -e "$rules_file" ]; then
        print_info "udev rules already exist"
    else
        print_info "Creating udev rules..."
        echo "$rules_content" | sudo tee "$rules_file" > /dev/null
        sudo udevadm control --reload-rules
        sudo udevadm trigger
        print_info "udev rules created. You may need to log out and back in."
    fi
}

setup_autoload() {
    print_info "Setting up uinput autoload..."
    
    modules_file="/etc/modules-load.d/uinput.conf"
    
    if [ -e "$modules_file" ]; then
        print_info "uinput autoload already configured"
    else
        echo 'uinput' | sudo tee "$modules_file" > /dev/null
        print_info "uinput will be loaded automatically on boot"
    fi
}

list_input_devices() {
    print_info "Listing input devices..."
    
    for device in /dev/input/event*; do
        if [ -e "$device" ]; then
            echo ""
            echo "Device: $device"
            if command -v evtest > /dev/null 2>&1; then
                sudo evtest --info "$device" 2>/dev/null | head -n 5
            else
                ls -l "$device"
            fi
        fi
    done
}

build_project() {
    print_info "Building virtual keyboard..."
    
    if make; then
        print_info "Build successful!"
        return 0
    else
        print_error "Build failed"
        return 1
    fi
}

show_help() {
    cat << EOF
Virtual Keyboard Setup Script

Usage: ./setup.sh [command]

Commands:
  check        - Check system requirements and permissions
  setup        - Set up udev rules and autoload (requires sudo)
  build        - Build the project
  list         - List available input devices
  full         - Do everything (check, setup, build)
  help         - Show this help message

EOF
}

# Main script
case "$1" in
    check)
        check_uinput
        check_permissions
        ;;
    setup)
        if [ "$(id -u)" -ne 0 ]; then
            print_error "This command requires sudo privileges"
            exit 1
        fi
        setup_udev_rules
        setup_autoload
        check_uinput
        ;;
    build)
        build_project
        ;;
    list)
        list_input_devices
        ;;
    full)
        check_uinput
        check_permissions
        echo ""
        printf "Do you want to set up udev rules? (requires sudo) [y/N] "
        read -r response
        if [ "$response" = "y" ] || [ "$response" = "Y" ]; then
            setup_udev_rules
            setup_autoload
        fi
        echo ""
        build_project
        ;;
    help|"")
        show_help
        ;;
    *)
        print_error "Unknown command: $1"
        show_help
        exit 1
        ;;
esac
