#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#ifndef _WIN32
#include <arpa/inet.h>
#include <netinet/tcp.h>
#else
#include <winsock2.h>
#endif
#include "onvif.h"
#include "onvifService.h"
#include "deviceinformation.nsmap"
/*#include "networkinterfaces.nsmap"
#include "getdns.nsmap"
#include "getscopes.nsmap"
#include "getcapabilities.nsmap"
#include "getservices.nsmap"
#include "wsdlgetvideosources.nsmap"
#include "getprofiles.nsmap"
#include "getsnapshoturi.nsmap"
#include "wsdlgetprofile.nsmap"
#include "getstreamuri.nsmap"
#include "getvideosourceconfiguration.nsmap"
#include "getterminationtime.nsmap"*/
#include "../param.h"
#include "../ptz.h"
#include "../vadcDrv.h"


const char *http_ok = "HTTP/1.1 200 OK";
const char *http_server= "gSOAP/2.8";
const char *http_connection = "close";
const char *http_fail405="HTTP/1.1 405";
const char *http_notallowed="Method Not Allowed";
extern Onvif_Server g_OnvifServer;
extern unsigned short g_wOnvifPort;
unsigned int g_OnvifServiceRunning = 1;
extern SYS_PARAM g_sys_param;

int BuildHttpHeaderString(const struct Http_Buffer *pBuf,char *sBuffer,int nLen)
{
	if(pBuf->nHasAction)
		sprintf(sBuffer,"%s\r\nServer: %s\r\nContent-Type: application/soap+xml; charset=utf-8; action=\"%s\"\r\nContent-Length: %d\r\nConnection: %s\r\n\r\n",http_ok,http_server,pBuf->action,nLen,http_connection);
	else
		sprintf(sBuffer,"%s\r\nServer: %s\r\nContent-Type: application/soap+xml; charset=utf-8;\r\nContent-Length: %d\r\nConnection: %s\r\n\r\n",http_ok,http_server,nLen,http_connection);
	return strlen(sBuffer);
}

int BuildHttpFailHeaderString(const struct Http_Buffer *pBuf,char *sBuffer,int nLen)
{
	if(pBuf->nHasAction)
		sprintf(sBuffer,"%s %s\r\nServer: %s\r\nContent-Type: application/soap+xml; charset=utf-8; action=\"%s\"\r\nContent-Length: %d\r\nConnection: %s\r\n\r\n",http_fail405,http_notallowed,http_server,pBuf->action,nLen,http_connection);
	else
		sprintf(sBuffer,"%s %s\r\nServer: %s\r\nContent-Type: application/soap+xml; charset=utf-8;\r\nContent-Length: %d\r\nConnection: %s\r\n\r\n",http_fail405,http_notallowed,http_server,nLen,http_connection);
	return strlen(sBuffer);
}

int BuildDevInfoHeaderString(char *sBuffer,struct Namespace *pNamespaces)
{
	return BuildCommonHeaderString(sBuffer,pNamespaces);
}

int BuildDeviceInfoString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<tds:GetDeviceInformationResponse>");
	strcat(sBuffer,"<tds:Manufacturer>ONVIF</tds:Manufacturer>");
	strcat(sBuffer,"<tds:Model>IPC</tds:Model>");
	sprintf(TmpBuffer,"<tds:FirmwareVersion>V%02d.%02d.%02d.%02d</tds:FirmwareVersion>",
	        HIWORD(g_sys_param.sysInfo.nHardwareVersion),
	        LOWORD(g_sys_param.sysInfo.nHardwareVersion),
	        HIWORD(g_sys_param.sysInfo.nSoftwareVersion),
	        LOWORD(g_sys_param.sysInfo.nSoftwareVersion));
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tds:SerialNumber>%s</tds:SerialNumber>",g_sys_param.sysInfo.strDeviceID);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tds:HardwareId>V%02d.%02d.%02d.%02d</tds:HardwareId>",
	        HIWORD(g_sys_param.sysInfo.nHardwareVersion),
	        LOWORD(g_sys_param.sysInfo.nHardwareVersion),
	        HIWORD(g_sys_param.sysInfo.nSoftwareVersion),
	        LOWORD(g_sys_param.sysInfo.nSoftwareVersion));
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tds:GetDeviceInformationResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}

int BuildNetworkInterfaceString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	get_ip_addr(ETH_WIRE_DEV,localIP);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<tds:GetNetworkInterfacesResponse>");
	strcat(sBuffer,"<tds:NetworkInterfaces token=\"eth0\">");
	strcat(sBuffer,"<tt:Enabled>true</tt:Enabled>");
	strcat(sBuffer,"<tt:Info>");
	strcat(sBuffer,"<tt:Name>NetworkInterfaces</tt:Name>");
	sprintf(TmpBuffer,"<tt:HwAddress>%02X-%02X-%02X-%02X-%02X-%02X</tt:HwAddress>",
	        g_sys_param.network.strPhyAddr[0],g_sys_param.network.strPhyAddr[1],
	        g_sys_param.network.strPhyAddr[2],g_sys_param.network.strPhyAddr[3],
	        g_sys_param.network.strPhyAddr[4],g_sys_param.network.strPhyAddr[5]);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:MTU>1500</tt:MTU>");
	strcat(sBuffer,"</tt:Info>");
	strcat(sBuffer,"<tt:Link>");
	strcat(sBuffer,"<tt:AdminSettings>");
	strcat(sBuffer,"<tt:AutoNegotiation>true</tt:AutoNegotiation>");
	strcat(sBuffer,"<tt:Speed>100</tt:Speed>");
	strcat(sBuffer,"<tt:Duplex>Full</tt:Duplex>");
	strcat(sBuffer,"</tt:AdminSettings>");
	strcat(sBuffer,"<tt:OperSettings>");
	strcat(sBuffer,"<tt:AutoNegotiation>true</tt:AutoNegotiation>");
	strcat(sBuffer,"<tt:Speed>100</tt:Speed>");
	strcat(sBuffer,"<tt:Duplex>Half</tt:Duplex>");
	strcat(sBuffer,"</tt:OperSettings>");
	strcat(sBuffer,"<tt:InterfaceType>0</tt:InterfaceType>");
	strcat(sBuffer,"</tt:Link>");
	strcat(sBuffer,"<tt:IPv4>");
	strcat(sBuffer,"<tt:Enabled>true</tt:Enabled>");
	strcat(sBuffer,"<tt:Config>");
	strcat(sBuffer,"<tt:Manual>");
	sprintf(TmpBuffer,"<tt:Address>%s</tt:Address>",localIP);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:PrefixLength>23</tt:PrefixLength>");
	strcat(sBuffer,"</tt:Manual>");
	if(g_sys_param.network.nDhcpOnFlag)
		strcat(sBuffer,"<tt:DHCP>true</tt:DHCP>");
	else
		strcat(sBuffer,"<tt:DHCP>false</tt:DHCP>");
	strcat(sBuffer,"</tt:Config>");
	strcat(sBuffer,"</tt:IPv4>");
	strcat(sBuffer,"</tds:NetworkInterfaces>");
	strcat(sBuffer,"</tds:GetNetworkInterfacesResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);

}

int BuildGetDNSString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");

	strcat(sBuffer,"<tds:GetDNSResponse>");
	strcat(sBuffer,"<tds:DNSInformation>");
	if(g_sys_param.network.nDhcpOnFlag)
		strcat(sBuffer,"<tt:FromDHCP>true</tt:FromDHCP>");
	else
		strcat(sBuffer,"<tt:FromDHCP>false</tt:FromDHCP>");
	strcat(sBuffer,"<tt:DNSManual>");
	strcat(sBuffer,"<tt:Type>IPv4</tt:Type>");
	sprintf(TmpBuffer,"<tt:IPv4Address>%s</tt:IPv4Address>",g_sys_param.network.byDnsAddr);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tt:DNSManual>");
	strcat(sBuffer,"</tds:DNSInformation>");
	strcat(sBuffer,"</tds:GetDNSResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);

}

int BuildGetScopesString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");

	strcat(sBuffer,"<tds:GetScopesResponse>");
	strcat(sBuffer,"<tds:Scopes>");
	strcat(sBuffer,"<tt:ScopeDef>Fixed</tt:ScopeDef>");
	strcat(sBuffer,"<tt:ScopeItem>onvif://www.onvif.org/type/video_encoder</tt:ScopeItem>");
	strcat(sBuffer,"</tds:Scopes>");
	strcat(sBuffer,"<tds:Scopes>");
	strcat(sBuffer,"<tt:ScopeDef>Fixed</tt:ScopeDef>");
	strcat(sBuffer,"<tt:ScopeItem>onvif://www.onvif.org/type/ptz</tt:ScopeItem>");
	strcat(sBuffer,"</tds:Scopes>");
	strcat(sBuffer,"<tds:Scopes>");
	strcat(sBuffer,"<tt:ScopeDef>Fixed</tt:ScopeDef>");
	strcat(sBuffer,"<tt:ScopeItem>onvif://www.onvif.org/type/audio_encoder</tt:ScopeItem>");
	strcat(sBuffer,"</tds:Scopes>");
	strcat(sBuffer,"<tds:Scopes>");
	strcat(sBuffer,"<tt:ScopeDef>Fixed</tt:ScopeDef>");
	strcat(sBuffer,"<tt:ScopeItem>onvif://www.onvif.org/location/city/china</tt:ScopeItem>");
	strcat(sBuffer,"</tds:Scopes>");
	strcat(sBuffer,"<tds:Scopes>");
	strcat(sBuffer,"<tt:ScopeDef>Fixed</tt:ScopeDef>");
	strcat(sBuffer,"<tt:ScopeItem>onvif://www.onvif.org/hardware/5201H</tt:ScopeItem>");
	strcat(sBuffer,"</tds:Scopes>");
	strcat(sBuffer,"<tds:Scopes>");
	strcat(sBuffer,"<tt:ScopeDef>Fixed</tt:ScopeDef>");
	sprintf(TmpBuffer,"<tt:ScopeItem>onvif://www.onvif.org/name/%s</tt:ScopeItem>","IP_CAMERA");
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tds:Scopes>");
	strcat(sBuffer,"</tds:GetScopesResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);

}

int BuildGetCapabilitiesString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	get_ip_addr(ETH_WIRE_DEV,localIP);
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");

	strcat(sBuffer,"<tds:GetCapabilitiesResponse>");
	strcat(sBuffer,"<tds:Capabilities>");
	strcat(sBuffer,"<tt:Device>");
	if(g_wOnvifPort == 80)
		sprintf(TmpBuffer,"<tt:XAddr>http://%s/onvif/Device_service</tt:XAddr>",localIP);
	else
		sprintf(TmpBuffer,"<tt:XAddr>http://%s:%d/onvif/Device_service</tt:XAddr>",localIP,g_wOnvifPort);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:Network>");
	strcat(sBuffer,"<tt:IPFilter>false</tt:IPFilter>");
	strcat(sBuffer,"<tt:ZeroConfiguration>false</tt:ZeroConfiguration>");
	strcat(sBuffer,"<tt:IPVersion6>false</tt:IPVersion6>");
	strcat(sBuffer,"<tt:DynDNS>true</tt:DynDNS>");
	strcat(sBuffer,"</tt:Network>");
	strcat(sBuffer,"<tt:System>");
	strcat(sBuffer,"<tt:DiscoveryResolve>true</tt:DiscoveryResolve>");
	strcat(sBuffer,"<tt:DiscoveryBye>true</tt:DiscoveryBye>");
	strcat(sBuffer,"<tt:RemoteDiscovery>false</tt:RemoteDiscovery>");
	strcat(sBuffer,"<tt:SystemBackup>false</tt:SystemBackup>");
	strcat(sBuffer,"<tt:SystemLogging>false</tt:SystemLogging>");
	strcat(sBuffer,"<tt:FirmwareUpgrade>true</tt:FirmwareUpgrade>");
	strcat(sBuffer,"<tt:SupportedVersions><tt:Major>1</tt:Major><tt:Minor>0</tt:Minor></tt:SupportedVersions>");
	strcat(sBuffer,"<tt:SupportedVersions><tt:Major>2</tt:Major><tt:Minor>0</tt:Minor></tt:SupportedVersions>");
	strcat(sBuffer,"</tt:System>");
	strcat(sBuffer,"<tt:IO>");
	strcat(sBuffer,"<tt:InputConnectors>1</tt:InputConnectors>");
	strcat(sBuffer,"<tt:RelayOutputs>1</tt:RelayOutputs>");
	strcat(sBuffer,"</tt:IO>");
	strcat(sBuffer,"<tt:Security>");
	strcat(sBuffer,"<tt:TLS1.1>false</tt:TLS1.1><tt:TLS1.2>false</tt:TLS1.2>");
	strcat(sBuffer,"<tt:OnboardKeyGeneration>false</tt:OnboardKeyGeneration>");
	strcat(sBuffer,"<tt:AccessPolicyConfig>false</tt:AccessPolicyConfig>");
	strcat(sBuffer,"<tt:X.509Token>false</tt:X.509Token>");
	strcat(sBuffer,"<tt:SAMLToken>false</tt:SAMLToken>");
	strcat(sBuffer,"<tt:KerberosToken>false</tt:KerberosToken>");
	strcat(sBuffer,"<tt:RELToken>false</tt:RELToken>");
	strcat(sBuffer,"</tt:Security>");
	strcat(sBuffer,"</tt:Device>");
	strcat(sBuffer,"<tt:Events>");
	if(g_wOnvifPort == 80)
		sprintf(TmpBuffer,"<tt:XAddr>http://%s/onvif/Events_service</tt:XAddr>",localIP);
	else
		sprintf(TmpBuffer,"<tt:XAddr>http://%s:%d/onvif/Events_service</tt:XAddr>",localIP,g_wOnvifPort);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:WSSubscriptionPolicySupport>true</tt:WSSubscriptionPolicySupport>");
	strcat(sBuffer,"<tt:WSPullPointSupport>true</tt:WSPullPointSupport>");
	strcat(sBuffer,"<tt:WSPausableSubscriptionManagerInterfaceSupport>true</tt:WSPausableSubscriptionManagerInterfaceSupport>");
	strcat(sBuffer,"</tt:Events>");
	if(g_wOnvifPort == 80)
		sprintf(TmpBuffer,"<tt:Imaging><tt:XAddr>http://%s/onvif/Imaging_service</tt:XAddr></tt:Imaging>",localIP);
	else
		sprintf(TmpBuffer,"<tt:Imaging><tt:XAddr>http://%s:%d/onvif/Imaging_service</tt:XAddr></tt:Imaging>",localIP,g_wOnvifPort);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:Media>");
	if(g_wOnvifPort == 80)
		sprintf(TmpBuffer,"<tt:XAddr>http://%s/onvif/Media_service</tt:XAddr>",localIP);
	else
		sprintf(TmpBuffer,"<tt:XAddr>http://%s:%d/onvif/Media_service</tt:XAddr>",localIP,g_wOnvifPort);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:StreamingCapabilities>");
	strcat(sBuffer,"<tt:RTPMulticast>false</tt:RTPMulticast>");
	strcat(sBuffer,"<tt:RTP_TCP>true</tt:RTP_TCP>");
	strcat(sBuffer,"<tt:RTP_RTSP_TCP>true</tt:RTP_RTSP_TCP>");
	strcat(sBuffer,"</tt:StreamingCapabilities>");
	strcat(sBuffer,"<tt:Extension>");
	strcat(sBuffer,"<tt:ProfileCapabilities><tt:MaximumNumberOfProfiles>1</tt:MaximumNumberOfProfiles></tt:ProfileCapabilities>");
	strcat(sBuffer,"</tt:Extension>");
	strcat(sBuffer,"</tt:Media>");
	if(g_wOnvifPort == 80)
		sprintf(TmpBuffer,"<tt:PTZ><tt:XAddr>http://%s/onvif/PTZ_service</tt:XAddr></tt:PTZ>",localIP);
	else
		sprintf(TmpBuffer,"<tt:PTZ><tt:XAddr>http://%s:%d/onvif/PTZ_service</tt:XAddr></tt:PTZ>",localIP,g_wOnvifPort);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tds:Capabilities>");
	strcat(sBuffer,"</tds:GetCapabilitiesResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);

}

int BuildWsdlGetVideoSourcesString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");

	strcat(sBuffer,"<trt:GetVideoSourcesResponse>");
	strcat(sBuffer,"<trt:VideoSources token=\"0_VSC\">");
	sprintf(TmpBuffer,"<tt:Framerate>%d</tt:Framerate>",(int)g_sys_param.videoEnc[0][0].nFramerate);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tt:Resolution><tt:Width>%d</tt:Width><tt:Height>%d</tt:Height></tt:Resolution>",
	        (int)g_sys_param.videoEnc[0][0].nEncodeWidth,
	        (int)g_sys_param.videoEnc[0][0].nEncodeHeight);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</trt:VideoSources>");
	strcat(sBuffer,"</trt:GetVideoSourcesResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);

}

int BuildGetProfilesString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<trt:GetProfilesResponse>");
	strcat(sBuffer,"<trt:Profiles fixed=\"true\" token=\"0_main\">");
	strcat(sBuffer,"<tt:Name>0_main</tt:Name>");
	strcat(sBuffer,"<tt:VideoSourceConfiguration token=\"0_VSC\">");
	strcat(sBuffer,"<tt:Name>0_VSC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>0</tt:UseCount>");
	strcat(sBuffer,"<tt:SourceToken>0_VSC</tt:SourceToken>");
	sprintf(TmpBuffer,"<tt:Bounds height=\"%d\" width=\"%d\" y=\"0\" x=\"0\"></tt:Bounds>",
	        (int)g_sys_param.videoEnc[0][0].nEncodeHeight,
	        (int)g_sys_param.videoEnc[0][0].nEncodeWidth);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tt:VideoSourceConfiguration>");
	strcat(sBuffer,"<tt:AudioSourceConfiguration token=\"0_ASC\">");
	strcat(sBuffer,"<tt:Name>0_ASC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>0</tt:UseCount>");
	strcat(sBuffer,"<tt:SourceToken>0_ASC</tt:SourceToken>");
	strcat(sBuffer,"</tt:AudioSourceConfiguration>");
	strcat(sBuffer,"<tt:VideoEncoderConfiguration token=\"0_VEC_main\">");
	strcat(sBuffer,"<tt:Name>0_VEC_main</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>0</tt:UseCount>");
	strcat(sBuffer,"<tt:Encoding>H264</tt:Encoding>");
	sprintf(TmpBuffer,"<tt:Resolution><tt:Width>%d</tt:Width><tt:Height>%d</tt:Height></tt:Resolution>",
	        (int)g_sys_param.videoEnc[0][0].nEncodeWidth,
	        (int)g_sys_param.videoEnc[0][0].nEncodeHeight);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:Quality>31</tt:Quality>");
	strcat(sBuffer,"<tt:RateControl>");
	sprintf(TmpBuffer,"<tt:FrameRateLimit>%d</tt:FrameRateLimit>",
	        (int)g_sys_param.videoEnc[0][0].nFramerate);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tt:EncodingInterval>%d</tt:EncodingInterval>",
	        (int)g_sys_param.videoEnc[0][0].nKeyInterval);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:BitrateLimit>4000</tt:BitrateLimit>");
	strcat(sBuffer,"</tt:RateControl>");
	strcat(sBuffer,"<tt:H264>");
	strcat(sBuffer,"<tt:GovLength>25</tt:GovLength>");
	strcat(sBuffer,"<tt:H264Profile>Baseline</tt:H264Profile>");
	strcat(sBuffer,"</tt:H264>");
	strcat(sBuffer,"<tt:Multicast>");
	strcat(sBuffer,"<tt:Address><tt:Type>IPv4</tt:Type></tt:Address>");
	strcat(sBuffer,"<tt:Port>0</tt:Port>");
	strcat(sBuffer,"<tt:TTL>0</tt:TTL>");
	strcat(sBuffer,"<tt:AutoStart>false</tt:AutoStart>");
	strcat(sBuffer,"</tt:Multicast>");
	strcat(sBuffer,"<tt:SessionTimeout>PT1H37M41.620S</tt:SessionTimeout>");
	strcat(sBuffer,"</tt:VideoEncoderConfiguration>");
	strcat(sBuffer,"<tt:AudioEncoderConfiguration token=\"0_AEC\">");
	strcat(sBuffer,"<tt:Name>0_AEC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>0</tt:UseCount>");
	strcat(sBuffer,"<tt:Encoding>G711</tt:Encoding>");
	strcat(sBuffer,"<tt:Bitrate>64</tt:Bitrate>");
	strcat(sBuffer,"<tt:SampleRate>8000</tt:SampleRate>");
	strcat(sBuffer,"<tt:Multicast>");
	strcat(sBuffer,"<tt:Address><tt:Type>IPv4</tt:Type></tt:Address>");
	strcat(sBuffer,"<tt:Port>0</tt:Port>");
	strcat(sBuffer,"<tt:TTL>0</tt:TTL>");
	strcat(sBuffer,"<tt:AutoStart>false</tt:AutoStart>");
	strcat(sBuffer,"</tt:Multicast>");
	strcat(sBuffer,"<tt:SessionTimeout>PT1H37M41.620S</tt:SessionTimeout>");
	strcat(sBuffer,"</tt:AudioEncoderConfiguration>");
	strcat(sBuffer,"<tt:PTZConfiguration token=\"0_PTZ\">");
	strcat(sBuffer,"<tt:Name>0_PTZ</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:NodeToken>0_PTZ</tt:NodeToken>");
	strcat(sBuffer,"<tt:DefaultAbsolutePantTiltPositionSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:DefaultAbsolutePantTiltPositionSpace>");
	strcat(sBuffer,"<tt:DefaultAbsoluteZoomPositionSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:DefaultAbsoluteZoomPositionSpace>");
	strcat(sBuffer,"<tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace>");
	strcat(sBuffer,"<tt:DefaultRelativeZoomTranslationSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:DefaultRelativeZoomTranslationSpace>");
	strcat(sBuffer,"<tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace>");
	strcat(sBuffer,"<tt:DefaultContinuousZoomVelocitySpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:DefaultContinuousZoomVelocitySpace>");
	strcat(sBuffer,"<tt:DefaultPTZSpeed>");
	strcat(sBuffer,"<tt:PanTilt space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace\" y=\"1\" x=\"0\"></tt:PanTilt>");
	strcat(sBuffer,"<tt:Zoom space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace\" x=\"1\"></tt:Zoom>");
	strcat(sBuffer,"</tt:DefaultPTZSpeed>");
	strcat(sBuffer,"<tt:DefaultPTZTimeout>P16DT3H41M8.564S</tt:DefaultPTZTimeout>");
	strcat(sBuffer,"<tt:PanTiltLimits>");
	strcat(sBuffer,"<tt:Range>");
	strcat(sBuffer,"<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:URI>");
	strcat(sBuffer,"<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>");
	strcat(sBuffer,"<tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange>");
	strcat(sBuffer,"</tt:Range>");
	strcat(sBuffer,"</tt:PanTiltLimits>");
	strcat(sBuffer,"<tt:ZoomLimits>");
	strcat(sBuffer,"<tt:Range>");
	strcat(sBuffer,"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:URI>");
	strcat(sBuffer,"<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>");
	strcat(sBuffer,"</tt:Range>");
	strcat(sBuffer,"</tt:ZoomLimits>");
	strcat(sBuffer,"</tt:PTZConfiguration>");
	strcat(sBuffer,"</trt:Profiles>");
	strcat(sBuffer,"<trt:Profiles fixed=\"true\" token=\"0_sub\">");
	strcat(sBuffer,"<tt:Name>0_sub</tt:Name>");
	strcat(sBuffer,"<tt:VideoSourceConfiguration token=\"0_VSC\">");
	strcat(sBuffer,"<tt:Name>0_VSC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>0</tt:UseCount>");
	strcat(sBuffer,"<tt:SourceToken>0_VSC</tt:SourceToken>");
	sprintf(TmpBuffer,"<tt:Bounds height=\"%d\" width=\"%d\" y=\"0\" x=\"0\"></tt:Bounds>",
	        (int)g_sys_param.videoEnc[0][1].nEncodeHeight,
	        (int)g_sys_param.videoEnc[0][1].nEncodeWidth);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tt:VideoSourceConfiguration>");
	strcat(sBuffer,"<tt:AudioSourceConfiguration token=\"0_ASC\">");
	strcat(sBuffer,"<tt:Name>0_ASC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>0</tt:UseCount>");
	strcat(sBuffer,"<tt:SourceToken>0_ASC</tt:SourceToken>");
	strcat(sBuffer,"</tt:AudioSourceConfiguration>");
	strcat(sBuffer,"<tt:VideoEncoderConfiguration token=\"0_VEC_sub\">");
	strcat(sBuffer,"<tt:Name>0_VEC_sub</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>0</tt:UseCount>");
	strcat(sBuffer,"<tt:Encoding>H264</tt:Encoding>");
	sprintf(TmpBuffer,"<tt:Resolution><tt:Width>%d</tt:Width><tt:Height>%d</tt:Height></tt:Resolution>",
	        (int)g_sys_param.videoEnc[0][1].nEncodeWidth,
	        (int)g_sys_param.videoEnc[0][1].nEncodeHeight);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:Quality>31</tt:Quality>");
	strcat(sBuffer,"<tt:RateControl>");
	sprintf(TmpBuffer,"<tt:FrameRateLimit>%d</tt:FrameRateLimit>",
	        (int)g_sys_param.videoEnc[0][1].nFramerate);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tt:EncodingInterval>%d</tt:EncodingInterval>",
	        (int)g_sys_param.videoEnc[0][1].nKeyInterval);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:BitrateLimit>1500</tt:BitrateLimit>");
	strcat(sBuffer,"</tt:RateControl>");
	strcat(sBuffer,"<tt:H264><tt:GovLength>25</tt:GovLength><tt:H264Profile>Baseline</tt:H264Profile></tt:H264>");
	strcat(sBuffer,"<tt:Multicast>");
	strcat(sBuffer,"<tt:Address><tt:Type>IPv4</tt:Type></tt:Address>");
	strcat(sBuffer,"<tt:Port>0</tt:Port>");
	strcat(sBuffer,"<tt:TTL>0</tt:TTL>");
	strcat(sBuffer,"<tt:AutoStart>false</tt:AutoStart>");
	strcat(sBuffer,"</tt:Multicast>");
	strcat(sBuffer,"<tt:SessionTimeout>PT1H37M41.620S</tt:SessionTimeout>");
	strcat(sBuffer,"</tt:VideoEncoderConfiguration>");
	strcat(sBuffer,"<tt:AudioEncoderConfiguration token=\"0_AEC\">");
	strcat(sBuffer,"<tt:Name>0_AEC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>0</tt:UseCount>");
	strcat(sBuffer,"<tt:Encoding>G711</tt:Encoding>");
	strcat(sBuffer,"<tt:Bitrate>64</tt:Bitrate>");
	strcat(sBuffer,"<tt:SampleRate>8000</tt:SampleRate>");
	strcat(sBuffer,"<tt:Multicast>");
	strcat(sBuffer,"<tt:Address><tt:Type>IPv4</tt:Type></tt:Address>");
	strcat(sBuffer,"<tt:Port>0</tt:Port>");
	strcat(sBuffer,"<tt:TTL>0</tt:TTL>");
	strcat(sBuffer,"<tt:AutoStart>false</tt:AutoStart>");
	strcat(sBuffer,"</tt:Multicast>");
	strcat(sBuffer,"<tt:SessionTimeout>PT1H37M41.620S</tt:SessionTimeout>");
	strcat(sBuffer,"</tt:AudioEncoderConfiguration>");
	strcat(sBuffer,"<tt:PTZConfiguration token=\"0_PTZ\">");
	strcat(sBuffer,"<tt:Name>0_PTZ</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:NodeToken>0_PTZ</tt:NodeToken>");
	strcat(sBuffer,"<tt:DefaultAbsolutePantTiltPositionSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:DefaultAbsolutePantTiltPositionSpace>");
	strcat(sBuffer,"<tt:DefaultAbsoluteZoomPositionSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:DefaultAbsoluteZoomPositionSpace>");
	strcat(sBuffer,"<tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace>");
	strcat(sBuffer,"<tt:DefaultRelativeZoomTranslationSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:DefaultRelativeZoomTranslationSpace>");
	strcat(sBuffer,"<tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace>");
	strcat(sBuffer,"<tt:DefaultContinuousZoomVelocitySpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:DefaultContinuousZoomVelocitySpace>");
	strcat(sBuffer,"<tt:DefaultPTZSpeed>");
	strcat(sBuffer,"<tt:PanTilt space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace\" y=\"1\" x=\"0\"></tt:PanTilt>");
	strcat(sBuffer,"<tt:Zoom space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace\" x=\"1\"></tt:Zoom>");
	strcat(sBuffer,"</tt:DefaultPTZSpeed>");
	strcat(sBuffer,"<tt:DefaultPTZTimeout>P16DT3H41M8.564S</tt:DefaultPTZTimeout>");
	strcat(sBuffer,"<tt:PanTiltLimits>");
	strcat(sBuffer,"<tt:Range>");
	strcat(sBuffer,"<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:URI>");
	strcat(sBuffer,"<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>");
	strcat(sBuffer,"<tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange>");
	strcat(sBuffer,"</tt:Range>");
	strcat(sBuffer,"</tt:PanTiltLimits>");
	strcat(sBuffer,"<tt:ZoomLimits>");
	strcat(sBuffer,"<tt:Range>");
	strcat(sBuffer,"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:URI>");
	strcat(sBuffer,"<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>");
	strcat(sBuffer,"</tt:Range>");
	strcat(sBuffer,"</tt:ZoomLimits>");
	strcat(sBuffer,"</tt:PTZConfiguration>");
	strcat(sBuffer,"</trt:Profiles>");
	strcat(sBuffer,"</trt:GetProfilesResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);

}

int BuildGetServicesString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");

	strcat(sBuffer,"<tds:GetServicesResponse>");
	strcat(sBuffer,"</tds:GetServicesResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}

int BuildGetSnapshotUriString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");

	strcat(sBuffer,"<trt:GetSnapshotUriResponse>");
	strcat(sBuffer,"<trt:MediaUri xsi:nil=\"true\"/>");
	strcat(sBuffer,"</trt:GetSnapshotUriResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}

int BuildwsdlGetProfileString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<trt:GetProfileResponse>");
	strcat(sBuffer,"<trt:Profile fixed=\"true\" token=\"0_main\">");
	strcat(sBuffer,"<tt:Name>0_main</tt:Name>");
	strcat(sBuffer,"<tt:VideoSourceConfiguration token=\"0_VSC\">");
	strcat(sBuffer,"<tt:Name>0_VSC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>0</tt:UseCount>");
	strcat(sBuffer,"<tt:SourceToken>0_VSC</tt:SourceToken>");
	sprintf(TmpBuffer,"<tt:Bounds height=\"%d\" width=\"%d\" y=\"0\" x=\"0\"></tt:Bounds>",
	        (int)g_sys_param.videoEnc[0][0].nEncodeHeight,
	        (int)g_sys_param.videoEnc[0][0].nEncodeWidth);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tt:VideoSourceConfiguration>");
	strcat(sBuffer,"<tt:AudioSourceConfiguration token=\"0_ASC\">");
	strcat(sBuffer,"<tt:Name>0_ASC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>0</tt:UseCount>");
	strcat(sBuffer,"<tt:SourceToken>0_ASC</tt:SourceToken>");
	strcat(sBuffer,"</tt:AudioSourceConfiguration>");
	strcat(sBuffer,"<tt:VideoEncoderConfiguration token=\"0_VEC_main\">");
	strcat(sBuffer,"<tt:Name>0_VEC_main</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>0</tt:UseCount>");
	strcat(sBuffer,"<tt:Encoding>H264</tt:Encoding>");
	sprintf(TmpBuffer,"<tt:Resolution><tt:Width>%d</tt:Width><tt:Height>%d</tt:Height></tt:Resolution>",
	        (int)g_sys_param.videoEnc[0][0].nEncodeWidth,
	        (int)g_sys_param.videoEnc[0][0].nEncodeHeight);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:Quality>31</tt:Quality>");
	strcat(sBuffer,"<tt:RateControl>");
	sprintf(TmpBuffer,"<tt:FrameRateLimit>%d</tt:FrameRateLimit>",
	        (int)g_sys_param.videoEnc[0][0].nFramerate);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tt:EncodingInterval>%d</tt:EncodingInterval>",
	        (int)g_sys_param.videoEnc[0][0].nKeyInterval);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:BitrateLimit>4000</tt:BitrateLimit>");
	strcat(sBuffer,"</tt:RateControl>");
	strcat(sBuffer,"<tt:H264>");
	strcat(sBuffer,"<tt:GovLength>25</tt:GovLength>");
	strcat(sBuffer,"<tt:H264Profile>Baseline</tt:H264Profile>");
	strcat(sBuffer,"</tt:H264>");
	strcat(sBuffer,"<tt:Multicast>");
	strcat(sBuffer,"<tt:Address><tt:Type>IPv4</tt:Type></tt:Address>");
	strcat(sBuffer,"<tt:Port>0</tt:Port>");
	strcat(sBuffer,"<tt:TTL>0</tt:TTL>");
	strcat(sBuffer,"<tt:AutoStart>false</tt:AutoStart>");
	strcat(sBuffer,"</tt:Multicast>");
	strcat(sBuffer,"<tt:SessionTimeout>PT1H37M41.620S</tt:SessionTimeout>");
	strcat(sBuffer,"</tt:VideoEncoderConfiguration>");
	strcat(sBuffer,"<tt:AudioEncoderConfiguration token=\"0_AEC\">");
	strcat(sBuffer,"<tt:Name>0_AEC</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>0</tt:UseCount>");
	strcat(sBuffer,"<tt:Encoding>G711</tt:Encoding>");
	strcat(sBuffer,"<tt:Bitrate>64</tt:Bitrate>");
	strcat(sBuffer,"<tt:SampleRate>8000</tt:SampleRate>");
	strcat(sBuffer,"<tt:Multicast>");
	strcat(sBuffer,"<tt:Address><tt:Type>IPv4</tt:Type></tt:Address>");
	strcat(sBuffer,"<tt:Port>0</tt:Port>");
	strcat(sBuffer,"<tt:TTL>0</tt:TTL>");
	strcat(sBuffer,"<tt:AutoStart>false</tt:AutoStart>");
	strcat(sBuffer,"</tt:Multicast>");
	strcat(sBuffer,"<tt:SessionTimeout>PT1H37M41.620S</tt:SessionTimeout>");
	strcat(sBuffer,"</tt:AudioEncoderConfiguration>");
	strcat(sBuffer,"<tt:PTZConfiguration token=\"0_PTZ\">");
	strcat(sBuffer,"<tt:Name>0_PTZ</tt:Name>");
	strcat(sBuffer,"<tt:UseCount>1</tt:UseCount>");
	strcat(sBuffer,"<tt:NodeToken>0_PTZ</tt:NodeToken>");
	strcat(sBuffer,"<tt:DefaultAbsolutePantTiltPositionSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/PositionGenericSpace</tt:DefaultAbsolutePantTiltPositionSpace>");
	strcat(sBuffer,"<tt:DefaultAbsoluteZoomPositionSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/PositionGenericSpace</tt:DefaultAbsoluteZoomPositionSpace>");
	strcat(sBuffer,"<tt:DefaultRelativePanTiltTranslationSpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/TranslationGenericSpace</tt:DefaultRelativePanTiltTranslationSpace>");
	strcat(sBuffer,"<tt:DefaultRelativeZoomTranslationSpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/TranslationGenericSpace</tt:DefaultRelativeZoomTranslationSpace>");
	strcat(sBuffer,"<tt:DefaultContinuousPanTiltVelocitySpace>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:DefaultContinuousPanTiltVelocitySpace>");
	strcat(sBuffer,"<tt:DefaultContinuousZoomVelocitySpace>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:DefaultContinuousZoomVelocitySpace>");
	strcat(sBuffer,"<tt:DefaultPTZSpeed>");
	strcat(sBuffer,"<tt:PanTilt space=\"http://www.onvif.org/ver10/tptz/PanTiltSpaces/GenericSpeedSpace\" y=\"1\" x=\"0\"></tt:PanTilt>");
	strcat(sBuffer,"<tt:Zoom space=\"http://www.onvif.org/ver10/tptz/ZoomSpaces/ZoomGenericSpeedSpace\" x=\"1\"></tt:Zoom>");
	strcat(sBuffer,"</tt:DefaultPTZSpeed>");
	strcat(sBuffer,"<tt:DefaultPTZTimeout>P16DT3H41M8.564S</tt:DefaultPTZTimeout>");
	strcat(sBuffer,"<tt:PanTiltLimits>");
	strcat(sBuffer,"<tt:Range>");
	strcat(sBuffer,"<tt:URI>http://www.onvif.org/ver10/tptz/PanTiltSpaces/VelocityGenericSpace</tt:URI>");
	strcat(sBuffer,"<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>");
	strcat(sBuffer,"<tt:YRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:YRange>");
	strcat(sBuffer,"</tt:Range>");
	strcat(sBuffer,"</tt:PanTiltLimits>");
	strcat(sBuffer,"<tt:ZoomLimits>");
	strcat(sBuffer,"<tt:Range>");
	strcat(sBuffer,"<tt:URI>http://www.onvif.org/ver10/tptz/ZoomSpaces/VelocityGenericSpace</tt:URI>");
	strcat(sBuffer,"<tt:XRange><tt:Min>-1</tt:Min><tt:Max>1</tt:Max></tt:XRange>");
	strcat(sBuffer,"</tt:Range>");
	strcat(sBuffer,"</tt:ZoomLimits>");
	strcat(sBuffer,"</tt:PTZConfiguration>");
	strcat(sBuffer,"</trt:Profile>");
	strcat(sBuffer,"</trt:GetProfileResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}


int BuildGetStreamUriString(char *sBuffer,char *sStreamString,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	get_ip_addr(ETH_WIRE_DEV,localIP);
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<trt:GetStreamUriResponse>");
	strcat(sBuffer,"<trt:MediaUri>");

	if(0 == strcmp(sStreamString,"0_main"))
		sprintf(TmpBuffer,"<tt:Uri>rtsp://%s:%d/0/%s:%s/main</tt:Uri>",
		        localIP,
		        (int)g_sys_param.rtsp.nRtspPort,
		        g_sys_param.userInfo.Admin.strName,
		        g_sys_param.userInfo.Admin.strPsw);
	else
		sprintf(TmpBuffer,"<tt:Uri>rtsp://%s:%d/0/%s:%s/sub</tt:Uri>",
		        localIP,
		        (int)g_sys_param.rtsp.nRtspPort,
		        g_sys_param.userInfo.Admin.strName,
		        g_sys_param.userInfo.Admin.strPsw);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:InvalidAfterConnect>false</tt:InvalidAfterConnect>");
	strcat(sBuffer,"<tt:InvalidAfterReboot>false</tt:InvalidAfterReboot>");
	strcat(sBuffer,"<tt:Timeout>PT1H37M40.700S</tt:Timeout>");
	strcat(sBuffer,"</trt:MediaUri>");
	strcat(sBuffer,"</trt:GetStreamUriResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}

int BuildGetVideoSourceConfigureString(char *sBuffer,char *sStreamString,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	get_ip_addr(ETH_WIRE_DEV,localIP);
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<trt:GetVideoSourceConfigurationResponse>");
	sprintf(TmpBuffer,"<trt:Configuration token=\"%s\">",sStreamString);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tt:Name>%s</tt:Name>",sStreamString);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:UseCount>0</tt:UseCount>");
	sprintf(TmpBuffer,"<tt:SourceToken>%s</tt:SourceToken>",sStreamString);
	strcat(sBuffer,TmpBuffer);
	if(0 == strcmp(sStreamString,"0_VSC"))
		sprintf(TmpBuffer,"<tt:Bounds height=\"%d\" width=\"%d\" y=\"0\" x=\"0\"></tt:Bounds>",
		        (int)g_sys_param.videoEnc[0][0].nEncodeHeight,
		        (int)g_sys_param.videoEnc[0][0].nEncodeWidth);
	else
		sprintf(TmpBuffer,"<tt:Bounds height=\"%d\" width=\"%d\" y=\"0\" x=\"0\"></tt:Bounds>",
		        (int)g_sys_param.videoEnc[0][1].nEncodeHeight,
		        (int)g_sys_param.videoEnc[0][1].nEncodeWidth);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</trt:Configuration>");
	strcat(sBuffer,"</trt:GetVideoSourceConfigurationResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}


int BuildGetUsersConfigureString(char *sBuffer,char *sStreamString,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	USER_INFO_PARAM userInfo;
	int ret = 0;
	int i = 0;
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	ret = getUserInfoParam(&userInfo);
	if (ret < 0)
	{
		return -1;
	}
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<tds:GetUsersResponse>");
	sprintf(TmpBuffer,"<tds:User>");
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tt:Username>%s</tt:Username>", userInfo.Admin.strName);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tt:Password>%s</tt:Password>", userInfo.Admin.strPsw);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<tt:UserLevel>Administrator");
	strcat(sBuffer,"</tt:UserLevel>");
	strcat(sBuffer,"</tds:User>");

	while(i++ <= 9)
	{
		sprintf(TmpBuffer,"<tds:User>");
		strcat(sBuffer,TmpBuffer);
		sprintf(TmpBuffer,"<tt:Username>%s</tt:Username>", userInfo.Users[i].strName);
		strcat(sBuffer,TmpBuffer);
		sprintf(TmpBuffer,"<tt:Password>%s</tt:Password>", userInfo.Users[i].strPsw);
		strcat(sBuffer,TmpBuffer);
		strcat(sBuffer,"<tt:UserLevel>Operator");
		
		strcat(sBuffer,"</tt:UserLevel>");
		strcat(sBuffer,"</tds:User>");
	}

	strcat(sBuffer,"</tds:GetUsersResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}


int BuildGetImagingSettingsConfigureString(char *sBuffer,char *sStreamString,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	VIDEO_IN_ATTR vinAttr;
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	getVideoInAttrParam(0, &vinAttr);
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<timg:GetImagingSettingsResponse>");
	strcat(sBuffer,"<timg:ImagingSettings>");
	sprintf(TmpBuffer,"<tt:Brightness>%d</tt:Brightness>", vinAttr.nBrightness);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tt:ColorSaturation>%d</tt:ColorSaturation>", vinAttr.nSaturation);
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<tt:Contrast>%d</tt:Contrast>", vinAttr.nContrast);
	strcat(sBuffer,TmpBuffer);

	strcat(sBuffer,"</timg:ImagingSettings>");
	strcat(sBuffer,"</timg:GetImagingSettingsResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}

int BuildSetImagingSettingsConfigureString(char *sBuffer,char *sStreamBrightness, char *sStreamSaturation, char *sStreamContrast,struct Namespace *pNameSpace)
{
	int nLen;
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	VIDEO_IN_ATTR vinAttr;
	getVideoInAttrParam(0, &vinAttr);
	vinAttr.nBrightness = atoi(sStreamBrightness);
	vinAttr.nSaturation= atoi(sStreamSaturation);
	vinAttr.nContrast= atoi(sStreamContrast);
	vadcDrv_SetBrightness(0, vinAttr.nBrightness);
	vadcDrv_SetContrast(0, vinAttr.nContrast);
	vadcDrv_SetSaturation(0, vinAttr.nSaturation);
	setVideoInAttrParam(0, &vinAttr);
	printf("vinAttr.nBrightness = %d,vinAttr.nContrast = %d,vinAttr.nSaturation= %d \n", vinAttr.nBrightness, vinAttr.nContrast, vinAttr.nSaturation);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<timg:SetImagingSettingsResponse>");
	strcat(sBuffer,"</timg:SetImagingSettingsResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}

int BuildGetSystemDateAndTimeConfigureString(char *sBuffer,char *sStreamString,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	DATE_PARAM param;
	int ret = 0;
	int i = 0;
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);


	memset(&param, 0, sizeof(DATE_PARAM));
	ret = getSystemTimeExt(&param.year, &param.month, &param.day, &param.week, &param.hour, &param.minute, &param.second);
	if (ret < 0)
	{
		return -1;
	}
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<tds:GetSystemDateAndTimeResponse>");
	sprintf(TmpBuffer,"<tds:SystemDateAndTime>");
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer, "<tds:DateTimeType>Manual</tds:DateTimeType>");
	strcat(sBuffer, "<tds:DaylightSavings>false</tds:DaylightSavings>");
	strcat(sBuffer, "<tds:TimeZone><tds:TZ></tds:TZ></tds:TimeZone>");

#if 1
	//填充UTC 时间参数
	sprintf(TmpBuffer, "%s%d%s", "<tds:UTCDateTime><tds:Time><tds:Hour>", param.hour, "</tds:Hour>");
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer, "%s%d%s", "<tds:Minute>", param.minute, "</tds:Minute>");
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer, "%s%d%s%s", "<tds:Second>", param.second, "</tds:Second>", "</tds:Time>");
	strcat(sBuffer,TmpBuffer);

	//填充UTC 日期参数
	sprintf(TmpBuffer, "%s%s%d%s", "<tds:Date>", "<tds:Year>", param.year, "</tds:Year>");
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer, "%s%d%s", "<tds:Month>", param.month, "</tds:Month>");
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer, "%s%d%s%s", "<tds:Day>", param.day, "</tds:Day>", "</tds:Date> </tds:UTCDateTime>");
	strcat(sBuffer,TmpBuffer);
#endif

	//填充LOCAL 时间参数
	sprintf(TmpBuffer, "%s%d%s", "<tds:LocalDateTime><tds:Time><tds:Hour>", param.hour, "</tds:Hour>");
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer, "%s%d%s", "<tds:Minute>", param.minute, "</tds:Minute>");
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer, "%s%d%s%s", "<tds:Second>", param.second, "</tds:Second>", "</tds:Time>");
	strcat(sBuffer,TmpBuffer);

	//填充LOCAL 日期参数
	sprintf(TmpBuffer, "%s%s%d%s", "<tds:Date>", "<tds:Year>", param.year, "</tds:Year>");
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer, "%s%d%s", "<tds:Month>", param.month, "</tds:Month>");
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer, "%s%d%s%s", "<tds:Day>", param.day, "</tds:Day>", "</tds:Date> </tds:LocalDateTime>");
	strcat(sBuffer,TmpBuffer);
	
	strcat(sBuffer,"</tds:SystemDateAndTime>");
	strcat(sBuffer,"</tds:GetSystemDateAndTimeResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}


int BuildSetSystemDateAndTimeConfigureString(char *sBuffer,char *sStreamString,struct Namespace *pNameSpace, const struct Http_Buffer *pBuf)
{
	int nLen;
	int ret = 0;
	DATE_PARAM param;

	char strHour[10]={0};
	char strMinute[10]={0};
	char strSecod[10]={0};
	char strYear[10]={0};
	char strMonth[10]={0};
	char strDay[10]={0};
		
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	printf("pBuf->Buffer+pBuf->headerlen = %s\n", pBuf->Buffer+pBuf->headerlen);

	memset(&param, 0, sizeof(DATE_PARAM));
	XmlGetStringValue(pBuf->Buffer+pBuf->headerlen,"Hour",strHour,1024);
	XmlGetStringValue(pBuf->Buffer+pBuf->headerlen,"Minute",strMinute,1024);
	XmlGetStringValue(pBuf->Buffer+pBuf->headerlen,"Second",strSecod,1024);
	XmlGetStringValue(pBuf->Buffer+pBuf->headerlen,"Year",strYear,1024);
	XmlGetStringValue(pBuf->Buffer+pBuf->headerlen,"Month",strMonth,1024);
	XmlGetStringValue(pBuf->Buffer+pBuf->headerlen,"Day",strDay,1024);


	printf("%s%s%s\n", strHour, strMinute, strSecod);

	param.hour = atoi(strHour);
	param.minute = atoi(strMinute);
	param.second = atoi(strSecod);
	param.year  = atoi(strYear);
	param.month = atoi(strMonth);
	param.minute= atoi(strMinute);

	ret = setSystemTime(param.year, param.month, param.day, param.hour, param.minute, param.second);
	if (ret < 0)
	{
		return -1;
	}
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<timg:SetSystemDateAndTimeResponse>");
	strcat(sBuffer,"</timg:SetSystemDateAndTimeResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}



int BuildGetCertificatesResponseConfigureString(char *sBuffer,char *sStreamString,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	DATE_PARAM param;
	int ret = 0;
	int i = 0;
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	memset(&param, 0, sizeof(DATE_PARAM));
	ret = getSystemTimeExt(&param.year, &param.month, &param.day, &param.week, &param.hour, &param.minute, &param.second);
	if (ret < 0)
	{
		return -1;
	}

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<tds:GetCertificatesResponse>");
	strcat(sBuffer,"<tds:NvtCertificate>");
	strcat(sBuffer,"<tds:CertificateID></tds:CertificateID>");
	strcat(sBuffer,"<tds:Certificate xmime:contentType="">");
	strcat(sBuffer,"<tds:Data></tds:Data>");

	strcat(sBuffer,"</tds:Certificate>");
	strcat(sBuffer,"</tds:NvtCertificate>");
	strcat(sBuffer,"</tds:GetCertificatesResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}


 int BuildGetCertificatesStatusResponseConfigureString(char *sBuffer,char *sStreamString,struct Namespace *pNameSpace)
 {
	 int nLen;
	 char TmpBuffer[1024];
	 int ret = 0;
	 nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	 BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
 
	 //start soap-env body
	 strcat(sBuffer,"<SOAP-ENV:Body>");
	 strcat(sBuffer,"<tds:GetCertificatesStatusResponse>");
	 strcat(sBuffer,"<tds:CertificateStatus>");
	 strcat(sBuffer,"<tds:CertificateID></tds:CertificateID>");
	 strcat(sBuffer,"<tds:Status>false</tds:Status>");
	 
	 strcat(sBuffer,"</tds:CertificateStatus>");
	 strcat(sBuffer,"</tds:GetCertificatesStatusResponse>");
	 strcat(sBuffer,"</SOAP-ENV:Body>");
	 strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	 return strlen(sBuffer);
 }


int BuildGetOptionsConfigureString(char *sBuffer,char *sStreamString,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	VIDEO_IN_ATTR vinAttr;
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<timg:GetOptionsResponse>");
	strcat(sBuffer,"<timg:ImagingOptions>");
    
	strcat(sBuffer,"<tt:Brightness><tt:Min>0.0</tt:Min><tt:Max>255</tt:Max></tt:Brightness>");
    strcat(sBuffer,"<tt:ColorSaturation><tt:Min>0.0</tt:Min><tt:Max>255</tt:Max></tt:ColorSaturation>");
    strcat(sBuffer,"<tt:Contrast><tt:Min>0.0</tt:Min><tt:Max>255</tt:Max></tt:Contrast>");
    strcat(sBuffer,"<tt:Exposure><tt:Min>0.0</tt:Min><tt:Max>255</tt:Max></tt:Exposure>");
	strcat(sBuffer,"</timg:ImagingOptions>");
	strcat(sBuffer,"</timg:GetOptionsResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}



int BuildGetInitialTerminationTimeString(char *sBuffer,char *suuid,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	char localIP[16];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);
	get_ip_addr(ETH_WIRE_DEV,localIP);
	strcat(sBuffer,"<SOAP-ENV:Header>");
	sprintf(TmpBuffer,"<wsa5:MessageID>%s</wsa5:MessageID>",suuid);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<wsa5:ReplyTo SOAP-ENV:mustUnderstand=\"true\">");
	strcat(sBuffer,"<wsa5:Address>http://www.w3.org/2005/08/addressing/anonymous</wsa5:Address>");
	strcat(sBuffer,"</wsa5:ReplyTo>");
	if(g_wOnvifPort == 80)
		sprintf(TmpBuffer,"<wsa5:To SOAP-ENV:mustUnderstand=\"true\">http://%s/onvif/Events_service</wsa5:To>",localIP);
	else
		sprintf(TmpBuffer,"<wsa5:To SOAP-ENV:mustUnderstand=\"true\">http://%s:%d/onvif/Events_service</wsa5:To>",localIP,g_wOnvifPort);
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"<wsa5:Action SOAP-ENV:mustUnderstand=\"true\">http://www.onvif.org/ver10/events/wsdl/EventPortType/CreatePullPointSubscriptionRequest</wsa5:Action>");
	strcat(sBuffer,"</SOAP-ENV:Header>");

	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"<tev:CreatePullPointSubscriptionResponse>");
	strcat(sBuffer,"<tev:SubscriptionReference><wsa5:Address xsi:nil=\"true\"/></tev:SubscriptionReference>");
	sprintf(TmpBuffer,"<wsnt:CurrentTime>1970-01-01T00:00:00Z</wsnt:CurrentTime>");
	strcat(sBuffer,TmpBuffer);
	sprintf(TmpBuffer,"<wsnt:TerminationTime>1970-01-01T00:00:00Z</wsnt:TerminationTime>");
	strcat(sBuffer,TmpBuffer);
	strcat(sBuffer,"</tev:CreatePullPointSubscriptionResponse>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
}

int BuildGetFailString(char *sBuffer,struct Namespace *pNameSpace)
{
	int nLen;
	char TmpBuffer[1024];
	nLen = sprintf(sBuffer,"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	BuildDevInfoHeaderString(sBuffer+nLen,pNameSpace);

	//start soap-env body
	strcat(sBuffer,"<SOAP-ENV:Body>");
	strcat(sBuffer,"SOAP-ENV:Fault>");
	strcat(sBuffer,"<faultcode>SOAP-ENV:Client</faultcode>");
	strcat(sBuffer,"<faultstring>HTTP GET method not implemented</faultstring>");
	strcat(sBuffer,"</SOAP-ENV:Fault>");
	strcat(sBuffer,"</SOAP-ENV:Body>");
	strcat(sBuffer,"</SOAP-ENV:Envelope>\r\n");
	return strlen(sBuffer);
}


void ProcessAction(const struct Http_Buffer *pBuf,int sock)
{
	int nLen,nLen2;
	char TmpBuf[65535],TmpBuf2[65535];
	PTZ_CMD   ptzCmd;
	char strPTZ[100];
	char strUser[100];
	char strPasswd[100];


	if(pBuf->nHasAction)
	{
		if(XmlContainString(pBuf->action,"GetDeviceInformation"))
		{
			nLen = BuildDeviceInfoString(TmpBuf,devinfo_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetNetworkInterfaces"))
		{
			nLen = BuildNetworkInterfaceString(TmpBuf,devinfo_namespaces);//networkinterface_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetDNS"))
		{
			nLen = BuildGetDNSString(TmpBuf,devinfo_namespaces);//GetDNS_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetScopes"))
		{
			nLen = BuildGetScopesString(TmpBuf,devinfo_namespaces);//GetScopes_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetCapabilities"))
		{
			nLen = BuildGetCapabilitiesString(TmpBuf,devinfo_namespaces);//GetCapabilities_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetServices"))
		{
			nLen = BuildGetServicesString(TmpBuf,devinfo_namespaces);//GetServices_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"wsdlGetVideoSources"))
		{
			nLen = BuildWsdlGetVideoSourcesString(TmpBuf,devinfo_namespaces);//wsdlGetVideoSources_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetProfiles"))
		{
			nLen = BuildGetProfilesString(TmpBuf,devinfo_namespaces);//GetProfiles_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetSnapshotUri"))
		{
			nLen = BuildGetSnapshotUriString(TmpBuf,devinfo_namespaces);//GetSnapshotUri_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		
		else if(XmlContainString(pBuf->action,"wsdlGetProfile"))
		{
			nLen = BuildwsdlGetProfileString(TmpBuf,devinfo_namespaces);//WsdlGetProfile_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetStreamUri"))
		{
			char strStream[1024]="0_main";
			XmlGetStringValue(pBuf->Buffer + pBuf->headerlen,"ProfileToken",strStream,1024);
			nLen = BuildGetStreamUriString(TmpBuf,strStream,devinfo_namespaces);//GetStreamUri_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetVideoSourceConfiguration"))
		{
			char strStream[1024]="0_VSC";
			nLen = BuildGetVideoSourceConfigureString(TmpBuf,strStream,devinfo_namespaces);//GetVideoSourceConfiguration_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"GetImagingSettings"))
		{
			char strStream[1024]="0_VSC";
			nLen = BuildGetImagingSettingsConfigureString(TmpBuf,strStream,devinfo_namespaces);//GetVideoSourceConfiguration_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
			//printf("TmpBuf2 = %s\n", TmpBuf2);
		}
		else if(XmlContainString(pBuf->action,"SetImagingSettings"))
		{
			printf("SetImagingSettings::pBUf = %s\n", pBuf);
			char strBrightness[10]={0};
			char strContrast[10]={0};
			char strColorSaturation[10]={0};

			XmlGetStringValue(pBuf->Buffer+pBuf->headerlen,"Brightness",strBrightness,1024);
			XmlGetStringValue(pBuf->Buffer+pBuf->headerlen,"Contrast",strContrast,1024);
			XmlGetStringValue(pBuf->Buffer+pBuf->headerlen,"ColorSaturation",strColorSaturation,1024);
			printf("strBrightness = %s %s %s\n", strBrightness, strContrast, strColorSaturation);
			nLen = BuildSetImagingSettingsConfigureString(TmpBuf,strBrightness,strColorSaturation,strContrast, devinfo_namespaces);//GetVideoSourceConfiguration_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		
		else if(XmlContainString(pBuf->action,"GetOptions"))
		{
			char strStream[1024]="0_VSC";
			nLen = BuildGetOptionsConfigureString(TmpBuf,strStream,devinfo_namespaces);//GetVideoSourceConfiguration_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
			//printf("TmpBuf2 = %s\n", TmpBuf2);
		}
		else if(XmlContainString(pBuf->action,"CreateUsers"))
		{
			int ret = 0;
			ret = XmlGetStringValue(pBuf,"Username",strUser,100);
			printf("strUser = %s ret = %d\n", strUser, ret );
			ret = XmlGetStringValue(pBuf,"Password",strPasswd,100);
			printf("strUser = %s ret = %d\n", strPasswd, ret );

			nLen = BuildwsdlGetProfileString(TmpBuf,devinfo_namespaces);//WsdlGetProfile_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}

		else if(XmlContainString(pBuf->action,"GetUsers"))
		{
			char strStream[1024]="0_VSC";
			nLen = BuildGetUsersConfigureString(TmpBuf,strStream,devinfo_namespaces);//GetVideoSourceConfiguration_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
			printf("TmpBuf2 = %s\n", TmpBuf2);
		}

		else if(XmlContainString(pBuf->action,"ContinuousMove"))
		{
			XmlGetStringValue(pBuf,"Velocity",strPTZ,100);
			nLen = BuildwsdlGetProfileString(TmpBuf,devinfo_namespaces);//WsdlGetProfile_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			if(strstr(strPTZ,"x=\"-0.5\""))
				ptzCmd.nCmd = RIGHT_START;
			else if(strstr(strPTZ,"x=\"0.5\""))
				ptzCmd.nCmd = LEFT_START;
			else if(strstr(strPTZ,"y=\"0.5\""))
				ptzCmd.nCmd = DOWN_START;
			else if(strstr(strPTZ,"y=\"-0.5\""))
				ptzCmd.nCmd = UP_START;
			else
				printf("ptz control command error\n");
			ptzControl(0, ptzCmd);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		else if(XmlContainString(pBuf->action,"Stop"))
		{
			ptzCmd.nCmd = LEFT_STOP;
			ptzControl(0, ptzCmd);
			nLen = BuildwsdlGetProfileString(TmpBuf,devinfo_namespaces);//WsdlGetProfile_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		/*网络参数设置*/
		else if(XmlContainString(pBuf->action,"GetRelayOutputs"))
		{
			ptzCmd.nCmd = LEFT_STOP;
			ptzControl(0, ptzCmd);
			nLen = BuildwsdlGetProfileString(TmpBuf,devinfo_namespaces);//WsdlGetProfile_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
		/*系统时间设置*/
		else if(XmlContainString(pBuf->action,"GetSystemDateAndTime"))
		{
			char strStream[1024]="0_VSC";
			nLen = BuildGetSystemDateAndTimeConfigureString(TmpBuf,strStream,devinfo_namespaces);//GetVideoSourceConfiguration_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
			printf("TmpBuf2 = %s\n", TmpBuf2);
		}

		else if(XmlContainString(pBuf->action,"SetSystemDateAndTime"))
		{
			char strStream[1024]="0_VSC";
			nLen = BuildSetSystemDateAndTimeConfigureString(TmpBuf,strStream,devinfo_namespaces, pBuf);//GetVideoSourceConfiguration_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
			printf("TmpBuf2 = %s\n", TmpBuf2);
		}

		//CertificatesResponse

		else if(XmlContainString(pBuf->action,"GetCertificatesStatus"))
		{
			char strStream[1024]="0_VSC";
			nLen = BuildGetCertificatesStatusResponseConfigureString(TmpBuf,strStream,devinfo_namespaces);//GetVideoSourceConfiguration_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
			printf("okk\n");
			return 0;
//			printf("TmpBuf2 = %s\n", TmpBuf2);
		}
		
		else if(XmlContainString(pBuf->action,"GetCertificates"))
		{
			char strStream[1024]="0_VSC";
			nLen = BuildGetCertificatesResponseConfigureString(TmpBuf,strStream,devinfo_namespaces);//GetVideoSourceConfiguration_namespaces);
			nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
			printf("two okk\n");
			return 0;
			//printf("TmpBuf2 = %s\n", TmpBuf2);
		}
	
		else
		{
			printf("::ProcessAction COMMAND %s\n", pBuf->action);
			nLen = BuildGetFailString(TmpBuf,devinfo_namespaces);
			nLen2 = BuildHttpFailHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
	}
	else
	{
		char Value[1024];
		if(0 == XmlGetStringValue(pBuf->Buffer+pBuf->headerlen,"InitialTerminationTime",Value,1024))
		{
			char suuid[1024];
			if(0 == XmlGetStringValue(pBuf->Buffer+pBuf->headerlen,"a:MessageID",suuid,1024))
			{
				nLen = BuildGetInitialTerminationTimeString(TmpBuf,suuid,devinfo_namespaces);//GetTerminationTime_namespaces);
				nLen2 = BuildHttpHeaderString(pBuf,TmpBuf2,nLen);
				strcat(TmpBuf2,TmpBuf);
				send(sock,TmpBuf2,nLen + nLen2,0);
			}
		}
		else
		{
			nLen = BuildGetFailString(TmpBuf,devinfo_namespaces);
			nLen2 = BuildHttpFailHeaderString(pBuf,TmpBuf2,nLen);
			strcat(TmpBuf2,TmpBuf);
			send(sock,TmpBuf2,nLen + nLen2,0);
		}
	}
	//printf("recved %d bytes:%s\n\n\n",nLen+nLen2,pBuf->Buffer);
	//printf("send %d bytes:%s\n\n\n",nLen+nLen2,TmpBuf2);
}


int ONVIF_NewConnThread(void *pPara)
{
	struct ONVIF_ConnThread *pConnThread = (struct ONVIF_ConnThread *)pPara;
	int sock = pConnThread->sock;
	int ret = 0;
	fd_set fset;
	struct timeval to;
	struct sockaddr_in addr;
	struct Http_Buffer RecvBuf;
	int   nNoDataCount = 0;
	memset(&RecvBuf,0,sizeof(RecvBuf));
	while(1)
	{
		if(nNoDataCount >= 5)
			break;
		FD_ZERO(&fset);
		FD_SET(sock,&fset);
		memset(&to,0,sizeof(to));
		to.tv_sec = 5;
		to.tv_usec = 0;
		ret = select(sock + 1,&fset,NULL,NULL,&to);
		if(ret == 0)
		{
			nNoDataCount ++;
			continue;
		}
		if(ret < 0 && errno == EINTR)
		{
			nNoDataCount ++;
			continue;
		}
		if(ret < 0)
			break;
		if(!FD_ISSET(sock,&fset))
			break;
		ret = recv(sock,RecvBuf.Buffer + RecvBuf.nBufLen,65535 - RecvBuf.nBufLen,0);
		//printf("%s(%d) onvif tcp recved %d bytes\n",inet_ntoa(pConnThread->remote_addr.sin_addr),ntohs(pConnThread->remote_addr.sin_port),ret);
		if(ret <= 0)
			break;
		nNoDataCount = 0;
		RecvBuf.nBufLen += ret;
		//printf("%s(%d) onvif tcp recved total %d bytes:%s\n",inet_ntoa(pConnThread->remote_addr.sin_addr),ntohs(pConnThread->remote_addr.sin_port),RecvBuf.nBufLen,RecvBuf.Buffer);
		ret = HttpParse(&RecvBuf);
		if(ret >= 0)
		{

			ProcessAction(&RecvBuf,sock);
			break;
		}

	}
	printf("onvif connect thread exit\n");
	close(sock);
	free(pConnThread);
	pthread_exit(NULL);
	return 0;
}

int ONVIFNewConnect(int hsock,struct sockaddr_in *pAddr)
{
	int ret;
	struct ONVIF_ConnThread *pConnThread = (struct ONVIF_ConnThread *)malloc(sizeof(struct ONVIF_ConnThread));
	if(pConnThread == NULL)
	{
		printf("onvif new connect malloc memeory error\n");
		return -1;
	}
	memset(pConnThread,0,sizeof(struct ONVIF_ConnThread));
	pConnThread->sock = hsock;
	memcpy(&pConnThread->remote_addr,pAddr,sizeof(struct sockaddr_in));
	ret = pthread_create(&pConnThread->hthread,NULL,(void *)&ONVIF_NewConnThread,pConnThread);
	if(ret < 0)
	{
		printf("create onvif new connect thread error\n");
		close(hsock);
		free(pConnThread);
		return -1;
	}
	return 0;
}
int hListenSock = -1;


void stop_sock(int signo) 
{
	printf("oops! hListenSock stop!!!\n");
	close(hListenSock);
	exit(0);
}


int ONVIF_ServiceThread()
{
	int ret = 0;
	int len = 0;
	int opt;
	int hConnSock = -1;
	//int hListenSock = -1;
	fd_set fset;
	struct timeval to;
	struct sockaddr_in addr;
	FD_ZERO(&fset);
	memset(&to,0,sizeof(to));
	memset(&addr,0,sizeof(addr));
	len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(g_wOnvifPort);
	hListenSock = socket(AF_INET,SOCK_STREAM,0);
	if(hListenSock < 0)
	{
		printf("create onvif service listen sock error\n");
		pthread_exit(NULL);
		return -1;
	}
	ret = bind(hListenSock,(struct sockaddr *)&addr,sizeof(addr));
	if(ret < 0)
	{
		printf("bind onvif service listen sock error\n");
		close(hListenSock);
		pthread_exit(NULL);
		return ret;
	}
	ret = listen(hListenSock,200);
	if(ret < 0)
	{
		printf("listen onvif sock error\n");
		close(hListenSock);
		pthread_exit(NULL);
		return ret;
	}
	
	while(g_OnvifServiceRunning)
	{
		hConnSock = -1;
		to.tv_sec = 3;
		to.tv_usec = 0;
		FD_SET(hListenSock,&fset);
		ret = select(hListenSock + 1,&fset,NULL,NULL,&to);
		if(ret == 0)
			continue;
		if(ret < 0)
		{
			if(errno == EINTR)
				continue;
			else
			{
				printf("onvif listen sock select error\n");
				break;
			}
		}
		if(!FD_ISSET(hListenSock,&fset))
			continue;
		if(!g_OnvifServiceRunning)
			break;
		hConnSock = accept(hListenSock,(struct sockaddr *)&addr,&len);
		if(hConnSock < 0)
		{
			if(errno == 1000)
			{
				printf("too many file opened\n");
				continue;
			}
		}
		printf("client connected:%s(%d)\n",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
		opt = 1;
		ret = setsockopt(hConnSock,IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
		if (ret < 0)
		{
			//printf("set onvif tcp sockopt error\n");
			close(hConnSock);
			continue;
		}
		ONVIFNewConnect(hConnSock,&addr);
	}
	close(hListenSock);
	pthread_exit(NULL);
	return 0;
}

int ONVIF_ServiceStart()
{
	int ret;
	g_OnvifServiceRunning = 1;
	//signal(SIGINT, stop_sock); 
	
	ret = pthread_create(&g_OnvifServer.hServiceThread,NULL,(void *)&ONVIF_ServiceThread,NULL);
	if(ret < 0)
	{
		printf("create onvif service thread error\n");
		return -1;
	}
	return 0;
}

void ONVIF_ServiceStop()
{
	g_OnvifServiceRunning = 0;
	pthread_join(g_OnvifServer.hServiceThread,NULL);
}
