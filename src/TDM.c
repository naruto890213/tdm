#include <stdio.h>   
#include <sys/types.h>   
#include <sys/socket.h>   
#include <netinet/in.h>   
#include <arpa/inet.h>   
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <fcntl.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <assert.h>

#include "sock_base.h"
#include "pravitelib.h"
#include "sub_server_public.h"
#include "errlog.h"
#include "tdm.h"
#include <pthread.h>

/*****************************************************************************************
本文件的函数主要是tdm模块中的操作函数
******************************************************************************************/
char locale_ip[20]; //add by lk 20140305
#define MASK_NUM 0x1F

//用于多线程中锁住数据库，使持有同一socket的多线程只能依次发送数据
pthread_mutex_t mysql_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t test_mutex = PTHREAD_MUTEX_INITIALIZER;
int mysql_fd = -1;//add by lk 20150628
Mysql_Fd Data_Fd;
extern int PickUp_Web_Data(TD_ComProtocl_SendFrame_t *para, int len, void *Src, int fd);
Data_Spm *Get_Node_By_SN(char *SN);
pthread_t sim_manage;

/*****************************************************************************************
sigexit：
	输入参数:
		dunno: 扑捉到的信号值
	输出参数：扑捉到相应的信号值，销毁分配的内存空间，退出程序
******************************************************************************************/
int setnonblocking(int sockfd)
{
	if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1) 
	{
		return -1;
	}

	return 0;
}

//add by 20160424
static int Is_Valid_SN(char *SN)
{
	unsigned long long SN_NUM = 0;

	if(SN)
		SN_NUM = atoll(SN);

	if((SN_NUM > 172150210000000 && SN_NUM < 172150211000000) || (SN_NUM > 860172008100000 && SN_NUM < 860172009000000))
		return 1;

	LogMsg(MSG_ERROR, "The SN is %s, the SN_NUM is %lu\n", SN, SN_NUM);
	return 0;
}

static void sigexit(int dunno)
{
	switch (dunno)
    {
        case SIGABRT:
            LogMsg(MSG_MONITOR, "Get a signal -- SIGABRT \n");
			break;
        case SIGINT:
            LogMsg(MSG_MONITOR, "Get a signal -- SIGINT \n");
			break;
        case SIGSEGV:
            LogMsg(MSG_MONITOR, "Get a signal -- SIGSEGV \n");
			break;
		default:
			break;
    }

    ConnectToMysqlDeInit(&Data_Fd);
    exit(-1);
}

/*****************************************************************************************
getValue：
	输入参数:
		para: Web结构体指针，用于保存数值
		sn: 用于保存解析出来的SN
		src: 传入的数据
	输出参数：扑捉到相应的信号值，销毁分配的内存空间，退出程序
******************************************************************************************/
void getValue(Web_Data *para, char *sn, char *src, char *dst, int *fd1, int *fd2)
{
    char *str = strstr(src, ":");
    str = strtok(str, ",");
	if(str)
		*fd1 = atoi(str + 1);

    str = strtok(NULL, ",");
	if(str)
	{
		str = strstr(str, ":");
		if(str)
			*fd2 = atoi(str + 1);
	}

    str = strtok(NULL, ",");
	if(str)
	{
		str = strstr(str, ":");
		if(str)
			para->type = atoi(str + 1);
	}

    str = strtok(NULL, ",");
	if(str)
	{
		str = strstr(str, ":");
		if(str)
			memcpy(sn, str + 2, 15);
	}

    str = strtok(NULL, "}");
	if(str)
	{
		str = strstr(str, ":");
		if(str)
		{
			switch(para->type)
			{
				case 8:
				case 10:
				case 11:
				case 15:
					//printf("the str is %s, the len of str is %d\n", str + 2, (int)strlen(str + 2));
					memcpy(dst, str + 2, (int)strlen(str + 2) - 1);
					break;
				default:
					para->num = atoi(str + 1);
					break;
			}
		}
	}
}

static int Coding_With_TT_Reinsert(int len, int fd, int CMD_TAG)//add by 20160309
{
	TD_ComProtocl_SendFrame_t para;
	memset(&para, 0, sizeof(para));
	para.Cmd_TAG = CMD_TAG;
	PickUp_Web_Data(&para, 0, NULL, fd);

	return 0;
}

/*****************************************************************************************
AccessAuth_Result_Pack：
	该函数主要用于对认证结果进行封包处理
	输入参数:
		Src: 设备认证结果结构体指针
		dst: 用于封装认证结果
	输出参数：返回结构体大小
******************************************************************************************/
int AccessAuth_Result_Pack(Data_Spm *Src, TD_ComProtocl_SendFrame_t *dst)
{
	unsigned int dst_offset = 0;

	dst->Cmd_TAG = TAG_RESULT_ACCESS_AUTH;
	if(Src->Result > 0)
	{
		dst->FrameSizeH = 0;
		dst->FrameSizeL = 1;
		dst->Result = AA_RESULT_STATE_FAIL;
		
		if(Src->Result == SockErr)
			dst->Frame_Data[0] = IpChange;
		else
			dst->Frame_Data[0] = (unsigned char)(Src->Result);
			
		if(1 == Src->factoryFlag)
		{
			memcpy(dst->Frame_Data + 1, &(Src->ServerInfo), strlen(Src->ServerInfo));
			dst_offset = 5 + strlen(Src->ServerInfo);
			//printf("\n");
		}
		else
			dst_offset = 5;
	}
	else //add by lk 20150203
	{
		TD_Result_t para;
		Data_IP para1;	
		memset(&para1, 0, sizeof(para1));
		memset(&para, 0, sizeof(para));	
		Set_Dada_IP(&para1, Src->node->IP);

		dst->FrameSizeH = 0;
		dst->Result = AA_RESULT_STATE_SUCCESS;
		dst->FrameSizeL = sizeof(para);

		memcpy(para.imsi, Src->IMSI, LEN_OF_IMSI);
		para.minite_Remain = (int)Src->minite_Remain;

		if(Src->node)
			para.NeedFile = (unsigned char)(Src->node->NeedFile);

		para.RequestFlag = 0;
		para.UpdateFlag = 0;
		
		if(Src->orderType != 2)
		{
			para.Data_Clr_Flag = Src->lastStart;
		}

		//printf("This come from %s, the SN is %s, IsNet:%d, IsApn:%d, speedType:%d, ifVPN:%d\n\n", __func__, Src->SN, Src->node->IsNet, Src->node->IsApn, Src->node->speedType, Src->ifVPN);

		para.Unused[0] = MASK_NUM & (Src->node->IsApn | Src->node->IsNet | Src->node->speedType | Src->ifVPN);//add by lk 20150805

#if 1
		printf("the NeedFile is %d, IsApn is %d, ifVPN is %d the Unused[0] is %d, the IMEI is %lld, ifRoam is %d, the Src->node->IsNet is %d\n", 
				para.NeedFile, Src->node->IsApn, Src->ifVPN, para.Unused[0], Src->node->IMEI, Src->node->ifRoam, Src->node->IsNet);
#endif

		para.Unused[1] = Src->node->ifRoam;
		para.threshold_data = Src->threshold;
		para.Vusim_Active_Flag = Src->node->Vusim_Active_Flag;
		para.IP[0] = para1.ip[0];
		para.IP[1] = para1.ip[1];
		para.IP[2] = para1.ip[2];
		para.IP[3] = para1.ip[3];
		para.threshold_battery = 6;
		para.system_id = Src->node->SimBank_ID;
		para.channel_id = Src->node->Channel_ID;
		para.byteRemain = Src->node->Port;
		dst_offset = 4 + sizeof(para);
		memcpy(dst->Frame_Data, &para, sizeof(para));
		//printf("The Port is %d, IP is %d.%d.%d.%d\n", para.byteRemain, para.IP[0], para.IP[1], para.IP[2], para.IP[3]);

#if 0
		if(Src->node->IMEI)
		{
			memcpy(dst->Frame_Data + sizeof(para), &(Src->node->IMEI), 8);
			dst_offset = dst_offset + 8;
		}
#endif
	}
	//printf("AccessAuth_Result_Pack:dst_offset=%d\n", dst_offset);

	return dst_offset;
}

/*****************************************************************************************
TT_Result_Pack：
	该函数主要是用于处理透传结构体打包
	输入参数:
		Src：需要发送的数据
		Dst: 接收到的数据内容
		num: 结构体实际数据的长度
		CMD：结构体头部的CMD类型
		Result：非零值表示异常，0值表示正常
	输出参数：返回封包后的数据大小
******************************************************************************************/
int TT_Result_Pack(TT_RESULT_t * Src, TD_ComProtocl_SendFrame_t *Dst, int num, int CMD, int Result)
{
	Dst->Cmd_TAG = CMD;
	Dst->FrameSizeH = 0;
	Dst->FrameSizeL = num;
	while(Result > 255)
    {
        Result = Result - 256; //add by lk 20150427
    }

	Dst->Result = Result;
	if(Src != NULL)
		memcpy(Dst->Frame_Data, Src->ReData, num);

	return (int)(4 + num);
}

int TT_Result_Pack1(unsigned char *Src, TD_ComProtocl_SendFrame_t *Dst, int len, int CMD, int Result)
{
	Dst->Cmd_TAG = CMD;
	Dst->FrameSizeH = 0;
	Dst->FrameSizeL = len;
	while(Result > 255)
    {
        Result = Result - 256; //add by lk 20150427
    }

	Dst->Result = Result;
	if(Src != NULL)
		memcpy(Dst->Frame_Data, Src, len);

	return (int)(4 + len);
}

/*****************************************************************************************
TT_Info_Process：
	该函数主要是用于处理TDM和SPM直接的数据交互
	输入参数:
		IMSI：imsi数据
		Src：需要发送的数据
		Dst: 接收到的数据内容
		iLen：需要发送的数据长度
	输出参数：返回交互后的数据长度
******************************************************************************************/
int TT_Info_Process(unsigned char *Src, TT_RESULT_t *Dst, int iLen)
{
    memcpy(Dst->ReData, Src, iLen);

    return iLen;
}

int PickUp_Data_Spm(unsigned char *Dst, Data_Spm *para, int *sendbytes)
{
    Tdm_to_Spm Data;
    memset(&Data, 0, sizeof(Data));

	if((para->CMD_TAG == TAG_CMDTT) && (para->Buff[5] != (para->len - 6)) && (para->node->Port <= 0))
	{
		Data_IP para1;
		int size = sizeof(para1);
		int len1 = para->len - size;
		memcpy(&para1, para->Buff + len1, size);	

		sprintf(para->node->IP, "%d.%d.%d.%d", para1.ip[0], para1.ip[1], para1.ip[2], para1.ip[3]);
		para->node->Port = para1.port;
		para->node->SimBank_ID = para1.system_id;
		para->node->Channel_ID = para1.channel_id;
		para->len = len1;
	}

	if(para->socket_spm <= 0)
	{
		if(para->node->Port <= 0)
		{
			int ret = Get_Data_IP_SN(para->sIMSI, para);
			if(ret)
			{
				LogMsg(MSG_ERROR, "the %s:imsi->%s spmIp is %s, the spm_port is %d, the ret is %d, SN is %s\n", 
															__func__, para->sIMSI, para->node->IP, para->node->Port, ret, para->SN);
				return 0;
			}
		}

		if(para->node->Port > 0)
		{
			para->socket_spm = ConnectToServerTimeOut(para->node->IP, para->node->Port, 2);
        	if(para->socket_spm <= 0)
        	{
				LogMsg(MSG_ERROR, "the %s:%d imsi->%s spmIp is %s, spm_port is %d, the err is %s\n", 
											__func__, __LINE__, para->sIMSI, para->node->IP, para->node->Port, strerror(errno));
				Set_SimCards_Status(para->sIMSI);//add by lk 20150928
				return 0;
        	}
		}
	}

    Data.Cmd = para->CMD_TAG;
    Data.len = para->len;
	Data.SimBank_ID = para->node->SimBank_ID; //add by lk 20150909
	Data.Channel_ID = para->node->Channel_ID;//add by lk 20150909

	if((para->len > 0) && (para->Buff != NULL))
    	memcpy(Data.buff, para->Buff, para->len);

	if(para->IMSI)
    	memcpy(Data.imsi, para->IMSI, 9);

    memcpy(Dst, &Data, sizeof(Data));
	*sendbytes = sizeof(Data);

    return para->socket_spm;
}

int Data_Coding_Spm(Data_Spm *para, unsigned char *Src, int len, unsigned char *Dst)
{
	int sendbytes = -1;

	sendbytes = send(para->socket_spm, (char *)Src, len, 0);
    if(sendbytes <= 0)
    {
		LogMsg(MSG_ERROR, "SendMsg to SPM faild!, SN:%s err is %s, the len is %d\n", para->SN, strerror(errno), len);
		sendbytes = -12;
    }
	else
	{
		sendbytes = GetMsgFromSock(para->socket_spm, (char *)Dst, 0, 0, 2);
		if(sendbytes <= 0)
    	{
			LogMsg(MSG_ERROR, "SN:%s %s->Get %d!, err is %s->%s:%d\n", para->SN, __func__, sendbytes, strerror(errno), para->node->IP, para->node->Port);
			sendbytes = -13;
    	}
	}

	if(sendbytes <= 0)
	{
		if(para->socket_spm > 0)
			close(para->socket_spm);

    	para->socket_spm = -1;
	}

	return sendbytes;	
}

int Send_Change_IP(Data_Spm *para)
{
	unsigned char buff[4];
	int sendbytes = 4;
	memset(buff, 0, sizeof(buff));

	buff[0] = TAG_IP;

	switch(para->MCC)
	{
		case 454:
		case 455:
		case 460:
    		Set_Deal_From_SimPool(para->SN, NULL, para->MCC, 0, NULL);
			break;
		default:
			break;
	}
	para->factoryFlag = 0;
	para->Result = 0;
	write(para->fd1, (char *)buff, sendbytes);

	return 0;
}

int Coding_With_HB_Socket(TD_ComProtocl_RecvFrame_t *p_Recv, tmd_pthread *para, unsigned char *TempBuf, int fd)
{
	unsigned char buff[4] = {'\0'};                       
	buff[0] = TAG_HB_RET;
	write(fd, (char *)buff, 4);

	memcpy(TempBuf, p_Recv->Frame_Data, p_Recv->FrameSize);
	memcpy(para->SN, ((heartBeatWithData_t *)(p_Recv->Frame_Data))->SN, 15);

	if(Is_Valid_SN(para->SN))
	{
		return TAG_HB;
	}
	LogMsg(MSG_ERROR, "The para->SN is %s\n", para->SN);

	return -1;
}

int Coding_With_HB(Data_Spm *para)
{
	if(para->len != 0)
	{
	    heartBeatWithData_t heartBeat;
	    memset(&heartBeat, 0, sizeof(heartBeat));
	    memcpy(&heartBeat, para->Buff, sizeof(heartBeat));
	    memcpy(para->IMSI, heartBeat.IMSI, 9);
	    memcpy(para->SN, heartBeat.SN, 15);
		Hex2String(para->IMSI, para->sIMSI, LEN_OF_IMSI);
	}
	
	return 0;
}

int Coding_With_NewHB_Socket(TD_ComProtocl_RecvFrame_t *p_Recv, tmd_pthread *para, unsigned char *TempBuf, int fd)
{
	unsigned char buff[4] = {'\0'};                 
    buff[0] = TAG_HB_NEW_RET;              
	write(fd, (char *)buff, 4);

	para->num = p_Recv->FrameSize;
	memcpy(TempBuf, p_Recv->Frame_Data, para->num);

	return 0;
}

int Coding_With_NewHB(Data_Spm *para, imsi_node *node)
{
	Log_Data Data;
	memset(&Data, 0, sizeof(Data));
	memcpy(Data.SN, para->SN, 15);
	printf("this come from TAG_HB_NEW: the sn is %s\n\n", para->SN);
	
	if(strcmp(para->sIMSI, "000000000000000000"))
	{
		if(!node)
			add_data_imsi_list(para->sIMSI, time(NULL), &Data_Fd.imsi_list, node);//add by 20160201
	}

	para->type = 1;
	Heart_Log_Pack(para->SN, para->sIMSI, (HB_NEW_t *)(para->Buff), Data.Buff);
	printf("the Buff is %s\n", Data.Buff);
	Data.len = strlen(Data.Buff);
	
	threadpool_add_job(para->pool, &Data, sizeof(Data));
	Delay(0, 20);

	return 0;
}

int Coding_With_CFMD_Soeckt(unsigned char *Buff, int fd, char *SN, int num, tmd_pthread *para)
{
	unsigned char buff[256] = {'\0'};
	unsigned char sBuff[256] = {'\0'};
    char Version[12] = {'\0'};       
	unsigned int version;
    Con_Data Data;                   
    int length = sizeof(Con_Data);   
    int len = 4;                     
	int ifTest = 0;
    int sendbytes = -1;              
    int minite_Remain = -1;          
    Tell_Server_Bill Tell_Server_para;
                                     
    memset(&Data, 0, length);//add by lk 20151020
    memset(&Tell_Server_para, 0, sizeof(Tell_Server_Bill));
    memcpy(&Tell_Server_para, Buff, num);
    memcpy(Version, Tell_Server_para.Version, 10);
	version = atoll(Version);

	pthread_mutex_lock(&Data_Fd.list_mutex);
    para->node = get_data_list_by_SN(para->SN, &Data_Fd.head);
    pthread_mutex_unlock(&Data_Fd.list_mutex);

    if(para->node)
        ifTest = para->node->ifTest;

    if(version >= 1601080936)
    {
        length = 40;
    }
    printf("This come from %s SN:%s Vsim_Action_State is %d, the MCC is %d, the version is %u, ifTest is %d\n\n", __func__, SN, Tell_Server_para.Vsim_Action_State, Tell_Server_para.MCC, version, ifTest);

    switch(Tell_Server_para.Vsim_Action_State)
    {
        case 2:
        case 0:
            sendbytes = Get_Config_By_MCC(Tell_Server_para.MCC, Data.config, SN);
            if(sendbytes == 0)
            {
                LogMsg(MSG_ERROR, "Get_Config_By_MCC->faild!, SN->%s Get_Data_Config return:%d\n", __func__, SN, sendbytes);
            }
            else
            {
                memcpy(buff + 4, &Data, length);
                len = 4 + length;
            }
            printf("This come from TAG_CFMD:%s the Config is %s\n", SN, Data.config);
            break;
        case 1:
            break;
        case 3:
            break;
        default:
            break;
    }

    memcpy(buff, &minite_Remain, 4);

#if 0
	if(version >= 1601080936)
	{
		memcpy(buff + len, &ifTest, 4);
    	len = len + 4;
	}
#endif
    sendbytes = TT_Result_Pack1(buff, (TD_ComProtocl_SendFrame_t *)sBuff, len, TAG_CFMD, 0);	
	write(fd, (char *)sBuff, sendbytes);

	return 0;
}

static int Coding_With_CFMD(Data_Spm *para)
{
    char Version[12] = {'\0'};
    int minite_Remain = -1;
    Tell_Server_Bill Tell_Server_para;
	Log_Data Data;
	memset(&Data, 0, sizeof(Data));

    memset(&Tell_Server_para, 0, sizeof(Tell_Server_Bill));
    memcpy(&Tell_Server_para, para->Buff, para->len);
    memcpy(para->SN, Tell_Server_para.SN, 15);
    memcpy(Version, Tell_Server_para.Version, 10);
    para->version = atol(Version);//modify by lk 20160111

	memcpy(Data.SN, para->SN, 15);

    Hex2String(Tell_Server_para.Imsi, para->sIMSI, LEN_OF_IMSI);
    para->versionAPK = Tell_Server_para.ApkVersion;//add by 20160125
    para->MCC = Tell_Server_para.MCC;
    para->state = Tell_Server_para.Vsim_Action_State;

    switch(Tell_Server_para.Vsim_Action_State)
    {
		case 2:
			Set_Deal_From_SimPool(para->SN, para->sIMSI, para->MCC, 0, &minite_Remain);
		case 0:
			para->locale_flag = 1;
			break;
		case 3:
		default:
			break;
    }

	if(para->socket_spm > 0)
	{
		close(para->socket_spm);
		para->socket_spm = 0;//add by 20160308
	}

	Local_Confirm_Pack(para->SN, para->sIMSI, Version, para->versionAPK, Tell_Server_para.ICCID, Data.Buff);

	Data.len = strlen(Data.Buff);
#if 0
    printf("This come from TAG_CFMD SN:%s Vsim_Action_State is %d, the MCC is %d, the version is %u, the Dst is %s, node is %p\n\n", 
			para->SN, Tell_Server_para.Vsim_Action_State, Tell_Server_para.MCC, para->version, Dst, para->node);
#endif

	threadpool_add_job(para->pool, &Data, sizeof(Data));
	SetFd_From_Device(para->SN, 0, para->fd1);

	return 0;
}

int Coding_With_Low_Vsim(int socket_fd, unsigned char *TempBuf, int num, char *sIMSI)
{
	int sendbytes = -1;
	unsigned char SendBuf[128];
	memset(SendBuf, 0, sizeof(SendBuf));//add by lk 20150908

	unsigned char temp_imsi[12] = {'\0'};
	unsigned char IMSI_T[10] = {'\0'};
	Rqst_Low_Vsim Low_Vsim_t;//add by lk 20150807
	memset(&Low_Vsim_t, 0, sizeof(Low_Vsim_t));
	memcpy(&Low_Vsim_t, TempBuf, num);
	sendbytes = Get_Low_simcards(Low_Vsim_t.SN, Low_Vsim_t.Mcc, temp_imsi);
	if(1 == sendbytes)
	{
		sendbytes = TT_Result_Pack1(IMSI_T, (TD_ComProtocl_SendFrame_t *)SendBuf, 9, TAG_LOW_VSIM, sendbytes);//add by lk 20150408
	}
	else
		sendbytes = TT_Result_Pack1(NULL, (TD_ComProtocl_SendFrame_t *)SendBuf, 0, TAG_LOW_VSIM, sendbytes);//add by lk 20150408

	PutMsgToSock(socket_fd, (char *)SendBuf, sendbytes);

	return 0;
}

int Coding_With_Deal(Data_Spm *para)
{
    unsigned char SendBuf[256] = {'\0'};
    Deal_t Deal_Data;
    TD_ComProtocl_SendFrame_t *p_send = (TD_ComProtocl_SendFrame_t *)SendBuf;

    memcpy(&Deal_Data, para->Buff, sizeof(Deal_t));
    p_send->Cmd_TAG = TAG_DEAL_RET;
    p_send->Result = 0;
    p_send->FrameSizeL = 4;

    Hex2String(Deal_Data.IMSI, para->sIMSI, LEN_OF_IMSI); //add by lk 20150902

    Deal_Proc(para, Deal_Data.MCC);
    memcpy(p_send->Frame_Data, &para->minite_Remain, p_send->FrameSizeL);

    LogMsg(MSG_MONITOR, "This come from %s imsi is %s, SN is %s, MCC is %d, new_start_flag is %d, the minite_Remain is %d\n", 
									__func__, para->sIMSI, para->SN, Deal_Data.MCC, para->new_start_flag, para->minite_Remain);

    write(para->fd1, (char *)SendBuf, p_send->FrameSizeL + 4);

	//Login_Log_Pack(NULL, para, 1);

    return 0;
}

static int Coding_With_VPN_RQST(Data_Spm *para)
{
	int ret = -1;
	int code = 0;
    unsigned char SendBuf[256] = {'\0'};
    TD_ComProtocl_SendFrame_t *p_send = (TD_ComProtocl_SendFrame_t *)SendBuf;
	p_send->Cmd_TAG = TAG_VPN_PROFILE_RQST;

	ret = GetVPN_From_SimPool(para, &code);
	if(ret)	
	{
		p_send->Result = 0;
		p_send->FrameSizeL = strlen(para->vpnc);
		memcpy(p_send->Frame_Data, para->vpnc, p_send->FrameSizeL);
		LogMsg(MSG_MONITOR, "The SN is %s, the vpnc is %s\n", para->SN, para->vpnc);
	}
	else
	{
		p_send->Result = 1;
		p_send->FrameSizeL = 0;
		LogMsg(MSG_MONITOR, "The SN is %s, find not vpn\n", para->SN);
	}

	write(para->fd1, (char *)SendBuf, p_send->FrameSizeL + 4);

	if(ret)
		VPN_Log(para->SN, para->vpnc, ret, code, 0);
	else
		VPN_Log(para->SN, "无", ret, 0, 0);

	return 0;
}

static int Coding_With_VPN_CMD(Data_Spm *para)
{
	unsigned char SendBuf[48] = {'\0'};
	Release_Vpn Data;
	int code = 0;
	char vpn[200] = {'\0'};
	memset(&Data, 0, sizeof(Data));
	memcpy(&Data, para->Buff, sizeof(Data));
    TD_ComProtocl_SendFrame_t *p_send = (TD_ComProtocl_SendFrame_t *)SendBuf;

#if 0
	int i;
	for(i = 0; i < len; i++)
	{
		printf("%02x ", TempBuf[i]);
	}
	printf("\n\n");
#endif

	p_send->Cmd_TAG = TAG_VPN_RELEASE_CMD;
	p_send->FrameSizeL = 0;
	write(para->fd1, (char *)SendBuf, p_send->FrameSizeL + 4);

	if(strlen(Data.Vpn) <= 5)
		return 0;

	if(SetVPN_Status(Data.Vpn, Data.SN, vpn, &code))	
	{
		p_send->Result = 1;
	}
	else
	{
		p_send->Result = 0;
	}
	
	para->vpn_flag = 0;
	VPN_Log(Data.SN, Data.Vpn, 1, 0, 1);

	return 0;
}

static int Coding_With_VPN_CMD_Socket(int fd)
{
	unsigned char SendBuf[48] = {'\0'};
    TD_ComProtocl_SendFrame_t *p_send = (TD_ComProtocl_SendFrame_t *)SendBuf;

	p_send->Cmd_TAG = TAG_VPN_RELEASE_CMD;
	p_send->FrameSizeL = 0;
	p_send->Result = 1;

	write(fd, (char *)SendBuf, 4);

	return 0;
}

int Coding_With_LogOut_Socket(int fd)
{
	unsigned char SendBuf[4] = {'\0'};
	
	SendBuf[0] = TAG_LOGOUT;
	write(fd, (char *)SendBuf, 4);

	return 0;
}

int Coding_With_LogOut(Data_Spm *para)
{
	Device_Logout logout_t;
	time_t now; 
	Log_Data Data;
	
	memset(&Data, 0, sizeof(Data));
	memset(&logout_t, 0, sizeof(logout_t));
	memcpy(&logout_t, para->Buff, sizeof(logout_t));
	Hex2String(logout_t.Imsi, para->sIMSI, 9);

	if((para->MCC == 460) && (para->vpn_flag > 0) && (strlen(para->vpnc) > 5))
	{
		para->vpn_flag = 0;
   		SetVPN_Status(para->vpnc, logout_t.SN, NULL, NULL);	
    	VPN_Log(logout_t.SN, para->vpnc, 1, 0, 1);
	}

	now = time(NULL);
	if(!memcmp(para->sIMSI, "000000000000000000", 18))
		add_data_list(para->sIMSI, now - TIMEOUT_IMST, &Data_Fd.imsi_list);

	if((now - para->outtime) <= 25)
	{
		return 0;
	}
	para->outtime = now;
	printf("This come from Coding_With_LogOut->SN is %s, sIMSI is %s, now is %ld, outtime is %ld\n", logout_t.SN, para->sIMSI, now, para->outtime);

	SetFd_From_Device(logout_t.SN, 0, 0);
	memcpy(Data.SN, logout_t.SN, 15);
	LOG_OUT_Log_Pack(logout_t.SN, para->sIMSI, Data.Buff);
	Data.len = strlen(Data.Buff);

	threadpool_add_job(para->pool, &Data, sizeof(Data));
	Delay(0, 20);

	if(para->socket_spm > 0)
	{
		close(para->socket_spm);
		para->socket_spm = 0;
	}

	return 0;
}

int Coding_With_ADN(Data_Spm *para, int socket_fd)
{
	unsigned char SendBuf[258];
	memset(SendBuf, 0, sizeof(SendBuf));
	TD_ComProtocl_SendFrame_t *p_send = (TD_ComProtocl_SendFrame_t *)SendBuf;
	p_send->Cmd_TAG = TAG_ADN;
	p_send->Result = 0;
	p_send->FrameSizeL = strlen(para->node->Vusim_Active_Num);
	memcpy(p_send->Frame_Data, para->node->Vusim_Active_Num, p_send->FrameSizeL);	

#if 0
	int i;
	printf("this come from TAG_ADN: call_number is %s, the SendBuf is:\n", para->node->Vusim_Active_Num);
	for(i = 0; i < (p_send->FrameSizeL + 4); i++)
	{
		printf("%02x ", SendBuf[i]);
	}	
	printf("\n\n");
#endif

	write(socket_fd, (char *)SendBuf, p_send->FrameSizeL + 4);
	
	return 0;
}

static int Coding_With_Local(Data_Spm *para, imsi_node *node)
{
	Log_Data Data;
	memset(&Data, 0, sizeof(Log_Data));
	memcpy(Data.SN, para->SN, 15);

#if 1
	int cnt = para->cnt;
	if(cnt % 2)
		printf("This come from TAG_LOCAL_STA the SN is %s, the cnt is %d, the node is %p, fd is %d, sIMSI is %s\n\n", para->SN, cnt, para->node, para->fd1, para->sIMSI);
    para->cnt = cnt + 1;
#endif

    if(memcmp(para->sIMSI, "000000000000000000", 18))
    {
		Add_imsi_node_by_Data(&Data_Fd.imsi_list, NULL, para);
    }

    para->type = 2;

	Local_Log_Pack(para->SN, para->sIMSI, para->Buff, para->len, para->version, Data.Buff);
	Data.len = strlen(Data.Buff);

	threadpool_add_job(para->pool, &Data, sizeof(Data));
	Delay(0, 10);

	return 0;
}

int Coding_With_Local_Socket(TD_ComProtocl_RecvFrame_t *p_Recv, unsigned char *Buff, int fd, tmd_pthread *para, int *CMD_TAG)
{
	unsigned char SendBuf[4] = {'\0'};
	memcpy(para->IMSI, p_Recv->Frame_Data, 9);
	memcpy(Buff, p_Recv->Frame_Data, p_Recv->FrameSize);
	para->num = p_Recv->FrameSize;
    int cnt = ((Vsim_UpLoad_Flow *)(Buff))->Upload_Flow_Id;
	int	Battery = ((Vsim_UpLoad_Flow *)(Buff))->Battery;
	*CMD_TAG = 10;

	while(cnt > 255)                  
    {                                 
        cnt = cnt - 256; //add by lk 20150427
    }                                 
                                      
	printf("This come from TAG_LOCAL_STA the SN is %s, the cnt is %d, Battery is %d, after send shutdown_cmd, the para->node is %p\n", para->SN, cnt, Battery, para->node);
    SendBuf[0] = TAG_LOCAL_STA_RET;   
    SendBuf[3] = cnt;
	write(fd, (char *)SendBuf, 4);

	if((Battery < 3) && (Battery != 0)) 
	{
  		Coding_With_TT_Reinsert(0, fd, TAG_SHUTDOWN_CMD);
		//LogMsg(MSG_MONITOR, "This come from TAG_LOCAL_STA the SN is %s, the cnt is %d, Battery is %d, after send shutdown_cmd\n", para->SN, cnt, Battery);
	}

	if(para->node)
	{
		para->node->pool = para->pool;
		para->node->len = para->num;
		para->node->fd1 = fd;

		if(para->node->IMSI[0] != 0x80)
			memcpy(para->node->IMSI, para->IMSI, 9);

		memcpy(para->node->Buff, Buff, sizeof(para->node->Buff));
		Coding_With_Local(para->node, NULL);
	}

	return 0;
}

int Coding_With_APN(Data_Spm *para, int socket_fd)
{
	unsigned char SendBuf[SendBufSize];
	memset(SendBuf, 0, sizeof(SendBuf));//add by lk 20150908

	printf("the APN is %s\n", para->node->APN);

	TD_ComProtocl_SendFrame_t *p_send = (TD_ComProtocl_SendFrame_t *)SendBuf;
	p_send->Cmd_TAG = TAG_GET_APN_INFO;
	p_send->Result = 0;
	p_send->FrameSizeL = strlen(para->node->APN);
	memcpy(p_send->Frame_Data, para->node->APN, p_send->FrameSizeL);	

	write(socket_fd, (char *)SendBuf, p_send->FrameSizeL + 4);

	return 0;
}

int Coding_with_Selnet(Data_Spm *para, int socket_fd)
{
	unsigned char SendBuf[SendBufSize];
	memset(SendBuf, 0, sizeof(SendBuf));//add by lk 20150908
	TD_ComProtocl_SendFrame_t *p_send = (TD_ComProtocl_SendFrame_t *)SendBuf;
	p_send->Cmd_TAG = TAG_AUTONET;
	p_send->Result = 0;
	p_send->FrameSizeL = strlen(para->node->Selnet);
	memcpy(p_send->Frame_Data, para->node->Selnet, p_send->FrameSizeL);	
	//printf("this come from TAG_AUTONET: selnet  is %s\n\n", para->node->Selnet);
	write(socket_fd, (char *)SendBuf, p_send->FrameSizeL + 4);

	return 0;
}

int Coding_With_CMDTT(Data_Spm *para)
{
	para->type = 1;
	int i;
	int sendbytes = -1;
	int result = 0;
	unsigned char SendBuf[256];
	unsigned char RecvBuf[520];
	memset(SendBuf, 0, sizeof(SendBuf));
	memset(RecvBuf, 0, sizeof(RecvBuf));

	para->tt_cnt = para->Buff[0];
	Hex2String(para->IMSI, para->sIMSI, LEN_OF_IMSI);
	if(!para->node)
	{
		Add_imsi_node_by_Data(&Data_Fd.imsi_list, NULL, para);
	}

	para->socket_spm = PickUp_Data_Spm(SendBuf, para, &sendbytes);
	if(sendbytes > 1)
	{
		sendbytes = Data_Coding_Spm(para, SendBuf, sendbytes, RecvBuf);
	}
	else
		sendbytes = 0;

	if(1 == sendbytes)
	{
		result = RecvBuf[0];
		switch(result)
		{
			case 1:
			case 2:
			case 3:
			case 4:
			case 6:
			case 7:
			case 8:
				sendbytes = TT_Result_Pack(NULL, (TD_ComProtocl_SendFrame_t *)RecvBuf, 1, TAG_RESULT_CMDTT, para->Buff[0]);
				Set_SimCards_Status(para->sIMSI);//add by lk 20150917
				para->Result = 3;//add by 20160308
				break;
			case 5:
			case 9:
			case 10:
			case 11:
				printf("this come from -10\n");
				sendbytes = TT_Result_Pack(NULL, (TD_ComProtocl_SendFrame_t *)RecvBuf, 2, TAG_RESULT_CMDTT, para->Buff[0]);
				break;
			default:
				break;
		}
		printf("the result is %d, the RecvBuf[0] is %d\n", result, RecvBuf[0]);
		write(para->fd1, (char *)RecvBuf, sendbytes);//modify by lk 20151020
		TT_Log_Pack_Data(para->SN, (int)para->tt_cnt, para->sIMSI, RecvBuf, -result, para);//modify by lk 20150908
	}
	else if(sendbytes > 1)
	{
		unsigned char tmp_buff[128] = {'\0'};
		i = sendbytes;//add by lk 20151020
		memcpy(tmp_buff, RecvBuf, sendbytes);//add by lk 20151020
		memset(SendBuf, 0, sizeof(SendBuf));//add by lk 20150908
		sendbytes = TT_Info_Process(RecvBuf, (TT_RESULT_t *)SendBuf, sendbytes);
		memset(RecvBuf, 0, sizeof(RecvBuf));//add by lk 20151020	
		sendbytes = TT_Result_Pack((TT_RESULT_t *)SendBuf, (TD_ComProtocl_SendFrame_t *)RecvBuf, sendbytes, TAG_RESULT_CMDTT, para->Buff[0]);
		write(para->fd1, (char *)RecvBuf, sendbytes);

		TT_Log_Pack_Data(para->SN, (int)para->tt_cnt, para->sIMSI, tmp_buff, i, para);//modify by lk 20150908
	}
	else
	{
		sendbytes = TT_Result_Pack(NULL, (TD_ComProtocl_SendFrame_t *)RecvBuf, 1, TAG_RESULT_CMDTT, para->Buff[0]);
		write(para->fd1, (char *)RecvBuf, sendbytes);//modify by lk 20151020
		TT_Log_Pack_Data(para->SN, (int)para->tt_cnt, para->sIMSI, RecvBuf, 0, para);//modify by lk 20150908
	}

#if 0
    LogMsg(MSG_MONITOR, "This come from %s SN:%s -> %s!, the cnt is %d, the sendbytes is %d, the ip is %s, port is %d\n",  __func__, 
			para->SN, para->sIMSI, (int)para->tt_cnt, sendbytes, para->node->IP, para->node->Port);
#endif

	return 0;
}

int Init_Data_Spm(Data_Spm *para)
{
	memset(para, 0, sizeof(Data_Spm));
	para->socket_spm = -1;

	return 0;
}

int Coding_With_ACCESS_AUTH(Data_Spm *para, imsi_node *node)
{
	int sendbytes = 0;
	unsigned char SendBuf[SendBufSize] = {'\0'};

	sendbytes = SIM_Allocate_Proc(para);//add by lk 20150701
	sendbytes = AccessAuth_Result_Pack(para, (TD_ComProtocl_SendFrame_t *)SendBuf);
	if(para->fd1)
		write(para->fd1, (char *)SendBuf, sendbytes);

	para->type = 1;

	return 0;
}

int Coding_With_SIMFILE_REQUEST(Data_Spm *para)
{
	int sendbytes = -1;
	unsigned char SendBuf[SendBufSize] = {'\0'};
	unsigned char RecvBuf[SendBufSize] = {'\0'};

	para->socket_spm = PickUp_Data_Spm(SendBuf, para, &sendbytes);
	if(sendbytes > 1)
	{
		sendbytes = Data_Coding_Spm(para, SendBuf, sendbytes, RecvBuf);
		printf("The TAG_SIMFILE_REQUEST: recv %d bytes from spm\n", sendbytes);
	}
	else
		sendbytes = 0;

	if(5 >= sendbytes)
	{
		Set_SimCards_Status(para->sIMSI);//add by lk 20150917
	}
	else if(sendbytes > 5)
	{
		write(para->fd1, (char *)RecvBuf, sendbytes);
	}

	return 0;
}

int Coding_With_Err(int type, int spm_fd, char *SN)
{
	if(spm_fd > 0)
	{
		close(spm_fd);
	}

	switch(type)
	{
		case 1:
			SetFd_From_Device(SN, 0, -1);
			break;
		case 2:
			SetFd_From_Device(SN, -1, 0);
			break;
	}

	return 0;
}

void Printf(void *Src, int len)
{
	int i;
	char *p = (char *)Src;
	printf("\nthe address of Src is%p->", Src);
	for(i = 0; i < len; i++)	
	{
		printf(" %02x", p[i]);
	}
	printf("\n");
}
/*****************************************************************************************
******************************************************************************************/

static void Coding_With_Date(imsi_node *node, Data_Spm *para, int CMD_TAG)
{
	switch(CMD_TAG)
    {
        case TAG_ACCESS_AUTH:
        {
            Coding_With_ACCESS_AUTH(para, node);
            break;
        }
        case TAG_SIMFILE_REQUEST: //add by ywy 2015-03-05
        {
            Coding_With_SIMFILE_REQUEST(para);
            break;
        }
        case TAG_IP://add by lk 20150518
        {
            Send_Change_IP(para);
            break;
        }
        case TAG_CMDTT:
        {
            Coding_With_CMDTT(para);
            break;
        }
        case TAG_HB:
        {
            Coding_With_HB(para);
            break;
        }
        case TAG_HB_NEW:
        {
            Coding_With_NewHB(para, node);
            break;
        }
        case TAG_CFMD:
        {
            Coding_With_CFMD(para);
            break;
        }
        case TAG_LOCAL_STA:
        {
            Coding_With_Local(para, node);
            break;
        }
		case TAG_LOGOUT:
		{
			Coding_With_LogOut(para);
			break;
		}
        case TAG_LOW_VSIM://add by lk 20150720
        {
            Coding_With_Low_Vsim(para->fd1, para->Buff, para->len, para->sIMSI);
            break;
        }
        case TAG_DEAL:
        {
            Coding_With_Deal(para);
            break;
        }
        case TAG_VPN_PROFILE_RQST:
        {
			LogMsg(MSG_MONITOR, "This come TAG_VPN_PROFILE_RQST SN is %s\n", para->SN);
            Coding_With_VPN_RQST(para);
            break;
        }
        case TAG_VPN_RELEASE_CMD:
        {
			LogMsg(MSG_MONITOR, "This come TAG_VPN_RELEASE_CMD SN is %s\n", para->SN);
            Coding_With_VPN_CMD(para);
            break;
        }
        default:
            break;
	}

	return ;
}

void *Coding_With_CMD_Data_Pool(void *arg)
{
	Node_Data *para = (Node_Data *)arg;
	if(para)
	{
		Data_Spm *node = Get_Node_By_SN(para->SN); 
		if(node)
		{
			node->CMD_TAG = para->CMD_TAG;
			node->fd1 = para->fd;
			node->pool = para->pool;
			node->len = para->len;
			memcpy(node->Buff, para->Buff, sizeof(node->Buff));
			
			if(node->IMSI[0] != 0x80)
				memcpy(node->IMSI, para->IMSI, 9);

			Coding_With_Date(node->node, node, para->CMD_TAG);
		}
	}

	return NULL;
}

int PickUp_Web_Data(TD_ComProtocl_SendFrame_t *para, int len, void *Src, int fd)
{
	int sendbytes = 0;
	unsigned char SendBuf[512];
	memset(SendBuf, 0, sizeof(SendBuf));

    para->FrameSizeL = len;
    sendbytes = 4 + len;

	if(len > 0)
    	memcpy(para->Frame_Data, (char *)Src, len);

    memcpy(SendBuf, para, sendbytes);

#if 0
	int i;
	printf("the para->CMD_TAG is %x, the SendBuf is :\n", para->Cmd_TAG);
	for(i = 0; i < sendbytes; i++)
	{
		printf("%02x ", SendBuf[i]);
	}
	printf("\n");
#endif
	
	if(fd > 0)
		write(fd, (char *)SendBuf, sendbytes);

	return 0;
}

/*****************************************************************************************
Client_Web_Manage：
	该函数主要是用于处理和Web数据请求的交互
	输入参数:
		data： mongoDB数据结构体指针
		iclient_sock：socket文件描述符
	输出参数：非零为失败，零值为正常退出
******************************************************************************************/
int Client_Web_Manage(int iclient_sock)
{
	unsigned char RecvBuf[RecvBufSize] = {'\0'};
    int recvbytes = 0, fd1 = 0, fd2 = 0, fd = 0;
    char sn[16] = {'\0'};
    char dst[256] = {'\0'};
    TD_ComProtocl_SendFrame_t para;
    Web_Data para1;
	Data_Spm *node = NULL;
    memset(&para, 0, sizeof(para));
    memset(&para1, 0, sizeof(para1));

    recvbytes = read(iclient_sock, (char *)RecvBuf, RecvBufSize);
    if(recvbytes <= 0)
    {
        printf("the errno is %d, the recvbytes is %d\n\n", errno, recvbytes);
        return -1;
    }

    printf("this come from TAG_WEB, the RecvBuf is %s\n\n", RecvBuf);
    getValue(&para1, sn, (char *)RecvBuf, dst, &fd1, &fd2);//add by lk 20150525
	node = get_data_list_by_SN(sn, &Data_Fd.head);
	if(node)
	{
		fd = node->fd1;
	}

#if 0
    printf("the type is %d, the num is %d\n", para1.type, para1.num);
    printf("the sn is %s ,the para.FrameSizeH = %d, para.Result = %d\n", sn, para.FrameSizeH, para.Result);
	printf("the fd1 is %d, fd2 is %d, the fd is %d\n", fd1, fd2, fd);
    printf("the dst is %s, the len of dst is %d\n", dst, (int)strlen(dst));
#endif

    switch(para1.type)
    {
        case Web_Rom_Log:
        case Web_Loc_Log:
            para.Cmd_TAG = TAG_WEB_RET;
			PickUp_Web_Data(&para, 2, &para1, fd);
			break;
        case Web_Rom_Up:
            para.Cmd_TAG = TAG_UPDATE_ROAM;
			PickUp_Web_Data(&para, 2, &para1, fd);
			break;
        case Web_Loc_Up:
            para.Cmd_TAG = TAG_UPDATE_LOCAL;
			PickUp_Web_Data(&para, 2, &para1, fd);
			break;
        case Web_Rom_Up_Apk:
            para.Cmd_TAG = TAG_UPDATE_ROAM_APK;
			PickUp_Web_Data(&para, 2, &para1, fd);
			break;
		case Web_Loc_Up_Apk:
            para.Cmd_TAG = TAG_UPDATE_LOCAL_APK;
			PickUp_Web_Data(&para, 2, &para1, fd);
			break;
		case Web_Set_Up_Apk://add by lk 20151105
            para.Cmd_TAG = TAG_UPDATE_LOCAL_SAPK;
            PickUp_Web_Data(&para, 2, &para1, fd);
            break;
        case Web_MIP_Up:
            para.Cmd_TAG = TAG_UPDATE_MIP;
			PickUp_Web_Data(&para, 2, &para1, fd);
			break;
        case Web_iMsg:
            para.Cmd_TAG = TAG_MSG;
			PickUp_Web_Data(&para, strlen(dst), dst, fd);
            break;
        case Web_SWAP_VUSIM://add by lk 20150531
            printf("this come from TAG_SWAP_VUSIM, the fd1 is %d\n", fd1);
            para.Cmd_TAG = TAG_SWAP_VUSIM;
			PickUp_Web_Data(&para, 0, NULL, fd);

////////////////////////////add by 20160309/////////////////////////
			if(node)
			{
				node->Result = OutOfService;
			}
			printf("%s-SN:%s node is %p\n\n", __func__, sn, node);
////////////////////////////////end///////////////////////////////

            break;
        case Web_SPEED:
            para.Cmd_TAG = TAG_LIMIT_SPEED_CMD;
			PickUp_Web_Data(&para, strlen(dst), dst, fd);
			break;
		case Web_APN:
            para.Cmd_TAG = TAG_APN;
			PickUp_Web_Data(&para, strlen(dst), dst, fd);
            break;
        case Web_VPN:
            para.Cmd_TAG = TAG_VPN_CMD;
			PickUp_Web_Data(&para, strlen(dst), dst, fd);
            break;
        case Web_AUTONET:
            para.Cmd_TAG = TAG_AUTONET;
			PickUp_Web_Data(&para, strlen(dst), dst, fd);
            break;
        case Web_Reinsert_Sim: //ywy add 150811
            para.Cmd_TAG = TAG_REINSERT_VSIM_CMD;
			PickUp_Web_Data(&para, 0, NULL, fd);
            break;
        case Web_Remote_shutdown: //ywy add 150811
            para.Cmd_TAG = TAG_SHUTDOWN_CMD;
			PickUp_Web_Data(&para, 0, NULL, fd);
            break;
		case Web_Wifi_PWD:
            para.Cmd_TAG = TAG_REMOTE_RESET_PWD_CMD;
			PickUp_Web_Data(&para, 0, NULL, fd);
			break;
        default:
            break;
    }

    return 0;
}

/*****************************************************************************************
TDM_init：
	该函数主要是用于初始化TDM程序
	输入参数:
		iServer_sock：用于保存创建的监听socket文件描述符
	输出参数：非零为失败，零值为正常退出
******************************************************************************************/
int TDM_init(int *iServer_sock)
{
	int iListenPort;
	char sListenPort[8];
	char SC_PATH[128];
	SetMsgLevel(TDM_LOG_LEVEL);
	RegisterLogMsg(TDM_LOG_FILE_NAME, TDM_LOG_FILE_NAME_ERR, TDM_LOG_LEVEL);
	strcpy(SC_PATH, "/home/SubServer");
	strcat(SC_PATH, SERVER_CONFIG);
	ReadString(SC_PATH, "sub_server", "socket", "port", sListenPort);
	iListenPort = 11111;

	LogMsg(MSG_MONITOR, "TDM:Listen to Port %d\n", iListenPort);

	ConnectToMysqlInit(&Data_Fd);

	return 0;
}

/*****************************************************************************************
sig_chld：
	该函数主要是用于扑捉进程退出
	输入参数:
		signo：信号值
	输出参数：无
******************************************************************************************/
static void sig_chld(int signo)  
{
    while( waitpid(-1, NULL, WNOHANG) > 0 );  
    return;  
}

void *Create_Epoll_Pthread(void *arg)
{
	pthread_detach(pthread_self());
	int socket = *(int *)arg;
    int connfd, kdpfd, nfds, n, curfds = 0;
    struct epoll_event ev;
    struct epoll_event events[MAXEPOLLSIZE];
    struct sockaddr_in cliaddr;

	memset(&ev, 0, sizeof(struct epoll_event));//add by 20160423
    socklen_t socklen = sizeof(struct sockaddr_in);
    char buf[MAXLINE];

    kdpfd = epoll_create(MAXEPOLLSIZE);
    ev.events = EPOLLIN;
    ev.data.fd = socket;
    if(epoll_ctl(kdpfd, EPOLL_CTL_ADD, socket, &ev) < 0)
    {
        fprintf(stderr, "epoll set insertion error: fd=%d\n", socket);
        return NULL;
    }

    curfds = 1;

    while(1)
    {
        nfds = epoll_wait(kdpfd, events, curfds, -1);
        if (nfds == -1)
        {
			if(errno == EINTR)
            	continue;
			else
			{
            	perror("epoll_wait");
				LogMsg(MSG_ERROR, "epoll_wait error, %s", strerror (errno));
				exit(0);
			}
        }

        for(n = 0; n < nfds; n++)
        {
            if(events[n].data.fd == socket)
            {
                connfd = accept(socket, (struct sockaddr *)&cliaddr, &socklen);
                if (connfd < 0)
                {
                    perror("accept error");
                    continue;
				}
				sprintf(buf, "accept form %s:%d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
                printf("%s", buf);

                if (curfds >= MAXEPOLLSIZE)
                {
                    fprintf(stderr, "too many connection, more than %d\n", MAXEPOLLSIZE);
                    close(connfd);
                    continue;
                }

				if (setnonblocking(connfd) < 0) 
				{
					perror("setnonblocking error");
				}

                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = connfd;
                if (epoll_ctl(kdpfd, EPOLL_CTL_ADD, connfd, &ev) < 0)
                {
                    fprintf(stderr, "add socket '%d' to epoll failed: %s\n", connfd, strerror(errno));
                    continue;
                }
                curfds++;
                continue;
            }

			Client_Web_Manage(events[n].data.fd);
			close(events[n].data.fd);//add by lk 20150901
			epoll_ctl(kdpfd, EPOLL_CTL_DEL, events[n].data.fd, &ev);
			curfds--;
        }
    }

	pthread_exit(NULL);
}

/*****************************************************************************************
TDM_Proc：
	该函数主要是TDM程序监听终端设备的主函数
	输入参数:
		iServer_sock：监听的socket文件描述符
	输出参数：无
******************************************************************************************/

#if 1
#define MAX_LINE 65535
void free_tmd_pthread(tmd_pthread *para)
{
	event_free(para->read_event);
	free(para);
	para = NULL;
}

static void Coding_With_TT_Data(TD_ComProtocl_RecvFrame_t *p_Recv, unsigned char *TempBuf, tmd_pthread *para, int socket_fd)
{
	memcpy(para->IMSI, p_Recv->Frame_Data, 9);//add bylk 20150304
    memcpy(TempBuf, p_Recv->Frame_Data + 9, para->num);

    if((strlen(para->SN) < 10) && (para->IMSI[0] == 0x80 && (para->IMSI[1] > 0x90 && para->IMSI[1] <= 0x99)))
    {
        if(TempBuf[5] != (para->num - 6))
        {
            int len = sizeof(Data_IP);
            Data_IP data;
            memset(&data, 0, len);
            memcpy(&data, p_Recv->Frame_Data + p_Recv->FrameSize - len, len);

			if(data.SN < 210000000)
            	sprintf(para->SN, "860172%09d", data.SN);
			else
            	sprintf(para->SN, "172150%09d", data.SN);
        }
        else
        {
            char sIMSI[20] = {'\0'};
            Hex2String(para->IMSI, sIMSI, LEN_OF_IMSI);
            imsi_node *node = get_imsi_list(sIMSI, &Data_Fd.imsi_list);
            if(node)
            {
                memcpy(para->SN, node->lastDeviceSN, strlen(node->lastDeviceSN));
            }
        }
    }

    if(!Is_Valid_SN(para->SN))
    {
        Coding_With_TT_Reinsert(0, socket_fd, Web_SWAP_VUSIM);
    }

	return;
}

static int Coding_With_CMD_Data_Pack(TD_ComProtocl_RecvFrame_t *p_Recv, unsigned char *TempBuf, tmd_pthread *para, int fd, int CMD_TAG)
{
	memcpy(TempBuf, p_Recv->Frame_Data, p_Recv->FrameSize);
    para->num = p_Recv->FrameSize;
	unsigned long long SN_NUM = 0;

    switch(CMD_TAG)
    {
        case TAG_VPN_PROFILE_RQST:
            memcpy(para->SN, ((Rqst_Vpn_Profile *)(p_Recv->Frame_Data))->SN, 15);
            break;
        case TAG_VPN_RELEASE_CMD:
            memcpy(para->SN, ((Release_Vpn *)(p_Recv->Frame_Data))->SN, 15);
            Coding_With_VPN_CMD_Socket(fd);
            break;
        case TAG_DEAL:
            sprintf(para->SN, "%llu", ((Deal_t *)(p_Recv->Frame_Data))->SN);
            break;
        case TAG_LOW_VSIM:
            memcpy(para->SN, ((Rqst_Low_Vsim *)(p_Recv->Frame_Data))->SN, 15);
            break;
        case TAG_LOGOUT:
            memcpy(para->SN, ((Device_Logout *)(p_Recv->Frame_Data))->SN, 15);
            Coding_With_LogOut_Socket(fd);
            break;
        case TAG_CFMD:
            memcpy(para->SN, ((Tell_Server_Bill *)(p_Recv->Frame_Data))->SN, 15);
            Coding_With_CFMD_Soeckt(TempBuf, fd, para->SN, para->num, para);
            break;
    }

	SN_NUM = atoll(para->SN);
	if((SN_NUM > 172150210000000 && SN_NUM < 172150211000000) || (SN_NUM > 860172008100000 && SN_NUM < 860172009000000))
	{
		return CMD_TAG;
	}
	LogMsg(MSG_ERROR, "The SN_NUM is %lu, the para->SN is %s\n", SN_NUM, para->SN);

	return -1;
}

static void Coding_With_Out_Sql(tmd_pthread *para, int *CMD_TAG, int fd)
{
	printf("%s->CMD_TAG is %02x, the node is %p\n", __func__, *CMD_TAG, para->node);

	if(para->node == NULL)
    {
		para->node = Get_Node_By_SN(para->SN);
	}	

	if(para->node)
    {
        if(para->node->node)
        {
            switch(*CMD_TAG)
            {
                case TAG_AUTONET:
                    Coding_with_Selnet(para->node, fd);
                    break;
                case TAG_GET_APN_INFO:
                    Coding_With_APN(para->node, fd);
                    break;
                case TAG_ADN:
                    Coding_With_ADN(para->node, fd);
                    break;
            }
        }
    }

	*CMD_TAG = 10;

	return;
}

static void Coding_With_Frame_Data(TD_ComProtocl_RecvFrame_t *p_Recv, tmd_pthread *para, unsigned char *TempBuf)
{
	memcpy(TempBuf, p_Recv->Frame_Data, p_Recv->FrameSize);
	para->num = p_Recv->FrameSize;
	if(p_Recv->Cmd_TAG == TAG_ACCESS_AUTH)
		memcpy(para->SN, ((AccessAuth_Def *)(p_Recv->Frame_Data))->DeviceSN, 15);
	
	return;
}

static int Cmd_Test(int CMD_TAG)
{
	if(CMD_TAG <= 0)
		return -1;

	switch(CMD_TAG)
    {
        case TAG_ACCESS_AUTH:
        case TAG_SIMFILE_REQUEST:
        case TAG_CMDTT:
        case TAG_HB_NEW:
        case TAG_VPN_PROFILE_RQST:
        case TAG_VPN_RELEASE_CMD:
        case TAG_DEAL:
        case TAG_LOW_VSIM:
        case TAG_LOGOUT:
        case TAG_CFMD:
        case TAG_LOCAL_STA:
        case TAG_HB:
        case TAG_AUTONET:
        case TAG_GET_APN_INFO:
        case TAG_ADN:
        case TAG_IP:
			break;
        default:
			CMD_TAG = -1;
			break;
    }

	return CMD_TAG;
}

static int Coding_With_Recv_Data(int fd, tmd_pthread *para, unsigned char *TempBuf)
{
   	unsigned char buf[512] = {'\0'};
	int CMD_TAG = -1;	
	int len = read(fd, (char *)buf, sizeof(buf));
    if(len <= 0)
    {
        return -1;
    }

	TD_ComProtocl_RecvFrame_t *p_Recv = (TD_ComProtocl_RecvFrame_t *)buf;
    CMD_TAG = Cmd_Test(p_Recv->Cmd_TAG);
	if(-1 == CMD_TAG)
		return CMD_TAG;

    if(p_Recv->FrameSize == 0)
    {
        para->num = 0;
    }
    else
    {
        para->num = p_Recv->FrameSize - 9;
    }

	if(para->num < 0)
		para->num = 0;

    switch(CMD_TAG)
    {
        case TAG_ACCESS_AUTH:
        case TAG_SIMFILE_REQUEST:
			Coding_With_Frame_Data(p_Recv, para, TempBuf);
            break;
        case TAG_CMDTT:
			Coding_With_TT_Data(p_Recv, TempBuf, para, fd);//modify by 20160309
            break;
        case TAG_HB_NEW: //add by lk 20150506
			Coding_With_NewHB_Socket(p_Recv, para, TempBuf, fd);//add by 20160218
            break;
        case TAG_VPN_PROFILE_RQST://add by 20160104
        case TAG_VPN_RELEASE_CMD://add by 20160104
        case TAG_DEAL:
        case TAG_LOW_VSIM:
        case TAG_LOGOUT://add by lk 20150611
        case TAG_CFMD:
			CMD_TAG = Coding_With_CMD_Data_Pack(p_Recv, TempBuf, para, fd, CMD_TAG);
            break;
        case TAG_LOCAL_STA:
			Coding_With_Local_Socket(p_Recv, TempBuf, fd, para, &CMD_TAG);
            break;
        case TAG_HB:
			CMD_TAG = Coding_With_HB_Socket(p_Recv, para, TempBuf, fd);//add by 20160218
            break;
        case TAG_AUTONET://add by lk 20151028
		case TAG_GET_APN_INFO:
		case TAG_ADN:
			Coding_With_Out_Sql(para, &CMD_TAG, fd);
			break;
        case TAG_IP://add by lk 20150518
            break;
        default:
            break;
    }

    return CMD_TAG;
}

void Destory_Node(tmd_pthread *para, int socket_fd)
{
	if(para->node)
	{
		para->node->fd1 = 0;
		if(para->node->socket_spm > 0)
		{
			close(para->node->socket_spm);
			para->node->socket_spm = 0;
		}
	}

	free_tmd_pthread(para);
	close(socket_fd);

	return;
}

Data_Spm *Get_Node_By_SN(char *SN)
{
	Data_Spm *node = NULL;

	if(Is_Valid_SN(SN))
	{
		pthread_mutex_lock(&Data_Fd.list_mutex);
		node = get_data_list_by_SN(SN, &Data_Fd.head);
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
				printf("This come from malloc(sizeof(Data_Spm)) \n");
				list_add_tail(&node->list, &Data_Fd.head);
			}
		}
    	pthread_mutex_unlock(&Data_Fd.list_mutex);

		if(node)
		{
			if(node->socket_spm > 0)
			{
				close(node->socket_spm);
				node->socket_spm = 0;
			}
		}
	}

	return node;
}

void socket_read_cb(int fd, short events, void *arg)
{
	tmd_pthread *para = (tmd_pthread *)arg;
	unsigned char TempBuf[256] = {'\0'};
	Node_Data Data;
    memset(&Data, 0, sizeof(Data));
	
	int CMD_TAG = Coding_With_Recv_Data(fd, para, TempBuf);	
	if(CMD_TAG == -1)
	{
		Destory_Node(para, fd);
		return;
	}
		
	if(CMD_TAG != 10)
	{
		memcpy(Data.SN, para->SN, 16);
		Data.fd = fd;
		Data.len = para->num;
		Data.CMD_TAG = CMD_TAG;
		Data.pool = para->pool;
		memcpy(Data.Buff, TempBuf, sizeof(Data.Buff));
		memcpy(Data.IMSI, para->IMSI, 9);

		threadpool_add_job(para->pool_rt, &Data, sizeof(Data));
	}
}

tmd_pthread *alloc_Data_SPM(common_data *para, evutil_socket_t fd)
{
	struct timeval timeout = {90, 0};
	tmd_pthread *state = malloc(sizeof(tmd_pthread));
	if(!state)
		return NULL;

	memset(state, 0, sizeof(tmd_pthread));
	state->pool = para->pool;
	state->pool_rt = para->pool_rt;
	
	state->read_event = event_new(NULL, -1, 0, NULL, NULL);//仅仅是为了动态创建一个event结构体 
	event_assign(state->read_event, para->base, fd, EV_READ | EV_PERSIST, socket_read_cb, state);
	event_add(state->read_event, &timeout);

	return state;
}

void do_accept(evutil_socket_t listener, short event, void *arg)
{
	common_data *para = (common_data *)arg;
    struct sockaddr_storage ss;
    socklen_t slen = sizeof(ss);
    int fd = accept(listener, (struct sockaddr*)&ss, &slen);
    if (fd < 0) 
	{
        perror("accept");
    } 
	else if (fd > FD_SETSIZE) 
	{
        close(fd);
    } 
	else 
	{
        evutil_make_socket_nonblocking(fd);
		tmd_pthread *client = alloc_Data_SPM(para, fd);
		assert(client);
    }
}

void run(int port, void (*callback)(evutil_socket_t, short, void *))
{
    evutil_socket_t listener;
    struct sockaddr_in sin;
	common_data data;
	memset(&data, 0, sizeof(data));
    struct event_base *base = NULL;
    struct event *listener_event;

	struct threadpool *pool = threadpool_init(POOL_NUM, POOL_NUM * 4, work_func);
	if(NULL == pool)
	{
		LogMsg(MSG_ERROR, "failed to malloc threadpool!\n");	
		return;
	}

	struct threadpool *pool_rt = threadpool_init(POOL_NUM, POOL_NUM * 2, Coding_With_CMD_Data_Pool);
	if(NULL == pool_rt)
	{
		LogMsg(MSG_ERROR, "failed to malloc threadpool pool_rt!\n");	
		return;
	}

    base = event_base_new();
    if (!base)
        return; /*XXXerr*/

	data.base = base;
	data.pool = pool;
	data.pool_rt = pool_rt;

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(port);

    listener = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(listener);

    int one = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0) 
	{
        perror("bind");
        return;
    }

    if (listen(listener, 16) < 0) 
	{
        perror("listen");
        return;
    }

    listener_event = event_new(base, listener, EV_READ|EV_PERSIST, callback, (void*)&data);
    /*XXX check it */
    event_add(listener_event, NULL);

    event_base_dispatch(base);
    event_base_free(base);    
	
	threadpool_destroy(pool);
	threadpool_destroy(pool_rt);
	//printf("This come from %s\n", __func__);
}
#endif

int main(int argc, char ** argv )
{
	pthread_t web_t;

	memset(locale_ip, 0, sizeof(locale_ip));
    GetLocalIp(locale_ip); //add by lk 20150305
	printf("the IP is %s\n", locale_ip);

	TDM_init(NULL);

	signal(SIGINT, sigexit);
	signal(SIGSEGV, sigexit);

#if 1
	sigset_t signal_mask;
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGPIPE);
    int rc = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
    if(rc != 0)
    {
        printf("block sigpipe error\n");
    }
#endif

    if(signal(SIGCHLD, sig_chld) == SIG_ERR )  
	{  
        perror("signal(SIGCHLD) error");  
        exit(errno);  
	}

	int server_Sockfd1 = CreateListenService(Web_Port, MAX_Connect);
	if(server_Sockfd1 > 0)
	{
		pthread_create(&web_t, NULL, Create_Epoll_Pthread, &server_Sockfd1);
	}

	run(11113, do_accept);

	return 0;
}
