ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../Makefile.param
	include $(PARAM_FILE)
endif

obj-m := hi_i2c.o
hi_i2c-y += hii2c.o

all:
	$(CC) -g -Wall -o i2c_read i2c_read.c	strfunc.c
	$(CC) -g -Wall -o i2c_write i2c_write.c	strfunc.c
	make -C $(LINUX_ROOT) M=$(PWD) modules

clean:
	make -C $(LINUX_ROOT) M=$(PWD) clean
	rm -f  i2c_read i2c_write

