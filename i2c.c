#include <linux/init.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include "my.h"
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define MNAME "myi2c"
struct cdev* cdev;
unsigned int devno;
const int count=1;
struct class *cls;
struct device *dev;

char kbuf[128] = {0};
struct i2c_client *gclient;
int i2c_read_serial_version(unsigned short reg)
{
	int ret;
	char r_buf[] = {(reg>>8)&0xff,(reg&0xff)};
	char val;
	struct i2c_msg r_msg[] = {
		[0] = {
			.addr = gclient->addr,
			.flags =0,
			.len = 2,
			.buf = r_buf,
		},
		[1] = {
			.addr = gclient->addr,
			.flags =1,
			.len = 1,
			.buf = &val,
		},
	};

	ret = i2c_transfer(gclient->adapter,r_msg,ARRAY_SIZE(r_msg));
	if(ret != ARRAY_SIZE(r_msg)){
		printk("i2c transfer error\n");
		return -EAGAIN;
	}

	return val;
}

int i2c_read_tmp_hum(unsigned char reg)
{
	int ret;
	char r_buf[] = {reg};
	unsigned short val;
	struct i2c_msg r_msg[] = {
		[0] = {
			.addr = gclient->addr,
			.flags =0,
			.len = 1,
			.buf = r_buf,
		},
		[1] = {
			.addr = gclient->addr,
			.flags =1,
			.len = 2,
			.buf = (__u8*)&val,
		},
	};

	ret = i2c_transfer(gclient->adapter,r_msg,ARRAY_SIZE(r_msg));
	if(ret != ARRAY_SIZE(r_msg)){
		printk("i2c transfer error\n");
		return -EAGAIN;
	}

	return val>>8 | val<<8;
}
int mycdev_open(struct inode *inode, struct file *file)
{
	return 0;
}
int mycdev_close(struct inode *inode, struct file *file)
{
	return 0;
}
long mycdev_ioctl(struct file *file,
	unsigned int cmd, unsigned long args)
{
	int ret,data;

	switch(cmd){
		case GET_TMP:
			data = i2c_read_tmp_hum(0xe3);
			if(data < 0){
				printk("i2c read tmp error\n");
				return -EINVAL;
			}
			break;
		case GET_HUM:
			data = i2c_read_tmp_hum(0xe5);
			if(data < 0){
				printk("i2c read hum error\n");
				return -EINVAL;
			}
			break;
	}

	data = data & 0xffff;
	ret = copy_to_user((void *)args,(void *)&data,sizeof(int));
	if(ret){
		printk("copy data to user error\n");
		return -EIO;
	}

	return 0;
}
const struct file_operations fops = {
	.open = mycdev_open,
	.unlocked_ioctl = mycdev_ioctl,
	.release = mycdev_close,
};
int myprobe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	gclient = client;
	cdev=cdev_alloc();
	if(cdev==NULL)
	{
		printk(KERN_ERR "cdev_alloc error\n");
		ret = -ENOMEM;
		goto error1;
	}
	cdev_init(cdev,&fops);
	ret=alloc_chrdev_region(&devno,0,count,MNAME);
	if(ret)
	{
		printk(KERN_ERR "alloc_chrdev_region error\n");
		goto error2;
	}
	ret=cdev_add(cdev,devno,count);
	if(ret)
	{
		printk(KERN_ERR "cdev_add error\n");
		goto error3;
	}
	cls=class_create(THIS_MODULE,MNAME);
	if(IS_ERR(cls))
	{
		printk(KERN_ERR "class_create error\n");
		ret=PTR_ERR(cls);
		goto error4;
	}
	dev=device_create(cls,NULL,devno,NULL,MNAME);
	if(IS_ERR(dev))
	{
		printk(KERN_ERR "device_create error\n");
		ret=PTR_ERR(dev);
		goto error5;
	}
	printk(KERN_ERR "i2c_init\n");
	return 0;
error5:
	class_destroy(cls);
error4:
	cdev_del(cdev);
error3:
	unregister_chrdev_region(devno,count);
error2:
	kfree(cdev);
error1:
	return ret;
}
int myremove(struct i2c_client *client)
{
	device_destroy(cls,devno);
	class_destroy(cls);
	cdev_del(cdev);
	unregister_chrdev_region(devno,count);
	kfree(cdev);
	printk(KERN_ERR "i2c_exit\n");
	return 0;
}
const struct of_device_id ofmatch[] = {
	{.compatible = "hqyj,si7006",},
	{}
};
MODULE_DEVICE_TABLE(of,ofmatch);//ÈÈ²å°Î
struct i2c_driver si7006 = {
	.probe = myprobe,
	.remove = myremove,
	.driver = {
		.name = "my i2c",
		.of_match_table = ofmatch,
	}
};

module_i2c_driver(si7006);
MODULE_LICENSE("GPL");

