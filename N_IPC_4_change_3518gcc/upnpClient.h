#ifndef __UPNP_CLIENT_H_
#define __UPNP_CLIENT_H_

int UPNP_AddMapPort(char *pRemoteHost, int nExternalPort, char *pMapProtocal, int nInternalPort, char *pInternalClient, char *pDescription);
int UPNP_DelMapPort(char *pRemoteHost, int nExternalPort, char *pMapProtocal);

int UPNP_Init();
int SNAPSHOT_Destroy();
int UPNP_Start();
int UPNP_Stop();
int UPNP_Pause();
int UPNP_Resume();
int UPNP_GetStatus();

#endif


