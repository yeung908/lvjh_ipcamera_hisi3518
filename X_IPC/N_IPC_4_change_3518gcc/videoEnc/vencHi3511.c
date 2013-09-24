#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include "videoEncModule.h"

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vi.h"
#include "hi_comm_vpp.h"
#include "hi_comm_venc.h"
#include "hi_comm_md.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"
#include "mpi_vpp.h"
#include "mpi_venc.h"


//#define VENC_DEBUG				1

#define MAX_BUFF_SIZE			524288
#define MAX_MOBILE_SIZE			256000

static const HI_U8 g_SOI[2] = {0xFF, 0xD8};
static const HI_U8 g_EOI[2] = {0xFF, 0xD9};

static int g_hi3511_venc_type[MAX_ENC_CHANNEL][MAX_CHANNEL_ENC_NUM];

static int g_hi3511_h264_chn_flag[MAX_ENC_CHANNEL][MAX_CHANNEL_ENC_NUM];
static int g_hi3511_jpeg_chn_flag[MAX_ENC_CHANNEL];
char g_hi3511_h264_buffer[MAX_CHANNEL_ENC_NUM][MAX_BUFF_SIZE];
char g_hi3511_jpeg_buffer[MAX_BUFF_SIZE];

// Add the code by Jerry.Zhuang, 2010-12-14
static int g_hi3511_mobile_chn_flag[MAX_ENC_CHANNEL];
char g_hi3511_mobile_buffer[MAX_ENC_CHANNEL][MAX_MOBILE_SIZE];

// Add the code by Jerry.Zhuang, 2009-01-31
static int g_hi3511_h264_width[MAX_ENC_CHANNEL];
static int g_hi3511_h264_height[MAX_ENC_CHANNEL];

// OSD Font Library
extern unsigned int HanZiTableMask[16];
extern unsigned long long HanZi16Lib[];
extern unsigned long long Ansc16Lib[];
char g_osd_buffer[30000];

// OSD
static g_hi3511_osd_flag[MAX_ENC_CHANNEL][MAX_CHANNEL_ENC_NUM][MAX_OSD_NUM];
REGION_HANDLE g_hi3511_osd_handle[MAX_ENC_CHANNEL][MAX_CHANNEL_ENC_NUM][MAX_OSD_NUM];
OSD_DATA_PARAM g_hi3511_osd_param[MAX_ENC_CHANNEL][MAX_CHANNEL_ENC_NUM][MAX_OSD_NUM];

static g_hi3511_jpeg_osd_flag[MAX_ENC_CHANNEL][MAX_OSD_NUM];
REGION_HANDLE g_hi3511_jpeg_osd_handle[MAX_ENC_CHANNEL][MAX_OSD_NUM];//modify by zhangjing 2013-04-07

char g_jpeg_osd_buffer[4][30000];    //modify by zhangjing 2013-04-07
OSD_TO_JPGE g_osd_param_to_jpeg[MAX_OSD_NUM];  //add by zhangjing


static g_hi3511_mobile_osd_flag[MAX_ENC_CHANNEL];
REGION_HANDLE g_hi3511_mobile_osd_handle[MAX_ENC_CHANNEL];

// LOGO
static g_hi3511_logo_flag[MAX_ENC_CHANNEL][MAX_CHANNEL_ENC_NUM];
REGION_HANDLE g_hi3511_logo_handle[MAX_ENC_CHANNEL][MAX_CHANNEL_ENC_NUM];
LOGO_DATA_PARAM g_hi3511_logo_param[MAX_ENC_CHANNEL];
char g_logo_buffer[30000];

// MASK
static g_hi3511_mask_flag[MAX_ENC_CHANNEL][MAX_MASK_NUM];
REGION_HANDLE g_hi3511_mask_handle[MAX_ENC_CHANNEL][MAX_MASK_NUM];

// MD
int g_hi3511_md_flag[MAX_ENC_CHANNEL];
unsigned char g_hi3511_md_value[MAX_ENC_CHANNEL];
unsigned char g_hi3511_md_mask[MAX_ENC_CHANNEL][7500];	// Add the code by Jerry.Zhuang, 2009-01-16
int g_hi3511_md_count[MAX_ENC_CHANNEL];

static int g_hi3511_h264_error_flag = 0;
static int g_hi3511_h264_timeout_count[MAX_ENC_CHANNEL][MAX_CHANNEL_ENC_NUM];

// 宏块转换
int macroCount(char *pSrc, int num)
{
	int i = 0;
	int count = 0;
	
	for (i=0; i<num; i++)
	{
		if (pSrc[i] == 1)
		{
			count++;
		}
	}
	
	return count;
}

int macroCovert(char *src, char *dest, int value, int flag)
{
	int i = 0;
	int j = 0;
	int n = 0;
	char temp = 0;
	
	switch (flag)
	{
	case 1: 	// PAL, D1
		{
			for (j=0; j<9; j++)
			{
				for (i=0; i<11; i++)
				{
					if (src[j*11+i+0])
					{
						temp = value;
					}
					else
					{
						temp = 0;
					}

					dest[j*180+i*4+0] = temp;
					dest[j*180+i*4+1] = temp;
					dest[j*180+i*4+2] = temp;
					dest[j*180+i*4+3] = temp;
				
					if (i==10)
					{
						dest[j*180+i*4+4] = temp;
					}
				}
				memcpy(dest+(j*4+1)*45, dest+(j*4+0)*45, 45);
				memcpy(dest+(j*4+2)*45, dest+(j*4+0)*45, 45);
				memcpy(dest+(j*4+3)*45, dest+(j*4+0)*45, 45);
			}
		}
		break;

	case 2: 	// PAL, HD1
		{
			for (j=0; j<9; j++)
			{
				for (i=0; i<11; i++)
				{
					if (src[j*11+i+0])
					{
						temp = value;
					}
					else
					{
						temp = 0;
					}

					dest[j*90+i*4+0] = temp;
					dest[j*90+i*4+1] = temp;
					dest[j*90+i*4+2] = temp;
					dest[j*90+i*4+3] = temp;

					if (i==10)
					{
						dest[j*90+i*4+4] = temp;
					}
				}
				memcpy(dest+(j*2+1)*45, dest+(j*2+0)*45, 45);
			}
		}
		break;

	case 3: 	// PAL, CIF
		{
			for (j=0; j<9; j++)
			{
				for (i=0; i<11; i++)
				{
					if (src[j*11+i+0])
					{
						temp = value;
					}
					else
					{
						temp = 0;
					}

					dest[j*44+i*2+0] = temp;
					dest[j*44+i*2+1] = temp;
				}
				memcpy(dest+(j*2+1)*22, dest+(j*2+0)*22, 22);
			}
		}
		break;

	case 4: 	// PAL, QCIF
		{
			memcpy(dest, src, 99);
		}
		break;

	case 5: 	// NTSC, D1
		{
			for (j=0; j<9; j++)
			{
				for (i=0; i<11; i++)
				{
					if (src[j*11+i+0])
					{
						temp = value;
					}
					else
					{
						temp = 0;
					}

					dest[j*135+i*4+0] = temp;
					dest[j*135+i*4+1] = temp;
					dest[j*135+i*4+2] = temp;
					dest[j*135+i*4+3] = temp;
				
					if (i == 10)
					{
						dest[j*135+i*4+4] = temp;
					}
				}
				
				// Add the code by Jerry.Zhuang, 2009-01-16
				memcpy(dest+(j*3+1)*45, dest+(j*3+0)*45, 45);
				memcpy(dest+(j*3+2)*45, dest+(j*3+0)*45, 45);
				if (j == 8)
				{
					memcpy(dest+(j*3+3)*45, dest+(j*3+0)*45, 45);
					memcpy(dest+(j*3+4)*45, dest+(j*3+0)*45, 45);
					memcpy(dest+(j*3+5)*45, dest+(j*3+0)*45, 45);
				}
			}
		}
		break;

	case 6: 	// NTSC, HD1
		{
			for (j=0; j<9; j++)
			{
				for (i=0; i<11; i++)
				{
					if (src[j*11+i+0])
					{
						temp = value;
					}
					else
					{
						temp = 0;
					}

					dest[j*90+i*4+0] = temp;
					dest[j*90+i*4+1] = temp;
					dest[j*90+i*4+2] = temp;
					dest[j*90+i*4+3] = temp;

					if (i==10)
					{
						dest[j*90+i*4+4] = temp;
					}
				}
				memcpy(dest+(j*2+1)*45, dest+(j*2+0)*45, 45);
			}
		}
		break;

	case 7: 	// NTSC, CIF
		{
			for (j=0; j<9; j++)
			{
				for (i=0; i<11; i++)
				{
					if (src[j*11+i+0])
					{
						temp = value;
					}
					else
					{
						temp = 0;
					}

					dest[j*44+i*2+0] = temp;
					dest[j*44+i*2+1] = temp;
				}
				memcpy(dest+(j*2+1)*22, dest+(j*2+0)*22, 22);
			}
		}
		break;

	case 8: 	// NTSC, QCIF
		{
			memcpy(dest, src, 99);
		}
		break;
		
	case 12:	// UXVAG 100*75, 11*9 
		{
			for (j=0; j<9; j++)
			{
				for (i=0; i<11; i++)
				{
					if (src[j*11+i+0])
					{
						temp = value;
					}
					else
					{
						temp = 0;
					}

					dest[j*800+i*9+0] = temp;
					dest[j*800+i*9+1] = temp;
					dest[j*800+i*9+2] = temp;
					dest[j*800+i*9+3] = temp;
					dest[j*800+i*9+4] = temp;
					dest[j*800+i*9+5] = temp;
					dest[j*800+i*9+6] = temp;
					dest[j*800+i*9+7] = temp;
					dest[j*800+i*9+8] = temp;
				
					if (i == 10)
					{
						dest[j*800+i*9+9] = temp;
					}
				}
								
				// Add the code by Jerry.Zhuang, 2009-01-16
				memcpy(dest+(j*8+1)*100, dest+(j*8+0)*100, 100);
				memcpy(dest+(j*8+2)*100, dest+(j*8+0)*100, 100);
				memcpy(dest+(j*8+3)*100, dest+(j*8+0)*100, 100);
				memcpy(dest+(j*8+4)*100, dest+(j*8+0)*100, 100);
				memcpy(dest+(j*8+5)*100, dest+(j*8+0)*100, 100);
				memcpy(dest+(j*8+6)*100, dest+(j*8+0)*100, 100);
				memcpy(dest+(j*8+7)*100, dest+(j*8+0)*100, 100);
				
				if (j == 8)
				{
					memcpy(dest+(j*8+8)*100, dest+(j*8+0)*100, 100);
					memcpy(dest+(j*8+9)*100, dest+(j*8+0)*100, 100);
					memcpy(dest+(j*8+10)*100, dest+(j*8+0)*100, 100);
				}
			}			
		}
		break;
		
	case 13:	// XXVGA
		{
			for (j=0; j<9; j++)
			{
				for (i=0; i<11; i++)
				{
					if (src[j*11+i+0])
					{
						temp = value;
					}
					else
					{
						temp = 0;
					}

					dest[j*400+i*7+0] = temp;
					dest[j*400+i*7+1] = temp;
					dest[j*400+i*7+2] = temp;
					dest[j*400+i*7+3] = temp;
					dest[j*400+i*7+4] = temp;
					dest[j*400+i*7+5] = temp;
					dest[j*400+i*7+6] = temp;
				
					if (i == 10)
					{
						dest[j*400+i*7+7] = temp;
						dest[j*400+i*7+8] = temp;
						dest[j*400+i*7+9] = temp;
					}
				}
								
				// Add the code by Jerry.Zhuang, 2009-01-16
				memcpy(dest+(j*5+1)*80, dest+(j*5+0)*80, 80);
				memcpy(dest+(j*5+2)*80, dest+(j*5+0)*80, 80);
				memcpy(dest+(j*5+3)*80, dest+(j*5+0)*80, 80);
				memcpy(dest+(j*5+4)*80, dest+(j*5+0)*80, 80);
			}			
		}		
		break;

	default:
		{
		
		}
		break;
	}

	return 0;
}

int mdMacroConvertExt(char *src, char *dest, int width, int height, int value)
{
	if (src==NULL || dest==NULL || width<=0 || height<=0)
	{
		return -1;
	}
	
	memset(dest, 0, 45*36);

	if (width==720 && height==576)
	{
		macroCovert(src, dest, value, 1);
	}
	if (width==720 && height==288)
	{
		macroCovert(src, dest, value, 2);
	}
	if (width==352 && height==288)
	{
		macroCovert(src, dest, value, 3);
	}
	if (width==176 && height==144)
	{
		macroCovert(src, dest, value, 4);
	}
	if (width==720 && height==480)
	{
		macroCovert(src, dest, value, 5);
	}
	if (width==720 && height==240)
	{
		macroCovert(src, dest, value, 6);
	}
	if (width==352 && height==240)
	{
		macroCovert(src, dest, value, 7);
	}
	if (width==176 && (height==120 || height==112 || height==128))
	{
		macroCovert(src, dest, value, 8);
	}
	if (width==640 && height==480)
	{
		macroCovert(src, dest, value, 9);
	}
	if (width==320 && height==240)
	{
		macroCovert(src, dest, value, 10);
	}
	if (width==160 && (height==120 || height==112 || height==128))
	{
		macroCovert(src, dest, value, 11);
	}
	if (width==1600 && height==1200)
	{
		macroCovert(src, dest, value, 12);
	}
	if (width==1280 && height==720)
	{
		macroCovert(src, dest, value, 13);
	}
	return 0;
}

int RGB888_RGB555(char *in, char *out, int width, int height)
{
	int i = 0;
	int count = 0;
	int value = 0;
	int color = 0;

	count = width*height;

	for (i=0; i<count; i++)
	{
		value = 0;

		value = (in[i*3+0])>>3;
		value += (((in[i*3+1])>>3)<<5);
		value += (((in[i*3+2])>>3)<<10);

		out[i*2+0] = (char)((value>>0)&0xFF);
		out[i*2+1] = (char)((value>>8)&0xFF);
	}

	return 0;
}

int RGB2BGR(char *in, char *out, int width, int height)
{
	int i = 0;
	int count = 0;

	count = width*height;

	for (i=0; i<count; i++)
	{
		out[i*3+0] = in[i*3+2];
		out[i*3+1] = in[i*3+1];
		out[i*3+2] = in[i*3+0];
	}

	return 0;
}

static int putFont(char *outBuf, char *inBuf, int width, int height, int color)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int n = 0;
	int Zcode;
	int Bcode;
	int Len;
	unsigned long long Buf[4]; 
	char *temp = NULL;
	char *buffer = NULL;
	
	if (outBuf==NULL || inBuf==NULL || width<=0 || height<=0)
	{
		return -1;
	}
	
	buffer = inBuf;
	
	memset(outBuf, 60, width*height*3);

	while (buffer[n])
	{		
		if (buffer[n] >= 0x80)
		{
			Zcode = buffer[n]-0xa1;
			Bcode = buffer[n+1]-0xa1;
			Len = (Zcode*94+Bcode)*4;
			Buf[0] = HanZi16Lib[Len+0];
			Buf[1] = HanZi16Lib[Len+1];
			Buf[2] = HanZi16Lib[Len+2];
			Buf[3] = HanZi16Lib[Len+3];
			
			temp = (char *)Buf;

			for (i=0; i<256; i++)
			{
				j = 7-i%8;
				k = 0;
				k = 1<<j;

				if (temp[i/8]&k)
				{
					outBuf[i/16*width*3+n*8*3+i%16*3+0] = (char)((color>>16)&0xFF);
					outBuf[i/16*width*3+n*8*3+i%16*3+1] = (char)((color>>8)&0xFF);
					outBuf[i/16*width*3+n*8*3+i%16*3+2] = (char)((color>>0)&0xFF);					
				}
			}
			n += 2;
		}
		else
		{
			Len = buffer[n]*2;
			Buf[0] = Ansc16Lib[Len+0];
			Buf[1] = Ansc16Lib[Len+1];
			
			temp = (char *)Buf;
			
			for (i=0; i<128; i++)
			{
				j = 7-i%8;
				k = 0;
				k = 1<<j;
				
				if (temp[i/8]&k)
				{
					outBuf[i/8*width*3+n*8*3+i%8*3+0] = (char)((color>>16)&0xFF);
					outBuf[i/8*width*3+n*8*3+i%8*3+1] = (char)((color>>8)&0xFF);
					outBuf[i/8*width*3+n*8*3+i%8*3+2] = (char)((color>>0)&0xFF);
					
				}
			}
			
			n++;
		}
	}

	return 0;
}

static int putFontExt(char *outBuf, char *inBuf, int width, int height, int color)
{
	int i = 0;
	int j = 0;
	int k = 0;
	int n = 0;
	int Zcode;
	int Bcode;
	int Len;
	unsigned long long Buf[4]; 
	char *temp = NULL;
	char *buffer = NULL;
	int value = 0;
	
	if (outBuf==NULL || inBuf==NULL || width<=0 || height<=0)
	{
		return -1;
	}
	
	buffer = inBuf;
	
	memset(outBuf, 0, width*height*3);
					
	value = ((color>>3)&0x1F);
	value |= (((color>>11)&0x1F)<<5);
	value |= (((color>>19)&0x1F)<<10);
	
	// Add the code by Jerry.Zhuang, 2010-01-11
	value |= 0x8000;
	
	while (buffer[n])
	{		
		if (buffer[n] >= 0x80)
		{
			Zcode = buffer[n]-0xa1;
			Bcode = buffer[n+1]-0xa1;
			Len = (Zcode*94+Bcode)*4;
			Buf[0] = HanZi16Lib[Len+0];
			Buf[1] = HanZi16Lib[Len+1];
			Buf[2] = HanZi16Lib[Len+2];
			Buf[3] = HanZi16Lib[Len+3];
			
			temp = (char *)Buf;

			for (i=0; i<256; i++)
			{
				j = 7-i%8;
				k = 0;
				k = 1<<j;

				if (temp[i/8]&k)
				{
					outBuf[i/16*width*2+n*8*2+i%16*2+0] = (char)((value>>0)&0xFF);
					outBuf[i/16*width*2+n*8*2+i%16*2+1] = (char)((value>>8)&0xFF);			
				}
			}
			n += 2;
		}
		else
		{
			Len = buffer[n]*2;
			Buf[0] = Ansc16Lib[Len+0];
			Buf[1] = Ansc16Lib[Len+1];
			
			temp = (char *)Buf;
			
			for (i=0; i<128; i++)
			{
				j = 7-i%8;
				k = 0;
				k = 1<<j;
				
				if (temp[i/8]&k)
				{				
					outBuf[i/8*width*2+n*8*2+i%8*2+0] = (char)((value>>0)&0xFF);
					outBuf[i/8*width*2+n*8*2+i%8*2+1] = (char)((value>>8)&0xFF);
				}
			}
			
			n++;
		}
	}
	
	return 0;
}

int EnableEncode(int VeGroup, int VeChn, int ViDev, int ViChn, VENC_CHN_ATTR_S *pstAttr)
{
	HI_S32 s32Ret = 0;

	printf("EnableEncode: %d %d %d %d\n", VeGroup, VeChn, ViDev, ViChn);

	s32Ret = HI_MPI_VENC_CreateGroup(VeGroup);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_CreateGroup err 0x%x\n",s32Ret);
		return -1;
	}
	
#ifdef BOGUS
	s32Ret = HI_MPI_VENC_BindInput(VeGroup, ViDev, ViChn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_BindInput err 0x%x\n",s32Ret);
		return -1;
	}
#endif /* BOGUS */
	
	s32Ret = HI_MPI_VENC_CreateChn(VeChn, pstAttr, HI_NULL);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_CreateChn err 0x%x\n",s32Ret);
		return -1;
	}
	
	s32Ret = HI_MPI_VENC_RegisterChn(VeGroup, VeChn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_RegisterChn err 0x%x\n",s32Ret);
		return -1;
	}
	
	s32Ret = HI_MPI_VENC_StartRecvPic(VeChn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_StartRecvPic err 0x%x\n",s32Ret);
		return -1;
	}

	return 0;
}

int DisableEncode(int VeGroup, int VeChn)
{
	int nRet = 0;
	printf("DisableEncode: %d %d\n", VeGroup, VeChn);

	nRet = HI_MPI_VENC_StopRecvPic(VeChn);
	printf("HI_MPI_VENC_StopRecvPic: 0x%x\n", nRet);
	nRet = HI_MPI_VENC_UnRegisterChn(VeChn);
	printf("HI_MPI_VENC_UnRegisterChn: 0x%x\n", nRet);
	nRet = HI_MPI_VENC_DestroyChn(VeChn);
	printf("HI_MPI_VENC_DestroyChn: 0x%x\n", nRet);
	nRet = HI_MPI_VENC_DestroyGroup(VeGroup);
	printf("HI_MPI_VENC_DestroyGroup: 0x%x\n", nRet);

	return 0;
}

int EnableSnapshot(int VeGroup, int VeChn, int ViDev, int ViChn, VENC_CHN_ATTR_S *pstAttr)
{
	HI_S32 s32Ret;
	
	printf("EnableSnapshot: %d %d %d %d\n", VeGroup, VeChn, ViDev, ViChn);

	s32Ret = HI_MPI_VENC_CreateGroup(VeGroup);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_CreateGroup err 0x%x\n",s32Ret);
		return -1;
	}
	s32Ret = HI_MPI_VENC_CreateChn(VeChn, pstAttr, HI_NULL);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_CreateChn err 0x%x(%d)\n", s32Ret, __LINE__);
		return -1;
	}

	return 0;
}

int DisableSnapshot(int VeGroup, int VeChn)
{
	int nRet = 0;
	printf("DisableSnapshot: %d %d\n", VeGroup, VeChn);

	nRet = HI_MPI_VENC_DestroyChn(VeChn);
	printf("HI_MPI_VENC_DestroyChn: 0x%x\n", nRet);
	nRet = HI_MPI_VENC_DestroyGroup(VeGroup);
	printf("HI_MPI_VENC_DestroyGroup: 0x%x\n", nRet);

	return 0;
}

int EnableSnapshotExt(int VeGroup, int VeChn, int ViDev, int ViChn, VENC_CHN_ATTR_S *pstAttr)
{
	HI_S32 s32Ret;
	
	printf("EnableSnapshot: %d %d %d %d\n", VeGroup, VeChn, ViDev, ViChn);

	s32Ret = HI_MPI_VENC_CreateGroup(VeGroup);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_CreateGroup err 0x%x\n",s32Ret);
		return -1;
	}
	s32Ret = HI_MPI_VENC_CreateChn(VeChn, pstAttr, HI_NULL);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_CreateChn err 0x%x(%d)\n", s32Ret, __LINE__);
		return -1;
	}
#ifdef BOGUS
	s32Ret = HI_MPI_VENC_BindInput(VeGroup, ViDev, ViChn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_BindInput err 0x%x\n",s32Ret);
		return -1;
	}
	
#endif /* BOGUS */
	s32Ret = HI_MPI_VENC_RegisterChn(VeGroup, VeChn);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_RegisterChn err 0x%x\n",s32Ret);
		return -1;
	}
	
	s32Ret = HI_MPI_VENC_StartRecvPic(VeChn);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_StartRecvPic err 0x%x\n",s32Ret);
		return -1;
	}

	return 0;
}

int DisableSnapshotExt(int VeGroup, int VeChn)
{
	int nRet = 0;
	printf("DisableSnapshot: %d %d\n", VeGroup, VeChn);

	nRet = HI_MPI_VENC_StopRecvPic(VeChn);
	printf("HI_MPI_VENC_StopRecvPic err 0x%x\n", nRet);
	nRet = HI_MPI_VENC_UnRegisterChn(VeChn);
	printf("HI_MPI_VENC_UnRegisterChn err 0x%x\n", nRet);
#ifdef BOGUS
	nRet = HI_MPI_VENC_UnbindInput(VeGroup);
	printf("HI_MPI_VENC_UnbindInput err 0x%x\n", nRet);
#endif /* BOGUS */
	nRet = HI_MPI_VENC_DestroyChn(VeChn);
	printf("HI_MPI_VENC_DestroyChn: 0x%x\n", nRet);
	nRet = HI_MPI_VENC_DestroyGroup(VeGroup);
	printf("HI_MPI_VENC_DestroyGroup: 0x%x\n", nRet);

	return 0;
}

int GetSnapPic(int SnapChn, char *stream, int *size)
{
	HI_S32 s32Ret;
	HI_S32 s32VencFd;
	VENC_CHN_STAT_S stStat;
	VENC_STREAM_S stStream;
	fd_set read_fds;
	char *temp = NULL;
	int offset = 0;
	int i = 0;
	
	MOBILE_UpdateOSD();

	s32VencFd = HI_MPI_VENC_GetFd(SnapChn);
	if(s32VencFd < 0)
	{
		printf("HI_MPI_VENC_GetFd err \n");
		return -1;
	}

	FD_ZERO(&read_fds);
	FD_SET(s32VencFd,&read_fds);
	
	temp = stream;
	
	s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, NULL);
	
	if (s32Ret < 0) 
	{
		printf("select err\n");
		return -1;
	}
	else if (0 == s32Ret) 
	{
		printf("GetSnapPic: time out\n");
		return -1;
	}
	else
	{
		if (FD_ISSET(s32VencFd, &read_fds))
		{
			s32Ret = HI_MPI_VENC_Query(SnapChn, &stStat);
			
			if (s32Ret != HI_SUCCESS) 
			{
				printf("HI_MPI_VENC_Query:0x%x\n",s32Ret);
				return -1;
			}
			
			stStream.pstPack = g_hi3511_jpeg_buffer;			
			stStream.u32PackCount = stStat.u32CurPacks;
			
			s32Ret = HI_MPI_VENC_GetStream(SnapChn, &stStream, HI_TRUE);
			if (HI_SUCCESS != s32Ret) 
			{
				printf("HI_MPI_VENC_GetStream:0x%x\n",s32Ret);
				stStream.pstPack = NULL;
				return -1;
			}
			
			memcpy(temp, g_SOI, sizeof(g_SOI));
			offset += sizeof(g_SOI);

			for (i=0; i< stStream.u32PackCount; i++)
			{
				memcpy(temp+offset, stStream.pstPack[i].pu8Addr[0], stStream.pstPack[i].u32Len[0]);
				offset += stStream.pstPack[i].u32Len[0];
		
				if (stStream.pstPack[i].u32Len[1] > 0)
				{
					memcpy(temp+offset, stStream.pstPack[i].pu8Addr[1], stStream.pstPack[i].u32Len[1]);
					offset += stStream.pstPack[i].u32Len[1];
				}	
			}

			memcpy(temp+offset, g_EOI, sizeof(g_EOI));
			offset += sizeof(g_EOI);
		
			*size = offset;
			
			s32Ret = HI_MPI_VENC_ReleaseStream(SnapChn,&stStream);
			if (s32Ret) 
			{
				printf("HI_MPI_VENC_ReleaseStream:0x%x\n",s32Ret);
				stStream.pstPack = NULL;
				return -1;
			}
			
			stStream.pstPack = NULL;
		}
			
	}
	
	return 0;
}

int JPEG_EnableOSD(unsigned int index)
{
	HI_S32 s32Ret;
	REGION_ATTR_S stRgnAttr;
	REGION_CTRL_PARAM_U param;
	REGION_CRTL_CODE_E enCtrl;
			
	memset(&stRgnAttr, 0, sizeof(REGION_ATTR_S));
	//printf("jpeg get OSD index:%d,x=%d--y=%d--\n",index,g_osd_param_to_jpeg[index].nxPos,g_osd_param_to_jpeg[index].nyPos);
	if (index<0 || index>=MAX_OSD_NUM)
	{
		return -1;
	}
	
	if (g_hi3511_jpeg_osd_flag[0][index]== 1)
	{
		HI_MPI_RGN_Destroy(g_hi3511_jpeg_osd_handle[0][index]);
		g_hi3511_jpeg_osd_flag[0][index] = 0;
	}
		
	stRgnAttr.enType = OVERLAY_REGION;
	stRgnAttr.unAttr.stOverlay.bPublic = HI_FALSE;
	stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
	stRgnAttr.unAttr.stOverlay.stRect.s32X = g_osd_param_to_jpeg[index].nxPos;
	stRgnAttr.unAttr.stOverlay.stRect.s32Y = g_osd_param_to_jpeg[index].nyPos;
	stRgnAttr.unAttr.stOverlay.stRect.u32Width = g_osd_param_to_jpeg[index].ndata_len;
	stRgnAttr.unAttr.stOverlay.stRect.u32Height = 16;
	stRgnAttr.unAttr.stOverlay.u32BgAlpha = 0;	// 128
	stRgnAttr.unAttr.stOverlay.u32FgAlpha = 128;	// 128
	stRgnAttr.unAttr.stOverlay.u32BgColor = 0x0;
	//stRgnAttr.unAttr.stOverlay.VeGroup = 0;
	stRgnAttr.unAttr.stOverlay.VeGroup = 2;
	
	s32Ret = HI_MPI_RGN_Create((g_hi3511_jpeg_osd_handle[0][index]),&stRgnAttr);
	if(s32Ret != HI_SUCCESS)
	{
		g_hi3511_jpeg_osd_flag[0][index] = 0;
		printf("HI_MPI_RGN_Create err %d 0x%x!\n", __LINE__, s32Ret);
		//printf("(%d %d)%d %d %d %d\n", nChannel, nIndex, stRgnAttr.unAttr.stOverlay.stRect.s32X, stRgnAttr.unAttr.stOverlay.stRect.s32Y, stRgnAttr.unAttr.stOverlay.stRect.u32Width, stRgnAttr.unAttr.stOverlay.stRect.u32Height);		
		return -1;
	}
	
	g_hi3511_jpeg_osd_flag[0][index] = 1;
					
	memset(&param, 0, sizeof(REGION_CTRL_PARAM_U));
	param.stBitmap.u32Width = stRgnAttr.unAttr.stOverlay.stRect.u32Width;
	param.stBitmap.u32Height = 16;
	param.stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
	param.stBitmap.pData = g_jpeg_osd_buffer[index];
	
	enCtrl = REGION_SET_BITMAP;		
	s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_jpeg_osd_handle[0][index], enCtrl, &param);
	if (s32Ret != HI_SUCCESS)
	{
		printf("set region bitmap faild 0x%x!\n", s32Ret);
		return -1;
	}
		
	// Add the code by Jerry.Zhuang, 2009-12-10
	enCtrl = REGION_SET_ALPHA0;
	param.u32Alpha = 0;
	s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_jpeg_osd_handle[0][index], enCtrl, &param);
	if (s32Ret != HI_SUCCESS)
	{
		printf("REGION_SET_ALPHA0 0x%x!\n",s32Ret);
		return -1;
	}
	enCtrl = REGION_SET_ALPHA1;
	param.u32Alpha = 128;
	s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_jpeg_osd_handle[0][index], enCtrl, &param);
	if(s32Ret != HI_SUCCESS)
	{
		printf("REGION_SET_ALPHA1 faild 0x%x!\n",s32Ret);
		return -1;
	}
	if(g_osd_param_to_jpeg[index].nShow){
		enCtrl = REGION_SHOW;
		s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_jpeg_osd_handle[0][index], enCtrl, &param);
		if (s32Ret != HI_SUCCESS)
		{
			printf("set region bitmap faild 0x%x!\n",s32Ret);
			return -1;
		}
	}
	return 0;
}

int JPEG_DisableOSD()
{
	int i;
	for(i=0;i<MAX_OSD_NUM;i++){
		if (g_hi3511_jpeg_osd_flag[0][i] == 1)
		{
			HI_MPI_RGN_Destroy(g_hi3511_jpeg_osd_handle[0][i]);
			g_hi3511_jpeg_osd_flag[0][i] = 0;
		}
	}
	return 0;
}

int MOBILE_UpdateOSD()
{
	HI_S32 s32Ret;
	REGION_ATTR_S stRgnAttr;
	REGION_CTRL_PARAM_U param;
	REGION_CRTL_CODE_E enCtrl;
			
	memset(&stRgnAttr, 0, sizeof(REGION_ATTR_S));
	
	if (g_hi3511_mobile_osd_flag[0] == 1)
	{
		memset(&param, 0, sizeof(REGION_CTRL_PARAM_U));
		param.stBitmap.u32Width = 152;
		param.stBitmap.u32Height = 16;
		param.stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
		param.stBitmap.pData = g_jpeg_osd_buffer;
		
		enCtrl = REGION_SET_BITMAP;			
		s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_mobile_osd_handle[0], enCtrl, &param);
		if (s32Ret != HI_SUCCESS)
		{
			printf("set region bitmap faild 0x%x!\n", s32Ret);
			return -1;
		}
		
		enCtrl = REGION_SHOW;
		s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_mobile_osd_handle[0], enCtrl, &param);
		if (s32Ret != HI_SUCCESS)
		{
			printf("show region bitmap faild 0x%x!\n",s32Ret);
			return -1;
		}
	}
	
	return 0;
}

int MOBILE_EnableOSD()
{
	HI_S32 s32Ret;
	REGION_ATTR_S stRgnAttr;
	REGION_CTRL_PARAM_U param;
	REGION_CRTL_CODE_E enCtrl;
			
	memset(&stRgnAttr, 0, sizeof(REGION_ATTR_S));
	
	if (g_hi3511_mobile_osd_flag[0] == 1)
	{
		HI_MPI_RGN_Destroy(g_hi3511_mobile_osd_handle[0]);
		g_hi3511_mobile_osd_flag[0] = 0;
	}
		
	stRgnAttr.enType = OVERLAY_REGION;
	stRgnAttr.unAttr.stOverlay.bPublic = HI_FALSE;
	stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
	stRgnAttr.unAttr.stOverlay.stRect.s32X = 16;
	stRgnAttr.unAttr.stOverlay.stRect.s32Y = 16;
	stRgnAttr.unAttr.stOverlay.stRect.u32Width = 152;
	stRgnAttr.unAttr.stOverlay.stRect.u32Height = 16;
	stRgnAttr.unAttr.stOverlay.u32BgAlpha = 0;	// 128
	stRgnAttr.unAttr.stOverlay.u32FgAlpha = 128;	// 128
	stRgnAttr.unAttr.stOverlay.u32BgColor = 0x0;
	//stRgnAttr.unAttr.stOverlay.VeGroup = 0;
	stRgnAttr.unAttr.stOverlay.VeGroup = 3;
	
	s32Ret = HI_MPI_RGN_Create(&stRgnAttr, &g_hi3511_mobile_osd_handle[0]);
	if(s32Ret != HI_SUCCESS)
	{
		g_hi3511_mobile_osd_flag[0] = 0;
		printf("HI_MPI_RGN_Create err %d 0x%x!\n", __LINE__, s32Ret);
		//printf("(%d %d)%d %d %d %d\n", nChannel, nIndex, stRgnAttr.unAttr.stOverlay.stRect.s32X, stRgnAttr.unAttr.stOverlay.stRect.s32Y, stRgnAttr.unAttr.stOverlay.stRect.u32Width, stRgnAttr.unAttr.stOverlay.stRect.u32Height);		
		return -1;
	}
	
	g_hi3511_mobile_osd_flag[0] = 1;
					
	memset(&param, 0, sizeof(REGION_CTRL_PARAM_U));
	param.stBitmap.u32Width = stRgnAttr.unAttr.stOverlay.stRect.u32Width;
	param.stBitmap.u32Height = 16;
	param.stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
	param.stBitmap.pData = g_jpeg_osd_buffer;
	
	enCtrl = REGION_SET_BITMAP;			
	s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_mobile_osd_handle[0], enCtrl, &param);
	if (s32Ret != HI_SUCCESS)
	{
		printf("set region bitmap faild 0x%x!\n", s32Ret);
		return -1;
	}
		
	// Add the code by Jerry.Zhuang, 2009-12-10
	enCtrl = REGION_SET_ALPHA0;
	param.u32Alpha = 0;
	s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_mobile_osd_handle[0], enCtrl, &param);
	if (s32Ret != HI_SUCCESS)
	{
		printf("REGION_SET_ALPHA0 0x%x!\n",s32Ret);
		return -1;
	}
	enCtrl = REGION_SET_ALPHA1;
	param.u32Alpha = 128;
	s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_mobile_osd_handle[0], enCtrl, &param);
	if(s32Ret != HI_SUCCESS)
	{
		printf("REGION_SET_ALPHA1 faild 0x%x!\n",s32Ret);
		return -1;
	}
		
	enCtrl = REGION_SHOW;
	s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_mobile_osd_handle[0], enCtrl, &param);
	if (s32Ret != HI_SUCCESS)
	{
		printf("set region bitmap faild 0x%x!\n",s32Ret);
		return -1;
	}

	return 0;
}

int MOBILE_DisableOSD()
{
	if (g_hi3511_mobile_osd_flag[0] == 1)
	{
		HI_MPI_RGN_Destroy(g_hi3511_mobile_osd_handle[0]);
		g_hi3511_mobile_osd_flag[0] = 0;
	}

	return 0;
}

int StartSnap(int VeGroup, int SnapChn, int ViDev, int ViChn, char *stream, int *size)
{
	HI_S32 s32Ret;
	int ret = 0;
	OSD_DATA_PARAM osd;
	char buffer[64];
#ifdef BOGUS
	s32Ret = HI_MPI_VENC_BindInput(VeGroup, ViDev, ViChn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_BindInput err 0x%x\n",s32Ret);
		return -1;
	}
#endif /* BOGUS */
	
	s32Ret = HI_MPI_VENC_RegisterChn(VeGroup, SnapChn);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_RegisterChn err 0x%x\n",s32Ret);
		return -1;
	}
	
	s32Ret = HI_MPI_VENC_StartRecvPic(SnapChn);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_StartRecvPic err 0x%x\n",s32Ret);
		return -1;
	}
	
	// Add the code by Jerry.Zhuang, 2010-04-28

	ret = JPEG_EnableOSD(0); //Add the code by zhangjing 2013 -04-07
	if(ret == -1){
		printf("JPEG OSD TIME FAILED");
		goto exit;
	} 
	ret = JPEG_EnableOSD(1);
	if(ret == -1) {
		printf("JPEG OSD BRFAMER FAILED");
		goto exit;
	}
	ret = JPEG_EnableOSD(2);
	if(ret == -1){
		printf("JPEG OSD BRFAMER FAILED");
		goto exit;
	}

	//save jpeg picture
	s32Ret = GetSnapPic(SnapChn, stream, size);
	if(s32Ret != HI_SUCCESS)
	{
		printf("GetSnapPic err 0x%x\n",s32Ret);
		return -1;
	}

	// Add the code by Jerry.Zhuang, 2010-04-28
exit:
	JPEG_DisableOSD();
	
	s32Ret = HI_MPI_VENC_StopRecvPic(SnapChn);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_StopRecvPic err 0x%x\n",s32Ret);
		return -1;
	}

	s32Ret = HI_MPI_VENC_UnRegisterChn(SnapChn);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_UnRegisterChn err 0x%x\n",s32Ret);
		return -1;
	}

#ifdef BOGUS
	s32Ret = HI_MPI_VENC_UnbindInput(VeGroup);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_VENC_UnbindInput err 0x%x\n",s32Ret);
		return -1;
	}
#endif /* BOGUS */
	
	return 0;
}

pthread_mutex_t g_hi3511_mutex;

int Hi3511EncOpen(int nChannel, int nStreamType)
{
	int i = 0;
	int j = 0;
	
	for (i=0; i<MAX_ENC_CHANNEL; i++)
	{
		for (j=0; j<MAX_CHANNEL_ENC_NUM; j++)
		{
			g_hi3511_h264_chn_flag[i][j] = 0;
			g_hi3511_h264_timeout_count[i][j] = 0;
			g_hi3511_jpeg_chn_flag[i] = 0;
		}
	}	
	return 0;
} 

int Hi3511EncClose(int nChannel, int nStreamType)
{
	int i = 0;
	int j = 0;
	int nChn = 0;

	if (nChannel<0 || nChannel>=MAX_ENC_CHANNEL)
	{
		return -1;	
	}
	
	printf("Hi3511EncClose ...\n");
	
	// Add the code by Jerry.Zhuang, 2009-04-22
	

	if (g_hi3511_md_flag[nChannel] == 1)
	{
		HI_MPI_VDA_DestroyChn(0);
		g_hi3511_md_flag[nChannel] = 0;
	}

	// JPEG
	if (g_hi3511_jpeg_chn_flag[nChannel])
	{
		nChn = nChannel*3+2;
		DisableSnapshot(nChn, nChn);
		g_hi3511_jpeg_chn_flag[nChannel] = 0;
	}
	if (g_hi3511_h264_chn_flag[nChannel][1])
	{
		nChn = nChannel*3+1;
		DisableEncode(nChn, nChn);
		g_hi3511_h264_chn_flag[nChannel][1] = 0;
		g_hi3511_h264_timeout_count[nChannel][1] = 0;
	}
	if (g_hi3511_h264_chn_flag[nChannel][0])
	{
		nChn = nChannel*3+0;
		DisableEncode(nChn, nChn);
		g_hi3511_h264_chn_flag[nChannel][0] = 0;
		g_hi3511_h264_timeout_count[nChannel][1] = 0;
	}
	
	pthread_mutex_destroy(&g_hi3511_mutex);
	
	return 0;
}

int OSDSetup(int nChannel, int nStreamType, int nIndex, OSD_DATA_PARAM osd, int nShow)
{
	HI_S32 s32Ret;

	//printf("OSDSetup: %d %d %d %d %s\n", nChannel, nStreamType, nIndex, nShow, osd.data);
	
	if (nChannel<0 || nChannel>=MAX_ENC_CHANNEL)
	{
		return -1;
	}
	if (nStreamType<0 || nStreamType>=MAX_CHANNEL_ENC_NUM)
	{
		return -1;
	}
	if (nIndex<0 || nIndex>=MAX_OSD_NUM)
	{
		return -1;
	}
	if (strlen(osd.data) <= 0)
	{
		return -1;
	}
	osd.nxPos = osd.nxPos-(osd.nxPos%4);
	osd.nyPos = osd.nyPos-(osd.nyPos%4);
	// Add the code by Jerry.Zhuang, 2009-04-21
	//memcpy(&g_hi3511_osd_param[nChannel][nStreamType][nIndex], &osd, sizeof(OSD_DATA_PARAM));

	if (g_hi3511_osd_flag[nChannel][nStreamType][nIndex] != 1)
	{
		REGION_ATTR_S stRgnAttr;
		REGION_CTRL_PARAM_U param;
		REGION_CRTL_CODE_E enCtrl;
			
		memset(&stRgnAttr, 0, sizeof(REGION_ATTR_S));
		
		stRgnAttr.enType = OVERLAY_REGION;
		stRgnAttr.unAttr.stOverlay.bPublic = HI_FALSE;
		stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
		stRgnAttr.unAttr.stOverlay.stRect.s32X = osd.nxPos;
		stRgnAttr.unAttr.stOverlay.stRect.s32Y = osd.nyPos;
		stRgnAttr.unAttr.stOverlay.stRect.u32Width = strlen(osd.data)*8;
		stRgnAttr.unAttr.stOverlay.stRect.u32Height = 16;
		stRgnAttr.unAttr.stOverlay.u32BgAlpha = 0;	// 128
		stRgnAttr.unAttr.stOverlay.u32FgAlpha = 128;	// 128
		stRgnAttr.unAttr.stOverlay.u32BgColor = 0x0;
		stRgnAttr.unAttr.stOverlay.VeGroup = nStreamType;
		//printf("osd coordx:%d,y:%d\n",osd.nxPos,osd.nyPos);
		s32Ret = HI_MPI_RGN_Create(&stRgnAttr, &g_hi3511_osd_handle[nChannel][nStreamType][nIndex]);
		if(s32Ret != HI_SUCCESS)
		{
			printf("HI_MPI_RGN_Create err %d 0x%x!\n", __LINE__, s32Ret);
			printf("(%d %d)%d %d %d %d\n", nChannel, nIndex, stRgnAttr.unAttr.stOverlay.stRect.s32X, stRgnAttr.unAttr.stOverlay.stRect.s32Y, stRgnAttr.unAttr.stOverlay.stRect.u32Width, stRgnAttr.unAttr.stOverlay.stRect.u32Height);		
			return -1;
		}
		
		g_hi3511_osd_flag[nChannel][nStreamType][nIndex] = 1;
			
		memset(&param, 0, sizeof(REGION_CTRL_PARAM_U));
		param.stBitmap.u32Width = stRgnAttr.unAttr.stOverlay.stRect.u32Width;
		param.stBitmap.u32Height = 16;
		param.stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
		param.stBitmap.pData = g_osd_buffer;
	
		putFontExt(g_osd_buffer, osd.data, strlen(osd.data)*8, 16, osd.color);
		//putFontExt(g_osd_buffer, osd.data, strlen(osd.data)*8, 16, 0x000000);
		// Add the code by Jerry.Zhuang, 2010-04-28
	//	if (nIndex == 0)
	//	{
		memcpy(g_jpeg_osd_buffer[nIndex], g_osd_buffer, 30000);
		if(!nStreamType){
			g_osd_param_to_jpeg[nIndex].ndata_len = strlen(osd.data)*8;
			g_osd_param_to_jpeg[nIndex].nxPos = osd.nxPos;
			g_osd_param_to_jpeg[nIndex].nyPos = osd.nyPos;
			g_osd_param_to_jpeg[nIndex].nShow = nShow;
		}
		
	
		enCtrl = REGION_SET_BITMAP; 		
		s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_osd_handle[nChannel][nStreamType][nIndex], enCtrl, &param);
		if (s32Ret != HI_SUCCESS)
		{
			printf("set region bitmap faild 0x%x!\n", s32Ret);
			return -1;
		}
		
		// Add the code by Jerry.Zhuang, 2009-12-10
		enCtrl = REGION_SET_ALPHA0;
		param.u32Alpha = 0;
		s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_osd_handle[nChannel][nStreamType][nIndex], enCtrl, &param);
		if(s32Ret != HI_SUCCESS)
		{
			printf("REGION_SET_ALPHA0 0x%x!\n",s32Ret);
			return -1;
		}
		enCtrl = REGION_SET_ALPHA1;
		param.u32Alpha = 128;
		s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_osd_handle[nChannel][nStreamType][nIndex], enCtrl, &param);
		if(s32Ret != HI_SUCCESS)
		{
			printf("REGION_SET_ALPHA1 faild 0x%x!\n",s32Ret);
			return -1;
		}
		
		if (nShow)
		{
			enCtrl = REGION_SHOW;
			s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_osd_handle[nChannel][nStreamType][nIndex], enCtrl, &param);
			if (s32Ret != HI_SUCCESS)
			{
				printf("set region bitmap faild 0x%x!\n",s32Ret);
				return -1;
			}
		}
		
		//printf("OSDSetup(NEW): %d %d %d %d %s\n", nChannel, nStreamType, nIndex, nShow, osd.data);
		
		// Add the code by Jerry.Zhuang, 2009-04-21
		memcpy(&g_hi3511_osd_param[nChannel][nStreamType][nIndex], &osd, sizeof(OSD_DATA_PARAM));
		
		return 0;
	}
	else
	{
		//if (strlen(osd.data) != strlen(g_hi3511_osd_param[nChannel][nStreamType][nIndex].data))
		if (strlen(osd.data) != strlen(g_hi3511_osd_param[nChannel][nStreamType][nIndex].data)
			|| osd.color != g_hi3511_osd_param[nChannel][nStreamType][nIndex].color)
		{			
			REGION_ATTR_S stRgnAttr;
			REGION_CTRL_PARAM_U param;
			REGION_CRTL_CODE_E enCtrl;
	
			HI_MPI_RGN_Destroy(g_hi3511_osd_handle[nChannel][nStreamType][nIndex]);
			g_hi3511_osd_flag[nChannel][nStreamType][nIndex] = 0;
			
			stRgnAttr.enType = OVERLAY_REGION;
			stRgnAttr.unAttr.stOverlay.bPublic = HI_FALSE;
			stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
			stRgnAttr.unAttr.stOverlay.stRect.s32X= osd.nxPos;
			stRgnAttr.unAttr.stOverlay.stRect.s32Y= osd.nyPos;
			stRgnAttr.unAttr.stOverlay.stRect.u32Width = strlen(osd.data)*8;
			stRgnAttr.unAttr.stOverlay.stRect.u32Height = 16;
			stRgnAttr.unAttr.stOverlay.u32BgAlpha = 0;
			stRgnAttr.unAttr.stOverlay.u32FgAlpha = 128;
			stRgnAttr.unAttr.stOverlay.u32BgColor = 0x0;
			//stRgnAttr.unAttr.stOverlay.VeGroup = 0;
			stRgnAttr.unAttr.stOverlay.VeGroup = nStreamType;
			s32Ret = HI_MPI_RGN_Create(&stRgnAttr, &g_hi3511_osd_handle[nChannel][nStreamType][nIndex]);
			if(s32Ret != HI_SUCCESS)
			{
				printf("HI_MPI_RGN_Create err %d 0x%x!\n", __LINE__, s32Ret);
				return -1;
			}
	
			g_hi3511_osd_flag[nChannel][nStreamType][nIndex] = 1;	
			
			memset(&param, 0, sizeof(REGION_CTRL_PARAM_U));
			param.stBitmap.u32Width = stRgnAttr.unAttr.stOverlay.stRect.u32Width;
			param.stBitmap.u32Height = 16;
			param.stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
			param.stBitmap.pData = g_osd_buffer;
	
			putFontExt(g_osd_buffer, osd.data, strlen(osd.data)*8, 16, osd.color);
			// Add the code by Jerry.Zhuang, 2010-04-28
			//if (nIndex == 0)
			//{
				//printf("JPEG OSD: %s\n", osd.data);
			memcpy(g_jpeg_osd_buffer[nIndex], g_osd_buffer, 30000);
			if(!nStreamType){
				g_osd_param_to_jpeg[nIndex].ndata_len= strlen(osd.data)*8;
				g_osd_param_to_jpeg[nIndex].nxPos = osd.nxPos;
				g_osd_param_to_jpeg[nIndex].nyPos = osd.nyPos;
				g_osd_param_to_jpeg[nIndex].nShow = nShow;
			}
			//}
	
			enCtrl = REGION_SET_BITMAP;
			s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_osd_handle[nChannel][nStreamType][nIndex], enCtrl, &param);
			if (s32Ret != HI_SUCCESS)
			{
				printf("set region bitmap faild 0x%x!\n",s32Ret);
				return -1;
			}
			
			// Add the code by Jerry.Zhuang, 2009-12-10
			enCtrl = REGION_SET_ALPHA0;
			param.u32Alpha = 0;
			s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_osd_handle[nChannel][nStreamType][nIndex], enCtrl, &param);
			if(s32Ret != HI_SUCCESS)
			{
				printf("REGION_SET_ALPHA0 0x%x!\n",s32Ret);
				return -1;
			}
			enCtrl = REGION_SET_ALPHA1;
			param.u32Alpha = 128;
			s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_osd_handle[nChannel][nStreamType][nIndex], enCtrl, &param);
			if(s32Ret != HI_SUCCESS)
			{
				printf("REGION_SET_ALPHA1 faild 0x%x!\n",s32Ret);
				return -1;
			}
	
			if (nShow)
			{
				enCtrl = REGION_SHOW;
				s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_osd_handle[nChannel][nStreamType][nIndex], enCtrl, &param);
				if (s32Ret != HI_SUCCESS)
				{
					printf("set region bitmap faild 0x%x!\n",s32Ret);
					return -1;
				}
			}
	
			// Add the code by Jerry.Zhuang, 2009-04-21
			memcpy(&g_hi3511_osd_param[nChannel][nStreamType][nIndex], &osd, sizeof(OSD_DATA_PARAM));
		
			//printf("OSDSetup(CONTEXT LEN): %d %d %d %d %s\n", nChannel, nStreamType, nIndex, nShow, osd.data);
			
			return 0;			
		}
		else
		{
			// POSTION
			if (osd.nxPos!=g_hi3511_osd_param[nChannel][nStreamType][nIndex].nxPos
				|| osd.nyPos!=g_hi3511_osd_param[nChannel][nStreamType][nIndex].nyPos)
			{
				REGION_CTRL_PARAM_U param;
				REGION_CRTL_CODE_E enCtrl;
	
				memset(&param, 0, sizeof(REGION_CTRL_PARAM_U));
				param.stPoint.s32X = osd.nxPos;
				param.stPoint.s32Y = osd.nyPos;
	
				enCtrl = REGION_SET_POSTION;
	
				s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_osd_handle[nChannel][nStreamType][nIndex], enCtrl, &param);
					
				if(s32Ret != HI_SUCCESS)
				{
					printf("REGION_SET_POSTION faild 0x%x!\n",s32Ret);
					return -1;
				}
				
				//printf("OSDSetup(POS): %d %d %d %d %s\n", nChannel, nStreamType, nIndex, nShow, osd.data);
			}
	
			// 
			if (strcmp(osd.data, g_hi3511_osd_param[nChannel][nStreamType][nIndex].data) != 0)
			{
				REGION_CTRL_PARAM_U param;
				REGION_CRTL_CODE_E enCtrl;
	
				memset(&param, 0, sizeof(REGION_CTRL_PARAM_U));
				param.stBitmap.u32Width = strlen(osd.data)*8;
				param.stBitmap.u32Height = 16;
				param.stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
				param.stBitmap.pData = g_osd_buffer;
	
				putFontExt(g_osd_buffer, osd.data, strlen(osd.data)*8, 16, osd.color);
				// Add the code by Jerry.Zhuang, 2010-04-28
				//if (nIndex == 0)
				//{
					//printf("JPEG OSD: %s\n", osd.data);
				memcpy(g_jpeg_osd_buffer[nIndex], g_osd_buffer, 30000);
				if(!nStreamType){
					g_osd_param_to_jpeg[nIndex].ndata_len = strlen(osd.data)*8;
					g_osd_param_to_jpeg[nIndex].nxPos = osd.nxPos;
					g_osd_param_to_jpeg[nIndex].nyPos = osd.nyPos;
					g_osd_param_to_jpeg[nIndex].nShow = nShow;
				}
				//}
	
				enCtrl = REGION_SET_BITMAP;
				s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_osd_handle[nChannel][nStreamType][nIndex], enCtrl, &param);
				if (s32Ret != HI_SUCCESS)
				{
					printf("set region bitmap faild 0x%x!\n",s32Ret);
					return -1;
				}
				
				// Add the code by Jerry.Zhuang, 2009-12-10
				enCtrl = REGION_SET_ALPHA0;
				param.u32Alpha = 0;
				s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_osd_handle[nChannel][nStreamType][nIndex], enCtrl, &param);
				if(s32Ret != HI_SUCCESS)
				{
					printf("REGION_SET_ALPHA0 0x%x!\n",s32Ret);
					return -1;
				}
				enCtrl = REGION_SET_ALPHA1;
				param.u32Alpha = 128;
				s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_osd_handle[nChannel][nStreamType][nIndex], enCtrl, &param);
				if(s32Ret != HI_SUCCESS)
				{
					printf("REGION_SET_ALPHA1 faild 0x%x!\n",s32Ret);
					return -1;
				}
				
				//printf("OSDSetup(CONTEXT): %d %d %d %d %s\n", nChannel, nStreamType, nIndex, nShow, osd.data);
			}
	
			// Show
			if (nShow)
			{
				REGION_CTRL_PARAM_U param;
				REGION_CRTL_CODE_E enCtrl;
	
				memset(&param, 0, sizeof(REGION_CTRL_PARAM_U));
				enCtrl = REGION_SHOW;
	
				s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_osd_handle[nChannel][nStreamType][nIndex], enCtrl, &param);
				if (s32Ret != HI_SUCCESS)
				{
					printf("set region bitmap faild 0x%x!\n",s32Ret);
					return -1;
				}
			}
			else
			{
				REGION_CTRL_PARAM_U param;
				REGION_CRTL_CODE_E enCtrl;
	
				memset(&param, 0, sizeof(REGION_CTRL_PARAM_U));
				enCtrl = REGION_HIDE;
	
				s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_osd_handle[nChannel][nStreamType][nIndex], enCtrl, &param);
				if (s32Ret != HI_SUCCESS)
				{
					printf("set region bitmap faild 0x%x!\n",s32Ret);
					return -1;
				}
			}
	
			// Add the code by Jerry.Zhuang, 2009-04-21
			memcpy(&g_hi3511_osd_param[nChannel][nStreamType][nIndex], &osd, sizeof(OSD_DATA_PARAM));
	
			return 0;
		}
	}
}



int LOGOSetup(int nChannel, LOGO_DATA_PARAM logo, int nShow)
{
	int i = 0;
	HI_S32 s32Ret = 0;
	
	printf("LOGO: %d %d %d %d %d\n", logo.nxPos, logo.nyPos, logo.nWidth, logo.nHeight, nShow);
	
	if (nChannel<0 || nChannel>=MAX_CHANNEL_ENC_NUM)
	{
		return -1;
	}
	
	
#ifdef HD_CMOS
	if (logo.nxPos<0 || logo.nxPos>=1600)
	{
		return -1;
	}
	if (logo.nyPos<0 || logo.nyPos>=1200)
	{
		return -1;
	}
#endif	

#if 0	
	if (logo.nxPos<0 || logo.nxPos>=720)
	{
		return -1;
	}
	if (logo.nyPos<0 || logo.nyPos>=576)
	{
		return -1;
	}
#endif
	
	for (i=0; i<MAX_CHANNEL_ENC_NUM; i++)
	{
		if (g_hi3511_logo_flag[nChannel][i] != 1)
		{
			REGION_ATTR_S stRgnAttr;
			REGION_CTRL_PARAM_U param;
			REGION_CRTL_CODE_E enCtrl;
		
			if (logo.nWidth==0 || logo.nHeight==0 || nShow==0)
			{
				return 0;
			}
				
			stRgnAttr.enType = OVERLAY_REGION;
			stRgnAttr.unAttr.stOverlay.bPublic = HI_FALSE;
			stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
			stRgnAttr.unAttr.stOverlay.stRect.s32X= logo.nxPos;
			stRgnAttr.unAttr.stOverlay.stRect.s32Y= logo.nyPos;
			stRgnAttr.unAttr.stOverlay.stRect.u32Width = logo.nWidth;
			stRgnAttr.unAttr.stOverlay.stRect.u32Height = logo.nHeight;
			stRgnAttr.unAttr.stOverlay.u32BgAlpha = 128;
			stRgnAttr.unAttr.stOverlay.u32FgAlpha = 128;
			stRgnAttr.unAttr.stOverlay.u32BgColor = 0;
			//stRgnAttr.unAttr.stOverlay.VeGroup = 0;
			stRgnAttr.unAttr.stOverlay.VeGroup = i;
			s32Ret = HI_MPI_RGN_Create(&stRgnAttr, &g_hi3511_logo_handle[nChannel][i]);
			if(s32Ret != HI_SUCCESS)
			{
				printf("HI_MPI_RGN_Create err %d 0x%x!\n", __LINE__, s32Ret);
				return -1;
			}
				
			g_hi3511_logo_flag[nChannel][i] = 1;
		
			memset(&param, 0, sizeof(REGION_CTRL_PARAM_U));
			param.stBitmap.u32Width = logo.nWidth;
		    param.stBitmap.u32Height = logo.nHeight;
		    param.stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
		
			// 
			RGB888_RGB555(logo.data, g_logo_buffer, logo.nWidth, logo.nHeight);
		
			param.stBitmap.pData = g_logo_buffer;
		
			enCtrl = REGION_SET_BITMAP;
						
			s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_logo_handle[nChannel][i], enCtrl, &param);
			if (s32Ret != HI_SUCCESS)
			{
				printf("set region bitmap faild %d 0x%x!\n", __LINE__, s32Ret);
				return -1;
			}
		
			if (nShow)
			{
				enCtrl = REGION_SHOW;
			}
			else
			{
				enCtrl = REGION_HIDE;
			}
			s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_logo_handle[nChannel][i], enCtrl, &param);
			if (s32Ret != HI_SUCCESS)
			{
				printf("set region bitmap faild %d 0x%x!\n", __LINE__, s32Ret);
				return -1;
			}
		
			memcpy(&g_hi3511_logo_param[nChannel], &logo, sizeof(LOGO_DATA_PARAM));
		
			//return 0;
		}
		else
		{
			if (logo.nWidth!=g_hi3511_logo_param[nChannel].nWidth 
				|| logo.nHeight!=g_hi3511_logo_param[nChannel].nHeight
				|| logo.data != NULL)
			{
				REGION_ATTR_S stRgnAttr;
				REGION_CTRL_PARAM_U param;
				REGION_CRTL_CODE_E enCtrl;
		
				//printf("11111111(%d %d)(%d %d)(%p)\n", g_hi3511_logo_param[nChannel].nWidth, g_hi3511_logo_param[nChannel].nHeight, logo.nWidth, logo.nHeight, logo.data);
				HI_MPI_RGN_Destroy(g_hi3511_logo_handle[nChannel][i]);
				g_hi3511_logo_flag[nChannel][i] = 0;
			
				if (logo.nWidth==0 || logo.nHeight==0 || nShow==0)
				{
					return 0;
				}
				
				stRgnAttr.enType = OVERLAY_REGION;
				stRgnAttr.unAttr.stOverlay.bPublic = HI_FALSE;
				stRgnAttr.unAttr.stOverlay.enPixelFmt = PIXEL_FORMAT_RGB_1555;
				stRgnAttr.unAttr.stOverlay.stRect.s32X= logo.nxPos;
				stRgnAttr.unAttr.stOverlay.stRect.s32Y= logo.nyPos;
				stRgnAttr.unAttr.stOverlay.stRect.u32Width = logo.nWidth;
				stRgnAttr.unAttr.stOverlay.stRect.u32Height = logo.nHeight;
				stRgnAttr.unAttr.stOverlay.u32BgAlpha = 128;
				stRgnAttr.unAttr.stOverlay.u32FgAlpha = 128;
				stRgnAttr.unAttr.stOverlay.u32BgColor = 0;
				//stRgnAttr.unAttr.stOverlay.VeGroup = 0;
				stRgnAttr.unAttr.stOverlay.VeGroup = i;
				s32Ret = HI_MPI_RGN_Create(&stRgnAttr, &g_hi3511_logo_handle[nChannel][i]);
				if(s32Ret != HI_SUCCESS)
				{
					printf("HI_MPI_RGN_Create err %d 0x%x!\n", __LINE__, s32Ret);
					return -1;
				}
		
				g_hi3511_logo_flag[nChannel][i] = 1;
		
				memset(&param, 0, sizeof(REGION_CTRL_PARAM_U));
				param.stBitmap.u32Width = logo.nWidth;
				param.stBitmap.u32Height = logo.nHeight;
				param.stBitmap.enPixelFormat = PIXEL_FORMAT_RGB_1555;
		
				// 
				if (logo.data != NULL)
				{
					RGB888_RGB555(logo.data, g_logo_buffer, logo.nWidth, logo.nHeight);
				}
		
				param.stBitmap.pData = g_logo_buffer;
		
				enCtrl = REGION_SET_BITMAP;
						
				s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_logo_handle[nChannel][i], enCtrl, &param);
				if (s32Ret != HI_SUCCESS)
				{
					printf("set region bitmap faild %d 0x%x!\n", __LINE__, s32Ret);
					return -1;
				}
		
				enCtrl = REGION_SHOW;
				s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_logo_handle[nChannel][i], enCtrl, &param);
				if (s32Ret != HI_SUCCESS)
				{
					printf("set region bitmap faild %d 0x%x!\n", __LINE__, s32Ret);
					return -1;
				}
		
				//return 0;
			}
			else
			{
				//printf("222222222\n");
				REGION_CTRL_PARAM_U param;
				REGION_CRTL_CODE_E enCtrl;
		
				memset(&param, 0, sizeof(REGION_CTRL_PARAM_U));
				param.stPoint.s32X = logo.nxPos;
				param.stPoint.s32Y = logo.nyPos;
		
				enCtrl = REGION_SET_POSTION;
		
				s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_logo_handle[nChannel][i], enCtrl, &param);
						
				if(s32Ret != HI_SUCCESS)
				{
					printf("REGION_SET_POSTION faild %d 0x%x!\n", __LINE__, s32Ret);
					return -1;
				}
				memcpy(&g_hi3511_logo_param[nChannel], &logo, sizeof(LOGO_DATA_PARAM));
						
				// Show
				if (nShow)
				{
					REGION_CTRL_PARAM_U param;
					REGION_CRTL_CODE_E enCtrl;
		
					memset(&param, 0, sizeof(REGION_CTRL_PARAM_U));
					enCtrl = REGION_SHOW;
		
					s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_logo_handle[nChannel][i], enCtrl, &param);
					if (s32Ret != HI_SUCCESS)
					{
						printf("set region bitmap faild %d 0x%x!\n", __LINE__, s32Ret);
						return -1;
					}
				}
				else
				{
					REGION_CTRL_PARAM_U param;
					REGION_CRTL_CODE_E enCtrl;
		
					memset(&param, 0, sizeof(REGION_CTRL_PARAM_U));
					enCtrl = REGION_HIDE;
		
					s32Ret = HI_MPI_RGN_SetAttr(g_hi3511_logo_handle[nChannel][i], enCtrl, &param);
					if (s32Ret != HI_SUCCESS)
					{
						printf("set region bitmap faild %d 0x%x!\n", __LINE__, s32Ret);
						return -1;
					}
				}
		
				//return 0;
			}
		}
	}
	
	return 0;
}

int EnableMD(int nChannel, int value)
{
	HI_S32 s32Ret;
	VENC_CHN VeChn = nChannel;
	MD_CHN_ATTR_S stMdAttr;
	MD_REF_ATTR_S  stRefAttr;

	// set MD attribute
	stMdAttr.stMBMode.bMBSADMode =HI_TRUE;
	stMdAttr.stMBMode.bMBMVMode = HI_FALSE;
	stMdAttr.stMBMode.bMBPelNumMode = HI_FALSE;
	stMdAttr.stMBMode.bMBALARMMode = HI_FALSE;
	stMdAttr.u16MBALSADTh = value*256;
	//stMdAttr.u16MBALSADTh = value;
	stMdAttr.u8MBPelALTh = value;
	stMdAttr.u8MBPerALNumTh = value;
	stMdAttr.enSADBits = MD_SAD_8BIT;
	stMdAttr.stDlight.bEnable = HI_FALSE;
	//stMdAttr.stDlight.bEnable = HI_TRUE;
	stMdAttr.u32MDInternal = 0;
	//stMdAttr.u32MDInternal = 1;
	stMdAttr.u32MDBufNum = 16;
	//stMdAttr.u32MDBufNum = 1;

	//set MD frame
	stRefAttr.enRefFrameMode = MD_REF_AUTO;
	stRefAttr.enRefFrameStat = MD_REF_DYNAMIC;
    
	s32Ret =  HI_MPI_VDA_SetChnAttr(VeChn, &stMdAttr);
	if(s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_MD_SetChnAttr Err 0x%x\n", s32Ret);
		return -1;
	}
	
#ifdef BOGUS
	s32Ret = HI_MPI_MD_SetRefFrame(VeChn, &stRefAttr);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_MD_SetRefFrame Err 0x%x\n", s32Ret);
		return -1;
	}
#endif /* BOGUS */

	s32Ret = HI_MPI_VDA_StartRecvPic(VeChn);
	if (s32Ret != HI_SUCCESS)
	{
		printf("HI_MPI_MD_EnableChn Err 0x%x\n", s32Ret);
		return -1;
	}

	return 0;
}

int DisableMD(int nChannel)
{
	HI_S32 s32Ret;
	VENC_CHN VeChn = nChannel;
	
	s32Ret = HI_MPI_VDA_DestroyChn(VeChn);
	
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VDA_DestroyChn Err 0x%x\n",s32Ret);
        return -1;
    } 

	return 0;
}

int SadResult()
{
	return 0;
}

int MDResult(MD_DATA_S *pstMdData)
{
	int alarmNum = 0;
	int alarmFlag = 0;
	float rate = 0;

	if (pstMdData == NULL)
	{
		return 0;
	}
	
	//printf("MD:%d * %d\n", pstMdData->u16MBWidth, pstMdData->u16MBHeight);

	// get MD SAD data
	if(pstMdData->stMBMode.bMBSADMode)
	{
		HI_S32 i,j;
		HI_U16* pTmp = NULL;
		HI_U16 sadValue = 0;
		
		for(i=0; i<pstMdData->u16MBHeight; i++)
		{
			pTmp = (HI_U16 *)((HI_U32)pstMdData->stMBSAD.pu32Addr + i*pstMdData->stMBSAD.u32Stride);
			
			for(j=0; j<pstMdData->u16MBWidth; j++)
			{
				//printf("%2d",*pTmp);
				sadValue = *pTmp;
				if (sadValue >= g_hi3511_md_value[0]%100 && g_hi3511_md_value[0]>0 && g_hi3511_md_mask[0][i*pstMdData->u16MBWidth+j]==1)
				{
					alarmNum++;
				}				

				pTmp++;
			}
			
			//printf("\n");
		}
		
		//printf("MD: alarmNum: %d\n", alarmNum);
		
		// Add the code by Jerry.Zhuang, 2009-01-31
		if (alarmNum == 0)
		{
			return 0;	
		}
		rate = (float)(g_hi3511_md_value[0])/100.0;
		//alarmFlag = (int)(g_hi3511_md_count[0]*rate);
		alarmFlag = 1;
		
		//rate = (float)(100-g_hi3511_md_value[0])/100.0;
		//alarmFlag = (int)(g_hi3511_md_count[0]*rate);

		//printf("Alarm Num: %d %d %d(%d)\n", alarmNum, alarmFlag, g_hi3511_md_count[0], g_hi3511_md_value[0]);

		if (alarmNum>alarmFlag && alarmFlag>0)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	return 0;
}

int GetMD(int nChannel)
{
	HI_S32 s32Ret;
	HI_S32 s32MdFd;
	MD_DATA_S stMdData;
	VENC_CHN VeChn = nChannel;
	fd_set read_fds;
	struct timeval TimeoutVal; 
	int alarmFlag = 0;

	s32MdFd = HI_MPI_VDA_GetFd(VeChn);
	// Add the code by Jerry.Zhuang, 2009-04-14
	if (s32MdFd < 0)
	{
		printf("HI_MPI_MD_GetFd err 0x%x\n", s32MdFd);
		return 0;
	}

	FD_ZERO(&read_fds);
	FD_SET(s32MdFd,&read_fds);

	TimeoutVal.tv_sec = 2;
	TimeoutVal.tv_usec = 0;
	s32Ret = select(s32MdFd+1, &read_fds, NULL, NULL, &TimeoutVal);

	if (s32Ret < 0) 
	{
		printf("select err\n");
		return 0;
	}
	else if (0 == s32Ret) 
	{
		printf("GetMD: time out\n");
		return 0;
	}
	else
	{
		memset(&stMdData, 0, sizeof(MD_DATA_S));
			
		if (FD_ISSET(s32MdFd, &read_fds))
		{
			s32Ret = HI_MPI_VDA_GetData(VeChn, &stMdData, HI_IO_NOBLOCK);
			if(s32Ret != HI_SUCCESS)
			{
				printf("HI_MPI_MD_GetData err 0x%x\n",s32Ret);
				return -1;
			}
		}
	
		alarmFlag = MDResult(&stMdData);

		s32Ret = HI_MPI_VDA_ReleaseData(VeChn, &stMdData);
		if(s32Ret != HI_SUCCESS)
		{
		    printf("Md HI_MPI_VDA_ReleaseData Data Err 0x%x\n",s32Ret);
			return 0;
		}
	}

	return alarmFlag;
}

VENC_ATTR_H264_S g_hi3511_stH264Attr;
VENC_ATTR_JPEG_S g_hi3511_stJpegAttr;
VENC_ATTR_MJPEG_S g_hi3511_stMjpegAttr;
VENC_ATTR_MPEG4_S g_hi3511_stMpeg4Attr;


#if 0
int GetVideoEncAttr(VENC_PARAM *pEncParam, VENC_CHN_ATTR_S *pstAttr)
{
	if (pEncParam==NULL || pstAttr==NULL)
	{
		return -1;
	}
	SAMPLE_RC_E enRcMode = SAMPLE_RC_CBR;
	PIC_SIZE_E enSize = PIC_HD720;
	VIDEO_NORM_E enNorm = VIDEO_ENCODING_MODE_PAL;

	VENC_ATTR_H264_CBR_S    stH264Cbr;
    VENC_ATTR_H264_VBR_S    stH264Vbr;
    VENC_ATTR_H264_FIXQP_S  stH264FixQp;
    VENC_ATTR_MJPEG_S stMjpegAttr;
    VENC_ATTR_MJPEG_FIXQP_S stMjpegeFixQp;
    VENC_ATTR_JPEG_S stJpegAttr;
    SIZE_S stPicSize;
	
	memset(pstAttr, 0, sizeof(VENC_CHN_ATTR_S));
	
	switch (pEncParam->reserve)
	{
	case 0:		// H.264
		{
			VENC_ATTR_H264_S stH264Attr;
			
			memset(&stH264Attr, 0, sizeof(VENC_ATTR_H264_S));
			
{
			stH264Attr.u32MaxPicWidth = pEncParam->nEncodeWidth;
			stH264Attr.u32MaxPicHeight = pEncParam->nEncodeHeight;
			stH264Attr.u32PicWidth = pEncParam->nEncodeWidth;/*the picture width*/
			stH264Attr.u32PicHeight = pEncParam->nEncodeWidth;/*the picture height*/
			stH264Attr.u32BufSize  = pEncParam->nEncodeWidth * pEncParam->nEncodeWidth* 2;/*stream buffer size*/
			stH264Attr.u32Profile  = 0;/*0: baseline; 1:MP; 2:HP   ? */
			stH264Attr.bByFrame = HI_TRUE;/*get stream mode is slice mode or frame mode?*/
			stH264Attr.bField = HI_FALSE;  /* surpport frame code only for hi3516, bfield = HI_FALSE */
			stH264Attr.bMainStream = HI_TRUE; /* surpport main stream only for hi3516, bMainStream = HI_TRUE */
			stH264Attr.u32Priority = 0; /*channels precedence level. invalidate for hi3516*/
			stH264Attr.bVIField = HI_FALSE;/*the sign of the VI picture is field or frame. Invalidate for hi3516*/
			memcpy(&pstAttr->stVeAttr.stAttrH264e, &stH264Attr, sizeof(VENC_ATTR_H264_S));

			if(SAMPLE_RC_CBR == enRcMode)
			{
				pstAttr->stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
				stH264Cbr.u32Gop			= (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
				stH264Cbr.u32StatTime		= 1; /* stream rate statics time(s) */
				stH264Cbr.u32ViFrmRate		= (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;/* input (vi) frame rate */
				stH264Cbr.fr32TargetFrmRate = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;/* target frame rate */
				switch (enSize)
				{
				  case PIC_QCIF:
					   stH264Cbr.u32BitRate = 256; /* average bit rate */
					   break;
				  case PIC_QVGA:	/* 320 * 240 */
				  case PIC_CIF: 

					   stH264Cbr.u32BitRate = 512;
					   break;

				  case PIC_D1:
				  case PIC_VGA:    /* 640 * 480 */
					   stH264Cbr.u32BitRate = 768;
					   break;
				  case PIC_HD720:	/* 1280 * 720 */
					   stH264Cbr.u32BitRate = 1024*2;
					   break;
				  case PIC_HD1080:	/* 1920 * 1080 */
					   stH264Cbr.u32BitRate = 1024*4;
					   break;
				  default :
					   stH264Cbr.u32BitRate = 1024;
					   break;
				}
				
				stH264Cbr.u32FluctuateLevel = 0; /* average bit rate */
				memcpy(&pstAttr->stRcAttr.stAttrH264Cbr, &stH264Cbr, sizeof(VENC_ATTR_H264_CBR_S));
			}
			else if (SAMPLE_RC_FIXQP == enRcMode) 
			{
				pstAttr->stRcAttr.enRcMode = VENC_RC_MODE_H264FIXQP;
				stH264FixQp.u32Gop = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
				stH264FixQp.u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
				stH264FixQp.fr32TargetFrmRate = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
				stH264FixQp.u32IQp = 20;
				stH264FixQp.u32PQp = 23;
				memcpy(&pstAttr->stRcAttr.stRcAttr.stAttrH264FixQp, &stH264FixQp,sizeof(VENC_ATTR_H264_FIXQP_S));
			}
			else if (SAMPLE_RC_VBR == enRcMode) 
			{
				pstAttr->stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
				stH264Vbr.u32Gop = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
				stH264Vbr.u32StatTime = 1;
				stH264Vbr.u32ViFrmRate = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
				stH264Vbr.fr32TargetFrmRate = (VIDEO_ENCODING_MODE_PAL== enNorm)?25:30;
				stH264Vbr.u32MinQp = 24;
				stH264Vbr.u32MaxQp = 32;
				switch (enSize)
				{
				  case PIC_QCIF:
					   stH264Vbr.u32MaxBitRate= 256*3; /* average bit rate */
					   break;
				  case PIC_QVGA:	/* 320 * 240 */
				  case PIC_CIF:
					   stH264Vbr.u32MaxBitRate = 512*3;
					   break;
				  case PIC_D1:
				  case PIC_VGA:    /* 640 * 480 */
					   stH264Vbr.u32MaxBitRate = 1024*2*3;
					   break;
				  case PIC_HD720:	/* 1280 * 720 */
					   stH264Vbr.u32MaxBitRate = 1024*3*3;
					   break;
				  case PIC_HD1080:	/* 1920 * 1080 */
					   stH264Vbr.u32MaxBitRate = 1024*6*3;
					   break;
				  default :
					   stH264Vbr.u32MaxBitRate = 1024*4*3;
					   break;
				}
				memcpy(&pstAttr->stRcAttr.stAttrH264Vbr, &stH264Vbr, sizeof(VENC_ATTR_H264_VBR_S));
			}
			else
			{
				return HI_FAILURE;
			}
		}

#ifdef VENC_DEBUG
			printf("H.264 EnableEncode:\nWidth: %d\nHeight: %d\nBitrate: %d\nViFrameRate: %d\nFrameRate: %d\nGOP: %d\nCBR: %d\nPicLevel: %d\n", 
					stH264Attr.u32PicWidth, 
					stH264Attr.u32PicHeight, 
					stH264Attr.u32Bitrate, 
					stH264Attr.u32ViFramerate,
					stH264Attr.u32TargetFramerate,
					stH264Attr.u32Gop, 
					stH264Attr.bCBR, 
					stH264Attr.u32PicLevel);
#endif
		}
		break;
		
	case 1:		// MJPEG
		{
			VENC_ATTR_MJPEG_S stMjpegAttr;
			
			memset(&stMjpegAttr, 0, sizeof(VENC_ATTR_MJPEG_S));
			
			stMjpegAttr.u32Priority = 0;
			stMjpegAttr.u32PicWidth = pEncParam->nEncodeWidth;
			stMjpegAttr.u32PicHeight = pEncParam->nEncodeHeight;
			stMjpegAttr.bMainStream = HI_TRUE;
			stMjpegAttr.bByFrame = HI_TRUE;				
						
#ifdef HD_CMOS
			stMjpegAttr.bVIField = HI_FALSE;
#endif
	
#ifdef CCD				
			if (stMjpegAttr.u32PicHeight==576 || stMjpegAttr.u32PicHeight==480)
			{
				stMjpegAttr.bVIField = HI_TRUE;
			}
			else
			{
				stMjpegAttr.bVIField = HI_FALSE;
			}
#endif				
				
#ifdef HD_CMOS	
			stMjpegAttr.u32ViFramerate = 30;
			switch (pEncParam->nEncodeHeight)
			{
			case 1200:
				stMjpegAttr.u32MCUPerECS = 7500;
				break;
				
			case 720:
				stMjpegAttr.u32MCUPerECS = 3600;
				break;
				
			// Add the code by Jerry.Zhuang, 2009-04-25
			case 600:
			case 608:
				stMjpegAttr.u32MCUPerECS = 1900;
				break;
				
			case 288:
			case 300:
			case 304:
				stMjpegAttr.u32MCUPerECS = 475;
				break;
			case 576:
				stMjpegAttr.u32MCUPerECS = 720;
				break;
			case 480:
				stMjpegAttr.u32MCUPerECS = 1350;
				break;
			case 240:
				stMjpegAttr.u32MCUPerECS = 675;
				break;
		
			break;
				
			}	
#endif	

#ifdef CCD			
			if (stMjpegAttr.u32PicHeight==576 || stMjpegAttr.u32PicHeight==288 || stMjpegAttr.u32PicHeight==144)
			{
				stMjpegAttr.u32ViFramerate = 25;
			}
			else
			{
				stMjpegAttr.u32ViFramerate = 30;
			}
			switch (pEncParam->nEncodeHeight)
			{
			case 576:
				stMjpegAttr.u32MCUPerECS = 1620;
				break;
				
			case 288:
				if (pEncParam->nEncodeWidth == 352)
				{
					stMjpegAttr.u32MCUPerECS = 396;
				}
				else
				{
					stMjpegAttr.u32MCUPerECS = 810;
				}
				break;
				
			case 144:
				stMjpegAttr.u32MCUPerECS = 99;
				break;
				
			case 480:
				stMjpegAttr.u32MCUPerECS = 1350;
				break;
				
			case 240:
				if (pEncParam->nEncodeWidth == 352)
				{
					stMjpegAttr.u32MCUPerECS = 330;
				}
				else
				{
					stMjpegAttr.u32MCUPerECS = 675;
				}
				break;
				
			case 128:
				stMjpegAttr.u32MCUPerECS = 88;
				break;
			}
#endif				
			stMjpegAttr.u32TargetFramerate = pEncParam->nFramerate;
			stMjpegAttr.u32TargetBitrate = pEncParam->nBitRate/1024;

#ifdef HD_CMOS
			stMjpegAttr.u32BufSize = 1600*1200*1.5;
#endif

#ifdef CCD			
			stMjpegAttr.u32BufSize = 720*576*2;
#endif				
			
			memset(pstAttr, 0 ,sizeof(VENC_CHN_ATTR_S));
			pstAttr->enType = PT_MJPEG;
			//pstAttr->pValue = (HI_VOID *)&stMjpegAttr;
			
			// Add the code by Jerry.Zhuang, 2009-04-21
			pstAttr->pValue = (HI_VOID *)&g_hi3511_stMjpegAttr;
			memcpy(&g_hi3511_stMjpegAttr, &stMjpegAttr, sizeof(VENC_ATTR_MJPEG_S));
#ifdef VENC_DEBUG				
			printf("MJPEG EnableEncode:\nWidth: %d\nHeight: %d\nBitrate: %d\nViFrameRate: %d\nFrameRate: %d\n", 
					stMjpegAttr.u32PicWidth, 
					stMjpegAttr.u32PicHeight, 
					stMjpegAttr.u32TargetBitrate, 
					stMjpegAttr.u32ViFramerate,
					stMjpegAttr.u32TargetFramerate);		
#endif					
		}
		break;
		
	case 2:		// MPEG4
		{
			VENC_ATTR_MPEG4_S stMpeg4Attr;
			
			memset(&stMpeg4Attr, 0, sizeof(VENC_ATTR_MPEG4_S));
			
			stMpeg4Attr.u32Priority = 0;
			stMpeg4Attr.u32PicWidth = pEncParam->nEncodeWidth;
			stMpeg4Attr.u32PicHeight = pEncParam->nEncodeHeight;		
								
#ifdef HD_CMOS
			stMpeg4Attr.bVIField = HI_FALSE;
#endif

#ifdef CCD				
			if (stMpeg4Attr.u32PicHeight==576 || stMpeg4Attr.u32PicHeight==480)
			{
				stMpeg4Attr.bVIField = HI_TRUE;
			}
			else
			{
				stMpeg4Attr.bVIField = HI_FALSE;
			}
#endif				
			stMpeg4Attr.u32TargetBitrate = pEncParam->nBitRate/1024;
				
#ifdef HD_CMOS	
			stMpeg4Attr.u32ViFramerate = 30;
#endif	

#ifdef CCD			
			if (stMpeg4Attr.u32PicHeight==576 || stMpeg4Attr.u32PicHeight==288 || stMpeg4Attr.u32PicHeight==144)
			{
				stMpeg4Attr.u32ViFramerate = 25;
			}
			else
			{
				stMpeg4Attr.u32ViFramerate = 30;
			}
#endif				
			stMpeg4Attr.u32TargetFramerate = pEncParam->nFramerate;

#ifdef HD_CMOS
			stMpeg4Attr.u32BufSize = 640*480*2*2;
#endif

#ifdef CCD			
			stMpeg4Attr.u32BufSize = 720*576*2*2;
#endif				
			stMpeg4Attr.u32Gop = pEncParam->nKeyInterval;
			stMpeg4Attr.u32MaxDelay = 1;
    			
			memset(pstAttr, 0 ,sizeof(VENC_CHN_ATTR_S));
			//pstAttr->enType = PT_H264;
			//pstAttr->pValue = (HI_VOID *)&stMpeg4Attr;
			
			// Add the code by Jerry.Zhuang, 2009-04-21
			pstAttr->pValue = (HI_VOID *)&g_hi3511_stMpeg4Attr;
			memcpy(&g_hi3511_stMpeg4Attr, &stMpeg4Attr, sizeof(VENC_ATTR_MPEG4_S));
#ifdef VENC_DEBUG
			printf("MPEG4 EnableEncode:\nWidth: %d\nHeight: %d\nBitrate: %d\nViFrameRate: %d\nFrameRate: %d\nGOP: %d\n", 
					stMpeg4Attr.u32PicWidth, 
					stMpeg4Attr.u32PicHeight, 
					stMpeg4Attr.u32TargetBitrate, 
					stMpeg4Attr.u32ViFramerate,
					stMpeg4Attr.u32TargetFramerate,
					stMpeg4Attr.u32Gop);
#endif		
						
		}		
		break;
		
	case 3:		// MOBILE
		{
			// JPEG Snapshot
			VENC_ATTR_JPEG_S stJpegAttr;
			
			memset(&stJpegAttr, 0, sizeof(VENC_ATTR_JPEG_S));
			
			stJpegAttr.u32BufSize = pEncParam->nEncodeWidth*pEncParam->nEncodeHeight*2;
			stJpegAttr.u32PicWidth = pEncParam->nEncodeWidth;
			stJpegAttr.u32PicHeight = pEncParam->nEncodeHeight;
			stJpegAttr.bVIField = HI_FALSE;
			stJpegAttr.bByFrame = HI_TRUE;
			stJpegAttr.u32MCUPerECS = pEncParam->nEncodeWidth*pEncParam->nEncodeHeight/256;
			stJpegAttr.u32ImageQuality = 1;	// old:0[0~5]
		
			memset(pstAttr, 0 ,sizeof(VENC_CHN_ATTR_S));
			pstAttr->enType = PT_JPEG;
			pstAttr->pValue = (HI_VOID *)&g_hi3511_stJpegAttr;
			memcpy(&g_hi3511_stJpegAttr, &stJpegAttr, sizeof(VENC_ATTR_JPEG_S));
		}
		break;
	}
	
	return 0;
}
#endif

int GetVideoEncAttr(VENC_PARAM *pEncParam, VENC_CHN_ATTR_S *pstAttr)
{
	if (pEncParam==NULL || pstAttr==NULL)
	{
		return -1;
	}
	
	memset(pstAttr, 0, sizeof(VENC_CHN_ATTR_S));
	
	switch (pEncParam->reserve)
	{
	case 0:		// H.264
		{
			VENC_ATTR_H264_S stH264Attr;
			
			memset(&stH264Attr, 0, sizeof(VENC_ATTR_H264_S));
			
			stH264Attr.u32Priority = 0;
			stH264Attr.u32PicWidth = pEncParam->nEncodeWidth;
			stH264Attr.u32PicHeight = pEncParam->nEncodeHeight;
			stH264Attr.bMainStream = HI_TRUE;
			stH264Attr.bByFrame = HI_TRUE;				
				
			// Add the code by Jerry.Zhuang, 2009-02-10
			if (pEncParam->nEncodeMode == 0)
			{
				stH264Attr.bCBR = HI_FALSE;
				if (pEncParam->nMinQuantizer > 5)
				{
					stH264Attr.u32PicLevel = 5;
				}
				else
				{
					stH264Attr.u32PicLevel = pEncParam->nMinQuantizer;
				}
			}
			else
			{
				stH264Attr.bCBR = HI_FALSE;
				if (pEncParam->nMinQuantizer > 5)
				{
					stH264Attr.u32PicLevel = 5;
				}
				else
				{
					stH264Attr.u32PicLevel = pEncParam->nMinQuantizer;
				}
			}
				
#ifdef HD_CMOS
			stH264Attr.bVIField = HI_FALSE;
			if(stH264Attr.u32PicHeight==480)
			stH264Attr.bVIField = HI_TRUE;
				
#endif
			
#ifdef CCD				
			if (stH264Attr.u32PicHeight==576 || stH264Attr.u32PicHeight==480)
			{
				stH264Attr.bVIField = HI_TRUE;
			}
			else
			{
				stH264Attr.bVIField = HI_FALSE;
			}

#endif				
			stH264Attr.u32Bitrate = pEncParam->nBitRate/1000;
				
#ifdef HD_CMOS	
			stH264Attr.u32ViFramerate = 30;
#endif	

#ifdef CCD			
			if (stH264Attr.u32PicHeight==576 || stH264Attr.u32PicHeight==288 || stH264Attr.u32PicHeight==144)
			{
				stH264Attr.u32ViFramerate = 25;
			}
			else
			{
				stH264Attr.u32ViFramerate = 30;
			}
#endif				
			stH264Attr.u32TargetFramerate = pEncParam->nFramerate;

#ifdef HD_CMOS
			stH264Attr.u32BufSize = 1600*1200*2*2;
			//stH264Attr.u32BufSize = 720*576*2*2;
#endif

#ifdef CCD		
			printf("entry ccd encode ***************\n\n");
			stH264Attr.u32BufSize = 720*576*2*2;
#endif				
			stH264Attr.u32Gop = pEncParam->nKeyInterval;
			stH264Attr.u32MaxDelay = 0;
    			
			memset(pstAttr, 0 ,sizeof(VENC_CHN_ATTR_S));
			pstAttr->enType = PT_H264;
			//pstAttr->pValue = (HI_VOID *)&stH264Attr;
			
			// Add the code by Jerry.Zhuang, 2009-04-21
			pstAttr->pValue = (HI_VOID *)&g_hi3511_stH264Attr;
			memcpy(&g_hi3511_stH264Attr, &stH264Attr, sizeof(VENC_ATTR_H264_S));
#ifdef VENC_DEBUG
			printf("H.264 EnableEncode:\nWidth: %d\nHeight: %d\nBitrate: %d\nViFrameRate: %d\nFrameRate: %d\nGOP: %d\nCBR: %d\nPicLevel: %d\n", 
					stH264Attr.u32PicWidth, 
					stH264Attr.u32PicHeight, 
					stH264Attr.u32Bitrate, 
					stH264Attr.u32ViFramerate,
					stH264Attr.u32TargetFramerate,
					stH264Attr.u32Gop, 
					stH264Attr.bCBR, 
					stH264Attr.u32PicLevel);
#endif
		}
		break;
		
	case 1:		// MJPEG
		{
			VENC_ATTR_MJPEG_S stMjpegAttr;
			
			memset(&stMjpegAttr, 0, sizeof(VENC_ATTR_MJPEG_S));
			
			stMjpegAttr.u32Priority = 0;
			stMjpegAttr.u32PicWidth = pEncParam->nEncodeWidth;
			stMjpegAttr.u32PicHeight = pEncParam->nEncodeHeight;
			stMjpegAttr.bMainStream = HI_TRUE;
			stMjpegAttr.bByFrame = HI_TRUE;				
						
#ifdef HD_CMOS
			stMjpegAttr.bVIField = HI_FALSE;
#endif
	
#ifdef CCD				
			if (stMjpegAttr.u32PicHeight==576 || stMjpegAttr.u32PicHeight==480)
			{
				stMjpegAttr.bVIField = HI_TRUE;
			}
			else
			{
				stMjpegAttr.bVIField = HI_FALSE;
			}
#endif				
				
#ifdef HD_CMOS	
			stMjpegAttr.u32ViFramerate = 30;
			switch (pEncParam->nEncodeHeight)
			{
			case 1200:
				stMjpegAttr.u32MCUPerECS = 7500;
				break;
				
			case 720:
				stMjpegAttr.u32MCUPerECS = 3600;
				break;
				
			// Add the code by Jerry.Zhuang, 2009-04-25
			case 600:
			case 608:
				stMjpegAttr.u32MCUPerECS = 1900;
				break;
				
			case 288:
			case 300:
			case 304:
				stMjpegAttr.u32MCUPerECS = 475;
				break;
			case 576:
				stMjpegAttr.u32MCUPerECS = 720;
				break;
			case 480:
				stMjpegAttr.u32MCUPerECS = 1350;
				break;
			case 240:
				stMjpegAttr.u32MCUPerECS = 675;
				break;
		
			break;
				
			}	
#endif	

#ifdef CCD			
			if (stMjpegAttr.u32PicHeight==576 || stMjpegAttr.u32PicHeight==288 || stMjpegAttr.u32PicHeight==144)
			{
				stMjpegAttr.u32ViFramerate = 25;
			}
			else
			{
				stMjpegAttr.u32ViFramerate = 30;
			}
			switch (pEncParam->nEncodeHeight)
			{
			case 576:
				stMjpegAttr.u32MCUPerECS = 1620;
				break;
				
			case 288:
				if (pEncParam->nEncodeWidth == 352)
				{
					stMjpegAttr.u32MCUPerECS = 396;
				}
				else
				{
					stMjpegAttr.u32MCUPerECS = 810;
				}
				break;
				
			case 144:
				stMjpegAttr.u32MCUPerECS = 99;
				break;
				
			case 480:
				stMjpegAttr.u32MCUPerECS = 1350;
				break;
				
			case 240:
				if (pEncParam->nEncodeWidth == 352)
				{
					stMjpegAttr.u32MCUPerECS = 330;
				}
				else
				{
					stMjpegAttr.u32MCUPerECS = 675;
				}
				break;
				
			case 128:
				stMjpegAttr.u32MCUPerECS = 88;
				break;
			}
#endif				
			stMjpegAttr.u32TargetFramerate = pEncParam->nFramerate;
			stMjpegAttr.u32TargetBitrate = pEncParam->nBitRate/1024;

#ifdef HD_CMOS
			stMjpegAttr.u32BufSize = 1600*1200*1.5;
#endif

#ifdef CCD			
			stMjpegAttr.u32BufSize = 720*576*2;
#endif				
			
			memset(pstAttr, 0 ,sizeof(VENC_CHN_ATTR_S));
			pstAttr->enType = PT_MJPEG;
			//pstAttr->pValue = (HI_VOID *)&stMjpegAttr;
			
			// Add the code by Jerry.Zhuang, 2009-04-21
			pstAttr->pValue = (HI_VOID *)&g_hi3511_stMjpegAttr;
			memcpy(&g_hi3511_stMjpegAttr, &stMjpegAttr, sizeof(VENC_ATTR_MJPEG_S));
#ifdef VENC_DEBUG				
			printf("MJPEG EnableEncode:\nWidth: %d\nHeight: %d\nBitrate: %d\nViFrameRate: %d\nFrameRate: %d\n", 
					stMjpegAttr.u32PicWidth, 
					stMjpegAttr.u32PicHeight, 
					stMjpegAttr.u32TargetBitrate, 
					stMjpegAttr.u32ViFramerate,
					stMjpegAttr.u32TargetFramerate);		
#endif					
		}
		break;
		
	case 2:		// MPEG4
		{
			VENC_ATTR_MPEG4_S stMpeg4Attr;
			
			memset(&stMpeg4Attr, 0, sizeof(VENC_ATTR_MPEG4_S));
			
			stMpeg4Attr.u32Priority = 0;
			stMpeg4Attr.u32PicWidth = pEncParam->nEncodeWidth;
			stMpeg4Attr.u32PicHeight = pEncParam->nEncodeHeight;		
								
#ifdef HD_CMOS
			stMpeg4Attr.bVIField = HI_FALSE;
#endif

#ifdef CCD				
			if (stMpeg4Attr.u32PicHeight==576 || stMpeg4Attr.u32PicHeight==480)
			{
				stMpeg4Attr.bVIField = HI_TRUE;
			}
			else
			{
				stMpeg4Attr.bVIField = HI_FALSE;
			}
#endif				
			stMpeg4Attr.u32TargetBitrate = pEncParam->nBitRate/1024;
				
#ifdef HD_CMOS	
			stMpeg4Attr.u32ViFramerate = 30;
#endif	

#ifdef CCD			
			if (stMpeg4Attr.u32PicHeight==576 || stMpeg4Attr.u32PicHeight==288 || stMpeg4Attr.u32PicHeight==144)
			{
				stMpeg4Attr.u32ViFramerate = 25;
			}
			else
			{
				stMpeg4Attr.u32ViFramerate = 30;
			}
#endif				
			stMpeg4Attr.u32TargetFramerate = pEncParam->nFramerate;

#ifdef HD_CMOS
			stMpeg4Attr.u32BufSize = 640*480*2*2;
#endif

#ifdef CCD			
			stMpeg4Attr.u32BufSize = 720*576*2*2;
#endif				
			stMpeg4Attr.u32Gop = pEncParam->nKeyInterval;
			stMpeg4Attr.u32MaxDelay = 1;
    			
			memset(pstAttr, 0 ,sizeof(VENC_CHN_ATTR_S));
			//pstAttr->enType = PT_H264;
			//pstAttr->pValue = (HI_VOID *)&stMpeg4Attr;
			
			// Add the code by Jerry.Zhuang, 2009-04-21
			pstAttr->pValue = (HI_VOID *)&g_hi3511_stMpeg4Attr;
			memcpy(&g_hi3511_stMpeg4Attr, &stMpeg4Attr, sizeof(VENC_ATTR_MPEG4_S));
#ifdef VENC_DEBUG
			printf("MPEG4 EnableEncode:\nWidth: %d\nHeight: %d\nBitrate: %d\nViFrameRate: %d\nFrameRate: %d\nGOP: %d\n", 
					stMpeg4Attr.u32PicWidth, 
					stMpeg4Attr.u32PicHeight, 
					stMpeg4Attr.u32TargetBitrate, 
					stMpeg4Attr.u32ViFramerate,
					stMpeg4Attr.u32TargetFramerate,
					stMpeg4Attr.u32Gop);
#endif		
						
		}		
		break;
		
	case 3:		// MOBILE
		{
			// JPEG Snapshot
			VENC_ATTR_JPEG_S stJpegAttr;
			
			memset(&stJpegAttr, 0, sizeof(VENC_ATTR_JPEG_S));
			
			stJpegAttr.u32BufSize = pEncParam->nEncodeWidth*pEncParam->nEncodeHeight*2;
			stJpegAttr.u32PicWidth = pEncParam->nEncodeWidth;
			stJpegAttr.u32PicHeight = pEncParam->nEncodeHeight;
			stJpegAttr.bVIField = HI_FALSE;
			stJpegAttr.bByFrame = HI_TRUE;
			stJpegAttr.u32MCUPerECS = pEncParam->nEncodeWidth*pEncParam->nEncodeHeight/256;
			stJpegAttr.u32ImageQuality = 1;	// old:0[0~5]
		
			memset(pstAttr, 0 ,sizeof(VENC_CHN_ATTR_S));
			pstAttr->enType = PT_JPEG;
			pstAttr->pValue = (HI_VOID *)&g_hi3511_stJpegAttr;
			memcpy(&g_hi3511_stJpegAttr, &stJpegAttr, sizeof(VENC_ATTR_JPEG_S));
		}
		break;
	}
	
	return 0;
}


int VideoEncSetup(int nChannel, int nStreamType, VENC_PARAM *pEncParam)
{
	HI_S32 s32Ret = 0;
	VENC_CHN_ATTR_S stAttr;
	
	int nChn = 0;

	if (nChannel<0 || nChannel>=MAX_ENC_CHANNEL)
	{
		return -1;	
	}
	if (nStreamType<0 || nStreamType>=MAX_CHANNEL_ENC_NUM+1)
	{
		return -1;	
	}			
	if (pEncParam == NULL)
	{
		return -1;
	}
	printf("Entry VideoEncSetup nStreamType = %d\n", nStreamType);
	switch (nStreamType)
		{
case 0:
	{
		// 第一种码流
		VENC_ATTR_JPEG_S stJpegAttr;
		
		memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));
		memset(&stJpegAttr, 0, sizeof(VENC_ATTR_JPEG_S));
		
		nChn = 0;
		s32Ret = GetVideoNewEncAttr(pEncParam, &stAttr);
		if (s32Ret != 0)
		{		
			return -1;
		}
		#if 0		
		s32Ret = EnableEncode(nChn, nChn, 0, nChannel, &stAttr);
		if (s32Ret != 0)
		{
			printf("EnableEncode1: Failed(%d)\n", __LINE__);
			return -1;
		}
		g_hi3511_h264_chn_flag[nChannel][0] = 1;
		
		// Add the code by Jerry.Zhuang, 2009-04-23
		g_hi3511_venc_type[nChannel][nStreamType] = pEncParam->reserve;
					
		// Add the code by Jerry.Zhuang, 2009-01-31
		g_hi3511_h264_width[nChannel] = pEncParam->nEncodeWidth;
		g_hi3511_h264_height[nChannel] = pEncParam->nEncodeHeight;
		
		// JPEG Snapshot
		memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));
		memset(&stJpegAttr, 0, sizeof(VENC_ATTR_JPEG_S));
		
		nChn = nChannel*4+2;
		if (g_hi3511_jpeg_chn_flag[nChannel] == 1)
		{
			printf("DisableSnapshot: %d\n", __LINE__);
			DisableSnapshot(nChn, nChn);
			g_hi3511_jpeg_chn_flag[nChannel] = 0;
		}
	
#ifdef HD_CMOS
		switch (pEncParam->nEncodeHeight)
		{
		case 1200:
			stJpegAttr.u32BufSize = 1600*1200*2;
			stJpegAttr.u32PicWidth = pEncParam->nEncodeWidth;
			stJpegAttr.u32PicHeight = pEncParam->nEncodeHeight;
			stJpegAttr.bVIField = HI_FALSE;
			stJpegAttr.bByFrame = HI_TRUE;
			stJpegAttr.u32MCUPerECS = 7500;
			stJpegAttr.u32ImageQuality = 3;	// old:0[0~5]
			break;
						
		case 720:
			stJpegAttr.u32BufSize = 1280*720*2;
			stJpegAttr.u32PicWidth = pEncParam->nEncodeWidth;
			stJpegAttr.u32PicHeight = pEncParam->nEncodeHeight;
			stJpegAttr.bVIField = HI_FALSE;
			stJpegAttr.bByFrame = HI_TRUE;
			stJpegAttr.u32MCUPerECS = 3600;
			stJpegAttr.u32ImageQuality = 3;	// old:0[0~5]
			break;

		case 576:
			stJpegAttr.u32BufSize = 576*720*2;
			stJpegAttr.u32PicWidth = pEncParam->nEncodeWidth;
			stJpegAttr.u32PicHeight = pEncParam->nEncodeHeight;
			stJpegAttr.bVIField = HI_FALSE;
			stJpegAttr.bByFrame = HI_TRUE;
			stJpegAttr.u32MCUPerECS = 1620;
			stJpegAttr.u32ImageQuality = 3;	// old:0[0~5]
			printf("stJpegAttr.u32PicWidth = %d, stJpegAttr.u32PicHeight= %d", stJpegAttr.u32PicWidth, stJpegAttr.u32PicHeight);
			break;
			
		case 480:
			stJpegAttr.u32BufSize = 640*480*2;
			stJpegAttr.u32PicWidth = pEncParam->nEncodeWidth;
			stJpegAttr.u32PicHeight = pEncParam->nEncodeHeight;
			stJpegAttr.bVIField =  HI_FALSE;
			stJpegAttr.bByFrame = HI_TRUE;
			stJpegAttr.u32MCUPerECS = 600;
			stJpegAttr.u32ImageQuality = 3; // old:0[0~5]
			printf("640*480--->stJpegAttr.u32PicWidth = %d, stJpegAttr.u32PicHeight= %d", stJpegAttr.u32PicWidth, stJpegAttr.u32PicHeight);
			break;

		case 288:
			stJpegAttr.u32BufSize = 320*288*2;
			stJpegAttr.u32PicWidth = pEncParam->nEncodeWidth;
			stJpegAttr.u32PicHeight = pEncParam->nEncodeHeight;
			stJpegAttr.bVIField = HI_FALSE;
			stJpegAttr.bByFrame = HI_TRUE;
			stJpegAttr.u32MCUPerECS = 150;
			stJpegAttr.u32ImageQuality = 3; // old:0[0~5]
			printf("320*288--->stJpegAttr.u32PicWidth = %d, stJpegAttr.u32PicHeight= %d", stJpegAttr.u32PicWidth, stJpegAttr.u32PicHeight);
			break;;
			
		case 240:
			stJpegAttr.u32BufSize = 320*240*2;
			stJpegAttr.u32PicWidth = pEncParam->nEncodeWidth;
			stJpegAttr.u32PicHeight = pEncParam->nEncodeHeight;
			stJpegAttr.bVIField = HI_FALSE;
			stJpegAttr.bByFrame = HI_TRUE;
			stJpegAttr.u32MCUPerECS = 140;
			stJpegAttr.u32ImageQuality = 3; // old:0[0~5]
			printf("320*240--->stJpegAttr.u32PicWidth = %d, stJpegAttr.u32PicHeight= %d", stJpegAttr.u32PicWidth, stJpegAttr.u32PicHeight);
			break;
			
			
		}				
#endif
	
#ifdef CCD				
		stJpegAttr.u32BufSize = 720*576*2;
		stJpegAttr.u32PicWidth = 720;
		switch (pEncParam->nEncodeHeight)
		{
		case 576:
		case 288:
		case 144:
			stJpegAttr.u32PicHeight = 576;
			stJpegAttr.u32MCUPerECS = 1620;
			break;
	
		case 480:
		case 240:
		case 112:
		case 128:
			stJpegAttr.u32PicHeight = 480;
			stJpegAttr.u32MCUPerECS = 1350;
			break;
		}
		stJpegAttr.bVIField = HI_TRUE;
		stJpegAttr.bByFrame = HI_TRUE;
		stJpegAttr.u32ImageQuality = 1;	// old:0[0~5]
#endif				
	
		memset(&stAttr, 0 ,sizeof(VENC_CHN_ATTR_S));
		stAttr.enType = PT_JPEG;
		stAttr.pValue = (HI_VOID *)&stJpegAttr;
				
		//printf("EnableSnapshot: %d %d %d %d\n", nChn, nChn, 0, nChannel);
	
		s32Ret = EnableSnapshot(nChn, nChn, 0, nChannel, &stAttr);
		if (s32Ret != 0)
		{
			printf("EnableSnapshot: Failed 0x%x\n", s32Ret);
			g_hi3511_jpeg_chn_flag[nChannel] = 0;
			return -1;
		}
		else
		{
			printf("EnableSnapshot: OK\n");
			g_hi3511_jpeg_chn_flag[nChannel] = 1;
		}	
		#endif
	}



	break;
	
	
	case 1:
	{
		// 第二种码流
		memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));
		
		nChn = 1;
				
		if (g_hi3511_h264_chn_flag[nChannel][1] == 1)
		{
			printf("DisableEncode2: %d\n", __LINE__);
			DisableEncode(nChn, nChn);
			g_hi3511_h264_chn_flag[nChannel][1] = 0;
		}
			
		s32Ret = GetVideoEncAttr(pEncParam, &stAttr);
		if (s32Ret != 0)
		{
			printf("GetVideoEncAttr2: Failed(%d)\n", __LINE__);
			return -1;
		}
				
		s32Ret = EnableEncode(nChn, nChn, 0, nChannel, &stAttr);
		if (s32Ret != 0)
		{
			printf("EnableEncode2: Failed(%d)\n", __LINE__);
			return -1;
		}
		g_hi3511_h264_chn_flag[nChannel][1] = 1;
		
		// Add the code by Jerry.Zhuang, 2009-04-23
		g_hi3511_venc_type[nChannel][nStreamType] = pEncParam->reserve;
	}
	break;
	
	case 2:
	{
		// 手机码流
		VENC_ATTR_JPEG_S stJpegAttr;
		
		memset(&stAttr, 0, sizeof(VENC_CHN_ATTR_S));
		memset(&stJpegAttr, 0, sizeof(VENC_ATTR_JPEG_S));
		
		nChn = 2;
				
		if (g_hi3511_mobile_chn_flag[nChannel] == 1)
		{
			printf("DisableEncode3: %d\n", __LINE__);
			//DisableSnapshot(nChn, nChn);
			DisableSnapshotExt(nChn, nChn);
			g_hi3511_mobile_chn_flag[nChannel] = 0;
		}
		
		pEncParam->reserve = 3;
		s32Ret = GetVideoEncAttr(pEncParam, &stAttr);
		if (s32Ret != 0)
		{
			printf("GetVideoEncAttr3: Failed(%d)\n", __LINE__);
			return -1;
		}
		
		//s32Ret = EnableSnapshot(nChn, nChn, 0, nChannel, &stAttr);
		s32Ret = EnableSnapshotExt(nChn, nChn, 0, nChannel, &stAttr);
		if (s32Ret != 0)
		{
			printf("EnableSnapshot: Failed 0x%x\n", s32Ret);
			g_hi3511_mobile_chn_flag[nChannel] = 0;
			return -1;
		}
				
		MOBILE_EnableOSD();
			
		g_hi3511_mobile_chn_flag[nChannel] = 1;
	}	
	break;
	
	}

	return 0;
}

int Hi3511EncSetup(int nChannel, int nStreamType, int opt, void *param)
{
	int ret = -1;

	if (nChannel<0 || nChannel>=MAX_ENC_CHANNEL)
	{
		return -1;	
	}
	if (opt < 0)
	{
		return -1;
	}
	if (param == NULL)
	{
		return -1;
	}
	
	switch (opt)
	{
	case venc_setup_enc_param:
		{
			if (param == NULL)
			{
				return -1;
			}
			
			VIDEO_ENC_PARAM *pParam = (VIDEO_ENC_PARAM *)param;			
			VENC_PARAM *pEncParam = &(pParam->param.encParam);
			
			ret = VideoEncSetup(nChannel, nStreamType, pEncParam);

			printf("Hi3511EncSetup: %d %d (%d)\n", nChannel, nStreamType, ret);
		}
		break;

	case venc_setup_md:
		{
			
			int ret;
			unsigned int md = 0;			
			VIDEO_ENC_PARAM *pEncParam = (VIDEO_ENC_PARAM *)param; 				
			
			md = pEncParam->param.nMD;
			if (md!=0 && md!=1)
			{
				break;
			}
			
			if (md == 1)
			{
				if (g_hi3511_md_flag[nChannel] == 0)
				{
					ret = EnableMD(nChannel, g_hi3511_md_value[nChannel]); 
					if (ret != 0)
					{
						break;
					}
					g_hi3511_md_flag[nChannel] = 1;
				}
			}
			else
			{
				if (g_hi3511_md_flag[nChannel] == 1)
				{
					ret = DisableMD(nChannel); 
					if (ret != 0)
					{
						break;
					}
					g_hi3511_md_flag[nChannel] = 0;
				}
			}
			
			ret = 0;			
		}
		break;

	case venc_setup_md_area:
		{
			int hiRet;	
			int i = 0;
			int j = 0;
			int count = 0;
			HI_U8 mask[1620];
			VIDEO_ENC_PARAM *pEncParam = (VIDEO_ENC_PARAM *)param; 	
			
			// Add the code by Jerry.Zhuang, 2009-01-31
			g_hi3511_md_value[nChannel] = pEncParam->param.mdParam.nSensibility;
			mdMacroConvertExt(pEncParam->param.mdParam.mask, g_hi3511_md_mask[nChannel], g_hi3511_h264_width[nChannel], g_hi3511_h264_height[nChannel], 1);
			g_hi3511_md_count[nChannel] = macroCount(g_hi3511_md_mask[nChannel], (g_hi3511_h264_width[nChannel]/16)*(g_hi3511_h264_height[nChannel]/16));
			
			ret = 0;
		}
		break;

	case venc_setup_osd:
		{
			unsigned int nIndex = 0;	
			VIDEO_ENC_PARAM *pEncParam = (VIDEO_ENC_PARAM *)param; 
			
			nIndex = pEncParam->param.encOSD.nIndex;
			if (nIndex > 4)
			{
				break;
			}
			//printf("OSD Setup(%d): %d %d %s %d\n", nIndex, pEncParam->param.encOSD.data.nxPos, pEncParam->param.encOSD.data.nyPos, pEncParam->param.encOSD.data.data, pEncParam->param.encOSD.nShow);
			//ret = OSDSetup(nChannel, nIndex, pEncParam->param.encOSD.data, pEncParam->param.encOSD.nShow);
			ret = OSDSetup(nChannel, nStreamType, nIndex, pEncParam->param.encOSD.data, pEncParam->param.encOSD.nShow);
		}
		break;
		
	case venc_setup_logo:
		{
			VIDEO_ENC_PARAM *pEncParam = (VIDEO_ENC_PARAM *)param; 

			ret = LOGOSetup(nChannel, pEncParam->param.encLogo.data, pEncParam->param.encLogo.nShow);
		}
		break;

	default:
		ret = -1;
		//ret = 0;
		break;			
	}
	
	return ret;
}

int Hi3511EncGetSetup(int nChannel, int nStreamType, int opt, void *param)
{
	return 0;
}

int Hi3511EncStart(int nChannel, int nStreamType)
{	
	return 0;
}

int Hi3511EncStop(int nChannel, int nStreamType)
{
	return 0;
}

int Hi3511EncGetStream(int nChannel, int nStreamType, void *stream, int *size, int fpH264File)
{
	HI_S32 s32Ret;
	HI_S32 s32VencFd = -1;
	HI_S32 VeChn = 0;
	VENC_CHN_STAT_S stStat;
	VENC_STREAM_S stStream;
	fd_set read_fds;
	struct timeval TimeoutVal; 
	
	VIDOE_FRAME_HEADER *head = NULL;
	unsigned char *temp = NULL;
	int i = 0;
	int offset = 0;
	
	#if 0	
	if (stream==NULL || size==NULL)
	{
		return -1;
	}
	if (nChannel<0 || nChannel>=MAX_ENC_CHANNEL)
	{
		return -1;
	}
	if (nStreamType<0 || nStreamType>=MAX_CHANNEL_ENC_NUM)
	{
		return -1;	
	}
	#endif
	
	
	// Add the code by Jerry.Zhuang, 2009-06-19
	if (g_hi3511_h264_chn_flag[nChannel][nStreamType] != 1)
	{
		//return -1;
	}
	g_hi3511_h264_chn_flag[nChannel][nStreamType] = 1;
	VeChn = 0;

	*size = 0;
	//printf("g_hi3511_h264_chn_flag[nChannel][nStreamType] = %d\n", g_hi3511_h264_chn_flag[nChannel][nStreamType] );
			
	//VeChn = nChannel*3+nStreamType;
	
	//pthread_mutex_lock(&g_hi3511_mutex);
    
	s32VencFd = HI_MPI_VENC_GetFd(0);
	if (s32VencFd < 0)
	{
		printf("HI_MPI_VENC_GetFd(): Failed, %d\n", g_hi3511_h264_timeout_count[nChannel][nStreamType]);

		// Add the code by Jerry.Zhuang, 2009-03-27
		g_hi3511_h264_timeout_count[nChannel][nStreamType]++;
				
		if (g_hi3511_h264_timeout_count[nChannel][nStreamType] > 5)
		{			
			return -2;	
		}
		
		return -1;
	}
	
	FD_ZERO(&read_fds);
	FD_SET(s32VencFd, &read_fds);
	TimeoutVal.tv_sec = 3;
	TimeoutVal.tv_usec = 0;
	//printf("__func__ %s line %d\n", __func__, __LINE__);
	s32Ret = select(s32VencFd+1, &read_fds, NULL, NULL, &TimeoutVal);
		
	if (s32Ret < 0) 
	{
		printf("select error: %d %d\n", nStreamType, g_hi3511_h264_timeout_count[nChannel][nStreamType]);
		
		// Add the code by Jerry.Zhuang, 2009-03-27
		g_hi3511_h264_timeout_count[nChannel][nStreamType]++;
		if (g_hi3511_h264_timeout_count[nChannel][nStreamType] > 5)
		{		
			return -2;	
		}
		
		return -1;
	}
	else if (0 == s32Ret) 
	{
		printf("select time out: %d %d\n", nStreamType, g_hi3511_h264_timeout_count[nChannel][nStreamType]);
		
		// Add the code by Jerry.Zhuang, 2009-03-27
		g_hi3511_h264_timeout_count[nChannel][nStreamType]++;
		if (g_hi3511_h264_timeout_count[nChannel][nStreamType] > 5)
		{			
			return -2;	
		}
				
		return -1;
	}
	else
	{
		if (FD_ISSET(s32VencFd, &read_fds))
		{
		//printf("__func__ %s line %d\n", __func__, __LINE__);
	
			// Add the code by Jerry.Zhuang, 2009-04-14
			memset(&stStream, 0, sizeof(stStream));
			memset(&stStat, 0, sizeof(stStat));
			
			s32Ret = HI_MPI_VENC_Query(VeChn, &stStat);
			if (s32Ret != HI_SUCCESS) 
			{
				printf("HI_MPI_VENC_Query:0x%x err\n", s32Ret);
								
				return -1;
			}
			
			// Add the code by Jerry.Zhuang, 2009-03-30
			if (sizeof(VENC_PACK_S)*stStat.u32CurPacks > MAX_BUFF_SIZE)
			{
				printf("HI_MPI_VENC_Query: %d > %d\n", sizeof(VENC_PACK_S)*stStat.u32CurPacks, MAX_BUFF_SIZE);
								
				return -1;
			}
				
			stStream.pstPack = (VENC_PACK_S*)g_hi3511_h264_buffer[nStreamType];

			if (NULL == stStream.pstPack)  
			{
				printf("malloc memory err!\n");
				
				return -1;
			}

			stStream.u32PackCount = stStat.u32CurPacks;
			s32Ret = HI_MPI_VENC_GetStream(VeChn, &stStream, HI_FALSE);
			if (HI_SUCCESS != s32Ret) 
			{
			//	printf("HI_MPI_VENC_GetStream:0x%x\n",s32Ret);
				stStream.pstPack = NULL;
				
				g_hi3511_h264_timeout_count[nChannel][nStreamType]++;
				if (g_hi3511_h264_timeout_count[nChannel][nStreamType] > 5)
				{
					return -2;	
				}
					
				return -1;
			}
			head = (VIDOE_FRAME_HEADER *)stream;
			temp = stream+sizeof(VIDOE_FRAME_HEADER);
			
			// Add the code by Jerry.Zhuang, 2009-04-23
			if (g_hi3511_venc_type[nChannel][nStreamType] == 1)
			{
				memcpy(temp, g_SOI, sizeof(g_SOI));
				offset += sizeof(g_SOI);
			}		
	
			for (i=0; i< stStream.u32PackCount; i++)
			{
				// Add the code by Jerry.Zhuang, 2009-03-30
				if (stStream.pstPack[i].u32Len[0]+offset > MAX_BUFF_SIZE)
				{
					// Add the code by Jerry.Zhuang, 2009-04-18
					HI_MPI_VENC_ReleaseStream(VeChn, &stStream);
					stStream.pstPack = NULL;
					
					printf("[DEBUG]: Hi3511EncGetStream(0: %d + %d > %d)\n", stStream.pstPack[i].u32Len[0], offset, MAX_BUFF_SIZE);
						
					return -1;
				}
				
				memcpy(temp+offset, stStream.pstPack[i].pu8Addr[0], stStream.pstPack[i].u32Len[0]);
				offset += stStream.pstPack[i].u32Len[0];

				#if 0
				fwrite(stStream.pstPack[i].pu8Addr[0], stStream.pstPack[i].u32Len[0], 1, fpH264File);
				fflush(fpH264File);
				#endif
				if (stStream.pstPack[i].u32Len[1] > 0)
				{
					// Add the code by Jerry.Zhuang, 2009-03-30
					if (stStream.pstPack[i].u32Len[1]+offset > MAX_BUFF_SIZE)
					{
						// Add the code by Jerry.Zhuang, 2009-04-18
						HI_MPI_VENC_ReleaseStream(VeChn, &stStream);
						stStream.pstPack = NULL;	
					
						printf("[DEBUG]: Hi3511EncGetStream(1: %d + %d > %d)\n", stStream.pstPack[i].u32Len[1], offset, MAX_BUFF_SIZE);
													
						return -1;
					}
				
					memcpy(temp+offset, stStream.pstPack[i].pu8Addr[1], stStream.pstPack[i].u32Len[1]);
					offset += stStream.pstPack[i].u32Len[1];
					#if 0
					fwrite(stStream.pstPack[i].pu8Addr[1], stStream.pstPack[i].u32Len[1], 1, fpH264File);
					fflush(fpH264File);
					#endif
				}	
			}

			//printf("offset = %d\n", offset);
			
			write(fpH264File, temp, offset);
			//usleep(1000);
		
			
			// Add the code by Jerry.Zhuang, 2009-04-23
			if (g_hi3511_venc_type[nChannel][nStreamType] == 1)
			{
				memcpy(temp+offset, g_EOI, sizeof(g_EOI));
				offset += sizeof(g_EOI);
			}	

			head->timestamp = stStream.pstPack[0].u64PTS/1000;
			
			if (g_hi3511_venc_type[nChannel][nStreamType] == 0)
			{
				if (stStream.pstPack[0].DataType.enH264EType == H264E_NALU_SPS)
				{
					head->IFrameFlag = 1;
				}
				else
				{
					head->IFrameFlag = 0;
				}
			}
			else
			{
				head->IFrameFlag = 1;
			}

			//printf("Stream: %d\n", stStream.pstPack[0].DataType.enH264EType);

			head->nSeq = stStream.u32Seq;

				
			s32Ret = HI_MPI_VENC_ReleaseStream(0, &stStream);
			if (s32Ret) 
			{
				printf("HI_MPI_VENC_ReleaseStream:0x%x\n", s32Ret);
				stStream.pstPack = NULL;
				
				g_hi3511_h264_timeout_count[nChannel][nStreamType]++;
				if (g_hi3511_h264_timeout_count[nChannel][nStreamType] > 5)
				{
					return -2;	
				}
					
				return -1;
			}
			free(stStream.pstPack);
			stStream.pstPack = NULL;			
		}
		else
		{
			g_hi3511_h264_timeout_count[nChannel][nStreamType]++;
			if (g_hi3511_h264_timeout_count[nChannel][nStreamType] > 5)
			{
				return -2;	
			}
				
			return -1;
		}
	}
	
	*size = offset + sizeof(VIDOE_FRAME_HEADER);
	g_hi3511_h264_timeout_count[nChannel][nStreamType] = 0;

	//pthread_mutex_unlock(&g_hi3511_mutex);
	
	return 0;
}

int Hi3511EncReleaseStream(int nChannel, int nStreamType)
{
	return 0;
}

int Hi3511EncGetJPEG(int nChannel, int nStreamType, void *stream, int *size)
{	
	int nRet = -1;
	int nChn = 0;
	
	if (nStreamType == 3)	// Mobile Stream
	{
		nChn = nChannel*3+3;
		*size = 0;
		nRet = GetSnapPic(nChn, stream, size);
		//nRet = StartSnap(nChn, nChn, 0, 0, stream, size);
		if (nRet != HI_SUCCESS)
		{
			printf("GetSnapPic err 0x%x\n", nRet);
			return -1;
		}
		else
		{
			//printf("StartSnap(%d %d)\n", nChn, *size);
		}
	}
	else
	{
		nChn = nChannel*3+2;
		*size = 0;
		
		nRet = StartSnap(nChn, nChn, 0, 0, stream, size);
	
		//printf("StartSnap(%d %d)\n", nChn, *size);
	}
		
	return nRet;
}

int Hi3511EncGetVideo(int nChannel, int nStreamType, void *stream, int *size)
{	
	return 0;
}

int Hi3511EncReleaseVideo(int nChannel, int nStreamType)
{
	return 0;
}

int Hi3511EncRequestIFrame(int nChannel, int nStreamType)
{
	return 0;
}

int Hi3511EncInsertUserInfo(int nChannel, int nStreamType, void *info, int size)
{
	return 0;	
}

int Hi3511EncGetMDStatus(int nChannel, int nStreamType, MD_STATUS *status)
{
	int nRet = 0;

	if (g_hi3511_md_flag[nChannel] == 1)
	{
		nRet = GetMD(nChannel);
	}	

	return nRet;	
}

static struct vencModuleInfo Hi3511VideoEncInfoStruct = 
{
	open:			Hi3511EncOpen,
	close:			Hi3511EncClose,
	setup:			Hi3511EncSetup,
	getSetup:		Hi3511EncGetSetup,
	start:			Hi3511EncStart,
	stop:			Hi3511EncStop,
	getStream:		Hi3511EncGetStream,
	releaseStream:	Hi3511EncReleaseStream,
	getJpeg:		Hi3511EncGetJPEG,

	getVideo:		Hi3511EncGetVideo,
	releaseVideo:	Hi3511EncReleaseVideo,
	requestIFrame:	Hi3511EncRequestIFrame,
	inserUserInfo:	Hi3511EncInsertUserInfo,
	getMDStatus:	Hi3511EncGetMDStatus,
	
};

vencModuleInfo_t Hi3511VideoEncInfo = &Hi3511VideoEncInfoStruct;

