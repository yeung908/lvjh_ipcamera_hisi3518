#ifndef __tm3k_FDISK_H_
#define __tm3k_FDISK_H_

typedef struct
{
	unsigned int totalsize;
	unsigned int usedsize;
	unsigned int availablesize;
	unsigned int usedpercent;
}DISKSIZE, *PDISKSIZE;

int tm3k_HD_FDisk(char *Dev, int DataPartionSize, int BackupPartionSize);
int tm3k_HD_Format(char *Dev, int PartionNo);
int tm3k_HD_IsDiskFormated(char *Dev);
int tm3k_HD_GetDiskPartionNum(char *Dev);
int tm3k_HD_GetPartionVol(char *Dev, unsigned char PartionNo);
char *tm3k_HD_GetDiskName(int nDiskNo);
int tm3k_HD_GetDiskNum();
int tm3k_HD_GetDiskInfo(char *devicename, PDISKSIZE disksize);

#endif
