
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
	int ret =0;
	unsigned int device_addr, reg_addr, reg_value, value;
		
	if(argc != 4)
    {
    	printf("usage: %s <device_addr> <reg_addr> <value>. sample: %s 0x56 0x0 0x28\n", argv[0], argv[0]);
        return -1;
    }
	
	fd = open("/dev/gpioi2c", 0);
    if(fd<0)
    {
    	printf("Open gpioi2c error!\n");
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
    
    if (StrToNumber(argv[3], &reg_value))
    {    
    	return 0;
    }
    
    printf("device_addr:0x%2x; reg_addr:0x%2x; reg_value:0x%2x.\n", device_addr, reg_addr, reg_value);
    
    value = ((device_addr&0xff)<<24) | ((reg_addr&0xff)<<16) | (reg_value&0xffff);
    
    ret = ioctl(fd, GPIO_I2C_WRITE, &value);
       
    return 0;
}
