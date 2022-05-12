#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include "my.h"
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>

#define MNAME "mygpio"
struct cdev* cdev;
unsigned int devno;
const int count=6;
struct class *cls;
struct device *dev;
struct device_node* node;
struct gpio_desc*  gd[6];
char *name[6] = {"led1","led2","led3","fan","motor","buzzer"};
int myopen(struct inode * inode, struct file * file)
{
	return 0;
}
int myrelease(struct inode * inode, struct file * file)
{
	gpiod_set_value(gd[0],0);
	gpiod_set_value(gd[1],0);
	gpiod_set_value(gd[2],0);
	gpiod_set_value(gd[3],0);
	gpiod_set_value(gd[4],0);
	gpiod_set_value(gd[5],0);
	return 0;
}
long myioctl(struct file *file,unsigned int cmd, unsigned long args)
{
	switch(cmd){
			case LED1_ON:gpiod_set_value(gd[0],1);break;
			case LED1_OFF:gpiod_set_value(gd[0],0);break;
			case LED2_ON:gpiod_set_value(gd[1],1);break;
			case LED2_OFF:gpiod_set_value(gd[1],0);break;
			case LED3_ON:gpiod_set_value(gd[2],1);break;
			case LED3_OFF:gpiod_set_value(gd[2],0);break;
			case FAN_ON:gpiod_set_value(gd[3],1);break;
			case FAN_OFF:gpiod_set_value(gd[3],0);break;
			case MOTOR_ON:gpiod_set_value(gd[4],1);break;
			case MOTOR_OFF:gpiod_set_value(gd[4],0);break;
			case BUZZER_ON:gpiod_set_value(gd[5],1);break;
			case BUZZER_OFF:gpiod_set_value(gd[5],0);break;
			case LED1_CHANGE:gpiod_set_value(gd[0],!(gpiod_get_value(gd[0])));break;
			case LED2_CHANGE:gpiod_set_value(gd[1],!(gpiod_get_value(gd[1])));break;
			case LED3_CHANGE:gpiod_set_value(gd[2],!(gpiod_get_value(gd[2])));break;
		}
	return 0;
}
const struct file_operations fops={
	.open=myopen,
	.release=myrelease,
	.unlocked_ioctl = myioctl,
};

int myprobe(struct platform_device * pdev)
{
	int ret,i;
	cdev=cdev_alloc();
	if(cdev==NULL)
	{
		printk(KERN_ERR "cdev_alloc error\n");
		ret = -ENOMEM;
		goto error1;
	}
	cdev_init(cdev,&fops);
	ret=alloc_chrdev_region(&devno,0,1,MNAME);
	if(ret)
	{
		printk(KERN_ERR "alloc_chrdev_region error\n");
		goto error2;
	}
	ret=cdev_add(cdev,devno,1);
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

	//µÆºÍÆäËû
	node=pdev->dev.of_node;
	for(i=0;i<count;i++)
	{
		gd[i]=gpiod_get_from_of_node(node,name[i],0,GPIOD_OUT_LOW,NULL);
		if(IS_ERR(gd[i])){
			printk(KERN_ERR "gpiod_get_from_of_node error\n");
			ret=PTR_ERR(gd[i]);
			goto error6;
		}
		gpiod_direction_output(gd[i],0);
	}
	printk(KERN_ERR "gpio_init\n");
	return 0;
error6:
	for(--i;i>=0;i++){
		gpiod_set_value(gd[i],0);
		gpiod_put(gd[i]);
	}
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
int myremove(struct platform_device * pdev)
{
	int i;
	device_destroy(cls,devno);
	class_destroy(cls);
	cdev_del(cdev);
	unregister_chrdev_region(devno,count);
	kfree(cdev);
	for(i=0;i<count;i++){
		gpiod_set_value(gd[i],0);
		gpiod_put(gd[i]);
	}
	printk(KERN_ERR "gpio_exit\n");
	return 0;
}
struct of_device_id	 ofmatch[]={
		{.compatible="hqyj,mywork",},
		{}
};
struct platform_driver pdrv={
	.probe=myprobe,
	.remove=myremove,
	.driver={

	.name = "work",
	.of_match_table=ofmatch,
	}
};
static int __init myinit(void)
{
	return platform_driver_register(&pdrv);
}
static void __exit myexit(void)
{
	platform_driver_unregister(&pdrv);

}
module_init(myinit);
module_exit(myexit);
MODULE_LICENSE("GPL");

