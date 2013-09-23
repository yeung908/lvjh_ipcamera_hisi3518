/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : gpioi2c_16.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2010/08/24
  Description   : GPIO-I2C for 8bit register and 16bit Data Transfer, eg. MT9P031
  History       :
  1.Date        : 2010/08/24
    Author      :
    Modification: change base address    ---2011-03-19  changed by z00174593
    change by   : z00174593
******************************************************************************/

#include <linux/module.h>
//#include <asm/hardware.h>		/* current os disable IO_ADDRESS function      */
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/fcntl.h>

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>

#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/io.h>

#include "gpioi2c_ex.h"

#ifdef HI_FPGA
#define GPIO_0_BASE 0x20140000

#define SCL                 (1 << 2)    /* GPIO 0_0 */
#define SDA                 (1 << 3)    /* GPIO 0_1 */
#define GPIO_I2C_SCL_REG    IO_ADDRESS(GPIO_0_BASE + 0x10)
#define GPIO_I2C_SDA_REG    IO_ADDRESS(GPIO_0_BASE + 0x20)
#define GPIO_I2C_SCLSDA_REG IO_ADDRESS(GPIO_0_BASE + 0x30)
#else
#define GPIO_0_BASE 0x20160000

#define SCL                 (1 << 1)    /* GPIO 2_1 */
#define SDA                 (1 << 0)    /* GPIO 2_0 */
#define GPIO_I2C_SCL_REG    IO_ADDRESS(GPIO_0_BASE + 0x08)
#define GPIO_I2C_SDA_REG    IO_ADDRESS(GPIO_0_BASE + 0x04)
#define GPIO_I2C_SCLSDA_REG IO_ADDRESS(GPIO_0_BASE + 0x0C)
#endif

#define GPIO_0_DIR IO_ADDRESS_VERIFY(GPIO_0_BASE + 0x400)

void __iomem *reg_gpio0_base_va;
#define IO_ADDRESS_VERIFY(x) (reg_gpio0_base_va + ((x)-(GPIO_0_BASE)))


#define HW_REG(reg)         *((volatile unsigned int *)(reg))
#define DELAY(us)           time_delay_us(us)

/*
 * I2C by GPIO simulated  clear 0 routine.
 *
 * @param whichline: GPIO control line
 *
 */
static void i2c_clr(unsigned char whichline)
{
	unsigned char regvalue;

	if(whichline == SCL)
	{
		regvalue = HW_REG(GPIO_0_DIR);
		regvalue |= SCL;
		HW_REG(GPIO_0_DIR) = regvalue;

		HW_REG(GPIO_I2C_SCL_REG) = 0;
		return;
	}
	else if(whichline == SDA)
	{
		regvalue = HW_REG(GPIO_0_DIR);
		regvalue |= SDA;
		HW_REG(GPIO_0_DIR) = regvalue;

		HW_REG(GPIO_I2C_SDA_REG) = 0;
		return;
	}
	else if(whichline == (SDA|SCL))
	{
		regvalue = HW_REG(GPIO_0_DIR);
		regvalue |= (SDA|SCL);
		HW_REG(GPIO_0_DIR) = regvalue;

		HW_REG(GPIO_I2C_SCLSDA_REG) = 0;
		return;
	}
	else
	{
		printk("Error input.\n");
		return;
	}

}

/*
 * I2C by GPIO simulated  set 1 routine.
 *
 * @param whichline: GPIO control line
 *
 */
static void  i2c_set(unsigned char whichline)
{
	unsigned char regvalue;

	if(whichline == SCL)
	{
		regvalue = HW_REG(GPIO_0_DIR);
		regvalue |= SCL;
		HW_REG(GPIO_0_DIR) = regvalue;

		HW_REG(GPIO_I2C_SCL_REG) = SCL;
		return;
	}
	else if(whichline == SDA)
	{
		regvalue = HW_REG(GPIO_0_DIR);
		regvalue |= SDA;
		HW_REG(GPIO_0_DIR) = regvalue;

		HW_REG(GPIO_I2C_SDA_REG) = SDA;
		return;
	}
	else if(whichline == (SDA|SCL))
	{
		regvalue = HW_REG(GPIO_0_DIR);
		regvalue |= (SDA|SCL);
		HW_REG(GPIO_0_DIR) = regvalue;

		HW_REG(GPIO_I2C_SCLSDA_REG) = (SDA|SCL);
		return;
	}
	else
	{
		printk("Error input.\n");
		return;
	}
}

/*
 *  delays for a specified number of micro seconds rountine.
 *
 *  @param usec: number of micro seconds to pause for
 *
 */
void time_delay_us(unsigned int usec)
{
	volatile int i,j;

	for(i=0;i<usec * 5;i++)
	{
		for(j=0;j<47;j++)
		{;}
	}
}

/*
 * I2C by GPIO simulated  read data routine.
 *
 * @return value: a bit for read
 *
 */

static unsigned char i2c_data_read(void)
{
	unsigned char regvalue;

	regvalue = HW_REG(GPIO_0_DIR);
	regvalue &= (~SDA);
	HW_REG(GPIO_0_DIR) = regvalue;
	DELAY(1);

	regvalue = HW_REG(GPIO_I2C_SDA_REG);
	if((regvalue&SDA) != 0)
		return 1;
	else
		return 0;
}



/*
 * sends a start bit via I2C rountine.
 *
 */
static void i2c_start_bit(void)
{
        DELAY(1);
        i2c_set(SDA | SCL);
        DELAY(1);
        i2c_clr(SDA);
        DELAY(1);
}

/*
 * sends a stop bit via I2C rountine.
 *
 */
static void i2c_stop_bit(void)
{
        /* clock the ack */
        DELAY(1);
        i2c_set(SCL);
        DELAY(1);
        i2c_clr(SCL);

        /* actual stop bit */
        DELAY(1);
        i2c_clr(SDA);
        DELAY(1);
        i2c_set(SCL);
        DELAY(1);
        i2c_set(SDA);
        DELAY(1);
}

/*
 * sends a character over I2C rountine.
 *
 * @param  c: character to send
 *
 */
static void i2c_send_byte(unsigned char c)
{
    int i;
    local_irq_disable();
    for (i=0; i<8; i++)
    {
        DELAY(1);
        i2c_clr(SCL);
        DELAY(1);

        if (c & (1<<(7-i)))
            i2c_set(SDA);
        else
            i2c_clr(SDA);

        DELAY(1);
        i2c_set(SCL);
        DELAY(1);
        i2c_clr(SCL);
    }
    DELAY(1);
   // i2c_set(SDA);
    local_irq_enable();
}

/*  receives a character from I2C rountine.
 *
 *  @return value: character received
 *
 */
static unsigned char i2c_receive_byte(void)
{
    int j=0;
    int i;
    unsigned char regvalue;

    local_irq_disable();
    for (i=0; i<8; i++)
    {
        DELAY(1);
        i2c_clr(SCL);
        DELAY(1);
        i2c_set(SCL);

        regvalue = HW_REG(GPIO_0_DIR);
        regvalue &= (~SDA);
        HW_REG(GPIO_0_DIR) = regvalue;
        DELAY(1);

        if (i2c_data_read())
            j+=(1<<(7-i));

        DELAY(1);
        i2c_clr(SCL);
    }
    local_irq_enable();
    DELAY(1);
   // i2c_clr(SDA);
   // DELAY(1);

    return j;
}

/*  receives an acknowledge from I2C rountine.
 *
 *  @return value: 0--Ack received; 1--Nack received
 *
 */
static int i2c_receive_ack(void)
{
    int nack;
    unsigned char regvalue;

    DELAY(1);

    regvalue = HW_REG(GPIO_0_DIR);
    regvalue &= (~SDA);
    HW_REG(GPIO_0_DIR) = regvalue;

    DELAY(1);
    i2c_clr(SCL);
    DELAY(1);
    i2c_set(SCL);
    DELAY(1);

    nack = i2c_data_read();

    DELAY(1);
    i2c_clr(SCL);
    DELAY(1);
  //  i2c_set(SDA);
  //  DELAY(1);

    if (nack == 0)
        return 1;

    return 0;
}

/*  send an acknowledge to Device.
 *
 *  @return value: NA
 *
 */
static void i2c_send_ack(void)
{
    //DELAY(1);
    //i2c_clr(SCL);
    DELAY(1);
    //i2c_set(SDA);
    i2c_clr(SDA);
    DELAY(1);
    i2c_set(SCL);
    DELAY(1);
    i2c_clr(SCL);
    //DELAY(1);
    i2c_set(SDA);
    //DELAY(1);
}

/*  send an non-acknowledge to Device.
 *
 *  @return value: NA
 *
 */
static void i2c_send_nack(void)
{
    DELAY(1);
    i2c_set(SDA);
    DELAY(1);
    i2c_clr(SCL);
    DELAY(1);
    //i2c_set(SDA); //i2c_clr(SDA);
    //DELAY(1);
    i2c_set(SCL);
    DELAY(1);
    i2c_clr(SCL);
    DELAY(1);
    i2c_clr(SDA);
    DELAY(1);
}

EXPORT_SYMBOL(gpio_i2c_read_ex);
unsigned short gpio_i2c_read_ex(unsigned char devaddress, unsigned short address, unsigned int addr_len, unsigned int data_len)
{
    int i = 0;
    unsigned short tmpH = 0, tmpL = 0;
    unsigned short rxdata;

    i2c_start_bit();
    i2c_send_byte((unsigned char)(devaddress));
    i2c_receive_ack();

    for(i = 0; i < addr_len; i++)
    {
	    //printk("gpio_i2c_read_ex: 0x%x \n\n", (unsigned char)(address >> ((addr_len - i -1) * 8)));
        i2c_send_byte((unsigned char)(address >> ((addr_len - i -1) * 8)));
        i2c_receive_ack();
    }

	//DELAY(1);
    i2c_start_bit();
    i2c_send_byte((unsigned char)(devaddress) | 1);
    i2c_receive_ack();
#if 0
    rxdata = i2c_receive_byte();
    //i2c_send_ack();
#else
    if(2 == data_len)
    {
        tmpH = i2c_receive_byte();
        i2c_send_ack();
    }

	tmpL = i2c_receive_byte();
#endif

    i2c_send_nack();
    i2c_stop_bit ();

    rxdata = ((tmpH << 8) & 0xff00) | (tmpL & 0xff);

    return rxdata;
}


EXPORT_SYMBOL(gpio_i2c_write_ex);
void gpio_i2c_write_ex(unsigned char devaddress,
                    unsigned short address, unsigned int addr_len,
                    unsigned short data, unsigned int data_len)
{
    int i = 0;

    i2c_start_bit();
    i2c_send_byte((unsigned char)(devaddress));
    i2c_receive_ack();

    for(i = 0; i < addr_len; i++)
    {
        //printk("gpio_i2c_write_ex: 0x%x.\n", (unsigned char)(address >> ((addr_len - i -1) * 8)));
        i2c_send_byte((unsigned char)(address >> ((addr_len - i -1) * 8)));
        i2c_receive_ack();
    }

#if 0
    i2c_send_byte(data);
#else
    if(2 == data_len)
    {
    	i2c_send_byte((unsigned char)((data&0xff00)>>8));
    	i2c_receive_ack();
    }
	i2c_send_byte((unsigned char)(data&0x00ff));
#endif
   // i2c_receive_ack();//add by hyping for tw2815
    i2c_stop_bit();
}

long gpioi2c_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    I2C_DATA_S __user *argp = (I2C_DATA_S __user*)arg;
	unsigned char      device_addr;
	unsigned short     reg_addr;
	unsigned short     reg_val;
    unsigned int       addr_len;
    unsigned int       data_len;

	switch(cmd)
	{
		case GPIO_I2C_READ:
             device_addr =  argp->dev_addr;
             reg_addr    =  argp->reg_addr;
             addr_len    =  argp->addr_byte_num;
             data_len    =  argp->data_byte_num;

             #if 0
             printk("addr_len    : %2x \n",addr_len   );
             printk("device_addr : %2x \n",device_addr);
		     printk("reg_addr    : %2x \n",reg_addr   );
             #endif

			 reg_val = gpio_i2c_read_ex(device_addr, reg_addr, addr_len, data_len);
			 argp->data = reg_val ;
			 break;

		case GPIO_I2C_WRITE:
			 device_addr =  argp->dev_addr;
             reg_addr    =  argp->reg_addr;
             addr_len    =  argp->addr_byte_num;
             data_len    =  argp->data_byte_num;
             reg_val     =  argp->data;
			 gpio_i2c_write_ex(device_addr, reg_addr, addr_len, reg_val, data_len);

             #if 0
             printk("device_addr : %0x \n",device_addr);
             printk("reg_addr    : %0x \n",reg_addr   );
             printk("addr_len    : %0x \n",addr_len   );
             printk("reg_val    : %0x \n",reg_val   );
             printk("data_len    : %0x \n",data_len   );
             #endif
			 break;

		default:
			return -1;
	}
    return 0;
}

int gpioi2c_open(struct inode * inode, struct file * file)
{
    return 0;
}
int gpioi2c_close(struct inode * inode, struct file * file)
{
    return 0;
}


static struct file_operations gpioi2c_fops = {
    .owner      = THIS_MODULE,
    //.ioctl      = gpioi2c_ioctl,
    .unlocked_ioctl   = gpioi2c_ioctl,
    .open       = gpioi2c_open,
    .release    = gpioi2c_close
};


static struct miscdevice gpioi2c_dev = {
   .minor		= MISC_DYNAMIC_MINOR,
   .name		= "gpioi2c_ex",
   .fops    = &gpioi2c_fops,
};

static int __init gpio_i2c_init(void)
{
    int ret;
    //unsigned int reg;
    reg_gpio0_base_va = ioremap_nocache((unsigned long)GPIO_0_BASE, (unsigned long)0x10000);


    ret = misc_register(&gpioi2c_dev);
    if(0 != ret)
    	return -1;

#if 1
    //printk(KERN_INFO OSDRV_MODULE_VERSION_STRING "\n");
    //reg = HW_REG(SC_PERCTRL1);
    //reg |= 0x00004000;
    //HW_REG(SC_PERCTRL1) = reg;
    i2c_set(SCL | SDA);
#endif
    return 0;
}

static void __exit gpio_i2c_exit(void)
{
    iounmap((void*)reg_gpio0_base_va);
    misc_deregister(&gpioi2c_dev);
}


#ifdef MODULE
//#include <linux/compile.h>
#endif

module_init(gpio_i2c_init);
module_exit(gpio_i2c_exit);
//MODULE_INFO(build, UTS_VERSION);
MODULE_LICENSE("GPL");




