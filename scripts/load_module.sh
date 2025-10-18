#!/bin/bash
# A script to build, insert, and set permissions for the msgbuf module.

# Exit immediately if any command fails
set -e

echo "Building the kernel module..."
# Go into the module directory and build it
(cd ../module && make)

echo "Loading module msgbuf.ko..."
# Insert the module into the kernel
sudo insmod ../module/msgbuf.ko

echo "Setting permissions for /dev/msgbuf..."
# Get the major number from /proc/devices
# We use awk to find 'msgbuf' and print the first column ($1)
MAJOR=$(grep 'msgbuf' /proc/devices | awk '{print $1}')

# Check if we got the major number
if [ -z "$MAJOR" ]; then
    echo "Error: Could not find 'msgbuf' in /proc/devices."
    echo "Was the module load successful?"
    sudo rmmod msgbuf
    exit 1
fi

# Remove the device node if it already exists
sudo rm -f /dev/msgbuf

echo "Creating device node /dev/msgbuf with major number $MAJOR"
# Create the character device node
sudo mknod /dev/msgbuf c $MAJOR 0

# Set permissions to read/write for everyone
sudo chmod 666 /dev/msgbuf

echo "Module loaded and device created successfully."
ls -l /dev/msgbuf