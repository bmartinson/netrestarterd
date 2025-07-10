# netrestarterd
A daemon for MacOS that detects network interruptions for VPN DNS changes and resets system route tables and network interfaces to automatically help keep you accessing the world wide web.

## Why Does This Exist?
When using a work VPN, whenever an employee would put their computer to sleep while keeping their VPN connection active, or if they disconnected from the VPN, their route tables and network interfaces would not refresh correctly leading to a seemingly dead internet connection. The only way to resolve this would be to refresh such system settings manually via the terminal, to turn the active interface off and on (Wi-Fi toggle off/on), or reboot the computer. So, this daemon was developed to always run and keep an eye out for DNS changes due to an internet disruption when coming off of the VPN and to automate the refresh of the network interfaces and route tables.

## Install
All you need to install, uninstall, manually start/stop, or debug the tool can be driven off of the provided bash scripts. The binary will be compiled and installed using the `install.sh` script as will the appropriate plist files to setup the launch daemon.

```sh
# compile the source code, install the binary, configure the daemon, and execute it
sudo ./install.sh

# get rid of everything
sudo ./uninstall.sh

# uninstall any existing installs and re-compile and run interactively
sudo ./debug.sh
```

Hope this helps! :)