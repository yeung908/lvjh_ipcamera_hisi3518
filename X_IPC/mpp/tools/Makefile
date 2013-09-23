ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../Makefile.param
     include $(PARAM_FILE)
endif

SRC  := $(wildcard *.c)
OBJ  := $(SRC:%.c=%.o)

INC_FLAGS := -I$(REL_INC)
CFLAGS += -Wall -g -lpthread $(INC_FLAGS)
LIBS += $(REL_LIB)/libisp.a
LIBS += $(REL_LIB)/libmpi.a

ifneq ($(HIARCH),hi3518)
LIBS += $(REL_LIB)/libhdmi.a
else
CFLAGS += -DHI_3518	
endif

# sensor input type: ar130 mn34031
SNS:=$(MAKECMDGOALS)

ifeq ($(SNS),)
SNS:=ar130
endif

ifeq ($(SNS), ar130)
LIBS += $(REL_LIB)/libsns_ar0130_720p.a
CFLAGS += -DSNS_AR0130
endif

ifeq ($(SNS), 9m034)
LIBS += $(REL_LIB)/libsns_9m034.a
CFLAGS += -DSNS_AR0130
endif

ifeq ($(SNS), imx104)
LIBS += $(REL_LIB)/libsns_imx104.a
CFLAGS += -DSNS_IMX104
endif

ifeq ($(SNS), mn34031)
LIBS += $(REL_LIB)/libsns_mn34031_720p.a
CFLAGS += -DSNS_MN34031
endif

ifeq ($(SNS), ov9712)
LIBS += $(REL_LIB)/libsns_ov9712.a
CFLAGS += -DSNS_OV9712
endif

TARGET := $(OBJ:%.o=%)

.PHONY : clean all $(SNS)

all: $(TARGET)

$(TARGET):%:%.o 
	$(CC) $(CFLAGS)  -o $@ $^ $(LIBS) $(AUDIO_LIBA) 
	@rm *.o
clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ)
ifneq ($(SNS), clean)
$(SNS): all
	@echo
endif
