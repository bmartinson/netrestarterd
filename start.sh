#!/bin/bash

# Exit if not running as sudo
if [[ $EUID -ne 0 ]]; then
  echo "This script must be run as root. Please run with sudo."
  exit 1
fi

echo "Starting daemon..."

# Kill any existing instances of the daemon
sudo pkill -9 netrestarterd

# Load the daemon
if [ ! -f /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist ]; then
  echo "  Plist file not found at /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist"
  exit 2
fi

# Make sure permissions are for sure correct
sudo chmod 644 /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist
sudo chown root:wheel /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist

# Unload the daemon and load it in case it is already loaded.
launchctl unload /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist 2>/dev/null
launchctl load /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist

if [ $? -eq 0 ]; then
  echo "  Daemon started successfully."
else
  echo "  Failed to start daemon. Please check the logs for errors."
fi
