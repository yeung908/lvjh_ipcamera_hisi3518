ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../Makefile.param
	include $(PARAM_FILE)
endif

obj-m := ssp_pana.o
ssp-y += hi_ssp.o

all:
	$(CC) -g -Wall -o ssp_read ssp_read.c strfunc.c	
	$(CC) -g -Wall -o ssp_write ssp_write.c	strfunc.c
	make -C $(LINUX_ROOT) M=$(PWD) modules

clean:
	make -C $(LINUX_ROOT) M=$(PWD) clean
	rm -f  ssp_read ssp_write

