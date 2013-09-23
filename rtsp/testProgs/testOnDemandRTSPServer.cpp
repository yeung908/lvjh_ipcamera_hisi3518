
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

UsageEnvironment* env;
Boolean reuseFirstSource = True;
Boolean iFramesOnly = False;

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
			   char const* streamName, char const* inputFileName); // fwd

int main(int argc, char** argv) {
  TaskScheduler* scheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);

  UserAuthenticationDatabase* authDB = NULL;
  USER_INFO_PARAM user_info_param;
  getUSERConfigure(&user_info_param);

  // Create the RTSP server:
  RTSPServer* rtspServer = RTSPServer::createNew(*env, 554, authDB);
  if (rtspServer == NULL) {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    exit(1);
  }	

  char const* descriptionString
    = "Session streamed by \"testOnDemandRTSPServer\"";
  {
	  char streamName[80];
	  memset(streamName, 0, 80);
	  sprintf(streamName, "%s%s%s%s%s", "0/",  user_info_param.Admin.strName, ":",user_info_param.Admin.strPsw, "/main");
	  char const* inputFileName = CHN0_SOUR;
      OutPacketBuffer::maxSize = 1048576;
	  ServerMediaSession* sms
		= ServerMediaSession::createNew(*env, streamName, streamName,
						descriptionString);
	  sms->addSubsession(H264VideoFileServerMediaSubsession
				 ::createNew(*env, inputFileName, reuseFirstSource));
	  rtspServer->addServerMediaSession(sms);
  
	  announceStream(rtspServer, sms, streamName, inputFileName);
	}
  {
	char sudStreamName[80];
	memset(sudStreamName, 0, 80);
  	sprintf(sudStreamName, "%s%s%s%s%s", "0/",  user_info_param.Admin.strName, ":",user_info_param.Admin.strPsw, "/sub");
	printf("sudStreamName = %s\n\n\n", sudStreamName);
    char const* inputFileName = CHN1_SOUR;
	OutPacketBuffer::maxSize = 1048576;
    ServerMediaSession* sms
      = ServerMediaSession::createNew(*env, sudStreamName, sudStreamName,
				      descriptionString);
    sms->addSubsession(H264VideoFileServerMediaSubsession
		       ::createNew(*env, inputFileName, reuseFirstSource));
    rtspServer->addServerMediaSession(sms);

    announceStream(rtspServer, sms, sudStreamName, inputFileName);
  }

  {
	char sudStreamName[80];
	memset(sudStreamName, 0, 80);
  	sprintf(sudStreamName, "%s%s%s%s%s", "0/",  user_info_param.Admin.strName, ":",user_info_param.Admin.strPsw, "/2sub");
	printf("sudStreamName = %s\n\n\n", sudStreamName);
    char const* inputFileName = CHN2_SOUR;
	OutPacketBuffer::maxSize = 1048576;
    ServerMediaSession* sms
      = ServerMediaSession::createNew(*env, sudStreamName, sudStreamName,
				      descriptionString);
    sms->addSubsession(H264VideoFileServerMediaSubsession
		       ::createNew(*env, inputFileName, reuseFirstSource));
    rtspServer->addServerMediaSession(sms);

    announceStream(rtspServer, sms, sudStreamName, inputFileName);
  }

  if (rtspServer->setUpTunnelingOverHTTP(readConfigFile(RTSPPORT_CONFIGURE_FILE)
) ) {
   *env << "\n(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling.)\n";
  } else {
    *env << "\n(RTSP-over-HTTP tunneling is not available.)\n";
  }

  env->taskScheduler().doEventLoop(); 
  return 0; 
}

static void announceStream(RTSPServer* rtspServer, ServerMediaSession* sms,
			   char const* streamName, char const* inputFileName) {
  char* url = rtspServer->rtspURL(sms);
  UsageEnvironment& env = rtspServer->envir();
  delete[] url;
}
