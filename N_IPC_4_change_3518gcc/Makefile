# Cross Compile Tools
CC 			:=  arm-hisiv100nptl-linux-gcc -g
AR			:= arm-hisiv100nptl-linux-ar

# MPP
MPP_VERSION				:= V1.1.2.3

MPP_INSTALL_PATH			:= /usr/local/hi3511/Hi3511_VSSDK_

# Macro Of Platform
#CFLAGS		:= -DHI3511 

# Macro of Media Library
LIB_PATH	:= $(MPP_INSTALL_PATH)$(MPP_VERSION)/mpp/lib.rel
LIBS 		:=	$(LIB_PATH)/libmpi.a \
				$(LIB_PATH)/lib_VoiceEngine.a \
 				$(LIB_PATH)/lib_amr_spc.a \
				$(LIB_PATH)/lib_amr_fipop.a \
				$(LIB_PATH)/lib_aec.a \
				$(LIB_PATH)/libloadbmp.a \
				$(LIB_PATH)/libtde.a \
				$(LIB_PATH)/lib_aacdec.a \
				$(LIB_PATH)/lib_aacenc.a \
# Macro Of Header
CFLAGS		+= -I $(MPP_INSTALL_PATH)$(MPP_VERSION)/pub/include
CFLAGS		+= -I $(MPP_INSTALL_PATH)$(MPP_VERSION)/mpp/include
CFLAGS		+= -I $(MPP_INSTALL_PATH)$(MPP_VERSION)/mpp/include/mkp

# Macro Of Functions
CFLAGS		+= -DMPP_V1_1_2_3
CFLAGS		+= -DHD_CMOS
CFLAGS		+= -DRS_485
CFLAGS		+= -DRECORD
CFLAGS		+= -DWIFI_RALINK
CFLAGS		+= -DMOBILE_VIEW
CFLAGS		+= -DDEBUG
#CFLAGS		+= -DRECORD_TEST
CFLAGS		+= -DENC_DEBUG
CFLAGS		+= -DYIYUAN
#CFLAGS		+= -FULL_PACKET
# Macro of Test

# Macro Of Library
#LDFLAGS		:= -static
LDFLAGS		+= -lpthread -lm -Wall
LDFLAGS		+= -L ./libs_HI3511/ -lnetSDK 
LDFLAGS		+= -L ./libs_HI3511/ -laudioIn -laudioOut -laudioEnc -laudioDec -lvideoIn -lvideoEnc
LDFLAGS		+= $(LIBS) 
LDFLAGS		+= -L ./libs_HI3511/ -lftp -ltftp -lsmtp
LDFLAGS		+= -L ./libs_HI3511/ -larping -lcrc -ldhcp -lmtd -lnetTools -lsysCheck
LDFLAGS		+= -L ./libs_HI3511/ -lrecordSDK

# Object path configure
OBJSPATH 	:= ./debug_HI3511
OBJS 		:= $(wildcard $(OBJSPATH)/*.o)

# Specific Rules
.PHONY: all

all:
	$(if $(shell find $(OBJSPATH)), $(shell cd ./), $(shell mkdir $(OBJSPATH)))

	cd ./recordsdk; $(MAKE) -f Makefile.HI3511
	cd ./netSDK; $(MAKE) -f Makefile.HI3511
	cd ./arping; $(MAKE) -f Makefile.HI3511	
	cd ./crc; $(MAKE) -f Makefile.HI3511
	cd ./mtd; $(MAKE) -f Makefile.HI3511
	cd ./sysCheck; $(MAKE) -f Makefile.HI3511
	cd ./netTools; $(MAKE) -f Makefile.HI3511	
	cd ./dhcp; $(MAKE) -f Makefile.HI3511	
	cd ./ftp; $(MAKE) -f Makefile.HI3511
	cd ./tftp; $(MAKE) -f Makefile.HI3511
	cd ./smtp; $(MAKE) -f Makefile.HI3511
	cd ./audioDec; $(MAKE) -f Makefile.HI3511
	cd ./audioEnc; $(MAKE) -f Makefile.HI3511
	cd ./audioIn; $(MAKE) -f Makefile.HI3511
	cd ./audioOut; $(MAKE) -f Makefile.HI3511
	cd ./videoEnc; $(MAKE) -f Makefile.HD
	cd ./videoIn; $(MAKE) -f Makefile.HD
	
	cd ..	
	
	$(CC) -c audioDecAppModule.c -o $(OBJSPATH)/audioDecAppModule.o $(CFLAGS)
	$(CC) -c audioEncAppModule.c -o $(OBJSPATH)/audioEncAppModule.o $(CFLAGS)
	$(CC) -c videoEncAppModule.c -o $(OBJSPATH)/videoEncAppModule.o $(CFLAGS)
	$(CC) -c audioStream.c -o $(OBJSPATH)/audioStream.o $(CFLAGS)
	$(CC) -c alarmProc.c -o $(OBJSPATH)/alarmProc.o $(CFLAGS)
	$(CC) -c com.c -o $(OBJSPATH)/com.o $(CFLAGS)
	$(CC) -c util.c -o $(OBJSPATH)/util.o $(CFLAGS)
	$(CC) -c param.c -o $(OBJSPATH)/param.o $(CFLAGS)
	$(CC) -c watchDog.c -o $(OBJSPATH)/watchDog.o $(CFLAGS)
	$(CC) -c probe.c -o $(OBJSPATH)/probe.o $(CFLAGS)
	$(CC) -c searchDevice.c -o $(OBJSPATH)/searchDevice.o $(CFLAGS)
	$(CC) -c talkModule.c -o $(OBJSPATH)/talkModule.o $(CFLAGS)
	$(CC) -c vadcDrv.c -o $(OBJSPATH)/vadcDrv.o $(CFLAGS)
	$(CC) -c rtc.c -o $(OBJSPATH)/rtc.o $(CFLAGS)
	$(CC) -c ptz.c -o $(OBJSPATH)/ptz.o $(CFLAGS)
	$(CC) -c dvsInit.c -o $(OBJSPATH)/dvsInit.o $(CFLAGS)
	$(CC) -c miscDrv.c -o $(OBJSPATH)/miscDrv.o $(CFLAGS)
	$(CC) -c indicator.c -o $(OBJSPATH)/indicator.o $(CFLAGS)
	$(CC) -c ircut.c -o $(OBJSPATH)/ircut.o $(CFLAGS)
	$(CC) -c update.c -o $(OBJSPATH)/update.o $(CFLAGS)
	$(CC) -c comTransfer.c -o $(OBJSPATH)/comTransfer.o $(CFLAGS)
	$(CC) -c session.c -o $(OBJSPATH)/session.o $(CFLAGS)
	$(CC) -c dvs.c -o $(OBJSPATH)/dvs.o $(CFLAGS)
	$(CC) -c pppoe.c -o $(OBJSPATH)/pppoe.o $(CFLAGS)
	$(CC) -c ddns.c -o $(OBJSPATH)/ddns.o $(CFLAGS)
	$(CC) -c ntpSetup.c -o $(OBJSPATH)/ntpSetup.o $(CFLAGS)
	$(CC) -c wlanSetup.c -o $(OBJSPATH)/wlanSetup.o $(CFLAGS)
	$(CC) -c snapshot.c -o $(OBJSPATH)/snapshot.o $(CFLAGS)
	$(CC) -c acodecDrv.c -o $(OBJSPATH)/acodecDrv.o $(CFLAGS)
	$(CC) -c 3g.c -o $(OBJSPATH)/3g.o $(CFLAGS)
	$(CC) -c upnp.c -o $(OBJSPATH)/upnp.o $(CFLAGS)
	$(CC) -c cgi_param.c -o $(OBJSPATH)/cgi_param.o $(CFLAGS)
	
	@echo $(OBJS)

# IPCAM
	$(CC) $(CFLAGS) $(OBJS) -o $(OBJSPATH)/gun_hd.out $(LDFLAGS)	

clean: 
	cd ./netSDK; $(MAKE) -f Makefile.HI3511 clean
	cd ./ftp; $(MAKE) -f Makefile.HI3511 clean
	cd ./tftp; $(MAKE) -f Makefile.HI3511 clean
	cd ./smtp; $(MAKE) -f Makefile.HI3511 clean
	cd ./audioDec; $(MAKE) -f Makefile.HI3511 clean
	cd ./audioEnc; $(MAKE) -f Makefile.HI3511 clean
	cd ./audioIn; $(MAKE) -f Makefile.HI3511 clean
	cd ./audioOut; $(MAKE) -f Makefile.HI3511 clean
	cd ./videoEnc; $(MAKE) -f Makefile.HD clean
	cd ./videoIn; $(MAKE) -f Makefile.HD clean

	rm -f $(OBJSPATH)/*.o $(OBJSPATH)/*.a $(OBJSPATH)/*.out
	rm -f *~
	rmdir $(OBJSPATH)
