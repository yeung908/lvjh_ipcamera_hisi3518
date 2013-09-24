rem 生成onvif.h后，再#import "wsse.h"
rem 
rem deviceio.wsdl 
wsdl2h.exe -sck -c -t WS-typemap.dat -o onvif.h remotediscovery.wsdl
rem wsdl2h.exe -sck -c -t G:\work\onvif\gsoap-2.8\gsoap\WS\WS-typemap.dat -o onvif.h remotediscovery.wsdl
rem wsdl2h.exe -sck -c -t WS-typemap.dat -o onvif.h remotediscovery.wsdl
