#ifndef __VENC_MODULE_H_
#define __VENC_MODULE_H_

#define MAX_ENC_CHANNEL 	4
#define MAX_CHANNEL_ENC_NUM 2

#define MAX_OSD_NUM			4

#define MAX_MASK_NUM			5
#define MAX_MARSEILLES_NUM	1

#define V_ENC_D1      		4
#define V_ENC_HD1     		3
#define V_ENC_CIF     		2
#define V_ENC_QCIF    		1
#define V_ENC_NOVIEW  		0

typedef enum 
{
	venc_setup_enc_param,
	venc_get_enc_param,

	venc_setup_enc_framerate,
	venc_get_enc_framerate,
	venc_setup_enc_bitrate,
	venc_get_enc_bitrate,
	venc_setup_enc_gop,
	venc_get_enc_gop,
	venc_setup_enc_maxqp,
	venc_get_enc_maxqp,
	venc_setup_enc_mode,
	venc_get_enc_mode,

	// CBR
	venc_setup_cbr,
	venc_get_cbr,

	// VBR
	venc_setup_vbr,
	venc_get_vbr,

	// MD
	venc_setup_md,
	venc_get_md,
	venc_setup_md_area,
	venc_get_md_area,

	// JPEG
	venc_setup_jpeg_q,
	venc_get_jpeg_q,
    
	// OSD
	venc_creat_osd,
	venc_delete_osd,
	venc_setup_osd,
	venc_setup_osd_show,
	venc_setup_osd_alpha,
	venc_get_osd_alpha,
	venc_setup_osd_level,
	venc_get_osd_level,
	venc_setup_osd_position,
	venc_get_osd_position,  
    
	// LOGO
	venc_creat_logo,
	venc_delete_logo,
	venc_setup_logo,
	venc_setup_logo_show,
	venc_setup_logo_alpha,
	venc_get_logo_alpha,
	venc_setup_logo_level,
	venc_get_logo_level,
	venc_setup_logo_position,
	venc_get_logo_position,     

	// MASK
	venc_creat_mask,
	venc_delete_mask,
	venc_setup_mask,
	venc_setup_mask_show,
	venc_setup_mask_alpha,    
	venc_setup_mask_level,
	venc_get_mask_level,
	venc_setup_mask_position,
	venc_get_mask_position,

	// WATERMARK
	venc_setup_watermark,
	venc_get_watermark,

	// MARSEILLES
	venc_setup_mosaic,
	venc_get_mosaic,
	
	// Dynamic Logo
	venc_setup_dynamic_logo,
	venc_get_dynamic_logo,
}venc_opt;


typedef struct
{
	unsigned long nEncodeMode;		// 0: CBR, 1: VBR
	
	unsigned long nEncodeWidth;		
	unsigned long nEncodeHeight;	
	unsigned long nKeyInterval;		
	unsigned long nFramerate;		 
	unsigned long nBitRate;			
	unsigned long nMaxQuantizer;	
	unsigned long nMinQuantizer;	
	
	unsigned long reserve;			
	
}VENC_PARAM;

typedef struct
{
	unsigned char chMask[1620];
	unsigned char chValue[1620];
	
}MD_STATUS;

typedef struct
{
	unsigned int nxPos;
	unsigned int nyPos;
	
}POSITION_PARAM;

typedef struct 
{	
	unsigned int nxPos;
	unsigned int nyPos;
	unsigned char data[128];
	unsigned long color;

}OSD_DATA_PARAM;
typedef struct
{
	unsigned int nxPos;
	unsigned int nyPos;
	unsigned char ndata_len;
	unsigned int  nShow;
}OSD_TO_JPGE;


typedef struct 
{
	unsigned int nChannel;
	unsigned int nIndex;
	unsigned int nShow;
	OSD_DATA_PARAM data;

}VIDEO_OSD_PARAM;

typedef struct 
{	
	unsigned int nxPos;
	unsigned int nyPos;
	unsigned int nWidth;
	unsigned int nHeight;
	unsigned char *data;

}LOGO_DATA_PARAM;

typedef struct 
{
	unsigned int nChannel;
	unsigned int nIndex;
	unsigned int nShow;
	LOGO_DATA_PARAM data;

}VIDEO_LOGO_PARAM;

typedef struct 
{
	unsigned int nxPos;
	unsigned int nyPos;
	unsigned int nWidth;
	unsigned int nHeight;
	
}MASK_DATA_PARAM;

typedef struct 
{
	unsigned int nChannel;
	unsigned int nIndex;
	unsigned int nShow;
	MASK_DATA_PARAM data;
	
}VIDEO_MASK_PARAM;

typedef struct
{
	unsigned int nChannel;
	unsigned int nOnFlag;
	char strKey[8];
	char strSymbol[16];
	unsigned int nDensity;
    
}WATERMARK_PARAM;

// 视频移动检测参数
typedef struct
{
	unsigned char nSensibility;				//视频移动检测的灵敏度
	unsigned char mask[11*9];				//视频移动检测的宏块
	unsigned long nReserve;					//保留
	
}VIDEO_MOTION_PARAM;

typedef struct videoEncParam 
{
	union 
	{
		//
		VENC_PARAM encParam;
		//vinEncCbrSetup CBR;
		//vinEncVbrSetup VBR;
		unsigned int nEncFrameRate;
		unsigned int nEncBitRate;
		unsigned int nEncKeyInterval;
		unsigned int nEncMaxQuantizer;
		unsigned int nEncMode;
        
		// MD
		unsigned int nMD;
		unsigned char chMotionArea[1620];
		
		// Add the code by lvjh, 2009-01-31
		VIDEO_MOTION_PARAM mdParam;
        
		// JPEG
		unsigned int nJpegQ;
        
		// OSD
		VIDEO_OSD_PARAM encOSD;
		unsigned int nOsdShow;
		unsigned int nOsdAlpha;
		unsigned int nOsdLevel;
		POSITION_PARAM osdPos;

		// LOGO
		VIDEO_LOGO_PARAM encLogo;
		unsigned int nLogoShow;
		unsigned int nLogoAlpha;
		unsigned int nLogoLevel;
		POSITION_PARAM logoPos;
        
		// MASK
		VIDEO_MASK_PARAM encMask;
		unsigned int nMaskShow;
		POSITION_PARAM maskPos;

		// 
		VIDEO_MASK_PARAM encMosaic;

		// WATERMARK
		WATERMARK_PARAM wmParam;
        
	}param;
    
}VIDEO_ENC_PARAM;

typedef struct
{
	unsigned long long timestamp;
	unsigned short IFrameFlag;
	unsigned int nSeq;
	unsigned short nReserve;

}VIDOE_FRAME_HEADER;

typedef struct vencModuleInfo  *vencModuleInfo_t;

typedef int (*vencOpenFun_t)(int nChannel, int nStreamType);
typedef int (*vencCloseFun_t)(int nChannel, int nStreamType);
typedef int (*vencSetupFun_t)(int nChannel, int nStreamType, int opt, void *param);
typedef int (*vencGetSetupFun_t)(int nChannel, int nStreamType, int opt, void *param);
typedef int (*vencStartFun_t)(int nChannel, int nStreamType);
typedef int (*vencStopFun_t)(int nChannel, int nStreamType);
typedef int (*vencGetStreamFun_t)(int nChannel, int nStreamType, void *stream, int *size, int fpH264File);
typedef int (*vencReleaseStreamFun_t)(int nChannel, int nStreamType);
typedef int (*vencGetJpegFun_t)(int nChannel, int nStreamType, void *stream, int *size);
typedef int (*vencGetVideoFun_t)(int nChannel, int nStreamType, void *stream, int *size);
typedef int (*vencReleaseVideoFun_t)(int nChannel, int nStreamType);
typedef int (*vencRequestIFrameFun_t)(int nChannel, int nStreamType);
typedef int (*vencInserUserInfoFun_t)(int nChannel, int nStreamType, void *info, int size);
typedef int (*vencGetMDStatusFun_t)(int nChannel, int nStreamType, MD_STATUS *status);

struct vencModuleInfo 
{
    vencOpenFun_t               open;
    vencCloseFun_t              close;
    vencSetupFun_t				setup;
    vencGetSetupFun_t			getSetup;
    vencStartFun_t				start;
    vencStopFun_t				stop;
    vencGetStreamFun_t			getStream;
    vencReleaseStreamFun_t		releaseStream;
    vencGetJpegFun_t			getJpeg;
    vencGetVideoFun_t			getVideo;
    vencReleaseVideoFun_t		releaseVideo;
    vencRequestIFrameFun_t		requestIFrame;
    vencInserUserInfoFun_t		inserUserInfo;
    vencGetMDStatusFun_t		getMDStatus;
};

#endif
