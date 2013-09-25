#ifndef __DVS_INIT_H_
#define __DVS_INIT_H_

int dvsInit();
int setVideoFormat(int nChannel, int nStreamType, int nWidth, int nHeight, int nVencType);
int getVideoFormat(int nChannel, int nStreamType, int *nWidth, int *nHeight, int *nVencType);

#endif
