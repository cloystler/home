#include <linux/init.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include "my.h"

#define MNAME "spi"
struct cdev* cdev;
unsigned int devno;
const int count=1;
struct class *cls;
struct device *dev;
struct spi_device *gspi;
u8 code1[] = {
	0x3f, //--0
	0x06, //--1
	0x5b, //--2
	0x4f, //--3
	0x66, //--4
	0x6d, //--5
	0x7d, //--6
	0x07, //--7
	0x7f, //--8
	0x6f, //--9
	0x77, //--A
	0x7c, //--b
	0x39, //--c
	0x5e, //--d
	0x79, //--e
	0x71, //--f
};
u8 code2[] = {
	0xBF, //--0
	0x86, //--1
	0xDB, //--2
	0xCF, //--3
	0xE6, //--4
	0xED, //--5
	0xFD, //--6
	0x87, //--7
	0xFF, //--8
	0xEF, //--9
	0xF7, //--A
	0xFC, //--b
	0xB9, //--c
	0xDE, //--d
	0xF9, //--e
	0xF1, //--f
};

u8 which[] = {
	0x1, //sg0
	0x2, //sg1
	0x4, //sg2
	0x8, //sg3
};
int myopen(struct inode *inode, struct file *file)
{
	return 0;
}
int myclose(struct inode *inode, struct file *file)
{
	//ÕÀ≥ˆ∫ÛÕ£÷πœ‘ æ
	u8 buf[2] = {0xf,0x0};
	spi_write(gspi,buf,ARRAY_SIZE(buf));
	return 0;
}
long myioctl(struct file *file,unsigned int cmd, unsigned long args)
{
	u8 buf[2];
	int arr[2],ret;
	//spi_write–¥£¨spi_read∂¡£¨spi_write_then_read∂¡–¥
	switch(cmd){
		case SEG_DAT:
			ret = copy_from_user(arr,(void *)args,GET_CMD_SIZE(SEG_DAT));
			if(ret){
				printk(KERN_ERR "copy_from_user error\n");
				return -EINVAL;
			}
			buf[0]=which[arr[0]];
			buf[1]=code1[arr[1]];
			spi_write(gspi,buf,ARRAY_SIZE(buf));
			break;
		case SEG_DATPOINT:
			ret = copy_from_user(arr,(void *)args,GET_CMD_SIZE(SEG_DATPOINT));
			if(ret){
				printk(KERN_ERR "copy_from_user error\n");
				return -EINVAL;
			}
			buf[0]=which[arr[0]];
			buf[1]=code2[arr[1]];
			spi_write(gspi,buf,ARRAY_SIZE(buf));
			break;
		default: printk("ioctl error\n");
				break;
	}
	return 0;
}
const struct file_operations fops = {
	.open = myopen,
	.release = myclose,
	.unlocked_ioctl = myioctl,
};
int m74hc595_probe(struct spi_device *spi)
{
	int ret;
	u8 buf[2] = {0xf,0x0};
	gspi = spi;
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
	spi_write(gspi,buf,ARRAY_SIZE(buf));
	printk(KERN_ERR "spi_init\n");
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
int m74hc595_remove(struct spi_device *spi)
{
	u8 buf[2] = {0xf,0x0};
	spi_write(gspi,buf,ARRAY_SIZE(buf));
	device_destroy(cls,devno);
	class_destroy(cls);
	cdev_del(cdev);
	unregister_chrdev_region(devno,count);
	kfree(cdev);
	printk(KERN_ERR "spi_exit\n");
	return 0;
}
const struct of_device_id ofmatch[] = {
	{.compatible = "hqyj,m74hc595",},
	{}
};
MODULE_DEVICE_TABLE(of,ofmatch);

struct spi_driver m74hc595 = {
	.probe = m74hc595_probe,
	.remove = m74hc595_remove,
	.driver = {
		.name = "myspi",
		.of_match_table = ofmatch,
	}
};
module_spi_driver(m74hc595);
MODULE_LICENSE("GPL");

