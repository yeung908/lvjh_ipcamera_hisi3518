ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../Makefile.param
	include $(PARAM_FILE)
endif

obj-m := tw2865.o
tw_2865-y += tw2865.o

EXTRA_CFLAGS += -D$(HI_FPGA)
#EXTRA_CFLAGS += -DHI_I2C
EXTRA_CFLAGS += -DHI_GPIO_I2C

EXTRA_CFLAGS+=-I$(PWD)/../gpio-i2c
EXTRA_CFLAGS+=-I$(PWD)/../hi_i2c

default:
	make -C $(LINUX_ROOT) M=$(PWD) modules 
clean: 
	make -C $(LINUX_ROOT) M=$(PWD) clean
