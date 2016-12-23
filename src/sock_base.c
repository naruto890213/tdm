#include "sock_base.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/un.h>
#include <fcntl.h>
#include <netinet/tcp.h>//add for TCP_KEEP ALIVE SET
#include <netdb.h>            /*gethostbyname function */
#include <linux/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <ifaddrs.h>

/*从socket取指定长度的数据放入sBuf中
iType=MSG_PEEK或MSG_WAITALL
iTimeOut超时时间,如果为0,则阻塞（不会超时）*/
int GetMsgFromSock(int iSockFd, char* sBuf, int iLen, int iType, int iTimeOut)
{
	int iRtn, iCurDataLen, iPos = 0;
	struct timeval tv;
	
	tv.tv_sec = iTimeOut;
	tv.tv_usec = 0;
	
#if 1
	fd_set fdsRead;
	do{
		FD_ZERO(&fdsRead);
		FD_SET(iSockFd, &fdsRead);

		if(iTimeOut == 0) 
		{
			iRtn = select(iSockFd + 1, &fdsRead, NULL, NULL, NULL);
		}
		else 
		{
			iRtn = select(iSockFd + 1, &fdsRead, NULL, NULL, &tv);
		}

		if(iRtn < 0) 
		{
			perror("select");
			return ERR_SELECT;
		}
		else if(iRtn == 0) 
		{
			printf("timeout\n");
			return ERR_TIME_OUT; 
		}

		if((iRtn = ioctl(iSockFd, FIONREAD, &iCurDataLen)) < 0) 
		{
			printf("ioctl err\n");
			return ERR_IOCTL;
		}

		if(iLen == 0) 
			iLen = iCurDataLen;

		if(iCurDataLen > iLen-iPos) 
		{
			iCurDataLen = iLen-iPos;
		}
		if((iRtn = recv(iSockFd, sBuf+iPos, iCurDataLen, iType))<=0&&(iCurDataLen>0||iTimeOut==0)) 
		{
			printf("return err\n");
			return ERR_RECV;
		}
		iPos += iRtn;
	}while(iPos < iLen);
	 sBuf[iLen]='\0';
#else
	setsockopt(iSockFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
	iCurDataLen = recv(iSockFd, sBuf, iLen, 0);
#endif

	return iLen;
}

int GetMsgFromSock1(int iSockFd, char* sBuf, int iLen, int iTimeOut)
{
	int iRtn = 0;
	struct timeval tv;
	fd_set fdsRead;
	
	tv.tv_sec = iTimeOut;
	tv.tv_usec = 0;
	
	FD_ZERO(&fdsRead);
	FD_SET(iSockFd, &fdsRead);
	iRtn = select(iSockFd + 1, &fdsRead, NULL, NULL, &tv);

	if(iRtn < 0) 
	{
		perror("select");
		return ERR_SELECT;
	}
	else if(iRtn == 0) 
	{
		printf("timeout\n");
		return ERR_TIME_OUT; 
	}

	if((iRtn = recv(iSockFd, sBuf, iLen, 0)) <= 0) 
	{
		printf("return err\n");
		return ERR_RECV;
	}
	sBuf[iRtn] = 0;

	return iRtn;
}

/*向socket写入指定长度的sBuf中的数据*/
int PutMsgToSock(int iSockFd, char* sBuf, int iLen)
{
	int iRtn,iPos=0;
	struct timeval tv;
	fd_set fdsWrite;
	
	tv.tv_sec = 3;
	tv.tv_usec = 0;
	
	do{
		FD_ZERO(&fdsWrite);
		FD_SET(iSockFd, &fdsWrite);
		iRtn = select(iSockFd + 1, NULL, &fdsWrite, NULL, &tv);
		if(iRtn < 0 && errno != EINTR) 
			return ERR_SELECT;
		if(iRtn == 0) 
			return ERR_TIME_OUT;
		iRtn = send(iSockFd, sBuf+iPos, iLen - iPos, 0);
		if(iRtn < 0) 
			return ERR_SEND;
		iPos+=iRtn;
	}while(iPos < iLen);

	return iLen;
}

int ConnectToServer(char* sIpAddr,int iPort)
{
	int iSockFd;
	struct sockaddr_in sai;
	memset(&sai, 0 , sizeof(sai));
	
	if ((iSockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		return ERR_SOCKET;

	sai.sin_family = AF_INET;
	sai.sin_port = htons(iPort);
	sai.sin_addr.s_addr = inet_addr(sIpAddr);
	bzero(&(sai.sin_zero), 8); 
	if (connect(iSockFd, (struct sockaddr *)&sai, sizeof(struct sockaddr)) < 0) 
		return ERR_CONNECT;

	return iSockFd;
}

int ConnectToServerTimeOut(char* sIpAddr, int iPort, int iTimeout)
{
    int iSockFd;
    struct sockaddr_in sai;
    memset(&sai, 0 , sizeof(sai));

    if ((iSockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return ERR_SOCKET;

    sai.sin_family = AF_INET;
    sai.sin_port = htons(iPort);
    sai.sin_addr.s_addr = inet_addr(sIpAddr);
    bzero(&(sai.sin_zero), 8);

    if (connect(iSockFd, (struct sockaddr *)&sai, sizeof(struct sockaddr)) < 0)
        return ERR_CONNECT;

	struct timeval timeout = {iTimeout, 0};
    int ret = setsockopt(iSockFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    if(ret == -1)
    {
        printf("This come from %s, errno is %d, err is %s\n", __func__, errno, strerror(errno));
    }

    return iSockFd;
}

int Data_Coing(char *Src, Json_date *data)
{
    char *buf = Src;
    char *p = NULL;
    char *outer_ptr = NULL;
    char *inner_ptr = NULL;
    int i = 0;
    int j = 0;

	if(Src == NULL || !strlen(Src))
		return -1;

    if((p = strtok_r(buf, "|", &outer_ptr)) != NULL)
    {
ret:
        if(j >= 1)
        {
            data->Len = (int)strlen(p + 1) - 1;
            memcpy(data->Data, p + 1, data->Len);
    		return data->Result;
        }
        buf = p;
        j++;
        while((p = strtok_r(buf, ",", &inner_ptr)) != NULL)
        {
            switch(i)
            {
                case 0:
                    data->Head = atoi(p);
                    break;
                case 1:
                    data->Type = atoi(p);
                    break;
                case 2:
                    data->Para_Num = atoi(p);
                    break;
                case 3:
                    data->Result = atoi(p);
                    break;
            }
            i++;
            buf = NULL;
        }

        buf = NULL;
    	if((p = strtok_r(buf, "[", &outer_ptr)) != NULL)
			goto ret;
    }

    return data->Result;
}//add by lk 20150630

int Get_Key_Value(char *Src, char *Key, char *Dst, int Len)
{
    char *p = NULL;
	char *saveptr1;
	char buff[512] = {'\0'};
    memset(Dst, 0, Len);

    if(Src == NULL || 0 == strlen(Src))
        return -1;
	else
		memcpy(buff, Src, strlen(Src));

    p = strstr(buff, Key);
    if(p == NULL)
        return -2;

    p = strstr(p, ":");
    if(p == NULL)
        return -3;

    p = strstr(p, "\"");
    if(p == NULL)
        return -4;

    p = strtok_r(p, "\"", &saveptr1);
    if(p == NULL)
        return -5;

    memcpy(Dst, p, strlen(p));

    return 0;
}//add by lk 20150630

int Find_Str_By_Key(char *Src, char *Key, char *Dst, int len)
{
    char *outer_ptr = NULL;
    char *inner_ptr = NULL;
    char *p = NULL;
    char Tmp[1024] = {'\0'};

    if(!Src)
        return -1;

    memcpy(Tmp, Src, strlen(Src));
    p = strstr(Tmp, Key);
    if(p)
    {
        if(strtok_r(p, ":", &outer_ptr))
        {
            p = strtok_r(outer_ptr, "\"", &inner_ptr);
            if(p)
            {
                memset(Dst, 0, len);
                memcpy(Dst, p, strlen(p));
                return 0;
            }
        }
    }

    return -1;
}

int Delay(long sec, long ms)
{
    struct timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = ms*1000;

    return select(0, NULL, NULL, NULL, &tv);
}//add by lk 20150630

/*创建监听服务*/
int CreateListenService(int iPortNo, int iMaxConnect)
{
	int iSockFd;
	int reuse = 1;
	struct sockaddr_in saiServer;
	
	if((iSockFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
		return ERR_SOCKET;

	memset(&saiServer, 0, sizeof(struct sockaddr_in));
	saiServer.sin_family = AF_INET;
	saiServer.sin_addr.s_addr = INADDR_ANY;
	saiServer.sin_port = htons(iPortNo);
	
#if 1
	if(setsockopt(iSockFd, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse))==-1)
	{
		close(iSockFd);
		return ERR_SOCKET;
	}
#else

	int keepAlive = 1; // 开启keepalive属性
	int keepIdle = 180; // 如该连接在180秒内没有任何数据往来,则进行探测 
	int keepInterval = 5; // 探测时发包的时间间隔为5 秒
	int keepCount = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.
	if(setsockopt(iSockFd, SOL_SOCKET, SO_KEEPALIVE, (void *)&keepAlive, sizeof(keepAlive)) == -1)
	{
		close(iSockFd);
		return ERR_SOCKET;
	}
	if(setsockopt(iSockFd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepIdle, sizeof(keepIdle)) == -1)
	{
		close(iSockFd);
		return ERR_SOCKET;
	}
	if(setsockopt(iSockFd, SOL_TCP, TCP_KEEPINTVL, (void *)&keepInterval, sizeof(keepInterval)) == -1)
	{
		close(iSockFd);
		return ERR_SOCKET;
	}
	if(setsockopt(iSockFd, SOL_TCP, TCP_KEEPCNT, (void *)&keepCount, sizeof(keepCount)) ==- 1)
	{
		close(iSockFd);
		return ERR_SOCKET;
	}
#endif

	if(bind(iSockFd, (struct sockaddr *)&saiServer, sizeof(struct sockaddr_in)) < 0)
	{
		close(iSockFd);
		return ERR_BIND;
	}

	if(listen(iSockFd, iMaxConnect)<0)
	{
		close(iSockFd);
		return ERR_LISTEN;
	}
	
	return iSockFd;
}

/*接到客户端请求*/
int AcceptConnection(int iServerSockFd, char *sClientIp)
{
	struct timeval timeout = {180, 0};//add by lk 20151204
	unsigned int iLen = sizeof(struct sockaddr_in);
	int iSockFd, ret;
	struct sockaddr_in saiClient;
	
#if 0
	fd_set fdsRead;
	FD_ZERO(&fdsRead);
	FD_SET(iServerSockFd, &fdsRead);
	
	ret = select(iServerSockFd + 1, &fdsRead, NULL, NULL, NULL); 
	if(ret == -1) 
		return ERR_SELECT;
#endif

	iSockFd = accept(iServerSockFd,(struct sockaddr *)&saiClient,&iLen);
	if(iSockFd<0) 
		return ERR_ACCEPT;

	strcpy(sClientIp,inet_ntoa(saiClient.sin_addr));

	ret = setsockopt(iSockFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	if(ret == -1)
		perror("setsockopt");

	return iSockFd;
}

int UdpPutMsg(int iSockFd,char * sIP,int iPort,void * sMsg,int iLen)
{
 /*
  * function: send message to udp port on  remote host 
  *
  */
	struct sockaddr_in sin;		/* an Internet endpoint address */
	int iRet;
	
	memset( &sin, 0, sizeof(sin) );
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(sIP );
	sin.sin_port = htons(iPort);


	iRet = sendto( iSockFd, sMsg, iLen, 0,
	               (struct sockaddr *)&sin,sizeof(struct sockaddr));

	if(iRet != iLen){
		printf("send fail.\n");
		return -1;
	}
	return 0;
}

void WaitClose(int iSockID,int iWaitSec)
{
	char sBuf[2048];
	while(GetMsgFromSock(iSockID,sBuf,0,MSG_WAITALL,iWaitSec)>0);
	close(iSockID);
}

/*============================================================
Used for IPC
==============================================================*/
int ConnectToProcess(char *Process_Path)
{
	int connect_fd;
	struct sockaddr_un srv_addr;
	struct timeval timeo = {0, 500000};//add by lk 20150604
	socklen_t len = sizeof(timeo);

	connect_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(connect_fd < 0)
	{
		perror("client create socket failed");
		return ERR_SOCKET;
	}

	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path, Process_Path);
	setsockopt(connect_fd, SOL_SOCKET, SO_SNDTIMEO, &timeo, len);//add by lk 20150604

	if((connect(connect_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr))) == -1)
	{
		perror("connect to server failed!");
		printf("the errno is %d\n", errno);
		close(connect_fd);
		return ERR_CONNECT;
	}

	return connect_fd;
}

int CreateListenProcess(char *Process_Path)
{
	int lsn_fd;
	struct sockaddr_un srv_addr;
	memset(&srv_addr, 0, sizeof(srv_addr));
	//create socket to bind 
	if((lsn_fd = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
	{
		perror("can't create communication socket!");
		return ERR_SOCKET;
	}
	//set server addr_param  
	srv_addr.sun_family = AF_UNIX;
	strncpy(srv_addr.sun_path, Process_Path, sizeof(srv_addr.sun_path) - 1);
	unlink(Process_Path);
	//bind sockfd and sockaddr
	if(-1 == bind(lsn_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr)))
	{
		perror("can't bind local sockaddr!");
		close(lsn_fd);
		unlink(Process_Path);
		return ERR_BIND;
	}

	//listen lsn_fd, try listen 1
	if(-1 == listen(lsn_fd, 1))
	{
		perror("can't listen client connect request");
		close(lsn_fd);
		unlink(Process_Path);
		return ERR_LISTEN;
	}

	return lsn_fd;
}

int IsConnection(int lsn_fd, int timeout)
{
	fd_set fdsRead;
    struct timeval tv;

	tv.tv_sec = timeout;
    tv.tv_usec = 0;

    FD_ZERO(&fdsRead);
    FD_SET(lsn_fd, &fdsRead);

    int ret = select(lsn_fd + 1, &fdsRead, NULL, NULL, &tv);
	if(ret == 0)
	{
		//printf("timeout 111111111111111111111111111111111111\n");
		return 0;
	}
	else if(ret)
	{
		//printf("there is new coming\n");
		return lsn_fd;
	}

	return -1;
}

int AcceptProcessConnection(int lsn_fd, char *Process_Path, int timeout)//add by lk 20150127
{
	int apt_fd;
	socklen_t clt_len;
	struct sockaddr_un clt_addr;
	clt_len = sizeof(clt_addr);
	
	printf("the lsn_fd is %d\n", lsn_fd);
	apt_fd = accept(lsn_fd, (struct sockaddr*)&clt_addr, &clt_len);
	if(apt_fd < 0)
	{
		perror("can't listen client connect request");
		return ERR_ACCEPT; 
	}

	return apt_fd;
}

int PutMsgToProcess(int iSock,char * sBuf,int iLen)
{
	int iRtn = 0, iPos = 0;
	do{
		iRtn = write(iSock, sBuf + iPos, iLen - iPos);
		if(iRtn<0) 
			return ERR_SEND;

		iPos+=iRtn;
	}while(iPos<iLen);

	return iLen;
}


/*从socket取指定长度的数据放入sBuf中
iType=MSG_PEEK或MSG_WAITALL
iTimeOut超时时间,如果为0,则阻塞（不会超时）*/
int GetMsgFromProcess(int iSockFd, char* sBuf, int iLen, int iTimeOut)
{
	int iRtn = 0, iPos = 0;
	struct timeval start;
	struct timeval end;
	
	int nFlags = fcntl(iSockFd, F_GETFL, 0);
    fcntl(iSockFd, F_SETFL,(nFlags | O_NONBLOCK));
	
	gettimeofday(&start, NULL);
	do{
		gettimeofday(&end, NULL);
		if(((end.tv_sec - start.tv_sec) > iTimeOut) && (iTimeOut != 0)) 
		{
			if(iPos != 0)
			{
				return iPos;
			}
			return ERR_TIME_OUT;
		}

		iRtn = read(iSockFd, sBuf + iPos, iLen - iPos);
		if(iRtn < 0)
		{
			iRtn = 0;
		}
		else if(iRtn == 0)
		{
			break;
		}
		iPos += iRtn;
	}while(iPos < iLen);
	sBuf[iPos] = 0;

	return iPos;
}

int GetIPbyDNS(char *hostname, char *IP)
{   struct hostent *host;
    struct in_addr in;
    struct sockaddr_in addr_in;
    int h_errno;
    if((hostname == NULL)||(IP == NULL))
    {
		printf("hostname is wrong!\n");
        return -1;
    }

    if((host = gethostbyname(hostname)) != NULL)
    {
        memcpy(&addr_in.sin_addr.s_addr, host->h_addr, 4);
        in.s_addr = addr_in.sin_addr.s_addr;
        strcpy(IP, inet_ntoa(in));
        printf("IP got by dns is : %s \n",IP);
        return 0;
    }
    else
    {
        perror("gethostbyname error!!!\n");
        return -2;
    }
}

int Set_Recv_Sock_TimeOut(int sockfd, int iTimeout)
{
    struct timeval timeout;
    timeout.tv_sec = iTimeout;
    timeout.tv_usec = 0;

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    return 0;
}

void GetLocalIp(char *ip)
{
    struct ifaddrs * ifAddrStruct = NULL;
    void * tmpAddrPtr = NULL;

    getifaddrs(&ifAddrStruct);
    while (ifAddrStruct != NULL)
    {
        if (ifAddrStruct->ifa_addr->sa_family == AF_INET)
        {
            tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if(strcmp(addressBuffer, "127.0.0.1"))
            {
                memcpy(ip, addressBuffer, strlen(addressBuffer));
                printf("the ip is %s\n", ip);
            }
        }
        ifAddrStruct = ifAddrStruct->ifa_next;
    }
}
