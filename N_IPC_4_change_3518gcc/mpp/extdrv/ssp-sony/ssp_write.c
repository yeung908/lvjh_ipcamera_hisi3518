/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : ssp_write.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2010/09/27
  Description   : 
  History       :
  1.Date        : 2010/09/27
    Author      : x00100808
    change      : z00174593
    Modification: mode the parameter

******************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "strfunc.h"
#include "hi_ssp.h"


int main(int argc , char* argv[])
{
	int fd = -1;
	int ret;
	unsigned int dev_addr, reg_addr, reg_value, value;
		
	if(argc != 4)
    {
    	printf("usage: %s <reg_addr> <reg_value>. sample: %s 0x2010 0x0927\n", argv[0], argv[0]);
        return -1;
    }
	
	fd = open("/dev/ssp", 0);
    if(fd<0)
    {
    	printf("Open /dev/ssp error!\n");
    	return -1;
    }
    
    if (StrToNumber(argv[1], &dev_addr))
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
    
    value = (((dev_addr&0xff)<<16)|((reg_addr&0xff)<<8) | (reg_value&0xff));
    
    ret = ioctl(fd, SSP_WRITE_ALT, &value);

    printf("ssp_write %#x, %#x, %#x\n", dev_addr, reg_addr, reg_value);

	close(fd);
        
    return 0;
}

