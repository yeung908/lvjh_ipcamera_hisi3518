#
# sensor lib Makefile
#

ifeq ($(PARAM_FILE), )
	PARAM_FILE:=../../../../Makefile.param
	include $(PARAM_FILE)
endif

ifeq ($(MPP_BUILD), y)
EXT_PATH := $(SDK_PATH)/mpp/$(EXTDRV)
else
EXT_PATH := $(SDK_PATH)/mpp/extdrv
endif
ISP_PATH := $(SDK_PATH)/mpp/component/isp
LIBPATH = $(ISP_PATH)/lib
OBJPATH = ./obj

ARFLAGS = rcv
ARFLAGS_SO = -shared -fPIC -o
CFLAGS  = -Wall
CFLAGS  += -O2

ifeq ($(HIGDB),HI_GDB)
CFLAGS += -g
endif

BUS_DIR := $(EXT_PATH)/ssp-pana/

ISP_INC := $(ISP_PATH)/include
INC := -I$(BUS_DIR) -I$(REL_INC) -I$(ISP_INC)

COMPILE = $(CC) $(CFLAGS) $(DFLAGS) -lm

$(OBJPATH)/%.o: ./%.c
	@[ -e $(LIBPATH) ] || mkdir $(LIBPATH)
	@[ -e $(OBJPATH) ] || mkdir $(OBJPATH)
	@$(COMPILE) -o $@ -c $< $(INC)

SRCS = $(wildcard ./*.c)
OBJS = $(SRCS:%.c=%.o)
OBJS := $(OBJS:./%=obj/%)

TARGETLIB := $(LIBPATH)/libsns_mn34041.a
TARGETLIB_SO := $(LIBPATH)/libsns_mn34041.so

all:$(TARGETLIB)
$(TARGETLIB):$(OBJS)
	@mkdir -p $(LIBPATH)
	@($(AR) $(ARFLAGS) $(TARGETLIB) $(OBJS))
	@($(CC) $(ARFLAGS_SO) $(TARGETLIB_SO) $(OBJS))

clean:
	@$(RM) -rf $(TARGETLIB) $(OBJS)
	@$(RM) -rf $(LIBPATH) $(OBJPATH)
