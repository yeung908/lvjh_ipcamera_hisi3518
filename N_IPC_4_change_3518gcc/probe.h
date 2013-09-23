#ifndef __PROBE_H_
#define __PROBE_H_

int probeModuleInit();
int probeModuleDeInit();

int getResetParamStatus();
int getProbeInStatus(int nChannel);
int setProbeInStatus(int nChannel);
int getTalkRequestStatus();
int getProbeOutStatus(int nChannel);
int setProbeOutStatus(int nChannel, int time);
int manualSetProbeOutStatus(int nChannel, int status);

int probeProcStart();
int probeProcStop();
int probeProcPause();
int probeAlarmStop();
int probeAlarmStart();


int probeProcResume();

#endif
