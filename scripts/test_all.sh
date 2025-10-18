#!/bin/bash
# A script to build, load, test, and unload the msgbuf module.

# Exit immediately if any command fails
set -e

echo "===== Building User-Space Tests ====="
# Go into the userspace directory and build the tests
(cd ../userspace && make clean && make)
echo " "

echo "===== Loading Kernel Module ====="
# Run the load script (which also builds the module)
# We must be in the scripts/ directory for this to work
sudo ./load_module.sh
echo " "

echo "===== Running Test 1: Basic Operations ====="
../userspace/test_msgbuf
echo " "

echo "===== Running Test 2: Concurrent Processes ====="
../userspace/test_concurrent
echo " "

echo "===== Running Test 3: Stress Threads ====="
../userspace/test_stress
echo " "

echo "===== All Tests Complete. Final Statistics: ====="
cat /proc/msgbuf_stats
echo " "

echo "===== Unloading Kernel Module ====="
sudo ./unload_module.sh
echo " "

echo "===== Test Run Finished Successfully. ====="