#ifndef __AOUT_MODULE_H_
#define __AOUT_MODULE_H_

#define MAX_AOUT_CHANNEL 4

typedef struct
{
	unsigned int nSample;
	unsigned int nChannel;
	unsigned int nBitsPerChannel;
}AOUT_PARAM;

typedef struct aoutModuleInfo  *aoutModuleInfo_t;

typedef int (*aoutOpenFun_t)(int nChannel);
typedef int (*aoutCloseFun_t)(int nChannel);
typedef int (*aoutSetupFun_t)(int nChannel, void *param);
typedef int (*aoutGetSetupFun_t)(int nChannel, void *param);
typedef int (*aoutStartFun_t)(int nChannel);
typedef int (*aoutStopFun_t)(int nChannel);
typedef int (*aoutGetStreamFun_t)(int nChannel, void *stream, int *size);
typedef int (*aoutReleaseStreamFun_t)(int nChannel);

struct aoutModuleInfo 
{
    aoutOpenFun_t				open;
    aoutCloseFun_t				close;
    aoutSetupFun_t				setup;
    aoutGetSetupFun_t			getSetup;
    aoutStartFun_t				start;
    aoutStopFun_t				stop;
    aoutGetStreamFun_t			getStream;
    aoutReleaseStreamFun_t		releaseStream;

};

#endif
