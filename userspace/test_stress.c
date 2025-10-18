#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h> // For threads
#include <sys/ioctl.h>
#include "msgbuf.h"

#define DEVICE "/dev/msgbuf"
#define NUM_THREADS 50
#define NUM_OPS_PER_THREAD 1000

/**
 * @brief The function each thread will execute.
 */
void* thread_func(void* arg) {
    int fd;
    char write_buf[64];
    char read_buf[64];
    int thread_id = *(int*)arg;

    // 1. Open the device
    // Each thread gets its own file descriptor
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Thread: open");
        return NULL;
    }

    // 2. Perform a mix of operations repeatedly
    for (int i = 0; i < NUM_OPS_PER_THREAD; i++) {
        // --- Write Operation ---
        snprintf(write_buf, sizeof(write_buf), "Thread %d, op %d", thread_id, i);
        if (write(fd, write_buf, strlen(write_buf)) < 0) {
            perror("Thread: write");
        }

        // --- Lseek Operation ---
        if (lseek(fd, 0, SEEK_SET) < 0) {
            perror("Thread: lseek");
        }

        // --- Read Operation ---
        if (read(fd, read_buf, sizeof(read_buf) - 1) < 0) {
            perror("Thread: read");
        }

        // --- (Optional) IOCTL Operation ---
        // Let's clear the buffer 1% of the time to cause chaos
        if (i % 100 == 0) {
            if (ioctl(fd, MSGBUF_IOCTL_CLEAR) < 0) {
                perror("Thread: ioctl clear");
            }
        }
    }

    // 3. Close the device
    close(fd);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    printf("Starting stress test with %d threads (%d ops each)...\n",
           NUM_THREADS, NUM_OPS_PER_THREAD);
    printf("Total operations: %d\n", NUM_THREADS * NUM_OPS_PER_THREAD * 2); // (read+write)

    // --- Launch all threads ---
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }

    // --- Wait for all threads to finish ---
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Stress test finished.\n");
    printf("Check /proc/msgbuf_stats and dmesg for errors.\n");

    return 0;
}