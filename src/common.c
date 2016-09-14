#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "sub_server_public.h"
#include "sock_base.h"
#include "tdm.h"
#include "pravitelib.h"
#include "device.h"

pthread_mutex_t pthread_mysql_tdm = PTHREAD_MUTEX_INITIALIZER;
extern int mysql_fd;//add by lk 20150804
extern pthread_mutex_t mysql_mutex;
time_t lastTime = 0;
extern Mysql_Fd Data_Fd;
Config_Data Config_DATA[MAX_CON_NUM];//add by lk 20151016
extern char locale_ip[20]; //add by lk 20140305
static void destory_list(struct list_head *head);//add by 20160125
static void destory_imsi_list(struct list_head *head);
static int ConnectToJavaServer(Mysql_Fd *para);

char LIAN_IP[] = "218.17.107.11:11111,120.24.221.252:8070,120.24.221.252:8070,DOMAINNAME:test.easy2go.cn";
char Master_IP[] = "218.17.107.11:11113,120.24.221.252:8070,120.24.221.252:8070,DOMAINNAME:test.easy2go.cn";
char Test_IP[] = "58.250.57.153:11115,120.24.221.252:8070,120.24.221.252:8070,DOMAINNAME:test.easy2go.cn";

void Print_Data_Spm(Data_Spm Src, char *Buff)
{
	printf("The SN:%s -> minite_Remain is %d, sIMSI is %s, factoryFlag is %d, IP is %s Port is %d, SimBank_ID is %d, Channel_ID is %d\n", 
			Src.SN, Src.minite_Remain, Src.sIMSI, Src.factoryFlag, Src.node->IP, Src.node->Port, Src.node->SimBank_ID, Src.node->Channel_ID);

	if(Buff)
		printf("the battery is %d\n\n", Buff[21]);

	return;
}

void ConnectToMysqlInit(Mysql_Fd *para)
{
	int i;

	for(i = 0; i < SOCKET_NUM; i++)
	{
		para->Deal_Fd[i] = -1;
		para->Logs_Fd[i] = -1;
		pthread_mutex_init(&para->Deal_mutex[i], NULL);
		pthread_mutex_init(&para->Logs_mutex[i], NULL);
	}

	para->Sim_Fd = -1;
	para->Device_Fd = -1;

	pthread_mutex_init(&para->Sim_mutex, NULL);
	pthread_mutex_init(&para->Device_mutex, NULL);

	pthread_mutex_init(&para->list_mutex, NULL);//add by 20160123
	pthread_mutex_init(&para->imsi_mutex, NULL);//add by 20160123

	INIT_LIST_HEAD(&para->head);
	INIT_LIST_HEAD(&para->imsi_list);
	para->device_time = time(NULL);

	ConnectToJavaServer(para);
}

void ConnectToMysqlDeInit(Mysql_Fd *para)
{
	int i;

	for(i = 0; i < SOCKET_NUM; i++)
	{
		if(para->Deal_Fd[i] > 0)
			close(para->Deal_Fd[i]);

		if(para->Logs_Fd[i] > 0)
			close(para->Logs_Fd[i]);

		pthread_mutex_destroy(&para->Deal_mutex[i]);
		pthread_mutex_destroy(&para->Logs_mutex[i]);
	}

	if(para->Sim_Fd > 0)
		close(para->Sim_Fd);

	if(para->Device_Fd > 0)
		close(para->Device_Fd);

	pthread_mutex_destroy(&para->Device_mutex);
	pthread_mutex_destroy(&para->Sim_mutex);
	pthread_mutex_destroy(&para->list_mutex);

	destory_list(&para->head);
	destory_imsi_list(&para->imsi_list);
}

static int ConnectToJavaServer(Mysql_Fd *para)
{
	int i;

	for(i = 0; i < SOCKET_NUM; i++)
	{
		if(para->Deal_Fd[i] < 0)
		{
			para->Deal_Fd[i] = ConnectToServer(ServerIp, ServerPort);
			if(para->Deal_Fd[i] <= 0)
				LogMsg(MSG_ERROR, "Deal_Fd[%d] connect faild, the err is %s\n", i, strerror(errno));
		}

		if(para->Logs_Fd[i] < 0)
		{
			para->Logs_Fd[i] = ConnectToServer(ServerIp, ServerPort);
			if(para->Logs_Fd[i] <= 0)
				LogMsg(MSG_ERROR, "Logs_Fd[%d] connect faild, the err is %s\n", i, strerror(errno));
		}
	}

	if(para->Device_Fd < 0)
	{
		para->Device_Fd = ConnectToServer(ServerIp, ServerPort);
		if(para->Device_Fd <= 0)
			LogMsg(MSG_ERROR, "Device_Fd connect faild, the err is %s\n", strerror(errno));
	}

	if(para->Sim_Fd < 0)
	{
		para->Sim_Fd = ConnectToServer(ServerIp, ServerPort);
		if(para->Sim_Fd <= 0)
			LogMsg(MSG_ERROR, "Sim_Fd connect faild, the err is %s\n", strerror(errno));
	}

	return 0;
}

int getNetworkType(int networkType, char *Dst)
{
	int ret = 0;

    switch(networkType)
    {
        case NETWORK_TYPE_GPRS:
            memcpy(Dst, "GPRS", 4);
            goto NET_2G;
        case NETWORK_TYPE_EDGE:
            memcpy(Dst, "EDGE", 4);
            goto NET_2G;
        case NETWORK_TYPE_CDMA:
            memcpy(Dst, "CMDA", 4);
            goto NET_2G;
        case NETWORK_TYPE_1xRTT:
            memcpy(Dst, "1xRTT", 5);
            goto NET_2G;
        case NETWORK_TYPE_IDEN:
            memcpy(Dst, "IDEN", 4);
NET_2G:
            ret = 2;
            break;
        case NETWORK_TYPE_UMTS:
            memcpy(Dst, "UMTS", 4);
            goto NET_3G;
        case NETWORK_TYPE_EVDO_0:
            memcpy(Dst, "EVDO_0", 6);
            goto NET_3G;
        case NETWORK_TYPE_EVDO_A:
            memcpy(Dst, "EVDO_A", 6);
            goto NET_3G;
        case NETWORK_TYPE_HSDPA:
            memcpy(Dst, "HSDPA", 5);
            goto NET_3G;
        case NETWORK_TYPE_HSUPA:
            memcpy(Dst, "HSUPA", 5);
            goto NET_3G;
        case NETWORK_TYPE_HSPA:
            memcpy(Dst, "HSPA", 4);
            goto NET_3G;
        case NETWORK_TYPE_EVDO_B:
            memcpy(Dst, "EVDO_B", 6);
            goto NET_3G;
        case NETWORK_TYPE_EHRPD:
            memcpy(Dst, "EHRPD", 5);
            goto NET_3G;
        case NETWORK_TYPE_HSPAP:
            memcpy(Dst, "HSPAP", 5);
NET_3G:
            ret = 3;
            break;
        case NETWORK_TYPE_LTE:
            memcpy(Dst, "LTE", 3);
            ret = 4;
            break;
        default:
            break;
    }

    return ret;
}

int SIM_Allocate_Err(SimAlloc_Err_Code Errcode, Data_Spm *simalloc)
{
	switch(Errcode)
	{
		case UnrecognizedDevSN:
			simalloc->Result = UnrecognizedDevSN;
			break;
		case NoDealForDevSN:
			simalloc->Result = NoDealForDevSN;
			break;
		case OutOfService:
			simalloc->Result = OutOfService;
			break;
		case NoSimUsable:
			simalloc->Result = NoSimUsable;
			break;
		case MallocErr:
			simalloc->Result = MallocErr;
			break;
		case IpChange://add by lk 20150515
			simalloc->Result = IpChange;
			break;
		case LowPower:
			simalloc->Result = LowPower;
			break;
		case SockErr:
			simalloc->factoryFlag = 1;
			simalloc->Result = SockErr;	
			break;
		default:
			break;
	}

	return simalloc->Result;
}

static Data_Spm *get_data_list(char *imsi, struct list_head *head)
{
	Data_Spm *node = NULL;
	struct list_head *p = NULL;

	list_for_each(p, head)
	{
		node = list_entry(p, Data_Spm, list);
		if(!memcmp(node->sIMSI, imsi, 18))
		{
			return node;
		}
	}

	return NULL;
}

imsi_node *get_imsi_list(char *imsi, struct list_head *head)
{
	imsi_node *node = NULL;
	struct list_head *p = NULL;

	list_for_each(p, head)
	{
		node = list_entry(p, imsi_node, list);
		if(!memcmp(node->imsi, imsi, 18))
		{
			return node;
		}
	}

	return NULL;
}

Data_Spm *get_data_list_by_SN(char *SN, struct list_head *head)
{
	Data_Spm *node = NULL;
	struct list_head *p = NULL;

	list_for_each(p, head)
	{
		node = list_entry(p, Data_Spm, list);
		if(!memcmp(node->SN, SN, 15))
		{
			return node;
		}
	}

	return NULL;
}

time_t Get_time_by_sn(char *SN, time_t now, int type)
{
	time_t curtime = 0;

	pthread_mutex_lock(&Data_Fd.list_mutex);
    Data_Spm *node = get_data_list_by_SN(SN, &Data_Fd.head);
    pthread_mutex_unlock(&Data_Fd.list_mutex);
    if(node)
    {
        if((time(NULL) - node->outtime) < 60)
            return 1;
		else
			node->outtime = now;
		switch(type)
		{
			case 0:
				curtime = node->start_time;		
				break;
			case 1:
        		if((time(NULL) - node->outtime) > 60)
				{
					node->outtime = now;
					curtime = 1;
				}
				break;
			default:
				break;
		}
    }

	return curtime;
}

int add_data_list(char *imsi, time_t now, struct list_head *head)
{
	pthread_mutex_lock(&Data_Fd.imsi_mutex);
	imsi_node *node = get_imsi_list(imsi, head);
	if(node)
	{
		node->lastTime = now;	
	}
	else
	{
		node = (imsi_node *)malloc(sizeof(imsi_node));
		if(node == NULL)
		{
			perror("malloc");
			pthread_mutex_unlock(&Data_Fd.imsi_mutex);
			return -1;
		}
		memset(node, 0, sizeof(imsi_node));
		memcpy(node->imsi, imsi, strlen(imsi));
		node->lastTime = now;
		list_add_tail(&node->list, head);
	}
	pthread_mutex_unlock(&Data_Fd.imsi_mutex);

	return 0;
}

int add_data_imsi_list(char *imsi, time_t now, struct list_head *head, imsi_node *node)
{
    pthread_mutex_lock(&Data_Fd.imsi_mutex);
    node = get_imsi_list(imsi, head);
    if(node)
    {
        node->lastTime = now;   
    }
    else
    {
        node = (imsi_node *)malloc(sizeof(imsi_node));
        if(node == NULL)
        {
            perror("malloc");
            pthread_mutex_unlock(&Data_Fd.imsi_mutex);
            return -1;
        }
        memset(node, 0, sizeof(imsi_node));
        memcpy(node->imsi, imsi, strlen(imsi));
        node->lastTime = now;
        list_add_tail(&node->list, head);
    }
    pthread_mutex_unlock(&Data_Fd.imsi_mutex);

    return 0;
}

int add_data_list_by_sn(char *imsi, char *SN, time_t now, time_t logtime, time_t outtime, struct list_head *head)
{
#if 0
	pthread_mutex_lock(&Data_Fd.list_mutex);
	Data_Spm *node = get_data_list_by_SN(SN, head);
	if(node)
	{
		node->lastTime = now;	
		if(logtime != 0)
			node->logtime = logtime;
		
		if(outtime != 0)
			node->outtime = outtime;
	}
	else
	{
		node = (Data_Spm *)malloc(sizeof(Data_Spm));
		if(node == NULL)
		{
			perror("malloc");
			pthread_mutex_unlock(&Data_Fd.list_mutex);
			return -1;
		}
		memset(node, 0, sizeof(Data_Spm));
		if(imsi)
			memcpy(node->sIMSI, imsi, strlen(imsi));

		if(SN)
			memcpy(node->SN, SN, strlen(SN));

		node->lastTime = now;
		node->logtime = now;
		list_add_tail(&node->list, head);
	}
	pthread_mutex_unlock(&Data_Fd.list_mutex);
#endif

	return 0;
}

int del_data_list(char *imsi, struct list_head *head)
{
	struct timeval start;
	struct timeval end;

	pthread_mutex_lock(&Data_Fd.list_mutex);
	gettimeofday(&start, NULL);
	Data_Spm *node = get_data_list(imsi, head);
	if(node)
	{
		list_del(&node->list);
		free(node);
	}
	gettimeofday(&end, NULL);
   	printf("Time spend on %s is %ld ms\n", __func__, ((end.tv_sec-start.tv_sec) * 1000 + (end.tv_usec-start.tv_usec)/1000));
	pthread_mutex_unlock(&Data_Fd.list_mutex);

	return 0;
}

void display_list(struct list_head *head)
{
    struct list_head *p = NULL;

    list_for_each(p, head)
    {
        Data_Spm *node = list_entry(p, Data_Spm, list);
        printf("the SN is %s, the imsi is %s, lastTime is %ld\n", node->SN, node->sIMSI, node->lastTime);
    }
}

static void destory_list(struct list_head *head)
{
    struct list_head *p = NULL;
    Data_Spm *node = NULL;
	pthread_mutex_lock(&Data_Fd.list_mutex);
    list_for_each(p, head)
    {
        node = list_entry(p, Data_Spm, list);
        list_del(p);

		if(node)
        	free(node);

		node = NULL;
    }
	pthread_mutex_unlock(&Data_Fd.list_mutex);
}

static void destory_imsi_list(struct list_head *head)
{
    struct list_head *p = NULL;
    imsi_node *node = NULL;
	pthread_mutex_lock(&Data_Fd.imsi_mutex);
    list_for_each(p, head)
    {
        node = list_entry(p, imsi_node, list);
        list_del(p);

		if(node)
        	free(node);

		node = NULL;
    }
	pthread_mutex_unlock(&Data_Fd.imsi_mutex);
}

int Send_Deal_Data(char *Src, char *Dst, int type, int *Fd, pthread_mutex_t *lock)
{
    Json_date Data;
    char recvbuff[1024] = {'\0'};
    memset(&Data, 0, sizeof(Data));
	Data.Result = -1;
	int ret = -1;
	int len = (int)strlen(Src);

	pthread_mutex_lock(lock);
	if(*Fd <= 0)
	{
		*Fd = ConnectToServer(ServerIp, ServerPort);
		if(*Fd <= 0)
		{
            LogMsg(MSG_ERROR, "This come from %s connect err is %s\n", __func__, strerror(errno));
			Delay(1, 0);
			goto ret;
		}
	}

    ret = send(*Fd, Src, len, 0);
	if(ret <= 0)
	{
		close(*Fd);
		*Fd = -1;
		LogMsg(MSG_ERROR, "This come from %s send err is %s\n", __func__, strerror(errno));
		Delay(1, 0);
	}
	else
	{
		ret = GetMsgFromSock(*Fd, recvbuff, 0, 0, 3);//add by lk 
		if(ret > 0)
		{
    		Data_Coing(recvbuff, &Data);
			if(type && !Data.Result)
			{
				memcpy(Dst, Data.Data, strlen(Data.Data));
			}
		}
		else
		{
			close(*Fd);
			*Fd = -1;
			LogMsg(MSG_ERROR, "This come from %s recv err is %s\n", __func__, strerror(errno));
		}
	}
    Delay(0, 50);

ret:
	pthread_mutex_unlock(lock);

	return Data.Result;
}

int Send_Sim_Data(char *Src, char *Dst, int type, int *Fd, pthread_mutex_t *lock)
{
	int ret = -1;
    Json_date Data;
	int len = (int)strlen(Src);

	pthread_mutex_lock(lock);
	if(*Fd <= 0)
	{
		*Fd = ConnectToServer(ServerIp, ServerPort);
		if(*Fd <= 0)
		{
            LogMsg(MSG_ERROR, "This come from %s connect err is %s\n", __func__, strerror(errno));
			Delay(0, 50);
			goto ret;
		}
	}

    ret = send(*Fd, Src, len, 0);
	if(ret <= 0)
	{
		close(*Fd);
		*Fd = -1;
       	LogMsg(MSG_ERROR, "This come from %s send err is %s\n", __func__, strerror(errno));
	}
	else
	{
		switch(type)
		{
			case 1:
    			memset(&Data, 0, sizeof(Data));
				Data.Result = -1;
    			char recvbuff[1024] = {'\0'};
				ret = GetMsgFromSock(*Fd, recvbuff, 0, 0, 2);//add by lk 
				if(ret > 0)
				{
    				Data_Coing(recvbuff, &Data);
					if(type && !Data.Result)
					{
						if(Dst)
							memcpy(Dst, Data.Data, strlen(Data.Data));
					}
				}
				else
				{
					close(*Fd);
					*Fd = -1;
       				LogMsg(MSG_ERROR, "This come from %s recv err is %s\n", __func__, strerror(errno));
				}
				ret = Data.Result;
				break;
			default:
				Delay(0, 10);
				Data.Result = 0;
				break;
		}
	}

ret:
	pthread_mutex_unlock(lock);

	return ret;
}

int Send_Logs_Data(char *Src, int len, int *Fd, pthread_mutex_t *lock)
{
    int ret = -1;
	char buff[1024] = {'\0'};
	struct timeval start;
    struct timeval end;

    pthread_mutex_lock(lock);
	gettimeofday(&start, NULL);

    if(*Fd <= 0)
    {
        *Fd = ConnectToServer(ServerIp, ServerPort);
        if(*Fd <= 0)
        {
            LogMsg(MSG_ERROR, "This come from %s connect err is %s, the Src is %s\n", __func__, strerror(errno), Src);
            Delay(0, 50);
            goto ret;
        }
    }

    ret = send(*Fd, Src, len, 0);
    if(ret <= 0)
    {
        LogMsg(MSG_ERROR, "%s send faild, the errno is %d, err is %s, the Src is %s, the len is %d, the len1 is %d\n", __func__, errno, strerror(errno), Src, len, (int)strlen(Src));
        close(*Fd);
        *Fd = -1;
		Delay(0, 50);
    }
	else
	{
		ret = GetMsgFromSock(*Fd, buff, 0, 0, 1);
		if(ret <= 0)
		{
        	LogMsg(MSG_ERROR, "%s recv faild, the errno is %d, err is %s, ret is %d, the len is %d, len1 is %d\n", __func__, errno, strerror(errno), ret, len, (int)strlen(Src));
        	close(*Fd);
        	*Fd = -1;
		}
	}

ret:
	gettimeofday(&end, NULL);
	printf("the %s:time speen is %ld ms\n", __func__, ((end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)/1000));
    pthread_mutex_unlock(lock);

    return 0;
}

void *work_func(void *arg)
{
	Log_Data *p = NULL;
	p = (Log_Data *)arg;
	int num = 0;

	if(p)
	{
		num = atoll(p->SN) % SOCKET_NUM;

		if((!strlen(p->Buff)))
			LogMsg(MSG_ERROR, "The SN is %s, the Buff is %s, the len is %d\n", p->SN, p->Buff, p->len);
		else
		{
			if(num >= 0 && num < SOCKET_NUM)
				Send_Logs_Data(p->Buff, strlen(p->Buff), &Data_Fd.Logs_Fd[num], &Data_Fd.Logs_mutex[num]);
		}
	}
	
	return NULL;
}

int Send_Device_Data(char *Src, char *Dst, int type)
{
	int ret = -1;
	int cnt = 0;
	int len = (int)strlen(Src);

	pthread_mutex_lock(&Data_Fd.Device_mutex);
	if(Data_Fd.Device_Fd <= 0)
	{
		Data_Fd.Device_Fd = ConnectToServer(ServerIp, ServerPort);
		cnt++;
		if(Data_Fd.Device_Fd <= 0)
		{
			LogMsg(MSG_ERROR, "This come from %s connect err is %s\n", __func__, strerror(errno));
			Delay(0, 50);
			goto lk;
		}
	}

    ret = send(Data_Fd.Device_Fd, Src, len, 0);
	if(ret <= 0)
	{
		close(Data_Fd.Device_Fd);
		Data_Fd.Device_Fd = -1;
		LogMsg(MSG_ERROR, "This come from %s send err is %s\n", __func__, strerror(errno));
	}

lk:
	pthread_mutex_unlock(&Data_Fd.Device_mutex);

	return 0;
}

int VPN_Log(char *SN, char *vpn, int Result, int code, int type)
{
    char buff[256] = {'\0'};
    sprintf(buff, "45445445,2001,7|{'SN':'%s','type':'07','TTContext':'%s','ContextLen':%d,'gSta':%d,'battery':%d,'lastTime':%ld}", SN, vpn, type, code, Result, time(NULL));
    int num = atoll(SN) % SOCKET_NUM;
    Send_Logs_Data(buff, strlen(buff), &Data_Fd.Logs_Fd[num], &Data_Fd.Logs_mutex[num]);

    return 0;
}

int Get_Low_simcards(char *SN, int code, unsigned char *imsi)
{
    int cnt = -1;
    char buff[256] = {'\0'};
    char Dst[128] = {'\0'};
    char Key[64] = {'\0'};
    sprintf(buff, "45445445,1005,2|{'lastDeviceSN':'%s','MCC':%d}", SN, code);
    
    cnt = Send_Sim_Data(buff, Dst, 1, &Data_Fd.Sim_Fd, &Data_Fd.Sim_mutex);
    if(cnt == 0)
    {
        if(!Get_Key_Value(Dst, "IMSI", Key))
        {
            str2Hex(Key, imsi, strlen(Key));
            cnt = 1;
        }
    }
    return cnt;
}

int Set_Device_SN_VPN(char *SN, char *vpn, int vpn_flag)//add by 20160226
{
    pthread_mutex_lock(&Data_Fd.list_mutex);
    Data_Spm *node = get_data_list_by_SN(SN, &Data_Fd.head);
    if(node == NULL)
    {
        node = (Data_Spm *)malloc(sizeof(Data_Spm));
        if(node)
        {
            memset(node, 0, sizeof(Data_Spm));
            memcpy(node->SN, SN, 15);
            list_add_tail(&node->list, &Data_Fd.head);
        }
        else
            LogMsg(MSG_ERROR, "%s:SN->%s malloc faid err is %s\n", __func__, SN, strerror(errno));
    }

    if(node)
    {
        memcpy(node->vpnc, vpn, strlen(vpn));
        node->vpn_flag = vpn_flag;
    }
    pthread_mutex_unlock(&Data_Fd.list_mutex);

    return 0;
}

int Get_VPN_From_SN_List(char *SN, char *vpn)
{
    int num = 0;

    pthread_mutex_lock(&Data_Fd.list_mutex);
    Data_Spm *node = get_data_list_by_SN(SN, &Data_Fd.head);
    if(node)
    {
        printf("This come from %s, SN is %s, vpn is %s, the vpn_flag is %d\n", __func__, SN, node->vpnc, node->vpn_flag);
        if((strlen(node->vpnc) > 10) && (node->vpn_flag == 1))
        {
            memcpy(vpn, node->vpnc, strlen(node->vpnc));
            num = 1;
        }
    }
    else
    {
        printf("This come from %s, SN is %s, get vpn faild\n", __func__, SN);
    }
    pthread_mutex_unlock(&Data_Fd.list_mutex);

    return num;
}

int GetVPN_From_SimPool(Data_Spm *para, int *code)
{
	return 0;
}

int SetVPN_Status(char *vpn, char *SN, char *dst, int *code)
{
	int iNum_rows = 0;

	return iNum_rows;
}

int Add_imsi_node_by_Data(struct list_head *head, imsi_info *Data, Data_Spm *para)
{
	pthread_mutex_lock(&Data_Fd.imsi_mutex);

	if((para->node == NULL))
	{
ret:
		para->node = get_imsi_list(para->sIMSI, head);
		if(!para->node)
		{
			para->node = (imsi_node *)malloc(sizeof(imsi_node));
			if(para->node)
			{
				memset(para->node, 0, sizeof(imsi_node));
				memcpy(para->node->imsi, para->sIMSI, 18);
				list_add_tail(&para->node->list, head);
			}
		}
	}
	else
	{
		if(memcmp(para->sIMSI, para->node->imsi, 18))
			goto ret;
	}

	if(para->node)
	{
		if(Data)
			memcpy(para->node, Data, sizeof(imsi_info));

		if(para->node->free_flag == 1)
		{
			para->node->free_flag = 0;
		}

		memcpy(para->node->lastDeviceSN, para->SN, 15);
		para->node->lastTime = time(NULL);
		//printf("The SN is %s, imsi is %s, the lastTime is %ld\n", para->SN, para->sIMSI, para->node->lastTime);
	}
	pthread_mutex_unlock(&Data_Fd.imsi_mutex);

	return 0;
}

int Local_Log_Pack(char *sn, char *imsi, void *Src, int length, unsigned int version, char *Dst)
{
	Vsim_UpLoad_Flow *Data = (Vsim_UpLoad_Flow *)Src;
	long long Up_Flow_All, Down_Flow_All, Day_Used;
	char net_buff[12] = {'\0'};

	getNetworkType(Data->Network_Type, net_buff);

	if(Data->remain_min <= 0)
		Data->remain_min = 1440;

	if(version >= 1601080936)
	{
		Up_Flow_All = Data->Up_Flow_All * 1024;
		Down_Flow_All = Data->Down_Flow_All * 1024;
		Day_Used = Data->Day_Used * 1024;
	}
	else
	{
		Up_Flow_All = Data->Up_Flow_All;
		Down_Flow_All = Data->Down_Flow_All;
		Day_Used = Data->Day_Used;
	}

	sprintf(Dst, "45445445,2001,20|{'SN':'%s','location':'%d.%d.%d.%d','UploadFlowCount':'%d','wifiCount':'%d',\
'battery':'%d','wifiState':'%d','gSta':'%d','gStrenth':'%d','upFlowAll':'%lld','downFlowAll':'%lld','dayUsedFlow':'%lld','minsRemaining':'%d',\
'roamGStrenth':'%d','roamUpFlowAll':'%d','roamDownFlowAll':'%d','roamDayUsedFlow':'%d','FlowDistinction':'%d','IMSI':'%s','TTContext':'%s','lastTime':'%ld'}&&",
	sn, Data->Mcc, Data->Mnc, Data->Lac, Data->Cid, Data->Upload_Flow_Id, Data->Wifi_count, Data->Battery, Data->Wifi_State * 2, \
	Data->_3g_status, Data->_3g_strength, Up_Flow_All, Down_Flow_All, Day_Used, Data->remain_min, Data->Roam_3g_Strength, \
	Data->Roam_Up_Flow_All, Data->Roam_Down_Flow_All, Data->Roam_Day_Used, Data->Flow_All, imsi, net_buff, time(NULL));

	printf("the Dst is %s, the strlen(Dst) is %d\n", Dst, (int)strlen(Dst));

    return 0;
}

int Heart_Log_Pack(char *sn, char *imsi, HB_NEW_t *Src, char *Dst)
{
	sprintf(Dst, "45445445,2002,6|{'SN':'%s','location':'%d.%d.%d.%d','dayUsedFlow':'%d','roamGStrenth':%d,'IMSI':'%s','lastTime':'%ld'}&&", \
			sn, Src->MCC, Src->MNC, Src->LAC, Src->CID, Src->Local_DataUsed, Src->Sig_Strength, imsi, time(NULL));

	return 0;
}

int Local_Confirm_Pack(char *sn, char *imsi, char *Version, int apkversion, char *iccid, char *Dst)
{
	sprintf(Dst, "45445445,2003,7|{'SN':'%s','version':'%d','IMSI':'%s','versionAPK':'%d','TTContext':'%s','lastTime':'%ld'}&&",
   		sn, atoi(Version), imsi, apkversion, iccid, time(NULL));

	return 0; 
}

int Get_Data_IP_SN(char *imsi, Data_Spm *para)
{
	return 0;	
}

int TT_Log_Pack_Data(char *sn, int cnt, char *imsi, unsigned char *Buff, int len, Data_Spm *para)
{
	Log_Data Data;
	memset(&Data, 0, sizeof(Data));
	char TT_buf[160] = {'\0'};

	if(len > 0)
		Hex2String(Buff, TT_buf, len);	
	
	imsi[18] = '\0';//add by lk 201509010
	memcpy(Data.SN, sn, 15);
	sprintf(Data.Buff, "45445445,2005,7|{'SN':'%s','TTCnt':%d,'IMSI':'%s','TTContext':'%s','ContextLen':'%d','lastTime':'%ld'}&&", sn, cnt, imsi, TT_buf, len, time(NULL));
	Data.len = strlen(Data.Buff);
	printf("This come from %s, the Buff is %s\n", __func__, Data.Buff);

	threadpool_add_job(para->pool, &Data, sizeof(Data));
	Delay(0, 20);

	return 0;
}

int TT_Log(char *sn, int cnt, char *imsi, unsigned char *Buff, int len)
{
	char TT_buf[160] = {'\0'};
	char buff[320] = {'\0'};

	if(len > 0)
		Hex2String(Buff, TT_buf, len);	
	
	imsi[18] = '\0';//add by lk 201509010
	sprintf(buff, "45445445,2005,7|{'SN':'%s','TTCnt':%d,'IMSI':'%s','TTContext':'%s','ContextLen':'%d','lastTime':%ld}&&", sn, cnt, imsi, TT_buf, len, time(NULL));

	return 0;
}

int LOG_OUT_Log_Pack(char *sn, char *sIMSI, char *Dst)
{
	if(!strcmp(sIMSI, "000000000000000000"))
	{
		sIMSI = NULL;
	}	

	sprintf(Dst, "45445445,2001,4|{'SN':'%s','type':'06','IMSI':'%s','lastTime':%ld}&&", sn, sIMSI, time(NULL));

	return 0;
}

static int Get_Deal_From_SimPool_socket(char *buff, char *Dst, int vir_flag, int *Fd, pthread_mutex_t *lock)
{
    Json_date Data;
    char recvbuff[1024] = {'\0'};
    memset(&Data, 0, sizeof(Data));
    Data.Result = -1;
    int ret = -1;

    pthread_mutex_lock(lock);
    if(*Fd <= 0)
    {
        *Fd = ConnectToServer(ServerIp, ServerPort);
        if(*Fd <= 0)
        {
        	LogMsg(MSG_ERROR, "%s connect faild, errno is %d, err is %s\n", __func__, errno, strerror(errno));
			Data.Result = 4;
            goto ret;
        }
    }

    ret = send(*Fd, buff, strlen(buff), 0);
    if(ret <= 0)
    {
        close(*Fd);
        *Fd = -1;
		Data.Result = 4;
        LogMsg(MSG_ERROR, "%s send faild, errno is %d, err is %s\n", __func__, errno, strerror(errno));
    }
    else
    {
        ret = GetMsgFromSock(*Fd, recvbuff, 0, 0, 3);//add by lk 
        if(ret > 0)
        {
            Data_Coing(recvbuff, &Data);
            if(!Data.Result)
            {
                memcpy(Dst, Data.Data, strlen(Data.Data));
            }
        }
        else
        {
            close(*Fd);
            LogMsg(MSG_ERROR, "Get_Data_From_Deal faild, errno is %d, err is %s\n", errno, strerror(errno));
            *Fd = -1;
			Data.Result = 4;
        }
    }

ret:
    pthread_mutex_unlock(lock);

    return Data.Result;
}

int Set_SimCards_Status(char *sIMSI)
{
	char buff[256] = {'\0'};
	char Dst[128] = {'\0'};

	if(!memcmp(sIMSI, "000000000000000000", 18))
		return 0;

	sprintf(buff, "45445,1005,2|{'IMSI':'%s','serverIp':''}&&", sIMSI);
	printf("%s->buff is %s\n", __func__, buff);

	return Send_Sim_Data(buff, Dst, 1, &Data_Fd.Sim_Fd, &Data_Fd.Sim_mutex);
}

int Get_Deal_From_SimPool(Data_Spm *para1, char *Dst, void *para, int Data)
{
    char buff[360] = {'\0'};
	char location[320] = {'\0'};
	AccessAuth_Def *p = (AccessAuth_Def *)para;

	sprintf(location, "'SN':'%s','nowtime':'%ld','MCC':'%d','MNC':'%d','LAC':'%d','CID':'%d','Data':'%d','firmWareVer':'%d','firmWareAPKVer':'%d','battery':'%d','minsRemaining':'%d','tdmIpPort':'%s:%d'", 
		para1->SN, time(NULL), para1->MCC, p->MNC, p->LAC, p->CID, Data, p->FirmwareVer, para1->versionAPK, p->DeviceSN[21], para1->take_time, Web_IP1, Web_Port);

    if(!para1->vir_flag)
        sprintf(buff, "333,1001,11|{%s}&&", location);
    else
        sprintf(buff, "333,1003,11|{%s}&&", location);

	memset(para1->speedStr, 0, sizeof(para1->speedStr));
	printf("the buff is %s, the strlen(buff) is %d\n", buff, (int)strlen(buff));

	int sn_num = (atoll(para1->SN) % SOCKET_NUM);

	int cnt = Get_Deal_From_SimPool_socket(buff, Dst, para1->vir_flag, &Data_Fd.Deal_Fd[sn_num], &Data_Fd.Deal_mutex[sn_num]);

    return cnt;
}

static int Get_Key_Value_Int(char *Src, char *Dst)
{
	char Key[64] = {'\0'};
	int Ret = 0;
	
	if(!Get_Key_Value(Src, Dst, Key))
	{
		Ret = atoi(Key);
		printf("the %s is %d\n", Dst, Ret);
	}

	return Ret;
}

static int Get_Key_Value_VPN(char *Src, Data_Spm *para)
{
	para->vpn = Get_Key_Value_Int(Src, "ifVPN");

	switch(para->vpn)
	{
		case 0:
			para->ifVPN = 0;
			break;
		case 1:
			para->ifVPN = 0x08;
			break;
		case 2:
			para->ifVPN = 0x10;
			break;
	}

	return 0;
}

static int Get_Key_Value_Factory(char *Src, Data_Spm *para)
{
	para->factoryFlag = Get_Key_Value_Int(Src, "factoryFlag");

	if(1 == para->factoryFlag)
	{
		Get_Key_Value(Src, "serverInfo", para->ServerInfo);	
		printf("the serverInfo is %s\n", para->ServerInfo);
	}

	return para->factoryFlag;
}

static int Get_Key_Value_IMEI(char *Src, char *Dst, long long *IMEI)
{
	char Key[64] = {'\0'};

	if(!Get_Key_Value(Src, "IMEI", Key))
    {
		*IMEI = atoll(Key);
		printf("the IMEI is %s, the IMEI is %lld\n", Key, *IMEI);
    }

	return 0;
}

static void Get_Key_Value_IPAndPort(char *Src, char *Dst, void *para, void *para1)
{
	char Key[64] = {'\0'};
	char *outer_ptr = NULL;
	char *p = NULL;

	if(!Get_Key_Value(Src, Dst, Key))
	{
		printf("the %s is %s\n", Dst, Key);
		if((p = strtok_r(Key, ":", &outer_ptr)) != NULL)
        {
			memcpy(para, p, strlen(p));
        }

        if((p = strtok_r(NULL, ":", &outer_ptr)) != NULL)
        {
			*(int *)para1 = atoi(p);
        }
	}
}

static void Get_Key_Value_Str_Type(char *Src, char *Dst, void *para, void *para1)
{
    char Key[64] = {'\0'};
    char *outer_ptr = NULL;
    char *p = NULL;

    if(!Get_Key_Value(Src, Dst, Key))
    {
        printf("the %s is %s\n", Dst, Key);
        if((p = strtok_r(Key, "-", &outer_ptr)) != NULL)
        {
        	*(int *)para = atoi(p);
        }

        if((p = strtok_r(NULL, "-", &outer_ptr)) != NULL)
        {
            *(int *)para1 = atoi(p);
        }
    }
}

static int Get_Key_Value_Str(char *Src, char *Dst, char *para, int len)
{
	char Key[64] = {'\0'};
	int	len1 = 0;
	
	if(!Get_Key_Value(Src, Dst, Key))
	{
		len1 = (int)strlen(Key);	
		printf("the %s is %s, the strlen is %d\n", Dst, Key, len1);
		if(len1 > len)
		{
			memcpy(para, Key, len1);
			len1 = 1;
		}
		else
			len1 = 0;
	}

	return len1;
}

static int Get_Key_Value_Vusim_Active_Flag(char *Src, imsi_info *para)
{
	char Key[64] = {'\0'};	

	if(!Get_Key_Value(Src, "SIMIfActivated", Key))
    {
        printf("the SIMIfActivated is %s, the strlen of Key is %d\n", Key, (int)strlen(Key));
        if(!memcmp(Key, "否", 2))
            para->Vusim_Active_Flag = 1;
    }

    if(1 == para->Vusim_Active_Flag)
    {
        if(!Get_Key_Value(Src, "Vusim_Active_Num", Key))
        {
            if((int)strlen(Key) > 9)
                memcpy(para->Vusim_Active_Num, Key, strlen(Key));
            else
                para->Vusim_Active_Flag = 0;

            printf("the Vusim_Active_Num is %s\n", Key);
        }
    }

	return para->Vusim_Active_Flag;
}

static int Get_Key_Value_IMSI(char *Src, char *Dst, char *sIMSI, Data_Spm *para)
{
	char Key[64] = {'\0'};

	if(!Get_Key_Value(Src, "IMSI", Key))
	{
		printf("the IMSI is %s\n", Key);
		memcpy(sIMSI, Key, strlen(Key));
		memcpy(para->sIMSI, Key, strlen(Key));
		str2Hex(Key, para->IMSI, strlen(Key));
	}

	return 0;
}

static int Get_Data_From_Deal(char *Src, Data_Spm *para, char *SN)
{
	imsi_info Data;
	memset(&Data, 0, sizeof(imsi_info));
	printf("the Src is %s\n", Src);

	para->minite_Remain = Get_Key_Value_Int(Src, "minsRemaining");
	para->ifTest = Get_Key_Value_Int(Src, "ifTest");
	para->orderType = Get_Key_Value_Int(Src, "orderType");
	Data.ifRoam = Get_Key_Value_Int(Src, "ifRoam");
	Data.speedType = Get_Key_Value_Int(Src, "speedType");
	Data.NeedFile = Get_Key_Value_Int(Src, "fileUpdate");
	Data.IsApn = Get_Key_Value_Str(Src, "APN", Data.APN, 3);
	Get_Key_Value_VPN(Src, para);
	Get_Key_Value_Factory(Src, para);
	Get_Key_Value_IPAndPort(Src, "IPAndPort", Data.IP, &Data.Port);
	Get_Key_Value_Str_Type(Src, "slotNumber", &Data.SimBank_ID, &Data.Channel_ID);
	Get_Key_Value_Str(Src, "speedStr", para->speedStr, 4);
	Get_Key_Value_Vusim_Active_Flag(Src, &Data);
	Get_Key_Value_IMEI(Src, "IMEI", &Data.IMEI);
	Get_Key_Value_IMSI(Src, "IMSI", Data.imsi, para);

	if(Get_Key_Value_Str(Src, "Selnet", Data.Selnet, 4))
		Data.IsNet = 0x02;

    if(1 == Data.speedType)
        Data.speedType = 0x04;

	Add_imsi_node_by_Data(&Data_Fd.imsi_list, &Data, para);

	return para->minite_Remain;
}

int Set_Dada_IP(Data_IP *para, char *ip)
{
	if(!ip)
		return -1;

	char simip[20] = {'\0'}; 
	memcpy(simip, ip, strlen(ip));
	char *p = simip;
	int i = 0;

	while((p = strtok(p, ".")))
	{
		para->ip[i] = atoi(p);
		p = NULL;
		i++;
	}

	return 0;
}

static int Get_SpeedStr_From_Socket(Data_Spm *para)
{
	int sn_num = (atoll(para->SN) % SOCKET_NUM);
	int cnt = 0;
	char buff[128] = {'\0'};	
	char Dst[256] = {'\0'};

	sprintf(buff, "45445445,3002,2|{'SN':'%s','MCC':'%d'}", para->SN, para->MCC);

	cnt = Get_Deal_From_SimPool_socket(buff, Dst, 0, &Data_Fd.Deal_Fd[sn_num], &Data_Fd.Deal_mutex[sn_num]);

	if(!cnt)
		Get_Key_Value_Str(Dst, "speedStr", para->speedStr, 4);

	return cnt;
}

int Set_Deal_From_SimPool(char *sn, char *imsi, int userCountry, int factoryFlag, int *minite_Remain)
{
	int cnt = -1;
	char buff[128] = {'\0'};
	char Dst[256] = {'\0'};
	char Key[56] = {'\0'};
	
	sprintf(buff, "45445445,3002,5|{'SN':'%s','userCountry':%d,'intradayDate':%ld,'factoryFlag':%d,'IMSI':'%s'}&&", sn, \
			userCountry, time(NULL), factoryFlag, imsi);

	int sn_num = (atoll(sn) % SOCKET_NUM);
	
	if(minite_Remain == NULL)
	{
		cnt = Send_Deal_Data(buff, NULL, 0, &Data_Fd.Deal_Fd[sn_num], &Data_Fd.Deal_mutex[sn_num]);
	}
	else
	{
		cnt = Send_Deal_Data(buff, Dst, 1, &Data_Fd.Deal_Fd[sn_num], &Data_Fd.Deal_mutex[sn_num]);
		if(cnt == 0)
		{
			if(!Get_Key_Value(Dst, "minsRemaining", Key))
			{
				int count = atoi(Key);
				if(count <= 0)
				{
					*minite_Remain = 1;
				}
				else
				{
					*minite_Remain = count;
				}
			}
			else
			{
				LogMsg(MSG_ERROR, "%s->Dst is %s\n", __func__, Dst);
			}
		}
		else
		{
			LogMsg(MSG_ERROR, "%s->return:%d, SN:%s, errno:%d, err:%s, the MCC is %d\n", __func__, cnt, sn, errno, strerror(errno), userCountry);
			*minite_Remain = 1;
		}
	}

	return cnt;
}

int SetFd_From_Device(char *SN, int socket1, int socket2)
{
	return 0;
}

int Get_Config_By_MCC(int code, char *buff, Data_Spm *para)
{
	if(strlen(para->speedStr) > 5)
		memcpy(buff, para->speedStr, sizeof(para->speedStr));
	else
		Get_SpeedStr_From_Socket(para);
		
    return 0;
}

static int Get_Deal_From_Remote(Data_Spm *para, void *para1, int Data)
{
	char buff[512] = {'\0'};
	int cnt = -1, count = 0;
	AccessAuth_Def *p = (AccessAuth_Def *)para1;

	if(p->MCC != para->MCC)
		para->MCC = p->MCC;

	switch(para->MCC)
	{
		case 454:
		case 455:
		case 460:
        	memcpy(para->ServerInfo, Test_IP, strlen(Test_IP));
			break;
		default:
        	para->factoryFlag = 1;
        	memcpy(para->ServerInfo, Master_IP, strlen(Master_IP));
        	return SIM_Allocate_Err(IpChange, para); //add by lk 20150211    
			break;
	}

lk:
	cnt = Get_Deal_From_SimPool(para, buff, p, Data);	
	if(cnt)
	{
		LogMsg(MSG_ERROR, "%s:SN->%s Get_Deal_From_SimPool return %d\n", __func__, para->SN, cnt);
		switch(cnt)
		{
			case 1:
        		return SIM_Allocate_Err(LowPower, para); //add by lk 20150211    
			case 2:
				return SIM_Allocate_Err(NoSimUsable, para);
			case 3:
			case 500:
				return SIM_Allocate_Err(OutOfService, para);
			case 4:
				if(0 == count)
				{
					count = 1;
	        		LogMsg(MSG_ERROR, "%s:SN->%s Get_Deal_From_SimPool frist faild, return %d\n", __func__, para->SN, cnt);
	        		goto lk;
				}
				return SIM_Allocate_Err(OutOfService, para);
			default:
				break;
		}
	}
	else
	{
		if(!para->vir_flag)
	        Get_Data_From_Deal(buff, para, para->SN);
	}

	if(1 == para->factoryFlag)
	{
        return SIM_Allocate_Err(IpChange, para);
	}

	para->Result = 0;  

	if(para->lastStart == 1)
    {
        if(para->orderType == 2)
        {
            para->new_start_flag = 2;
        }
		else
            para->new_start_flag = 1;
    }
    else
        para->new_start_flag = 0;

	if(para->socket_spm > 0)
	{
		close(para->socket_spm);
		para->socket_spm = 0;
	}

	return 0;
}

int Deal_Proc(Data_Spm *para, int MCC)
{
	para->minite_Remain = 1;
	if(!MCC)
		MCC = 460;

#if 0
	Get_Deal_From_Remote(para, MCC);

	if((para->lastStart == 1) || (para->minite_Remain > 1))//add by 20160315
    {
        if(para->orderType == 2)
            para->minite_Remain = 1439;
        else
            para->minite_Remain = 1440;
    }
#endif

    return 0;
}

int SIM_Allocate_Proc(Data_Spm *Src)
{
	int Ret = 0;
	AccessAuth_Def * p_Src = (AccessAuth_Def *)(Src->Buff);
	unsigned short data_size = 0;//add by lk 20151103
	Src->vir_flag = p_Src->DeviceSN[18];//add by lk 20151104

	memcpy(&Src->take_time, p_Src->DeviceSN + 19, 2);
	memcpy(&data_size, p_Src->DeviceSN + 16, 2);//add by lk 20151103
	memcpy(&Src->versionAPK, p_Src->LastIMSI, 4);

	printf("the vir_flag is %d, the SN is %s\n", Src->vir_flag, p_Src->DeviceSN);

	if(p_Src->MCC == 0)
		p_Src->MCC = 460;

	Ret = Get_Deal_From_Remote(Src, p_Src, data_size);
	if(Ret)
		return Ret;

	Src->Result = 0;
	Src->start_time = time(NULL);

	return 0;
}
