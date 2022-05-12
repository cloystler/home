#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/poll.h>
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>


#define MNAME "mykey"
struct cdev* cdev;
unsigned int devno;
int count=1;
struct class *cls;
struct device *dev;
struct timer_list mytimer;
struct device_node* node;
struct gpio_desc*  gd[3];
struct gpio_desc *gdk[3];
char *irqname[3] = {"key1","key2","key3"};
unsigned int irqno[3];
struct work_struct work;
wait_queue_head_t wq;
int condition=0;
int num;
ssize_t myread(struct file * file, char __user * ubuf, size_t size, loff_t * offt)
{
	int ret;
	if(file->f_flags&O_NONBLOCK)
	{
		return -EINVAL;
	}
	else
	{
		ret=wait_event_interruptible(wq,condition);
		if(ret< 0){
			//printk("wait_event_interruptible error\n");
			return ret;
		}
	}
	if(size>sizeof(num)) size=sizeof(num);
	ret=copy_to_user(ubuf,&num,size);
	if(ret){
		printk("copy_to_user error\n");
		return -EIO;
	}
	condition=0;
	return 0;
}
int myopen(struct inode * inode, struct file * file)
{
	return 0;
}
int myrelease(struct inode * inode, struct file * file)
{
	return 0;
}
const struct file_operations fops={
	.open=myopen,
	.read=myread,
	.release=myrelease,
};
void mytimectl(struct timer_list *timer)
{
	schedule_work(&work);
}
irqreturn_t myirq(int irqno, void *dev)
{
	mod_timer(&mytimer,jiffies+1);
	return IRQ_HANDLED;
}
void mywork(struct work_struct *work)
{
	if(!gpiod_get_value(gd[0]))
	{
		num=1;
		condition=1;
		wake_up_interruptible(&wq);
	}
	if(!gpiod_get_value(gd[1]))
	{
		num=2;
		condition=1;
		wake_up_interruptible(&wq);
	}
	if(!gpiod_get_value(gd[2]))
	{
		num=3;
		condition=1;
		wake_up_interruptible(&wq);
	}
}
int myprobe(struct platform_device * pdev)
{
	int ret,i;
	cdev=cdev_alloc();
	if(cdev==NULL)
	{
		printk(KERN_ERR "gpiod_get_from_of_node error\n");
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
	//定时器,jiffies=内核时钟节拍数
	mytimer.expires=jiffies+1;
	timer_setup(&mytimer,mytimectl,0);
	add_timer(&mytimer);
	//中断
	node=pdev->dev.of_node;
	for(i=0;i<3;i++)
	{
		//解析软中断号
		irqno[i]=irq_of_parse_and_map(node,i);
		if(irqno[i] == 0){
			printk("irq_of_parse_and_map error\n");
			ret= -EAGAIN;
			goto error6;
		}
		//注册中断
		ret=request_irq(irqno[i],myirq,IRQF_TRIGGER_FALLING,irqname[i],NULL);
		if(ret){
			printk("request_irq error\n");
			goto error6;
		}
		//gpio_desc结构体
		gd[i] = gpio_to_desc(of_get_named_gpio(node,"keys",i));
		if(gd[i] == NULL){
			printk("gpio_to_desc error\n");
			ret=-EAGAIN;
			goto error6;
		}
	}
	//底半部
	INIT_WORK(&work,mywork);
	//阻塞
	init_waitqueue_head(&wq);
	printk(KERN_ERR "key_init\n");
	return 0;
error6:
	for(--i;i>0;i--)
	{
		free_irq(irqno[i],NULL);
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
	for(i=0;i<3;i++)
	{
		free_irq(irqno[i],NULL);
	}
	cancel_work_sync(&work);
	printk(KERN_ERR "key_exit\n");
	return 0;
}
struct of_device_id	 ofmatch[]={
		{.compatible="hqyj,mykeys",},
		{}
};
struct platform_driver pdrv={
	.probe=myprobe,
	.remove=myremove,
	.driver={
	.name = "duangduangduang",
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
