/******************************************************************************
* 版权信息：(C) 2006-2010, 深圳市泰达仕科技有限公司研发部版权所有
* 系统名称：录像SDK
* 文件名称：baseType.h
* 文件说明：该文件描述了数据基本类型
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

#ifndef	__BASETYPE_H_
#define	__BASETYPE_H_

typedef	char				int8;
typedef	unsigned char		uint8;
typedef	short			int16;
typedef	unsigned short	uint16;
typedef	long				int32;
typedef	unsigned long		uint32;

typedef	char				INT8;
typedef	unsigned char		UINT8;
typedef	short			INT16;
typedef	unsigned short	UINT16;
typedef	long				INT32;
typedef	unsigned long		UINT32; 

typedef unsigned long		DWORD;
typedef unsigned char		BYTE;
typedef unsigned short		WORD;
typedef float				FLOAT;
typedef void*				LPVOID;
typedef int				BOOL;
typedef void* 			HANDLE;

#ifndef	TRUE
#define	TRUE	(1 == 1)
#endif

#ifndef	FALSE
#define	FALSE	(1 == 0)
#endif

#ifndef	true
#define	true	(1 == 1)
#endif
#ifndef	false
#define	false	(1 == 0)
#endif

#endif

