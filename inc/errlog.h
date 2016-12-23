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

typedef enum en_msg{	/*������Ϣ+��־��Ϣ*/
	E_MSG = 1 ,
	L_MSG = 2 
} T_MSG;

/* 
 *	����ԭ�Ͷ���
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
������Ҫ���Ÿ��ⲿʹ�õı������꣬������������������֮ǰ
����ĺ�LIBCOMMON_DEV֮�ڵ�ֻ�ṩ�����������ʱ���õ��ĸ�������
*/



//#ifdef LIBCOMMON_DEV
#define MAXLINE	4096	

//#endif /*LIBCOMMON_DEV*/



#endif
