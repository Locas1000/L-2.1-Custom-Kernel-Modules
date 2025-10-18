#!/bin/bash
# A script to remove the /dev/msgbuf device and unload the kernel module.

# Exit immediately if any command fails
set -e

echo "Removing device node /dev/msgbuf..."
# Remove the device node
sudo rm -f /dev/msgbuf

echo "Unloading module msgbuf..."
# Remove the module from the kernel
sudo rmmod msgbuf

echo "Module unloaded successfully."
# Check dmesg for the unload message
sudo dmesg | tail -n 1