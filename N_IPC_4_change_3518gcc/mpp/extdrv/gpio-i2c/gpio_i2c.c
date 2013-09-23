
#include <linux/module.h>
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

#include "gpio_i2c.h" 







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

#define GPIO_0_DIR IO_ADDRESS(GPIO_0_BASE + 0x400)


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
	int i,j;
	
	for(i=0;i<usec * 5;i++)
	{
		for(j=0;j<47;j++)
		{;}
	}
}

//------------------------------------add for 9024---------------------
/*
add for sil9024
*/
//------------------------------------------------------------------------------
// Function Name:  siiReadSegmentBlockEDID
// Function Description: Reads segment block of EDID from HDMI Downstream Device
//------------------------------------------------------------------------------
byte siiReadSegmentBlockEDID(byte SlaveAddr, byte Segment, byte Offset, byte *Buffer, byte Length)
{

	   //I2CSendAddr(EDID_SEG_ADDR);
     //i2c_stop_bit();
     
	   //I2CSendByte(Segment);
     //i2c_stop_bit()

	
     //I2CSendAddr(SlaveAddr);
     //i2c_stop_bit();


	   //I2CSendByte(Offset);
     //i2c_stop_bit();

	   //I2CSendAddr (SlaveAddr|1);
		 //I2CSendStop();

    	//for (i = 0; i < Length - 1; i++)
   		//Buffer[i] = I2CGetByte(NOT_LAST_BYTE);
   		//Buffer[i] = I2CGetByte(LAST_BYTE);
   		  //Buffer[i] = i2c_data_read(NOT_LAST_BYTE);
        //Buffer[i] = i2c_data_read(LAST_BYTE);
        //i2c_stop_bit();
     
        //return 1;

       return 1;
}
EXPORT_SYMBOL(siiReadSegmentBlockEDID);
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

#if 0
static void i2c_send_ack(void)
{
    DELAY(1);
    i2c_clr(SCL);
    DELAY(1);
    i2c_set(SDA);
    DELAY(1);
    i2c_set(SCL);
    DELAY(1);
    i2c_clr(SCL);
    DELAY(1);
    i2c_clr(SDA);
    DELAY(1);
}
#endif

EXPORT_SYMBOL(gpio_i2c_read);
unsigned char gpio_i2c_read(unsigned char devaddress, unsigned char address)
{
    int rxdata;
    
    i2c_start_bit();
    i2c_send_byte((unsigned char)(devaddress));
    i2c_receive_ack();
    i2c_send_byte(address);
    i2c_receive_ack();   
    i2c_start_bit();
    i2c_send_byte((unsigned char)(devaddress) | 1);
    i2c_receive_ack();
    rxdata = i2c_receive_byte();
    //i2c_send_ack();
    i2c_stop_bit();

    return rxdata;
}


EXPORT_SYMBOL(gpio_i2c_write);
void gpio_i2c_write(unsigned char devaddress, unsigned char address, unsigned char data)
{
    i2c_start_bit();
    i2c_send_byte((unsigned char)(devaddress));
    i2c_receive_ack();
    i2c_send_byte(address);
    i2c_receive_ack();
    i2c_send_byte(data); 
   // i2c_receive_ack();//add by hyping for tw2815
    i2c_stop_bit();
}

//int gpioi2c_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
long gpioi2c_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int val;
	
	char device_addr, reg_addr;
	short reg_val;
	
	
	switch(cmd)
	{
		case GPIO_I2C_READ:
			val = *(unsigned int *)arg;
			device_addr = (val&0xff000000)>>24;
			reg_addr = (val&0xff0000)>>16;
			
			reg_val = gpio_i2c_read(device_addr, reg_addr);
			*(unsigned int *)arg = (val&0xffff0000)|reg_val;			
			break;
		
		case GPIO_I2C_WRITE:
			val = *(unsigned int *)arg;
			device_addr = (val&0xff000000)>>24;
			reg_addr = (val&0xff0000)>>16;
			
			reg_val = val&0xffff;
			gpio_i2c_write(device_addr, reg_addr, reg_val);
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
    .unlocked_ioctl = gpioi2c_ioctl,
    .open       = gpioi2c_open,
    .release    = gpioi2c_close
};


static struct miscdevice gpioi2c_dev = {
   .minor		= MISC_DYNAMIC_MINOR,
   .name		= "gpioi2c",
   .fops  = &gpioi2c_fops,
};

static int __init gpio_i2c_init(void)
{
    int ret;
    //unsigned int reg;
    
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
    misc_deregister(&gpioi2c_dev);
}


module_init(gpio_i2c_init);
module_exit(gpio_i2c_exit);

#ifdef MODULE
//#include <linux/compile.h>
#endif
//MODULE_INFO(build, UTS_VERSION);
MODULE_LICENSE("GPL");




