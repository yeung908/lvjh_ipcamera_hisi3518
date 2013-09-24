soapcpp2.exe -2 -L -c onvif.h -d..\ -IG:\work\onvif\gsoap-2.8\gsoap\import

cd ..

mkdir xml
move *.xml xml

mkdir nsmap
move *.nsmap nsmap
copy /y nsmap\RemoteDiscoveryBinding.nsmap onvif.nsmap
