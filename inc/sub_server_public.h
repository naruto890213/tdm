#ifndef __SUB_SERVER_PUBLIC__
#define __SUB_SERVER_PUBLIC__

#define SIM_STATUS_unavailable		0
#define SIM_STATUS_available		1

#define SIM_PM_CMD_ADD				1
#define SIM_PM_CMD_DEL				3
#define SIM_PM_CMD_CHG				2
#define SIM_PM_CMD_ACPT_AUTHEN		0
#define	SIM_MS_CMD_ADD_SUB_SERVER	4

#define MAX_Connect 				200
#define RecvBufSize 				512
#define SendBufSize 				1024
#define FileTransferBlockSize 		512
#define FileBlockNum_MAX      		6

#define tag_fcb   					0x7A
#define tag_fcp   					0x7B
#define tag_fct   					0x7C
#define tag_fch   					0x7D
#define tag_atr   					0x7E //ywy add at 20150207

#define TAG_ERR						-1
#define TAG_IP						0x02//add by lk 20150518
#define TAG_ACCESS_AUTH				0x81
#define	TAG_SIMFILE_REQUEST			0x82
#define	TAG_CMDTT					0x83
#define	TAG_HB						0x84
#define	TAG_CFMD					0x85
#define TAG_ADN						0x86
#define TAG_LOCAL_STA				0x87
#define TAG_CONF_FILE_UPDATE		0x88
#define TAG_SWAP_VUSIM				0x89
#define TAG_LOGOUT					0x9C
#define TAG_APN						0x9D
#define TAG_AUTONET                 0x9E //add by lk 20150713
#define TAG_LOW_VSIM				0x9F //add by lk 20150720
#define TAG_REMOTE_RESET_PWD_CMD    0xA1
#define TAG_DEAL					0xB0
#define TAG_DEAL_RET				0xB1

#define TAG_WEB						0x90
#define TAG_GET_APN_INFO			0x8A //add by lk 20150428
#define TAG_HB_NEW					0x8B //add by lk 20150506
#define TAG_UPDATE_LOCAL			0x8C //add by lk 20150507
#define TAG_UPDATE_ROAM				0x8D //add by lk 20150507
#define TAG_UPDATE_LOCAL_APK		0x8E //add by lk 20150507
#define TAG_UPDATE_ROAM_APK			0x8F //add by lk 20150507
#define TAG_UPDATE_MIP				0x98 //add by lk 20150507
#define TAG_MSG						0x99 //add by lk 20150525
#define TAG_LIMIT_SPEED_CMD			0x9A //add by lk 20150525

#define TAG_SHUTDOWN_CMD            0xA0
#define TAG_REINSERT_VSIM_CMD       0xA2
#define TAG_VPN_CMD 				0xA3  //ywy  add 0813
#define TAG_UPDATE_LOCAL_SAPK		0xA4 //add 1105

//#define	TAG_TERMINAL_TURN_OFF		0xA5

#define TAG_VPN_PROFILE_RQST		0xA5//add by 20160104
#define TAG_VPN_RELEASE_CMD			0xA6//add by 20160104
#define TAG_UPDATE_ROAM_PAPK		0xA7//add by 20160216



#define	TAG_RESULT_ACCESS_AUTH		0x91
#define	TAG_RESULT_SIMFILE_REQUEST	0x92
#define	TAG_RESULT_CMDTT			0x93
#define TAG_HB_RET					0x94
#define TAG_LOCAL_STA_RET			0x97
#define TAG_HB_NEW_RET				0x9B
#define TAG_WEB_RET					0x41

#define	SERVER_CONFIG 				"/ServerConfig.ini"
#define SUB_SERVER_DIR 				"/home/SubServer"
#define LEN_OF_IMSI 				9
#define	LEN_OF_ICCID 				10
#define LEN_OF_SN 					15
#define SHARE_FILE_SPM_MS_NAME 		"/SPM_ADD_MS.dat"
#define SHARE_DIR_SPM_MS 			"/home/SubServer/SPM_MS"
#define SHARE_DIR_SPM_TDM 			"/home/Su:Server/SPM_TDM"
#define SHARE_DIR_TDM_MS 			"/home/SubServer/TDM_MS"
#define SPM_DIR 					"/home/SubServer/SPM"
#define	TDM_DIR 					"/home/SubServer/TDM"

#if 1
#define ServerIp 					"192.168.0.110"//add by lk 20150626
#else
#define ServerIp 					"112.74.141.241"//add by lk 20150626
#endif
#define ServerPort 					8090 //add by lk 20150626
#define SpmPort	   					10000

typedef struct
{
    unsigned char	Cmd_TAG;
    unsigned char	FrameSize;//modify by lk 20151217
    unsigned char	Frame_Data[RecvBufSize];
}TD_ComProtocl_RecvFrame_t;

typedef struct
{
    unsigned char Cmd_TAG;
    unsigned char FrameSizeL;
    unsigned char FrameSizeH;
    unsigned char Result;
    unsigned char Frame_Data[SendBufSize];
}TD_ComProtocl_SendFrame_t;

#if 0
typedef struct 
{
	int				Result;
	int				ErrCode;
	int				NeedFile;
	int				dataSpeed;
	int				minite_Remain;
	unsigned long	Data_Remain;
	unsigned char	IMSI[LEN_OF_IMSI];
	char			SN[64];
	int				threshold; //add by lk 20150203
	int			    Vusim_Active_Flag;   //ywy add 20150209
	char			Vusim_Active_Num[128];//ywy add 20150209
	int				IsApn;//add by lk 20150429
	char			apn[256]; // add by lk 20150429
	int				factoryFlag; //add by lk 20150515
	char			ServerInfo[256]; //add by lk 20150515
	unsigned char   dataLimit;
	int             IsNet; //add by lk 20150805
    char            selnet[8];//add by lk 20150805
    int             priority; //add by lk 20150805
	int             port;//add by lk 20150820
    char            ip[16];//add by lk 20150820
}AccessAuth_Result_t;
#endif

#if 0
typedef struct 
{
	int SIM_sta;
	int Com;
	int TDM_pid;
	AccessAuth_Def AA_INFO;
	unsigned char IMSI[LEN_OF_IMSI];
	unsigned char ICCID[LEN_OF_ICCID];
	char clientIp[33];
	time_t AccessTime;
}SIM_DEV_STA;
#endif

typedef struct _Json_date_t
{
    int Head;
    int Type;
    int Para_Num;
    int Result;
    int Len;
    char Data[512];
}Json_date;//add by lk 20150630

typedef struct
{
	unsigned char tag;
	char Name[64];
	int BlockNum;
}SIM_FILE_REQUEST_INFO_t;

typedef struct
{
	unsigned char tag;
	unsigned char TotalBlock;
	unsigned char BlockNum;
	unsigned short BlockSize;
	unsigned char ContentBuf[512];
	unsigned char Cs;
}SIM_FILE_REQUEST_RESULT_t;

typedef struct
{
	int Cmd;
	int Status;
	unsigned char IMSI[16];
	unsigned char ICCID[16];
	int comport;
}data_share_def;

typedef enum
{
	SIM_STA_UNUSABLE,
	SIM_STA_USABLE,
	SIM_STA_USING,
	SIM_STA_OTHERS,
}SIM_STA_DEF;

typedef struct Tdm_to_Spm_t
{
    int             Cmd;
    int             len;
    unsigned char   imsi[9];
    unsigned char   buff[127];
	int             SimBank_ID;//add by lk 20150909
    int             Channel_ID;//add by lk 20150909
}Tdm_to_Spm;

#endif
