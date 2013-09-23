#include <stdio.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "strfunc.h"
#include "gpio_i2c.h"


int main(int argc , char* argv[])
{
	int fd = -1;
	int ret;
	unsigned int device_addr, reg_addr, reg_addr_end,reg_value, value;
	
	
	if ((argc != 3) && (argc != 4))
    {
    	printf("usage: %s <device_addr> <reg_addr>. sample: %s 0x56 0x0\n", argv[0], argv[0]);
        return -1;
    }
	
	fd = open("/dev/gpioi2c", 0);
    if (fd<0)
    {
    	printf("Open gpioi2c dev error!\n");
    	return -1;
    }
    
    if (StrToNumber(argv[1], &device_addr))
    {    	
    	return 0;
    }
       
    if (StrToNumber(argv[2], &reg_addr))
    {    
    	return 0;
    }
    
    if (3 == argc)
    {
        printf("device_addr:0x%2x; reg_addr:0x%2x.\n", device_addr, reg_addr);
        value = ((device_addr&0xff)<<24) | ((reg_addr&0xff)<<16);        
        ret = ioctl(fd, GPIO_I2C_READ, &value);        
        reg_value = value&0xff;        
        printf("0x%2x\n", reg_value);
    }
    else if (4 == argc)
    {
        int cur_addr;
        if (StrToNumber(argv[3], &reg_addr_end))
        {
            return 0;
        }
        if (reg_addr_end < reg_addr)
        {
            printf("end addr(0x%2x) should bigger than start addr(0x%2x)\n",
                reg_addr_end, reg_addr);
            return 0;
        }
        printf("device_addr:0x%2x; reg_addr_start:0x%2x; reg_addr_end:0x%2x.\n", 
            device_addr, reg_addr, reg_addr_end);
        for (cur_addr=reg_addr; cur_addr<reg_addr_end+1; cur_addr++)
        {
            value = ((device_addr&0xff)<<24) | ((cur_addr&0xff)<<16);   
            ret = ioctl(fd, GPIO_I2C_READ, &value);            
            reg_value = value&0xff;
            printf("0x%x 0x%x\n", cur_addr, reg_value);
        }
    }
            
    return 0;
}
