#ifndef __TDM_H__
#define	__TDM_H__

#include <pthread.h>
#include "sub_server_public.h" //add by lk 20150203
#include "threadpool.h"
#include "errlog.h"
#include "list.h"

#define MASK_NUM 					0x1F
#define TIMEOUT_IMST				3600
#define MAXEPOLLSIZE 				10000
#define MAX_CON_NUM                 100//add by lk 20151016

#define	tag_MCC   					0x70
#define	tag_MNC   					0x71
#define	tag_CID   					0x72
#define	tag_LOC   					0x73
#define	tag_SN    					0x74

#define BALANCE_NUM                 200//add by lk 20151103

#define	ACCESSAUTHENTICATE_TIMEOUT 	30
#define	RECV_WAIT_TIME            	10
#define MAX_SN_NUM					200

#define SQL_TIMEOUT					600
#define	LOGIN_TIMEOUT				1800

#define Java_TIMEOUT				3
#define SOCKET_NUM					50
#define Web_Port					10160
#define Web_IP						"218.17.107.11"
#define Web_IP1						"113.88.197.13"
#define POOL_NUM					200

/*
the Macro definition for access authenticate of terminal device
*/

#define HOSTNAME 					"test.easy2go.cn"
#define	AA_RESULT_STATE_SUCCESS 	0x31
#define	AA_RESULT_STATE_FAIL    	0x32
#define AA_RESULT_STATE_ERROR   	0x33
#define	AA_RESULT_FILERQ_TRUE 		0x01
#define	AA_RESULT_FILERQ_FALSE 		0x00
#define AA_ERR_SN_NOT_EXIST			0x01
#define AA_ERR_CHARGES_OWED			0x02
#define AA_ERR_VIOLATE_AA_INFO		0x03
#define AA_ERR_NO_SIM_AVALIABLE		0x31
#define AA_ERR_CONNECT_SCM			0x32
#define AA_ERR_SENDTO_SCM			0x33
#define AA_ERR_RECVFROM_SCM			0x34
#define AA_ERR_SENDTO_MS			0x35
#define AA_ERR_RECVFROM_MS_TIMEOUT	0x36
#define AA_ERR_RECVFROM_MS			0x37
#define AA_ERR_SERVER_ABNORMAL		0x38
#define AA_ERR_UNKNOWN				0x39

#define	TDM_LOG_LEVEL 				MSG_MONITOR
#define	TDM_LOG_FILE_NAME 			"/home/run/tdm/TDM.log"
#define	TDM_LOG_FILE_NAME_ERR 		"/home/run/tdm/TDM_err.log"
#define TDM_CONFIG_FILE				"config/IPconfig"

#define Web_Rom_Log					0x01
#define Web_Loc_Log					0x02
#define Web_Rom_Up					0x03
#define Web_Loc_Up					0x04
#define Web_Rom_Up_Apk				0x05
#define Web_Loc_Up_Apk				0x06
#define Web_MIP_Up					0x07
#define Web_iMsg					0x08
#define Web_SWAP_VUSIM				0x09
#define Web_SPEED					0x0A
#define Web_APN						0x0B
#define Web_AUTONET         		0x0C
#define Web_Reinsert_Sim    		0x0D //150811
#define Web_Remote_shutdown 		0x0E
#define Web_VPN             		0x0F//150812
#define Web_Wifi_PWD				0x10 //20150901
#define Web_Set_Up_Apk              0x11 //20151105

#define Web_NUM						0x1C
#define Web_STR             		0x2C
#define Web_RET						0x3C

#define UnConnect           		-2
#define UnWrite             		-3
#define UnRede              		-4
#define	CELL_STATION_BUFLEN 300
                                  
typedef enum
{
	UnrecognizedDevSN = 1,
	NoDealForDevSN,//2
	OutOfService,//3
	NoSimUsable,//4
	MallocErr,//timeout
	IpChange,//6
	LowPower,//7 add by 20160314
	SockErr,//8
}SimAlloc_Err_Code;

typedef enum{
    NETWORK_TYPE_UNKNOWN = 0,
    NETWORK_TYPE_GPRS,
    NETWORK_TYPE_EDGE,
    NETWORK_TYPE_UMTS,
    NETWORK_TYPE_CDMA,
    NETWORK_TYPE_EVDO_0,
    NETWORK_TYPE_EVDO_A,
    NETWORK_TYPE_1xRTT,
    NETWORK_TYPE_HSDPA,
    NETWORK_TYPE_HSUPA,
    NETWORK_TYPE_HSPA,
    NETWORK_TYPE_IDEN,
    NETWORK_TYPE_EVDO_B,
    NETWORK_TYPE_LTE,
    NETWORK_TYPE_EHRPD,
    NETWORK_TYPE_HSPAP,
}Net_Type_Code;//add by 20160303

typedef struct
{
	unsigned short		MCC;
	unsigned char		MNC;
	unsigned char		Sig_Strength;
	unsigned int		LAC;
	unsigned int		CID;
	unsigned int		Local_DataUsed;
}HB_NEW_t;//add by lk 20150511

typedef struct 
{
	unsigned char 		BN;
	unsigned char 		BlockLenH;
	unsigned char 		BlockLenL;
	unsigned char 		BBuf[FileTransferBlockSize];
	unsigned char 		CS;
}FileBlockInfo_Def;

typedef struct
{
	char 				FileName[128];
	unsigned char 		Total;
	FileBlockInfo_Def 	FileContent[FileBlockNum_MAX];
}FileBlock_Def;

typedef struct __Node_Data_t
{
	int 				CMD_TAG;
	int					fd;
	void				*pool;
	int					len;
	char				SN[16];
	unsigned char		IMSI[12];
	unsigned char 		Buff[256];
}Node_Data;

typedef struct __result_sock_t
{
    unsigned char		NeedFile;
    unsigned char		RequestFlag;
    unsigned char		UpdateFlag;
	unsigned char		Vusim_Active_Flag;   //ywy add 20150209

	unsigned char		Data_Clr_Flag;
	unsigned char		Unused[2];
	unsigned char		imsi[9];

    unsigned short		threshold_data;
	unsigned short		dataSpeed;

	int					minite_Remain;

	unsigned short		byteRemain;
	unsigned char		system_id;//add by 20151009
	unsigned char		channel_id;//add by 20151009

	unsigned char		IP[4];//add by 20151008

	unsigned char		threshold_battery;//add by 20160315
	unsigned char		reserve[3];
}TD_Result_t; //add by lk 20150204

typedef struct AA_INFO_LINKLIST_t
{
	unsigned char 		tag;
	unsigned char 		len;
	unsigned char 		*p_value;
	struct AA_INFO_LINKLIST_t *pNext;
}AA_INFO_LINKLIST_t;

typedef struct
{
	unsigned char 		IMSI[LEN_OF_IMSI];
	unsigned char 		APDU_Head[5];
	unsigned char 		Datalen;
	unsigned char 		Data[128];
}TT_INFO_t;

typedef struct
{
	unsigned char 		Len;
	unsigned char 		ReData[128];
}TT_RESULT_t;

typedef struct Rqst_Low_Vsim_t
{
    char 				SN[16];
    int 				Mcc;
}Rqst_Low_Vsim;

typedef struct imsi_info_t
{
    char                IP[16];//IP地址
    char                APN[64];//APN内容

    char                Vusim_Active_Num[126];//sim卡激活号码
    unsigned char       speedType;
    unsigned char       NeedFile;//sim卡文件是否需要强制更新

    char                Selnet[8];
    long long           IMEI;//IMEI号

    unsigned short      Port;//sim卡所在的监听端口号
    char                SimBank_ID;//系统编号
    char                Channel_ID;//通道编号

    unsigned char       ifRoam;
    unsigned char       IsNet;
    unsigned char       IsApn;
    unsigned char       Vusim_Active_Flag;//sim卡是否已经激活
}imsi_info;//add by 20160225

typedef struct __Data_Spm
{
	int					socket_spm;//connect to spm
	int					fd1;//Terminal socket

	unsigned char 		len;
	unsigned char		cnt;
	unsigned char		state;
	char				Result;//add by 20160219

	char				sIMSI[18];
	unsigned char		vir_flag;
	unsigned char	 	ifVPN;	

	char				SN[16];

	unsigned short		take_time;
	unsigned char		type;
	unsigned char 		CMD_TAG;

	unsigned char		new_start_flag;
	unsigned char		locale_flag;
	unsigned char		tt_cnt;
	unsigned char		IMSI[9];

	unsigned int		version;//add by 20160108
	unsigned int   		versionAPK;//add by 20160125
	unsigned char 		Buff[256];

	time_t				lastTime;//simcards time
	time_t				outtime;//logout time
	time_t				TT_time;//TT lastTime

////////////////this is deal_info///////////////////////
	char				speedStr[40];

	unsigned char		lastStart;//订单的最后一个周期重启
	unsigned char		orderType;//订单模式
	unsigned char		factoryFlag;
	unsigned char		vpn;

	unsigned short		MCC;	
	unsigned short		minite_Remain;
	unsigned short		threshold;
	unsigned short		dataLimit;

	time_t				start_time;
	char				ServerInfo[128];		
	int                 ifTest;
/////////////////////end///////////////////////////////

////////////////this is vpn_info//////////////////////
    char                vpnc[127];//add by 20160226
    unsigned char       vpn_flag;
/////////////////////end//////////////////////////////

	imsi_info			node;//add by 20160918
	struct threadpool	*pool;
	struct list_head 	list;
}Data_Spm;//add by lk 20150331

typedef struct _Mysql_Fd
{
	int 				Logs_Fd[SOCKET_NUM];
	pthread_mutex_t 	Logs_mutex[SOCKET_NUM];
	pthread_mutex_t 	list_mutex;//add by 20160123
	struct list_head 	head;//add by 20160123
}Mysql_Fd;//add by lk 20150921

typedef struct
{
    int             	Result;
    int             	ErrCode;
    int             	dataSpeed;
    unsigned short  	minite_Remain;
    unsigned long   	Data_Remain;
    char            	SN[16];
    int             	threshold; //add by lk 20150203
    int             	factoryFlag; //add by lk 20150515
    char            	ServerInfo[256]; //add by lk 20150515
    int             	priority; //add by lk 20150805
    int             	port;//add by lk 20150820
    unsigned short  	logintime;//add by lk 20151029
    unsigned int    	apkversion;//add by 20160125
    unsigned char   	IMSI[LEN_OF_IMSI];
    unsigned char   	dataLimit;
    unsigned char   	IsConfig;//add by lk 20151102
    unsigned char   	orderType;//add by 20160104
	imsi_info			Data;
}AccessAuth_Result_t;

typedef struct __Data_IP
{
	unsigned char		ip[4];
	unsigned short		port;
	unsigned char		system_id;
	unsigned char		channel_id;
	unsigned int		SN;
}Data_IP; //add by lk 20151008

typedef struct
{   
    unsigned long long  SN;
    unsigned short  	MCC;
    unsigned char   	IMSI[9];
}Deal_t;

typedef struct _Config_Data
{
    int     			MCC;
    char    			Config[40];
}Config_Data;//add by lk 20151013

typedef struct _Con_Data_t
{
    char 				config[40];
    char 				vpn[200];//add by lk 20151020
}Con_Data;

typedef struct Vsim_UpLoad_Gps_t
{
	unsigned char		Vsim_Imsi[9];
	unsigned int		Upload_Flow_Id;
	unsigned char		Battery;
	char 				Cell_Station[CELL_STATION_BUFLEN];
}Vsim_UpLoad_Gps;//add by lk 20151217

typedef struct Rqst_Vpn_Profile_t
{
    char 				SN[16];
    unsigned char 		VpnType;//1:大流量 vpn,2：小流量vpn
}Rqst_Vpn_Profile;//add by 20160106

typedef struct Release_Vpn_t
{
    char 				SN[16];
    char 				Vpn[50];//IP|VPN帐号
}Release_Vpn;//add by 20160106

typedef struct int_node_sn
{
    int                 fd1;
    int                 fd2;
    int                 MCC;//add by 20160126
    char                SN[16];
    char                imsi[20];
    char                vpn[127];//add by 20160226
    unsigned char       vpn_flag;
    time_t              lastTime;//simcards time
    time_t              logtime;//login time
    time_t              outtime;//logout time
    struct list_head    list;
}int_node;//add by 20160123

typedef struct free_imsi_t
{
	char 				imsi[800][20];
}free_imsi;//add by 20160123

typedef struct left_imsi_t
{
	int 				num;//add by 20160128
	char 				imsi[800][20];
}left_imsi;//add by 20160123

typedef struct _common_Data_t
{
    struct event_base 	*base;
	struct threadpool 	*pool;
	struct threadpool 	*pool_rt;
}common_data;

typedef struct _tmd_pthread
{
	pthread_t 			client_t;
	int					num;
	char 				SN[16];
	Data_Spm			*node;
	struct event		*read_event;//add by 20160217
	unsigned char 		IMSI[12];
	struct threadpool	*pool;
	struct threadpool	*pool_rt;
}tmd_pthread; //tdm线程数据库

typedef struct _TT_local_date
{
	int					flag;
	unsigned char		Data[128];
}TT_local_date;//add by lk 20150227

typedef struct _Web_Data
{
	unsigned char 		type;
	unsigned char 		num;
}Web_Data; //add by 20150331

typedef struct _Log_Data
{
	char 				SN[16];
	char 				Buff[1004];
	unsigned int		len;
}Log_Data;//add by 20160314

extern int SIM_Allocate_Proc(Data_Spm *Src);
extern int Heart_Log_Pack(char *sn, char *imsi, HB_NEW_t *Src, char *Dst);
extern int SetFd_From_Device(char *SN, int socket1, int socket2);
extern int Set_Deal_From_SimPool(char *sn, char *imsi, int userCountry, int factoryFlag, int *minite_Remain);
extern int LOG_OUT_Log_Pack(char *sn, char *sIMSI, char *Dst);
extern int Get_Low_simcards(char *SN, int code, unsigned char *imsi);
extern int Get_Data_IP_SN(char *imsi, Data_Spm *para);
extern int Set_Sim_Status(char *imsi);
extern void ConnectToMysqlInit(void);
extern void ConnectToMysqlDeInit(void);
extern int Set_SimCards_Status(char *sIMSI);
extern int Set_Dada_IP(Data_IP *para, char *ip);
extern int Get_Config_By_MCC(int code, char *buff, Data_Spm *para);
extern int Deal_Proc(Data_Spm *para, int MCC);
extern int GetVPN_From_SimPool(Data_Spm *para, int *code);
extern int SetVPN_Status(char *vpn, char *SN, char *dst, int *code);
extern int VPN_Log(char *SN, char *vpn, int Result, int code, int type);
extern int add_data_list_by_sn(char *imsi, char *SN, time_t now, time_t logtime, time_t outtime, struct list_head *head);
extern Data_Spm *get_data_list_by_SN(char *SN);
extern int Local_Log_Pack(char *sn, char *imsi, void *Src, int length, unsigned int version, char *Dst);
extern int Local_Confirm_Pack(char *sn, char *imsi, char *Version, int apkversion, char *iccid, char *Dst);
extern void *work_func(void *arg);
extern int Get_VPN_From_SN_List(char *SN, char *vpn);
extern int TT_Log_Pack_Data(char *sn, int cnt, char *imsi, unsigned char *Buff, int len, Data_Spm *para);
extern Data_Spm *Get_Node_By_SN(char *SN);
extern int Is_Valid_SN(char *SN);
extern void Close_Socket_Fd(int *Fd);
#endif
