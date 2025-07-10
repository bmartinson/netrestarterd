#!/bin/bash

# Exit if not running as sudo
if [[ $EUID -ne 0 ]]; then
  echo "This script must be run as root. Please run with sudo."
  exit 1
fi

echo "Starting daemon..."

# Load the daemon
launchctl load /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist

if [ $? -eq 0 ]; then
  echo "  Daemon started successfully."
else
  echo "  Failed to start daemon. Please check the logs for errors."
fi
