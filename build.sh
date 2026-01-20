#!/bin/bash

# LoRaScope Build Script for Linux/macOS

echo "========================================"
echo "LoRaScope Build Script"
echo "========================================"
echo ""

# Check if PlatformIO is installed
if ! command -v pio &> /dev/null; then
    echo "ERROR: PlatformIO is not installed or not in PATH"
    echo "Please install PlatformIO: https://platformio.org/install"
    exit 1
fi

# Set default environment
ENV="m5cardputer"

# Parse command line arguments
if [ $# -eq 0 ]; then
    echo "Usage: ./build.sh [environment] [command]"
    echo ""
    echo "Environments:"
    echo "  m5cardputer        - M5Cardputer standard (default)"
    echo "  m5cardputer_adv    - M5Cardputer ADV"
    echo "  m5cardputer_sx1262 - M5Cardputer with SX1262 module"
    echo "  m5cardputer_rf95   - M5Cardputer with RF95 module"
    echo ""
    echo "Commands:"
    echo "  clean              - Clean build artifacts"
    echo "  build              - Build firmware (default)"
    echo "  upload             - Upload to device"
    echo "  monitor            - Open serial monitor"
    echo "  all                - Clean, build and upload"
    echo ""
    echo "Examples:"
    echo "  ./build.sh                      - Build for m5cardputer"
    echo "  ./build.sh m5cardputer_adv      - Build for m5cardputer_adv"
    echo "  ./build.sh m5cardputer build    - Build for m5cardputer"
    echo "  ./build.sh m5cardputer upload   - Upload to device"
    echo ""
    read -p "Enter environment (default: m5cardputer): " ENV_INPUT
    ENV=${ENV_INPUT:-m5cardputer}
    CMD="build"
else
    ENV=$1
    CMD=${2:-build}
fi

echo ""
echo "Environment: $ENV"
echo "Command: $CMD"
echo ""

# Execute command
case $CMD in
    clean)
        echo "Cleaning build artifacts..."
        pio run -e $ENV -t clean
        ;;
    build)
        echo "Building firmware..."
        pio run -e $ENV
        ;;
    upload)
        echo "Uploading to device..."
        pio run -e $ENV -t upload
        ;;
    monitor)
        echo "Opening serial monitor..."
        pio device monitor -p /dev/ttyUSB0
        ;;
    all)
        echo "Cleaning, building and uploading..."
        pio run -e $ENV -t clean
        pio run -e $ENV -t upload
        ;;
    *)
        echo "ERROR: Unknown command '$CMD'"
        exit 1
        ;;
esac

if [ $? -eq 0 ]; then
    echo ""
    echo "========================================"
    echo "SUCCESS: Command completed"
    echo "========================================"
else
    echo ""
    echo "========================================"
    echo "ERROR: Command failed"
    echo "========================================"
    exit 1
fi
