#ifndef __PRAVITE_LIB__
#define	__PRAVITE_LIB__

typedef union 
{
	unsigned long B64;
	unsigned char B08[8];
}B64_B08;
extern int GetCharFromKeyboard(char * string);
int str2Hex(char *src,unsigned char *dst,int ilen);
int Hex2String(unsigned char *src,char *dst,int src_len);
int ReadString(char * sFilename,char * sBlock,char *sSection,char * sItem,char * sResult);
unsigned long get_file_size(const char *path);

//int     mig_log(fmt,va_alist);

#endif
