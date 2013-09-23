ifeq ($(PARAM_FILE), )
		PARAM_FILE:=../../Makefile.param
		include $(PARAM_FILE)
endif 

obj-m := gpioi2c_ex.o
gpioi2c-y += gpioi2c_ex.o

EXTRA_CFLAGS += -D$(HI_FPGA)
 
default:	
	$(CC) -g -Wall -o i2c_read_ex i2c_read_ex.c strfunc.c	
	$(CC) -g -Wall -o i2c_write_ex i2c_write_ex.c	strfunc.c
	@make -C $(LINUX_ROOT) M=$(PWD) modules 
clean:
	rm *.o i2c_read_ex i2c_write_ex -rf
	@make -C $(LINUX_ROOT) M=$(PWD) clean

