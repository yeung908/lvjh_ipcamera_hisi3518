#ifndef _PPPOE_H_
#define _PPPOE_H_

int PPPOE_Open();
int PPPOE_Close();
int PPPOE_Setup(char *user, char *password, char *dns1, char *dns2);
int PPPOE_GetSetup(char *user, char *password, char *dns1, char *dns2);
int PPPOE_Start();
int PPPOE_Stop();

#endif
