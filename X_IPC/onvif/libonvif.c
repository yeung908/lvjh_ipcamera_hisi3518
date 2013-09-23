#include "libonvif.h"
#include "onvifService.h"
#include "onvifDiscovery.h"
#include <stdio.h>
#include <stdlib.h>

int ONVIFStart()
{
	int ret = 0;
	ret = ONVIF_DiscoveryStart();
	if(ret < 0)
	{
		printf("ONVIF_DiscoveryStart() error\n");
		return ret;
	}
	ret = ONVIF_ServiceStart();
	if(ret < 0)
	{
		printf("ONVIF_ServiceStart() error\n");
		return ret;
	}
	return 0;
}

void ONVIFStop()
{
	ONVIF_DiscoveryStop();
	ONVIF_ServiceStop();
}
