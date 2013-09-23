/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : gpioi2c_16.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2010/08/24
  Description   : for 8bit register and 16bit data transfer
  History       :
  1.Date        : 2010/08/24
    Author      : x00100808
    Modification: Created file
    chnage by   : z00174593

******************************************************************************/

#ifndef __GPIO_I2C_16_H__
#define __GPIO_I2C_16_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */


#define GPIO_I2C_READ   0x01
#define GPIO_I2C_WRITE  0x03

typedef struct I2C_DATA_S
{
	unsigned char	dev_addr;
	unsigned int 	reg_addr;
	unsigned int 	addr_byte_num;
	unsigned int 	data;
  	unsigned int 	data_byte_num;
}I2C_DATA_S;

unsigned short gpio_i2c_read_ex(unsigned char devaddress, unsigned short address, unsigned int addr_len, unsigned int data_len);
void gpio_i2c_write_ex(unsigned char devaddress,
                    unsigned short address, unsigned int addr_len,
                    unsigned short data, unsigned int data_len);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif	/* __GPIO_I2C_16_H__ */
