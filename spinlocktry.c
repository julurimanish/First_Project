/****************************************************************************//**
*  \file       driver.c
*
*  \details    Simple linux driver (Spinlock)
*
*  \author     EmbeTronicX
*
*  \Tested with Linux raspberrypi 5.10.27-v7l-embetronicx-custom+
*
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include<linux/slab.h>                 //kmalloc()
#include<linux/uaccess.h>              //copy_to/from_user()
#include <linux/kthread.h>             //kernel threads
#include <linux/sched.h>               //task_struct 
#include <linux/delay.h>
#include <linux/err.h>
#include<linux/sysfs.h> 
#include<linux/kobject.h> 
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/err.h>
#include <asm/hw_irq.h>
//#include <linux/err.h>
 

DEFINE_SPINLOCK(etx_spinlock);

void tasklet_fn(unsigned long arg);

DECLARE_TASKLET(tasklet,tasklet_fn,1);
//spinlock_t etx_spinlock;
unsigned long etx_global_variable = 0;
 
dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;
 
static int __init etx_driver_init(void);
static void __exit etx_driver_exit(void);
 
static struct task_struct *etx_thread1;
//static struct task_struct *etx_thread2; 
 
/*************** Driver functions **********************/
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, 
                char __user *buf, size_t len,loff_t * off);
static ssize_t etx_write(struct file *filp, 
                const char *buf, size_t len, loff_t * off);
 /******************************************************/
 
int thread_function(void *pv);
//int thread_function2(void *pv);



        //Thread
int thread_function(void *pv)
{
    while(!kthread_should_stop()) {
        spin_lock_bh(&etx_spinlock);
        etx_global_variable++;
        printk(KERN_INFO "In EmbeTronicX Thread Function %lu\n", etx_global_variable);
        spin_unlock_bh(&etx_spinlock);
        msleep(1000);
    }
    return 0;
}


/*Tasklet Function*/
void tasklet_fn(unsigned long arg)
{
        spin_lock_bh(&etx_spinlock);
        etx_global_variable++;
        printk(KERN_INFO "Executing Tasklet Function : %lu\n", etx_global_variable);
        spin_unlock_bh(&etx_spinlock);
}  


    
/*
** thread function 1
*/
/*int thread_function1(void *pv)
{
    
    while(!kthread_should_stop()) {
        if(!spin_is_locked(&etx_spinlock)) {
            pr_info("Spinlock is not locked in Thread Function1\n");
        }
        spin_lock(&etx_spinlock);
        if(spin_is_locked(&etx_spinlock)) {
            pr_info("Spinlock is locked in Thread Function1\n");
        }
        etx_global_variable++;
        pr_info("In EmbeTronicX Thread Function1 %lu\n", etx_global_variable);
        spin_unlock(&etx_spinlock);
        msleep(1000);
    }
    return 0;
}


** thread function 2
/
int thread_function2(void *pv)
{
    while(!kthread_should_stop()) {
        spin_lock(&etx_spinlock);
        etx_global_variable++;
        pr_info("In EmbeTronicX Thread Function2 %lu\n", etx_global_variable);
        spin_unlock(&etx_spinlock);
        msleep(1000);
    }
    return 0;
}
*/
//File operation structure  
static struct file_operations fops =
{
        .owner          = THIS_MODULE,
        .read           = etx_read,
        .write          = etx_write,
        .open           = etx_open,
        .release        = etx_release,
};

/*
** This function will be called when we open the Device file
*/ 
static int etx_open(struct inode *inode, struct file *file)
{
        pr_info("Device File Opened...!!!\n");
        return 0;
}

/*
** This function will be called when we close the Device file
*/ 
static int etx_release(struct inode *inode, struct file *file)
{
        pr_info("Device File Closed...!!!\n");
        return 0;
}

/*
** This function will be called when we read the Device file
*/
static ssize_t etx_read(struct file *filp, 
                char __user *buf, size_t len, loff_t *off)
{
        pr_info("Read function\n");
 
        return 0;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filp, 
                const char __user *buf, size_t len, loff_t *off)
{
        pr_info("Write Function\n");
        return len;
}

/*
** Module Init function
*/
static int __init etx_driver_init(void)
{
        /*Allocating Major number*/
        if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0){
                pr_info("Cannot allocate major number\n");
                return -1;
        }
        pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev));
 
        /*Creating cdev structure*/
        cdev_init(&etx_cdev,&fops);
 
        /*Adding character device to the system*/
        if((cdev_add(&etx_cdev,dev,1)) < 0){
            pr_info("Cannot add the device to the system\n");
            goto r_class;
        }
 
        /*Creating struct class*/
        if(IS_ERR(dev_class = class_create(THIS_MODULE,"etx_class"))){
            pr_info("Cannot create the struct class\n");
            goto r_class;
        }
 
        /*Creating device*/
        if(IS_ERR(device_create(dev_class,NULL,dev,NULL,"etx_device"))){
            pr_info("Cannot create the Device \n");
            goto r_device;
        }
 
        
        /* Creating Thread 1 */
        etx_thread1 = kthread_run(thread_function,NULL,"eTx Thread1");
        if(etx_thread1) {
            pr_err("Kthread1 Created Successfully...\n");
        } else {
            pr_err("Cannot create kthread1\n");
             goto r_device;
        }
 
         /* Creating Thread 2
        etx_thread2 = kthread_run(thread_function2,NULL,"eTx Thread2");
        if(etx_thread2) {
            pr_err("Kthread2 Created Successfully...\n");
        } else {
            pr_err("Cannot create kthread2\n");
             goto r_device;
        }
        //spin_lock_init(&etx_spinlock);*/
	tasklet_schedule(&tasklet); 
        
        pr_info("Device Driver Insert...Done!!!\n");
        return 0;
 
 
r_device:
        class_destroy(dev_class);
r_class:
        unregister_chrdev_region(dev,1);
        cdev_del(&etx_cdev);
        return -1;
}

/*
** Module exit function
*/
static void __exit etx_driver_exit(void)
{
        kthread_stop(etx_thread1);
        //kthread_stop(etx_thread2);
	tasklet_kill(&tasklet);
        device_destroy(dev_class,dev);
        class_destroy(dev_class);
        cdev_del(&etx_cdev);
        unregister_chrdev_region(dev, 1);
        pr_info("Device Driver Remove...Done!!\n");
}
 
module_init(etx_driver_init);
module_exit(etx_driver_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("EmbeTronicX <embetronicx@gmail.com>");
MODULE_DESCRIPTION("A simple device driver - Spinlock");
MODULE_VERSION("1.18");