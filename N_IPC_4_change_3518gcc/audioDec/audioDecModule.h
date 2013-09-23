#ifndef __ADEC_MODULE_H_
#define __ADEC_MODULE_H_

#define MAX_ADEC_CHANNEL 4

typedef enum 
{
	// ±àÂë²ÎÊý 
    adec_setup_dec_param,
    adec_get_dec_param,
        
}adec_opt;

typedef struct 
{
	int nChannels;
	int nSampleRate;
	int nBitRate;
	int nMode;
	
}ADEC_PARAM;

typedef struct audioDecParam 
{
    int opt;
    int channel;

    union 
	{
		ADEC_PARAM adecParam;
        
    }param;
    
}AUDIO_DEC_PARAM;

typedef struct adecModuleInfo  *adecModuleInfo_t;

typedef int (*adecOpenFun_t)(int nChannel, int encType);
typedef int (*adecCloseFun_t)(int nChannel);
typedef int (*adecSetupFun_t)(int opt, void *param);
typedef int (*adecGetSetupFun_t)(int opt, void *param);
typedef int (*adecStartFun_t)(int nChannel);
typedef int (*adecStopFun_t)(int nChannel);
typedef int (*adecSendStreamFun_t)(int nChannel, void *stream, int *size);
typedef int (*adecReleaseStreamFun_t)(int nChannel);
typedef int (*adecGetAudioFun_t)(int nChannel, void *stream, int *size);
typedef int (*adecReleaseAudioFun_t)(int nChannel);

struct adecModuleInfo 
{
    adecOpenFun_t               open;
    adecCloseFun_t              close;
    adecSetupFun_t				setup;
    adecGetSetupFun_t			getSetup;
    adecStartFun_t				start;
    adecStopFun_t				stop;
    adecSendStreamFun_t			sendStream;
    adecReleaseStreamFun_t		releaseStream;
	adecGetAudioFun_t			getAudio;
	adecReleaseAudioFun_t		releaseAudio;

};

#endif
