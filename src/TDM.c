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

/*****************************************************************************************
sigexit：
	输入参数:
		dunno: 扑捉到的信号值
	输出参数：扑捉到相应的信号值，销毁分配的内存空间，退出程序
******************************************************************************************/
static int setnonblocking(int sockfd)
{
	if (fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0)|O_NONBLOCK) == -1) 
	{
		return -1;
	}

	return 0;
}

static int PickUp_Web_Data(TD_ComProtocl_SendFrame_t *para, int len, void *Src, int fd)
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

    ConnectToMysqlDeInit();
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
static void getValue(Web_Data *para, char *sn, char *src, char *dst)
{
	char Dst[56];

	if(!Find_Str_By_Key(src, "sn", Dst, sizeof(Dst)))
		memcpy(sn, Dst, 15);

	if(!Find_Str_By_Key(src, "type", Dst, sizeof(Dst)))
		para->type = atoi(Dst);

	if(!Find_Str_By_Key(src, "num", Dst, sizeof(Dst)))
	{
		switch(para->type)
		{
			case 8:
			case 10:
			case 11:
			case 15:
				memcpy(dst, Dst, (int)strlen(Dst));
				break;
			default:
				para->num = atoi(Dst);
				break;
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
static int AccessAuth_Result_Pack(Data_Spm *Src, TD_ComProtocl_SendFrame_t *dst)
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
		Set_Dada_IP(&para1, Src->node.IP);

		dst->FrameSizeH = 0;
		dst->Result = AA_RESULT_STATE_SUCCESS;
		dst->FrameSizeL = sizeof(para);

		memcpy(para.imsi, Src->IMSI, LEN_OF_IMSI);
		para.minite_Remain = (int)Src->minite_Remain;

		para.NeedFile = (unsigned char)(Src->node.NeedFile);
		
		if(Src->orderType != 2)
			para.Data_Clr_Flag = Src->lastStart;

		para.Unused[0] = MASK_NUM & (Src->node.IsApn | Src->node.IsNet | Src->node.speedType | Src->ifVPN);//add by lk 20150805

#if 1
		LogMsg(MSG_MONITOR, 
			"the %s sIMSI is %s NeedFile is %d, IsApn is %d, ifVPN is %d the Unused[0] is %d, the IMEI is %lld, ifRoam is %d, the IsNet is %d\n", 
			Src->SN, Src->sIMSI, para.NeedFile, Src->node.IsApn, Src->ifVPN, para.Unused[0], Src->node.IMEI, Src->node.ifRoam, Src->node.IsNet);
#endif

		para.Unused[1] = Src->node.ifRoam;
		para.threshold_data = Src->threshold;
		para.Vusim_Active_Flag = Src->node.Vusim_Active_Flag;
		para.IP[0] = para1.ip[0];
		para.IP[1] = para1.ip[1];
		para.IP[2] = para1.ip[2];
		para.IP[3] = para1.ip[3];
		para.threshold_battery = 6;
		para.system_id = Src->node.SimBank_ID;
		para.channel_id = Src->node.Channel_ID;
		para.byteRemain = Src->node.Port;
		dst_offset = 4 + sizeof(para);
		memcpy(dst->Frame_Data, &para, sizeof(para));

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

	if((para->CMD_TAG == TAG_CMDTT) && (para->Buff[5] != (para->len - 6)))
	{
		Data_IP para1;
		int size = sizeof(para1);
		int len1 = para->len - size;
		memcpy(&para1, para->Buff + len1, size);	

		sprintf(para->node.IP, "%d.%d.%d.%d", para1.ip[0], para1.ip[1], para1.ip[2], para1.ip[3]);
		para->node.Port = para1.port;
		para->node.SimBank_ID = para1.system_id;
		para->node.Channel_ID = para1.channel_id;
		para->len = len1;
	}

	if(para->socket_spm <= 0)
	{
		if(para->node.Port <= 0)
		{
			int ret = Get_Data_IP_SN(para->sIMSI, para);
			if(ret)
			{
				LogMsg(MSG_ERROR, "the %s:imsi->%s spmIp is %s, the spm_port is %d, the ret is %d, SN is %s\n", 
															__func__, para->sIMSI, para->node.IP, para->node.Port, ret, para->SN);
				return 0;
			}
		}

		if(para->node.Port > 0)
		{
			para->socket_spm = ConnectToServerTimeOut(para->node.IP, para->node.Port, 2);
        	if(para->socket_spm <= 0)
        	{
				LogMsg(MSG_ERROR, "the %s:%d imsi->%s spmIp is %s, spm_port is %d, the err is %s\n", 
											__func__, __LINE__, para->sIMSI, para->node.IP, para->node.Port, strerror(errno));
				Set_SimCards_Status(para->sIMSI);//add by lk 20150928
				return 0;
        	}
		}
	}

    Data.Cmd = para->CMD_TAG;
    Data.len = para->len;
	Data.SimBank_ID = para->node.SimBank_ID; //add by lk 20150909
	Data.Channel_ID = para->node.Channel_ID;//add by lk 20150909

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
			LogMsg(MSG_ERROR, "SN:%s %s->Get %d!, err is %s->%s:%d\n", para->SN, __func__, sendbytes, strerror(errno), para->node.IP, para->node.Port);
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

int Coding_With_NewHB(Data_Spm *para)
{
	Log_Data Data;
	memset(&Data, 0, sizeof(Data));
	memcpy(Data.SN, para->SN, 15);
	
	para->type = 1;
	Heart_Log_Pack(para->SN, para->sIMSI, (HB_NEW_t *)(para->Buff), Data.Buff);
	Data.len = strlen(Data.Buff);
	
	threadpool_add_job(para->pool, &Data, sizeof(Data));

	return 0;
}

static int Coding_With_CFMD_Soeckt(Data_Spm *para, unsigned int version, int MCC, int Vsim_Action_State)
{
    unsigned char buff[256] = {'\0'};
    unsigned char sBuff[256] = {'\0'};
    Con_Data Data;
    int length = sizeof(Con_Data);
    int len = 4;
    int sendbytes = -1;
    memset(&Data, 0, length);//add by lk 20151020

    if(version >= 1601080936)
        length = 40;

	if(MCC)
		Deal_Proc(para, MCC, 1);//modify by 20161220

    memcpy(buff, &para->minite_Remain, 4);

	if(0 == para->minite_Remain)
		goto err;

    switch(Vsim_Action_State)
    {
        case 0:
        case 2:
			if(MCC)
			{
            	Get_Config_By_MCC(MCC, Data.config, para);
            	memcpy(buff + 4, &Data, length);
            	len = 4 + length;

            	if(version >= 1605041005)
            	{
            	    memcpy(buff + len, &(para->ifTest), 4);
            	    len = len + 4;
            	}
			}
            break;
        default:
            break;
    }

err:
    sendbytes = TT_Result_Pack1(buff, (TD_ComProtocl_SendFrame_t *)sBuff, len, TAG_CFMD, 0);
    write(para->fd1, (char *)sBuff, sendbytes);

    return 0;
}

static int Coding_With_CFMD(Data_Spm *para)
{
    char Version[12] = {'\0'};
	char iccid[24] = {'\0'};
    Tell_Server_Bill Tell_Server_para;
	Log_Data Data;
	memset(&Data, 0, sizeof(Data));

    memset(&Tell_Server_para, 0, sizeof(Tell_Server_Bill));
    memcpy(&Tell_Server_para, para->Buff, para->len);
    memcpy(para->SN, Tell_Server_para.SN, 15);
    memcpy(Version, Tell_Server_para.Version, 10);
    para->version = atol(Version);//modify by lk 20160111
	memcpy(Data.SN, para->SN, 15);
	memcpy(iccid, Tell_Server_para.ICCID, 20);

	Coding_With_CFMD_Soeckt(para, para->version, Tell_Server_para.MCC, Tell_Server_para.Vsim_Action_State);//add by 20160914

    Hex2String(Tell_Server_para.Imsi, para->sIMSI, LEN_OF_IMSI);
    para->versionAPK = Tell_Server_para.ApkVersion;//add by 20160125

	if(Tell_Server_para.MCC)
    	para->MCC = Tell_Server_para.MCC;

    para->state = Tell_Server_para.Vsim_Action_State;

    switch(Tell_Server_para.Vsim_Action_State)
    {
		case 0:
		case 2:
			para->locale_flag = 1;
			break;
		default:
			break;
    }

	Close_Socket_Fd(&para->socket_spm);
	Local_Confirm_Pack(para->SN, para->sIMSI, Version, para->versionAPK, iccid, Data.Buff);
	Data.len = strlen(Data.Buff);

	threadpool_add_job(para->pool, &Data, sizeof(Data));

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

static int Coding_With_Deal(Data_Spm *para)
{
    unsigned char SendBuf[256] = {'\0'};
    Deal_t Deal_Data;
    TD_ComProtocl_SendFrame_t *p_send = (TD_ComProtocl_SendFrame_t *)SendBuf;

    memcpy(&Deal_Data, para->Buff, sizeof(Deal_t));
    p_send->Cmd_TAG = TAG_DEAL_RET;
    p_send->Result = 0;
    p_send->FrameSizeL = 5;

    Hex2String(Deal_Data.IMSI, para->sIMSI, LEN_OF_IMSI); //add by lk 20150902

    Deal_Proc(para, Deal_Data.MCC, 0);
    memcpy(p_send->Frame_Data, &para->minite_Remain, p_send->FrameSizeL);
	//p_send->Frame_Data[4] = para->lastStart;
	p_send->Frame_Data[4] = 1;

    LogMsg(MSG_MONITOR, "This come from %s imsi is %s, SN is %s, MCC is %d, the minite_Remain is %d, the lastStart is %d\n",
			 __func__, para->sIMSI, para->SN, Deal_Data.MCC, para->minite_Remain, para->lastStart);

    write(para->fd1, (char *)SendBuf, p_send->FrameSizeL + 4);

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
	}
	else
	{
		p_send->Result = 1;
		p_send->FrameSizeL = 0;
	}

	write(para->fd1, (char *)SendBuf, p_send->FrameSizeL + 4);

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
		p_send->Result = 1;
	else
		p_send->Result = 0;
	
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

	if(para->fd1 > 0)
		para->fd1 = 0;

#if 1
	now = time(NULL);
	if((now - para->outtime) <= 25)
		return 0;
#endif

	para->outtime = now;

	memcpy(Data.SN, logout_t.SN, 15);
	LOG_OUT_Log_Pack(logout_t.SN, para->sIMSI, Data.Buff);
	Data.len = strlen(Data.Buff);

	threadpool_add_job(para->pool, &Data, sizeof(Data));
	printf("This come from %s the Buff is %s\n", __func__, Data.Buff);

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
	p_send->FrameSizeL = strlen(para->node.Vusim_Active_Num);
	memcpy(p_send->Frame_Data, para->node.Vusim_Active_Num, p_send->FrameSizeL);	

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

static int Coding_With_Local(Data_Spm *para)
{
	Log_Data Data;
	memset(&Data, 0, sizeof(Log_Data));
	memcpy(Data.SN, para->SN, 15);

    para->type = 2;
	Local_Log_Pack(para->SN, para->sIMSI, para->Buff, para->len, para->version, Data.Buff);
	Data.len = strlen(Data.Buff);

	threadpool_add_job(para->pool, &Data, sizeof(Data));

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
        cnt = cnt - 256; //add by lk 20150427
                                      
    SendBuf[0] = TAG_LOCAL_STA_RET;   
    SendBuf[3] = cnt;
	write(fd, (char *)SendBuf, 4);

	if((Battery < 3) && (Battery != 0)) 
	{
  		Coding_With_TT_Reinsert(0, fd, TAG_SHUTDOWN_CMD);
		//LogMsg(MSG_MONITOR, "This come from TAG_LOCAL_STA the SN is %s, the cnt is %d, Battery is %d, after send shutdown_cmd\n", para->SN, cnt, Battery);
	}

	if(para->node == NULL)
		para->node = Get_Node_By_SN(para->SN);

	if(para->node)
	{
		para->node->pool = para->pool;
		para->node->len = para->num;
		para->node->fd1 = fd;

		if(para->node->IMSI[0] != 0x80)
			memcpy(para->node->IMSI, para->IMSI, 9);

		memcpy(para->node->Buff, Buff, sizeof(para->node->Buff));
		Coding_With_Local(para->node);
	}

	return 0;
}

int Coding_With_APN(Data_Spm *para, int socket_fd)
{
	unsigned char SendBuf[SendBufSize];
	memset(SendBuf, 0, sizeof(SendBuf));//add by lk 20150908

	TD_ComProtocl_SendFrame_t *p_send = (TD_ComProtocl_SendFrame_t *)SendBuf;
	p_send->Cmd_TAG = TAG_GET_APN_INFO;
	p_send->Result = 0;
	p_send->FrameSizeL = strlen(para->node.APN);
	memcpy(p_send->Frame_Data, para->node.APN, p_send->FrameSizeL);	

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
	p_send->FrameSizeL = strlen(para->node.Selnet);
	memcpy(p_send->Frame_Data, para->node.Selnet, p_send->FrameSizeL);	
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

	para->socket_spm = PickUp_Data_Spm(SendBuf, para, &sendbytes);
	if(sendbytes > 1)
		sendbytes = Data_Coding_Spm(para, SendBuf, sendbytes, RecvBuf);
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
				sendbytes = TT_Result_Pack(NULL, (TD_ComProtocl_SendFrame_t *)RecvBuf, 2, TAG_RESULT_CMDTT, para->Buff[0]);
				break;
			default:
				break;
		}
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

	return 0;
}

static int Coding_With_ACCESS_AUTH(Data_Spm *para)
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
		sendbytes = Data_Coding_Spm(para, SendBuf, sendbytes, RecvBuf);
	else
		sendbytes = 0;

	if(5 >= sendbytes)
		Set_SimCards_Status(para->sIMSI);//add by lk 20150917
	else if(sendbytes > 5)
		write(para->fd1, (char *)RecvBuf, sendbytes);

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

static void Coding_With_Date(Data_Spm *para, int CMD_TAG)
{
	switch(CMD_TAG)
    {
        case TAG_ACCESS_AUTH:
        {
            Coding_With_ACCESS_AUTH(para);
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
            Coding_With_NewHB(para);
            break;
        }
        case TAG_CFMD:
        {
            Coding_With_CFMD(para);
            break;
        }
        case TAG_LOCAL_STA:
        {
            Coding_With_Local(para);
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
            Coding_With_VPN_RQST(para);
            break;
        }
        case TAG_VPN_RELEASE_CMD:
        {
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

			Coding_With_Date(node, para->CMD_TAG);
		}
	}

	return NULL;
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
    int recvbytes = 0, fd = 0;
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

    getValue(&para1, sn, (char *)RecvBuf, dst);//add by lk 20150525
	node = get_data_list_by_SN(sn);
	if(node)
		fd = node->fd1;

	if(fd > 0)
	{
		write(iclient_sock, "ok\n", 3);
	}
	else
	{
		write(iclient_sock, "faild\n", 6);
		return -2;
	}

    switch(para1.type)
    {
        case Web_Rom_Log:
        case Web_Loc_Log:
            para.Cmd_TAG = TAG_WEB_RET;
			PickUp_Web_Data(&para, 2, &para1, fd);
			break;

        case Web_Rom_Up:
        case Web_Loc_Up:
        case Web_Rom_Up_Apk:
		case Web_Loc_Up_Apk:
		case Web_Set_Up_Apk://add by lk 20151105
		case Web_Roam_Up_Papk:
        case Web_MIP_Up:
		case 110:
		case 111:
		case 112:
		case 113:
		case 114:
		case 115:
		case 116:
		case 117:
		case 118:
		case 119:
		case 120:
		case 121:
		case 122:
		case 123:
		case 218:
		case 219:
		case 220:
		case 221:
		case 222:
		case 223:
			para.Cmd_TAG = TAG_UPDATE_ROAM;
			PickUp_Web_Data(&para, 2, &para1, fd);
			break;

        case Web_iMsg:
            para.Cmd_TAG = TAG_MSG;
			PickUp_Web_Data(&para, strlen(dst), dst, fd);
            break;

        case Web_SWAP_VUSIM://add by lk 20150531
            para.Cmd_TAG = TAG_SWAP_VUSIM;
			PickUp_Web_Data(&para, 0, NULL, fd);
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
	RegisterLogMsg(TDM_LOG_FILE_NAME, TDM_LOG_FILE_NAME_ERR, TDM_LOG_LEVEL);
	ConnectToMysqlInit();

	return 0;
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
                printf("accept form %s:%d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);

                if (curfds >= MAXEPOLLSIZE)
                {
                    fprintf(stderr, "too many connection, more than %d\n", MAXEPOLLSIZE);
                    close(connfd);
                    continue;
                }

				if (setnonblocking(connfd) < 0) 
					perror("setnonblocking error");

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
            break;
    }

	if(Is_Valid_SN(para->SN))
		return CMD_TAG;

	return -1;
}

static void Coding_With_Out_Sql(tmd_pthread *para, int *CMD_TAG, int fd)
{
	if(para->node == NULL)
    {
		para->node = Get_Node_By_SN(para->SN);
	}	

	if(para->node)
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

	*CMD_TAG = 10;

	return;
}

static void Coding_With_Frame_Data(TD_ComProtocl_RecvFrame_t *p_Recv, tmd_pthread *para, unsigned char *TempBuf)
{
	memcpy(TempBuf, p_Recv->Frame_Data, p_Recv->FrameSize);
	para->num = p_Recv->FrameSize;

	if(p_Recv->Cmd_TAG == TAG_ACCESS_AUTH)
		memcpy(para->SN, ((AccessAuth_Def *)(p_Recv->Frame_Data))->DeviceSN, 16);
	
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
        return -1;

	TD_ComProtocl_RecvFrame_t *p_Recv = (TD_ComProtocl_RecvFrame_t *)buf;
    CMD_TAG = Cmd_Test(p_Recv->Cmd_TAG);
	if(-1 == CMD_TAG)
		return CMD_TAG;

    if(p_Recv->FrameSize == 0)
        para->num = 0;
    else
        para->num = p_Recv->FrameSize - 9;

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
    int one = 1;
    evutil_socket_t listener;
    struct sockaddr_in sin;
	common_data data;
	memset(&data, 0, sizeof(data));
    struct event_base *base = NULL;
    struct event *listener_event;

	struct threadpool *pool = threadpool_init(POOL_NUM, POOL_NUM * 5, work_func);
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

	LogMsg(MSG_MONITOR, "TDM:Listen to Port %d\n", port);

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
	TDM_init(NULL);
	signal(SIGINT, sigexit);
	signal(SIGSEGV, sigexit);

	sigset_t signal_mask;
    sigemptyset(&signal_mask);
    sigaddset(&signal_mask, SIGPIPE);
    int rc = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
    if(rc != 0)
        printf("block sigpipe error\n");

	int server_Sockfd1 = CreateListenService(Web_Port, MAX_Connect);
	if(server_Sockfd1 > 0)
		pthread_create(&web_t, NULL, Create_Epoll_Pthread, &server_Sockfd1);

	run(11113, do_accept);

	return 0;
}
