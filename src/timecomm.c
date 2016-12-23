/*һЩ��ʱ����ص��ճ����躯��
 *��jinjm��2005��10������
 *һ�������ַ���"yyyymmddhh24miss"��ʾ��ʱ�䷵��UTC����
 *		1.GetTimet��		ʱ���ַ�����������յȲ���һ��Ҫ������ճ�����Ҫ����ϵͳ����mktime������Ϸ���;
 *		2.GetTimetValid��	ʱ���ַ�����������յȳɷ�һ��Ҫ�����ճ�����Ҫ�󣬷��򷵻�-1;
 *		3.GetWeek:			ȡ��ָ��ʱ�䴮��һ������ڣ�0~6�ֱ��������һ~��
 *		4.GetMonthDays:		ȡ��ָ��ʱ�䴮����һ���µ�������
 *�����͵�ǰʱ���йصĺ���
 *		1.GetCurTimeStr��		ͨ���������غ͵�ǰʱ���Ӧ��ʱ�䴮����ʽ"yyyymmddhh24miss";
 *		2.GetCurTimeStrCh��		ͨ���������غ͵�ǰʱ���Ӧ������ʱ�䴮����ʽ"yyyy��mm��dd��hh24ʱmi��ss��";GetCurTimeStrCh
 *		3.GetCurTimeStrExt��	ͨ���������غ͵�ǰʱ���Ӧ��ʱ�䴮����ʽ�ɶ���;
 *		4.ReturnsCurTimeStr��	ֱ��return�͵�ǰʱ���Ӧ��ʱ�䴮��ָ�룬��ʽ"yyyymmddhh24miss";
 *		5.ReturnsCurTimeStrCh��	ֱ��return�͵�ǰʱ���Ӧ������ʱ�䴮��ָ�룬��ʽ"yyyy��mm��dd��hh24ʱmi��ss��";
 *		6.ReturnsCurTimeStrExt��ֱ��return�͵�ǰʱ���Ӧ��ʱ�䴮��ָ�룬��ʽ�ɶ���;
 *����ָ��time_tֵ�ĺ���
 *		1.GetTimetStr:
 *		2.GetTimetStrCh:
 *		3.GetTimetStrExt:
 *		4.ReturnTimetStr:
 *		5.ReturnTimetStrCh:
 *		6.ReturnTimetStrExt:
 *�ġ����ڲ���
 *
 *
 *
 *
 */

#define _XOPEN_SOURCE       /* See feature_test_macros(7) */
#include <time.h>

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include "timecomm.h"
static int giDateOrig=7330;
//static int giYearOrgCN=1990;
static int gaiDateTable[]={
	0x5f52/*24402*/,0xe92 /*3730*/, 0xd26 /*3366*/, 0x352e/*13614*/,0xa57 /*2647*/, 
	0x8ad6/*35542*/,0x35a /*858*/,  0x6d5 /*1749*/, 0x5b69/*23401*/,0x749 /*1865*/,
	0x693 /*1683*/, 0x4a9b/*19099*/,0x52b /*1323*/, 0xa5b /*2651*/, 0x2aae/*10926*/,
	0x56a /*1386*/, 0x7dd5/*32213*/,0xba4 /*2980*/, 0xb49 /*2889*/, 0x5d53/*23891*/,
	0xa95 /*2709*/, 0x52d /*1325*/, 0x455d/*17757*/,0xab5 /*2741*/, 0x9baa/*39850*/,
	0x5d2 /*1490*/, 0xda5 /*3493*/, 0xeeaa/*61098*/,0xd4a /*3402*/, 0xc95 /*3221*/,
	0x4a9e/*19102*/,0x556 /*1366*/, 0xad5 /*2773*/, 0x2ada/*10970*/,0x6d2 /*1746*/,
	0x6765/*26469*/,0x725 /*1829*/, 0x64b /*1611*/, 0x5657/*22103*/,0xcab /*3243*/,
	0x55a /*1370*/, 0x356e/*13678*/,0xb56 /*2902*/, 0xbf52/*48978*/,0xb52 /*2898*/,
	0xb25 /*2853*/, 0xed2b/*60715*/,0xa4b /*2635*/, 0x4ab /*1195*/, 0x52bb/*21179*/,
	0x5ad /*1453*/, 0xb6a /*2922*/, 0x2daa/*11690*/,0xd92 /*3474*/, 0x7ea5/*32421*/,
	0xd25 /*3365*/, 0xa55 /*2645*/, 0xda5d/*55901*/,0x4b6 /*1206*/, 0x5b5 /*1461*/,
	0x36d6/*14038*/};

char *SolarDay2LunarDay(char *solarDay)
{
	static char sResult[40] = "\0";
	int iResult;
	int iDayLeave, iLoop, iLoopMonth;

	int bigSmallDist, leap, leapShift, count;
	iDayLeave = GetTimet(solarDay) / (24 * 3600) - giDateOrig;

	if(iDayLeave < 0 ||iDayLeave > 22295 )
		return NULL;

	for(iLoop = 0; iLoop < sizeof(gaiDateTable) / sizeof(gaiDateTable[0]); iLoop++)
	{
		bigSmallDist = gaiDateTable[iLoop];
		leap = bigSmallDist >> 12;
		if(leap > 12)
		{
			leap &= 7;
			leapShift = 1;
		}
		else
			leapShift=0;

		for(iLoopMonth = 1; iLoopMonth <= 12; iLoopMonth++)
		{
			count = (bigSmallDist & 1) + 29;
			if(iLoopMonth == leap)
					count -= leapShift;
			if(iDayLeave < count)	
			{
				iResult = (iLoop << 9) + (iLoopMonth << 5) + iDayLeave + 1;
				sprintf(sResult, "%04d%02d%02d%d", (iResult >> 9) & 0xFFF, (iResult >> 5) & 0xF, iResult & 0x1F, (iResult >> 21) & 0x1);
				return sResult;
			}
			iDayLeave -= count;
			if(iLoopMonth == leap)
			{
				count = 29 + leapShift;
				if(iDayLeave < count)
				{
					iResult = (iLoop << 9) + (iLoopMonth << 5) + iDayLeave + 1 + (1 << 21); 
					sprintf(sResult, "%04d%02d%02d%d", (iResult >> 9) & 0xFFF, (iResult >> 5) & 0xF, iResult & 0x1F, (iResult >> 21) & 0x1);
					return sResult;
				}	
				iDayLeave -= count;
			}
			bigSmallDist >>= 1;
		}
	}

	return NULL;
}

/*
���������ر�ʾָ��ʱ���time_tֵ
������sAnswerTime	ָ��ʱ���ַ��������ϸ�ʽ"yyyymmddhh24miss"
���أ����غ�sAnswerTime��Ӧ�ı�ʾʱ���time_tֵ
*/
time_t GetTimet(char *sInTimes)
{
	char sTempYear[5],sTempMon[3],sTempMDay[3],
		 sTempHour[3],sTempMin[3],sTempSec[3];
	time_t tt;
	struct tm tm;

	strncpy(sTempYear,sInTimes,4);sTempYear[4]=0;
	sTempMon[0]=sInTimes[4];
	sTempMon[1]=sInTimes[5];sTempMon[2]=0;
	sTempMDay[0]=sInTimes[6]; 
	sTempMDay[1]=sInTimes[7]; sTempMDay[2]=0;
	sTempHour[0]=sInTimes[8];
	sTempHour[1]=sInTimes[9]; sTempHour[2]=0;
	sTempMin[0]=sInTimes[10];
	sTempMin[1]=sInTimes[11]; sTempMin[2]=0;
	sTempSec[0]=sInTimes[12];
	sTempSec[1]=sInTimes[13]; sTempSec[2]=0;
	
	tm.tm_year=atoi(sTempYear)-1900;
	tm.tm_mon=atoi(sTempMon)-1;
	tm.tm_mday=atoi(sTempMDay);
	tm.tm_hour=atoi(sTempHour);
	tm.tm_min=atoi(sTempMin);
	tm.tm_sec=atoi(sTempSec);
	tm.tm_isdst=0;
	/*����ע��,�ڴ�������������2038���Ļ���������,��Ϊ1970��2038֮�������
	�Ѿ�����һ��int�͵ı�ʾ��Χ*/
	tt=mktime(&tm);
	/*printf("tt:%d:%d��%d��%d��%dʱ%d��%d��\n",tt,tm.tm_year,tm.tm_mon,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);*/
	return tt;
}
time_t GetTimetExt(char *sInTimes, char*sFormat)
{
	struct tm tm;
	char *pch = NULL;
	memset(&tm,0,sizeof(tm));
	pch = (char*)strptime(sInTimes, sFormat, &tm);
	if( pch == NULL)
		return -1;

	return mktime(&tm);
}


/********************************************
 ����ͬGetTimet���������ʱ��ɷֵĺϷ����жϡ�
 *******************************************/
time_t GetTimetValid(char *sInTimes)
{
	char sTempYear[5],sTempMon[3],sTempMDay[3],sTempHour[3],
	     sTempMin[3],sTempSec[3];
	struct tm tm;
	time_t t_ret;
	
	bzero(&tm,sizeof(struct tm));
	
	strncpy(sTempYear,sInTimes,4);sTempYear[4]=0;
	sTempMon[0]=sInTimes[4];
	sTempMon[1]=sInTimes[5]; sTempMon[2]=0;
	sTempMDay[0]=sInTimes[6];
	sTempMDay[1]=sInTimes[7]; sTempMDay[2]=0;
	sTempHour[0]=sInTimes[8];
	sTempHour[1]=sInTimes[9]; sTempHour[2]=0;
	sTempMin[0]=sInTimes[10];
	sTempMin[1]=sInTimes[11]; sTempMin[2]=0;
	sTempSec[0]=sInTimes[12];
	sTempSec[1]=sInTimes[13]; sTempSec[2]=0;
		
	tm.tm_year=atoi(sTempYear)-1900;
	if(tm.tm_year<=0)	return -1;
	
	tm.tm_mon=atoi(sTempMon)-1;
	if(tm.tm_mon<0||tm.tm_mon>11) return -1;
	
	tm.tm_mday=atoi(sTempMDay);
	if(tm.tm_mday<0||tm.tm_mday>31) return -1;
	
	tm.tm_hour=atoi(sTempHour);
	if(tm.tm_hour<0||tm.tm_hour>23) return -1;
	
	tm.tm_min=atoi(sTempMin);
	if(tm.tm_min<0||tm.tm_min>59) return -1;
	
	tm.tm_sec=atoi(sTempSec);
	if(tm.tm_sec<0||tm.tm_sec>59) return -1;
	
	tm.tm_isdst=0;
	t_ret=mktime(&tm);
	
	if(t_ret==-1) return -1;
	
	return t_ret;
}
/*****************************************************************************
ȡ��ϵͳ��ǰʱ����ַ�����ʾ��"yyyymmddhh24miss"
*****************************************************************************/
void GetCurTimeStr(char *sOutTimes) 
{
	time_t timet;
	struct tm *tm;
	
	timet = time(NULL);
	
	tm = localtime(&timet);
	sprintf(sOutTimes,"%.4d%.2d%.2d%.2d%.2d%.2d",1900+tm->tm_year,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
}
void GetCurTimeStrCh(char *sOutTimes) 
{
	time_t timet;
	struct tm *tm;
	
	timet = time(NULL);
	
	tm = localtime(&timet);
	sprintf(sOutTimes,"%.4d��%.2d��%.2d��%.2dʱ%.2d��%.2d��",1900+tm->tm_year,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
}
/*****************************************************************************
ȡ��ϵͳ��ǰʱ����ַ�����ʾ��ʱ���ʽ�ɶ��ƣ����Ʋ�����ϵͳ����strftime
*****************************************************************************/
void GetCurTimeStrExt(char *sOutTimes,char *sFormat) 
{
	time_t timet;
	struct tm *tm;
	int ret;
	char sTmp[40]="";
	
	timet= time(NULL);
	tm = localtime(&timet);
	if(sFormat==NULL||strlen(sFormat)==0)
		ret=strftime(sTmp,sizeof(sTmp),"%Y%m%d%H%M%S",tm);
	else
		ret=strftime(sTmp,sizeof(sTmp),sFormat,tm);
	if(ret<=0)
		strcpy(sOutTimes,"00000000000000");
	else
		strcpy(sOutTimes,sTmp);
}

/*****************************************************************************
����ϵͳ��ǰʱ����ַ���ָ�롣"yyyymmddhh24miss"
*****************************************************************************/

char * ReturnCurTimeStr()
{
	static char sTime[32];
	struct tm * tm;
	time_t  tTime;
	
	tTime = time(NULL);
	
	daylight =0 ;
	tzset();
	
	tm = localtime(&tTime);
	
	sprintf(sTime,"%04d%02d%02d%02d%02d%02d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,
											 tm->tm_hour,tm->tm_min,tm->tm_sec);
	return sTime;	
}
char * ReturnCurTimeStrCh()
{
	static char sTime[64];
	struct tm * tm;
	time_t  tTime;
	
	tTime = time(NULL);
	
	daylight =0 ;
	tzset();
	
	tm = localtime(&tTime);
	
	sprintf(sTime,"%04d��%02d��%02d��%02dʱ%02d��%02d��",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,
											 tm->tm_hour,tm->tm_min,tm->tm_sec);
	return sTime;	
}
/*****************************************************************************
����ϵͳ��ǰʱ����ַ���ָ�롣ʱ���ʽ�ɶ��ƣ����Ʋ�����ϵͳ����strftime
*****************************************************************************/
char * ReturnCurTimeStrExt(char *sFormat)
{
	static char sTime[64]="";
	int ret;
	struct tm * tm;
	time_t  tTime;
	
	tTime = time(NULL);
	
	daylight =0 ;
	tzset();
	
	tm = localtime(&tTime);
	if(sFormat==NULL||strlen(sFormat)==0)
		ret=strftime(sTime,sizeof(sTime),"%Y%m%d%H%M%S",tm);
	else
		ret=strftime(sTime,sizeof(sTime),sFormat,tm);
	
	return ret?sTime:NULL;	
}

/*
 *  ȡ�ú�ttime��Ӧ��ʱ���ַ���������-1����ʱ���ַ�����ֵ�޶���
 *  ʱ���ַ�����ʽ"yyyymmddhh24miss"
 */
int GetTimetStr(char *sTime,time_t ttime)
{
	struct tm *tm;
	
	if((tm = (struct tm*)localtime(&ttime)) == NULL) 
		return -1;
		
	if(tm->tm_isdst == 1)
	{
		ttime -= 3600;
		if((tm = (struct tm *)localtime(&ttime)) == NULL)
			return -1;
	}
	
	if(strftime(sTime,15,"%Y%m%d%H%M%S",tm) == (size_t)0)
		return -1;
	
	return 0;
}
/*
 *��������GetTimetStr�������д�����'��'��'��'��'��'��
 *
 */
int GetTimetStrCh(time_t ttime,char * sOut)
{
	struct tm *tm;

	if((tm = (struct tm*)localtime(&ttime)) == NULL) 
		return -1;
		
	if(tm->tm_isdst == 1)
	{
		ttime -= 3600;
		if((tm = (struct tm *)localtime(&ttime)) == NULL)
			return -1;
	}
	
	sprintf(sOut,"%04d��%02d��%02d��%02dʱ%02d��%02d��",tm->tm_year+1900,tm->tm_mon+1,
		tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	return 0;
	
}
/*
 *  ȡ�ú�ttime��Ӧ��ʱ���ַ���������-1����ʱ���ַ�����ֵ�޶���
 *  ʱ���ַ�����ʽ�ɶ��ƣ����Ʋ�����ϵͳ����strftime
 */
int GetTimetStrExt(char *sTime,time_t ttime,char *sFormat)
{
	char sTmp[48]="";
	struct tm *tm;
	
	if((tm = (struct tm*)localtime(&ttime)) == NULL) 
		return -1;
		
	if(tm->tm_isdst == 1)
	{
		ttime -= 3600;
		if((tm = (struct tm *)localtime(&ttime)) == NULL)
			return -1;
	}
	
	if(strftime(sTmp,sizeof(sTmp)-1,sFormat,tm) == (size_t)0)
		return -1;
	strcpy(sTime,sTmp);
	return 0;
}

char * ReturnTimetStr( time_t ttime)
{
	struct tm *tm;
	static char sTime[32];
	memset(sTime,0,sizeof(sTime));
	if((tm = (struct tm*)localtime(&ttime)) == NULL) 
		return (char*)0;
		
	if(tm->tm_isdst == 1)
	{
		ttime -= 3600;
		if((tm = (struct tm *)localtime(&ttime)) == NULL)
			return (char*)0;
	}
	
	if(strftime(sTime,15,"%Y%m%d%H%M%S",tm) == (size_t)0)
		return (char*)0;
	
	return sTime;
}
char *ReturnTimetStrCh(time_t ttime)
{
	struct tm *tm;
	static char sTime[32]="";
	if((tm = (struct tm*)localtime(&ttime)) == NULL) 
		return NULL;
		
	if(tm->tm_isdst == 1)
	{
		ttime -= 3600;
		if((tm = (struct tm *)localtime(&ttime)) == NULL)
			return NULL;
	}
	
	sprintf(sTime,"%04d��%02d��%02d��%02dʱ%02d��%02d��",tm->tm_year+1900,tm->tm_mon+1,
		tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	
	return sTime;
	
}
char * ReturnTimetStrExt( time_t ttime,char *sFormat)
{
	struct tm *tm;
	static char sTime[32];
	memset(sTime,0,sizeof(sTime));
	if((tm = (struct tm*)localtime(&ttime)) == NULL) 
		return (char*)0;
		
	if(tm->tm_isdst == 1)
	{
		ttime -= 3600;
		if((tm = (struct tm *)localtime(&ttime)) == NULL)
			return (char*)0;
	}
	
	if(strftime(sTime,sizeof(sTime),sFormat,tm) == (size_t)0)
		return (char*)0;
	
	return sTime;
}
/*
 *���ش������ڵ����֣�1��������һ��2�������ڶ��������ƣ�0����������
 */
int GetWeek(char *sDate)
{
    struct tm tms;
    char sTempStr[5],*pcCurPos;

    strncpy(sTempStr,sDate,4);
    sTempStr[4]='\0';
    tms.tm_year=atoi(sTempStr)-1900;

    pcCurPos=sDate+4;
    strncpy(sTempStr,pcCurPos,2);
    sTempStr[2]='\0';
    tms.tm_mon=atoi(sTempStr)-1;

    pcCurPos=sDate+6;
    strncpy(sTempStr,pcCurPos,2);
    sTempStr[2]='\0';
    tms.tm_mday=atoi(sTempStr);

    tms.tm_hour=1;
    tms.tm_min=0;
    tms.tm_sec=0;
    tms.tm_isdst=0;
    if(mktime(&tms)==-1) return -1;
    return tms.tm_wday;
}
int DiffSeconds(char *sBeginDate,char *sEndDate)
{
	time_t tEndDate,tBeginDate;
	tEndDate   = GetTimet(sEndDate);	
	tBeginDate = GetTimet(sBeginDate);
	return tEndDate-tBeginDate;
}
/* ���� sEndDate - sBeginDate��ʵ������*/
int DiffDays(char *sBeginDate,char *sEndDate)
{
	time_t tEndDate,tBeginDate;
	tEndDate   = GetTimet(sEndDate);	
	tBeginDate = GetTimet(sBeginDate);	

	return (int)difftime(tEndDate,tBeginDate)/(24*60*60)+1;
}
int DiffMonths(char *sBeginDate,char *sEndDate)
{
	char tmp1[5]="";
	char tmp2[5]="";
	int iYear1=0,iYear2=0;
	int iMonth1=0,iMonth2=0;
	strncpy(tmp1,sEndDate,4);
	strncpy(tmp2,sBeginDate,4);
	tmp1[4]=0;
	tmp2[4]=0;
	iYear1=atoi(tmp1);
	iYear2=atoi(tmp2);
	strncpy(tmp1,sEndDate+4,2);
	strncpy(tmp2,sBeginDate+4,2);
	tmp1[2]=0;
	tmp2[2]=0;
	iMonth1=atoi(tmp1);
	iMonth2=atoi(tmp2);
	return (iYear1-iYear2)*12+(iMonth1-iMonth2);
	
}
/*
 *���ڼ�һ��ϵ��
 */
void AddDaysModified(char *sDate,int day)
{
	char sTmpDate[15]="";
	time_t time;
	if(strlen(sDate)<14)
	{
		strncpy(sTmpDate,sDate,8);
		strcpy(sTmpDate+8,"000000");
	}
	else
		strncpy(sTmpDate,sDate,14);
	sTmpDate[14]=0;
	time=GetTimet(sTmpDate);
	time+=day*24*60*60;
	GetTimetStr(sDate,time);
}

int AddMonth(char *sDate,int iMonthCount)
{
	int iTotalDay, iLoop;
	int iSourceMonthDay=0,iTargetMonthDay=0;
	int iMonth,iDay,iYear;
	char sTmp[32];
	
	time_t tDate=GetTimet(sDate);
	strncpy(sTmp,sDate,4);sTmp[4]=0;iYear=atoi(sTmp);
	strncpy(sTmp,sDate+4,2);sTmp[2]=0;iMonth=atoi(sTmp);
	strncpy(sTmp,sDate+6,2);sTmp[2]=0;iDay=atoi(sTmp);
	iTotalDay=0;
	iSourceMonthDay=GetMonthDays(sDate);
	
	if(iMonthCount==0)
		return tDate;
	else if(iMonthCount>0)
	{
		strcpy(sTmp,sDate);
		/*for(iLoop=0;iLoop<=iMonthCount;iLoop++)*/
		for(iLoop=1;iLoop<iMonthCount;iLoop++)
		{
			sprintf(sTmp,"%04d%02d",iYear,iMonth+iLoop);
			iTargetMonthDay=GetMonthDays(sTmp);
			if(iLoop==iMonthCount)
				break;
			iTotalDay+=iTargetMonthDay;	
		}
		
		iTotalDay=iTotalDay-(iSourceMonthDay-iTargetMonthDay);
		iTotalDay+=(iSourceMonthDay-iDay);
		return tDate+iTotalDay*24*3600;
	}
	else
	{
		strcpy(sTmp,sDate);
		/*for(iLoop=0;iLoop>iMonthCount;iLoop--)*/
		for(iLoop=-1;iLoop>iMonthCount;iLoop--)
		{
			sprintf(sTmp,"%04d%02d",iYear,iMonth+iLoop);
			iTargetMonthDay=GetMonthDays(sTmp);
			
			iTotalDay+=iTargetMonthDay;
			
		}
		iTotalDay=iTotalDay-(iSourceMonthDay-iTargetMonthDay);
		iTotalDay+=(iSourceMonthDay-iDay);
		return tDate-iTotalDay*24*3600;
	}
	printf("Target:%d\nSource:%d\n",iTargetMonthDay,iSourceMonthDay);
	
		
	
	
}

void AddDays(char *sOut,char *sDate,int day)
{
	char sTmpDate[15]="";
	time_t time;
	if(strlen(sDate)<14)
	{
		strncpy(sTmpDate,sDate,8);
		strcpy(sTmpDate+8,"000000");
	}
	else
		strncpy(sTmpDate,sDate,14);
	sTmpDate[14]=0;
	time=GetTimet(sTmpDate);
	time+=day*24*60*60;
	GetTimetStr(sOut,time);
}
char * ReturnAddDays(char *sDate,int day)
{
	char sTmpDate[15]="";
	time_t time;
	if(strlen(sDate)<14)
	{
		strncpy(sTmpDate,sDate,8);
		strcpy(sTmpDate+8,"000000");
	}
	else
		strncpy(sTmpDate,sDate,14);
	sTmpDate[14]=0;
	time=GetTimet(sTmpDate);
	time+=day*24*60*60;
	return ReturnTimetStr(time);
}
/*
 *ʱ���һ��ϵ��
 */
void AddSeconds(char *sOut,char *sDate,int iSecond)
{
	time_t time;
	time=GetTimet(sDate);
	GetTimetStr(sOut,time+iSecond);
}
void AddSecondsModified(char *sDate,int iSecond)
{
	time_t time;
	time=GetTimet(sDate);
	GetTimetStr(sDate,time+iSecond);
}
char *ReturnAddSeconds(char *sDate,int iSecond)
{
	time_t time;
	time=GetTimet(sDate);
	return ReturnTimetStr(time+iSecond);
}
/*
 *���ʱ��sCheckedTime�ǲ�����ʱ���sBeginDate~sEndDate֮�䣬������������
 *������Ƿ�sBeginDate<=sCheckedTime<=sEndDate
 */
int IsDateBetween(char *sCheckedDate,char *sBeginDate,char *sEndDate)
{
	time_t beginTimet,endTimet,checkedTimet;
	
	beginTimet	=GetTimet(sBeginDate);
	endTimet	=GetTimet(sEndDate);
	checkedTimet=GetTimet(sCheckedDate);
	if(beginTimet<=checkedTimet&&checkedTimet<=endTimet)
		return TRUE;
	return FALSE;
}
int IsDateBetweenExt(char *sCheckedDate,char *sBeginDate,char *sEndDate,int includeMode)
{
	time_t beginTimet,endTimet,checkedTimet;
	
	beginTimet	=GetTimet(sBeginDate);
	endTimet	=GetTimet(sEndDate);
	checkedTimet=GetTimet(sCheckedDate);
	
	switch(includeMode)
	{
		case INCM_GeLe:
			if(beginTimet<=checkedTimet&&checkedTimet<=endTimet)
				return TRUE;
			break;
		case INCM_GtLt:
			if(beginTimet<checkedTimet&&checkedTimet<endTimet)
				return TRUE;
			break;
		case INCM_GeLt:
			if(beginTimet<=checkedTimet&&checkedTimet<endTimet)
				return TRUE;
			break;
		case INCM_GtLe:
			if(beginTimet<checkedTimet&&checkedTimet<=endTimet)
				return TRUE;
			break;
		default:
			if(beginTimet<=checkedTimet&&checkedTimet<=endTimet)
				return TRUE;
			break;
	}
	return FALSE;
}

int IsValidDate(char *sCheckedDate,char *sBeginDate,char *sEndDate)
{
	return IsDateBetween(sCheckedDate,sBeginDate,sEndDate);	
}
int IsValidDateExt(char *sCheckedDate,char *sBeginDate,char *sEndDate,int includeMode,int datePreMode)
{
	char sBeginDateTmp[15]="",sEndDateTmp[15]="",sCheckedDateTmp[15]="";
	switch(datePreMode)
	{
		case DPM_HOUR:
			strncpy(sBeginDateTmp,sBeginDate,10);sBeginDateTmp[10]=0;
			strncat(sBeginDateTmp,"0000",4);sBeginDateTmp[14]=0;
			
			strncpy(sEndDateTmp  ,sEndDate,10);sEndDateTmp[10]  =0;
			strncat(sEndDateTmp  ,"0000",4);sEndDateTmp[14]  =0;
			
			strncpy(sCheckedDateTmp,sCheckedDate,10);sCheckedDateTmp[10]=0;
			strncat(sCheckedDateTmp,"0000",4);sCheckedDateTmp[14]=0;
			break;
		case DPM_MIN:
			strncpy(sBeginDateTmp,sBeginDate,12);sBeginDateTmp[12]=0;
			strncat(sBeginDateTmp,"00",2);sBeginDateTmp[14]=0;
			
			strncpy(sEndDateTmp  ,sEndDate,12);sEndDateTmp[12]  =0;
			strncat(sEndDateTmp  ,"00",2);sEndDateTmp[14]  =0;
			
			strncpy(sCheckedDateTmp,sCheckedDate,12);sCheckedDateTmp[12]=0;
			strncat(sCheckedDateTmp,"00",2);sCheckedDateTmp[14]=0;
			break;
		case DPM_SEC:
			strncpy(sBeginDateTmp,sBeginDate,14);sBeginDateTmp[14]=0;
			
			strncpy(sEndDateTmp  ,sEndDate,14);sEndDateTmp[14]  =0;
			
			strncpy(sCheckedDateTmp,sCheckedDate,14);sCheckedDateTmp[14]=0;
			break;
		case DPM_DAY:
		default:
			strncpy(sBeginDateTmp,sBeginDate,8);sBeginDateTmp[8]=0;
			strncat(sBeginDateTmp,"000000",6);sBeginDateTmp[14]=0;
			
			strncpy(sEndDateTmp  ,sEndDate,8);sEndDateTmp[8]  =0;
			strncat(sEndDateTmp  ,"000000",6);sEndDateTmp[14]  =0;
			
			strncpy(sCheckedDateTmp,sCheckedDate,8);sCheckedDateTmp[8]=0;
			strncat(sCheckedDateTmp,"000000",6);sCheckedDateTmp[14]=0;
			break;	
		
	}
	return IsDateBetweenExt(sCheckedDateTmp,sBeginDateTmp,sEndDateTmp,includeMode);
	
}
/*�ж��Ƿ�����ȷ��ʱ���ַ���
 *	1�����ʱ�䳤��С����4λ��ʱ���ַ�������Ϊ�������򷵻�false
 *	2�����ʱ���ַ�������0~9������ַ����򷵻�false
 */
int IsDate(char *sDate)
{
	int i=0,len=0;
	len=strlen(sDate);
	if(len%2||len<4)
		return FALSE;
	for(i=0;i<len;i++)
	{
		if(sDate[i]<'0'||sDate[i]>'9')
			break;
	}
	if(i!=len)
		return FALSE;
	if(strncmp(sDate,"1900",4)<0||strncmp(sDate,"2999",4)>0)/*1999->1900,2020->2999*/
		return FALSE;
	if(strncmp(sDate+4,"01",2)<0||strncmp(sDate+4,"12",2)>0)
		return FALSE;
	if(strncmp(sDate+6,"01",2)<0||strncmp(sDate+6,"31",2)>0)
		return FALSE;
	return TRUE;	
}

/*�ж��Ƿ�����ȷ�������ַ���(Ŀǰֻ֧��8λ�������ж�)
 *	1�����ʱ�䳤�Ȳ�����8λ���򷵻�false
 *	2�����ʱ���ַ�������0~9������ַ����򷵻�false
 */
const int Months[]={31,28,31,30,31,30,31,31,30,31,30,31};   
int IsDateExt(const char *sDate)   
{   
  int i=0,len=0;
  char   sTempYear[5],sTempMon[3],sTempMDay[3];   
  int   iyear,imon,iday;   
  
  len=strlen(sDate);
  if(len!=8)  
    return  FALSE;   
  for(i=0;i<len;i++){
	if(sDate[i]<'0'||sDate[i]>'9')
	  break;
  }
  if(i!=len)
	 return FALSE;
    
  strncpy(sTempYear,sDate,4);   
  sTempYear[4]   =   0;   
    
  sTempMon[0]   =   sDate[4];   
  sTempMon[1]   =   sDate[5];   
  sTempMon[2]   =   0;   
    
  sTempMDay[0]   =   sDate[6];   
  sTempMDay[1]   =   sDate[7];   
  sTempMDay[2]   =   0;   
    
  iyear   =   atoi(sTempYear);   
  imon   =   atoi(sTempMon);   
  iday   =   atoi(sTempMDay);   
            
  if(iyear   ==   0){   
  	return   FALSE;   
  }
  if(imon<1||imon>12){   
  	return   FALSE;   
  }
  if( ((iyear%4==0 && iyear%100!=0)||iyear%400==0) && (imon==2) ){   
		if(iday<=0||iday>29){   
	  	return  FALSE;   
		}   
  }else  if(iday<=0 || iday>Months[imon-1]){   
  	return  FALSE;
  }
  return TRUE;   
}

/*CHENPX ADD 200903*/
char* AddMonths(char *sDate,int iMonthCount)
{
	static char sTemp[9];
	int iYear =0 ,iMonth=0,iDay=0,i=0;
	
	strncpy(sTemp,sDate,6);
	sTemp[6]=0;
	
	iYear=atoi(sTemp)/100;
	iMonth=atoi(sTemp+4);
	
	if(sDate[6]!=0)
		/*iDay=atoi(sTemp+6);  qiusy 20091217*/
		iDay=atoi(sDate+6);
	
	if(iMonthCount==0){
		return sDate;/*return sTemp; qiusy 20091217*/
	}else if(iMonthCount>0){
		for(i=0;i<iMonthCount;i++){
			if(++iMonth==13){
				iYear++;
				iMonth=1;
			}
		}
	}else{
		for(i=0;i>iMonthCount;i--){
			if(--iMonth==0){
				iYear--;
				iMonth=12;
			}
		}
	}
	
	if(iDay>0){
		if(iMonth==2&&((iYear%4==0 && iYear%100!=0)||iYear%400==0)){
			if(iDay>29) iDay=29;
		}else if (iMonth==2){ /*������2��28�� qiusy 20091229*/
			if(iDay>28) iDay=28;
		}else if(Months[iMonth-1]<iDay){
			iDay=Months[iMonth-1];
		}		
		sprintf(sTemp,"%04d%02d%02d",iYear,iMonth,iDay);
		sTemp[8]=0;		
	}else{
		sprintf(sTemp,"%04d%02d",iYear,iMonth);
		sTemp[6]=0;
	}
	
	return sTemp;
}  

/*
 *��������������ʱ��εĽ���
 *����н���������TRUE�����򷵻�FALSE
 *
 */
int GetTimeIntersection(char sBeginTime1[15],char sEndTime1[15],
                  		char sBeginTime2[15],char sEndTime2[15],
                  		char sBeginTime[15],char sEndTime[15])
{
	time_t beginTime1,endTime1;
	time_t beginTime2,endTime2;
	time_t timetTmp;
	beginTime1=GetTimet(sBeginTime1);
	endTime1=GetTimet(sEndTime1);
	if(beginTime1>endTime1)
	{
		timetTmp=beginTime1;
		beginTime1=endTime1;
		endTime1=timetTmp;	
	}
	
	beginTime2=GetTimet(sBeginTime2);
	endTime2=GetTimet(sEndTime2);
	if(beginTime2>endTime2)
	{
		timetTmp=beginTime2;
		beginTime2=endTime2;
		endTime2=timetTmp;	
	}
	
	if(beginTime1>endTime2||beginTime2>endTime1||
	   beginTime1==endTime1||beginTime2==endTime2)
		return FALSE;
	
	if(sBeginTime!=NULL)
	{
		if(beginTime1<beginTime2)
			GetTimetStr(sBeginTime,beginTime2);
		else
			GetTimetStr(sBeginTime,beginTime1);
	}
	if(sEndTime!=NULL)
	{
		if(endTime1<endTime2)
			GetTimetStr(sEndTime,endTime1);
		else
			GetTimetStr(sEndTime,endTime2);
	}
	return TRUE;
	
}
/*
 *��������������ʱ��εĲ���
 *����н���������TRUE�����򷵻�FALSE
 *
 */
int GetTimeUnion(char sBeginTime1[15],char sEndTime1[15],
                 char sBeginTime2[15],char sEndTime2[15],
                 char sBeginTime[15],char sEndTime[15])
{
	time_t beginTime1,endTime1;
	time_t beginTime2,endTime2;
	time_t timetTmp;
	beginTime1=GetTimet(sBeginTime1);
	endTime1=GetTimet(sEndTime1);
	if(beginTime1>endTime1)
	{
		timetTmp=beginTime1;
		beginTime1=endTime1;
		endTime1=timetTmp;	
	}
	
	beginTime2=GetTimet(sBeginTime2);
	endTime2=GetTimet(sEndTime2);
	if(beginTime2>endTime2)
	{
		timetTmp=beginTime2;
		beginTime2=endTime2;
		endTime2=timetTmp;	
	}
	
	if(beginTime1>endTime2||beginTime2>endTime1)
		return FALSE;

	if(sBeginTime!=NULL)
	{
		if(beginTime1<beginTime2)
			GetTimetStr(sBeginTime,beginTime1);
		else
			GetTimetStr(sBeginTime,beginTime2);
	}
	if(sEndTime!=NULL)
	{
		if(endTime1<endTime2)
			GetTimetStr(sEndTime,endTime2);
		else
			GetTimetStr(sEndTime,endTime1);
	}
	return TRUE;
	
}
int GetMonthDays(char *sDate)
{
	char sTmpDate[15]="";
	char sYear[5]="";
	char sMonth[3]="";
	strncpy(sYear,sDate,4);sYear[4]=0;
	strncpy(sMonth,sDate+4,2);sMonth[2]=0;
	sprintf(sTmpDate,"%04d%02d01",atoi(sYear),atoi(sMonth)+1);
	AddDaysModified(sTmpDate,-1);
	strncpy(sMonth,sTmpDate+6,2);
	sMonth[2]=0;
	return atoi(sMonth);
}


/*��С�ȴ�5��*/
void WaitSecond(int iSecond)
{
	time_t tDeadTime,tCurTime;

	if(iSecond<5) iSecond=5;

	//cmsprintf(0,0,"�ȴ��¼�,����˯��״̬%d��.\n",iSecond);

	tCurTime = time(NULL);
	tDeadTime=tCurTime+iSecond;	
	while(tCurTime<tDeadTime)
	{
		sleep(1);
		tCurTime = time(NULL);
	}
}

