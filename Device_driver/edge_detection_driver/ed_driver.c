#include <linux/device.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/mm.h>

#define BASE_ADDR   0x43c00000
#define SIZE_ADDR   0x10000
#define DIR_ADDR    0x0000
#define DATA_ADDR   0x0000
#define DRIVER_NAME "counter"
#define BIT_RESET    0x0


struct drv {
    dev_t num; // device number
    struct class *vclass; // create class
    struct device *vdevice; //create device file
    struct cdev *vcdev; //character device
    void __iomem *regbase;
} counter;

static int stringtoint(char *str) {
    int result = 0;
    int sign = 1;
    int i = 0;
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    }
    for (; str[i] != '\0'; ++i) {
        result = result * 10 + (str[i] - '0'); 
//	printk("result in stringtoint: %d and str[i]: %d", result, (str[i] - '0'));
    }
    return result * sign;
}


static void drv_set_val(int data) {
    u32 val = 0;
    uint32_t bit_data = (uint32_t)data;
    val = readl(counter.regbase + DATA_ADDR);
    if(bit_data == 0){
	    val &= bit_data;
    }
    else {
	val &= BIT_RESET;
	writel(val, counter.regbase + DATA_ADDR);
    	val |= bit_data;
    }
    printk(KERN_INFO "val: %u\n", val);
    writel(val, counter.regbase + DATA_ADDR);
}

static int mydev_open(struct inode *inode, struct file *file) {
    return 0;
}

static int mydev_release(struct inode *inode, struct file *file) {
    return 0;
}

#define MAX_DATA_SIZE 1024

char *kbuf; // Global variable representing the device's data buffer
size_t data_size = 0;
static ssize_t mydev_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos) {
    //char *kbuf;
    int result = 0;
    if (*pos >= SIZE_ADDR) {
        return -EINVAL;
    }
    kbuf = kmalloc(count, GFP_KERNEL);

    if (copy_from_user(kbuf, buf, count)) {
        kfree(kbuf);
        return -EFAULT;
    }
    kbuf[count-1] = '\0';
    result = stringtoint(kbuf);
    printk("result:%d and kbuf[count]: %c and kbuf[count-1]: %c and count: %d\n", result, kbuf[0], kbuf[1], count);
    *pos += count;
    printk("Offset in write: %lld\n", *pos);
    if(*pos > data_size){
	    data_size = *pos;
    }
    drv_set_val(result);
    return count;
}
ssize_t mydev_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
    // Check if the offset is beyond the end of the data buffer
    if (*offset >= data_size)
        return 0; // End of file

    // Calculate the number of bytes to read
    size_t bytes_to_read = min(length, data_size - *offset);

    // Copy data from the device's data buffer to the user buffer
    if (copy_to_user(buffer, kbuf + *offset, bytes_to_read) != 0)
        return -EFAULT; // Error copying data to user space

    // Update the offset
    *offset += bytes_to_read;
    printk("\n");
    return bytes_to_read;
}
/*
static ssize_t mydev_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    if (*f_pos >= SIZE_ADDR) {
        return -EINVAL;
    }
    if (copy_to_user(buf, *kbuf + *f_pos, count)) {
        return -EFAULT;
    }
    *f_pos += count;
    printk("Offset in read: %lld\n", *f_pos);
    return count;
}
*/
static const struct file_operations mydev_fops = {
    .owner = THIS_MODULE,
    .read = mydev_read,
    .write = mydev_write,
    .open = mydev_open,
    .release = mydev_release,
};

static int __init drvled_init(void) {
    int ret = 0;

    if (!request_mem_region(BASE_ADDR, SIZE_ADDR, DRIVER_NAME)) {
        pr_err("%s: Error requesting I/O memory region\n", DRIVER_NAME);
        ret = -EBUSY;
        goto ret_err_request_mem_region;
    }

    counter.regbase = ioremap(BASE_ADDR, SIZE_ADDR);
    if (!counter.regbase) {
        pr_err("%s: Error mapping I/O\n", DRIVER_NAME);
        ret = -ENOMEM;
        goto err_ioremap;
    }

    /* Allocate device number */
    ret = alloc_chrdev_region(&counter.num, 0, 1, DRIVER_NAME);
    if (ret < 0) {
        pr_err("%s: Can't allocate device number\n", DRIVER_NAME);
        goto err_alloc_chrdev_region;
    }

    /* Create class to contain the device file */
    counter.vclass = class_create(THIS_MODULE, "class_counter");
    if (IS_ERR(counter.vclass)) {
        pr_err("%s: Can't create class\n", DRIVER_NAME);
        ret = PTR_ERR(counter.vclass);
        goto err_class_create;
    }

    /* Create device file in /dev directory */
    counter.vdevice = device_create(counter.vclass, NULL, counter.num, NULL, "ip_ed");
    if (IS_ERR(counter.vdevice)) {
        pr_err("%s: Can't create device file\n", DRIVER_NAME);
        ret = PTR_ERR(counter.vdevice);
        goto err_device_create;
    }

    /* Allocate and register character device */
    counter.vcdev = cdev_alloc();
    if (counter.vcdev == NULL) {
        pr_err("%s: Can't allocate character device\n", DRIVER_NAME);
        ret = -ENOMEM;
        goto err_cdev_alloc;
    }

    /* Associate character device with operations */
    cdev_init(counter.vcdev, &mydev_fops);
    ret = cdev_add(counter.vcdev, counter.num, 1);
    if (ret < 0) {
        pr_err("%s: Can't register character device\n", DRIVER_NAME);
        goto err_cdev_add;
    }

    drv_set_val(4);
    printk("%s: Initialization successful\n", DRIVER_NAME);

    return 0;

err_cdev_add:
    kfree(counter.vcdev);
err_cdev_alloc:
    device_destroy(counter.vclass, counter.num);
err_device_create:
    class_destroy(counter.vclass);
err_class_create:
    unregister_chrdev_region(counter.num, 1);
err_alloc_chrdev_region:
    iounmap(counter.regbase);
err_ioremap:
    release_mem_region(BASE_ADDR, SIZE_ADDR);
ret_err_request_mem_region:
    return ret;
}

static void __exit drvled_exit(void) {
    /* Release cdev */
    cdev_del(counter.vcdev);

    /* Delete device file */
    device_destroy(counter.vclass, counter.num);

    /* Destroy class */
    class_destroy(counter.vclass);

    /* Release device number */
    unregister_chrdev_region(counter.num, 1);

    /* Unmap I/O memory */
    iounmap(counter.regbase);

    /* Release I/O memory region */
    release_mem_region(BASE_ADDR, SIZE_ADDR);

    printk("drvled: Exit\n");
}

module_init(drvled_init);
module_exit(drvled_exit);
MODULE_LICENSE("GPL");
