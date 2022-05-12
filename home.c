#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include "my.h"
typedef struct
{
	int fdi;
	int fds;
	int fdg;
	int fdk;
	float max;
	float min;
	float tmp;
	float hum;
	volatile char type;
	volatile char enable;
	volatile char sshow;
	volatile char tshow;
}home;
#define ERR_MSG(msg) do{\
	fprintf(stderr,"%d",__LINE__);\
	perror(msg);\
}while(0)
void do_spi(home* th)
{
	int arr[4],num;
	int brr[2];
	if(th->type=='h')
		num=(int)(th->hum*100);
	else
		num=(int)(th->tmp*100);
	arr[0]=num/1000;
	arr[1]=(num%1000)/100;
	arr[2]=(num%100)/10;
	arr[3]=num%10;
	for(int i=0;i<4;i++)
	{
		brr[0]=i;
		brr[1]=arr[i];
		if(i==1)
		{
			ioctl(th->fds,SEG_DATPOINT,brr);
		}else
		{
			ioctl(th->fds,SEG_DAT,brr);
		}
	}
}
void do_i2c(home* th)
{
	int tmp,hum,time=3000;
	int flagmin,flagmax,flag=1;
	if(th->tshow)
	{
	ioctl(th->fdi,GET_TMP,&tmp);
	ioctl(th->fdi,GET_HUM,&hum);
	th->tmp = (175.72*tmp/65536 - 46.85);
	th->hum = (125.0*hum/65536-6);
	if(th->enable=='y')
	{
		if((th->tmp>=th->max)&&(flagmax==0))
		{
			ioctl(th->fdg,FAN_ON);
			ioctl(th->fdg,BUZZER_ON);
			flagmax=1;
		}
		if((th->tmp<th->max)&&(flagmax==1))
		{
			ioctl(th->fdg,FAN_OFF);
			ioctl(th->fdg,BUZZER_OFF);
			flagmax=0;
		}
		if((th->tmp<=th->min)&&(flagmin==0))
		{
			ioctl(th->fdg,MOTOR_ON);
			ioctl(th->fdg,LED1_ON);
			ioctl(th->fdg,LED2_ON);
			ioctl(th->fdg,LED3_ON);
			flagmin=1;
		}
		if((th->tmp>th->min)&&(flagmin==1))
		{
			ioctl(th->fdg,MOTOR_OFF);
			ioctl(th->fdg,LED1_OFF);
			ioctl(th->fdg,LED2_OFF);
			ioctl(th->fdg,LED3_OFF);
			flagmin=0;	
		}
		printf("温度=%.2f,湿度=%.2f(按'e'退出)\n",th->tmp,th->hum);
	}else if(th->enable=='t')
	{
			ioctl(th->fdg,FAN_OFF);
			ioctl(th->fdg,BUZZER_OFF);
			ioctl(th->fdg,MOTOR_OFF);
			ioctl(th->fdg,LED1_OFF);
			ioctl(th->fdg,LED2_OFF);
			ioctl(th->fdg,LED3_OFF);
			flagmax=0;
			flagmin=0;	
			printf("退出报警模式\n");
			th->enable='n';
	}
	}
	while(time>0)
	{
		time--;
		if(th->sshow)
		{
			do_spi(th);
			if(!flag) flag=1;
		}else if(flag)
		{
			ioctl(th->fds,SEG_CLEAR);
			flag=0;
		}
	}
		
}
void do_key(home* th)
{
	int num;
	int count[3]={0,0,0};
	while(1)
	{
		read(th->fdk,&num,sizeof(num));
		count[num-1]++;
		switch(num)
		{
			case 1:ioctl(th->fdg,LED3_CHANGE);break;
			case 2:ioctl(th->fdg,LED2_CHANGE);break;
			case 3:ioctl(th->fdg,LED1_CHANGE);break;
		}
		printf("key%d=%d\n",num,count[num-1]);
	}
	
}
void do_mmenu(home* th)
{
		char c;
		while(1)
		{
			system("clear");
			printf("-------1.led1------------------------\n");
			printf("-------2.led2------------------------\n");
			printf("-------3.led3------------------------\n");
			printf("-------4.蜂鸣器----------------------\n");
			printf("-------5.马达------------------------\n");
			printf("-------6.风扇------------------------\n");
			printf("-------7.数码管----------------------\n");
			printf("-------8.温湿度传感器----------------\n");
			printf("-------9.退出------------------------\n");
			scanf("%c",&c);
			while(getchar()!='\n');
			switch(c)
			{
				case '1':ioctl(th->fdg,LED1_CHANGE);break;
				case '2':ioctl(th->fdg,LED2_CHANGE);break;
				case '3':ioctl(th->fdg,LED3_CHANGE);break;
				case '4':ioctl(th->fdg,BUZZER_CHANGE);break;
				case '5':ioctl(th->fdg,MOTOR_CHANGE);break;
				case '6':ioctl(th->fdg,FAN_CHANGE);break;
				case '7':th->sshow=~(th->sshow);break;
				case '8':th->tshow=~(th->tshow);break;
				case '9':return;
			}
		}
}
void* callback_0(void* arg)
{
	char c,t;
	home* th=(home*)arg;
	while(1)
	{
		if(th->enable=='n')
		{
			system("clear");
			printf("-------1.设置最高温度(目前为%.2f)--------\n",th->max);
			printf("-------2.设置最低温度(目前为%.2f)--------\n",th->min);
			printf("-------3.设置数码管模式(目前为%c)------\n",th->type);
			printf("-------4.启动温湿度报警----------------\n");
			printf("-------5.硬件控制----------------------\n");
			printf("-------6.退出--------------------------\n");
			scanf("%c",&c);
			while(getchar()!='\n');
			switch(c)
			{
				case '1':printf("请输入最高温度\n");
					scanf("%f",&th->max);
					while(getchar()!='\n');
					break;
				case '2':printf("请输入最低温度\n");
					scanf("%f",&th->min);
					while(getchar()!='\n');
					break;
				case '3':printf("请输入数码管模式(温度t/湿度h)\n");
					scanf("%c",&t);
					while(getchar()!='\n');
					if((t=='t')||(t=='h'))
						th->type=t;
					else
						printf("模式输入错误\n");
					break;
				case '4':th->enable='y';break;
				case '5':do_mmenu(th);break;
				case '6':printf("程序退出\n");
					exit(0);
					break;
			}
		}else if(th->enable=='y')
		{
			scanf("%c",&c);
			while(getchar()!='\n');
			if(c=='e')
				th->enable='t';
		}
	}
}
void* callback_t(void* arg)
{
	home* th=(home*)arg;
	while(1)
	{
		do_i2c(th);
	}
}
void* callback_k(void* arg)
{
	home* th=(home*)arg;
	do_key(th);
}
int main(int argc, const char *argv[])
{
	pthread_t tid,tidk,tidt;
	home th;
	if((th.fdi = open("/dev/myi2c",O_RDONLY)) < 0){
		perror("open i2c error");
		return -1;
	}
	if((th.fds = open("/dev/spi",O_RDWR)) < 0){
		perror("open spi error");
		return -1;
	}
	if((th.fdg = open("/dev/mygpio",O_RDWR)) < 0){
		perror("open gpio error");
		return -1;
	}
	if((th.fdk = open("/dev/mykey",O_RDONLY)) < 0){
		perror("open key error");
		return -1;
	}
	th.type='t';
	th.max=50;
	th.min=0;
	th.enable='n';
	th.tshow=0xff;
	th.sshow=0xff;
	if(pthread_create(&tid,NULL,callback_0,(void*)&th)!=0)
	{
		ERR_MSG("pthread_create_0");
		return -1;
	}
	if(pthread_create(&tidt,NULL,callback_t,(void*)&th)!=0)
	{
		ERR_MSG("pthread_create_t");
		return -1;
	}
	if(pthread_create(&tidk,NULL,callback_k,(void*)&th)!=0)
	{
		ERR_MSG("pthread_create_k");
		return -1;
	}
	pthread_join(tid,NULL);
	pthread_join(tidk,NULL);
	pthread_join(tidt,NULL);
	close(th.fdi);
	close(th.fds);
	close(th.fdg);
	close(th.fdk);
	return 0;
}
