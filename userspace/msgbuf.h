#ifndef MSGBUF_H
#define MSGBUF_H

#include <linux/ioctl.h> // For _IO, _IOR macros

// Define the magic number
#define MSGBUF_IOCTL_MAGIC 'M'

// Define the ioctl commands
#define MSGBUF_IOCTL_CLEAR   _IO(MSGBUF_IOCTL_MAGIC, 1)
#define MSGBUF_IOCTL_GETSIZE _IOR(MSGBUF_IOCTL_MAGIC, 2, int)
// Note: We're not using _IOW('M', 3) from the assignment example,
// but it could be added here if we implemented it.

#endif // MSGBUF_H