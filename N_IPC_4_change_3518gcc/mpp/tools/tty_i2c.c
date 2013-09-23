
#include <stdio.h>
#include <string.h>
#include <linux/i2c-dev.h>
//#include <linux/i2cbusses.h>
#include <linux/unistd.h>
#include <errno.h>
//#include <util.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
//#include "io.h"
//#include "hi_math.h"

#include "hi_type.h"
//#include "hi_comm_isp.h"
#include "mpi_sys.h"
#include "hi_io.h"

//#define TRACE_IT
//#define REG_8BIT
#define REG_16BIT

int open_i2c_dev(int i2cbus, char* filename, int a)
{
    return 0;
}

int set_slave_addr(int file, int address, int force)
{
    return 0;
}

int i2c_smbus_read_word_data(int file, int daddress)
{
    printf("%s, addr:0x%x\n", __func__, daddress);
    return 0;
}

int i2c_smbus_read_byte(int file)
{
    return 0;
}
int i2c_smbus_read_byte_data(int file, int daddress)
{
    return 0;    
}
int i2c_smbus_write_byte(int file, int daddress)
{
    printf("%s, addr:0x%x\n", __func__, daddress);    
    return 0;
}

int i2c_smbus_write_byte_data(int file, int daddress, int value)
{
    printf("%s, addr:0x%x, value:0x%x\n", __func__, daddress, value);    
    return 0;
}

int i2c_smbus_write_word_data(int file, int daddress, int value)
{
    printf("%s, addr:0x%x, value:0x%x\n", __func__, daddress, value);    
    return 0;
}

static int confirm(const char *filename, int address, int size, int daddress,
		   int pec)
{
	int dont = 0;

	fprintf(stderr, "WARNING! This program can confuse your I2C "
		"bus, cause data loss and worse!\n");

	/* Don't let the user break his/her EEPROMs */
	if (address >= 0x50 && address <= 0x57 && pec) {
		fprintf(stderr, "STOP! EEPROMs are I2C devices, not "
			"SMBus devices. Using PEC\non I2C devices may "
			"result in unexpected results, such as\n"
			"trashing the contents of EEPROMs. We can't "
			"let you do that, sorry.\n");
		return 0;
	}

	if (size == I2C_SMBUS_BYTE && daddress >= 0 && pec) {
		fprintf(stderr, "WARNING! All I2C chips and some SMBus chips "
		        "will interpret a write\nbyte command with PEC as a"
		        "write byte data command, effectively writing a\n"
		        "value into a register!\n");
		dont++;
	}

	fprintf(stderr, "I will read from device file %s, chip "
		"address 0x%02x, ", filename, address);
	if (daddress < 0)
		fprintf(stderr, "current data\naddress");
	else
		fprintf(stderr, "data address\n0x%02x", daddress);
	fprintf(stderr, ", using %s.\n",
		size == I2C_SMBUS_BYTE ? (daddress < 0 ?
		"read byte" : "write byte/read byte") :
		size == I2C_SMBUS_BYTE_DATA ? "read byte data" :
		"read word data");
	if (pec)
		fprintf(stderr, "PEC checking enabled.\n");

	fprintf(stderr, "Continue? [%s] ", dont ? "y/N" : "Y/n");
	fflush(stderr);
	
	#if 0
	if (!user_ack(!dont)) {
		fprintf(stderr, "Aborting on user request.\n");
		return 0;
	}
    #endif

	return 1;
}

int user_ack(int dont)
{
    return 1;
}

static int check_funcs(int file, int i2cbus, int size, int daddress, int pec)
{
	unsigned long funcs;

	/* check adapter functionality */
	if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
		fprintf(stderr, "Error: Could not get the adapter "
		        "functionality matrix: %s\n", strerror(errno));
		return -1;
	}

	switch (size) {
	case I2C_SMBUS_BYTE:
		if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE)) {
			fprintf(stderr, "Error: Adapter for i2c bus %d does "
			        "not have read byte capability\n", i2cbus);
			return -1;
		}
		if (daddress >= 0
		 && !(funcs & I2C_FUNC_SMBUS_WRITE_BYTE)) {
			fprintf(stderr, "Error: Adapter for i2c bus %d does "
			        "not have write byte capability\n", i2cbus);
			return -1;
		}
		break;

	case I2C_SMBUS_BYTE_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA)) {
			fprintf(stderr, "Error: Adapter for i2c bus %d does "
			        "not have read byte data capability\n", i2cbus);
			return -1;
		}
		break;

	case I2C_SMBUS_WORD_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_READ_WORD_DATA)) {
			fprintf(stderr, "Error: Adapter for i2c bus %d does "
			        "not have read word data capability\n", i2cbus);
			return -1;
		}
		break;
	}


	if (pec
	 && !(funcs & (I2C_FUNC_SMBUS_PEC | I2C_FUNC_I2C))) {
		fprintf(stderr, "Warning: Adapter for i2c bus %d does "
		        "not seem to support PEC\n", i2cbus);
	}

	return 0;
}

int	write_to_apb_bus(
	int dev, int adr, int val)
{
    #if defined(REG_8BIT)
    IOWR_8DIRECT(adr, 0, val);
    #endif
    
    #if defined(REG_16BIT)
    int adr_aligned;
    int msb;
    int current;
        
    adr_aligned = FLOOR_2_POWER(adr, 2);
    msb = adr - adr_aligned;
        
    current = IORD_16DIRECT(adr_aligned, 0);
    IOWR_16DIRECT(adr_aligned, 0, 
        msb ? ((current&0xff) | (val<<8)) : ((current&0xff00) | val));
    #endif
    
#if defined(TRACE_IO)
    fprintf(stderr, "write reg addr:0x%x, val:0x%x\n", adr_aligned+msb, val);
#endif
    return 0;    
}

int	read_from_apb_bus(
	int dev, int adr, int *val)
{
    #if defined(REG_8BIT)
    *val = IORD_8DIRECT(adr, 0);
    #endif

    #if defined(REG_16BIT)
    int adr_aligned;
    int msb;
    int current;
    
    adr_aligned = FLOOR_2_POWER(adr, 2);
    msb = adr - adr_aligned;
        
    current = IORD_16DIRECT(adr_aligned, 0);
    *val = msb ? ((current & 0xff00) >> 8) : (current & 0xff);
    #endif
    
#if defined(TRACE_IT)
    fprintf(stderr, "read reg addr:0x%x, val:0x%x\n", adr_aligned+msb, *val);
#endif

    return 0;
}

int	write_to_i2c_bus(
	int dev, int adr, int val)
{
	int	file;
	int	i2cbus = 0;
	char filename[20];
	unsigned long funcs;
	int size = I2C_SMBUS_BYTE;
	int daddress = adr;
	int address = dev;
	int	pec = 0;
	int	force = 0;
	int	yes = 1;
	int	value = val;
	int	res;
	int	e1 = 0;
	int	vmask = 0;

	file = open_i2c_dev(i2cbus, filename, 0);
	if (file < 0) {
		fprintf(stderr, "Failed to open_i2c_dev\n");

		return 1;
	}

#if 0
	/* check adapter functionality */
	if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
		fprintf(stderr, "Error: Could not get the adapter "
			"functionality matrix: %s\n", strerror(errno));
		fprintf(stderr, "Failed to ioctl\n");

		return 1;
	}
	#else
	funcs = 1;
#endif

	switch (size) {
	case I2C_SMBUS_BYTE_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE_DATA)) {
			fprintf(stderr, "Error: Adapter for i2c bus %d does "
	    		"not have byte write capability\n", i2cbus);

			fprintf(stderr, "Failed to I2C_FUNC_SMBUS_WRITE_BYTE_DATA\n");
		return 1;
		}
		break;

	case I2C_SMBUS_WORD_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_WRITE_WORD_DATA)) {
			fprintf(stderr, "Error: Adapter for i2c bus %d does "
	    		"not have word write capability\n", i2cbus);

			fprintf(stderr, "Failed to I2C_FUNC_SMBUS_WRITE_WORD_DATA\n");
		return 1;
		}
		break;
	}

	if (set_slave_addr(file, address, force) < 0)
	{
			fprintf(stderr, "Failed to set_slave_addr\n");
			return 1;
	}

	if (!yes) {
		int dont = 0;

		fprintf(stderr, "WARNING! This program can confuse your I2C "
		        "bus, cause data loss and worse!\n");

		if (address >= 0x50 && address <= 0x57) {
			fprintf(stderr, "DANGEROUS! Writing to a serial "
			        "EEPROM on a memory DIMM\nmay render your "
			        "memory USELESS and make your system "
			        "UNBOOTABLE!\n");
			dont = 1;
		}

		fprintf(stderr, "I will write to device file %s, chip address "
		        "0x%02x, data address\n0x%02x, data 0x%02x%s, mode "
		        "%s.\n", filename, address, daddress, value,
			vmask ? " (masked)" : "",
			size == I2C_SMBUS_BYTE_DATA ? "byte" : "word");
		if (pec)
			fprintf(stderr, "PEC checking enabled.\n");

		fprintf(stderr, "Continue? [%s] ", dont ? "y/N" : "Y/n");
		fflush(stderr);
		
		#if 0
		if (!user_ack(!dont)) {
			fprintf(stderr, "Aborting on user request.\n");

			fprintf(stderr, "Failed to user_ack\n");
			return 1;
		}
		#endif
	}
	
	if (vmask) {
		int oldvalue;
    
		if (size == I2C_SMBUS_WORD_DATA) {
			oldvalue = i2c_smbus_read_word_data(file, daddress);
		} else {
//			oldvalue = i2c_smbus_read_byte_data(file, daddress);
			i2c_smbus_write_byte(file, daddress);
			oldvalue = i2c_smbus_read_byte(file);
		}

		if (oldvalue < 0) {
			fprintf(stderr, "Error: Failed to read old value\n");

			return 1;
		}
		value = (value & vmask) | (oldvalue & ~vmask);

		if (!yes) {
			fprintf(stderr, "Old value 0x%0*x, write mask "
				"0x%0*x: Will write 0x%0*x to register "
				"0x%02x\n",
				size == I2C_SMBUS_WORD_DATA ? 4 : 2, oldvalue,
				size == I2C_SMBUS_WORD_DATA ? 4 : 2, vmask,
				size == I2C_SMBUS_WORD_DATA ? 4 : 2, value,
				daddress);

			fprintf(stderr, "Continue? [Y/n] ");
			fflush(stderr);
			
			if (!user_ack(1)) {
				fprintf(stderr, "Aborting on user request.\n");
				return 1;
			}
		}
	}

	if (pec) {
		if (ioctl(file, I2C_PEC, 1) < 0) {
			fprintf(stderr, "Error: Could not set PEC: %s\n",
			        strerror(errno));
			return 1;
		}
		if (!(funcs & (I2C_FUNC_SMBUS_PEC | I2C_FUNC_I2C))) {
			fprintf(stderr, "Warning: Adapter for i2c bus %d does "
			        "not seem to actually support PEC\n", i2cbus);
		}
	}

	e1 = 0;
	printf("begin write\n");
	if (size == I2C_SMBUS_WORD_DATA) {
		res = i2c_smbus_write_word_data(file, daddress, value);
	} else {
		res = i2c_smbus_write_byte_data(file, daddress, value);
	}
	if (res < 0) {
		fprintf(stderr, "Warning - write failed\n");
		e1++;
	}

	if (pec) {
		if (ioctl(file, I2C_PEC, 0) < 0) {
			fprintf(stderr, "Error: Could not clear PEC: %s\n",
				strerror(errno));
			close(file);
			return 1;
		}
	}

#if 0
	// Do not readback
	if (size == I2C_SMBUS_WORD_DATA) {
		res = i2c_smbus_read_word_data(file, daddress);
	} else {
		res = i2c_smbus_read_byte_data(file, daddress);
	}
#endif

	close(file);

#if 0
	if (res < 0) {
		fprintf(stderr, "Warning - readback failed\n");
		e1++;
	} else
	if (res != value) {
		e1++;
		fprintf(stderr, "Warning - data mismatch - wrote "
		        "0x%0*x, read back 0x%0*x\n",
		        size == I2C_SMBUS_WORD_DATA ? 4 : 2, value,
		        size == I2C_SMBUS_WORD_DATA ? 4 : 2, res);
	} else {
		fprintf(stderr, "Value 0x%0*x written, readback matched\n",
		        size == I2C_SMBUS_WORD_DATA ? 4 : 2, value);
	}
	return e1;
#endif

	return (res<0);
}

int	read_from_i2c_bus(
	int dev, int adr, int *val)
{
    int i2cbus, address, file, size, daddress;
	int	pec = 0;
	int	force = 0;
	int	yes = 1;
	char filename[20];

	i2cbus = 0;
	size = I2C_SMBUS_BYTE;
	daddress = adr;
	address = dev;

	file = open_i2c_dev(i2cbus, filename, 0);
	if (file < 0
	 || check_funcs(file, i2cbus, size, daddress, pec)
	 || set_slave_addr(file, address, force))
	{
		fprintf(stderr, "Failed with open_i2c_dev || check_funcs || set_slave_addr\n");
		return 1;
	}

	if (!yes && !confirm(filename, address, size, daddress, pec))
	{
		fprintf(stderr, "Failed with confirm\n");
		return 1;
	}

	if (pec && ioctl(file, I2C_PEC, 1) < 0) {
		fprintf(stderr, "Error: Could not set PEC: %s\n",
		        strerror(errno));
		fprintf(stderr, "Failed with PEC\n");
		return 1;
	}

	switch (size) {
	case I2C_SMBUS_BYTE:
		if (daddress >= 0) {
			*val = i2c_smbus_write_byte(file, daddress);
			if (*val < 0)
				fprintf(stderr, "Warning - write failed\n");
		}
		*val = i2c_smbus_read_byte(file);
		break;
	case I2C_SMBUS_WORD_DATA:
		*val = i2c_smbus_read_word_data(file, daddress);
		break;
	default: /* I2C_SMBUS_BYTE_DATA */
		*val = i2c_smbus_read_byte_data(file, daddress);
	}
	close(file);

	if (*val < 0) {
		fprintf(stderr, "Error: Read failed\n");
		printf("failed\n");
		return 1;
	}
	return 0;
}

#define TAG_ADDR_MSB1 0x80
#define TAG_ADDR_MSB2 0x82
#define TAG_LUT_GAMMA 0x20
#define TAG_LUT_PREIRIDIX 0xc0
#define TAG_LUT_POSTIRIDIX 0xe0
#define LUT_TOTAL 3
static HI_U32 s_u32LUT[LUT_TOTAL] =
{
    0x0e20,    /* gamma */
    0x03c0,    /* pre-iridix */
    0x03e0,    /* post-iridix */
};

int is_lut(int _adr)
{
    int i;
    int is_lut = 0; 
    
    for (i = 0; i < LUT_TOTAL; i++)
    {
        if(_adr == s_u32LUT[i])
        {
            is_lut = 1;
            break;
        }
    }

    return is_lut;
}

/*****************************************************************************
 Prototype       : get_lut_addrs
 Description     : 
 Input           : _tag               **
                   _adr_lut_adr       **
                   _adr_lut_data_lsb  **
                   _adr_lut_data_msb  **
 Output          : None
 Return Value    : 
 Process         : 
 Note			 : 

  History         
  1.Date         : 2010/10/11
    Author       : c55300
    Modification : Created function

*****************************************************************************/
void get_lut_addrs(int _adr, int* _adr_lut_adr, int *_adr_lut_data_lsb, int* _adr_lut_data_msb)
{
    *_adr_lut_adr = _adr;
    *_adr_lut_data_lsb = _adr + 2;
    *_adr_lut_data_msb = _adr + 3;

    return;
}

/* 
* 
* write cmd:
* 1, "write 38  8e+a       addr_msb*2+b"
* 2, "write 38  addr_lsb  data      "
* 
* read cmd:
* 1, "write 38 8e+a        addr_msb*2+b"
* 2, "read  38 addr_lsb             "
* 
* a=0, if addr_msb< 0x7F; 
* a=1, if addr_msb>=0x80.
* b=0, if addr_lsb< 0x7f;
* b=1, if addr_lsb>=0x80.
*
* gamma LUT
* 1, "wr 38  20  idx0 data_lsb data_msb idx1 data_lsb data_msb"
* 2, ....
*/
#define EXT_REG_MSB_BASE 0x100
int	apb_responder(char * line)
{
	char	_dummy[10];
	int	_dev,_adr,_val;
    static int s_adr_msb_mul2, s_adr_lsb;
    static int s_adr_msb;
    int _tag;
    
#if defined(TRACE_IT)
	fprintf(stderr,"Running apb_responder\n");
#endif

	sscanf(line, "%s %x %x", _dummy, &_dev, &_tag);
    if ((0== strcmp(_dummy, "write")) && 
        ((TAG_ADDR_MSB1 == _tag) || (TAG_ADDR_MSB2 == _tag)))
    {
		sscanf(line, "%s %x %x %x", _dummy, &_dev, &_tag, &s_adr_msb_mul2);
		sprintf(line, "success %2.2x:%2.2x=%2.2x\n", _dev, _tag, s_adr_msb_mul2);
		
		s_adr_msb = (s_adr_msb_mul2 >> 1) | 
		    ((TAG_ADDR_MSB2 == _tag) ? EXT_REG_MSB_BASE : 0x00);
		return 0;
    }

	if(0 == strncmp("read", line, 4))
	{
		sscanf(line, "%s %x %x %x", _dummy, &_dev, &s_adr_lsb, &_val);
		_adr = (s_adr_msb << 8) | (s_adr_lsb);
		_adr = _adr + (MULTI_OF_2_POWER(s_adr_msb_mul2,2) ? 0x00: 0x80);
		if(0 == read_from_apb_bus(
				 _dev/2, _adr, &_val))
		{
			//sprintf(line, "read_result %2.2x:%2.2x        %2.2x\n", _dev, _adr, _val);
			sprintf(line, "read_result %2.2x:%2.2x        %2.2x\n", _dev, s_adr_lsb, _val);
		}
		else
		{
			sprintf(line, "failed\n");
		}
    }
	else if(0 == strncmp("write", line, 5))
	{
		sscanf(line, "%s %x %x %x", _dummy, &_dev, &s_adr_lsb, &_val);
		_adr = ((s_adr_msb << 8) | (s_adr_lsb));
		_adr = _adr + (MULTI_OF_2_POWER(s_adr_msb_mul2,2) ? 0x00: 0x80);

		if ( !is_lut(_adr) )
        {
    		if(0 != write_to_apb_bus(_dev/2, _adr, _val))
    		{
    			sprintf(line, "failure\n");
    		}
    		else
    		{
    			sprintf(line, "success %2.2x:%2.2x=%2.2x\n", _dev, _adr, _val);
    		}
		}
        else
        {
            int _adr_lut_adr = 0;
            int _adr_lut_data_lsb = 0;
            int _adr_lut_data_msb = 0;
            int _idx0 = 0;
            int _data0_lsb = 0;
            int _data0_msb = 0;
            int _idx1 = 0;
            int _data1_lsb = 0;
            int _data1_msb = 0;

	        sscanf(line, "%s %x %x %x %x %x %x %x %x", 
	            _dummy, &_dev, _dummy, 
	            &_idx0, &_data0_lsb, &_data0_msb, 
	            &_idx1, &_data1_lsb, &_data1_msb);
	        
	        get_lut_addrs(_adr, &_adr_lut_adr, &_adr_lut_data_lsb, &_adr_lut_data_msb);
		        #if defined(TRACE_IT)
	        printf("lut %x--adr:0x%x, _adr_lut_data_lsb:0x%x, _adr_lut_data_msb:0x%x\n",
	            _tag,_adr_lut_adr, _adr_lut_data_lsb, _adr_lut_data_msb);
	        #endif
	        
	        #if 1
    		write_to_apb_bus(_dev/2, _adr_lut_adr, _idx0);
	        write_to_apb_bus(_dev/2, _adr_lut_data_lsb, _data0_lsb);
	        write_to_apb_bus(_dev/2, _adr_lut_data_msb, _data0_msb);
    		write_to_apb_bus(_dev/2, _adr_lut_adr, _idx1);
	        write_to_apb_bus(_dev/2, _adr_lut_data_lsb, _data1_lsb);
	        write_to_apb_bus(_dev/2, _adr_lut_data_msb, _data1_msb);
	        #endif
	    }
    }
	else if(0 == strcmp("exit", line))
	{
		_exit(0);
	}
	else if(0 == strncmp("ping_udp_i2c", line, 4))
	{
		sprintf(line, "pong_udp_i2c");
	}
	else
	{
		sprintf(line, "failure unknown command\n");
	}
   
	return 0;
}

int	i2c_responder(char * line)
{
	char	_dummy[10];
	int	_dev,_adr,_val;

#if defined(TRACE_IT)
	fprintf(stderr,"Running i2c_responder\n");
#endif
	if(0 == strncmp("read", line, 4))
	{
		int	_val;

#if defined(TRACE_IT)
		fprintf(stderr, "read command\n");
#endif

		sscanf(line, "%s %x %x", _dummy, &_dev, &_adr);

#if defined(TRACE_IT)
		fprintf(stderr, "dev = %x adr = %x\n", _dev, _adr);
#endif

#if 1
		if(0 == read_from_i2c_bus(
				 _dev/2, _adr, &_val))
		{
			sprintf(line, "read_result %2.2x:%2.2x        %2.2x\n", _dev, _adr, _val);
					
#if defined(TRACE_IT)
			fprintf(stderr, "responce length = %d\n", strlen(line));
			fprintf(stderr, "%s\n", line);
			fprintf(stderr, "0123456789012345678901234567890\n");
#endif	
		}
		else
		{
#if defined(TRACE_IT)
			fprintf(stderr, "Failed\n");
#endif
			sprintf(line, "failed\n");
		}
#endif
	}
	else if(0 == strncmp("write", line, 5))
	{
#if defined(TRACE_IT)
		fprintf(stderr, "write command\n");
#endif

		sscanf(line, "%s %x %x %x", _dummy, &_dev, &_adr, &_val);

#if defined(TRACE_IT)
		fprintf(stderr, "dev = %x adr = %x val = %x\n", _dev, _adr, _val);
#endif

		if(0 != write_to_i2c_bus(_dev/2, _adr, _val))
		{
			sprintf(line, "failure\n");
		}
		else
		{
			sprintf(line, "success %2.2x:%2.2x=%2.2x\n", _dev, _adr, _val);
		}
	}
	else if(0 == strcmp("exit", line))
	{
		_exit(0);
	}
	else if(0 == strncmp("ping_udp_i2c", line, 4))
	{
		sprintf(line, "pong_udp_i2c");
	}
	else
	{
		sprintf(line, "failure unknown command\n");
	}
#if defined(TRACE_IT)
	fprintf(stderr,"Done responder\n");
#endif

	return 0;
}

static int	read_std(char *line)
{
	// The client will respond to '>' greeting

	printf(">");
	fflush(stdout);

	scanf("%[^\n]", line);
	getchar();

	return 0;
}

static int	write_std(char *line)
{
	printf("%s", line);
	fflush(stdout);

	return 0;
}


#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

int                     sockfd;
struct sockaddr_in      serv_addr; 
struct sockaddr_in		cli_addr;
int						cli_len;

static int read_udp(char *line)
{
	int	_res;

#if defined(TRACE_IT)
	fprintf(stderr,"Reading UDP\n");
#endif

	cli_len = sizeof(cli_addr);
    _res = recvfrom(sockfd, line, 200, 0, (struct sockaddr *)&cli_addr, &cli_len);
   	if (_res < 0)
	{
		fprintf(stderr, "dg_echo: recvfrom error\n");
		return 1;
	}

#if defined(TRACE_IT)
    fprintf(stderr, "cmd len: %d\n", _res);
	fprintf(stderr, "Got %s\n", line);
#endif

	return 0;
}

static int write_udp(char *line)
{
#if defined(TRACE_IT)
	fprintf(stderr,"Writing UDP\n");
#endif

    if (sendto(sockfd, line, strlen(line), 0, (struct sockaddr *)&cli_addr, cli_len) != strlen(line))
	{
        fprintf(stderr, "dg_echo: sendto error\n");
		return 1;
	}
#if defined(TRACE_IT)
	fprintf(stderr,"Written UDP\n");
#endif
	return 0;
}

#define DEV_ISP 0x38
#define DEV_EXT 0x39 
#define DEV_SENSOR 0x50
int main(int argc, char **argv)
{
	char  _line[200];
	int	net = 0;

	if((argc == 2) && (0 == strcmp(argv[1], "-n")))
	{
		fprintf(stderr, "Using UDP\n");
		net = 1;

    	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		{
        	fprintf(stderr, "server: can't open datagram socket\n");
			return 1;
		}
    	memset((char *) &serv_addr, 0, sizeof(serv_addr));
    	memset((char *) &cli_addr, 0, sizeof(serv_addr));
    	serv_addr.sin_family      = AF_INET;
    	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    	serv_addr.sin_port        = htons(8000);

#if defined(TRACE_IT)
		fprintf(stderr,"Binding\n");
#endif
    	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		{
        	fprintf(stderr,"server: can't bind local address\n");
			close(sockfd);
			return 1;
		}
#if defined(TRACE_IT)
		fprintf(stderr,"Binded\n");
#endif
	}

	for(;;)
	{
		if(0 == net)
			read_std(_line);
		else
			read_udp(_line);

        {
            char _dummy[10];
            int _dev;


        	sscanf(_line, "%s %x", _dummy, &_dev);
            if ((DEV_ISP == _dev) || (DEV_EXT == _dev))
            {   
                apb_responder(_line);
            }
            else if(DEV_SENSOR == _dev)
            {
        		i2c_responder(_line);
            }
            else
            {
                
            }
        }
        
		if(0 == net)
			write_std(_line);
		else
			write_udp(_line);
	}
	return 0;
}