#ifndef __ERROR_H__
#define __ERROR_H__

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>		/* ANSI C header file */
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include "typecomm.h"
#include <libgen.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef OS_AIX
#include <sys/mode.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

enum MsgLevel{MSG_DEBUG=1,MSG_INFO,MSG_MONITOR,MSG_ALARM,MSG_ERROR,MSG_SMS};

typedef enum en_msg{	/*错误信息+日志信息*/
	E_MSG = 1 ,
	L_MSG = 2 
} T_MSG;

/* 
 *	函数原型定义
 */
void ErrExit(const char *fmt, ...);
void ErrMsg(const char *fmt, ...);
void SysErrDump(const char *fmt, ...);
void SysErrExit(const char *fmt, ...);
void SysErrMsg(const char *fmt, ...);
void RegisterLogMsg(char *sLogFileNor,char *sLogFileErr,int iMsgLevel);
void LogMsg(enum MsgLevel level,const char *fmt,...);
int SetMsgLevel(int newlevel);
#define OutDiagnoseMsg(format) \
	do\
	{\
		char tmpMsgStr[60]="";\
		strcpy(tmpMsgStr,format);\
		strcat(tmpMsgStr,"[%s line %d]");\
		SysErrMsg(tmpMsgStr,__FILE__,__LINE__);\
	}while(0);

void PrintFormat(enum en_msg en_,char *fmt,...);
int RegLockFile(char sFile[]);
void PrintSqlError(int iErr,char *sSql);

/*
对于需要开放给外部使用的变量，宏，函数等声明都放在这之前
下面的宏LIBCOMMON_DEV之内的只提供给开发本库的时候用到的各种声明
*/



//#ifdef LIBCOMMON_DEV
#define MAXLINE	4096	

//#endif /*LIBCOMMON_DEV*/



#endif
