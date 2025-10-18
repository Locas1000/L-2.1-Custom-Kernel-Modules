
# Homework 5: Custom Kernel Module (msgbuf)
Demo Video: https://drive.google.com/file/d/1QiDtLHc92UTiry-FdxHoGhqQU90DicNp/view?usp=sharing.
This project is a Linux Loadable Kernel Module (LKM) that implements a simple, thread-safe character device driver. The device, `/dev/msgbuf`, acts as a simple message buffer that can be written to and read from by user-space programs.

## Features

* **Character Device:** Creates a device node at `/dev/msgbuf` upon loading.
* **Buffer Operations:** Supports `write()` (overwrites buffer), `read()` (reads from start), `open()`, and `release()`.
* **Seeking:** Supports `lseek()` to reset the read/write offset.
* **Procfs Interface:** Creates a read-only file at `/proc/msgbuf_stats` to report device statistics (opens, reads, writes, bytes).
* **IOCTL Commands:**
    * `MSGBUF_IOCTL_GETSIZE`: Returns the current length of the message in the buffer.
    * `MSGBUF_IOCTL_CLEAR`: Clears the buffer and resets its length to 0.
* **Synchronization:** Fully thread-safe and safe for concurrent processes.
    * Uses a **mutex** to protect the main data buffer (for `read`, `write`, `ioctl`).
    * Uses a **spinlock** to protect the statistics counters for fast, atomic updates.

## Project Structure

```

homework5/
├── docs/
│   ├── DESIGN.md
│   ├── KERNEL\_LOGS.txt
│   └── TESTING.md
├── module/
│   ├── msgbuf.c
│   └── Makefile
├── scripts/
│   ├── load\_module.sh
│   └── unload\_module.sh
├── userspace/
│   ├── msgbuf.h
│   ├── test\_msgbuf.c
│   ├── test\_concurrent.c
│   ├── test\_stress.c
│   └── Makefile
└── README.md

````

## How to Build

1.  **Build the Kernel Module:**
    ```bash
    cd module/
    make
    ```

2.  **Build the User-Space Test Programs:**
    ```bash
    cd ../userspace/
    make
    ```

## How to Run & Test

Helper scripts are provided in the `scripts/` directory to manage the module.

1.  **Load the Module:**
    * This script will build, insert the module, and set permissions for `/dev/msgbuf`.
    ```bash
    cd scripts/
    sudo ./load_module.sh
    ```

2.  **Run Tests:**
    * From the `userspace/` directory, run the test executables.
    ```bash
    cd ../userspace/

    # Test 1: Basic Operations (write, read, lseek, ioctl)
    ./test_msgbuf

    # Test 2: Concurrency Test (multi-process)
    ./test_concurrent

    # Test 3: Stress Test (multi-thread)
    ./test_stress
    ```

3.  **Check Statistics:**
    * You can check the device statistics at any time:
    ```bash
    cat /proc/msgbuf_stats
    ```
    * You can also watch the kernel log for messages in real-time:
    ```bash
    sudo dmesg -w
    ```

4.  **Unload the Module:**
    * This script will remove the device node and unload the module.
    ```bash
    cd ../scripts/
    sudo ./unload_module.sh
    ```

## Design Decisions

A brief overview of the design is in `docs/DESIGN.md`. This file explains the choice of `mutex` vs. `spinlock` for synchronization, the use of `seq_file` for the `/proc` interface, and the implementation of `lseek`.
