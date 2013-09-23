#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#include "audioOutModule.h"

#include "hi_type.h"
#include "hi_debug.h"
#include "hi_comm_sys.h"
#include "hi_comm_aio.h"
#include "hi_comm_aenc.h"
#include "hi_comm_adec.h"
#include "mpi_sys.h"
#include "mpi_ai.h"

static int g_Hi3511_aout_pub_attr_flag = 0;
static int g_Hi3511_aout_open_flag = 0;
static int g_Hi3511_aout_standard = 0;

int Hi3511AoutOpen(int nChannel)
{
	if (nChannel<0 || nChannel>=MAX_AOUT_CHANNEL)
	{
		return -1;	
	}		
	
	return 0; 
} 

int Hi3511AoutClose(int nChannel)
{
	
	HI_S32 s32ret;
	AIO_ATTR_S stAttr;
	
	
	if (nChannel<0 || nChannel>=MAX_AOUT_CHANNEL)
	{
		return -1;	
	}
	
	printf("audio out Close func  \n");//add code by liujw
	s32ret = HI_MPI_AO_DisableChn(0, 0);
	if(HI_SUCCESS != s32ret)
	{
		printf("Disable ao chn %d err:0x%x\n", nChannel, s32ret);
		//return -1;
	}
	else
	printf("HI_MPI_AO_DisableChn OK\n");
	
	s32ret = HI_MPI_AO_Disable(0);
	if(HI_SUCCESS != s32ret)
	{
		printf("Disable ao dev %d err:0x%x\n", 0, s32ret);
		//return -1;
	}
	else
	printf("HI_MPI_AO_Disable OK\n");
	
	g_Hi3511_aout_open_flag = 0;	//add by liujw
	
	return 0;
}

int Hi3511AoutSetup(int nChannel, void *param)
{
	HI_S32 s32ret;
	AIO_ATTR_S stAttr;

	stAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
	stAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;
	stAttr.enSoundmode = AUDIO_SOUND_MODE_MOMO;
	stAttr.enWorkmode =AIO_MODE_I2S_MASTER ;//AIO_MODE_I2S_SLAVE
	
	stAttr.u32EXFlag = 0;
	//stAttr.u32FrmNum = 5;
	stAttr.u32FrmNum = 10;//add by liujw
	stAttr.u32PtNumPerFrm = 160;
    
	s32ret = HI_MPI_AO_SetPubAttr(0, &stAttr);
	if(HI_SUCCESS != s32ret)
	{
		printf("set ao %d attr err:0x%x::%s  %d\n", 0, s32ret, __FILE__, __LINE__);
		return -1;
	}
    #if 1   //add code by liujw 12-3-2
	s32ret = HI_MPI_AO_Enable(0);
	if(HI_SUCCESS != s32ret)
	{
		printf("enable ao dev %d err:0x%x\n", 0, s32ret);
		return -1;
	}
	#endif
	s32ret = HI_MPI_AO_EnableChn(0, 0);
	if(HI_SUCCESS != s32ret)
	{
		printf("enable ai chn %d err:0x%x\n", nChannel, s32ret);
		return -1;
	}
	g_Hi3511_aout_open_flag = 1;	//add by liujw
	
	return 0;
}

int Hi3511AoutGetSetup(int nChannel, void *param)
{
	return 0;
}

int Hi3511AoutStart(int nChannel)
{	
	if (nChannel<0 || nChannel>=MAX_AOUT_CHANNEL)
	{
		return -1;
	}	
	if (g_Hi3511_aout_open_flag == 0)
	{
		return -1;
	}

	return 0;
}

int Hi3511AoutStop(int nChannel)
{
	if (nChannel<0 || nChannel>=MAX_AOUT_CHANNEL)
	{
		return -1;
	}	
	if (g_Hi3511_aout_open_flag == 0)
	{
		return -1;
	}
	
	return 0;
}

int Hi3511AoutGetStream(int nChannel, void *stream, int *size)
{
	HI_S32 s32ret;
	
	if (stream==NULL || size==NULL)
	{
		return -1;
	}
	if (nChannel<0 || nChannel>=MAX_AOUT_CHANNEL)
	{
		return -1;
	}	
	if (g_Hi3511_aout_open_flag == 0)
	{
		return -1;
	}
	
	s32ret = HI_MPI_AO_SendFrame(0, 0, (AUDIO_FRAME_S *)stream, HI_IO_BLOCK);
	if (HI_SUCCESS != s32ret)
	{   
		printf("ao send frame err:0x%x\n",s32ret);	//add by liujw 12-3-3
		return NULL;
	}
	//printf("HI_MPI_AO_SendFrame ao send frame ok! \n"); //add by liujw 12-3-3
	*size = 0;
	
	return 0;
}

int Hi3511AoutReleaseStream(int nChannel)
{
	if (nChannel<0 || nChannel>=MAX_AOUT_CHANNEL)
	{
		return -1;
	}	
	if (g_Hi3511_aout_open_flag == 0)
	{
		return -1;
	}
	
	return 0;
}

static struct aoutModuleInfo Hi3511AoutInfoStruct = 
{
	open:           Hi3511AoutOpen,
	close:          Hi3511AoutClose,
	setup:			Hi3511AoutSetup,
	getSetup:		Hi3511AoutGetSetup,
	start:          Hi3511AoutStart,
	stop:			Hi3511AoutStop,
	getStream:		Hi3511AoutGetStream,
	releaseStream:	Hi3511AoutReleaseStream,
	
};

aoutModuleInfo_t Hi3511AoutInfo = &Hi3511AoutInfoStruct;
