#include "errlog.h"
#include "timecomm.h"
#include <stdlib.h>

#define OS_LINUX
static void	do_err(int, const char *, va_list);

/* 
������ϵͳ���÷��������󣬴�ӡһ����ϢȻ�󷵻�
*/
void SysErrMsg(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	do_err(1,fmt, ap);
	va_end(ap);
	return;
}

/* 
������ϵͳ���õ����������󣬴�ӡ��Ϣ��Ȼ���˳�
*/
void SysErrExit(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	do_err(1, fmt, ap);
	va_end(ap);
	exit(1);
}

/*
������ϵͳ���õ����������󣬴�ӡ��Ϣ������coreӳ���ļ���Ȼ���˳�����
*/
void SysErrDump(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	do_err(1, fmt, ap);
	va_end(ap);
	abort();		/* dump core and terminate */

}

/* 
��������ϵͳ�����޹ص��·��������󣬴�ӡ��Ϣ��Ȼ�󷵻�
*/
void ErrMsg(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	do_err(0, fmt, ap);
	va_end(ap);
	return;
}

/* 
��������ϵͳ�����޹ص��������󣬴�ӡ��Ϣ�����˳����̡�
*/
void ErrExit(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	do_err(0, fmt, ap);
	va_end(ap);
	exit(1);
}

void ErrDump(const char *fmt, ...)
{
	va_list		ap;

	va_start(ap, fmt);
	do_err(0, fmt, ap);
	va_end(ap);
	abort();

}
/* 
��������ӡ��ϢȻ�󷵻ص����ߡ�
������errnoflag ָ���Ƿ���ϵͳ��������Ĵ���
*/
static void do_err(int errnoflag,const char *fmt, va_list ap)
{
	int		errno_save, n;
	static char	buf[MAXLINE];
	char ellipsis[60]="............................................................";

	errno_save = errno;		/* value caller might want printed */
#ifdef	HAVE_VSNPRINTF
	vsnprintf(buf, sizeof(buf), fmt, ap);	/* this is safe */
#else
	vsprintf(buf, fmt, ap);					/* this is not safe */
#endif
	n = strlen(buf);
	
	if (errnoflag)
	{
		strncat(buf+n,ellipsis,60-n);
		sprintf(buf+60, ": %s", strerror(errno_save));
	}
	strcat(buf, "\n");

	fflush(stdout);		/* in case stdout and stderr are the same */
	printf("%s",buf);
	LogMsg(MSG_MONITOR,"%s",buf);
	fflush(stderr);

	return;
}

static int __gsiLogMsgLevel__=MSG_MONITOR;
FILE *__gpLogFileNormal__;
FILE *__gpLogFileError__;


int SetMsgLevel(int newlevel)
{
	int tmp=__gsiLogMsgLevel__;
	__gsiLogMsgLevel__=newlevel;

	return tmp;
	
}


void RegisterLogMsg(char *sLogFileNor,char *sLogFileErr,int iMsgLevel)
{
	if(iMsgLevel<MSG_DEBUG||iMsgLevel>MSG_SMS)
	{
		LogMsg(0,"MsgLevel is out of range!");
		exit(1);
	}
	__gsiLogMsgLevel__=iMsgLevel;
	__gpLogFileNormal__=fopen(sLogFileNor,"a");
	__gpLogFileError__=fopen(sLogFileErr,"a");
}

void InsertSMSInterface(char *buf)
{
	return;
	
}
/*
 *MsgLevel����Ϣ�ȼ�����ȡ��ֵ��MSG_DEBUG,MSG_INFO,MSG_MONITOR,MSG_ALARM,MSG_ERROR,MSG_SMS
 *			Ҳ����ֱ��ȡ���֣�������ȡ��������ֵ
 */
void LogMsg(enum MsgLevel level,const char *fmt,...)
{
	static char	sBuf[MAXLINE];
	va_list		ap;
	/*�ȹ��˲�Ҫ���к�����������
	1���Ѿ���ʼ����Ϣ�ȼ�,���ҵ�ǰ��Ϣ�ȼ�С�����õ���Ϣ�ȼ�
	2���Ѿ���ʼ����Ϣ�ȼ�,���ҵ�ǰ��Ϣ�ȼ��������е���Ϣ�ȼ���
		��Ҳ������0����Ҫ��Ϊ�˼����ϴ��룬��Ϊ�ϴ��������ֱ�Ӵ�0�������
	3��δ��ʼ����Ϣ�ȼ������ҵ�ǰ��Ϣ�ȼ�������0,
		��Ҫ��Ϊ�˼����ϴ��룬��Ϊ�ϴ��������ֱ�Ӵ�0�Ĳ���û�н��г�ʼ���������
		����Ҳ�����Ʋ����г�ʼ���Ļ���ֻ��0һ�ֺϷ�ȡֵ��
	*/
	if((__gsiLogMsgLevel__!=-1&&level<__gsiLogMsgLevel__)||
	   (__gsiLogMsgLevel__!=-1&&(level<MSG_DEBUG||level>MSG_SMS)&&level!=0)||
	   (__gsiLogMsgLevel__==-1&&level!=0))
		return;
	
	va_start(ap, fmt);
#ifdef	HAVE_VSNPRINTF
	vsnprintf(sBuf, sizeof(sBuf), fmt, ap);
#else
	vsprintf(sBuf, fmt, ap);
#endif
	
	va_end(ap);	
	if(__gsiLogMsgLevel__!=-1)
	{
		///*��ǰ��Ϣ�ȼ�������Χ������MSG_DEBUG*/
		//if(level<MSG_DEBUG||level>MSG_SMS||level==MSG_DEBUG)
		//	printf(">%s",sBuf);
		/*��ǰ��Ϣ���ڵ������õ���Ϣ�ȼ����Ŵ�ӡ*/
		if(level>=__gsiLogMsgLevel__)
			printf(">%s",sBuf);
		/*��ǰ��Ϣ�ȼ����ڵ���MSG_MONITOR���򲻹����õ���Ϣ�ȼ�����Ҫдlog�ļ�*/
		if(level>=MSG_MONITOR&&level<=MSG_SMS)
		{
			if(level==MSG_MONITOR)
			{
				if(__gpLogFileNormal__)
				{
					if(fmt[0]!='\b')//"\b"
						fprintf(__gpLogFileNormal__,"%sN:0:",ReturnCurTimeStr());
					fprintf(__gpLogFileNormal__,"%s",sBuf);
					fflush(__gpLogFileNormal__);
				}
			}
			if(level==MSG_ALARM||level==MSG_ERROR||level==MSG_SMS)
			{
				if(__gpLogFileError__)
				{
					fprintf(__gpLogFileError__,"%sE:0:",ReturnCurTimeStr());
					fprintf(__gpLogFileError__,"%s",sBuf);
					fflush(__gpLogFileError__);
				}
			}
		}
		if(level==MSG_SMS)
			InsertSMSInterface(sBuf);
	}
	else
	{
		if((level>=MSG_DEBUG&&level<=MSG_SMS)||level==0)
			//printf(">%s\n",sBuf);
			printf(">%s",sBuf);
		
		
	}
	fflush(stdout);
	
	return;
}



/*------------- ���� ������ ���� ----------------*/
/*
** ע�����ļ���������ļ�����ס,������ó�������ִ�е���,
** ������ĳ����˳�  ������ ����
*/
int RegLockFile(char sFile[])
{
	char *pch;
	int fd,len,ret;
	char sFileName[128];
	char sBuf[256];
	struct flock lock;
	char sDefaultDir[]=
#ifdef OS_AIX
	"/var/locks/";
#elif defined(OS_LINUX)
	"/var/lock/" ;
#elif defined(OS_HP)
	"/var/spool/locks/" ;
#elif defined(OS_SOLARIS)
	"/var/lock/" ;
#endif
	
	
	pch=strchr(sFile,'/');
	
	len=strlen(sFile);

	if(len<5)/*�϶�������׺.lck*/
	{
		if(pch)/*��Ӧ���Լ�ָ��Ŀ¼*/
			sprintf(sFileName,"%s.lck",sFile);
		else/*Ĭ��Ϊ/var�µ���Ŀ¼*/
			sprintf(sFileName,"%s%s.lck",sDefaultDir,sFile);
	}
	else
	{
		if(strstr(sFile,".lck")!=NULL)
		{
			if(pch)
				strcpy(sFileName,sFile);
			else
				sprintf(sFileName,"%s%s",sDefaultDir,sFile);
		}
		else
		{
			if(pch)
				sprintf(sFileName,"%s.lck",sFile);
			else
				sprintf(sFileName,"%s%s.lck",sDefaultDir,sFile);
		}
	}
	umask(0);

	fd = open( sFileName,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR |S_IRGRP|S_IWGRP |S_IROTH|S_IWOTH);
	if(fd<0)
	{
		LogMsg(MSG_MONITOR,"%s:%s",sFileName,strerror(errno));
		LogMsg(MSG_MONITOR,"����ϵ����Ա��ʹĿ¼%s���û���д",sDefaultDir);
		exit(-1);
	}
	lock.l_type = F_WRLCK;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;
	
	if(fcntl(fd,F_SETLK,&lock) < 0)
	{
		if(errno  == EACCES || errno == EAGAIN)
			PrintFormat(E_MSG,"�Ѿ�����ͬ�Ľ�����ִ��!");
	}

	ftruncate( fd, 0 );
	sprintf(sBuf,"%s\n",ReturnCurTimeStr());
	write(fd,sBuf,strlen(sBuf));
	sprintf(sBuf,"%d\n%s\n",getpid(),sFileName);
	write(fd,sBuf,strlen(sBuf));
	
	ret = fcntl(fd,F_GETFD,0);
	
	ret |= FD_CLOEXEC;
	fcntl( fd, F_SETFD, ret );
	
	return 1;
}

/*
** ��ʽ����Ϣ���
** ����������
*/
void PrintFormat(enum en_msg en_,char *fmt,...)
{
	#ifndef __MAX_MSG_LENGTH__
	#define __MAX_MSG_LENGTH__	99
	#endif
	
	static char	sBuf[__MAX_MSG_LENGTH__];
	char sTemp[__MAX_MSG_LENGTH__];
	va_list		ap;
	int iMaxLen = 0;
	
	va_start(ap, fmt);
#ifdef	HAVE_VSNPRINTF
	vsnprintf(sBuf, sizeof(sBuf), fmt, ap);
#else
	vsprintf(sBuf, fmt, ap);
#endif
	
	va_end(ap);	
	
	iMaxLen=__MAX_MSG_LENGTH__;
	if((iMaxLen=iMaxLen-strlen(sBuf)-21)>0){
		int i=0;
		
		memset(sTemp, 0, sizeof(sTemp));
		for(i=0;i<iMaxLen;i++){
			strcat(sTemp,".");
		}
		strcat(sBuf,sTemp);
		sprintf(sBuf,"%s[%s]",sBuf,ReturnCurTimeStrExt("%Y-%m-%d %H:%M:%S"));
	}
	
	if(en_==E_MSG){
		LogMsg(MSG_ERROR,sBuf);
		exit(-1);
	}
	
	LogMsg(MSG_MONITOR,sBuf);
	
	return ;
}
/*
void PrintSqlError(int iErr,char *sSql)
{
	if(CheckSqlResult("ִ��SQL���ʱ����")<0){
		printf("---------------------------------------------------------------------\n");
		printf("SQL=%s\n",sSql);
		printf("---------------------------------------------------------------------\n");
		
		if(iErr)
			exit(-1);
	};	
	
	return;
}*/

