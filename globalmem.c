#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/io.h>
//#include <asm-generic/switch_to.h>
#include <linux/uaccess.h>

#define GLOBALMEM_SIZE 0X1000 //4KB
#define MEM_CLEAR 0x1
#define GLOBALMEM_MAJOR 254

static int globalmem_major = GLOBALMEM_MAJOR;

struct globalmem_dev {
	struct cdev cdev;
	unsigned char mem[GLOBALMEM_SIZE];
};

struct globalmem_dev dev;
static const struct file_operations globalmem_fops; 

static void globalmem_setup_cdev(void);
static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos);
static ssize_t globalmem_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos);
static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig);
static int globalmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

int globalmem_init(void) {
	int result;
	dev_t devno = MKDEV(globalmem_major, 0);

	if (globalmem_major)
		result = register_chrdev_region(devno, 1, "globalmem");
	else {
		result = alloc_chrdev_region(&devno, 0, 1, "globalmem");
		globalmem_major = MAJOR(devno);
	}

	if (result < 0)
		return result;

	globalmem_setup_cdev();
	return 0;
}

void globalmem_exit(void) {
	cdev_del(&dev.cdev);
	unregister_chrdev_region(MKDEV(globalmem_major, 0), 1);
}

static void globalmem_setup_cdev(void) {
	int err, devno = MKDEV(globalmem_major, 0);
	cdev_init(&dev.cdev, &globalmem_fops);
	dev.cdev.owner = THIS_MODULE;
	dev.cdev.ops = &globalmem_fops;
	err = cdev_add(&dev.cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding globalmem", err);
}

static const struct file_operations globalmem_fops = {
	.owner = THIS_MODULE,
	.llseek = globalmem_llseek,
	.read = globalmem_read,
	.write = globalmem_write,
	.unlocked_ioctl = globalmem_ioctl,
};

static ssize_t globalmem_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos) {
	unsigned long p = *ppos;
	int ret = 0;
	if (p>=GLOBALMEM_SIZE)
		return count? - ENXIO:0;

	if (count>GLOBALMEM_SIZE - p)
		count = GLOBALMEM_SIZE - p;

	if (copy_to_user(buf, (void*)(dev.mem+p), count))
		ret = - EFAULT;
	else {
		*ppos+=count;
		ret=count;
		printk(KERN_INFO "read %d bytes from %d\n", count, p);
	}
}

static ssize_t globalmem_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos) {
	unsigned long p=*ppos;
	int ret=0;
	if (p>=GLOBALMEM_SIZE)
		return count? - ENXIO:0;

	if (count>GLOBALMEM_SIZE-p)
		count=GLOBALMEM_SIZE-p;

	if (copy_from_user(dev->mem + p, buf, count))
		ret= - EFAULT;
	else {
		*ppos+=count;
		ret=count;
		printk(KERN_INFO "Written %d bytes from %d\n", count, p);
	}
	return ret;
}

static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig) {
	loff_t ret;
	switch (orig) {
		case 0:
			if (offset<0) {
				ret= - EINVAL;
				break;
			}
			if ((unsigned int)offset > GLOBALMEM_SIZE) {
				ret= - EINVAL;
				break;
			}
			filp->f_pos=(unsigned int)offset;
			ret=filp->f_pos;
			break;
		case 1:
			if ((filp->f_pos+offset)>GLOBALMEM_SIZE) {
				ret= - EINVAL;
				break;
			}
			if ((filp->f_pos+offset)<0) {
				ret= - EINVAL;
				break;
			}
			filp->f_pos+=offset;
			ret=filp->f_pos;
			break;
		default:
			ret= - EINVAL;
	}
	return ret;
}

static int globalmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	switch (cmd) {
		case MEM_CLEAR:
			memset(dev->mem,o,GLOBALMEM_SIZE);
			printk(KERN_INFO "Globalmem is set to zero\n");
			break;
		default:
			return - EINVAL;
	}
	return 0;
}
