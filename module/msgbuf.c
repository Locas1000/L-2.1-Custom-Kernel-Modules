#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>       // For file_operations, inode
#include <linux/cdev.h>     // For the character device
#include <linux/uaccess.h>  // For copy_to_user/copy_from_user
#include <linux/device.h>   // For class_create/device_create
#include <linux/ioctl.h>    // For ioctl macros
#include <linux/slab.h>     // For kzalloc/kfree
#include <linux/string.h>   // For memset
#include <linux/proc_fs.h>  // For /proc file creation
#include <linux/seq_file.h> // For easy /proc file reading
#include <linux/mutex.h>    // For the mutex lock
#include <linux/spinlock.h> // For the spinlock

// --- IOCTL Definitions ---
// These are also in userspace/msgbuf.h
#define MSGBUF_IOCTL_MAGIC 'M'
#define MSGBUF_IOCTL_CLEAR   _IO(MSGBUF_IOCTL_MAGIC, 1)
#define MSGBUF_IOCTL_GETSIZE _IOR(MSGBUF_IOCTL_MAGIC, 2, int)

// --- Module Metadata ---
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Message Buffer Character Device");
MODULE_VERSION("1.0");

// --- Device Globals ---
#define DEVICE_NAME "msgbuf"
#define BUFFER_SIZE 4096

static int major_number;
static struct cdev msgbuf_cdev;
static struct class* msgbuf_class = NULL;

static char device_buffer[BUFFER_SIZE];
static size_t buffer_len = 0;

// --- ProcFS Globals ---
static struct proc_dir_entry *proc_entry;

static struct {
    unsigned long opens;
    unsigned long reads;
    unsigned long writes;
    unsigned long bytes_read;
    unsigned long bytes_written;
} stats;

// --- Synchronization Globals ---
static DEFINE_MUTEX(buffer_mutex);  // Mutex for the device_buffer
static DEFINE_SPINLOCK(stats_lock); // Spinlock for the stats structure

// --- Function Prototypes ---
static int __init msgbuf_init(void);
static void __exit msgbuf_exit(void);
static int msgbuf_open(struct inode *inode, struct file *file);
static int msgbuf_release(struct inode *inode, struct file *file);
static ssize_t msgbuf_read(struct file *file, char __user *user_buf,
                           size_t count, loff_t *offset);
static ssize_t msgbuf_write(struct file *file, const char __user *user_buf,
                            size_t count, loff_t *offset);
static long msgbuf_ioctl(struct file *file, unsigned int cmd,
                         unsigned long arg);
// llseek is handled by a default function

// --- ProcFS Function Prototypes ---
static int msgbuf_proc_show(struct seq_file *m, void *v);
static int msgbuf_proc_open(struct inode *inode, struct file *file);

// --- File Operations Structure ---
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = msgbuf_open,
    .release = msgbuf_release,
    .read = msgbuf_read,
    .write = msgbuf_write,
    .unlocked_ioctl = msgbuf_ioctl,
    .llseek = default_llseek, // <-- THE FIX IS HERE
};

// --- ProcFS Operations Structure ---
static const struct proc_ops msgbuf_proc_ops = {
    .proc_open = msgbuf_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

// --- Function Implementations ---

static int __init msgbuf_init(void) {
    int ret;
    dev_t dev_no;

    ret = alloc_chrdev_region(&dev_no, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "msgbuf: Failed to allocate major number\n");
        return ret;
    }
    major_number = MAJOR(dev_no);
    printk(KERN_INFO "msgbuf: Major number allocated: %d\n", major_number);

    cdev_init(&msgbuf_cdev, &fops);
    msgbuf_cdev.owner = THIS_MODULE;

    ret = cdev_add(&msgbuf_cdev, dev_no, 1);
    if (ret < 0) {
        printk(KERN_ERR "msgbuf: Failed to add cdev\n");
        unregister_chrdev_region(dev_no, 1);
        return ret;
    }

    msgbuf_class = class_create(DEVICE_NAME);
    if (IS_ERR(msgbuf_class)) {
        printk(KERN_ERR "msgbuf: Failed to create device class\n");
        cdev_del(&msgbuf_cdev);
        unregister_chrdev_region(dev_no, 1);
        return PTR_ERR(msgbuf_class);
    }

    device_create(msgbuf_class, NULL, dev_no, NULL, DEVICE_NAME);
    printk(KERN_INFO "msgbuf: Device /dev/msgbuf created\n");

    proc_entry = proc_create("msgbuf_stats", 0444, NULL, &msgbuf_proc_ops);
    if (proc_entry == NULL) {
        printk(KERN_ERR "msgbuf: Failed to create /proc entry\n");
        device_destroy(msgbuf_class, dev_no);
        class_destroy(msgbuf_class);
        cdev_del(&msgbuf_cdev);
        unregister_chrdev_region(dev_no, 1);
        return -ENOMEM;
    }
    printk(KERN_INFO "msgbuf: /proc/msgbuf_stats created\n");

    return 0; // Success
}

static void __exit msgbuf_exit(void) {
    dev_t dev_no = MKDEV(major_number, 0);

    proc_remove(proc_entry);
    device_destroy(msgbuf_class, dev_no);
    class_destroy(msgbuf_class);
    cdev_del(&msgbuf_cdev);
    unregister_chrdev_region(dev_no, 1);

    printk(KERN_INFO "msgbuf: Module unloaded and all resources freed\n");
}

// --- Character Device Function Implementations ---

static int msgbuf_open(struct inode *inode, struct file *file) {
    // Tell the VFS layer our max size for lseek
    inode->i_size = BUFFER_SIZE; // <-- THE OTHER FIX IS HERE

    spin_lock(&stats_lock);
    stats.opens++;
    spin_unlock(&stats_lock);

    printk(KERN_INFO "msgbuf: Device opened\n");
    return 0;
}

static int msgbuf_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "msgbuf: Device released\n");
    return 0;
}

static ssize_t msgbuf_read(struct file *file, char __user *user_buf,
                           size_t count, loff_t *offset) {

    size_t to_read;
    ssize_t ret;

    if (mutex_lock_interruptible(&buffer_mutex)) {
        return -ERESTARTSYS;
    }

    if (*offset >= buffer_len) {
        ret = 0;
        goto out;
    }

    to_read = min(count, (size_t)(buffer_len - *offset));

    if (copy_to_user(user_buf, device_buffer + *offset, to_read)) {
        ret = -EFAULT;
        goto out;
    }

    *offset += to_read;
    ret = to_read;

out:
    mutex_unlock(&buffer_mutex);

    if (ret > 0) {
        spin_lock(&stats_lock);
        stats.reads++;
        stats.bytes_read += ret;
        spin_unlock(&stats_lock);
    }

    printk(KERN_INFO "msgbuf: Read %zu bytes\n", ret);
    return ret;
}

static ssize_t msgbuf_write(struct file *file, const char __user *user_buf,
                            size_t count, loff_t *offset) {

    size_t to_write;
    ssize_t ret;

    if (count > BUFFER_SIZE) {
        to_write = BUFFER_SIZE;
    } else {
        to_write = count;
    }

    if (mutex_lock_interruptible(&buffer_mutex)) {
        return -ERESTARTSYS;
    }

    if (copy_from_user(device_buffer, user_buf, to_write)) {
        ret = -EFAULT;
        goto out_write;
    }

    buffer_len = to_write;
    ret = to_write;

    // We are overwriting, so reset offset for subsequent reads
    *offset = 0;

out_write:
    mutex_unlock(&buffer_mutex);

    if (ret > 0) {
        spin_lock(&stats_lock);
        stats.writes++;
        stats.bytes_written += ret;
        spin_unlock(&stats_lock);
    }

    printk(KERN_INFO "msgbuf: Wrote %zu bytes\n", ret);
    return ret;
}

static long msgbuf_ioctl(struct file *file, unsigned int cmd,
                         unsigned long arg) {

    long ret = -EINVAL;

    switch (cmd) {
    case MSGBUF_IOCTL_CLEAR:
        printk(KERN_INFO "msgbuf: IOCTL clear buffer\n");
        if (mutex_lock_interruptible(&buffer_mutex)) {
            return -ERESTARTSYS;
        }
        buffer_len = 0;
        memset(device_buffer, 0, BUFFER_SIZE);
        mutex_unlock(&buffer_mutex);
        ret = 0;
        break;

    case MSGBUF_IOCTL_GETSIZE:
        if (mutex_lock_interruptible(&buffer_mutex)) {
            return -ERESTARTSYS;
        }
        ret = buffer_len;
        mutex_unlock(&buffer_mutex);
        break;

    default:
        ret = -EINVAL;
    }

    return ret;
}

// --- ProcFS Function Implementations ---

static int msgbuf_proc_show(struct seq_file *m, void *v) {
    size_t current_len;
    unsigned long opens, reads, writes, b_read, b_written;

    spin_lock(&stats_lock);
    opens = stats.opens;
    reads = stats.reads;
    writes = stats.writes;
    b_read = stats.bytes_read;
    b_written = stats.bytes_written;
    spin_unlock(&stats_lock);

    mutex_lock(&buffer_mutex);
    current_len = buffer_len;
    mutex_unlock(&buffer_mutex);

    seq_printf(m, "Message Buffer Statistics\n");
    seq_printf(m, "=========================\n");
    seq_printf(m, "Opens:          %lu\n", opens);
    seq_printf(m, "Reads:          %lu\n", reads);
    seq_printf(m, "Writes:         %lu\n", writes);
    seq_printf(m, "Bytes Read:     %lu\n", b_read);
    seq_printf(m, "Bytes Written:  %lu\n", b_written);
    seq_printf(m, "Buffer Size:    %d\n", BUFFER_SIZE);
    seq_printf(m, "Current Length: %zu\n", current_len);
    return 0;
}

static int msgbuf_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, msgbuf_proc_show, NULL);
}

// --- Register init/exit functions ---
module_init(msgbuf_init);
module_exit(msgbuf_exit);