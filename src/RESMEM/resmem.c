/*-----------------------------------------------------------------------------
(C) Copyright Aveillant Ltd 2012

FILE
    resmem.h

ORIGINAL AUTHOR
    Peter Wurmsdobler

DESCRIPTION
    This module constitutes the driver for the reserved memory into which
    sensor data are streamed to, saved from and loaded into again.

-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
System level includes
-----------------------------------------------------------------------------*/
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include <asm/uaccess.h>

/*-----------------------------------------------------------------------------
Project includes
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
Module includes
-----------------------------------------------------------------------------*/
#include "resmem.h"

/*-----------------------------------------------------------------------------
Private defines
-----------------------------------------------------------------------------*/

#define SUCCESS 0

/*-----------------------------------------------------------------------------
Private data
-----------------------------------------------------------------------------*/

MODULE_LICENSE("GPL v2");

static char driver_name[] = "resmem";

static unsigned long resmem_hwaddr = 0;
static unsigned long resmem_length = 0;

module_param(resmem_hwaddr, ulong, S_IRUSR);
module_param(resmem_length, ulong, S_IRUSR);

unsigned long resmem_data_hwaddr = 0;
unsigned long resmem_data_length = 0;

EXPORT_SYMBOL(resmem_data_hwaddr);
EXPORT_SYMBOL(resmem_data_length);

static int occupied = 0;

/*-----------------------------------------------------------------------------
file operations
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
NAME
    resmem_open

DESCRIPTION
    open file operation for module device file

RETURNS
    SUCCESS or error code

-----------------------------------------------------------------------------*/
static int resmem_open(struct inode *inode, struct file *filp)
{
//    printk("%s: opened\n", driver_name);

    if (occupied)
    {
	printk(KERN_ERR "%s: occupied!\n", driver_name);
        return -EBUSY;
    }
    occupied = 1;

    return SUCCESS;
}

/*-----------------------------------------------------------------------------
NAME
    resmem_release

DESCRIPTION
    release file operation for module device file

RETURNS
    SUCCESS or error code

-----------------------------------------------------------------------------*/
static int resmem_release(struct inode *inode, struct file *filp)
{
    occupied = 0;

//    printk("%s: released\n", driver_name);

    return SUCCESS;
}

/*-----------------------------------------------------------------------------
NAME
    resmem_mmap

DESCRIPTION
    mmap file operation for module device file

RETURNS
    SUCCESS or error code

-----------------------------------------------------------------------------*/
static int resmem_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int result;
    unsigned long requested_size;

//    printk("%s: mmap...\n", driver_name);

    requested_size = vma->vm_end - vma->vm_start;
    if (requested_size != resmem_length)
    {
        printk(KERN_ERR "%s: Error: %lu reserved != %lu requested)\n",
            driver_name, resmem_length, requested_size);
        return -EAGAIN;
    }

    result = remap_pfn_range(vma, vma->vm_start,
        resmem_hwaddr >> PAGE_SHIFT,
        resmem_length, vma->vm_page_prot);
    if (result)
    {
        printk(KERN_ERR "%s: Error in calling remap_pfn_range: returned %d\n",
            driver_name, result);
        return -EAGAIN;
    }

//    printk("%s: mmap OK\n", driver_name);
    return SUCCESS;
}


/*-----------------------------------------------------------------------------
NAME
    resmem_ioctl

DESCRIPTION
    ioctl file operation for module device file

RETURNS
    SUCCESS or error code

-----------------------------------------------------------------------------*/
static long resmem_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
//    printk("%s: ioctl: cmd = %x\n", driver_name, cmd);

    if (_IOC_TYPE(cmd) != RESMEM_IOC_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > RESMEM_IOC_MAXNR) return -ENOTTY;

    switch(cmd)
    {
        case RESMEM_IOC_HWADDR:
        {
//            printk("%s: ioctl: RESMEM_IOC_HWADDR\n", driver_name);

            if (copy_to_user((void*)arg, &resmem_hwaddr, sizeof(resmem_hwaddr)))
            {
                printk("%s: ioctl: RESMEM_IOC_HWADDR: failed to copy to user\n", driver_name);
                return -EFAULT;
            }
            return SUCCESS;
        }

        case RESMEM_IOC_LENGTH:
        {
//            printk("%s: ioctl: RESMEM_IOC_LENGTH\n", driver_name);

            if (copy_to_user((void*)arg, &resmem_length, sizeof(resmem_length)))
            {
                printk("%s: ioctl: RESMEM_IOC_LENGTH: failed to copy to user\n", driver_name);
                return -EFAULT;
            }
            return SUCCESS;
        }

        default:
            return -ENOTTY;
    }

    return -ENOTTY;
}


/*-----------------------------------------------------------------------------
 file operations and device definition
-----------------------------------------------------------------------------*/

static const struct file_operations resmem_fops = {
    owner:      THIS_MODULE,
    unlocked_ioctl:      resmem_ioctl,
    open:       resmem_open,
    mmap:       resmem_mmap,
    release:    resmem_release,
};

static struct miscdevice resmem_dev = {
        MISC_DYNAMIC_MINOR,
        RESMEM_NAME,
        &resmem_fops
};


/*-----------------------------------------------------------------------------
NAME
    resmem_init

DESCRIPTION
    mandatory module exit function

RETURNS
    SUCCESS or error code

-----------------------------------------------------------------------------*/
static int resmem_init(void)
{
    printk("%s: init\n", driver_name);

    if (resmem_hwaddr == 0)
    {
        printk(KERN_ERR "%s: No address specified for reserved memory\n", driver_name);
        return -ENODEV;
    }

    if (resmem_length == 0)
    {
        printk(KERN_ERR "%s: No length specified for reserved memory\n", driver_name);
        return -ENODEV;
    }

    printk("%s: resmem_hwaddr=0x%lx, resmem_length=0x%lx\n",
        driver_name, resmem_hwaddr, resmem_length);

    resmem_data_hwaddr = resmem_hwaddr + 0;
    resmem_data_length = resmem_length - 0;

    /* register device */
    if (misc_register(&resmem_dev) < 0)
    {
        printk(KERN_ERR "%s: Unable to register resmem misc device\n", driver_name);
        return -ENODEV;
    }
    printk("%s: Registered device '/dev/%s'\n", driver_name, RESMEM_NAME);

    printk("%s: Sucessfully installed driver\n", driver_name);
    return SUCCESS;
}

/*-----------------------------------------------------------------------------
NAME
    resmem_exit

DESCRIPTION
    mandatory module exit function

RETURNS
    void

-----------------------------------------------------------------------------*/
static void resmem_exit(void)
{
    printk("%s: exit\n", driver_name);

    misc_deregister(&resmem_dev);
}

module_init(resmem_init);
module_exit(resmem_exit);
