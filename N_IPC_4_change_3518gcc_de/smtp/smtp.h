#ifndef __SMTP_H_
#define __SMTP_H_

#define FROM		1
#define TO 			2
#define CC 			3
#define BCC 		4
#define REPLYTO 	5

int SMTP_Send(char *emailServer, char *username, char *password, 
			  char *from, char *to, char *cc, char *bcc, char *replyto, char *subject, char *body, char *attach_filename);
int SMTP_Send_Ext(char *emailServer, char *username, char *password, 
				  char *from, char *to, char *cc, char *bcc, char *replyto, 
				  char *subject, char *body, char *filename, char *attach_buffer, int size);

#endif
