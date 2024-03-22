

#include<linux/module.h>
#include<linux/fs.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/cdev.h>
#include<linux/device.h>
//#include<linux/slab.h>
#include<linux/kdev_t.h>
#include<linux/uaccess.h>

#define NO_OF_DEVICES      4

#define MEM_SIZE_PCDEV1    1024
#define MEM_SIZE_PCDEV2    512
#define MEM_SIZE_PCDEV3    1024
#define MEM_SIZE_PCDEV4    512

int pcd_open(struct inode *inode, struct file *filp);
int pcd_release(struct inode *inode, struct file *file);
ssize_t pcd_read(struct file *filp, char __user *buf, size_t len, loff_t *f_ops);
ssize_t pcd_write(struct file *filp, const char __user *buf, size_t len, loff_t *f_ops);
loff_t pcd_lseek(struct file *filp, loff_t offset, int whence);


/* pseudo device's memory */
char device_buffer_pcdev1[MEM_SIZE_PCDEV1];
char device_buffer_pcdev2[MEM_SIZE_PCDEV2];
char device_buffer_pcdev3[MEM_SIZE_PCDEV3];
char device_buffer_pcdev4[MEM_SIZE_PCDEV4];

/*Device private data structure */

struct pcdev_private_data 
{
	char *buffer;
	unsigned size;
	const char *serial_number;
	int perm;
	struct cdev cdev;
};


struct pcdrv_private_data 
{
	int total_devices;
	dev_t device_number;
	struct class *class_pcd;
	struct device *device_pcd;
	struct pcdev_private_data pcdev_data[NO_OF_DEVICES];
};

static struct file_operations pcd_fops = 
{
	.owner=THIS_MODULE,
	.open = pcd_open,
	.read = pcd_read,
	.write = pcd_write,
	.llseek = pcd_lseek,
	.release = pcd_release
};

struct pcdrv_private_data pcdrv_data =
{
	.total_devices = NO_OF_DEVICES,
	.pcdev_data =
	{
		[0] = {
			.buffer        = device_buffer_pcdev1,
			.size          = MEM_SIZE_PCDEV1,
			.serial_number = "PCDEV1XYZ123",
			.perm          = O_RDONLY
		},

		[1] = {
			.buffer        = device_buffer_pcdev2,
			.size          = MEM_SIZE_PCDEV2,
			.serial_number = "PCDEV2XYZ123",
			.perm          = O_WRONLY
		},

		[2] = {
			.buffer        = device_buffer_pcdev3,
			.size          = MEM_SIZE_PCDEV3,
			.serial_number = "PCDEV3XYZ123",
			.perm          = O_RDWR
		},

		[3] = {
			.buffer        = device_buffer_pcdev4,
			.size          = MEM_SIZE_PCDEV4,
			.serial_number = "PCDEV4XYZ123",
			.perm          = O_RDWR
		}
	}
};

int check_permission(int dev_perm, int acc_mode)
{
	if(dev_perm == O_RDWR)
		return 0;

	if((dev_perm == O_RDONLY) && ( (acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE)))
		return 0;

	if((dev_perm == O_WRONLY) && ( (acc_mode & FMODE_WRITE) && !(acc_mode & FMODE_READ)))
		return 0;

	return -EPERM;
}

int __init pcd_driver_init(void)
{
	int ret;
	int i,temp;

	/* Dynamically allocate device number*/
	ret = alloc_chrdev_region(&pcdrv_data.device_number, 0, NO_OF_DEVICES, "pcdevs");
	
	if(ret<0)
	{
		pr_err("Alloc chrdev failed!!\n");
		goto out;
	}

	/*create device class under /sys/class/ */

	pcdrv_data.class_pcd = class_create(THIS_MODULE, "pcd_class");
	//pcdrv_data.class_pcd = class_create("pcd_class");        //Kerenl >6
	if(IS_ERR(pcdrv_data.class_pcd))
	{
		pr_err("Class creation failed!!\n");
		ret = PTR_ERR(pcdrv_data.class_pcd);
		goto unreg_chrdev;
	}

	for(i=0; i<NO_OF_DEVICES; i++)
	{
		pr_info("Device number <major>:<minor> = %d:%d\n",MAJOR(pcdrv_data.device_number+i),MINOR(pcdrv_data.device_number+i));

		/*Initialize the cdev structure with fops*/
		cdev_init(&pcdrv_data.pcdev_data[i].cdev, &pcd_fops);

		/* Register a device (cdev structure) with VFS */
		pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
		ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev, pcdrv_data.device_number+i, 1);

		if(ret < 0)
		{
			pr_err("Cdev add failed!!\n");
			goto cdev_del;
		}

		pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, pcdrv_data.device_number+i, NULL, "pcdev_%d",i+1);
		//pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, pcdrv_data.device_number+i, NULL, "pcdev");
		if(IS_ERR(pcdrv_data.device_pcd))
		{
			pr_err("Device create failed!!\n");
			ret = PTR_ERR(pcdrv_data.device_pcd);
			goto class_del;
		}
	}

	pr_info("Module init was successful!!!\n");

	return 0;

cdev_del:
class_del:
	temp = i;
	for(; temp>=0; temp--)
	{
		device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number+temp);
		cdev_del(&pcdrv_data.pcdev_data[temp].cdev);
	}

	class_destroy(pcdrv_data.class_pcd);

/*cdev_del:
	temp = i;
	for(; temp>=0; temp--)
		cdev_del(&pcdrv_data.pcdev_data[temp].cdev);
*/
unreg_chrdev:
	unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);

out:
	pr_info("Module insertion failed\n");
        return ret;
}


int pcd_open(struct inode *inode, struct file *filp)
{
	int ret;
	int minor;

	struct pcdev_private_data *pcdev_data;

	/*finding which device file is accessed*/
	minor = MINOR(inode->i_rdev);
	pr_info("Minor access = %d\n", minor);

	/*getting device private data structure*/
	pcdev_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev);
	
	/* to supply device private data to other methods of driver */
	filp->private_data = pcdev_data;
	ret = check_permission(pcdev_data->perm, filp->f_mode);

	(!ret)? pr_info("open was successful\n") :  pr_info("open was unsuccessful\n");
       return ret;
}

ssize_t pcd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_ops)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data *)filp->private_data;
	
	int max_size = pcdev_data->size;

	pr_info("Read requested for %zu bytes \n",count);
	pr_info("Current file position = %lld\n",*f_ops);
	
	if((*f_ops + count) > max_size)
	{
		count = max_size - *f_ops;
	}

	if(copy_to_user(buf, pcdev_data->buffer + (*f_ops), count))
	{
		pr_err("Data read error!!\n");
		return -EFAULT;
	}

	*f_ops += count;
	
	pr_info("Number of bytes successfully read = %zu\n",count);
	pr_info("Updated file position = %lld\n",*f_ops);

	return count;
}

ssize_t pcd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_ops)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data *)filp->private_data;
	
	int max_size = pcdev_data->size;

	pr_info("Write requested for %zu bytes\n",count);
	pr_info("Current file position = %lld\n",*f_ops);

	if((*f_ops + count) > max_size)
	{
		count = max_size - *f_ops;
	}

	if(!count)
	{
		pr_err("No space left on the device \n");
		return -ENOMEM;
	}

	if(copy_from_user(pcdev_data->buffer+(*f_ops), buf, count))
	{
		return -EFAULT;
	}

	*f_ops += count;

	pr_info("Number of bytes successfully written = %zu\n",count);
	pr_info("Updated file position = %lld\n",*f_ops);

	/*Return number of bytes which have been successfully written */
	return count;
	
}

loff_t pcd_lseek(struct file *filp, loff_t offset, int whence)
{
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data *)filp->private_data;
	
	int max_size = pcdev_data->size;
	
	loff_t temp;
	
	pr_info("lseek requested \n");
	pr_info("Current file position = %lld\n",filp->f_pos);

	switch(whence)
	{
	case SEEK_SET:
		if(offset > max_size || (offset < 0))
			return -EINVAL;
		filp->f_pos = offset;
		break;
		
	case SEEK_CUR:
		temp = filp->f_pos + offset;
		if(temp > max_size || (temp < 0))
			return -EINVAL;
		filp->f_pos = temp;
		break;
		
	case SEEK_END:
		temp = max_size + offset;
		if(temp > max_size || (temp < 0))
			return -EINVAL;
		filp->f_pos = temp;
		break;
		
	default:
		return -EINVAL;
	}
	pr_info("Updated file position = %lld\n",filp->f_pos);
	return filp->f_pos;
}

int pcd_release(struct inode *inode, struct file *file)
{
	pr_info("release was successful\n");
        return 0;
}

void __exit pcd_exit(void)
{
	int temp;
	//kfree(Dev_kernel_buffer);
	for(temp=0; temp<4; temp++)
	{
		device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number+temp);
		cdev_del(&pcdrv_data.pcdev_data[temp].cdev);
	}

	class_destroy(pcdrv_data.class_pcd);

/*cdev_del:
	temp = i;
	for(; temp>=0; temp--)
		cdev_del(&pcdrv_data.pcdev_data[temp].cdev);
*/
	unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);
	
	pr_info("Kernel Module Removed Successfully...\n");
}


module_init(pcd_driver_init);
module_exit(pcd_exit);

MODULE_LICENSE("GPL");
