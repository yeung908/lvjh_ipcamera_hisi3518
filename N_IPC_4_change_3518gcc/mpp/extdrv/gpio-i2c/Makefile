
ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../Makefile.param
	include $(PARAM_FILE)
endif
			
obj-m := gpioi2c.o
gpioi2c-y += gpio_i2c.o

EXTRA_CFLAGS += -D$(HI_FPGA) 
 
default:	
	@$(CC) -g -Wall -o i2c_read i2c_read.c strfunc.c	
	@$(CC) -g -Wall -o i2c_write i2c_write.c strfunc.c
	@make -C $(LINUX_ROOT) M=$(PWD) modules
clean:
	@rm *.o i2c_read i2c_write -rf
	@make -C $(LINUX_ROOT) M=$(PWD) clean
