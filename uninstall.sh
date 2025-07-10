#!/bin/bash

STARTING_DIR=$(pwd)
SCRIPT_DIR=$(dirname "$0")

# Exit if not running as sudo
if [[ $EUID -ne 0 ]]; then
  echo "This script must be run as root. Please run with sudo."
  exit 1
fi

echo "Uninstalling netrestarterd..."

# Remove existing binary if it exists
if [ -f /usr/local/bin/netrestarterd ]; then
  rm /usr/local/bin/netrestarterd
  echo "  /usr/local/bin/netrestarterd removed."
fi

if [ -f /var/run/netrestarterd.pid ]; then
  rm /var/run/netrestarterd.pid
  echo "  /var/run/netrestarterd.pid removed."
fi

# Remove existing plist if it exists
if [ -f /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist ]; then
  rm /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist
  echo "  /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist removed."
fi

echo ""
echo "Uninstallation complete!"

if [[ -x ./stop.sh ]]; then
  ./stop.sh
else
  echo "stop.sh not found or not executable in $SCRIPT_DIR"
  exit 1
fi
