#GSOAP_ROOT = /mnt/hgfs/work/onvif/gsoap-2.8/gsoap
GSOAP_ROOT = /mnt/share_windows/gsoap-2.8/gsoap

CC = gcc -g -O2 #-DWITH_OPENSSL -DHAVE_PTHREAD_H
INCLUDE = -I$(GSOAP_ROOT)
SERVER_OBJS = soapC.o soapServer.o stdsoap2.o function.o webServer.o 
SERVER_SSL_OBJS = soapC.o soapServer.o stdsoap2.o function.o webServerSsl.o 
DISCOVER_SERVER_OBJS = soapC.o soapServer.o stdsoap2.o function.o discoveryServer.o 
DISCOVER_CLIENT_OBJS = soapC.o soapClient.o stdsoap2.o function.o discoveryClient.o


all: discoverServer discoverClient 
#server

discoverServer: $(DISCOVER_SERVER_OBJS) 
	$(CC) -o discoveryServer $(DISCOVER_SERVER_OBJS) $(INCLUDE) -lpthread 
	#-lssl -lcrypto

discoverClient: $(DISCOVER_CLIENT_OBJS) 
	$(CC) -o discoveryClient $(DISCOVER_CLIENT_OBJS) $(INCLUDE) -lpthread 
	#-lssl -lcrypto

server: $(SERVER_OBJS)
	$(CC) -o webServer $(SERVER_OBJS) $(INCLUDE) -lpthread -lssl -lcrypto

serverSsl: $(SERVER_SSL_OBJS)
	$(CC) -o webServerSsl $(SERVER_SSL_OBJS) $(INCLUDE) -lpthread -lssl -lcrypto

clean:
	rm -f *.o *.a discoveryServer discoveryClient webServer webServerSsl


