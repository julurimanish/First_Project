#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/init.h>
#include<linux/fs.h>
//#include<linux/slab.h>
#include<linux/uaccess.h>
#include<linux/kdev_t.h>
#include<linux/device.h>
#include<linux/err.h>
#include<linux/cdev.h>

#define MEM_SIZE_MAX_PCDEV1  1024
#define MEM_SIZE_MAX_PCDEV2  512
#define MEM_SIZE_MAX_PCDEV3  1024
#define MEM_SIZE_MAX_PCDEV4  512
#define NO_OF_DEVICES 4


static int __init start(void);
static void __exit end(void);
static int etx_open(struct inode *inode,struct file *file);
static int etx_release(struct inode *inode,struct file *file);
ssize_t pcd_read(struct file *flip,char __user *buf,size_t count,loff_t *f_pos);
ssize_t pcd_write(struct file *flip,const char __user *buf,size_t count,loff_t *f_pos);

loff_t pcd_lseek(struct file *filp,loff_t offset,int whence );


/* pseudo device's memory */
char device_buffer_pcdev1[MEM_SIZE_MAX_PCDEV1];
char device_buffer_pcdev2[MEM_SIZE_MAX_PCDEV2];
char device_buffer_pcdev3[MEM_SIZE_MAX_PCDEV3];
char device_buffer_pcdev4[MEM_SIZE_MAX_PCDEV4];

/*Device private data structure */

struct pcdev_private_data
{
    char *buffer;
    unsigned size;
    const char *serial_number;
    int perm;
    struct cdev cdev;
};


/*Driver private data structure */
struct pcdrv_private_data
{
    int total_devices;
    dev_t device_number;
    struct pcdev_private_data pcdev_data[NO_OF_DEVICES];
     struct class *class_pcd;
     struct device *device_pcd;

};


static struct file_operations fops=
{
	.owner=THIS_MODULE,
	.read=pcd_read,
	.write=pcd_write,
	.open=etx_open,
	.release=etx_release,
	.llseek = pcd_lseek,
};



#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR   0x11

struct pcdrv_private_data pcdrv_data = 
{
           .total_devices = NO_OF_DEVICES,
           .pcdev_data = {

                         [0] = {
                                      .buffer = device_buffer_pcdev1,
                                      .size = MEM_SIZE_MAX_PCDEV1,
                                      .serial_number = "PCDEV1XYZ123",
                                      .perm = O_RDONLY
                          },

			[1] = {
                                     .buffer = device_buffer_pcdev2,
                                     .size = MEM_SIZE_MAX_PCDEV2,
                                     .serial_number = "PCDEV2XYZ123",
                                     .perm = O_WRONLY 
                               },

                          [2] = {
                                     .buffer = device_buffer_pcdev3,
                                     .size = MEM_SIZE_MAX_PCDEV3,
                                     .serial_number = "PCDEV3XYZ123",
                                     .perm = O_RDWR 
                                },

                            [3] = {
                                      .buffer = device_buffer_pcdev4,
                                      .size = MEM_SIZE_MAX_PCDEV4,
                                      .serial_number = "PCDEV4XYZ123",
                                      .perm = O_RDWR 
                                   }

             }

};
																	
int check_permission(int dev_perm,int acc_mode)
{
  if(acc_mode == O_RDWR)
	{
		return 0;
	}

	if((acc_mode == O_RDONLY))
{
	return 0;
}

	if((acc_mode== O_WRONLY))
{
	return 0;
}

return EPERM;


}

static int etx_open(struct inode *inode,struct file *file)
{
	int ret ;
	int minor_m;
	struct pcdev_private_data *pcdev_data;
	minor_m = MINOR(inode->i_rdev);
	pr_info("minor access = %d\n",minor_m);

	//file->private_data = pcdev_data;

	pcdev_data = container_of(inode->i_cdev,struct pcdev_private_data,cdev);

	file->private_data = pcdev_data;


ret = check_permission(pcdev_data->perm,file->f_mode);

(!ret)? pr_info("sucessfully opened\n"):pr_info("unable to open\n");
	return ret;
}

static int etx_release(struct inode *inode,struct file *file)
{
	printk("successfully closed a file\n");
	return 0;
}

ssize_t pcd_read(struct file *filp,char __user *buf,size_t count,loff_t *f_pos)
{ 
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;


	pr_info("no of bytes requested for read:%zu\n",count);
	pr_info("currebt file position:%lld\n",*f_pos);
//	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	int max_size = pcdev_data->size;



	if((*f_pos+count)>max_size)
	{
		count= max_size - *f_pos;
	}
	if(copy_to_user(buf,pcdev_data->buffer+(*f_pos),count))
	{
		return EFAULT;
	}
	*f_pos += count;
	pr_info("number of bytes successfully read :%zu\n",count);
	pr_info("updated file postion : %lld\n",*f_pos);
	  return count;

}




ssize_t pcd_write(struct file *filp,const char __user *buf,size_t count,loff_t *f_pos)
{

	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	int max_size = pcdev_data->size;

  	pr_info("no of bytes requested for write:%zu\n",count);
	pr_info("currebt file position:%lld\n",*f_pos);

	if((*f_pos+count)> max_size)
	{
		count= max_size - *f_pos;
	}

	if(!count)
	{
		return ENOMEM;
	}

	if(copy_from_user(pcdev_data->buffer+(*f_pos),buf,count))
	{
		return EFAULT;
	}
	*f_pos += count;
	pr_info("number of bytes successfully read :%zu\n",count);
	pr_info("updated file postion : %lld\n",*f_pos);
	return count;
	}


loff_t pcd_lseek(struct file *filp,loff_t offset,int whence)
{

	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;

	int max_size = pcdev_data->size;




	loff_t temp;

	pr_info("lseek requested\n");
	switch(whence)
	{
		case SEEK_SET:
			if((offset > max_size) || (offset <0))
			{
				return -EINVAL;
			}
			filp->f_pos = offset;
			break;

		case SEEK_CUR:
			temp = filp->f_pos + offset;
			if((temp > max_size) || (temp <0))
			{
				return -EINVAL;
			}
			filp->f_pos = temp;
			break;
			
		case SEEK_END:
			temp = max_size + offset;
			if((temp > max_size) || (temp <0))
			{
				return -EINVAL;
			}
			filp->f_pos = temp;
			break;
		default:
			return EINVAL;
	}
	return filp->f_pos;
}






static int __init start(void)
{
     int ret;
     int i;

	ret = alloc_chrdev_region(&pcdrv_data.device_number,0,NO_OF_DEVICES,"pcdevs");
	if(ret<0)
	{
		printk("failed to allocate\n");
		goto out;
	}
	for(i=0;i<NO_OF_DEVICES;i++)
{
	pr_info("device bumber <major>:<minor> = %d:%d\n",MAJOR(pcdrv_data.device_number+i),MINOR(pcdrv_data.device_number+i));
}


pcdrv_data.class_pcd = class_create(THIS_MODULE,"pcd_class");
if(IS_ERR(pcdrv_data.class_pcd)){
       pr_err("Class creation failed\n");
       ret = PTR_ERR(pcdrv_data.class_pcd);
      goto unreg_chrdev;
}

for(i=0;i<NO_OF_DEVICES;i++){
               pr_info("Device number <major>:<minor> = %d:%d\n",MAJOR(pcdrv_data.device_number+i),MINOR(pcdrv_data.device_number+i));

      /*Initialize the cdev structure with fops*/
      cdev_init(&pcdrv_data.pcdev_data[i].cdev,&fops);

     /* Register a device (cdev structure) with VFS */
     pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
     ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev,pcdrv_data.device_number+i,1);
     if(ret < 0){
         pr_err("Cdev add failed\n");
         goto cdev_del;
      }

pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd,NULL,pcdrv_data.device_number+i,NULL,"pcdrv-%d",i);
if(IS_ERR(pcdrv_data.device_pcd))
{
	pr_err("device creation failed\n");
ret = PTR_ERR(pcdrv_data.device_pcd);
goto class_del;
}
}

pr_info("Module init was successful\n");

          return 0;

cdev_del:
class_del:
	for(;i>=0;i--)
{
	device_destroy(pcdrv_data.class_pcd,pcdrv_data.device_number+i);
	cdev_del(&pcdrv_data.pcdev_data[i].cdev);
}
class_destroy(pcdrv_data.class_pcd);

unreg_chrdev:
          unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES);
out:
          pr_info("Module insertion failed\n");
          return ret;


	}

	static void __exit end(void)
	{
		//kfree(device_buffer);
int i;
	for(i=0;i<=NO_OF_DEVICES;i++)
{
	device_destroy(pcdrv_data.class_pcd,pcdrv_data.device_number+i);
	cdev_del(&pcdrv_data.pcdev_data[i].cdev);
}
class_destroy(pcdrv_data.class_pcd);

          unregister_chrdev_region(pcdrv_data.device_number,NO_OF_DEVICES);
          pr_info("Module insertion failed\n");


			
}
module_init(start);
module_exit(end);

MODULE_LICENSE("GPL");

