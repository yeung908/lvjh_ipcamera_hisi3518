#include <sys/types.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <libgen.h>
#include <ctype.h>
#include <time.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/mount.h>

#include <mtd/mtd-user.h>
#include <mtd/jffs2-user.h>

#include "mtd.h"
#include "crc32.h"

#define FLAG_NONE		0x00
#define FLAG_VERBOSE	0x01
#define FLAG_HELP		0x02
#define FLAG_FILENAME	0x04
#define FLAG_DEVICE		0x08

#define KB(x) ((x) / 1024)
#define PERCENTAGE(x,total) (((x) * 100) / (total))
#define BUFSIZE (10 * 1024)

int target_endian = __BYTE_ORDER;

int g_mtd_process = 0;
SendUpdateProgress g_send_update_progress_fun = NULL;

static int flash_eraseall(char *pDevice)
{
	mtd_info_t meminfo;
	int fd, clmpos = 0, clmlen = 8;
	erase_info_t erase;
	int isNAND, bbtest = 1;
	struct jffs2_unknown_node cleanmarker;
	int quiet = 0;
	int jffs2 = 1;
	int value = 0;

	if ((fd = open(pDevice, O_RDWR)) < 0) 
	{
		printf("Can not open the device: %s, Error: %s\n", pDevice, strerror(errno));
		return -1;
	}

	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) 
	{
		printf("Can not MEMGETINFO: %s, Error: %s\n", pDevice, strerror(errno));
		goto Exit;
	}

	erase.length = meminfo.erasesize;
	isNAND = meminfo.type == MTD_NANDFLASH ? 1 : 0;

	if (jffs2) 
	{
		cleanmarker.magic = cpu_to_je16 (JFFS2_MAGIC_BITMASK);
		cleanmarker.nodetype = cpu_to_je16 (JFFS2_NODETYPE_CLEANMARKER);
		
		if (!isNAND)
		{
			cleanmarker.totlen = cpu_to_je32 (sizeof (struct jffs2_unknown_node));
		}
		else 
		{
			struct nand_oobinfo oobinfo;

			if (ioctl(fd, MEMGETOOBSEL, &oobinfo) != 0) 
			{
				printf("Can not MEMGETOOBSEL: %s, Error: %s\n", pDevice, strerror(errno));
				goto Exit;
			}

			/* Check for autoplacement */
			if (oobinfo.useecc == MTD_NANDECC_AUTOPLACE) 
			{
				/* Get the position of the free bytes */
				if (!oobinfo.oobfree[0][1]) 
				{
					printf ("Eeep. Autoplacement selected and no empty space in oob\n");
					goto Exit;
				}
				clmpos = oobinfo.oobfree[0][0];
				clmlen = oobinfo.oobfree[0][1];
				if (clmlen > 8)
					clmlen = 8;
			} 
			else 
			{
				/* Legacy mode */
				switch (meminfo.oobsize) 
				{
				case 8:
					clmpos = 6;
					clmlen = 2;
					break;
				case 16:
					clmpos = 8;
					clmlen = 8;
					break;
				case 64:
					clmpos = 16;
					clmlen = 8;
					break;
				}
			}
			cleanmarker.totlen = cpu_to_je32(8);
		}
		cleanmarker.hdr_crc =  cpu_to_je32(crc32 (0, &cleanmarker,  sizeof (struct jffs2_unknown_node) - 4));
	}

	for (erase.start = 0; erase.start < meminfo.size; erase.start += meminfo.erasesize) 
	{
		if (bbtest) 
		{
			loff_t offset = erase.start;
			int ret = ioctl(fd, MEMGETBADBLOCK, &offset);
			if (ret > 0) 
			{
				if (!quiet)
					printf ("\nSkipping bad block at 0x%08x\n", erase.start);
				continue;
			} 
			else if (ret < 0) 
			{
				if (errno == EOPNOTSUPP) 
				{
					bbtest = 0;
					if (isNAND) 
					{
						printf("%s: Bad block check not available\n", pDevice);
						goto Exit;
					}
				} 
				else 
				{
					printf("\n%s: MTD get bad block failed: %s\n", pDevice, strerror(errno));
					goto Exit;
				}
			}
		}

		if (!quiet) 
		{
			printf("\rErasing %d Kibyte @ %x -- %2llu %% complete.",
			     meminfo.erasesize / 1024, erase.start,
			     (unsigned long long)
			     erase.start * 100 / meminfo.size);
		}

		if (ioctl(fd, MEMERASE, &erase) != 0) 
		{
			printf("\n%s: MTD Erase failure: %s\n", pDevice, strerror(errno));
			continue;
		}
		
		// Add the code by lvjh, 2009-02-27
		value = (unsigned long long)erase.start*100/meminfo.size/4;
		if (value < 1)
		{
			value = 1;
		}
		else if (value >= 26)
		{
			value = 26;
		}
		// 执行回调函数
		if (g_send_update_progress_fun)
		{
			g_send_update_progress_fun(value);
		}

		/* format for JFFS2 ? */
		if (!jffs2)
			continue;

		/* write cleanmarker */
		if (isNAND) 
		{
			struct mtd_oob_buf oob;
			oob.ptr = (unsigned char *) &cleanmarker;
			oob.start = erase.start + clmpos;
			oob.length = clmlen;
			if (ioctl (fd, MEMWRITEOOB, &oob) != 0) 
			{
				printf("\n%s: MTD writeoob failure: %s\n", pDevice, strerror(errno));
				continue;
			}
		} 
		else 
		{
			if (lseek (fd, erase.start, SEEK_SET) < 0) 
			{
				printf("\n%s: MTD lseek failure: %s\n", pDevice, strerror(errno));
				continue;
			}
			if (write (fd , &cleanmarker, sizeof (cleanmarker)) != sizeof (cleanmarker)) 
			{
				printf("\n%s: MTD write failure: %s\n", pDevice, strerror(errno));
				continue;
			}
		}
		if (!quiet)
		{
			printf (" Cleanmarker written at %x.", erase.start);
		}	
	}
	if (!quiet)
		printf("\n");
		
	return 0;
		
Exit:		
	if (fd > 0)
	{
		close(fd);	
	}
	
	return -1;
}

static int safe_open (const char *pathname, int flags)
{
	int fd = -1;

	fd = open (pathname, flags);
	if (fd < 0)
	{
		printf("While trying to open %s", pathname);
		if (flags & O_RDWR)
		  	printf (" for read/write access");
		else if (flags & O_RDONLY)
		  printf (" for read access");
		else if (flags & O_WRONLY)
		  	printf (" for write access");
		printf ("\n");
		
		return -1;
	 }

	return (fd);
}

static int safe_read (int fd, void *buf, size_t count, int verbose)
{
	ssize_t result;

	result = read (fd, buf, count);
	if (count != result)
	{
		if (verbose) 
			printf ("\n");
		if (result < 0)
		{
			printf ("While reading data from device\n");
			return -1;
		}
		printf("Short read count returned while reading from device\n");
		return -1;
	 }
	 
	 return result;
}

static int safe_rewind (int fd)
{
	if (lseek (fd, 0L, SEEK_SET) < 0)
	{
		printf ("While seeking to start of the device\n");
		return -1;
	}
	 
	return 0;
}

int all_region_write(int dev_fd, char *pDataBuffer, int nDataSize)
{
	int i = 0;
	int flags = 0;
	ssize_t result;
	size_t size,written;
	struct mtd_info_user mtd;
	struct erase_info_user erase;
	unsigned char *pSrc = NULL;
	unsigned char dest[BUFSIZE];
	int value = 0;
	
	flags = FLAG_VERBOSE;
	
	if (dev_fd <= 0)
	{
		return -1;
	}
	if (pDataBuffer == NULL)
	{
		return -1;
	}
	if (nDataSize <= 0)
	{
		return -1;
	}
	
	// does it fit into the device/partition
	if (ioctl (dev_fd, MEMGETINFO, &mtd) < 0)
	{
		printf("This doesn't seem to be a valid MTD flash device!\n");
		return -1;
	}
	 
	if (nDataSize > mtd.size)
	{
		return -1;
	}
	
	// erase enough blocks so that we can write the file
	erase.start = 0;
	erase.length = nDataSize & ~(mtd.erasesize - 1);
	if (nDataSize % mtd.erasesize)
	{
		erase.length += mtd.erasesize;
	}
		
	if (flags & FLAG_VERBOSE)
	{	
		int blocks = erase.length / mtd.erasesize;
			
		erase.length = mtd.erasesize;
		
		printf("Erasing blocks: 0/%d (0%%)\n", blocks);
		
		for (i=1; i<=blocks; i++)
		{
			printf ("Erasing blocks: %d/%d (%d%%)\n",i, blocks, PERCENTAGE (i,blocks));
			 
			if (ioctl (dev_fd,MEMERASE,&erase) < 0)
			{
				  printf("While erasing blocks 0x%.8x-0x%.8x\n",
						   (unsigned int) erase.start, (unsigned int) (erase.start + erase.length));
				  return -1;
			}
			erase.start += mtd.erasesize;
			
			// Add the code by lvjh, 2009-02-27
			value = PERCENTAGE (i,blocks)/4+25;	// 3
			if (value < 26)
			{
				value = 26;
			}
			else if (value >= 51)		// 33
			{
				value = 51;
			}
			// 执行回调函数
			if (g_send_update_progress_fun)
			{
				g_send_update_progress_fun(value);
			}
		}
		
		printf("Erasing blocks: %d/%d (100%%)\n",blocks,blocks);
	}
	else
	{
		// if not, erase the whole chunk in one shot
		if (ioctl (dev_fd,MEMERASE,&erase) < 0)
		{
			printf("While erasing blocks from 0x%.8x-0x%.8x\n",
						   (unsigned int) erase.start,(unsigned int) (erase.start + erase.length));
			return -1;
		}
	}
	
	// write the entire file to flash
	if (flags & FLAG_VERBOSE) 
	{
		printf("Writing data: 0k/%dk (0%%)\n", KB (nDataSize));
	}
	
	size = nDataSize;
	i = BUFSIZE;
	written = 0;
	pSrc = pDataBuffer;

	while (size)
	{
		if (size < BUFSIZE) 
		{
			i = size;
		}
		
		// read from filename
		pSrc = pDataBuffer+written;

		// write to device
		result = write (dev_fd, pSrc, i);
		if (i != result)
		{
			if (flags & FLAG_VERBOSE) 
			{
				printf ("\n");
			}
				
			if (result < 0)
			{
				printf("While writing data to 0x%.8x-0x%.8x\n", written,written + i);
				return -1;
			}
			printf("Short write count returned while writing to x%.8x-0x%.8x : %d/%d bytes written to flash\n",
					  written, written + i, written + result, nDataSize);
			return -1;
		}

		if (flags & FLAG_VERBOSE)
		{
		  printf ("Writing data: %dk/%dk (%d%%)\n",
				  KB (written + i),
				  KB (nDataSize),
				  PERCENTAGE (written + i, nDataSize));
		}
		
		// Add the code by lvjh, 2009-02-27
		value = PERCENTAGE (written + i, nDataSize)/4+50;	// 3
		if (value < 51)
		{
			value = 51;
		}
		else if (value >= 76)
		{
			value = 76;
		}
		// 执行回调函数
		if (g_send_update_progress_fun)
		{
			g_send_update_progress_fun(value);
		}
		
		written += i;
		size -= i;
	}
	 
	if (flags & FLAG_VERBOSE)
	{
		printf("Writing data: %dk/%dk (100%%)\n",
				 KB (nDataSize),
				 KB (nDataSize));
	}
	
	printf("Wrote %d / %dk bytes\n", written, nDataSize);
	
	// verify that flash == file data
	pSrc = pDataBuffer;
	safe_rewind (dev_fd);
	
	size = nDataSize;
	i = BUFSIZE;
	written = 0;
	
	if (flags & FLAG_VERBOSE)
	{
		printf ("Verifying data: 0k/%dk (0%%)\n",KB (nDataSize));
	}
	
	while (size)
	{
		if (size < BUFSIZE) 
		{
			i = size;
		}
		
		if (flags & FLAG_VERBOSE)
		{
			printf ("Verifying data: %dk/%dk (%d%%)\n",
					  KB (written + i),
					  KB (nDataSize),
					  PERCENTAGE (written + i,nDataSize));
		}

		// Add the code by lvjh, 2009-02-27
		value = PERCENTAGE (written + i,nDataSize)/4+76;
		if (value < 76)
		{
			value = 76;
		}
		else if (value >= 99)
		{
			value = 99;
		}
		// 执行回调函数
		if (g_send_update_progress_fun)
		{
			g_send_update_progress_fun(value);
		}
		
		// read from filename
		pSrc = pDataBuffer+written;

		// read from device
		safe_read(dev_fd, dest, i, flags & FLAG_VERBOSE);

		// compare buffers
		if (memcmp (pSrc, dest, i))
		{
			printf ("File does not seem to match flash data. First mismatch at 0x%.8x-0x%.8x\n",
					  written,written + i);
			return -1;
		}

		written += i;
		size -= i;
	 }

	if (flags & FLAG_VERBOSE)
	{
		printf("Verifying data: %dk/%dk (100%%)\n",
				 KB (nDataSize),
				 KB (nDataSize));
	}
	
	printf("Verified %d / %dk bytes\n",written,nDataSize);  
	
	return 0;
}

int all_region_write_ext(int dev_fd, int fil_fd)
{
	int i = 0;
	int flags = 0;
	ssize_t result;
	size_t size,written;
	struct mtd_info_user mtd;
	struct erase_info_user erase;
	struct stat filestat;
	unsigned char src[BUFSIZE],dest[BUFSIZE];
   
	if (fstat(fil_fd,&filestat) < 0)
	{
		return -1;
	}
	
	// does it fit into the device/partition
	if (filestat.st_size > mtd.size)
	{
		return -1;
	}
	
	// erase enough blocks so that we can write the file
	erase.start = 0;
	erase.length = filestat.st_size & ~(mtd.erasesize - 1);
	if (filestat.st_size % mtd.erasesize) 
		erase.length += mtd.erasesize;
		
	if (flags & FLAG_VERBOSE)
	{
		// if the user wants verbose output, erase 1 block at a time and show him/her what's going on 
		int blocks = erase.length / mtd.erasesize;
		erase.length = mtd.erasesize;
		
		printf("Erasing blocks: 0/%d (0%%)",blocks);
		
		for (i = 1; i <= blocks; i++)
		{
			printf ("\rErasing blocks: %d/%d (%d%%)",i,blocks,PERCENTAGE (i,blocks));
			 
			if (ioctl (dev_fd,MEMERASE,&erase) < 0)
			{
				  printf("While erasing blocks 0x%.8x-0x%.8x : %m\n",
						   (unsigned int) erase.start,(unsigned int) (erase.start + erase.length));
				  return -1;
			}
			erase.start += mtd.erasesize;
		}
		
		printf("\rErasing blocks: %d/%d (100%%)\n",blocks,blocks);
	}
	else
	{
		// if not, erase the whole chunk in one shot
		if (ioctl (dev_fd,MEMERASE,&erase) < 0)
		{
			printf("While erasing blocks from 0x%.8x-0x%.8x : %m\n",
						   (unsigned int) erase.start,(unsigned int) (erase.start + erase.length));
			return -1;
		}
	}
	
	// write the entire file to flash
	if (flags & FLAG_VERBOSE) 
		printf("Writing data: 0k/%luk (0%%)",KB (filestat.st_size));
		
	size = filestat.st_size;
	i = BUFSIZE;
	written = 0;

	while (size)
	{
		if (size < BUFSIZE) 
			i = size;
		if (flags & FLAG_VERBOSE)
		  printf ("\rWriting data: %dk/%luk (%lu%%)",
				  KB (written + i),
				  KB (filestat.st_size),
				  PERCENTAGE (written + i,filestat.st_size));

		// read from filename
		safe_read (fil_fd, src, i, flags & FLAG_VERBOSE);

		// write to device
		result = write (dev_fd, src, i);
		if (i != result)
		{
			if (flags & FLAG_VERBOSE) 
				printf ("\n");
				
			if (result < 0)
			{
				printf("While writing data to 0x%.8x-0x%.8x : %m\n",
						   written,written + i);
				return -1;
			}
			printf("Short write count returned while writing to x%.8x-0x%.8x: %d/%lu bytes written to flash\n",
					  written,written + i,written + result,filestat.st_size);
			return -1;
		}

		written += i;
		size -= i;
	}
	 
	if (flags & FLAG_VERBOSE)
		printf("\rWriting data: %luk/%luk (100%%)\n",
				 KB (filestat.st_size),
				 KB (filestat.st_size));
	printf("Wrote %d / %luk bytes\n",written,filestat.st_size);
	
	// verify that flash == file data
	safe_rewind (fil_fd);
	safe_rewind (dev_fd);
	size = filestat.st_size;
	i = BUFSIZE;
	written = 0;
	if (flags & FLAG_VERBOSE) 
		printf ("Verifying data: 0k/%luk (0%%)",KB (filestat.st_size));
	
	while (size)
	{
		if (size < BUFSIZE) 
			i = size;
		if (flags & FLAG_VERBOSE)
			printf ("\rVerifying data: %dk/%luk (%lu%%)",
					  KB (written + i),
					  KB (filestat.st_size),
					  PERCENTAGE (written + i,filestat.st_size));

		// read from filename
		safe_read (fil_fd, src, i, flags & FLAG_VERBOSE);

		// read from device
		safe_read (dev_fd, dest, i, flags & FLAG_VERBOSE);

		// compare buffers
		if (memcmp (src,dest,i))
		{
			printf ("File does not seem to match flash data. First mismatch at 0x%.8x-0x%.8x\n",
					  written,written + i);
			return -1;
		}

		written += i;
		size -= i;
	 }

	if (flags & FLAG_VERBOSE)
	 printf("\rVerifying data: %luk/%luk (100%%)\n",
				 KB (filestat.st_size),
				 KB (filestat.st_size));
	printf("Verified %d / %luk bytes\n",written,filestat.st_size);   
   
	return 0;
}

// API
int MTD_EraseRegion(char *pDevice, int nStart, int nCount)
{
	return 0;
}

int MTD_EraseAllRegion(char *pDevice)
{
	return 0;
}

int MTD_WriteRegion(char *pDevice)
{
	return 0;
}

int MTD_WriteAllRegion(char *pDevice, char *pDataBuffer, int nDataSize)
{
	int nRet = -1;
	int fd = -1;
	
	if (pDevice == NULL)
	{
		return -1;
	}
	if (strlen(pDevice) <= 0)
	{
		return -1;
	}
	
	// Add the code by lvjh, 2009-10-10
	nRet = flash_eraseall(pDevice);
	if (nRet < 0)
	{
		printf("flash_eraseall: Failed!\n");
		return -1;
	}
	
	if ((fd = safe_open(pDevice, O_SYNC|O_RDWR)) < 0) 
	{
		printf("Can not open the device: %s, Error: %s\n", pDevice, strerror(errno));
		return -1;
	}
	
	nRet = all_region_write(fd, pDataBuffer, nDataSize);
	
	close(fd);
	
	return nRet;
}

int MTD_WriteAllRegionExt(char *pDevice, char *pFileName)
{
	return 0;
}

int MTD_GetWriteProcess()
{
	return g_mtd_process;
}

int MTD_SetCallBack(SendUpdateProgress pFunc)
{
	if (pFunc == NULL)
	{
		return -1;
	}
	
	g_send_update_progress_fun = pFunc;
	
	return 0;
}

#ifdef TEST
int main()
{
	int nRet = 0;
	int fil_fd = -1;
	struct stat filestat;
	char *pDataBuffer = NULL;
	
	fil_fd = safe_open ("pt_ccd.img",O_RDONLY);
	if (fil_fd <= 0)
	{
		return -1;
	}
	if (fstat (fil_fd,&filestat) < 0)
	{
		return -1;
	}
	if (filestat.st_size <= 0)
	{
		return -1;
	}
	pDataBuffer = (char *)malloc(filestat.st_size);
	if (pDataBuffer == NULL)
	{
		close(fil_fd);
		return -1;
	}
	nRet = read(fil_fd, pDataBuffer, filestat.st_size);
	if (nRet != filestat.st_size)
	{
		close(fil_fd);
		free(pDataBuffer);
		
		return -1;
	}
	close(fil_fd);
	
	MTD_WriteAllRegion("/dev/mtd/2", pDataBuffer, filestat.st_size);
	
	free(pDataBuffer);
	
	return 0;
}
#endif

