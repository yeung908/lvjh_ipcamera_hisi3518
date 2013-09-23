# Hisilicon Hi3516 sample Makefile

include ../Makefile.param
#ifeq ($(SAMPLE_PARAM_FILE), )
#     SAMPLE_PARAM_FILE:=../Makefile.param
#     include $(SAMPLE_PARAM_FILE)
#endif

# target source
SRC  := $(wildcard *.c) 
OBJ  := $(SRC:%.c=%.o)
MPI_LIBS := $(REL_LIB)/libmpi.a
#MPI_LIBS += $(REL_LIB)/libhdmi.a
MPI_LIBS += $(REL_LIB)/libtde.a
TARGET := $(OBJ:%.o=%)
.PHONY : clean all

all: $(TARGET)

$(TARGET):%:%.o $(COMM_OBJ)
	$(CC) $(CFLAGS) -lpthread -lm -o $@ $^ $(SENSOR_LIBS) $(MPI_LIBS)  $(AUDIO_LIBA)
	#@cp $(TARGET) ../../../ko_hi3518/

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ)
	@rm -f $(COMM_OBJ)

cleanstream:
	@rm -f *.h264
	@rm -f *.jpg
	@rm -f *.mjp
	@rm -f *.mp4
