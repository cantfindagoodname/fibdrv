#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/string.h>

#include "bn.h"

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 500

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);

static ktime_t kt;

void fib_sequence(long long k, bn_t **out)
{
    if (k >= 2) {
        autofree bn_t *f1 = NULL, *f2 = NULL;
        autofree bn_t *z0 = NULL;
        assign(&z0, 0);
        assign(&f1, 0);
        assign(&f2, 1);
        for (int i = 32 - __builtin_clz(k); i >= 0; --i) {
            autofree bn_t *t1 = NULL, *t2 = NULL;
            /* t2 = a^2 + b^2 */
            mul(f1, f1, &t1);
            mul(f2, f2, &t2);
            add(t1, t2, &t2);

            /* t1 = a * (2b - a) */
            add(f2, f2, &t1);
            sub(t1, f1, &t1);
            mul(t1, f1, &t1);

            add(z0, t1, &f1);
            add(z0, t2, &f2);
            if (k & (1 << i)) {
                add(f1, f2, &t1);
                add(z0, f2, &f1);
                add(z0, t1, &f2);
            }
        }
        // display(f1);

        *out = krealloc(*out, sizeof(bn_t) + f1->size * sizeof(uint32_t),
                        GFP_KERNEL);
        (*out)->size = f1->size;
        for (int index = 0; index < f1->size; ++index)
            (*out)->arr[index] = f1->arr[index];
    } else {
        assign(out, k);
    }
}

bn_t *fib_time_proxy(long long k)
{
    bn_t *result = NULL;
    kt = ktime_get();
    fib_sequence(k, &result);
    kt = ktime_sub(ktime_get(), kt);

    return result;
}

static int fib_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&fib_mutex);
    return 0;
}

/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    int error_count = 0;
    autofree bn_t *sol = fib_time_proxy(*offset);

    int len = 0;
    char sol_buf[256];
    sol_buf[len] = '\0';

    len += snprintf(sol_buf + len, 256 - len, "%u", sol->arr[0]);
    for (size_t i = 1; i < sol->size; ++i) {
        len += snprintf(sol_buf + len, 256 - len, "%0*u", MAX_LEN, sol->arr[i]);
    }
    sol_buf[len] = '\0';
    printk(KERN_INFO "ANS >>>> %s\n", sol_buf);

    error_count = copy_to_user(buf, sol_buf, strlen(sol_buf) + 1);

    if (error_count == 0) {
        printk(KERN_INFO "Sent Fib to user_buffer\n");
        return (ssize_t) ktime_to_ns(kt);
    } else {
        printk(KERN_INFO "Fail to send Fib to user_buffer\n");
        return -EFAULT;
    }
}

/* write operation is skipped */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    return (ssize_t) ktime_to_ns(kt);
}

static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;

    mutex_init(&fib_mutex);

    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    fib_cdev->ops = &fib_fops;
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);
