#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h> // For wait()

#define DEVICE "/dev/msgbuf"
#define NUM_PROCESSES 10
#define NUM_WRITES 100

int main() {
    int i, j;
    pid_t pid;

    printf("Starting concurrent write test with %d processes...\n", NUM_PROCESSES);

    for (i = 0; i < NUM_PROCESSES; i++) {
        pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            // --- This is the Child Process ---
            int fd;
            char buf[128];

            fd = open(DEVICE, O_WRONLY);
            if (fd < 0) {
                perror("Child: open");
                exit(1);
            }

            for (j = 0; j < NUM_WRITES; j++) {
                snprintf(buf, sizeof(buf), "Child PID %d, iteration %d\n", getpid(), j);

                if (write(fd, buf, strlen(buf)) < 0) {
                    perror("Child: write");
                    // Don't exit, just report and continue
                }

                // Sleep for 1 millisecond to allow context switching
                usleep(1000);
            }

            close(fd);
            exit(0); // Child process exits
            // --- End of Child Process ---
        }
    }

    // --- This is the Parent Process ---
    // Wait for all child processes to finish
    printf("Parent waiting for all %d children...\n", NUM_PROCESSES);
    for (i = 0; i < NUM_PROCESSES; i++) {
        wait(NULL);
    }

    printf("Concurrent test finished.\n");
    printf("Check /proc/msgbuf_stats and dmesg for errors.\n");

    return 0;
}