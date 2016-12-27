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

Mysql_Fd Data_Fd;
char locale_ip[16]; //add by lk 20140305

char Test_IP[] = "218.17.107.11:11111,120.24.221.252:8070,120.24.221.252:8070,DOMAINNAME:test.easy2go.cn";
char Master_IP[] = "218.17.107.11:11113,120.24.221.252:8070,120.24.221.252:8070,DOMAINNAME:test.easy2go.cn";

void Print_Data_Spm(Data_Spm Src, char *Buff)
{
	printf("The SN:%s -> minite_Remain is %d, sIMSI is %s, factoryFlag is %d, IP is %s Port is %d, SimBank_ID is %d, Channel_ID is %d\n", 
			Src.SN, Src.minite_Remain, Src.sIMSI, Src.factoryFlag, Src.node.IP, Src.node.Port, Src.node.SimBank_ID, Src.node.Channel_ID);

	if(Buff)
		printf("the battery is %d\n\n", Buff[21]);

	return;
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

static int Init_Data_Spm(Data_Spm *para)
{
	memset(para, 0, sizeof(Data_Spm));
	para->socket_spm = -1;

	return 0;
}

int Is_Valid_SN(char *SN)
{
	unsigned long long SN_NUM = 0;

	if(SN)
		SN_NUM = atoll(SN);

	if((SN_NUM > 172150210000000 && SN_NUM < 172150211000000) || (SN_NUM > 860172008100000 && SN_NUM < 860172009000000))
		return 1;

	return 0;
}

void ConnectToMysqlInit(void)
{
	Mysql_Fd *para = &Data_Fd;
	int i;

	memset(locale_ip, 0, sizeof(locale_ip));
    GetLocalIp(locale_ip); //add by lk 20150305

	for(i = 0; i < SOCKET_NUM; i++)
	{
		para->Logs_Fd[i] = ConnectToServerTimeOut(Log_IP, ServerPort, Java_TIMEOUT);
		pthread_mutex_init(&para->Logs_mutex[i], NULL);
	}

	pthread_mutex_init(&para->list_mutex, NULL);//add by 20160123
	INIT_LIST_HEAD(&para->head);

	return;
}

static int ReConectJavaServer(int *Fd)
{
	if(*Fd <= 0)
	{
		*Fd = ConnectToServerTimeOut(Log_IP, ServerPort, 3);
		if(*Fd <= 0)
		{
            LogMsg(MSG_ERROR, "This come from %s connect err is %s\n", __func__, strerror(errno));
			return -1;
		}
		Set_Recv_Sock_TimeOut(*Fd, 1);	
	}

	return 0;
}

void Close_Socket_Fd(int *Fd)
{
	if(*Fd > 0)
	{
		close(*Fd);
		*Fd = 0;
	}

	return;
}

void ConnectToMysqlDeInit(void)
{
	int i;
	Mysql_Fd *para = &Data_Fd;

	for(i = 0; i < SOCKET_NUM; i++)
	{
		Close_Socket_Fd(&para->Logs_Fd[i]);
		pthread_mutex_destroy(&para->Logs_mutex[i]);
	}

	pthread_mutex_destroy(&para->list_mutex);
	destory_list(&para->head);
}

static int getNetworkType(int networkType, char *Dst)
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

static int SIM_Allocate_Err(SimAlloc_Err_Code Errcode, Data_Spm *simalloc)
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

Data_Spm *get_data_list_by_SN(char *SN)
{
	Data_Spm *node = NULL;
	struct list_head *head = &Data_Fd.head;
	struct list_head *p = NULL;

	list_for_each(p, head)
	{
		node = list_entry(p, Data_Spm, list);
		if(!memcmp(node->SN, SN, 15))
			return node;
	}

	return NULL;
}

Data_Spm *Get_Node_By_SN(char *SN)
{
    Data_Spm *node = NULL;

    if(Is_Valid_SN(SN))
    {
        pthread_mutex_lock(&Data_Fd.list_mutex);
        node = get_data_list_by_SN(SN);
        if(node == NULL)
        {
            node = (Data_Spm *)malloc(sizeof(Data_Spm));
            if(node == NULL)
            {
                LogMsg(MSG_ERROR, "%s:%d:SN->%s malloc faid err is %s\n", __func__, __LINE__, SN, strerror(errno));
            }
            else
            {
                Init_Data_Spm(node);
                memcpy(node->SN, SN, 15);
                list_add_tail(&node->list, &Data_Fd.head);
            }
        }
        pthread_mutex_unlock(&Data_Fd.list_mutex);

        if(node)
			Close_Socket_Fd(&node->socket_spm);
    }

    return node;
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

static int Send_Sim_Data(char *Src, char *Dst, int len)
{
	int fd = 0;
	int ret = -1;

	fd = ConnectToServerTimeOut(Log_IP, ServerPort, 2);
	if(fd <= 0)
	{
        LogMsg(MSG_ERROR, "This come from %s connect err is %s\n", __func__, strerror(errno));
		return ret;
	}

	Set_Recv_Sock_TimeOut(fd, 2);	

    if(write(fd, Src, strlen(Src)) <= 0)
	{
        LogMsg(MSG_ERROR, "This come from %s connect err is %s\n", __func__, strerror(errno));
		return ret;
	}

	ret = recv(fd, Dst, len, 0);
	close(fd);

	return ret;
}

static int Send_Logs_Data(char *Src, int len, int *Fd, pthread_mutex_t *lock)
{
    int ret = -1;
	int count = 0;
	char buff[512] = {'\0'};
	struct timeval start;
    struct timeval end;

    pthread_mutex_lock(lock);
	gettimeofday(&start, NULL);

lk:
	if(ReConectJavaServer(Fd))
    {
       	LogMsg(MSG_ERROR, "This come from %s connect err is %s, the Src is %s\n", __func__, strerror(errno), Src);
       	goto ret;
    }

    ret = send(*Fd, Src, len, 0);
    if(ret <= 0)
    {
		Close_Socket_Fd(Fd);
		if(0 == count)
		{
			count = 1;
			goto lk;
		}
        LogMsg(MSG_ERROR, "%s send faild, the errno is %d, err is %s, the Src is %s\n", __func__, errno, strerror(errno), Src);
		goto ret;
    }

	ret = recv(*Fd, buff, sizeof(buff), 0);
	if(ret <= 0)
	{
		gettimeofday(&end, NULL);
    	LogMsg(MSG_ERROR, "%s recv faild, time speen is %ld ms err is %s, ret is %d\n", __func__, 
					((end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)/1000), strerror(errno), ret);

		Close_Socket_Fd(Fd);
	}

ret:
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

int VPN_Log(char *SN, char *vpn, int Result, int code, int type)
{
    char buff[256] = {'\0'};
    sprintf(buff, "45445445,2001,7|{'SN':'%s','type':'07','TTContext':'%s','ContextLen':'%d','gSta':'%d','battery':'%d','lastTime':'%ld'}&&", 
					SN, vpn, type, code, Result, time(NULL));

    int num = atoll(SN) % SOCKET_NUM;
    Send_Logs_Data(buff, strlen(buff), &Data_Fd.Logs_Fd[num], &Data_Fd.Logs_mutex[num]);

    return 0;
}

int Get_Low_simcards(char *SN, int code, unsigned char *imsi)
{
    return -1;
}

int Set_Device_SN_VPN(char *SN, char *vpn, int vpn_flag)//add by 20160226
{
    pthread_mutex_lock(&Data_Fd.list_mutex);
    Data_Spm *node = get_data_list_by_SN(SN);
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
    Data_Spm *node = get_data_list_by_SN(SN);
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
        printf("This come from %s, SN is %s, get vpn faild\n", __func__, SN);

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

    return 0;
}

int Heart_Log_Pack(char *sn, char *imsi, HB_NEW_t *Src, char *Dst)
{
	sprintf(Dst, "45445445,2002,6|{'SN':'%s','location':'%d.%d.%d.%d','dayUsedFlow':'%d','roamGStrenth':'%d','IMSI':'%s','lastTime':'%ld'}&&", \
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
	sprintf(Data.Buff, "45445445,2005,7|{'SN':'%s','TTCnt':'%d','IMSI':'%s','TTContext':'%s','ContextLen':'%d','lastTime':'%ld'}&&", 
										sn, cnt, imsi, TT_buf, len, time(NULL));
	Data.len = strlen(Data.Buff);

	threadpool_add_job(para->pool, &Data, sizeof(Data));

	return 0;
}

int LOG_OUT_Log_Pack(char *sn, char *sIMSI, char *Dst)
{
	if(!strcmp(sIMSI, "000000000000000000"))
		sIMSI = NULL;

	sprintf(Dst, "45445445,2006,3|{'SN':'%s','IMSI':'%s','lastTime':'%ld'}&&", sn, sIMSI, time(NULL));

	return 0;
}

static int Get_Deal_From_SimPool_socket(char *buff, char *Dst, int len)
{
    Json_date Data;
    char recvbuff[1024] = {'\0'};
    memset(&Data, 0, sizeof(Data));

	if(Send_Sim_Data(buff, recvbuff, sizeof(recvbuff)))
    {
        Data_Coing(recvbuff, &Data);
        if(!Data.Result)
            memcpy(Dst, Data.Data, strlen(Data.Data));
    }

    return Data.Result;
}

int Set_SimCards_Status(char *sIMSI)
{
	char buff[256] = {'\0'};
	char Dst[128] = {'\0'};

	if(!memcmp(sIMSI, "000000000000000000", 18))
		return 0;

	sprintf(buff, "45445,1005,2|{'IMSI':'%s','serverIp':''}&&", sIMSI);
	//LogMsg(MSG_ERROR, "%s\n", buff);

	return Send_Sim_Data(buff, Dst, strlen(buff));
}

static int Login_Log_Pack_Data(Data_Spm *para, char *Dst, AccessAuth_Def *p, int Data)
{
	sprintf(Dst, "333,1001,12|{'SN':'%s','nowtime':'%ld','MCC':'%d','MNC':'%d','LAC':'%d','CID':'%d','Data':'%d',\
'firmWareVer':'%d','firmWareAPKVer':'%d','battery':'%d','minsRemaining':'%d','tdmIpPort':'%s:%d','serverCode':'0'}&&", 
	para->SN, time(NULL), para->MCC, p->MNC, p->LAC, p->CID, Data, p->FirmwareVer, para->versionAPK, p->DeviceSN[21], para->take_time, Web_IP, Web_Port);

	return (int)strlen(Dst);
}

static int Get_Deal_From_SimPool(Data_Spm *para1, char *Src, char *Dst)
{
	int cnt = 0;

	cnt = Get_Deal_From_SimPool_socket(Src, Dst, strlen(Src));
	if(cnt)
    {
        LogMsg(MSG_ERROR, "%s:SN->%s Get_Deal_From_SimPool return %d\n", __func__, para1->SN, cnt);
        switch(cnt)
        {
            case 1:
                return SIM_Allocate_Err(LowPower, para1); //add by lk 20150211    
            case 2:
                return SIM_Allocate_Err(OutOfService, para1);
            case 3:
            case 500:
            default:
                return SIM_Allocate_Err(NoSimUsable, para1);
        }
    }

    return 0;
}

static int Get_Key_Value_Int(char *Src, char *Dst)
{
	char Key[64] = {'\0'};
	int Ret = 0;
	
	if(!Get_Key_Value(Src, Dst, Key, sizeof(Key)))
		Ret = atoi(Key);

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
		Get_Key_Value(Src, "serverInfo", para->ServerInfo, sizeof(para->ServerInfo));	
		if(strlen(para->ServerInfo) < 10)
			para->factoryFlag = 0;
	}

	return para->factoryFlag;
}

static int Get_Key_Value_IMEI(char *Src, char *Dst, long long *IMEI)
{
	char Key[64] = {'\0'};

	if(!Get_Key_Value(Src, "IMEI", Key, sizeof(Key)))
		*IMEI = atoll(Key);

	return 0;
}

static void Get_Key_Value_IPAndPort(char *Src, char *Dst, void *para, void *para1)
{
	char Key[64] = {'\0'};
	char *outer_ptr = NULL;
	char *p = NULL;

	if(!Get_Key_Value(Src, Dst, Key, sizeof(Key)))
	{
		if((p = strtok_r(Key, ":", &outer_ptr)) != NULL)
			memcpy(para, p, strlen(p));

        if((p = strtok_r(NULL, ":", &outer_ptr)) != NULL)
			*(int *)para1 = atoi(p);
	}
}

static void Get_Key_Value_Str_Type(char *Src, char *Dst, void *para, void *para1)
{
    char Key[64] = {'\0'};
    char *outer_ptr = NULL;
    char *p = NULL;

    if(!Get_Key_Value(Src, Dst, Key, sizeof(Key)))
    {
        if((p = strtok_r(Key, "-", &outer_ptr)) != NULL)
        	*(unsigned char *)para = atoi(p);

        if((p = strtok_r(NULL, "-", &outer_ptr)) != NULL)
            *(unsigned char *)para1 = atoi(p);
    }
}

static int Get_Key_Value_Str(char *Src, char *Dst, char *para, int len)
{
	char Key[64] = {'\0'};
	int	len1 = 0;
	
	if(!Get_Key_Value(Src, Dst, Key, sizeof(Key)))
	{
		len1 = (int)strlen(Key);	
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

	if(!Get_Key_Value(Src, "SIMIfActivated", Key, sizeof(Key)))
    {
        printf("the SIMIfActivated is %s, the strlen of Key is %d\n", Key, (int)strlen(Key));
        if(!memcmp(Key, "否", 2))
            para->Vusim_Active_Flag = 1;
		else
            para->Vusim_Active_Flag = 0;
    }

    if(1 == para->Vusim_Active_Flag)
    {
        if(!Get_Key_Value(Src, "Vusim_Active_Num", Key, sizeof(Key)))
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

static int Get_Key_Value_IMSI(char *Src, char *Dst, Data_Spm *para)
{
	char Key[64] = {'\0'};

	if(!Get_Key_Value(Src, "IMSI", Key, sizeof(Key)))
	{
		memcpy(para->sIMSI, Key, strlen(Key));
		str2Hex(Key, para->IMSI, strlen(Key));
	}

	return 0;
}

static int Get_Data_From_Deal(char *Src, Data_Spm *para, char *SN)
{
	para->minite_Remain = Get_Key_Value_Int(Src, "minsRemaining");
	para->lastStart = Get_Key_Value_Int(Src, "lastStart");
	para->ifTest = Get_Key_Value_Int(Src, "ifTest");
	para->orderType = Get_Key_Value_Int(Src, "orderType");
	para->node.ifRoam = Get_Key_Value_Int(Src, "ifRoam");
	para->node.speedType = Get_Key_Value_Int(Src, "speedType");
	para->node.NeedFile = Get_Key_Value_Int(Src, "fileUpdate");
	para->node.IsApn = Get_Key_Value_Str(Src, "APN", para->node.APN, 3);
	Get_Key_Value_VPN(Src, para);
	Get_Key_Value_Factory(Src, para);
	Get_Key_Value_IPAndPort(Src, "IPAndPort", para->node.IP, &para->node.Port);
	Get_Key_Value_Str_Type(Src, "slotNumber", &para->node.SimBank_ID, &para->node.Channel_ID);
	Get_Key_Value_Str(Src, "speedStr", para->speedStr, 4);
	Get_Key_Value_Vusim_Active_Flag(Src, &para->node);
	Get_Key_Value_IMEI(Src, "IMEI", &para->node.IMEI);
	Get_Key_Value_IMSI(Src, "IMSI", para);

	if(Get_Key_Value_Str(Src, "Selnet", para->node.Selnet, 4))
		para->node.IsNet = 0x02;

    if(1 == para->node.speedType)
        para->node.speedType = 0x04;

	para->Result = 0;  

	return para->factoryFlag;
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
	int cnt = 0;
	char buff[128] = {'\0'};	
	char Dst[256] = {'\0'};

	sprintf(buff, "45445445,3002,2|{'SN':'%s','MCC':'%d'}&&", para->SN, para->MCC);

	cnt = Get_Deal_From_SimPool_socket(buff, Dst, strlen(buff));

	if(!cnt)
		Get_Key_Value_Str(Dst, "speedStr", para->speedStr, 4);

	LogMsg(MSG_MONITOR, "the SN is %s speedStr is %s, the Dst is %s\n", para->SN, para->speedStr, Dst);

	return cnt;
}

int Set_Deal_From_SimPool(char *sn, char *imsi, int userCountry, int factoryFlag, int *minite_Remain)
{
	char buff[128] = {'\0'};
	char Dst[256] = {'\0'};

	sprintf(buff, "45445445,3001,2|{'SN':'%s','MCC':'%d'}&&", sn, userCountry);
	
	return Get_Deal_From_SimPool_socket(buff, Dst, strlen(buff));
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
	AccessAuth_Def *p = (AccessAuth_Def *)para1;
	Close_Socket_Fd(&para->socket_spm);
	int cnt = 0;
    char buff[512] = {'\0'};
	char Dst[1024] = {'\0'};

	if(p->MCC == 0)
		p->MCC = 460;

	if(p->MCC != para->MCC)
		para->MCC = p->MCC;

	switch(para->MCC)
	{
		case 454:
		case 455:
		case 460:
        	para->factoryFlag = 1;
        	memcpy(para->ServerInfo, Test_IP, strlen(Test_IP));
        	return SIM_Allocate_Err(IpChange, para); //add by lk 20150211    
		default:
        	memcpy(para->ServerInfo, Master_IP, strlen(Master_IP));
			break;
	}

	Login_Log_Pack_Data(para, buff, p, Data);

	cnt = Get_Deal_From_SimPool(para, buff, Dst);
	if(!cnt)
		if(Get_Data_From_Deal(Dst, para, para->SN))
        	return SIM_Allocate_Err(IpChange, para);

	return cnt;
}

int Deal_Proc(Data_Spm *para, int MCC, int Type)
{
	char buff[356] = {'\0'};
	char Dst[1024] = {'\0'};
	int cnt = -1;
	if(!MCC)
		MCC = 460;

	para->minite_Remain = 0;

	if(Type)
        sprintf(buff, "45445445,3003,3|{'SN':'%s','MCC':'%d','nowtime':'%ld'}&&", para->SN, MCC, time(NULL));
    else
        sprintf(buff, "333,1001,12|{'SN':'%s','nowtime':'%ld','MCC':'%d','MNC':'0','LAC':'0','CID':'0','Data':'0','firmWareVer':'0',\
'firmWareAPKVer':'0','battery':'100','minsRemaining':'0','tdmIpPort':'%s:%d','serverCode':'0'}&&", para->SN, time(NULL), MCC, Web_IP, Web_Port);

	cnt = Get_Deal_From_SimPool(para, buff, Dst);
	if(!cnt)
	{
		para->lastStart = Get_Key_Value_Int(Dst, "lastStart");
		para->minite_Remain = Get_Key_Value_Int(Dst, "minsRemaining");

		if((0 == Type) && (1 == para->lastStart))//TAG_DEAL
			para->minite_Remain = 1440;
	}
	else if(cnt != OutOfService)
	{
		if(Type)
			para->minite_Remain = -1;//TAG_CFMD
		else
			para->minite_Remain = 1;//TAG_DEAL
	}

    return 0;
}

int SIM_Allocate_Proc(Data_Spm *Src)
{
	AccessAuth_Def *p_Src = (AccessAuth_Def *)(Src->Buff);
	unsigned short data_size = 0;//add by lk 20151103

	memcpy(&Src->take_time, p_Src->DeviceSN + 19, 2);
	memcpy(&data_size, p_Src->DeviceSN + 16, 2);//add by lk 20151103
	memcpy(&Src->versionAPK, p_Src->LastIMSI, 4);
	
	return Get_Deal_From_Remote(Src, p_Src, data_size);
}
