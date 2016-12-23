#ifndef __TYPECOMM_H__
#define __TYPECOMM_H__

typedef long	LONGINT;
typedef int		POINTER;

#if defined(OS_AIX)|| defined(OS_LINUX)||defined(OS_HP)
typedef unsigned short uint16;
typedef unsigned int   uint32;
#endif


#ifndef NOTFOUND
#define NOTFOUND 0
#endif
#ifndef FOUND 
#define FOUND 1
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE 
#define FALSE 0 
#endif
#ifndef SUCCESS
#define SUCCESS 1
#endif
#ifndef FAIL
#define FAIL 0
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif


#endif
