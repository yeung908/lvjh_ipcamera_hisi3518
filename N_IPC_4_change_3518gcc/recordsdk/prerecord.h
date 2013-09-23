/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：prerecord.h
* 文件说明：该文件描述了操作录像索引文件的函数声明
* 作    者：庄惠斌
*           包括：
*           1．最在录像文件数的宏定义
*			2. RECORD_FILE_INDEX数据结构的定义
*           3．操作录像索引文件的函数声明
* 版本信息：V1.0
* 设计日期: 2007-02-07
* 修改记录:
*   修改1      日    期:
*              姓    名:
*              修改内容:
*   修改:2     日    期:
*              姓    名:
*              修改内容:
* 其他说明: 无
******************************************************************************/

#ifndef __PRERECORD_H_
#define __PRERECORD_H_

int Open_prerecord(int channel, int size);
int Close_prerecord(int channel);
int Reset_prerecord(int channel);
int GetSize_prerecord(int channel);

int SendOneFrame_prerecord(int channel, void * buf, int size);
void* GetOneFrame_prerecord(int channel, void * buf, int *size);

#endif

