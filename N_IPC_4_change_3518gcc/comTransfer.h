#ifndef __COM_TRANSFER_H_
#define __COM_TRANSFER_H_

int RS232_Open();
int RS232_Clsoe();
int RS232_Setup(unsigned long nBaudRate, unsigned long nDataBit, unsigned long nParity, unsigned long nStopBit);
int RS232_Send(char *buffer, int size);
int RS232_Receive(char *buffer, int *size);

#endif
