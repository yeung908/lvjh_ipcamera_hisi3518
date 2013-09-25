#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/poll.h>
#include <linux/interrupt.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/timer.h>
#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/ioctl.h>
#include <asm/system.h>
#include <asm/delay.h>
#include <linux/delay.h>
#include <linux/signal.h>



#include "tv_gpio.h"

/*
报警输出：   ALM_OUT1     (OUTPUT) （GPIO5_1）该引脚输出常低，当有报警输入信号时候，输出高电平报警信号
报警输入：   ALM_IN1      （INPUT） （GPIO5_2）该引脚输入常高，输入为低时为报警输入信号
RS4585收发： RS485_T/R     (OUTPUT) （GPIO5_3）该引脚输出低RS485为收，输出为高RS485为发
红外Ircut自动切换：IR-CUT  （INPUT）       （GPIO5_4）该引脚切换高低电平需GPIO5_0输入电平高低指示，GPIO5_0输入为高时候GPIO5_4输出高；GPIO5_0输入为低时候GPIO5_4输出低。
一键复位：    PB_RST       （INPUT）    （GPIO5_5）该引脚输入常高，当输出为低5S时间则软件配置恢复出厂模式
系统指示灯：  SYSTEM-RUN   (OUTPUT)   （GPIO5_6）该引脚输出方波，2S切换一次高低电平
视频AD复位：/RST_AV       （OUTPUT）  （GPIO5_7）该引脚在主机上电后输出400ms的低电平，后输出高电平。
USB_HUB复位：RST_USBHUB（OUTPUT）  （GPIO6_2）该引脚在主机上电后输出400ms的低电平，后输出高电平。   
PHY芯片复位：/RST_PHY    （OUTPUT）   （GPIO6_3）该引脚在主机上电后输出400ms的低电平，后输出高电平。
/WDG_RST：看门狗输出电平:100ms的方波信号

IC2地址芯片：
PCF8563T：Write : 0xA2;   Read  : 0xA3
TLV320AIC3104IRHBR：0x18
SENSER：图像传感器 0X90(Default)
*/

typedef struct GPIO_device_st 
{
	int irq;                   /* IRQ for device */
} GPIO_device;

GPIO_device GPIO_dev =  
{
	6                   			/* IRQ for device */
};
static volatile int ev_press = 0;   //add code by zhangj,limit swicth ev_press flag
static volatile int limit_num = 0;  //定时器去抖用
static volatile int remove_shake_number = 0;
static volatile int limit_num_temp;
static pid_t pid_limit_switch_task = -1;
#if 1
static struct timer_list H_ttimer;

static enum{reset,clockwise,anticlockwise} hor_run_flag = reset,ver_run_flag=reset ;

#endif

static int g_probe_in_status = 0;
static int g_probe_out_status = 0;
static int g_default_param_flag = 0;

static int g_ir_cut_status = 0;
static int g_alramD0 = 0;
static int g_alramD1 = 0;
static int g_alramD2 = 0;
static int g_alramD3 = 0;

static int local_speed =3;
static int ver_local_speed =3;



//static int timer_counter = 0;
static unsigned int hor_counter = 0;
static unsigned int ver_counter = 0;

static  unsigned char hor_init_flag = 0;
static unsigned char ver_init_flag = 0;

static int hor_rotation_flag = 0;         //0:正常 1:巡航
static int ver_rotation_flag = 0;

static int goto_point_x = 0;
static int goto_point_y=0;

static int H_goto_start_flag=0;
static int V_goto_start_flag=0;
// GPIO5_0: IR_CUT_IN
// GPIO5_1: AO
// GPIO5_2: AI
// GPIO5_3: RS485
// GPIO5_4: IR_CUT_OUT
// GPIO5_5: RESET
// GPIO5_6: LED
// GPIO5_7: VAD_RST

// GPIO6_2: USB_RST
// GPIO6_3: PHY_RST

#define SYS_CTRL		IO_ADDRESS(0x101E0000 + 0x20)

#define HISILICON_SCTL_BASE 0x101E0000
#define TIMER3_BASE                0x101e3000 // 0x101e2000

#define TIMER3_LOAD               0x020  // 0x000
#define TIMER3_BGLOAD           0x038 // 0x018
#define TIMER3_CONTROL        0x028 // 0x008
#define TIMER3_INTCLR             0x02c// 0x00c

#define TIMER3_REG(x)	(TIMER3_BASE + (x))
#define TIMER3_readl(x)		readl(IO_ADDRESS(TIMER3_REG(x)))
#define TIMER3_writel(v,x)	        writel(v, IO_ADDRESS(TIMER3_REG(x)))

#define GPIO_1_BASE 	0x101E5000
#define GPIO_1_DIR 		IO_ADDRESS(GPIO_1_BASE + 0x400)
#define GPIO_2_BASE 	0x101E6000
#define GPIO_2_DIR 		IO_ADDRESS(GPIO_2_BASE + 0x400)
#define GPIO_5_BASE 	0x101F8000
#define GPIO_5_DIR 		IO_ADDRESS(GPIO_5_BASE + 0x400)
#define GPIO_6_BASE 	0x101F9000
#define GPIO_6_DIR 		IO_ADDRESS(GPIO_6_BASE + 0x400)
#define GPIO_7_BASE 	0x101FA000
#define GPIO_7_DIR 		IO_ADDRESS(GPIO_7_BASE + 0x400)

#define HOR_MAX_COOR 3954
#define VER_MAX_COOR 1079


#define HW_REG(reg) 	*((volatile unsigned int *)(reg))

#define IRQ				8
#define TIMER3_IRQ               5
/*
#define IR_CUT_IN		0
#define AO				1
#define AI				2
#define RS485			3
#define IR_CUT_OUT		4
#define RESET			5
#define LED				6
#define VAD_RST			7

#define USB_RST			2
#define PHY_RST			3
*/

#define IR_CUT_IN		0x01
#define AO					0x02
#define AI					0x04
#define RS485				0x08
#define IR_CUT_OUT	0x10
#define RESET			0x20
#define LED				0x40
#define VAD_RST			0x80


#define V_UP_LIMIT				0x80//0x20 //GPIO7_5
#define V_DOWN_LIMIT				0x20//0x80 //GPIO7_7

#define H_RIGHT_LIMIT			0x01  //GPIO6_0
#define H_LEFT_LIMIT				0x08  //GPIO6_3

#define ALRAM_D0						0x08
#define ALRAM_D1						0x10
#define ALRAM_D2						0x20
#define ALRAM_D3						0x40
#define ALRAM_GARRISON			0x80
#define ALRAM_OUT						0x80



#define HOR_MOTOR      0x0f
#define VER_MOTOR      0xf0



#define STEPEER_SPEED   HZ/2048



static unsigned char clock_wise[]={0x09,0x08,0x0C,0x04,0x06,0x02,0x03,0x01}; //反转表格
static unsigned char anti_clock_wise[]={0x01,0x03,0x02,0x06,0x04,0x0c,0x08,0x09}; 



void gpio_reset_motor_vertical(void)
{
	HW_REG(IO_ADDRESS(GPIO_6_BASE+(VER_MOTOR<<2))) = 0x00;//0
	return ;
}


void gpio_reset_motor_horizen(void)
{
	HW_REG(IO_ADDRESS(GPIO_7_BASE+(HOR_MOTOR<<2))) = 0x00;
	return ;
}

static DECLARE_WAIT_QUEUE_HEAD(limit_waitq);
/*去抖设计:检测到输入后每隔20ms检测一次，检测3次，若检测到一次，则认为检测到*/
static void H_time_tick(unsigned long arg)
{
	unsigned char value = 0;
	switch(limit_num){
		case limit_right:
			value = HW_REG(IO_ADDRESS(GPIO_6_BASE+(H_RIGHT_LIMIT<<2)));	
			if (!(value & (H_RIGHT_LIMIT)))
			{
				limit_num_temp = limit_num;
				printk("H_RIGHT_LIMIT(RIHGT):ev_press:%d!\n",limit_num_temp);
			}
		break;
	
		case limit_left:	
			value = HW_REG(IO_ADDRESS(GPIO_6_BASE+(H_LEFT_LIMIT<<2)));
			if (!(value & (H_LEFT_LIMIT)))
			{
				limit_num_temp = limit_num;
				printk("H_LEFT_LIMIT(LEFT):ev_press:%d\n",limit_num_temp);
			}
		break;
		
		case limit_up:
			value = HW_REG(IO_ADDRESS(GPIO_7_BASE+(V_UP_LIMIT<<2)));
			if (!(value & (V_UP_LIMIT)))
			{
				limit_num_temp = limit_num;
				printk("V_UP_LIMIT(UP):ev_press:%d\n",limit_num_temp);
			}
		break;

		case limit_down:
			value = HW_REG(IO_ADDRESS(GPIO_7_BASE+(V_DOWN_LIMIT<<2)));
			if (!(value & (V_DOWN_LIMIT)))
			{
				limit_num_temp = limit_num;
				printk("V_DOWN_LIMIT(DOWN):ev_press:%d\n",limit_num_temp);
			}
		break;
		default:
			break;
	}
	remove_shake_number++;
	if(3 <=  remove_shake_number)
	{
		remove_shake_number = 0;       //清零计数
		limit_num = 0;                                //用于确定检测哪个引脚的电平
		ev_press = limit_num_temp;
		if(ev_press != 0){
			limit_num_temp = 0;
			wake_up_interruptible(&limit_waitq);
		}
	}
	else{
		H_ttimer.expires=jiffies+2;
		add_timer(&H_ttimer);
	}
	
}

void gpio_init(void)
{
	unsigned long regvalue = 0;	
 
         regvalue = HW_REG(SYS_CTRL);
//	printk("SYS_CTRL0: %x\n", regvalue);

/**********SET GPIO5_0 ~GPIO5_7  GPIO6.2 GPIO6.3******/ 
	regvalue |= 0x40;
	regvalue &= 0xFFFBFFFF;

/**********SET GPIO7_0 ~GPIO7_3 GPIO6_4 ~GPIO6_7   GPIO6.2 GPIO6.3******/ 
	regvalue |= 0x800008;
	regvalue &= 0xFFFFF7FF;

/**********SET GPIO2.3 GPIO2.4 GPIO2.5 GPIO2.6 GPIO2.7 GPIO1.7 GPIO2.0******/ 
	regvalue &= 0xFFFEDFFF; 
	regvalue |= 0x20000; 
	//regvalue |= 0x80008;


	HW_REG(SYS_CTRL) = regvalue;
//	printk("SYS_CTRL1: %x\n", regvalue);
	
	// Set GPIO_5 Direction
	regvalue = HW_REG(GPIO_5_DIR);
	regvalue = 0xDA;
	//regvalue &= 0xEF;
	HW_REG(GPIO_5_DIR) = regvalue;
	
	//HW_REG(IO_ADDRESS(GPIO_5_BASE+(LED<<2))) = LED;
	//HW_REG(IO_ADDRESS(GPIO_5_BASE+(LED<<2))) = (~LED);
	
	// Set Output Data
	HW_REG(IO_ADDRESS(GPIO_5_BASE+(AO<<2))) = (~AO);
	HW_REG(IO_ADDRESS(GPIO_5_BASE+(RS485<<2))) = (RS485);
	HW_REG(IO_ADDRESS(GPIO_5_BASE+(IR_CUT_OUT<<2))) = (IR_CUT_OUT);
	HW_REG(IO_ADDRESS(GPIO_5_BASE+(LED<<2))) = (LED);
	HW_REG(IO_ADDRESS(GPIO_5_BASE+(VAD_RST<<2))) = (~VAD_RST);


#if 0
	// IR_CUT_IN
	regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x404));
	HW_REG(IO_ADDRESS(GPIO_5_BASE+0x404)) &= (~IR_CUT_IN);
	regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x408));
	HW_REG(IO_ADDRESS(GPIO_5_BASE+0x408)) |= (IR_CUT_IN);
	regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x41C));
	HW_REG(IO_ADDRESS(GPIO_5_BASE+0x41C)) |= (IR_CUT_IN);
	regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x410));
	HW_REG(IO_ADDRESS(GPIO_5_BASE+0x410)) |= (IR_CUT_IN);
#endif


	// Set GPIO_2 Direction
	regvalue = HW_REG(GPIO_2_DIR);
	regvalue &= 0xFE;
	HW_REG(GPIO_2_DIR) = regvalue;
#if 1
	// IR_CUT_IN
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x404));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x404)) &= (~IR_CUT_IN);
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x408));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x408)) |= (IR_CUT_IN);
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C)) |= (IR_CUT_IN);
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410)) |= (IR_CUT_IN);
#endif

	
	// AI
	regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x404));
	HW_REG(IO_ADDRESS(GPIO_5_BASE+0x404)) &= (~AI);
	regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x408));
	HW_REG(IO_ADDRESS(GPIO_5_BASE+0x408)) |= (AI);
	regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x41C));
	HW_REG(IO_ADDRESS(GPIO_5_BASE+0x41C)) |= (AI);
	regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x410));
	HW_REG(IO_ADDRESS(GPIO_5_BASE+0x410)) |= (AI);

	// RESET
	regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x404));
	HW_REG(IO_ADDRESS(GPIO_5_BASE+0x404)) &= (~RESET);		// 中断触发寄存器:边沿触发
	regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x408));
	HW_REG(IO_ADDRESS(GPIO_5_BASE+0x408)) |= (RESET);			// 双沿触发中断寄存器:双边沿
	regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x41C));
	HW_REG(IO_ADDRESS(GPIO_5_BASE+0x41C)) |= (RESET);			// 触发中断条件寄存器:上升沿
	regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x410));
	HW_REG(IO_ADDRESS(GPIO_5_BASE+0x410)) |= (RESET);			// 中断屏蔽寄存器

	
	// Set GPIO_6 Direction
	regvalue = HW_REG(GPIO_6_DIR);
	regvalue &= 0xF6;
	regvalue |= 0xF0;
	HW_REG(GPIO_6_DIR) = regvalue;

	// Set Output Data

  //phy deal:

	HW_REG(IO_ADDRESS(GPIO_5_BASE+(VAD_RST<<2))) = (VAD_RST);

  //usb deal: first 4s high continue low




	// Set GPIO_7 Direction
#if 1
	regvalue = HW_REG(GPIO_7_DIR);
	//regvalue &= 0x00;
	regvalue |= 0x0F;
	HW_REG(GPIO_7_DIR) = regvalue;

/********SET MOTOR STEPPER INITIALIZE STATUS **************/
	HW_REG(IO_ADDRESS(GPIO_7_BASE+(HOR_MOTOR<<2))) = 0x00;//0
	HW_REG(IO_ADDRESS(GPIO_6_BASE+(VER_MOTOR<<2))) = 0x00;//0

#endif

/*****************************SET WIRENESS ALARM GPIO***************************/
#if 1
//SET GPIO2.3 GPIO2.4 GPIO2.5 GPIO2.6 INPUT MODE 
	regvalue  = HW_REG(GPIO_2_DIR);
	regvalue &= 0x87;
  //regvalue |= 0x4;
	HW_REG(GPIO_2_DIR) = regvalue;

//SET GPIO2.7 OUTPUT MODE 
	regvalue  = HW_REG(GPIO_2_DIR);
	regvalue |= 0x80;
	HW_REG(GPIO_2_DIR) = regvalue;
	HW_REG(IO_ADDRESS(GPIO_2_BASE+(ALRAM_OUT<<2))) = ALRAM_OUT;


	regvalue  = HW_REG(GPIO_1_DIR);
	regvalue |= 0X80;
	HW_REG(GPIO_1_DIR) = regvalue;
	HW_REG(IO_ADDRESS(GPIO_1_BASE+(ALRAM_GARRISON<<2))) = ~ALRAM_GARRISON;
#endif

#if 1
	//ALRAM_D0 
	#if 1
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x404));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x404)) &= (~ALRAM_D0);		// 中断触发寄存器:边沿触发
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x408));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x408)) |= (ALRAM_D0);			// 双沿触发中断寄存器:双边沿
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C)) |= (ALRAM_D0);			// 中断清除寄存器：清除中断
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410)) |= (ALRAM_D0);			// 中断屏蔽寄存器
	#endif 

	//ALRAM_D1
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x404));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x404)) &= (~ALRAM_D1);		// 中断触发寄存器:边沿触发
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x408));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x408)) |= (ALRAM_D1);			// 双沿触发中断寄存器:双边沿
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C)) |= (ALRAM_D1);			// 中断清除寄存器：清除中断
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410)) |= (ALRAM_D1);			// 中断屏蔽寄存器

	//ALRAM_D2
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x404));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x404)) &= (~ALRAM_D2);		// 中断触发寄存器:边沿触发
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x408));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x408)) |= (ALRAM_D2);			// 双沿触发中断寄存器:双边沿
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C)) |= (ALRAM_D2);			// 中断清除寄存器：清除中断
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410)) |= (ALRAM_D2);			// 中断屏蔽寄存器

	//ALRAM_D3 
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x404));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x404)) &= (~ALRAM_D3);		// 中断触发寄存器:边沿触发
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x408));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x408)) |= (ALRAM_D3);			// 双沿触发中断寄存器:双边沿
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C)) |= (ALRAM_D3);			// 中断清除寄存器：清除中断
	regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410));
	HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410)) |= (ALRAM_D3);			// 中断屏蔽寄存器

//Initialize GPIO2_BASE D0 D1 D2 D3
	HW_REG(IO_ADDRESS(GPIO_2_BASE+ALRAM_D0)) = (~ALRAM_D0);			// 中断屏蔽寄存器
	HW_REG(IO_ADDRESS(GPIO_2_BASE+ALRAM_D1)) = (~ALRAM_D1);			// 中断屏蔽寄存器
	HW_REG(IO_ADDRESS(GPIO_2_BASE+ALRAM_D2)) = (~ALRAM_D2);			// 中断屏蔽寄存器
	HW_REG(IO_ADDRESS(GPIO_2_BASE+ALRAM_D3)) = (~ALRAM_D3);			// 中断屏蔽寄存器
#endif 

#if 1
	//V_UP_LIMIT
	regvalue = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x404));
	HW_REG(IO_ADDRESS(GPIO_7_BASE+0x404)) &= (~V_UP_LIMIT);		         // 中断触发寄存器:边沿触发
	regvalue = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x40C));
	HW_REG(IO_ADDRESS(GPIO_7_BASE+0x40C)) &= (~V_UP_LIMIT);			//触发条件寄存器:下降沿
	regvalue = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x408));
	HW_REG(IO_ADDRESS(GPIO_7_BASE+0x408)) &= (~V_UP_LIMIT);			// 双沿触发中断寄存器:单边沿
	regvalue = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x41C));
	HW_REG(IO_ADDRESS(GPIO_7_BASE+0x41C)) |= (V_UP_LIMIT);			// 中断清除寄存器：清除中断
	regvalue = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x410));
	HW_REG(IO_ADDRESS(GPIO_7_BASE+0x410)) |= (V_UP_LIMIT);			// 中断屏蔽寄存器

	//V_DOWN_LIMIT
	regvalue = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x404));
	HW_REG(IO_ADDRESS(GPIO_7_BASE+0x404)) &= (~V_DOWN_LIMIT);		         // 中断触发寄存器:边沿触发
	regvalue = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x40C));
	HW_REG(IO_ADDRESS(GPIO_7_BASE+0x40C)) &= (~V_DOWN_LIMIT);			//触发条件寄存器:下降沿
	regvalue = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x408));
	HW_REG(IO_ADDRESS(GPIO_7_BASE+0x408))&= (~V_DOWN_LIMIT);			// 双沿触发中断寄存器:单边沿
	regvalue = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x41C));
	HW_REG(IO_ADDRESS(GPIO_7_BASE+0x41C)) |= (V_DOWN_LIMIT);			// 中断清除寄存器：清除中断
	regvalue = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x410));
	HW_REG(IO_ADDRESS(GPIO_7_BASE+0x410)) |= (V_DOWN_LIMIT);			// 中断屏蔽寄存器


	//H_RIGHT_LIMIT
	regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x404));
	HW_REG(IO_ADDRESS(GPIO_6_BASE+0x404)) &= (~H_RIGHT_LIMIT);		         // 中断触发寄存器:边沿触发
	regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x40C));
	HW_REG(IO_ADDRESS(GPIO_6_BASE+0x40C)) &= (~H_RIGHT_LIMIT);			//触发条件寄存器:下降沿
	regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x408));
	HW_REG(IO_ADDRESS(GPIO_6_BASE+0x408)) &= (~H_RIGHT_LIMIT);			// 双沿触发中断寄存器:单边沿
	regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x41C));
	HW_REG(IO_ADDRESS(GPIO_6_BASE+0x41C)) |= (H_RIGHT_LIMIT);			// 中断清除寄存器：清除中断
	regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x410));
	HW_REG(IO_ADDRESS(GPIO_6_BASE+0x410)) |= (H_RIGHT_LIMIT);			// 中断屏蔽寄存器


	//H_LEFT_LIMIT
	regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x404));
	HW_REG(IO_ADDRESS(GPIO_6_BASE+0x404)) &= (~H_LEFT_LIMIT);		         // 中断触发寄存器:边沿触发
	regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x40C));
	HW_REG(IO_ADDRESS(GPIO_6_BASE+0x40C)) &= (~H_LEFT_LIMIT);			//触发条件寄存器:下降沿
	regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x408));
	HW_REG(IO_ADDRESS(GPIO_6_BASE+0x408)) &= (~H_LEFT_LIMIT);			// 双沿触发中断寄存器:单边沿
	regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x41C));
	HW_REG(IO_ADDRESS(GPIO_6_BASE+0x41C)) |= (H_LEFT_LIMIT);			// 中断清除寄存器：清除中断
	regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x410));
	HW_REG(IO_ADDRESS(GPIO_6_BASE+0x410)) |= (H_LEFT_LIMIT);			// 中断屏蔽寄存器
#endif
}
static irqreturn_t TIMER3_inth(int irq, void *dev_id, struct pt_regs *reg)
{
	static unsigned char hor_times = 0,ver_times = 0;
	unsigned char value = 0;
	TIMER3_writel(0x00,TIMER3_INTCLR);
	
	/*if(ver_counter % 10 == 1||hor_counter % 10 == 1)
		printk("timer coord X:%u,Y:%u\n",hor_counter,ver_counter);*/
	ver_times++;
	if(ver_times ==  ver_local_speed){
		ver_times = 0;
		if(ver_run_flag != reset){
			HW_REG(IO_ADDRESS(GPIO_6_BASE+(VER_MOTOR<<2))) = clock_wise[ver_counter%8] << 4;
			if(ver_run_flag == clockwise){
				ver_counter++;
				if(ver_counter >= VER_MAX_COOR &&  ver_init_flag == 0){
					ver_counter = VER_MAX_COOR;
					if(ver_rotation_flag == 0)
						ver_run_flag = reset;
					else
						ver_run_flag = anticlockwise;
				}
			}
			else{
				ver_counter--;
				if(ver_init_flag){
					if(ver_counter == VER_MAX_COOR/2){
						ver_init_flag = 0;
						ver_run_flag = reset;
					}
				}
				if(ver_counter == 0  && ver_init_flag == 0){
					ver_run_flag = reset;
					if(ver_rotation_flag == 0)
						ver_run_flag = reset;
					else
						ver_run_flag = clockwise;
				}
			}
			if(V_goto_start_flag)
			{
				if(goto_point_y > 0)
					goto_point_y--;
				else if(goto_point_y < 0)
					goto_point_y++;
				else
				{
					V_goto_start_flag = 0;
			 		ver_run_flag = reset;
				}
			}
		}else{
			HW_REG(IO_ADDRESS(GPIO_6_BASE+(VER_MOTOR<<2))) = 0x00;
		}
	}
	hor_times ++;
	if(hor_times == local_speed){
		hor_times = 0;

		if(hor_run_flag != reset){
			HW_REG(IO_ADDRESS(GPIO_7_BASE+(HOR_MOTOR<<2))) = anti_clock_wise[hor_counter%8];
			if(hor_run_flag == anticlockwise){
				hor_counter++;
				if(hor_counter >= HOR_MAX_COOR && hor_init_flag == 0){
					hor_counter = HOR_MAX_COOR;
					if(hor_rotation_flag == 0)
						hor_run_flag = reset;
					else
						hor_run_flag = clockwise;
					
				}
			}
			else{                   
				hor_counter--;
				if(hor_init_flag){
					if(hor_counter == HOR_MAX_COOR/2){
						hor_init_flag = 0;
						hor_run_flag = reset;
						#if 1
						/**************************水平自检完成后开启垂直自检开关*****************************/
						ver_init_flag = 1;
						value = HW_REG(IO_ADDRESS(GPIO_7_BASE+(V_DOWN_LIMIT<<2)));
						if (!(value & (V_DOWN_LIMIT)))
						{
							ver_run_flag = clockwise;
						}else{
							ver_run_flag = anticlockwise;
						}
						#endif
						/*******************************************************/
					}
				}
				if(hor_counter == 0 && hor_init_flag == 0){
					if(hor_rotation_flag == 0)
						hor_run_flag = reset;
					else
						hor_run_flag = anticlockwise;
				}
			}
			if(H_goto_start_flag)
			{
				if(goto_point_x > 0)
					goto_point_x--;
				else if(goto_point_x < 0)
					goto_point_x++;
				else
				{
					H_goto_start_flag = 0;
			 		hor_run_flag = reset;
				}
			}
		}else{
			HW_REG(IO_ADDRESS(GPIO_7_BASE+(HOR_MOTOR<<2))) = 0x00;
		}
			
	}
	
	return IRQ_HANDLED;
}
static irqreturn_t probe_alarm_inth(int irq, void *dev_id, struct pt_regs *reg)
{
	int flag = 0;
	unsigned char value = 0;
	unsigned char regvalue = 0;
	
	// Read 
	value = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x418));
	
	if (value & (AI))
	{
		g_probe_in_status = 1;

		regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_5_BASE+0x410)) &= (~AI);

		regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x41C));
		HW_REG(IO_ADDRESS(GPIO_5_BASE+0x41C)) |= (AI);

		regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_5_BASE+0x410)) |= (AI);

		flag = 1;

		printk("Alarm In: %x(%x)!\n", value, value&0x04);
	}
	if (value & (RESET))
	{
		value = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x80));
			
		if (value & (RESET))
		{
			printk("RESET(UP): %x(%x)!\n", value, value&0x20);
			g_default_param_flag = 2;
		}
		else
		{
			printk("RESET(DOWN): %x(%x)!\n", value, value&0x20);
			g_default_param_flag = 1;
		}
		
		regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_5_BASE+0x410)) &= (~RESET);

		regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x41C));
		HW_REG(IO_ADDRESS(GPIO_5_BASE+0x41C)) |= (RESET);

		regvalue = HW_REG(IO_ADDRESS(GPIO_5_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_5_BASE+0x410)) |= (RESET);

		flag = 1;
	}


//GPIO_6_BASE 控制水平方向的电机中断H_RIGHT_LIMIT H_LEFT_LIMIT
#if 1
	value = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x418));
#if 1
	if (value & (H_RIGHT_LIMIT))
	{
		limit_num = limit_right;
		H_ttimer.expires=jiffies+4;
		add_timer(&H_ttimer);
		printk("--DRIVER--ZJ--IN INTERRUPT\n");
		regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x40C));
		HW_REG(IO_ADDRESS(GPIO_6_BASE+0x40C)) |= (~H_RIGHT_LIMIT);			// 双沿触发中断寄存器:双边沿

		regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_6_BASE+0x410)) &= (~H_RIGHT_LIMIT);

		regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x41C));
		HW_REG(IO_ADDRESS(GPIO_6_BASE+0x41C)) |= (H_RIGHT_LIMIT);

		regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_6_BASE+0x410)) |= (H_RIGHT_LIMIT);

		flag = 1;
	}


	if (value & (H_LEFT_LIMIT))
	{

		limit_num = limit_left;
		H_ttimer.expires=jiffies+4;
		add_timer(&H_ttimer);

		printk("--DRIVER--ZJ--IN INTERRUPT\n");
		regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_6_BASE+0x410)) &= (~H_LEFT_LIMIT);

		regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x41C));
		HW_REG(IO_ADDRESS(GPIO_6_BASE+0x41C)) |= (H_LEFT_LIMIT);

		regvalue = HW_REG(IO_ADDRESS(GPIO_6_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_6_BASE+0x410)) |= (H_LEFT_LIMIT);

		flag = 1;
	}
#endif
#endif

//GPIO_7_BASE 控制水平方向的电机中断V_UP_LIMIT V_DOWN_LIMIT
#if 1
	value = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x418));
	
	if (value & (V_UP_LIMIT))
	{
		limit_num = limit_up;
		H_ttimer.expires=jiffies+4;
		add_timer(&H_ttimer);
		printk("--DRIVER--ZJ--IN INTERRUPT\n");
		regvalue = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_7_BASE+0x410)) &= (~V_UP_LIMIT);

		regvalue = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x41C));
		HW_REG(IO_ADDRESS(GPIO_7_BASE+0x41C)) |= (V_UP_LIMIT);

		regvalue = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_7_BASE+0x410)) |= (V_UP_LIMIT);

		flag = 1;
	}

	if (value & (V_DOWN_LIMIT))
	{
		limit_num = limit_down;
		H_ttimer.expires=jiffies+4;
		add_timer(&H_ttimer);
		printk("--DRIVER--ZJ--IN INTERRUPT\n");
		regvalue = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_7_BASE+0x410)) &= (~V_DOWN_LIMIT);

		regvalue = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x41C));
		HW_REG(IO_ADDRESS(GPIO_7_BASE+0x41C)) |= (V_DOWN_LIMIT);

		regvalue = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_7_BASE+0x410)) |= (V_DOWN_LIMIT);

		flag = 1;
	}
#endif

//GPIO_2_BASE 无线报警
#if 1
	value = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x418));
	
	if (value & (ALRAM_D0))
	{
		value = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x80));
			
		if (value & (ALRAM_D0))
		{
			printk("ALARM_D0(UP): %x(%x)!\n", value, value&0x20);
			g_alramD0 = 2;
			
		}
		else
		{
			printk("ALARM_D0(DOWN): %x(%x)!\n", value, value&0x20);
			g_alramD0 = 1;
			
		}

		regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410)) &= (~ALRAM_D0);		// 中断触发寄存器:边沿触发
		regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C));
		HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C)) |= (ALRAM_D0);			// 中断清除寄存器：清除中断
		regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410)) |= (ALRAM_D0);			// 中断屏蔽寄存器
		
		flag = 1;
	}

	if (value & (ALRAM_D1))
	{
		value = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x80));
			
		if (value & (ALRAM_D1))
		{
			printk("ALARM_D1(UP): %x(%x)!\n", value, value&0x20);
			g_alramD1 = 2;
			
		}
		else
		{
			printk("ALARM_D1(DOWN): %x(%x)!\n", value, value&0x20);
			g_alramD1 = 1;
			
		}

		regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410)) &= (~ALRAM_D1);		// 中断触发寄存器:边沿触发
		regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C));
		HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C)) |= (ALRAM_D1);			// 中断清除寄存器：清除中断
		regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410)) |= (ALRAM_D1);			// 中断屏蔽寄存器
		
		flag = 1;
	}


	if (value & (ALRAM_D2))
	{
		value = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x80));
			
		if (value & (ALRAM_D2))
		{
			printk("ALARM_D2(UP): %x(%x)!\n", value, value&0x20);
			g_alramD2 = 2;
			
		}
		else
		{
			printk("ALARM_D2(DOWN): %x(%x)!\n", value, value&0x20);
			g_alramD2 = 1;
			
		}

		regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410)) &= (~ALRAM_D2);		// 中断触发寄存器:边沿触发
		regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C));
		HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C)) |= (ALRAM_D2);			// 中断清除寄存器：清除中断
		regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410)) |= (ALRAM_D2);			// 中断屏蔽寄存器
		
		flag = 1;
	}

	if (value & (ALRAM_D3))
	{
		value = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x80));
			
		if (value & (ALRAM_D3))
		{
			printk("ALARM_D3(UP): %x(%x)!\n", value, value&0x20);
			g_alramD3 = 2;
			
		}
		else
		{
			printk("ALARM_D3(DOWN): %x(%x)!\n", value, value&0x20);
			g_alramD3 = 1;
			
		}

		regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410)) &= (~ALRAM_D3);		// 中断触发寄存器:边沿触发
		regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C));
		HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C)) |= (ALRAM_D3);			// 中断清除寄存器：清除中断
		regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410)) |= (ALRAM_D3);			// 中断屏蔽寄存器
		
		flag = 1;
	}

#if 1
	if (value & (IR_CUT_IN))
	{
		value = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x80));
			
		if (value & (IR_CUT_IN))
		{
			//printk("IR_CUT_IN(UP): %x(%x)!\n", value, value&0x20);
			g_ir_cut_status = 2;
			printk("g_ir_cut_status = %d\n", g_ir_cut_status);
		
		}
		else
		{
		//	printk("IR_CUT_IN(DOWN): %x(%x)!\n", value, value&0x20);
			g_ir_cut_status = 1;
			printk("g_ir_cut_status = %d\n", g_ir_cut_status);

		}
		
		regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410)) &= (~IR_CUT_IN);

		regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C));
		HW_REG(IO_ADDRESS(GPIO_2_BASE+0x41C)) |= (IR_CUT_IN);

		regvalue = HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410));
		HW_REG(IO_ADDRESS(GPIO_2_BASE+0x410)) |= (IR_CUT_IN);

		flag = 1;
	}
#endif


	if (flag == 1)
	{
		return IRQ_HANDLED;
	}
	else
	{
		return IRQ_NONE;
	}
}

static int misc_open(struct inode *inode ,struct file *file)
{

	return 0;
}

static int misc_release(struct inode *inode ,struct file *file)
{
	return 0;
}

static ssize_t misc_read(struct file *file, char __user * buf, size_t count, loff_t * offset)
{
	int ret = -1;
	GPIO_CMD cmd;
	 unsigned int err;
	 unsigned char value = 0;
	 int limit_value_temp = 0;
	
	ret = copy_from_user(&cmd, (GPIO_CMD *)buf, count);
	if (ret)
	{
		return -1;
	}
	
	switch (cmd.opt)
	{
	case GPIO_GET_PROBE_IN_STATUS:
		cmd.param.probeInStatus = g_probe_in_status;
		copy_to_user((GPIO_CMD *)buf, &cmd, count);
		g_probe_in_status = 0;
		ret = 0;
		break;
		
	case GPIO_GET_RESET_STATUS:
		cmd.param.resetStatus = g_default_param_flag;
		ret = copy_to_user((GPIO_CMD *)buf, &cmd, count);
		if (g_default_param_flag == 2)
		{
			g_default_param_flag = 0;
		}
		ret = 0;
		break;

	case GPIO_GET_MOTOR_LIMIT_STATUS:

		break;
	case GPIO_GET_MOTOR_LIMIT_VALUE:
		value = HW_REG(IO_ADDRESS(GPIO_6_BASE+(H_RIGHT_LIMIT<<2)));	
		if (!(value & (H_RIGHT_LIMIT)))
		{
			limit_value_temp |=value_right;
		//	printk("Timeover (RIHGT)--value:%02x!\n",limit_value_temp);
		}
	
		value = HW_REG(IO_ADDRESS(GPIO_6_BASE+(H_LEFT_LIMIT<<2)));
		if (!(value & (H_LEFT_LIMIT)))
		{
			limit_value_temp |=value_left;
		//	printk("Timeover (LEFT)--value:%02x!\n",limit_value_temp);
		}

		value = HW_REG(IO_ADDRESS(GPIO_7_BASE+(V_UP_LIMIT<<2)));
		if (!(value & (V_UP_LIMIT)))
		{
			limit_value_temp |=value_up;
		//	printk("Timeover (UP)--value:%02x!\n",limit_value_temp);
		}

		value = HW_REG(IO_ADDRESS(GPIO_7_BASE+(V_DOWN_LIMIT<<2)));
		if (!(value & (V_DOWN_LIMIT)))
		{
			limit_value_temp |=value_down;
		//	printk("Timeover (DOWN)--value:%02x!\n",limit_value_temp);
		}
		cmd.param.motorLimitValue = limit_value_temp;
		err = copy_to_user((GPIO_CMD *)buf, &cmd, count);
		ret = 0;
		break;
		
	case GPIO_GET_IR_CUT_STATUS:
#if 0
	value = HW_REG(IO_ADDRESS(GPIO_2_BASE+(IR_CUT_IN<<2)));
		if(value & (IR_CUT_IN))g_ir_cut_status= 1;
		else g_ir_cut_status = 0;
#endif
		cmd.param.ircutInStatus = g_ir_cut_status;
		ret = copy_to_user((GPIO_CMD *)buf, &cmd, count);
#if 1
		if (g_ir_cut_status == 2 || g_ir_cut_status == 1 )
		{
			g_ir_cut_status = 0;
		}
#endif
		ret = 0;
		break;

	case GPIO_GET_WIRELESS_ALARM_STATUS:
	//	printk("Entry GPIO_GET_WIRELESS_ALARM_STATUS \n");

#if 0
/********************input mode*****************/
		value = HW_REG(IO_ADDRESS(GPIO_7_BASE+0x80));

		value = HW_REG(IO_ADDRESS(GPIO_2_BASE+(ALRAM_D0<<2)));
		if(value & (ALRAM_D0))cmd.D0 = 1;
		else cmd.D0 = 0;
		value = HW_REG(IO_ADDRESS(GPIO_2_BASE+(ALRAM_D1<<2)));
		if(value & (ALRAM_D1))cmd.D1 = 1;
		else cmd.D1 = 0;
		value = HW_REG(IO_ADDRESS(GPIO_2_BASE+(ALRAM_D2<<2)));
		if(value & (ALRAM_D2))cmd.D2 = 1;
		else cmd.D2 = 0;
		value = HW_REG(IO_ADDRESS(GPIO_2_BASE+(ALRAM_D3<<2)));
		if(value & (ALRAM_D3))cmd.D3 = 1;
		else cmd.D3 = 0;
#endif 
/********************interrupt mode*************************/
		cmd.D0 = g_alramD0;
		cmd.D1 = g_alramD1;
		cmd.D2 = g_alramD2;
		cmd.D3 = g_alramD3;
	//	printk("d0:%d:d1:%d:d2:%d:d3:%d\n", cmd.D0, cmd.D1, cmd.D2, cmd.D3);

		ret = copy_to_user((GPIO_CMD *)buf, &cmd, count);
		if (g_alramD0 == 2 || g_alramD0 == 1 )
		{
			g_alramD0 = 0;
		}
		if (g_alramD1 == 2 || g_alramD1 == 1 )
		{
			g_alramD1 = 0;
		}
		if (g_alramD2 == 2 || g_alramD2 == 1 )
		{
			g_alramD2 = 0;
		}
		if (g_alramD3 == 2 || g_alramD3 == 1 )
		{
			g_alramD3 = 0;
		}
		ret = 0;
		break;

	default:
		break;
	}	
	
	return ret;
}

static int gpio_SetProbeOut(int nChannel, int status)
{
	int val = 0;
	
	if (nChannel<0 || nChannel>=PROBE_OUT_NUM)
	{
		return -1;
	}
		
	if (status)
	{
		val = g_probe_out_status | (0x01<<nChannel);		
	}
	else
	{
		val = g_probe_out_status & (~(0x01<<nChannel));
	}
	
	g_probe_out_status = val;
	
	if (val)
	{
		HW_REG(IO_ADDRESS(GPIO_5_BASE+(AO<<2))) = (AO);
	}
	else
	{
		HW_REG(IO_ADDRESS(GPIO_5_BASE+(AO<<2))) = (~AO);
	}

	//printk("gpio_SetProbeOut(%d %d)(%p %x)\n", nChannel, val, IO_ADDRESS(GPIO_5_BASE+(4<<AO)), val<<AO);

	return 0;
}

static int gpio_SetIndicator(int status)
{
	int val = 0;
	
	if (status)
	{
		val = 1;
		HW_REG(IO_ADDRESS(GPIO_5_BASE+(LED<<2))) = (LED);
	}
	else
	{
		val = 0;
		HW_REG(IO_ADDRESS(GPIO_5_BASE+(LED<<2))) = (~LED);
	}	
		
	return 0;
}

static int gpio_SetAlramGarrion(int status)
{
	int val = 0;
	
	if (status)
	{
		val = 1;
		HW_REG(IO_ADDRESS(GPIO_1_BASE+(ALRAM_GARRISON<<2))) = (ALRAM_GARRISON);
	}
	else
	{
		val = 0;
		HW_REG(IO_ADDRESS(GPIO_1_BASE+(ALRAM_GARRISON<<2))) = (~ALRAM_GARRISON);
	}	
		
	return 0;
}


static int gpio_SetAlramOut(int status)
{
	int val = 0;
	
	if (status)
	{
		val = 1;
		HW_REG(IO_ADDRESS(GPIO_2_BASE+(ALRAM_OUT<<2))) = (ALRAM_OUT);
	}
	else
	{
		val = 0;
	//	printk("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<status  = 0 >>>>>>>>>>>>>>>>>>>>>>>>>\n");
		HW_REG(IO_ADDRESS(GPIO_2_BASE+(ALRAM_OUT<<2))) = (~ALRAM_OUT);
	}	
		
	return 0;
}


static ssize_t misc_write(struct file * file, const char __user * buf, size_t count, loff_t * offset)
{
	int ret = -1;
	int i = 0;
	GPIO_CMD cmd;
	int status = 0;
	int nChannel = 0;
	
	ret = copy_from_user(&cmd, (GPIO_CMD *)buf, count);
	if (ret == -1)
	{
		//printk("copy_from_user: %d\n", ret);
		return -1;
	}
	
	switch (cmd.opt)
	{
	case GPIO_SET_PROBE_OUT_STATUS:	
		{			
			for (i=0; i<PROBE_OUT_NUM; i++)
			{
				nChannel = i;
				status = cmd.param.probeOutStatus & (0x01<<i);
				ret = gpio_SetProbeOut(nChannel, status);
			}
		}
		break;
				
	case GPIO_SET_INDICATOR:
		{
			status = cmd.param.indicatorStatus;
			ret = gpio_SetIndicator(status);
		}	
		break;

	case GPIO_SET_ALRAM_GARRISON_STATUS:
		{
			//printk("Entry  GPIO_SET_ALRAM_GARRISON_STATUS\n");
			status = cmd.param.alarmGarrisonStatus;
			ret = gpio_SetAlramGarrion(status);
		}	
		break;

	case GPIO_SET_ALRAM_OUT:
		{
			status = cmd.param.alarmOutStatus;
			ret = gpio_SetAlramOut(status);
		}	
		break;

	default:
		{
			printk("Not Support CMD: %d(%d)\n", cmd.opt, GPIO_SET_INDICATOR);
		}
		break;
	}
	
	return ret;
}


int misc_ioctl(
    struct inode *inode,
    struct file *file,
    unsigned int ioctl_num,/* The number of the ioctl */
    unsigned long ioctl_param) /* The parameter to it */
{
  struct stepper s;
  int ret;
  /* Switch according to the ioctl called */
  ret = copy_from_user(&s,(void *)ioctl_param,sizeof(struct stepper));
  if(ret){
	printk("misc ioctl get param from user failed!\n");
	return -1;
  }
  local_speed = s.Speed;
  ver_local_speed = local_speed +1;
  switch (ioctl_num) 
  {
    case IOCTL_SET_MSG:
	  
   	  switch (s.CmdID)
   	  {
/**********************************HORIZEN***********************************/
	    case H_STEPEER_CONTROL_RIGHT:       /*right*/
		  hor_run_flag = clockwise;
		
		 break;
	    case H_STEPEER_CONTROL_LETF:       /*left*/
		 hor_run_flag = anticlockwise;		//change the status of the stepper	  
		  break;
	    case H_STEPEER_CONTROL_STOP:      
		  hor_run_flag = reset;   
		  hor_rotation_flag = 0;
		  
		  break;
	    case H_STEPEER_CONTROL_ROTATION:   
		hor_run_flag = clockwise;
		hor_rotation_flag = 1;
		  break;

/**********************************VECTOR***********************************/
   	  case V_STEPEER_CONTROL_DOWN:       /*up*/
		  ver_run_flag = anticlockwise;
		  break;
	    case V_STEPEER_CONTROL_UP:       /*down*/
	  	 ver_run_flag = clockwise;		//change the status of the stepper	  
		  break;
	    case V_STEPEER_CONTROL_STOP:       /*stop*/
		  ver_run_flag = reset ;    //change the status of the stepper
		  ver_rotation_flag = 0;
		  break;
	    case V_STEPEER_CONTROL_ROTATION:    
		  ver_run_flag = clockwise;
		  ver_rotation_flag = 1;
		  break;
	    case HV_STEPEER_CONTROL_GET_COORD:
		  s.coord.x = hor_counter;
		  s.coord.y = ver_counter;
		  copy_to_user((void *)ioctl_param,&s,sizeof(struct stepper));
		  printk("motor driver get coord--X:%d--Y:%d--\n",s.coord.x,s.coord.y);
		  break;
	    case HV_STEPEER_CONTROL_GOTO:
		  goto_point_x = s.coord.x;
		  goto_point_y = s.coord.y;
		if(goto_point_x > HOR_MAX_COOR || goto_point_y > VER_MAX_COOR){
			printk("goto over\n");
			break;
		}
			
		goto_point_x -= hor_counter;
		goto_point_y -= ver_counter;

		H_goto_start_flag = 1;   //转到预置点标志位     0；跳 完成 1：跳转中
		V_goto_start_flag = 1;
		if(goto_point_x > 0) 
			hor_run_flag = anticlockwise;
		else if(goto_point_x < 0)
			hor_run_flag = clockwise;
		else
			hor_run_flag = reset;

		if(goto_point_y > 0) 
			ver_run_flag = clockwise;
		else if(goto_point_y < 0)
			ver_run_flag = anticlockwise;
		else
			ver_run_flag = reset;
		  break;
#if 0
	    case HV_STEPEER_CONTROL_INIT:
		 hor_run_flag = anticlockwise;
		 ver_run_flag = anticlockwise;
		 break;
#endif
       }      
       break;
	
    case IOCTL_GET_MSG:
      printk(("Device Ioctl GET_MSG\n"));
      break;
  }
  printk("--hor_run_flag value:%d--ver_run_flag:%d--\n",hor_run_flag,ver_run_flag);
  return 0;
}


#if 0
 static unsigned int misc_limit_poll( struct file *file, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    poll_wait(file, &limit_waitq, wait);
    if (ev_press)
        mask |= POLLIN | POLLRDNORM;
    return mask;
}
#endif

static struct file_operations gpio_misc_control_fops =
{ 
	owner:		THIS_MODULE, 
	open:			misc_open,
	release:	misc_release,
	read:			misc_read,
	write:		misc_write,
         ioctl:		misc_ioctl
     //    poll:         misc_limit_poll
}; 
static void ptz_init(void){
	unsigned char value;
	hor_init_flag = 1;
	value = HW_REG(IO_ADDRESS(GPIO_6_BASE+(H_RIGHT_LIMIT<<2)));
	if (!(value & (H_RIGHT_LIMIT)))
	{
		hor_run_flag = anticlockwise;
	}else{
		hor_run_flag = clockwise;
	}

	
}
static inline void hard_timer_init(void)
{
	unsigned long init_num= 0x5dc;//0xbb8;
	unsigned long t;
	TIMER3_writel(init_num,TIMER3_LOAD);
	TIMER3_writel(init_num,TIMER3_BGLOAD);
	t = readl(IO_ADDRESS(HISILICON_SCTL_BASE));
	writel(t & ~0x00600000, IO_ADDRESS(HISILICON_SCTL_BASE));
	TIMER3_writel(0x40,TIMER3_CONTROL);
	TIMER3_writel(0x00,TIMER3_INTCLR);
	TIMER3_writel(0xf0,TIMER3_CONTROL);
}
static int limit_switch_task(void  *data)
{
	unsigned char value = 0;
	struct sched_param param = { .sched_priority = 99 };

	sched_setscheduler(current, SCHED_FIFO, &param);
	current->flags |= PF_NOFREEZE;

	set_current_state(TASK_INTERRUPTIBLE);
	
	while(1){
		if(((int )hor_counter < -(HOR_MAX_COOR+100))||((int )hor_counter > HOR_MAX_COOR+100)){
			printk("\nmotor driver --hor--coord failed---%d--\n",hor_counter);
			hor_init_flag = 0;
			hor_counter = 0;
			hor_run_flag = reset;
		}
		if(((int )ver_counter < -(VER_MAX_COOR+200))||((int )ver_counter > VER_MAX_COOR+200)){
			printk("\nmotor driver --ver--coord failed---%d\n",ver_counter);
			ver_init_flag = 0;
			ver_counter = 0;
			ver_run_flag = reset;
		}
		value = HW_REG(IO_ADDRESS(GPIO_6_BASE+(H_RIGHT_LIMIT<<2)));	
		if (!(value & (H_RIGHT_LIMIT)))
		{
		//	printk("--kernel--limit RIGHT--\n");
			if(hor_rotation_flag||hor_init_flag)
				hor_run_flag = anticlockwise;
			else{
				if(hor_run_flag == clockwise) hor_run_flag = reset;
			}
		//	if(hor_counter)printk("current coord x:%d\ty:%d\n",hor_counter,ver_counter);	
			hor_counter = 0;
		}
		value = HW_REG(IO_ADDRESS(GPIO_6_BASE+(H_LEFT_LIMIT<<2)));
		if (!(value & (H_LEFT_LIMIT)))
		{
		//	printk("--kernel--limit LEFT--\n");
			if(hor_rotation_flag||hor_init_flag)
				hor_run_flag = clockwise;
			else{
				if(hor_run_flag == anticlockwise) hor_run_flag = reset;
			}
			
		//	printk("current coord x:%d\ty:%d\n",hor_counter,ver_counter);
			if(!hor_init_flag)  hor_counter = HOR_MAX_COOR;
		}
		value = HW_REG(IO_ADDRESS(GPIO_7_BASE+(V_UP_LIMIT<<2)));
		if (!(value & (V_UP_LIMIT)))
		{
			//printk("--kernel--limit UP--\n");
			if(ver_rotation_flag||ver_init_flag)
				ver_run_flag = anticlockwise;
			else{
				if(ver_run_flag == clockwise)  ver_run_flag = reset;
			}
		//	printk("current coord x:%d\ty:%d\n",hor_counter,ver_counter);
			if(!(ver_init_flag || hor_init_flag)) ver_counter = VER_MAX_COOR;
		}
		value = HW_REG(IO_ADDRESS(GPIO_7_BASE+(V_DOWN_LIMIT<<2)));
		if (!(value & (V_DOWN_LIMIT)))
		{
			//printk("--kernel--DOWN--\n");
			if(ver_rotation_flag||ver_init_flag)
				ver_run_flag = clockwise;
			else
			{
				if(ver_run_flag == anticlockwise) ver_run_flag = reset;
			}
		//	if(ver_counter)printk("current coord x:%d\ty:%d\n",hor_counter,ver_counter);
			ver_counter = 0;
		}
		wait_event_interruptible_timeout(limit_waitq,0,HZ/10);
		//printk("limit test kthread start->->->->->\n");
		
	}
	set_current_state(TASK_RUNNING);

	return 0;
}

static int open_limit_switch_task(void)
{
	struct task_struct *p_limit_switch;
	p_limit_switch = kthread_create(limit_switch_task, NULL, "limitswitch");
	if(IS_ERR(p_limit_switch) <0) {
		printk(KERN_ERR  "create limit switch test task failed!\n");
		return -1;
	}
	wake_up_process(p_limit_switch);
	pid_limit_switch_task = p_limit_switch->pid;
	if(pid_limit_switch_task > 0)
		wake_up_process(find_task_by_pid(pid_limit_switch_task));
	return 0;
}
static int __init  gpio_misc_control_init(void)
{ 
	int ret = 0; 
	
	
	ret = register_chrdev(misc_major, DEVICE_NAME, &gpio_misc_control_fops); 
	if (ret < 0) 
	{ 
		printk("%s can't register major number\n", DEVICE_NAME); 
		return ret; 
	}
    
	if (misc_major == 0) 
	{
		misc_major = ret;
	}
    	
#ifdef CONFIG_DEVFS_FS 
	devfs_mk_cdev(MKDEV(misc_major, 0), S_IFCHR|S_IRUGO|S_IWUSR, DEVICE_NAME); 
#endif 
	//do_gettimeofday(&tv);
//	

	gpio_init();

	hard_timer_init();

	 init_timer(&H_ttimer);
	 H_ttimer.function = H_time_tick;

#endif
//	test_motor_speed();
	
	//ret = request_irq(IRQ, &probe_alarm_inth, SA_SHIRQ, DEVICE_NAME, NULL); // SA_SHIRQ
	ret = request_irq(IRQ, &probe_alarm_inth, SA_SHIRQ, DEVICE_NAME, &GPIO_dev);
	if (ret) 
	{
		unregister_chrdev(misc_major, DEVICE_NAME); 
		printk(DEVICE_NAME " can't request irq(%d %d)\n", IRQ, ret);
		return ret;
	}
	/**/
	ret = request_irq(TIMER3_IRQ, &TIMER3_inth, SA_SHIRQ, DEVICE_NAME, &GPIO_dev);
	if (ret) 
	{
		unregister_chrdev(misc_major, DEVICE_NAME); 
		printk(DEVICE_NAME " can't request timer irq(%d %d)\n", IRQ, ret);
		return ret;
	}

	open_limit_switch_task();
	ptz_init();

	printk(DEVICE_NAME " Module OK!\n");
	
	return 0; 
} 

static void __exit gpio_misc_control_exit(void) 
{
	//free_irq(IRQ, NULL);
	free_irq(IRQ, &GPIO_dev);
	free_irq(TIMER3_IRQ, &GPIO_dev);
	unregister_chrdev(misc_major, DEVICE_NAME); 
	devfs_remove(DEVICE_NAME);
}

module_init(gpio_misc_control_init);
module_exit(gpio_misc_control_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lvjh");

