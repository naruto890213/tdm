#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <errno.h>

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "pravitelib.h"

int GetCharFromKeyboard(char * string)
{
	char buf[100];
	scanf("%s",buf); 
	//printf("%s",buf);
	if(0==strcmp(string,buf))
	{
		return 0;
	}

	return 1;
}


int str2Hex(char *src, unsigned char *dst, int src_len)
{
	int i = 0;
	int j;
	unsigned char buf[2];
	
	unsigned char *p_src = (unsigned char *)src;
	unsigned char *p_dst = dst;
	const char List_Capital[16] = 
	{
		'0','1','2','3',
		'4','5','6','7',
		'8','9','A','B',
		'C','D','E','F',
	};

	const char List_Small[16] = 
	{
		'0','1','2','3',
		'4','5','6','7',
		'8','9','a','b',
		'c','d','e','f',
	};

	do{
		memcpy(buf,p_src,2);
		*p_dst = 0x00;
		j = 0;
		do{
			if((buf[0] == List_Capital[j]) || (buf[0] == List_Small[j]))//High half Byte
			{
				*p_dst = (j<<4);
				break;
			}
			j++;
		}while(j<16);
		if(j==16)
			return 0;
		j = 0;
		do{
			if((buf[1] == List_Capital[j]) || (buf[1] == List_Small[j]))//Low half Byte
			{
				*p_dst += j;
				break;
			}
			j++;
		}while( j < 16);
		if(j==16)
			return 0;

		p_dst++;
		p_src += 2;
		i+=2;
	}while(i < src_len);

	return 1;
}

int Hex2String(unsigned char *src,char *dst,int src_len)
{
	int i;
	unsigned char temp;
	const char List[16] = 
	{
		'0','1','2','3',
		'4','5','6','7',
		'8','9','A','B',
		'C','D','E','F',
	};

	for(i=0;i<src_len;i++)
	{
		temp=(((src[i]) >> 4) & 0x0F);
		*(dst+2*i) = List[temp];
		temp = (src[i]&0x0F);
		*(dst + 2*i + 1) = List[temp];
	}
	*(dst+2*i)='\0';

	return 2*i;
}

void AllTrim (char sBuf[]);

//ReadString(INIT_FILE, "uniqueid", "socket", "socket_ip", host);
//ReadString(INIT_FILE, "uniqueid", "socket", "socket_port", port);

/* 读取配置表中的数据 */
/*配置表的写法如下*/
/*
{database} #BLOCK
[login]        #sSection
loginname= cdma            #sItem and sResult
*/
int ReadString(char * sFilename,char * sBlock,char *sSection,char * sItem,char * sResult)
{
	FILE * fpINI;
	char  sLine[1024];
	char *sTemp,*pTemp;
   
	fpINI = fopen(sFilename ,"r");
	if(fpINI == NULL)
		return -1;

	while(1)
	{
		fgets(sLine, 1024, fpINI);
		if(feof(fpINI))
		{
   			fclose(fpINI);
   			return 0;
   		}
   		sTemp = sLine;
   		while( (*sTemp)==' ' || (*sTemp)=='\t' )
   	  		sTemp++;      /*skip space and tabs*/
   		if( *sTemp != '{' || *(sTemp+1)!='$')
   	  		continue;
   		sTemp++;
   		sTemp++;
   		if(strncasecmp(sTemp,sBlock,strlen(sBlock))!=0)
   	  		continue;
   		sTemp+=strlen(sBlock);
   		if(*sTemp!='}')
   	  		continue;
   	
   		/*block has beed found*/
   		while(1)
   		{
   			fgets(sLine,1024,fpINI);
   			if(feof(fpINI))
   			{
   				fclose(fpINI);
   				return 0;
   			}
   			sTemp =  sLine;
   			while( (*sTemp)==' '|| (*sTemp)=='\t')
   		  		sTemp++;
   		  
   			if( *sTemp == '{' && *(sTemp+1)=='$')
   			{
   				fclose(fpINI);
   				return 0;       /*this is the next block*/
   			}
   			if( *sTemp!='[')
   		  		continue;
   			sTemp++;
   			if(strncasecmp(sTemp,sSection,strlen(sSection))!=0)
   		  		continue;
   		  
   			sTemp+=strlen(sSection);
   			if( *sTemp !=']')
   		  		continue;
   		
   			while(1)
   			{
   				fgets(sLine,1024,fpINI);
   				if(feof(fpINI))
   				{
   					fclose(fpINI);
   					return 0;
   				}
   				sTemp = sLine;
   			
   				while( *sTemp==' '|| *sTemp=='\t')
   			  		sTemp++;
   			
   				if(*sTemp == '[' || (*sTemp=='{' && *(sTemp+1)=='$') )
   				{
   					fclose(fpINI);
   					return 0;
   				}
   			  
   				if(strncasecmp(sTemp,sItem,strlen(sItem))!=0)
   			  		continue;
   			
   				sTemp+=strlen(sItem);
   			
   				while( *sTemp == ' '||*sTemp=='\t')
   			  		sTemp++;
   			
   				if(*sTemp!= '=' )
   			   		continue;
   			
   				sTemp++;
   			
   				fclose(fpINI);

   				/* add by hty trim and ignore comment. */	
   		    	pTemp=strchr(sTemp,'#'); 	
            	if(pTemp!=NULL)*pTemp=0;

   		    	pTemp=strchr(sTemp,'\n'); 	
            	if(pTemp!=NULL)*pTemp=0;
				pTemp=strchr(sTemp,'\r'); 	
            	if(pTemp!=NULL)*pTemp=0;
            		
            	AllTrim(sTemp);
            	/* add ok */                          
            	strcpy(sResult,sTemp);
   				return 1;
   			}
   		}
	}
}

void AllTrim (char sBuf[])
{
	int i, iFirstChar, iEndPos, iFirstPos;
	
	if(sBuf==NULL)
		return;
	iEndPos = iFirstChar = iFirstPos = 0;
	for (i = 0 ; sBuf[i] != '\0'; i++)
	{
		if( iFirstChar == 0 )
		{
			if( sBuf[i] != ' ' )
			{
				sBuf[0] = sBuf[i];
				iFirstChar = 1;
				iEndPos = iFirstPos;
			}
			else
				iFirstPos++;	
		}
		else
		{
			if( sBuf[i] != ' ')
				iEndPos = i;
			sBuf[i-iFirstPos] = sBuf[i];
		}
	}
	if( iFirstChar == 0 )
		sBuf[0] = '\0';
	else
		sBuf[iEndPos-iFirstPos+1]='\0';
	
}


unsigned long get_file_size(const char *path)  
{  
    unsigned long filesize = -1;      
    struct stat statbuff;  
    if(stat(path, &statbuff) < 0){  
        return filesize;  
    }else{  
        filesize = statbuff.st_size;  
    }  
    return filesize;  
}


#if 0
int     mig_log(fmt,va_alist)
char    *fmt ;
va_dcl
{
    va_list ap ;
    FILE    *fp ;
    char    log_file[81] ;
    struct  tm *p_tm ;
    time_t  clock ;

    ap=(char *)&va_alist ;

    time(&clock) ;
    p_tm=localtime(&clock) ;

    sprintf(log_file,"%s/mig_%4d%.2d%.2d.log",
                    getenv("HOME"),
                    p_tm->tm_year+1900,
                    p_tm->tm_mon+1,
                    p_tm->tm_mday) ;

    fp=fopen(log_file,"at") ;
    if(fp==(FILE *)0)
    {
        fprintf(stderr,"mig_err_log():can't open the file %s !\n",log_file);
        return ;
    }
    fprintf(fp,"%.2d/%.2d/%4d %.2d:%.2d:%.2d ",
            p_tm->tm_mon+1,\
            p_tm->tm_mday,\
            p_tm->tm_year+1900,\
            p_tm->tm_hour,\
            p_tm->tm_min,\
            p_tm->tm_sec) ;
    vfprintf(fp,fmt,ap) ;
    fclose(fp) ;
}
#endif
