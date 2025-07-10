#!/bin/bash

STARTING_DIR=$(pwd)
SCRIPT_DIR=$(dirname "$0")

# Exit if not running as sudo
if [[ $EUID -ne 0 ]]; then
  echo "This script must be run as root. Please run with sudo."
  exit 1
fi

cd "$SCRIPT_DIR"

if [[ -x ./uninstall.sh ]]; then
  ./uninstall.sh
else
  echo "uninstall.sh not found or not executable in $SCRIPT_DIR"
  exit 1
fi

if [[ -x ./install.sh ]]; then
  ./install.sh
else
  echo "install.sh not found or not executable in $SCRIPT_DIR"
  exit 1
fi

if [ -f /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist ]; then
  rm /Library/LaunchDaemons/com.bmartinson.netrestarterd.plist
fi

if [[ -x ./stop.sh ]]; then
  ./stop.sh
else
  echo "stop.sh not found or not executable in $SCRIPT_DIR"
  exit 1
fi

cd "$STARTING_DIR"

echo""
echo "Starting interactively..."

# Load the application in debug mode
/usr/local/bin/netrestarterd --debug
