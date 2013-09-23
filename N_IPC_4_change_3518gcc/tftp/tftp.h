#ifndef _TFTP_H_
#define _TFTP_H_

int TFTP_GetFile(char *tftpserver, int port, char *remotefile);
int TFTP_PutFile(char *tftpserver, int port, char *remotefile);
int TFTP_PutFile_Ext(char *tftpserver, int port, char *remotefile, char *filedata, int *datasize);

#endif
