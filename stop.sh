#!/bin/bash

# Exit if not running as sudo
if [[ $EUID -ne 0 ]]; then
  echo "This script must be run as root. Please run with sudo."
  exit 1
fi

echo "Stopping netrestarterd..."

sudo pkill -9 netrestarterd

echo "  Stopped."
