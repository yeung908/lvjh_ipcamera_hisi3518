
ifeq ($(PARAM_FILE), )
		PARAM_FILE:=../../Makefile.param
		include $(PARAM_FILE)
endif

obj-m := sil9024.o
sil_9024-y += sil9024.o

ifeq ($(CONFIG_GPIO_I2C),Y)
EXTRA_CFLAGS += -DHI_GPIO_I2C
EXTRA_CFLAGS+=-I$(PWD)/../gpio-i2c
else
EXTRA_CFLAGS += -DHI_I2C
EXTRA_CFLAGS+=-I$(PWD)/../hi_i2c
endif

EXTRA_CFLAGS += -D$(HI_FPGA)


default:
	@make -C $(LINUX_ROOT) M=$(PWD) modules 
#cp sil9024.ko ../../../user 
clean: 
	@make -C $(LINUX_ROOT) M=$(PWD) clean 
#rm ../../../user/sil9024.ko



