#ifndef __MY_H__
#define __MY_H__

#define LED1_ON _IO('l',1)
#define LED1_OFF _IO('l',2)
#define LED2_ON _IO('l',3)
#define LED2_OFF _IO('l',4)
#define LED3_ON _IO('l',5)
#define LED3_OFF _IO('l',6)
#define FAN_ON _IO('t',1)
#define FAN_OFF _IO('t',2)
#define MOTOR_ON _IO('t',3)
#define MOTOR_OFF _IO('t',4)
#define BUZZER_ON _IO('t',5)
#define BUZZER_OFF _IO('t',6)
#define GET_TMP _IOR('i',1,int)
#define GET_HUM _IOR('i',2,int)
#define SEG_DAT  _IOW('s',2,int[2])
#define SEG_DATPOINT  _IOW('s',3,int[2])
#define GET_CMD_SIZE(cmd)  ((cmd>>16)&(0x3fff))
#define LED1_CHANGE _IO('c',1)
#define LED2_CHANGE _IO('c',2)
#define LED3_CHANGE _IO('c',3)
#endif
