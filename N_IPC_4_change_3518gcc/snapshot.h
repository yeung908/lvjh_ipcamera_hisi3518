#ifndef __SNAPSHOT_H_
#define __SNAPSHOT_H_

int SNAPSHOT_Init();
int SNAPSHOT_Destroy();
int SNAPSHOT_Start();
int SNAPSHOT_Stop();
int SNAPSHOT_Pause();
int SNAPSHOT_Resume();
int SNAPSHOT_Send(int nChannel, int nType, void *buffer, int size, int uploadType);
int ALARM_Info_Upload(int nType, int nUploadType);
int manual_record_result(int nType);
int set_Email_Alarm_status(int nType);
int get_snapshot_buffer_size(char *snapshot_buffer, int *snapshot_size);



#endif


