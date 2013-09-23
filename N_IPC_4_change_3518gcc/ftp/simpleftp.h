#ifndef _SIMPLEFTP_H_
#define _SIMPLEFTP_H_

int FTP_GetFile(char *tftpserver, int port, char *username, char *password, char *localfile, char *remotefile);
int FTP_PutFile(char *tftpserver, int port, char *username, char *password, char *localfile, char *remotefile);
int FTP_PutFile_Ext(char *tftpserver, int port, char *username, char *password, char *localfile, char *localfiledata, unsigned long datasize, char *remotefile);

#endif
