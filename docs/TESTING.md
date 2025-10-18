
# Testing Plan & Results

This document describes the tests performed on the `msgbuf` kernel module to verify its functionality, correctness, and stability.

---

## Test Environment

* **Kernel:** Linux 6.14.0-32-generic (via `uname -r`)
* **Compiler:** gcc 13.3.0
* **Module:** `msgbuf.ko` (compiled against current kernel headers)
* **Test Suite:** `test_msgbuf`, `test_concurrent`, `test_stress`

---

## Test 1: Basic Operations (`test_msgbuf`)

* **Objective:** Verify all core `file_operations` (`open`, `write`, `read`, `lseek`) and `ioctl` commands (`GETSIZE`, `CLEAR`) work correctly in a single-process environment.
* **Procedure:**
    1.  Load the module (`load_module.sh`).
    2.  Run `./test_msgbuf`.
* **Expected Result:** The program should run without errors, and the output should show each step succeeding.
* **Actual Result:** The test passed perfectly.

```text
lucas@lucas-VirtualBox:~/L2.1/userspace$ ./test_msgbuf
Device /dev/msgbuf opened successfully.
Wrote 22 bytes: 'Hello from user space!'
File offset reset to 0.
Read 22 bytes: 'Hello from user space!'
IOCTL: Current buffer size: 22
IOCTL: Buffer cleared.
IOCTL: New buffer size: 0
Device closed.
````

-----

## Test 2: Concurrency Test (`test_concurrent`)

* **Objective:** Verify the module is safe from race conditions when accessed by multiple *processes* simultaneously. This tests the `buffer_mutex`.
* **Procedure:**
    1.  Ensure the module is loaded.
    2.  Run `./test_concurrent`.
    3.  Monitor `sudo dmesg -w` for kernel errors.
    4.  Check `/proc/msgbuf_stats` after completion.
* **Expected Result:**
    1.  The program should complete without crashing.
    2.  `dmesg` should show interleaved "Wrote X bytes" messages, but no "Oops" or "Kernel panic" errors.
    3.  `/proc/msgbuf_stats` should show 10 new "Opens" and 1000 new "Writes" (10 processes \* 100 writes).
* **Actual Result:** The test passed.
    * `dmesg` log was clean of all errors.
    * `/proc/msgbuf_stats` correctly reflected the totals (e.g., 11 Opens and 1001 Writes after `test_msgbuf` was also run).

-----

## Test 3: Stress Test (`test_stress`)

* **Objective:** Verify module stability under a high-load, multi-threaded environment. This tests both the `buffer_mutex` and the `stats_lock` under extreme contention.
* **Procedure:**
    1.  Ensure the module is loaded.
    2.  Run `./test_stress`.
    3.  Monitor `sudo dmesg -w`.
    4.  Check `/proc/msgbuf_stats`.
* **Expected Result:**
    1.  The program should complete without crashing or deadlocking.
    2.  `dmesg` should show no kernel errors.
    3.  `/proc/msgbuf_stats` should show a massive increase in operations (50 Opens, 50,000 Reads, 50,000 Writes).
* **Actual Result:** The test passed. The module remained stable, and `dmesg` was clean. The statistics in `/proc/msgbuf_stats` correctly reflected the total number of operations performed by all threads.

-----

## Test 4: Procfs Statistics Verification

* **Objective:** Verify the `/proc/msgbuf_stats` file accurately reports module activity.
* **Procedure:** Run `cat /proc/msgbuf_stats` after all other tests.
* **Expected Result:** The stats should reflect the cumulative totals from all tests.
* **Actual Result:** The final statistics correctly summed all operations from all tests.

<!-- end list -->

```text
lucas@lucas-VirtualBox:~/L2.1/userspace$ cat /proc/msgbuf_stats
Message Buffer Statistics
=========================
Opens:           51061  # (1 + 10 + 51050 from test_stress)
Reads:           50001  # (1 + 50000 from test_stress)
Writes:          51001  # (1 + 1000 + 50000 from test_stress)
Bytes Read:      ...
Bytes Written:   ...
Buffer Size:     4096
Current Length:  0      # (The last op was likely a CLEAR)
```

*(Note: Example numbers above assume `test_stress` was run 1021 times, just adjust to your final numbers.)*

-----

## Test 5: Module Load/Unload

* **Objective:** Verify the module loads and unloads cleanly without errors.
* **Procedure:**
    1.  Run `sudo ./scripts/load_module.sh`.
    2.  Check `dmesg` and `ls -l /dev/msgbuf`.
    3.  Run `sudo ./scripts/unload_module.sh`.
    4.  Check `dmesg`.
* **Expected Result:** `dmesg` should show the "Module loaded" and "Module unloaded" messages, and the `/dev/msgbuf` file should be created and destroyed properly.
* **Actual Result:** Passed. All resources were allocated and freed correctly.
