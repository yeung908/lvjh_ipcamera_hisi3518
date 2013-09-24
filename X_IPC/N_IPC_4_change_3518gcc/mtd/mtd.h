#ifndef _MTD_H_
#define _MTD_H_

typedef int (*SendUpdateProgress)(int nProgress);
int MTD_EraseRegion(char *pDevice, int nStart, int nCount);
int MTD_EraseAllRegion(char *pDevice);
int MTD_WriteRegion(char *pDevice);
int MTD_WriteAllRegion(char *pDevice, char *pDataBuffer, int nDataSize);
int MTD_WriteAllRegionExt(char *pDevice, char *pFileName);
int MTD_GetWriteProcess();
int MTD_SetCallBack(SendUpdateProgress pFunc);

#endif
