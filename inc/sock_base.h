#ifndef __SOCK_BASE_H__
#define __SOCK_BASE_H__

#include <sys/socket.h>
#ifndef OS_HP
#ifndef OS_LINUX
#include <sys/select.h>
#endif
#endif
#include <sys/time.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "sub_server_public.h"

#ifdef OS_HP
#define MSG_WAITALL 0x80
#endif
//#define MSG_WAITALL 0x80

#define CONN_TYPE_SHORT	0	/*¶ÌÁ¬½Ó*/
#define CONN_TYPE_LONG	1	/*³¤Á¬½Ó*/

#define	ERR_SELECT	-10	/*selectº¯ÊýÊ§°Ü*/
#define	ERR_IOCTL	-11	/*ioctlº¯ÊýÊ§°Ü*/
#define	ERR_SOCKET	-12	/*socketº¯ÊýÊ§°Ü*/
#define	ERR_BIND	-13	/*bindº¯ÊýÊ§°Ü*/
#define	ERR_LISTEN	-14	/*listenº¯ÊýÊ§°Ü*/
#define	ERR_ACCEPT	-15	/*acceptº¯ÊýÊ§°Ü*/
#define	ERR_CONNECT	-16	/*connectº¯ÊýÊ§°Ü*/
#define	ERR_RECV	-17	/*recvº¯ÊýÊ§°Ü*/
#define	ERR_SEND	-18	/*sendº¯ÊýÊ§°Ü*/
#define	ERR_TIME_OUT	-19	/*³¬Ê±*/

extern int GetMsgFromSock(int iSockFd,char* sBuf,int iLen,int iType,int iTimeOut);
extern int PutMsgToSock(int iSockFd,char* sBuf,int iLen);
extern int ConnectToServer(char* sIpAddr,int iPort);
extern int CreateListenService(int iPortNo,int iMaxConnect);
extern int AcceptConnection(int iServerSockFd, char *sClientIp);
extern void WaitClose(int iSockID,int iWaitSec);
extern int GetMsgFromProcess(int iSockFd,char * sBuf,int iLen,int iTimeOut);
extern int PutMsgToProcess(int iSock,char * sBuf,int iLen);
extern int ConnectToProcess(char * Process_Path);
extern int CreateListenProcess(char * Process_Path);
extern int AcceptProcessConnection(int lsn_fd,char * Process_Path, int timeout);
extern int IsConnection(int lsn_fd, int timeout);
extern void GetLocalIp(char *ip);
extern int Data_Coing(char *Src, Json_date *data);
extern int Get_Key_Value(char *Src, char *Key, char *Dst, int Len);
extern int Delay(long sec, long ms);
extern int GetMsgFromSock1(int iSockFd, char* sBuf, int iLen, int iTimeOut);
extern int GetIPbyDNS(char *hostname,char *IP);
extern int ConnectToServerTimeOut(char* sIpAddr, int iPort, int iTimeout);
extern int Set_Recv_Sock_TimeOut(int sockfd, int iTimeout);
extern int Find_Str_By_Key(char *Src, char *Key, char *Dst, int len);
#endif
