/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：recordSDK.h
* 文件说明：该文件描述了录像SDK的API
* 作    者：庄惠斌
* 版本信息：V1.0
* 设计日期: 2007-01-29
* 修改记录:
*   修改1      日    期:
*              姓    名:
*              修改内容:
*   修改:2     日    期:
*              姓    名:
*              修改内容:
* 其他说明: 无
******************************************************************************/

#ifndef	__RECORD_SDK_H_
#define	__RECORD_SDK_H_

#include "recordStruct.h"

// 录像SDK打开
int RECORDSDK_Open(int nChnNum);

// 录像SDK关闭
int RECORDSDK_Close();

// 获取录像SDK参数
int RECORDSDK_Setup(RECORD_SETUP setup);

// 设置SDK参数
int RECORDSDK_GetSetup(RECORD_SETUP *setup);

// 启动录像SDK
int RECORDSDK_Start();

// 停止录像SDK
int RECORDSDK_Stop();

// 暂停录像SDK
int RECORDSDK_Pause();

// 恢复录像SDK
int RECORDSDK_Resume();

// 发送帧数据到录像SDK
int RECORDSDK_SendAVData(int channel, void *buffer, int size);

// 录像SDK控制操作
int RECORDSDK_Operate(void *inParam, void *outParam, int *outSize);

#endif

