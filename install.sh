#!/bin/bash

STARTING_DIR=$(pwd)
SCRIPT_DIR=$(dirname "$0")

# Exit if not running as sudo
if [[ $EUID -ne 0 ]]; then
  echo "This script must be run as root. Please run with sudo."
  exit 1
fi

cd "$SCRIPT_DIR"

echo "Compiling and installing netrestarterd for execution..."

# Remove existing binary if it exists
if [ -f /usr/local/bin/netrestarterd ]; then
  rm /usr/local/bin/netrestarterd
  echo "  /usr/local/bin/netrestarterd removed."
fi

# Remove existing plist if it exists
if [ -f /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist ]; then
  rm /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist
  echo "  /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist removed."
fi

# Compile the binary
clang -o /usr/local/bin/netrestarterd ./src/netrestarterd.c \
  -framework SystemConfiguration \
  -framework CoreFoundation

# Change owner to root:wheel
chown root:wheel /usr/local/bin/netrestarterd

echo "  /usr/local/bin/netrestarterd installed."

# Copy plist and change owner
cp ./src/com.bmartinson.netrestarterd.plist /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist
chown root:wheel /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist

cd "$STARTING_DIR"

echo "  netrestarterd is configured for startup."

echo ""
echo "Installation complete!"
