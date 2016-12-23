#ifndef __TIMECOMM_H__
#define __TIMECOMM_H__

#include <time.h>
#include "typecomm.h"

/*INCM=INClude Mode:日期比较极点包含模式
用于函数IsDateBetweenExt和IsValidDateExt*/
#define INCM_GeLe	0	/*begin<=checked<=end*/
#define INCM_GtLt	1	/*begin< checked< end*/
#define INCM_GeLt	2	/*begin<=checked<=end*/
#define INCM_GtLe	3	/*begin<=checked<=end*/

/*DPM=Date Precision Mode:日期比较的精度模式
用于函数IsValidDateExt*/
#define DPM_DAY		0	/*比较到天*/
#define DPM_HOUR	1	/*比较到小时*/
#define DPM_MIN		2	/*比较到分*/
#define DPM_SEC		3	/*比较到秒*/



void AddDays(char *sOut,char *sDate,int day) ;
void AddDaysModified(char *sDate,int day) ;
char * ReturnAddDays(char *sDate,int day) ;
int AddMonth(char *sDate,int iMonthCount);
void AddSeconds(char *sOut,char *sDate,int iSecond) ;
void AddSecondsModified(char *sDate,int iSecond) ;
char *ReturnAddSeconds(char *sDate,int iSecond) ;

int GetTimeIntersection(char sBeginTime1[15],char sEndTime1[15],
                  		char sBeginTime2[15],char sEndTime2[15],
                  		char sBeginTime[15],char sEndTime[15]) ;
int GetTimeUnion(char sBeginTime1[15],char sEndTime1[15],
                 char sBeginTime2[15],char sEndTime2[15],
                 char sBeginTime[15],char sEndTime[15]) ;
int DiffDays(char *sBeginDate,char *sEndDate) ;
int DiffMonths(char *sBeginDate,char *sEndDate) ;
int DiffSeconds(char *sBeginDate,char *sEndDate) ;

void GetCurTimeStr(char *sOutTimes) ;
void GetCurTimeStrCh(char *sOutTimes) ;
void GetCurTimeStrExt(char *sOutTimes,char *sFormat) ;
char * ReturnCurTimeStr() ;
char * ReturnCurTimeStrCh() ;
char * ReturnCurTimeStrExt(char *sFormat) ;

int GetTimetStr(char *sTime,time_t ttime) ;
int GetTimetStrCh(time_t ttime,char * sOut) ;
int GetTimetStrExt(char *sTime,time_t ttime,char *sFormat) ;
char * ReturnTimetStr( time_t ttime) ;
char *ReturnTimetStrCh(time_t ttime) ;
char * ReturnTimetStrExt( time_t ttime,char *sFormat) ;

time_t GetTimet(char *sInTimes) ;
time_t GetTimetValid(char *sInTimes) ;
time_t GetTimetExt(char *sInTimes,char*sFormat) ;

int GetMonthDays(char *sDate) ;
int GetWeek(char *sDate) ;

int IsDate(char *sDate) ;
int IsDateBetween(char *sCheckedDate,char *sBeginDate,char *sEndDate) ;
int IsDateBetweenExt(char *sCheckedDate,char *sBeginDate,char *sEndDate,int includeMode) ;
int IsValidDate(char *sCheckedDate,char *sBeginDate,char *sEndDate) ;
int IsValidDateExt(char *sCheckedDate,char *sBeginDate,char *sEndDate,int includeMode,int datePreMode) ;

char *SolarDay2LunarDay(char *solarDay);

void WaitSecond(int iSecond) ;

/** 200903 add **/
char* AddMonths(char *sDate,int iMonthCount);

#endif


