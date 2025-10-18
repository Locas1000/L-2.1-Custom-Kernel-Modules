#include <stdio.h>
#include <stdlib.h> // For exit
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

// Include our shared header
#include "msgbuf.h"

#define DEVICE "/dev/msgbuf"

int main() {
    int fd;
    int size;
    char buffer[256] = {0};
    const char *msg = "Hello from user space!";

    // 1. Open the device
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }
    printf("Device %s opened successfully.\n", DEVICE);

    // 2. Test write
    ssize_t written = write(fd, msg, strlen(msg));
    if (written < 0) {
        perror("Failed to write to device");
        close(fd);
        return 1;
    }
    printf("Wrote %zd bytes: '%s'\n", written, msg);

    // 3. Test lseek (reset file offset to 0)
    // Our read function uses *offset, so this is important!
    if (lseek(fd, 0, SEEK_SET) < 0) {
        perror("Failed to lseek");
        close(fd);
        return 1;
    }
    printf("File offset reset to 0.\n");

    // 4. Test read
    ssize_t read_bytes = read(fd, buffer, sizeof(buffer) - 1);
    if (read_bytes < 0) {
        perror("Failed to read from device");
        close(fd);
        return 1;
    }
    printf("Read %zd bytes: '%s'\n", read_bytes, buffer);

    // 5. Test IOCTL GETSIZE
    size = ioctl(fd, MSGBUF_IOCTL_GETSIZE);
    if (size < 0) {
        perror("Failed to get size via ioctl");
        close(fd);
        return 1;
    }
    printf("IOCTL: Current buffer size: %d\n", size);

    // 6. Test IOCTL CLEAR
    if (ioctl(fd, MSGBUF_IOCTL_CLEAR) < 0) {
        perror("Failed to clear buffer via ioctl");
        close(fd);
        return 1;
    }
    printf("IOCTL: Buffer cleared.\n");

    // 7. Verify buffer is clear
    size = ioctl(fd, MSGBUF_IOCTL_GETSIZE);
    printf("IOCTL: New buffer size: %d\n", size);

    // 8. Close the device
    close(fd);
    printf("Device closed.\n");

    return 0;
}