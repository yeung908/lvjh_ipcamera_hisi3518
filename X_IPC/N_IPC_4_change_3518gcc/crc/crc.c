#include <stdlib.h>
#include <stdio.h>

unsigned long GetDataCRC(char *lpData, int nSize)
{
	int i = 0;
	char *buffer = NULL;
	int len = 0;
	unsigned int nCRC = 0;
	unsigned int crc32Table[256];
	unsigned int nPolynomial = 0x04C11DB7;
	
	for (i=0; i<=0xFF; i++)
	{
		unsigned int value = 0;
		int ref = i;
		int n = 0;
		int j = 0;
		
		for (n=1; n<(8+1); n++)
		{
			if (ref & 1)
			{
				value |= 1 << (8 - n);
			}
			ref >>= 1;
		}
		
		crc32Table[i]= value << 24;
		for (j = 0; j < 8; j++)
		{
			crc32Table[i] = (crc32Table[i] << 1) ^ (crc32Table[i] & (1 << 31) ? nPolynomial : 0);
		}
		
		value = 0;
		ref = crc32Table[i];
		for (n=1; n<(32+1); n++)
		{
			if (ref & 1)
			{
				value |= 1 << (32 - n);
			}
			ref >>= 1;
		}
		crc32Table[i] = value;
	}
	
	// calculate the crc check sum
	nCRC = -1;
	buffer = (char *)lpData;
	len = nSize;
	
	while (len--)
	{
		nCRC = (nCRC >> 8) ^ crc32Table[(nCRC & 0xFF) ^ *buffer++];
	}
	
	return nCRC^ 0xffffffff;
}
