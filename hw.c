// cpsysinfo.c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
/* Needed by all modules */
/* Needed for KERN_INFO */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("KRERK PIROMSOPA, PH.D. <Krerk.P@chula.ac.th>");
MODULE_DESCRIPTION("\"cpsysinfo\" Character Device");
#define DEVICENAME "cpsysinfo"
static int dev_major;
static int dev_open = 0;
static char *f_ptr;
static char active_process_buffer[4096];
static char amount_of_memory_buffer[4096];
// prototypes for device functions
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *inode, struct file *file);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
// File operations structor
// Only implement those that will be used.
static struct file_operations dev_fops = {
    .read = device_read,
    .open = device_open,
    .release = device_release};
int init_module(void)
{
    printk(KERN_INFO "CPCHAR: dev cpsysinfo init\n");
    dev_major = register_chrdev(0, DEVICENAME, &dev_fops);
    if (dev_major < 0)
    {
        printk(KERN_ALERT "Fail register_chrdev cpsysinfo with %d\n", dev_major);
        return dev_major;
    }
    printk(KERN_INFO "Device MajorNumber %d.\n", dev_major);
    printk(KERN_INFO "To create a device file:\n");
    printk(KERN_INFO "\t'mknod /dev/%s c %d 0'.\n", DEVICENAME, dev_major);
    printk(KERN_INFO "Try varying minor numbers.\n");
    printk(KERN_INFO "Please remove the device file and module when done.\n");
    /* * non 0 - means init_module failed */
    return 0;
}
void cleanup_module(void)
{
    printk(KERN_INFO "CPCHAR: dev cpsysinfo cleanup\n");
    unregister_chrdev(dev_major, DEVICENAME);
}

void readActiveProcess(void)
{
    struct task_struct *task;
    unsigned idx = 0;

    for_each_process(task)
    {
        idx += snprintf(active_process_buffer + idx, 128, "[%5d] %s\n", task->pid, task->comm);
    }
}

void readMemoryProcess(void)
{
    struct sysinfo* sysInfo;
    si_meminfo(sysInfo);
    unsigned idx = 0;
    long available = si_mem_available();
    idx += snprintf(amount_of_memory_buffer + idx, 128, "MemTotal %d\n", sysInfo->totalram);
    idx += snprintf(amount_of_memory_buffer + idx, 128, "MemFree %d\n", sysInfo->freeram);
    idx += snprintf(amount_of_memory_buffer + idx, 128, "MemAvailable %ld\n", available);
}

static int device_open(struct inode *inode, struct file *file)
{
    if (dev_open)
        return -EBUSY;
    dev_open++;
    printk(KERN_INFO "dev minor %d\n", MINOR(inode->i_rdev));
    switch (MINOR(inode->i_rdev))
    {
    case 0:
        readActiveProcess();
        f_ptr = (char *)active_process_buffer;
        break;
    case 1:
        readMemoryProcess();
        f_ptr = (char *)amount_of_memory_buffer;
        break;
    default:
        readActiveProcess();
        f_ptr = (char *)active_process_buffer;
        break;
    }
    // lock module
    try_module_get(THIS_MODULE);
    return 0;
}
static int device_release(struct inode *inode, struct file *file)
{
    dev_open--; /* We're now ready for our next caller */
    // release module
    module_put(THIS_MODULE);
    return 0;
}
static ssize_t device_read(struct file *filp,
                           char *buffer,
                           /* see include/linux/fs.h */
                           /* buffer to fill with data */
                           /* length of the buffer */
                           size_t length,
                           loff_t *offset)
{
    int bytes_read = 0;
    if (*f_ptr == 0)
    {
        return 0;
    }
    while (length && *f_ptr)
    {
        put_user(*(f_ptr++), buffer++);
        length--;
        bytes_read++;
    }
    return bytes_read;
}