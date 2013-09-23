#include <stdio.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "strfunc.h"
#include "hi_i2c.h"
unsigned int reg_width = 1;
unsigned int data_width = 1;

int main(int argc , char* argv[])
{
	int fd = -1;
	int ret;
	I2C_DATA_S  i2c_data ;
	unsigned int device_addr, reg_addr, reg_addr_end, reg_value;

	if (argc < 3)
    {
    	printf("usage: %s <device_addr> <reg_addr> <end_reg_addr> <reg_width> <data_width>. sample: \n", argv[0]);
        printf("------------------------------------------------------------------------------------\n");
        printf("\t\t%s 0x56 0x0 0x10 2 2. \n", argv[0]);
        printf("\t\t%s 0x56 0x0 0x10. default reg_width and data_width is 1. \n", argv[0]);

        return -1;
    }

	fd = open("/dev/hi_i2c", 0);  	
    if (fd<0)
    {
    	printf("Open hi_i2c dev error!\n");
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

    if(argc > 3)
    {
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

        if((argc > 4) && StrToNumber(argv[4], &reg_width))
            return 0;

        if((argc > 5) && StrToNumber(argv[5], &data_width))
            return 0;
    }
    else
    {
        reg_addr_end = reg_addr;
    }

    printf("dev_addr:0x%2x; reg_addr:0x%2x; reg_addr_end:0x%2x; reg_width: %d; data_width: %d. \n", device_addr,
            reg_addr, reg_addr_end, reg_width, data_width);
    {
        int cur_addr;

        for (cur_addr = reg_addr; cur_addr < reg_addr_end + reg_width; cur_addr ++)
        {
            i2c_data.dev_addr = device_addr ;
            i2c_data.reg_addr = cur_addr    ;
            i2c_data.addr_byte_num   = reg_width  ;
            i2c_data.data_byte_num   = data_width ;
            ret = ioctl(fd, CMD_I2C_READ, &i2c_data);
            reg_value =  i2c_data.data ;
            printf("0x%x 0x%x\n", cur_addr, reg_value);
        }
    }

	close(fd);

    return 0;
}
