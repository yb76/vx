/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       Contactless.c
**
** DATE CREATED:    08 Oct 2012
** DATE Modified:   27 May 2014
**
** AUTHOR:          Dwarika Pandey
**
** DESCRIPTION:     This module contains the ctls EMV functions
**-----------------------------------------------------------------------------
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <svc.h>
#include <svc_sec.h>
#include <math.h>
#include <stdlib.h>
#include <eoslog.h>
#include <ctlsinterface.h>

#include "auris.h"
#include "iris.h"
#include "irisfunc.h"
#include "Contactless.h"
#include "date.h"
#include "emvtags.hpp"
#include "emvcwrappers.h"
#include "ctlsmod.h"
#include "inputmacro.h"
#include "printer.h"

#define VIVO_HDR_STR "ViVOtech"
#define VIVO2_HDR_STR "ViVOtech2"
#define CTLS_BUFF_SZ 5000

//packet field offsets
#define V2_STATUS_OFS 11
#define V2_DAT_LEN_MSB_OFS 12
#define V2_DAT_LEN_LSB_OFS 13
#define V2_DATA_OFS 14
#define V2_CRC_MSB_OFS 14
#define V2_CRC_LSB_OFS 15
#define V2_FXD_BYTES 16
#define V2_ERR_T2_OFS 4
#define V2_T2EQD_SZ 21

//vivo status codes
#define V2_OK 0
#define V2_INCORRECT_TAG_HDR 0x01
#define V2_UNK_CMD 0x02
#define V2_UNK_SUB_CMD 0x03
#define V2_CRC_ERR 0x04
#define V2_INCORRECT_PARAM 0x05
#define V2_PARAM_NOT_SUPPORTED 0x06
#define V2_MAL_FORMATTED_DATA 0x07
#define V2_TXN_TIMED_OUT 0x08
#define V2_FAILED_NAK 0x0A
#define V2_CMD_NOT_ALLOWED 0x0B
#define V2_SUB_CMD_NOT_ALLOWED 0x0C
#define V2_BUFF_OVERFLOW 0x0D
#define V2_USER_IF_EVT 0x0E
#define V2_REQ_OL_AUTH 0x23

//vivo status codes
#define V1_OK 0
#define V1_INC_FRAME_Tag 0x01
#define V1_INC_FRAME_TYP 0x02
#define V1_UNK_FRAME_TYP 0x03
#define V1_UNK_CMD 0x04
#define V1_UNK_SUB_CMD 0x05
#define V1_CRC_ERROR 0x06
#define V1_FAIL 0x07
#define V1_TIMEOUT 0x08
#define V1_INC_PARAM 0x09
#define V1_CMD_NOT_SUPP 0x0A
#define V1_SUB_CMD_NOT_SUPP 0x0B
#define V1_PARAM_NOT_SUPP 0x0C
#define V1_CMD_ABORT 0x0D
#define V1_CMD_NOT_ALW 0x0E
#define V1_SUB_CMD_NOT_ALW 0x0F

#define V1_ACK_FRAME 'A'
#define V1_NACK_FRAME 'N'

#define V1_RTC_NO_ERR 0x00
#define V1_RTC_ERR_UNK 0x01
#define V1_RTC_INV_DATA 0x02
#define V1_RTC_ERR_NO_RTC_RSP 0x03

#define V1_KEYMGT_NO_ERR 0x00
#define V1_KEYMGT_UNK_ERR 0x01
#define V1_KEYMGT_INV_DATA 0x02
#define V1_KEYMGT_DATA_NOT_CMP 0x03
#define V1_KEYMGT_INV_KEY_NDX 0x04
#define V1_KEYMGT_INV_HSH_ALG_IND 0x05
#define V1_KEYMGT_INV_KEY_ALG_IND 0x06
#define V1_KEYMGT_INV_KEY_MOD_Len 0x07
#define V1_KEYMGT_INV_KEY_EXP 0x08
#define V1_KEYMGT_KEY_EXISTS 0x09
#define V1_KEYMGT_NO_SPACE 0x0A
#define V1_KEYMGT_KEY_NOT_FOUND 0x0B
#define V1_KEYMGT_CRYP_CHP_NO_RSP 0x0C
#define V1_KEYMGT_CRYP_CHP_COM_ERR 0x0D
#define V1_KEYMGT_RID_SLT_FULL 0x0E
#define V1_KEYMGT_NO_FREE_SLOTS 0x0F

//vivio error codes
#define V2_NO_ERR 0x00
#define V2_OUT_SEQ_CMD 0x01
#define V2_GOTO_CNT 0x02
#define V2_AMT_ZERO 0x03
#define V2_CRD_ERR_STAT 0x20
#define V2_COLLISION 0x21
#define V2_AMT_OVR_MAX 0x22
#define V2_REQ_OL_AUTH 0x23
#define V2_CRD_BLKD 0x25
#define V2_CRD_EXPD 0x26
#define V2_CRD_NOT_SUPP 0x27
#define V2_CRD_NOT_RSP 0x30
#define V2_UNK_DAT_ELMT 0x40
#define V2_DAT_ELMT_MISS 0x41
#define V2_CRD_GEN_AAC 0x42
#define V2_CRD_GEN_ARQC 0x43
#define V2_CA_PUB_KEY_FAIL 0x50
#define V2_ISS_PUB_KEY_FAIL 0x51
#define V2_SDA_FAIL 0x52
#define V2_ICC_PUB_KEY_FAIL 0x53
#define V2_DYN_SIG_VER_FAIL 0x54
#define V2_PRCS_RST_FAIL 0x55
#define V2_TRM_FAIL 0x56
#define V2_CVM_FAIL 0x57
#define V2_TAA_FAIL 0x58
#define V2_SD_MEM_ERR 0x61

_ctlsStru * p_ctls = NULL;
static unsigned short CTLSDEBUG=0;

enum
{
	TLV_TYPE_PRIMARY = 0,
	TLV_TYPE_CONSTRUCTED = 1
};

static char *sCfgData = NULL;
static long sCfgLength = 0;
static AID_DATA AIDlist[10];

#define RSP_DATA_LEN ((*(rsp+12)<<8)|*(rsp+13))

typedef struct TLV_
{
	short lenTag; // length of tag "DF8129" -> 3
	char sTag[10];
	unsigned long tag; // tag: 0xDF8129
	short lenLen;  // length of len
	long len;
	char val[1024];
	int tagtype;
} TLV;

//////////////////////////////////


typedef struct CfgTlv_ CfgAidTlv;
struct CfgTlv_
{
	long tag;
	int len;
	char *val;
	CfgAidTlv * next;
};

typedef struct CfgAid_ CfgAid;
struct CfgAid_
{
	char sAid[20];
	CfgAidTlv * TLVs;
	CfgAid * next;
};

typedef struct CfgGroup_ CfgGroup;
struct CfgGroup_
{
	short groupno;
	CfgAid * AIDs;
	CfgAidTlv * TLVs;
	CfgGroup * next;
};

CfgGroup *g_cfgGroups = NULL;
CfgAid* CfgAidFind(char *sAid,int *grpno);
CfgAidTlv * CfgTLVAdd(CfgAidTlv *pAidTlv, long Tag, char* sValue);

//CfgAid g_cfgAIDs = NULL;
//CfgAidTlv g_cfgAidTLVs = NULL;
//////////////////////////////////

static int InitComPort(char comPortNumber);
static void ResetMsg(void);
static void ResetRsp(void);
static void AddToMsg(char *subMsg, int length);
static void AddVivoHeader(void);
static void AddVivo2Header(void);
static void AppendCRC(void);
static void FormatAmt(char *amt, char *formattedAmt);
static int  GetNumDigits(char *amt, int length);
static int  CheckCRC(char *rsp);
static int  GetDataLen(char *rsp);

static void BuildSetPollOnDemandMsg(void);
static void BuildActivateTransactionMsg(char timeout);
static void BuildSetEMVTxnAmtMsg(void);
static void BuildCancelTransactionMsg(void);
static void BuildStoreLCDMsgMsg(char msgNdx, char *str, char *paramStr1, char *paramStr2, char *paramStr3);

static void BuildSetDateMsgCF(void);
static void BuildSetTimeMsgCF(void);
static void BuildSetDateMsgDF(void);
static void BuildSetCAPubKeyMsgCF(char dataLen1, char dataLen2);
static void BuildSetCAPubKeyMsgDF(char *data, char len);

static int SetPollOnDemandMode(void);
static int ActivateTransaction(char waitForRsp, int timeout);
static int CancelTransaction(int reason);
static int SetEMVTxnAmt(void);
static int GetPayment(char waitForRsp, int timeout);
static int StoreLCDMsg(char msgNdx, char *str, char *paramStr1, char *paramStr2, char *paramStr3);

static int SetSource(void);
static int SetDateTime(void);
static int SetCAPubKey(char *keyFile);
static int SetCAPubKeys(void);
static int DeleteCAPubKeys(void);

static void InitRdr(char comPortNumber);
static int CbAcquireCard(void *pParams);
static int CbProcessCard(void *pParams);
static int CbCancelAcquireCard(void *pParams);

static int SendRxMsg(int timeout);
static int GetRsp(char waitForRsp, int timeout);
static int AcquireRsp(char waitForRsp, int timeout);
static int ExtractCardDetails(void);
static int ExtractClearingRec(char *clrRec);
static int ExtractTrack(char *trk, char *trkInfo);
static int ExtractEMVDetails(void);
static int ExtractFailedEMVDetails(char *details, int length);

static int HandleErrRsp(void);
static int HandleReqOLErrRsp(void);
static int HandleNoReqOLErrRsp(void);
static long getTlv(char *tlv, TLV *tlvStruct);
static int processEmvTlv(char *tlv);

static int GetRspStatus(void);
static char GetFrameType(void);
static int HandleRTCNFrame(void);
static int HandleKeyMgtNFrame(void);
static int BuildGeneralMsg(char cmd,char subcmd,char *data,short datalen);

static void BuildRawMsg(char* tlv,int tlvlen);
static int SendRestoreRawMsg(void);


/*******************************************************
 * <<EMV4.2 Book3 Annex B>>
 * http://en.wikipedia.org/wiki/Basic_Encoding_Rules#BER_encoding
 * BER-TLV length fields in ISO/IEC 7816
 *       Byte1       Byte2 Byte3 Byte4 Byte5  N
 * 1Byte  00-7F       -     -     -     -     0-127
 * 2Byte  81         00-FF  -     -     -     0-255
 * 3Byte  82         0000 -FFFF   -     -     0-65535
 * 4Byte  83         000000 -  FFFFFF   -     0-16777215
 * ...
 *******************************************************/

static unsigned long BER_parseTag(char *pTag, unsigned long *lTag, long *lTagLen,int *iTagType) {
  unsigned long ber_tag = 0;
  char *p = pTag;
  unsigned char ch = *p;
  int idx = 0;
  
  if ((ch & 0x20) != 0x20) //Primitive data object
  {		if(iTagType) *iTagType = TLV_TYPE_PRIMARY;
  } else { 	//Constructed data object
		if(iTagType) *iTagType = TLV_TYPE_CONSTRUCTED;
  }
  ber_tag = (unsigned long)*p;
  //LOG_PRINTFF(0x00000001L,"parse Tag *p = %02x, tag = %ld", *p, ber_tag); 
  if((*pTag & 0x1F) != 0x1F ) { // TAG number
    if(lTag) *lTag = ber_tag;
    if(lTagLen) *lTagLen = 1;
    return(ber_tag);
  }
  //See subsequent bytes
  do {
    //if(ber_tag > 0xffffff) return(0); // too big
	++p; ++idx;
	ch = (unsigned char )*p;
	ber_tag = ber_tag * 256 + ch;
	
   //LOG_PRINTFF(0x00000001L,"parse Tag *p = %02x, tag = %ld, AND result=%02x", ch, ber_tag, (ch&0x80)); 
  } while((ch & 0x80) == 0x80) ;
  if(lTag) *lTag = ber_tag; 
  if(lTagLen) *lTagLen = idx +1;
  
  return(ber_tag);
}

static unsigned long Verifone_parseTag(char *pTLVs, unsigned long *plTag, long *plTagLen, char *pValue) {
	char *pIdx = pTLVs;
	int lenTag =0,lenLen = 1, lenValue=0 ;
	unsigned long lTag = 0;
	
	if( *pIdx ==0xdf && *(pIdx+1) ==0x81) {
		lenTag = 3;
		lTag = (unsigned char)*pIdx * 256 * 256 + (unsigned char) *(pIdx+1) * 256 + (unsigned char) *(pIdx+2);
	}else if((*pIdx & 0x1F) != 0x1F ) { // TAG number
		lenTag = 1;
		lTag = (unsigned char) *pIdx;
	} else {
		lenTag = 2;
		lTag = (unsigned char) *(pIdx) * 256 + (unsigned char) *(pIdx+1);
	}
	if(plTagLen) *plTagLen = lenTag;
	if(plTag) *plTag = lTag ;
	pIdx = pIdx + lenTag;
	lenValue = (unsigned char)*pIdx;
	++pIdx;
	
	if(pValue && lenValue) UtilHexToString((const char *)pIdx, lenValue , pValue); 
	return(lenTag + lenLen + lenValue );
}

//L
static char BER_LenLen(char *pLen) { return (((*pLen & 0x80) < 0x80)?1: ((*pLen & 0x7F) +1)); }
static unsigned long BER_GetLen(char *pLen) {
  unsigned long ber_len = 0;
  char *p = pLen;
  int lenlen = BER_LenLen(pLen);
  int i = 0;
  
  if(lenlen==1) ber_len = (unsigned short)*p;
  else {
    ++p;
    for (i=0;i<lenlen-1;i++,p++)  
      ber_len = ber_len*256 + (unsigned short)*p ;
  }

  return(ber_len);
}

static char msg[CTLS_BUFF_SZ] = {0};
static char rsp[CTLS_BUFF_SZ] = {0};
static int  msgLength = 0;
static int  rspLength = 0;

static int InitComPort(char comPortNumber)
{	
	int i = 0;
	int ret;
	
	ret = CTLSInitInterface(20000);
	LOG_PRINTFF(0x00000001L,"CTLSInitInterface ret %d", ret);
	if (ret != 0)
	{
		LOG_PRINTFF(0x00000001L,"error initialising CTLS reader");
		return -1;
	}

	// open CTLS
	for(i=0;i<20;i++) {
		ret = CTLSOpen();
		if(ret) SVC_WAIT(500);
		else break;
	}
	if (ret != 0)
	{
		LOG_PRINTFF(0x00000001L,"error initialising CTLS reader");
		return -1;
	}

	//set debug flag
	{

		char flag[30] = "";
		memset(flag,0,sizeof(flag));
		get_env("*DEBUG", flag, sizeof(flag));
		if(strncmp(flag,"1",1) == 0) {
			CTLSDEBUG = 1;
			SVC_WAIT(100);
			LOG_PRINTFF(0x00000001L,"get firmware version ");
			BuildRawMsg("\x29\x00\x00\x00",4);
			SendRxMsg(0);
		}

	}

	// if need to restore factory 
	{
		char jsonobj[30];
		unsigned long objlength=0;
		char *objdata = NULL;
		char *objvalue = NULL;

		strcpy(jsonobj,"RIS_PARAM");
  		objdata = (char*)IRIS_GetObjectData( jsonobj,(unsigned int *) &objlength);
        if(objdata) objvalue = (char*)IRIS_GetObjectTagValue( (const char *)objdata, "CTLS_RESTORE" );
		if(objvalue && strlen(objvalue) && strcmp(objvalue,"FACTORY")==0) {
			SVC_WAIT(100);
			SendRestoreRawMsg();
  			IRIS_StoreData(jsonobj,"CTLS_RESTORE", "DONE", 0/* false */);
		}

		if(objvalue) UtilStrDup(&objvalue , NULL);
		if(objdata) UtilStrDup(&objdata , NULL);

	}
	
	// configure the CTLS app
	CTLSClientUIParamHandler(UI_LED_STYLE_PARAM, &ret, sizeof(ret));
	CTLSClientUISetCardLogoBMP("");

	return ret;
}

static void ResetMsg()
{
	memset(msg, 0, sizeof(msg));
	msgLength = 0;
}

static void ResetRsp()
{	
	memset(rsp, 0, sizeof(rsp));
	rspLength = 0;	
}

static void AddToMsg(char *subMsg, int length)
{		
	memcpy(msg+msgLength, subMsg, length);	
	msgLength += length;	
}

static void AddVivoHeader(void)
{
	AddToMsg(VIVO_HDR_STR, strlen(VIVO_HDR_STR)+1);
}

static void AddVivo2Header()
{
	AddToMsg(VIVO2_HDR_STR, strlen(VIVO2_HDR_STR)+1);
}

static void AppendCRC()
{
	unsigned short crc = SVC_CRC_CCITT_M(msg, msgLength, 0xFFFF);	
	AddToMsg((char*)&crc, 2);	
}

static int GetDataLen(char *rsp)
{
	return ((*(rsp+V2_DAT_LEN_MSB_OFS) << 8) | *(rsp+V2_DAT_LEN_LSB_OFS));		
}

static int CheckCRC(char *rsp)
{
	unsigned short suppliedCRC = 
		*(rsp+V2_CRC_MSB_OFS+GetDataLen(rsp)) |
		(*(rsp+V2_CRC_LSB_OFS+GetDataLen(rsp)) << 8);
	
//	LOG_PRINTFF(0x00000001L,"CheckCRC");
	if(SVC_CRC_CCITT_M(rsp, GetDataLen(rsp)+V2_FXD_BYTES-2, 0xFFFF) == suppliedCRC)		
	{
	//	LOG_PRINTFF(0x00000001L,"CheckCRC 1");
		return CTLS_SUCCESS;	
	}
//	LOG_PRINTFF(0x00000001L,"CheckCRC 2");
	return CTLS_CRC_CHK_FAILED;
}

static int GetNumDigits(char *amt, int length)
{
	int numDigits = 0, i = 0;
	for(; i < length; i++) if(isdigit(amt[i])) numDigits++;
	return numDigits;
}

static void FormatAmt(char *amt, char *formattedAmt)
{
	int fAmtNdx = 0, tAmtNdx = 0, posToggle = 0;

	int padLength = 
		12-strlen(amt)+
		(strlen(amt)-GetNumDigits(amt, 12));		

	memset(formattedAmt, 0, 6);			

	fAmtNdx = (padLength-(padLength%2?1:0))/2; //pad with 0's	

	posToggle = padLength%2; //set byte position flag

	while(fAmtNdx < 6)
	{
		if(isdigit(amt[tAmtNdx])) 
		{			
			if(!posToggle)		
				formattedAmt[fAmtNdx] = ((amt[tAmtNdx]-0x30) << 4);				
			else			
				formattedAmt[fAmtNdx++] |= amt[tAmtNdx]-0x30;							
			
			posToggle = !posToggle;
		}
		tAmtNdx++;
	}
}

///////////////////helpers////////////////////

static int SendMsg() 
{	
	int res = -1;
	LOG_PRINTFF(0x00000001L,"SendMsg");
	res = CTLSSend((char *)msg, msgLength);
	if(CTLSDEBUG) {
		char stmp[1024]="";
		long len = 0;
		LOG_PRINTFF(0x00000001L,"CTLSSent:%d",msgLength);
		for(len=0;len<msgLength;len++) sprintf(&stmp[strlen(stmp)],"%02x",	msg[len]);
		for(len=0;len<msgLength*2;len+=100) LOG_PRINTFF(0x00000001L,"[%-.100s]",stmp+len);
	}else SVC_WAIT(50);
	SVC_WAIT(50);
	return res;
}

static int SendRxMsg(int timeout)
{			
	int snd;
	//LOG_PRINTFF(0x00000001L,"SendRxMsg");
	snd = SendMsg();		
	if(snd < 0) 
	{
		LOG_PRINTFF(0x00000001L,"SendRxMsg failed");
		return snd;
	}
	//LOG_PRINTFF(0x00000001L,"SendRxMsg 2");
	snd = GetRsp(1, timeout);	
	//LOG_PRINTFF(0x00000001L,"SendRxMsg snd :%d",snd);
	return snd;
}

static int GetRsp(char waitForRsp, int timeout)
{		 	
	int bytes;
	bytes = AcquireRsp(waitForRsp, timeout);
	//LOG_PRINTFF(0x00000001L,"BYTes :%d",bytes);
	ResetMsg();
	if(!bytes) 
	{
	//	//LOG_PRINTFF(0x00000001L,"GetRsp 2");
		return bytes;	
	}

	if( bytes < 0 ) 
	{
		return bytes;	
	}

	//Check CRC	
	return CheckCRC(rsp);
	//return CTLS_SUCCESS;
}

static int AcquireRsp(char waitForRsp, int timeout)
{		
	unsigned long t1 = read_ticks();
	extern int mcrHandle ;
	extern int conHandle;
	int cardinput = 0;
	int respok = 0;

	if(p_ctls) cardinput = 1;

	ResetRsp();

	CtlsResp(1, &respok);

	if(!timeout) timeout = 20000; //default

	if(cardinput) {
		if (mcrHandle == 0) mcrHandle = open(DEV_CARD, 0);
		//if(EmvIsCardPresent()) return(-1002);
	}

	for(;;)
	{				
		int i;
		if(cardinput)
		{
			unsigned char key = 0;
			int evt = 0;
			evt = read_evt(EVT_KBD|EVT_MAG|EVT_ICC1_INS);

			if(evt & EVT_MAG) {
				return(-1001);
			} else
			if(evt & EVT_ICC1_INS) {
				return(-1002);
			} else
			if( (evt & EVT_KBD) ) {
				if(read(conHandle, (char *)&key,1) == 1 ) {
						key &= 0x7F;
						if(key == KEY_CNCL)
						{
							return -1;
						}
						else if(key == KEY_FUNC)
						{
							return -2;
						}
				}

			}

		}
		
		if(	(i=CTLSReceive((char *)&rsp+rspLength, sizeof(rsp)-rspLength))>0)
		{	
			//attempt quick reads
			rspLength += i;			
			t1 = read_ticks();
			for(;;)
			{						
				if(	(i=CTLSReceive((char *)&rsp+rspLength, sizeof(rsp)-rspLength))>0)
				{		
					t1 = read_ticks();
					rspLength += i;
					SVC_WAIT(10);
					continue;
				}				
				break;				
			}
		}
		if(rspLength)
		{
			if(read_ticks() > t1+50)		
			{
				if(CTLSDEBUG) {
				char stmp[4096]="";
				long len = 0;
				long idx = 0;
				LOG_PRINTFF(0x00000001L,"CTLSreceived:%d",rspLength);
				for(len=0;len<rspLength;len++) sprintf(&stmp[strlen(stmp)],"%02x",	rsp[len]);
				for(len=0;len<rspLength*2;len+=100) LOG_PRINTFF(0x00000001L,"[%-.100s]",stmp+len);
				if(memcmp(rsp, VIVO2_HDR_STR,strlen(VIVO2_HDR_STR))==0) {
					idx = 11;
				}
				else if(memcmp(rsp, VIVO_HDR_STR,strlen(VIVO_HDR_STR))==0) {
					idx = 11;
				}
				if(rsp[idx] != 0x00 && rsp[idx]!=V2_REQ_OL_AUTH) {
					DebugPrint(" boyang status = %02x, cmd/subcmd = %02x %02x", rsp[idx],msg[idx-1],msg[idx]);
					LOG_PRINTFF(0x00000001L, "boyang status = %02x, cmd/subcmd = %02x %02x", rsp[idx],msg[idx-1],msg[idx]);
					respok = -1;
					CtlsResp(1, &respok);
				}
				SVC_WAIT(50);
				}
				else SVC_WAIT(100);

				return rspLength;			
			}
		}
		else
		{	
			if(!waitForRsp) 
			{
				return 0;			
			}
			
			if(read_ticks() > t1+timeout)
			{	
				return CTLS_RSP_TIMED_OUT;
			}			
		}
		SVC_WAIT(0);
	}	
}


static int HandleReqOLErrRsp()
{
	LOG_PRINTFF(0x00000001L,"handling req onl err");
	processEmvTlv(rsp+V2_DATA_OFS+V2_ERR_T2_OFS);	
	ExtractFailedEMVDetails(rsp+V2_DATA_OFS+31, GetDataLen(rsp)-31);
	return CTLS_EMV | CTLS_REQ_OL; //change this return value maybe
}

static int HandleNoReqOLErrRsp()
{	
	LOG_PRINTFF(0x00000001L,"handling no req onl err. err is %x", *(rsp+V2_DATA_OFS));

	//ExtractFailedEMVDetails(rsp+V2_DATA_OFS+9, GetDataLen(rsp)-9);

	//We know it's not req onl so what err is it?
	switch(*(rsp+V2_DATA_OFS))
	{
		//////////DECLINE TXN//////////
	case V2_NO_ERR:
		//something went wrong but nothing went wrong? BAIL!
		LOG_PRINTFF(0x00000001L,"ERROR! HandleNoReqOLErrRsp() called but ERR == 0");
		return CTLS_UNKNOWN | CTLS_DECLINED;
	case V2_AMT_ZERO:
		//IGNORE TXN!
		LOG_PRINTFF(0x00000001L,"ERROR! Contactless zero amount txn");
		return CTLS_UNKNOWN | CTLS_DECLINED;
	case V2_OUT_SEQ_CMD:
		//slap my wrists - software error
		LOG_PRINTFF(0x00000001L,"ERROR! Contactless received out of sequence command");
		return CTLS_UNKNOWN | CTLS_DECLINED;
	case V2_UNK_DAT_ELMT:
	case V2_DAT_ELMT_MISS:
		//reserved future use
		LOG_PRINTFF(0x00000001L,"ERROR! Reserved contactless error code has been used: %d", *(rsp+V2_DATA_OFS));
		return CTLS_UNKNOWN | CTLS_DECLINED;
	case V2_CA_PUB_KEY_FAIL:
		//must fix missing key problem, could try sending it if have it	
		LOG_PRINTFF(0x00000001L,"ERROR! Contactless missing public key");
		return CTLS_UNKNOWN | CTLS_DECLINED;
	case V2_ISS_PUB_KEY_FAIL:
		//must fix key problem, could try resending it
		LOG_PRINTFF(0x00000001L,"ERROR! Recovering contactless issuer public key");		
		return CTLS_UNKNOWN | CTLS_DECLINED;
	case V2_SDA_FAIL:	
		//retry not worth it
		LOG_PRINTFF(0x00000001L,"ERROR! Contactless data auth failed during SSAD");
		return CTLS_UNKNOWN | CTLS_DECLINED;
	case V2_ICC_PUB_KEY_FAIL:	
		//retry not worth it
		LOG_PRINTFF(0x00000001L,"ERROR! Contactless data auth failed during attempted recovery of ICC pub key");
		return CTLS_UNKNOWN | CTLS_DECLINED;
	case V2_DYN_SIG_VER_FAIL:
		//retry not worth it
		LOG_PRINTFF(0x00000001L,"ERROR! Contactless data auth failed during dynamic sig verif");				
		return CTLS_UNKNOWN | CTLS_DECLINED;
	case V2_PRCS_RST_FAIL:	
		//could be wrong emv params, retry not worth it
		LOG_PRINTFF(0x00000001L,"ERROR! Contactless processing restrictions step failed");
		return CTLS_EMV | CTLS_DECLINED;
	case V2_TRM_FAIL:	
		//could be wrong emv params, retry not worth it
		LOG_PRINTFF(0x00000001L,"ERROR! Contactless terminal risk management step failed"); 
		return CTLS_EMV | CTLS_DECLINED;
	case V2_CVM_FAIL:		
		//could be wrong emv params, retry not worth it
		LOG_PRINTFF(0x00000001L,"ERROR! Contactless cardholder verification step failed"); 
		return CTLS_EMV | CTLS_DECLINED;
	case V2_TAA_FAIL:
		LOG_PRINTFF(0x00000001L,"ERROR! Contactless terminal action analysis step failed"); 
		//could be wrong emv params, retry not worth it
		return CTLS_EMV | CTLS_DECLINED;
	case V2_CRD_ERR_STAT:
		//card returned error
		if(*(rsp+V2_DATA_OFS+3) >= 3) 			
			return CTLS_UNKNOWN | CTLS_FALL_FORWARD; 	
		//else fall through	
		break;
	case V2_CRD_EXPD:
		//visa only	
		return CTLS_UNKNOWN | CTLS_DECLINED;
	case V2_CRD_GEN_AAC:
		return CTLS_EMV | CTLS_DECLINED;
		//////////FALL FORWARD//////////	
	case V2_CRD_BLKD:
		//card blocked
	case V2_CRD_NOT_SUPP:
		//possibly no aid for card
	case V2_AMT_OVR_MAX:
		//txn above offline limit	
	case V2_GOTO_CNT:
		//fall forward
		LOG_PRINTFF(0x00000001L,"ERROR[%d]! Contactless try contact txn",(*(rsp+V2_DATA_OFS)));
		sprintf(p_ctls->TxnStatus,"%2d",99);
		return CTLS_UNKNOWN | CTLS_TRY_CONTACT;			
		//////////RETRY TXN//////////
	case V2_CRD_NOT_RSP:
		return CTLS_UNKNOWN | CTLS_FALL_FORWARD;
	case V2_COLLISION:
		//more than 1 card - retry txn
		return CTLS_UNKNOWN | CTLS_RETRY;	

		//////////REQUEST ONLINE AUTH//////////
	case V2_REQ_OL_AUTH:
		//THIS IS AN ERROR!!! WE ARE IN THIS PROC BECAUSE IT'S NOT REQ ONL!!
		//above card balance but below max offline
		return CTLS_UNKNOWN | V2_REQ_OL_AUTH;		

	case V2_CRD_GEN_ARQC:
		return CTLS_EMV | V2_REQ_OL_AUTH;
	
	case V2_SD_MEM_ERR:
		//only see this when requesting txn logs
		break;
	default:
		break;
	}

	LOG_PRINTFF(0x00000001L,"Unknown No Req OL Error Code");
	return CTLS_UNKNOWN | CTLS_DECLINED;
}

static int HandleErrRsp()
{	
	LOG_PRINTFF(0x00000001L,"HandleErrRsp");
	if(*(rsp+V2_DATA_OFS) == V2_REQ_OL_AUTH)
	{		
		LOG_PRINTFF(0x00000001L,"HandleErrRsp 1");
		return HandleReqOLErrRsp();	
	}	
	LOG_PRINTFF(0x00000001L,"HandleErrRsp 3");
	return HandleNoReqOLErrRsp();	
}

static int HandleRTCNFrame()
{
	LOG_PRINTFF(0x00000001L,"NACK RXD - ERR CODE IS %d", *(rsp+12));
	switch(*(rsp+12))
	{	
	case V1_RTC_ERR_UNK:
	case V1_RTC_INV_DATA:
	case V1_RTC_ERR_NO_RTC_RSP:
	case V1_RTC_NO_ERR:
	default:
		break;
	}
	return CTLS_FAILED;
}

static int HandleKeyMgtNFrame(void)
{
	LOG_PRINTFF(0x00000001L,"NACK RXD - ERR CODE IS %d", *(rsp+12));
	switch(*(rsp+12))
	{	
	case V1_KEYMGT_NO_ERR:
	case V1_KEYMGT_UNK_ERR:	
	case V1_KEYMGT_INV_DATA:
	case V1_KEYMGT_DATA_NOT_CMP:
	case V1_KEYMGT_INV_KEY_NDX:
	case V1_KEYMGT_INV_HSH_ALG_IND:
	case V1_KEYMGT_INV_KEY_ALG_IND:
	case V1_KEYMGT_INV_KEY_MOD_Len:
	case V1_KEYMGT_INV_KEY_EXP:
	case V1_KEYMGT_KEY_EXISTS:
	case V1_KEYMGT_NO_SPACE:
	case V1_KEYMGT_KEY_NOT_FOUND:
	case V1_KEYMGT_CRYP_CHP_NO_RSP:
	case V1_KEYMGT_CRYP_CHP_COM_ERR:
	case V1_KEYMGT_RID_SLT_FULL:
	case V1_KEYMGT_NO_FREE_SLOTS:
	default:
		break;
	}
	return CTLS_FAILED;
}

static int ExtractCardDetails()
{	
	int emvRes, magRes; 
	
	LOG_PRINTFF(0x00000001L,"ExtractCardDetails");
	
	emvRes = ExtractEMVDetails();	
	magRes = ExtractTrack("2", rsp+V2_DATA_OFS);	

	if(emvRes == CTLS_NO_CLR_REC) 
	{
		LOG_PRINTFF(0x00000001L,"ExtractCardDetails");
		return magRes == CTLS_SUCCESS ?CTLS_MSD:magRes;
	}

	LOG_PRINTFF(0x00000001L,"Clearing record present emvRes:%d",emvRes);

	if(emvRes != CTLS_SUCCESS)
	{
		LOG_PRINTFF(0x00000001L,"Clearing record present ");
		return emvRes;
	}
	else
	{		

	}		
	return (emvRes==CTLS_SUCCESS ?CTLS_EMV:emvRes);
}

static long getTlv(char *tlv, TLV *tlvStruct)
{
	unsigned long iLenTag=0, iLenLen=0,iLenValue = 0;
	unsigned long lTag=0;
	int iTagType = 0;
	char *pIdx = tlv ;
	int i=0;
	
	//LOG_PRINTFF(0x00000001L,"getTlv...1,%02x%02x%02x%02x%02x",*tlv,*(tlv+1),*(tlv+2),*(tlv+3),*(tlv+4));
	if(!tlv || !tlvStruct) return(0);
	memset(tlvStruct, 0, sizeof(TLV));
	
	//LOG_PRINTFF(0x00000001L,"getTlv...2");
	BER_parseTag(pIdx, &lTag, (long *)&iLenTag, &iTagType);
	
	for(i=0;i<iLenTag;i++) {sprintf(&tlvStruct->sTag[strlen(tlvStruct->sTag)],"%02X",*(tlv+i));}
	//LOG_PRINTFF(0x00000001L,"getTlv...3,%ld,%ld",lTag,iLenTag);
	if(lTag == 0) return(0); // something wrong

	tlvStruct->tag = lTag;
	tlvStruct->lenTag = iLenTag;
	tlvStruct->tagtype = iTagType;
	pIdx = pIdx + iLenTag;

	iLenLen = BER_LenLen(pIdx);
	tlvStruct->lenLen = iLenLen;
	iLenValue = BER_GetLen(pIdx) ;
	tlvStruct->len = iLenValue;
	
	pIdx = pIdx + iLenLen;
	//LOG_PRINTFF(0x00000001L,"getTlv...4,%ld",iLenLen);
	memcpy(tlvStruct->val, pIdx, iLenValue);
	//LOG_PRINTFF(0x00000001L,"getTlv...4,%ld,%ld,%ld,%02x%02x",iLenTag,iLenLen,iLenValue,tlvStruct->val[0],tlvStruct->val[1]);
	return(iLenTag + iLenLen+ iLenValue);
}


static int processEmvTlv(char *tlv)
{		
	TLV tlvStruct = {0};
	char string[1024];
	char stmp[30];
	char *pIdx = tlv;
	int iIdx = 0;

	{
		memset( &tlvStruct,0,sizeof(TLV));
		getTlv(pIdx, &tlvStruct);
		pIdx = tlvStruct.val;
		iIdx += tlvStruct.lenTag + tlvStruct.lenLen ;
		//LOG_PRINTFF(0x00000001L,"processEMVTlv type=%d,pIdx = %02x%02x%02x,tag=%ld", tlvStruct.tagtype,*pIdx,*(pIdx+1),*(pIdx+2),tlvStruct.tag);
	} 

	if(tlvStruct.tagtype == TLV_TYPE_CONSTRUCTED) { // constructed tag
		TLV tlv2 = {0};
		memset( &tlv2,0,sizeof(TLV));
		getTlv(tlvStruct.val, &tlv2);
		LOG_PRINTFF(0x00000001L,"processEMVTlv constructed type=%d,pIdx = %02x%02x%02x,tag=%ld", tlv2.tagtype,tlvStruct.val[0],tlvStruct.val[1],tlvStruct.val[2],tlv2.tag);
		iIdx += tlv2.lenTag + tlv2.lenLen+ tlv2.len ;
		memcpy(&tlvStruct,&tlv2,sizeof(TLV)); // don't need the construct tag
	} else  iIdx += tlvStruct.len;
	
	LOG_PRINTFF(0x00000001L,"processEMVTlv Idx = %d", iIdx);
	//trim the first byte if DF8129 -> 8129
	//switch(tlvStruct.tag & 0xFFFF)
	{			
		if( tlvStruct.tag <= 255 /*0xff*/)
			sprintf(stmp,"%X00", (tlvStruct.tag & 0xFFFF));
		else if(tlvStruct.tag >= 65536 /*0xffff*/) {
			sprintf(stmp,"LEN6%X", (tlvStruct.tag & 0XFFFFFF));
		} else 
			sprintf(stmp,"%X", (tlvStruct.tag & 0xFFFF));

		memcpy(&p_ctls->sTLvData[p_ctls->nTLvLen],stmp,strlen(stmp));
		p_ctls->nTLvLen = p_ctls->nTLvLen + strlen(stmp);

		sprintf(&p_ctls->sTLvData[p_ctls->nTLvLen],"%02X",tlvStruct.len);
		p_ctls->nTLvLen = p_ctls->nTLvLen + 2;

		memset(string,0x00,sizeof(string));
		if(tlvStruct.len) {
			UtilHexToString((const char *)tlvStruct.val , tlvStruct.len , string);
			memcpy(&p_ctls->sTLvData[p_ctls->nTLvLen],string,strlen(string));
		}
		
		LOG_PRINTFF(0x00000001L,"processEMVTlv V[%.100s]", string);
		p_ctls->nTLvLen = p_ctls->nTLvLen + strlen(string);
		
		return iIdx;
	}
}

static int ExtractClearingRec(char *clrRec)
{
	char *pLen = clrRec+1;
	int dataRead = 0;
	long len = BER_GetLen(pLen);
	long lenLen = BER_LenLen(pLen);
	char *pValue = pLen + lenLen;

	LOG_PRINTFF(0x00000001L,"ExtractClearingRec pLen[%02x%02x%02x]lenLen:%ld:", *pLen,*(pLen+1),*(pLen+2),lenLen);
	
	LOG_PRINTFF(0x00000001L,"ExtractClearingRec len:%d:", len);
	while(dataRead < len)
	{
		int read = processEmvTlv(pValue+dataRead);
		if(read > 0) dataRead += read;
		else 
		{
			LOG_PRINTFF(0x00000001L,"ExtractClearingRec 1");
			return read;		
		}
	}	
	
	return dataRead;
}

static int ExtractEMVDetails()
{
	int t1Len, t2Len, dataRead = 0, dataLen = GetDataLen(rsp);
	char *track1, *track2;

	LOG_PRINTFF(0x00000001L,"extracting EMV details");

	t1Len = *(rsp+V2_DATA_OFS);
	track1 = rsp+V2_DATA_OFS+1;
	
	t2Len = *(track1+t1Len);
	track2 = track1+1+t1Len;
	
	memcpy(p_ctls->sTrackOne,track1,t1Len);
	memcpy(p_ctls->sTrackTwo,track2,t2Len);
	
	dataRead += t1Len+t2Len+2;	   
	
	dataRead++;
	if(!*(track2+t2Len)) 
	{
		LOG_PRINTFF(0x00000001L,"extracting EMV details 1");
		//return CTLS_NO_CLR_REC;
	} else
		dataRead += ExtractClearingRec(rsp+V2_DATA_OFS+dataRead)+2;		

	LOG_PRINTFF(0x00000001L,"extracting EMV details 2,%d",dataRead);
	//Get remaining TLVs
	while(dataRead < dataLen)
	{			
		int read = processEmvTlv(rsp+V2_DATA_OFS+dataRead);				
		if(read > 0) dataRead += read;
		else 
		{
			LOG_PRINTFF(0x00000001L,"extracting EMV details 3");
			return read;
		}
	}	
	
	LOG_PRINTFF(0x00000001L,"extracting EMV details 4");
	return CTLS_SUCCESS;
}

static int ExtractFailedEMVDetails(char *details, int length)
{
	int dataRead = 0;
	
	LOG_PRINTFF(0x00000001L,"ExtractFailedEMVDetails");
	
	//Get remaining TLVs
	while(dataRead < length)
	{				
		int read = processEmvTlv(details+dataRead);		
		LOG_PRINTFF(0x00000001L,"ExtractFailedEMVDetails 1");
		if(read > 0) dataRead += read;
		else return read;
	}
	LOG_PRINTFF(0x00000001L,"ExtractFailedEMVDetails 2");
	return CTLS_SUCCESS;
}

//formats data so accepted by card_parse()
static int ExtractTrack(char *trk, char *trkInfo)
{	
	return CTLS_SUCCESS;
}

static int GetRspStatus()
{	
	if(rspLength < 1) return CTLS_NO_REPLY;	
	return *(rsp+V2_STATUS_OFS);
}

static int ExtractCtlsRsp(char* cmd)
{
	int dataRead = 0;	
	unsigned int groupno = 0;
	CfgAid *pAid = NULL;
	CfgAidTlv *pCurr = NULL;
	
	if(GetRspStatus() == V2_FAILED_NAK) return CTLS_FAILED;		

	while(dataRead < RSP_DATA_LEN)
	{		
		unsigned long lTag=0;
		char sValue[1024]="";
		int readLen = Verifone_parseTag(rsp+V2_DATA_OFS+dataRead, &lTag, NULL, sValue);		
		if(readLen > 0) {
			unsigned char *pTag = (unsigned char *)&lTag;
			LOG_PRINTFF(0x00000001L,"extract ctls rsp , tag = %02.2x%02.2x%02.2x%02.2x,%ld, sValue = %s",
					*pTag,*(pTag+1),*(pTag+2),*(pTag+3),lTag,sValue);

			//get configurable aid
			if(strncmp(cmd,"\x03\x04",2)==0) {
				switch(lTag)
				{
				case 0xffe4:
					if( strlen(sValue)) {
						groupno = atoi(sValue);
						CfgGroupAdd(&groupno);
					}
					break;
				case 0x9f06:
					CfgAidAdd(&groupno,sValue);
					break;
				default:
					break;
				}
			}

			//get configurable group
			if(strncmp(cmd,"\x03\x06",2)==0) {

				switch(lTag)
				{
				case 0xffe4:
					if( strlen(sValue)) {
						groupno = atoi(sValue);
						CfgGroupAdd(&groupno);
					}
					break;
				case 0x5f28:
				case 0x9c:
				case 0x9f1a:
				case 0x9f1b:
				case 0x9f33:
				case 0x9f35:
				case 0x9f40:
				case 0x9f66:
				case 0xfff1:
				case 0xfff4:
				case 0xfff5:
				case 0xfffb:
				case 0xfffc:
				case 0xfffd:
				case 0xfffe:
				case 0xffff:
				case 0x97:
				default:
					CfgAidTLVAdd(&groupno,NULL,lTag,sValue);
					break;
				}
			}
			
			//get additional aid parameters
			//must call after 03/04
			if(strncmp(cmd,"\xF0\x00",2)==0) {
				unsigned int grp=0;
				switch(lTag)
				{
				case 0x9f06:
					pAid = CfgAidFind(sValue,&grp);
					break;
				case 0x1ff2:
				case 0x1ff3:
				case 0x1ff4:
				case 0x1ff5:
				case 0x1ff6:
				case 0x1ff7:
				case 0x1ff8:
				case 0x1ff9:
				case 0x9f09:
				case 0x9f15:
				case 0x9f53:
				case 0x1ffc:
				case 0x5f36:
				case 0x1ffd:
				case 0x1ffe:
				case 0xdf03:
				case 0xdf04:
				case 0x1ff1:
				case 0x9f6d:
				case 0x9f58:
				case 0x9f59:
				case 0x9f5a:
				case 0x9f5e:
				case 0x1f51:
				case 0x9f01:
				case 0xdf8124:
				case 0xdf8125:
				case 0x9f7e:
				case 0xdf811e:
				case 0xdf812c:
				case 0xdf8119:
				case 0xdf8118:
				case 0xdf811b:
				case 0xdf8117:
				case 0xdf811f:
				case 0xdf811a:
				case 0xdf810c:
				case 0x1fb0:
				case 0x9f5d:
					if(pAid && pAid->TLVs ==NULL){
						pAid->TLVs = CfgTLVAdd(NULL,lTag, sValue);
						pCurr = pAid->TLVs;
					} else {
						pCurr =  CfgTLVAdd(pCurr,lTag, sValue);
					}
					break;
				default:
					break;
				}
			}

			dataRead += readLen;
		} else return readLen;
	}
		
	return CTLS_SUCCESS;
}

static char GetFrameType()
{
	return *(rsp+9);
}
////////////////////////////////////////

////////////////Build Contactless Msgs//////////////

static void BuildSetPollOnDemandMsg()
{	
	char fields[5] = {0x01, 0x01, 0x00, 0x01, 0x01};
	LOG_PRINTFF(0x00000001L,"BuildSetPollOnDemandMsg");
	ResetMsg();
	AddVivo2Header();
	AddToMsg(fields, sizeof(fields));		
	AppendCRC();
}

static void BuildActivateTransactionMsg(char timeout) //add txn amount and type
{		
	char currencyCode[5] = {0x30, 0,0,0,0};
	char fields[4] = {0x02, 0x01, 0x00, 0x22};
	char tlv_9f02[9] = {0x9F, 0x02, 0x06 ,0,0,0,0,0,0};
	char tlv_9a[5] = {0x9A, 0x03 ,0,0,0};
	char tlv_9f21[6] = {0x9F, 0x21, 0x03 ,0,0,0};
	char tlv_5f2a[5] = {0x5F, 0x2A, 0x02, 0,0};
	char tlv_9f1a[5] = {0x9F, 0x1A, 0x02, 0,0x36};
	char tlv_9c[3] = {0x9C, 0x01, 0};
	char hexDateTime[6] = {0};
	char DateTime[20];

	read_clock(DateTime);
	DateTime[14] = '\0';
	SVC_DSP_2_HEX(DateTime+2, hexDateTime, 6);	
	FormatAmt(p_ctls->sAmt, tlv_9f02+3);
	
	memcpy(tlv_9a+2, hexDateTime, 3);
	memcpy(tlv_9f21+3, hexDateTime+3, 3);
	//Dwarika
	memcpy(currencyCode+1, "036", 3);
	SVC_DSP_2_HEX(currencyCode, tlv_5f2a+3, 2);

	switch(0)
	{
	//case TxnType::purchase:		
	case 0:
		tlv_9c[2] = 0x00;
		break;
	//case TxnType::purchaseWithCashback:		
	case 1:
		tlv_9c[2] = 0x09;
		break;
	//case TxnType::refund:		
	case 2:
		tlv_9c[2] = 0x20;
		break;
	//case TxnType::cashAdvance:		
	case 3:
		tlv_9c[2] = 0x01;
		break;
	}

	ResetMsg();
	AddVivo2Header();	
	AddToMsg(fields, sizeof(fields));
	AddToMsg(&timeout, 1);
	AddToMsg(tlv_9f02, sizeof(tlv_9f02));
	AddToMsg(tlv_9a, sizeof(tlv_9a));
	AddToMsg(tlv_9f21, sizeof(tlv_9f21));
	AddToMsg(tlv_5f2a, sizeof(tlv_5f2a));
	AddToMsg(tlv_9f1a, sizeof(tlv_9f1a));
	AddToMsg(tlv_9c, sizeof(tlv_9c));
	AppendCRC();		
	LOG_PRINTFF(0x00000001L,"BuildActivateTransactionMsg 3");
}

static void BuildSetEMVTxnAmtMsg()
{	
	//char fields[4] = {0x04, 0x00, 0x00, 0x12};
	char fields[4] = {0x04, 0x00, 0x00, 0x0c};
	char tlv_9f02[9] = {0x9F, 0x02, 0x06 ,0,0,0,0,0,0};
	//char tlv_9f03[9] = {0x9F, 0x03, 0x06, 0,0,0,0,0,0};
	char tlv_9f03[3] = {0x9F, 0x03, 0x00};

	LOG_PRINTFF(0x00000001L,"BuildSetEMVTxnAmtMsg");
	
	FormatAmt(p_ctls->sAmt, tlv_9f02+3);
	LOG_PRINTFF(0x00000001L,"BuildSetEMVTxnAmtMsg 1");

	ResetMsg();
	AddVivo2Header();	
	AddToMsg(fields, sizeof(fields));	
	AddToMsg(tlv_9f02, sizeof(tlv_9f02));
	AddToMsg(tlv_9f03, sizeof(tlv_9f03));	
	AppendCRC();
	LOG_PRINTFF(0x00000001L,"BuildSetEMVTxnAmtMsg 2");
}

static int SendRestoreRawMsg()
{	
	char restoremsg[6] = "\xf8\x00\x00\x00\xf1\x00";
	BuildRawMsg(restoremsg,5);
	SendRxMsg(0);
	return(0);
}

static void BuildRawMsg(char* tlv,int tlvlen)
{	
	ResetMsg();
	AddVivo2Header();
	AddToMsg(tlv, tlvlen);
	tlvlen = tlvlen - 4;

	LOG_PRINTFF(0x00000001L,"build raw msg:%d",tlvlen);
	msg[12] = (tlvlen >> 8) & 0xFF;
	msg[13] = tlvlen & 0xFF;
	AppendCRC();
}

static void BuildCancelTransactionMsg()
{		
	//char fields[4] = {0x05, 0x01, 0x00, 0x00};
	char fields[5] = {0x05, 0x01, 0x00, 0x01,0x00};
	ResetMsg();
	AddVivo2Header();
	AddToMsg(fields, sizeof(fields));	
	AppendCRC();
}

static void BuildStoreLCDMsgMsg(char msgNdx, char *str, char *paramStr1, char *paramStr2, char *paramStr3)
{	
	char fields[4] = {0x01, 0x03, 0x00, 0x00};
	char data[512] = {0};
				
	if(msgNdx == 0xFD || msgNdx == 0xFE || msgNdx == 0xFF)
	{		
		data[0] = msgNdx;
		fields[3] = 1;
	}
	else
	{
		data[0] = msgNdx;
		data[1] = strlen(str);
		data[2] = strlen(paramStr1);
		data[3] = strlen(paramStr2);
		data[4] = strlen(paramStr3);
		sprintf(data+5, "%s%s%s%s", str, paramStr1, paramStr2, paramStr3);
		fields[3] = strlen(data+5)+5;
	}

	ResetMsg();
	AddVivo2Header();
	AddToMsg(fields, sizeof(fields));	
	AddToMsg(data, fields[3]);
	AppendCRC();	
}


static int SendGeneralMsg(char cmd,char subcmd,char *data,short datalen)
{
	int res = 0;
	BuildGeneralMsg(cmd,subcmd,data,datalen);
	LOG_PRINTFF(0x00000001L,"SendGeneralMsg %c%c..",cmd,subcmd);
	res = SendRxMsg(2000);
	if(res != CTLS_SUCCESS) return res;
	return CTLS_SUCCESS;
}

static int BuildGeneralMsg(char cmd,char subcmd,char *data,short datalen)
{
	char fields[4] = {0x00, 0x00, 0x00, 0x00};
	fields[0] = cmd;
	fields[1] = subcmd;
	if(datalen) fields[3] = datalen; 
	ResetMsg();
	AddVivo2Header();
	AddToMsg(fields, sizeof(fields));
	AddToMsg(data, datalen);
	AppendCRC();
	return(0);
}


static void BuildSetSourceMsg()
{ 
	char fields[6] = {0x01 , 0x05, 0x00, 0x02, 0x15, 0x05};	
	ResetMsg();
	AddVivo2Header();
	AddToMsg(fields, sizeof(fields));
	AppendCRC();
}

static void BuildSetDateMsgCF()
{
	char fields[2] = {0x25, 0x03};
	char dataLen = 0x04;	
	
	ResetMsg();
	AddVivoHeader();
	AddToMsg("C", 1);	
	AddToMsg(fields, sizeof(fields));
	AddToMsg("", 1);
	AddToMsg(&dataLen, 1);
	AppendCRC();		
}

static void BuildSetDateMsgDF()
{		
	char dateTime[15] = {0};	
	char hexDate[4] = {0};	
	read_clock(dateTime);	

	SVC_DSP_2_HEX(dateTime, hexDate, 4);	

	ResetMsg();
	AddVivoHeader();
	AddToMsg("D", 1);

	AddToMsg(hexDate, 1);
	AddToMsg(hexDate+1, 1);
	AddToMsg(hexDate+2, 1);
	AddToMsg(hexDate+3, 1);
	
	AppendCRC();			
}

static void BuildSetTimeMsgCF()
{
	char fields[2] = {0x25, 0x01};	
	char dateTime[15] = {0};
	char hexTime[3] = {0};

	read_clock(dateTime);
	SVC_DSP_2_HEX(dateTime+8, hexTime, 3);
	LOG_PRINTFF(0x00000001L,"BuildSetTimeMsgCF() hexTime:%s:",hexTime);
		
	ResetMsg();
	AddVivoHeader();
	AddToMsg("C", 1);
	AddToMsg(fields, sizeof(fields));
	AddToMsg(hexTime, 1);
	AddToMsg(hexTime+1, 1);
	AppendCRC();
}

static void BuildSetCAPubKeyMsgCF(char dataLen1, char dataLen2)
{
	char fields[4] = {0x24, 0x01, 0,0};	
	fields[2] = dataLen2;
	fields[3] = dataLen1;

	ResetMsg();
	AddVivoHeader();
	AddToMsg("C", 1);
	AddToMsg(fields, sizeof(fields));	
	AppendCRC();
}

static void BuildSetCAPubKeyMsgDF(char *data, char len)
{		
	ResetMsg();
	AddVivoHeader();
	AddToMsg("D", 1);	
	AddToMsg(data, len);
	AppendCRC();		
}

static void BuildDelCAPubKeysMsg()
{
	char fields[4] = {0x24, 0x03, 0,0};		
	ResetMsg();
	AddVivoHeader();
	AddToMsg("C", 1);
	AddToMsg(fields, sizeof(fields));	
	AppendCRC();
}

//////////////////////////////////////
/////////////////Contactless ops/////////////////
static void (*CTLS_cbDisplayStart	)(CTLSUIParms* pParms)	= NULL;	// Callbacks default to NULL function
static void (*CTLS_cbDisplayStop	)(void* tbd)			= NULL;
static void (*CTLS_cbDisplayLeds	)(CTLSLEDStatus* leds)	= NULL;
static void (*CTLS_cbDisplayClear	)(void* tbd)			= NULL;
static void (*CTLS_cbDisplayText	)(CTLSLCDText* pText)   = NULL;

void vdMystartUIHandler (CTLSUIParms *params)
{
	CTLS_cbDisplayStart(params);
}

void vdMystopUIHandler(void* tbd)
{
	CTLS_cbDisplayStop(tbd);
}

void vdMyledHandler(CTLSLEDStatus* leds)	
{	
	CTLS_cbDisplayLeds(leds);
}

void vdMyClearDisplay (void *tbd)
{
	CTLS_cbDisplayClear(tbd);
}

void vdMytextHandler(CTLSLCDText* pText)
{
	CTLS_cbDisplayText(pText);
}


int inSetCallbacks(void)
{
	int inRet;
	CTLSUIFuncs stMyUIFuncs; 

	memset (&stMyUIFuncs, 0x00, sizeof (stMyUIFuncs));

	inRet = CTLSGetUI (&stMyUIFuncs);	

	//Save the default handlers
	CTLS_cbDisplayStart =  stMyUIFuncs.startUIHandler;
	CTLS_cbDisplayStop  =  stMyUIFuncs.stopUIHandler;
	CTLS_cbDisplayLeds  =  stMyUIFuncs.ledHandler;
	CTLS_cbDisplayClear =  stMyUIFuncs.clearDisplayHandler;
	CTLS_cbDisplayText  =  stMyUIFuncs.textHandler;

	//Assign application handlers     
	stMyUIFuncs.startUIHandler = vdMystartUIHandler; 
	stMyUIFuncs.stopUIHandler  = vdMystopUIHandler;
	stMyUIFuncs.ledHandler	  = vdMyledHandler;
	stMyUIFuncs.clearDisplayHandler = vdMyClearDisplay;
	stMyUIFuncs.textHandler	  = vdMytextHandler;
	//Set back the new UI callbacks

	inRet  = CTLSSetUI (&stMyUIFuncs);	
	return inRet;	
}


static int GetPayment(char waitForRsp, int timeout)
{	
	LOG_PRINTFF(0x00000001L,"GetPayment");
	return ActivateTransaction(waitForRsp, timeout);
}

static int CancelTransaction(int reason)
{
	int res;

	if(strlen(p_ctls->TxnStatus) && atoi(p_ctls->TxnStatus)== V2_REQ_OL_AUTH)
	{// update balance
		char dt[30]="";
		char sCmd[64] = "";
		char tag_date[4];
		char tag_time[4];
		int idx = 0;
		__time_real("YYMMDDhhmmss",dt);

		UtilStringToHex(dt,6,tag_date);
		UtilStringToHex(dt+6,6,tag_time);

		memcpy(sCmd+idx, "\x03\x03\x00\x00",4); idx += 4;
		memcpy(sCmd+idx, "\x00",1); idx += 1; //status code
		memcpy(sCmd+idx, "\xe3\x00\x06\x30\x30\x30\x30\x30\x30",9); idx += 9;
		memcpy(sCmd+idx, "\x9a\x03",2); idx += 2;
		memcpy(sCmd+idx, tag_date,3); idx+=3;
		memcpy(sCmd+idx, "\x9f\x21\x03",3); idx += 3;
		memcpy(sCmd+idx, tag_time,3); idx+=3;

		BuildRawMsg(sCmd,idx);
		SendRxMsg(0);
		SVC_WAIT(50);
	}

	{
		BuildCancelTransactionMsg();	
		res = SendRxMsg(5000);	

		if(memcmp(rsp,VIVO2_HDR_STR,strlen(VIVO2_HDR_STR)) == 0) {
			int idx = strlen(VIVO2_HDR_STR)+1+1;
			char rspcode = *(rsp+idx);

			if(rspcode != 0x00) {
				//char sCmdHex[32];
				//char sCmd[64];
				//DebugPrint("boyang....cancel = %02x", rspcode );

				}

		}
	}

	LOG_PRINTFF(0x00000001L,"CancelTransaction 1");
	return res;
}
static int SetPollOnDemandMode()
{	
	LOG_PRINTFF(0x00000001L,"SetPollOnDemandMode");
	BuildSetPollOnDemandMsg();
	return SendRxMsg(0);
}
static int ActivateTransaction(char waitForRsp, int timeout)
{	
	int res = 0;
	LOG_PRINTFF(0x00000001L,"ActivateTransaction");
	BuildActivateTransactionMsg(timeout);
	LOG_PRINTFF(0x00000001L,"ActivateTransaction 1");
	if(waitForRsp) res =  SendRxMsg(timeout+500);		
	else res =  SendMsg();		
	LOG_PRINTFF(0x00000001L,"ActivateTransaction 2:res=%d",res);
	return(res);
}

static int SetEMVTxnAmt()
{
	BuildSetEMVTxnAmtMsg();
	return SendRxMsg(0);
}
static int StoreLCDMsg(char msgNdx, char *str, char *paramStr1, char *paramStr2, char *paramStr3)
{
	BuildStoreLCDMsgMsg(msgNdx, str, paramStr1, paramStr2, paramStr3);
	return SendRxMsg(5000);
}

static int SetSource()
{
	BuildSetSourceMsg();
	return SendRxMsg(0);
}
static int SetDateTime()
{
	int res;
	
	BuildSetDateMsgCF();
	res = SendRxMsg(0);
	if(res != CTLS_SUCCESS) return res;
	
	if(GetFrameType() == V1_ACK_FRAME) res = CTLS_SUCCESS;
	else if(GetFrameType() == V1_NACK_FRAME) res = HandleRTCNFrame();
	else return CTLS_FAILED;

	if(res != CTLS_SUCCESS) return res;	
	
	BuildSetDateMsgDF();
	res = SendRxMsg(0);
	if(res != CTLS_SUCCESS) return res;
	
	if(GetFrameType() == V1_ACK_FRAME) res = CTLS_SUCCESS;
	else if(GetFrameType() == V1_NACK_FRAME) res = HandleRTCNFrame();
	else return CTLS_FAILED;	

	if(res != CTLS_SUCCESS) return res;	
	
	BuildSetTimeMsgCF();
	res = SendRxMsg(0);
	if(res != CTLS_SUCCESS) return res;
	
	if(GetFrameType() == V1_ACK_FRAME) res = CTLS_SUCCESS;
	else if(GetFrameType() == V1_NACK_FRAME) res = HandleRTCNFrame();
	else res = CTLS_FAILED;	
	
	return res;
}

static int SetCAPubKey(char *keyFile)
{	
	char dataLen1, dataLen2;
	int bytes = dir_get_file_sz(keyFile);
	int fHndl, res;	
	unsigned char rid[10], idx[10], mod[1000], exp[10], chk[100],Modata[2048];
	unsigned int modlen, explen;
	unsigned char capkdata[1000];
	char temp[1024], temp2[1024], temp3[1024],tmpdata[2048];
	char *ptr;
	unsigned long ulmodlen;
	int i=0;
	
	if(bytes <= 0) return CTLS_FAILED;	
	
	fHndl = open(keyFile, O_RDONLY);
	if(fHndl < 0) return CTLS_FAILED;	
	res = read(fHndl, temp2, bytes);	
	close(fHndl);
	if(res < 0) 
	{
		return CTLS_FAILED;
	}
	
	strcpy(temp,&keyFile[2]);
	
	// extract all required data
	SVC_DSP_2_HEX(temp, (char *)rid, strlen(temp) - 8);
	SVC_DSP_2_HEX(&temp[strlen(temp) - 7], (char *)idx, 2);
	
	strncpy(temp3, temp2, 3);
	temp3[3] = 0;
	modlen = atoi(temp3);
	SVC_DSP_2_HEX(&temp2[3], (char *)mod, modlen * 2);
	strncpy(temp3, &temp2[3 + modlen * 2], 2);
	temp3[2] = 0;
	explen = atoi(temp3);
	SVC_DSP_2_HEX(&temp2[5 + modlen * 2], (char *)exp, explen * 2);

	memset(Modata,0x00,sizeof(Modata));
	memcpy(Modata, rid, 5);
	Modata[5] = idx[0];
	memcpy(&Modata[6], mod, modlen);
	memcpy(&Modata[6 + modlen], exp, explen);
	
	
	ulmodlen = (unsigned long)modlen + explen + 6;
	memset(chk,0x00,sizeof(chk));
	SHA1(NULL,Modata,ulmodlen,chk);
	
	memset(tmpdata,0x00,sizeof(tmpdata));
	ptr=tmpdata;
	for(i=0;i<bytes-32;i++)
		ptr+=sprintf(ptr,"%c ",chk[i]);
	memset(tmpdata,0x00,sizeof(tmpdata));
	ptr=tmpdata;
	for(i=0;i<bytes-32;i++)
		ptr+=sprintf(ptr,"%02x ",chk[i]);
	memset(tmpdata,0x00,sizeof(tmpdata));
	ptr=tmpdata;
	for(i=0;i<bytes-32;i++)
		ptr+=sprintf(ptr,"%d ",chk[i]);
	LOG_PRINTFF(0x00000001L,"SetCAPubKey chk2:%s",tmpdata);
	
	// reassemble the data
	memset(capkdata, 0x00, sizeof(capkdata));
	memcpy(capkdata, rid, 5);
	capkdata[5] = idx[0];
	capkdata[6] = 0x01;
	capkdata[7] = 0x01;
	memcpy(&capkdata[8], chk, 20);
	memcpy(&capkdata[28 + 4 - explen], exp, explen);
	capkdata[33] = modlen;
	memcpy(&capkdata[34], mod, modlen);
	
	bytes = 34+modlen;
	dataLen1 = (bytes <= 244?bytes:244);
	dataLen2 = (bytes <= 244?0:bytes-244);		

	BuildSetCAPubKeyMsgCF(dataLen1, dataLen2);
	res = SendRxMsg(0);	
	if(res != CTLS_SUCCESS) 
	{
		return res;
	}
	
	if(GetFrameType() == V1_ACK_FRAME) res = CTLS_SUCCESS;
	else if(GetFrameType() == V1_NACK_FRAME) res = HandleKeyMgtNFrame();
	else 
	{
		return CTLS_FAILED;
	}
	
	if(res != CTLS_SUCCESS) 
	{
		return res;		
	}
	BuildSetCAPubKeyMsgDF((char *)capkdata, dataLen1);
	res = SendRxMsg(0);	
	if(res != CTLS_SUCCESS)
	{
		return res;
	}
	if(GetFrameType() == V1_ACK_FRAME) res = CTLS_SUCCESS;
	else if(GetFrameType() == V1_NACK_FRAME) res = HandleKeyMgtNFrame();
	else 
	{
		return CTLS_FAILED;
	}
	if(res != CTLS_SUCCESS) 
	{
		return res;		
	}
	if(!dataLen2) 
	{
		return CTLS_SUCCESS;	
	}

	BuildSetCAPubKeyMsgDF((char *)(&capkdata[dataLen1]), dataLen2);
	res = SendRxMsg(0);	

	if(res != CTLS_SUCCESS) 
	{
		return res;
	}
	if(GetFrameType() == V1_ACK_FRAME) res = CTLS_SUCCESS;
	else if(GetFrameType() == V1_NACK_FRAME) res = HandleKeyMgtNFrame();
	else 
	{
		return CTLS_FAILED;
	}

	return res;
}  

static int SetCAPubKeys()
{	
	char fName[100] = {0};
	
	int res = DeleteCAPubKeys();
	if(res <= 0) return res;	
	strcpy(fName, "I:");
	if(dir_get_first(fName) < 0) return CTLS_FAILED;
	while(dir_get_next(fName) >= 0)
	{	
		if(strstr(fName, ".CEMV") || strstr(fName, ".cemv"))	
			if(SetCAPubKey(fName) != CTLS_SUCCESS) 				
			{
		//		LOG_PRINTFF(0x00000001L,"SetCAPubKeys 2");
				//return CTLS_FAILED;		
			}
	}
	
	return CTLS_SUCCESS;	
}
static int DeleteCAPubKeys(void)
{
	int res;
	BuildDelCAPubKeysMsg();
	res = SendRxMsg(2000);
	LOG_PRINTFF(0x00000001L,"DeleteCAPubKeys");
	if(res != CTLS_SUCCESS) return res;

	if(GetFrameType() == V1_ACK_FRAME) res = CTLS_SUCCESS;
	else if(GetFrameType() == V1_NACK_FRAME) res = HandleKeyMgtNFrame();
	else return CTLS_FAILED;

	return res;	
}

static int SendCfgLine()
{
	char sLine[2048] = {0};
	char sLineHex[1024] = {0};
	int i = 0;

	LOG_PRINTFF(0x00000001L,"cfg file 0, %d",sCfgLength);

	memset(sLine,0,sizeof(sLine));
	if(sCfgData==NULL||sCfgLength<=0) return(CTLS_SUCCESS); //no file

	for(i=0;i<sCfgLength;i++)
	{
		char buff = sCfgData[i];
		if( buff=='\n' || buff =='\r' || i==sCfgLength-1) { // END of line
			if(sLine[0] =='#'){
				LOG_PRINTFF(0x00000001L,"cfg comment = %.100s", sLine);
			} else if(strlen(sLine) > 5 && sLine[0] == 'R') { // R+command +subcommand + data
				char *psLine = &sLine[1];
				int iHexLen = strlen(psLine)/2;
				int result=0;
				
				LOG_PRINTFF(0x00000001L,"cfg line = %.100s", psLine);
				memset(sLineHex,0,sizeof(sLineHex));
				SVC_DSP_2_HEX(psLine, sLineHex, iHexLen);
				BuildRawMsg(sLineHex,iHexLen);
				SendRxMsg(0);
				if(CtlsResp(0,&result)!=0 && sLineHex[0]==0xf1 && sLineHex[1] == 0x00) {
					//send additional aid parameters
					// F1/00:SET EXIST ADDITIONAL AID PARAM
					// F1/01:SET NEW   ADDITIONAL AID PARAM
					LOG_PRINTFF(0x00000001L,"cfg line F1/00->F1/01");
					sLineHex[1] = 0x01;
					BuildRawMsg(sLineHex,iHexLen);
					SendRxMsg(0);
				}

				// check the response
				if(		(result==0 && sLineHex[0] ==0x03 && sLineHex[1] == 0x04)
					||	(result==0 && sLineHex[0] ==0x03 && sLineHex[1] == 0x06)
					||	(result==0 && sLineHex[0] ==0xf0 && sLineHex[1] == 0x00)
						)
				{
					ExtractCtlsRsp(sLineHex);
				}
			}
			memset(sLine,0,sizeof(sLine));
		} else
		if(buff == 'R' || buff == '#' || isxdigit(buff) || sLine[0] == '#') {
			sLine[strlen(sLine)] =  buff;
		}
	}
	
	return(CTLS_SUCCESS);
}
/////////////////////////////////////////////

/////////////PUBLIC CONTACTLESS FUNCS//////////////

typedef struct AcquireCardParams_
{
	char waitForRsp;
	int timeoutMs;
} AcquireCardParams;

int AcquireCard(char waitForRsp, int timeoutMs, _ctlsStru * ptr_ctls )
{			
	int res = -1;
	void *params = 0;		
	p_ctls = ptr_ctls;
	LOG_PRINTFF(0x00000001L,"AcquireCard");
		
	params = calloc(1, sizeof(AcquireCardParams));
	if(!params)
	{		
		return res;
	}

	if(p_ctls->nosaf) {
		char lcdMsg[100];
		strcpy(lcdMsg, "%F3%Pcc12Declined");
		//StoreLCDMsg(0x0d, lcdMsg, "", "", "");
	}
	
	((AcquireCardParams*)params)->waitForRsp = waitForRsp;
	((AcquireCardParams*)params)->timeoutMs = timeoutMs;
	LOG_PRINTFF(0x00000001L,"before CbAcquireCard" );
	res = CbAcquireCard(params);

	LOG_PRINTFF(0x00000001L,"AcquireCard res:%d",res);
	free(params);
	return res;
}

//Not to be called without first calling AcquireCard
int ProcessCard()
{
	int res = -1;	
	res = CbProcessCard( (void*)0);	
	return res;
}

void CancelAcquireCard(int reason)
{	
	void *params = 0;

	if(p_ctls->nosaf) {
		//char lcdMsg[] = "%F2%Pcc00Approved\n%Pcc15Thank you";
		//StoreLCDMsg(0x0d, lcdMsg, "", "", "");
	}

	LOG_PRINTFF(0x00000001L,"CancelAcquireCard");
	params = calloc(1, sizeof(int));
	if(!params)
	{
		return;
	}
	*(int*)params = reason;
	LOG_PRINTFF(0x00000001L,"CancelAcquireCard 1");
	CbCancelAcquireCard(params);
	free(params);
}


///////Execute once callbacks///////
static int CbAcquireCard(void *pParams)
{			
	SetEMVTxnAmt();
	return GetPayment(((AcquireCardParams*)pParams)->waitForRsp, 
									((AcquireCardParams*)pParams)->timeoutMs);
}
static int CbProcessCard(void *pParams)
{
	int ret = -1;
	int statusCode;
	
	statusCode = GetRspStatus();
	if(strlen(p_ctls->TxnStatus)==0) sprintf(p_ctls->TxnStatus,"%2d",statusCode);
	LOG_PRINTFF(0x00000001L,"CbProcessCard %s",p_ctls->TxnStatus);
	switch(statusCode)
	{	
	case CTLS_NO_REPLY:
		return CTLS_NO_CARD;
	case V2_FAILED_NAK:			
		ret = HandleErrRsp();	
		LOG_PRINTFF(0x00000001L,"CTLS FAILED NAK 1");
		return ret;
	case V2_REQ_OL_AUTH:		
		LOG_PRINTFF(0x00000001L,"REQ ONL AUTH");
		{
			int extRes = ExtractCardDetails();			
			LOG_PRINTFF(0x00000001L,"extract card details res is %d", extRes);
			
			if(extRes < 0)
			{				
				return CTLS_BAD_CARD_READ;
			}
			return extRes | CTLS_REQ_OL;
		}

	case V2_OK:		
		LOG_PRINTFF(0x00000001L,"OFFLINE AUTH");
		{
			int extRes = ExtractCardDetails();

			LOG_PRINTFF(0x00000001L,"extract card details res is %d", extRes);  
			
			if(extRes < 0)
			{	
				LOG_PRINTFF(0x00000001L,"V2_OK 1");
				return CTLS_BAD_CARD_READ;
			}			
			LOG_PRINTFF(0x00000001L,"V2_OK 2" );
			return extRes | CTLS_AUTHD;
		}

	case V2_INCORRECT_TAG_HDR:
	case V2_UNK_CMD:
	case V2_UNK_SUB_CMD:
	case V2_CRC_ERR:
	case V2_INCORRECT_PARAM:
	case V2_PARAM_NOT_SUPPORTED:
	case V2_MAL_FORMATTED_DATA:
	case V2_TXN_TIMED_OUT:
	case V2_CMD_NOT_ALLOWED:
	case V2_SUB_CMD_NOT_ALLOWED:
	case V2_BUFF_OVERFLOW:
	case V2_USER_IF_EVT:
	default:
		LOG_PRINTFF(0x00000001L,"UNEXPECTED RSP STATUS %d !!", statusCode);
		break;
	}
			
	return CTLS_UNKNOWN;
}
static int CbCancelAcquireCard(void *pParams)
{	
	LOG_PRINTFF(0x00000001L,"CbCancelAcquireCard");
	return CancelTransaction(*(int*)pParams);
}
////////////////////////

static void InitRdr(char comPortNumber)
{
	char lcdMsg[256] = {0};			
	static int ctlsInitRes = 0;
	static int  hCtlsRdr = -1;
	
	UtilStrDup(&sCfgData, NULL);
	sCfgData = IRIS_GetObjectData_all("CTLSEMVCFG.TXT", (unsigned int *)&sCfgLength, 0);

	//	LOG_PRINTFF(0x00000001L,"InitRdr");
	if(hCtlsRdr < 0) 
	{
		hCtlsRdr = InitComPort(comPortNumber);			
	}
	if(hCtlsRdr >= 0) 
	{	
		ctlsInitRes = SetPollOnDemandMode();
	}
//inSetCallbacks(); //boyang
	
	if(ctlsInitRes > 0) 
	{
		SendCfgLine();
	}
	
	if(ctlsInitRes > 0) 
	{
		ctlsInitRes = StoreLCDMsg(0xFF, "", "", "", "");
	}	
	if(ctlsInitRes > 0) 
	{
		strcpy(lcdMsg, "%F3%Pcc12PLEASE\n%Pcc37TRY AGAIN");
		ctlsInitRes = StoreLCDMsg(0x09, lcdMsg, "", "", "");
	}
	if(ctlsInitRes > 0) 
	{
		strcpy(lcdMsg, "%F3%Pcc12PRESENT 1 CARD\n%Pcc37ONLY");
		ctlsInitRes = StoreLCDMsg(0x0A, lcdMsg, "", "", "");
	}
	if(ctlsInitRes > 0) 
	{
		strcpy(lcdMsg, "%F3%Pcc12INSERT Or SWIPE\n%Pcc37CARD");
		ctlsInitRes = StoreLCDMsg(0x08, lcdMsg, "", "", "");
	}
	if(ctlsInitRes > 0) 
	{
		strcpy(lcdMsg, "%F3%Pcc12TRANSACTION CANCELLED..");
		ctlsInitRes = StoreLCDMsg(0x02, lcdMsg, "", "", "");
	}
	if(ctlsInitRes > 0) 
	{
		strcpy(lcdMsg, "%F3%Pcc12PROCESSING..");
		ctlsInitRes = StoreLCDMsg(0x03, lcdMsg, "", "", "");
	}
	if(ctlsInitRes > 0) 
	{
		strcpy(lcdMsg, "%F3%Pcc37PLEASE WAIT..");
		ctlsInitRes = StoreLCDMsg(0x04, lcdMsg, "", "", "");
	}
	if(ctlsInitRes > 0) 
	{
		strcpy(lcdMsg, "%F3%Pcc12TRANSACTION FAIL");
		ctlsInitRes = StoreLCDMsg(0x05, lcdMsg, "", "", "");
		//ctlsInitRes = StoreLCDMsg(0x05, "", "", "", "");
	}	
	if(ctlsInitRes > 0) 
	{
		strcpy(lcdMsg, "%F3%Pcc00TOTAL:%S1 %S3\n%Pcc30SWIPE,INSERT OR TAP");
		ctlsInitRes = StoreLCDMsg(0x06, lcdMsg, "", "", "");
	}	
	if(ctlsInitRes > 0) 
	{
		strcpy(lcdMsg, "%F2%Pcc15Thank you");
		StoreLCDMsg(0x0D, lcdMsg, "", "", "");

		strcpy(lcdMsg, "%F2%Pcc22Not Authorised");
		ctlsInitRes = StoreLCDMsg(0x0E, lcdMsg, "", "", "");

		strcpy(lcdMsg, "%F2%Pcc15Thank you");
		StoreLCDMsg(0x0F, lcdMsg, "", "", "");
	}
	{
		strcpy(lcdMsg, "%F3%Pcc12 ");
		ctlsInitRes = StoreLCDMsg(23, lcdMsg, "", "", "");
	}
	
	if(ctlsInitRes > 0) 
	{
		ctlsInitRes = SetSource();   
	}
	
	if(ctlsInitRes > 0)
	{
		ctlsInitRes = SetDateTime();
	}
	
	if(ctlsInitRes > 0) 
	{
		ctlsInitRes = SetCAPubKeys();
	}

	if(CTLSDEBUG) {
		int iret = 0;
		/*
		LOG_PRINTFF(0x00000001L,"Get Configurable AID (GCA) 41010");
		SendGeneralMsg(0x03,0x04,"\x9f\x06\x07\xa0\x00\x00\x00\x04\x10\x10",10);
		LOG_PRINTFF(0x00000001L,"Get Configurable AID (GCA) 31010");
		SendGeneralMsg(0x03,0x04,"\x9f\x06\x07\xa0\x00\x00\x00\x03\x10\x10",10);
		LOG_PRINTFF(0x00000001L,"Get Configurable AID (GCA) 2501");
		SendGeneralMsg(0x03,0x04,"\x9f\x06\x06\xa0\x00\x00\x00\x25\x01",9);
		//SVC_WAIT(100);
		LOG_PRINTFF(0x00000001L,"Get All Additional AID Parameters ");
		SendGeneralMsg(0xF0,0x01,NULL,0);
		//SVC_WAIT(100);
		LOG_PRINTFF(0x00000001L,"Get Additional AID Parameters 41010");
		iret = SendGeneralMsg(0xF0,0x00,"\x9f\x06\x07\xa0\x00\x00\x00\x04\x10\x10",10);
		if(iret==CTLS_SUCCESS) ExtractCtlsRsp("F000");
		//SVC_WAIT(100);
		LOG_PRINTFF(0x00000001L,"Get Additional AID Parameters 31010");
		iret = SendGeneralMsg(0xF0,0x00,"\x9f\x06\x07\xa0\x00\x00\x00\x03\x10\x10",10);
		if(iret==CTLS_SUCCESS) ExtractCtlsRsp("F000");
		//SVC_WAIT(100);
		LOG_PRINTFF(0x00000001L,"Get Additional AID Parameters 2501");
		iret = SendGeneralMsg(0xF0,0x00,"\x9f\x06\x06\xa0\x00\x00\x00\x25\x01",9);
		if(iret==CTLS_SUCCESS) ExtractCtlsRsp("F000");
		//SVC_WAIT(100);
		LOG_PRINTFF(0x00000001L,"Get Additional Reader Parameters ");
		//SendGeneralMsg(0xF6,0x01,"\x00",0);
		 *
		 */
	}

	LOG_PRINTFF(0x00000001L,"InitRdr out");
}

void InitCtlsPort(void)
{
	InitRdr('0');
}

int GetCtlsTxnLimit(char *aid,  int *p_translimit, int *p_cvmlimit,int *p_floorlimit)
{

		/*int i =0;
		char aid_chk[30];
		char aid_chk2[30];

		strnlwr (aid_chk,aid ,strlen(aid));

		for(i=0;;i++){
			if( AIDlist[i].GroupNo==0) break;
			strnlwr (aid_chk2 , (char *)(AIDlist[i].AID) ,strlen(AIDlist[i].AID));

			if( strncmp( aid_chk,aid_chk2,strlen(aid_chk2))==0) {
					*p_floorlimit = 0;//TODO
					*p_translimit = AIDlist[i].TranLimitExists ;
					*p_cvmlimit = AIDlist[i].CVMReqLimitExists ;
			}
		}
		*/
	char sValue[40]="";
	long lValue=0;

	strcpy(sValue,"");
	CfgAidTLVFind(aid, 0xfff1, sValue);
	if(strlen(sValue)) *p_translimit = atol(sValue);

	strcpy(sValue,"");
	CfgAidTLVFind(aid, 0xfff5, sValue);
	if(strlen(sValue)) *p_cvmlimit = atol(sValue);

	strcpy(sValue,"");
	CfgAidTLVFind(aid, 0x1ff5, sValue);
	if( strlen(sValue)) {
		lValue = strtol(sValue,NULL,16);
		*p_floorlimit = lValue;

	}
	return(0);
}

int CTLSEmvGetTac(char *tac_df,char *tac_dn,char *tac_ol, const char *AID)
{

		char sValue[40];

		strcpy(tac_df,"");
		strcpy(tac_dn,"");
		strcpy(tac_ol,"");

		CfgAidTLVFind(AID, 0xfffe, sValue);
		strcpy(tac_df,sValue);
		CfgAidTLVFind(AID, 0xffff, sValue);
		strcpy(tac_dn,sValue);
		CfgAidTLVFind(AID, 0xfffd, sValue);
		strcpy(tac_ol,sValue);

		return(0);
}

AID_DATA* getCtlsAIDlist()
{
	return (AIDlist);
}

int CtlsResp(int setget, int *ret)
{
	static int result = 0;

	if(setget == 0) //get
		*ret = result;
	if(setget ==1) //set
		result = *ret;

	return(result);
}


int CfgGroupAdd(unsigned int *pGroup)
{
	CfgGroup *ptr = g_cfgGroups;
	CfgGroup *prev = ptr;
	CfgGroup *newGrp = NULL;

	while(ptr) {
		if(ptr->groupno == *pGroup) return(1);
		else {
			prev = ptr;
			ptr = ptr->next;
		}
	}

	newGrp = (CfgGroup *)malloc(sizeof(CfgGroup));
	newGrp->groupno = *pGroup;
	newGrp->AIDs = NULL;
	newGrp->TLVs = NULL;
	newGrp->next = NULL;

	if(g_cfgGroups==NULL) {
		g_cfgGroups = newGrp;
	}
	else {
		prev->next = newGrp;
	}

	return(0);
}

int CfgAidAdd(int *pGroupno,char *sAid)
{
	CfgGroup *ptr = g_cfgGroups;
	CfgAid *pAid = NULL;
	CfgAid *prev = NULL;
	CfgAid *newAid = NULL;
	int found = 0;

	if(pGroupno ==NULL ) return(-1);

	while(ptr && !found) {
		if(ptr->groupno == *pGroupno) found = 1;
		else ptr = ptr->next;
	}

	if(!found) return(-1);	//Group not found

	if(ptr->AIDs) {
		pAid = ptr->AIDs;
		while(pAid) {
			if(strlen(pAid->sAid)&& strncmp(pAid->sAid,sAid,strlen(pAid->sAid))==0) return(1); //AID exists in this group
			else {
				prev = pAid;
				pAid = pAid->next;
			}
		}
	}

	newAid = (CfgAid *)malloc(sizeof(CfgAid));
	strcpy(newAid->sAid, sAid);
	newAid->TLVs = NULL;
	newAid->next = NULL;

	if(ptr->AIDs){
		//DebugDisp("insert grp[%d],AID [%s] add",ptr->groupno,sAid);
		prev->next = newAid;
	} else {
		//DebugDisp("insert grp[%d], AID [%s] new",ptr->groupno,sAid);
		ptr->AIDs = newAid;
	}
	return(0);
}

CfgAid* CfgAidFind(char *sAid,int *grpno)
{
	CfgGroup *ptr = g_cfgGroups;
	CfgAid *pAid = NULL;
	int found = 0;

	if(sAid ==NULL || strlen(sAid)== 0 ) return(NULL);

	while(ptr && !found) {
		if(ptr->AIDs) {
			pAid = ptr->AIDs;
			while(pAid && !found) {
				if(strlen(pAid->sAid) && strncmp(pAid->sAid,sAid,strlen(pAid->sAid))==0) {
					*grpno = ptr->groupno;
					found = 1;
				}
				else {
					pAid = pAid->next;
				}
			}
		}
		ptr = ptr->next;
	}

	return(pAid);
}

CfgAidTlv * CfgTLVAdd(CfgAidTlv *pAidTlv, long Tag, char* sValue)
{
	CfgAidTlv *pTLV = NULL;
	CfgAidTlv *prev = NULL;
	CfgAidTlv *newTlv = NULL;

	//if(pAidTlv==NULL) return(NULL);

	pTLV = pAidTlv;

	while(pTLV) {
		if(pTLV->tag==Tag)
			return(NULL); //found
		else {
			prev = pTLV;
			pTLV = pTLV->next;
		}
	}

	newTlv = malloc(sizeof(CfgAidTlv));
	newTlv->next = NULL;
	newTlv->tag = Tag;
	newTlv->val = malloc(strlen(sValue)+1);
	strcpy(newTlv->val,sValue);

	if(prev==NULL) {
		prev = newTlv;
	}
	else {
		prev->next = newTlv;
	}

	return(prev);
}

int CfgAidTLVAdd(int* pGroupno, char *sAid, long Tag, char* sValue)
{
	CfgGroup *pGrp = g_cfgGroups;
	CfgAid *pAid = NULL;
	CfgAidTlv *pTLV = NULL;
	int found = 0;

	if(pGroupno ==NULL) return(0);

	pGrp = (CfgGroup *) g_cfgGroups;
	while(pGrp) {
		if(*pGroupno == ((CfgGroup *)pGrp)->groupno) {
			found = 1;
			break;
		}
		pGrp = pGrp->next;
	}

	if(!found) return(0);

	if(sAid && strlen(sAid)) {
		found = 0;
		pAid = pGrp->AIDs;
		while(pAid) {
			if(pAid->sAid && strlen(pAid->sAid) && strncmp(sAid,pAid->sAid,strlen(pAid->sAid))==0) {
				found = 1; break;
			}
			else pAid = pAid->next;
		}
		if(!found) return(0);
	}


	if(pAid==NULL) {
		pTLV = CfgTLVAdd(pGrp->TLVs, Tag, sValue);
		if(pGrp->TLVs == NULL)	pGrp->TLVs = pTLV;
	}
	else {
		pTLV = CfgTLVAdd(pAid->TLVs, Tag, sValue);
		if(pAid->TLVs == NULL)	pAid->TLVs = pTLV;
	}

	CfgTLVAdd(pTLV, Tag, sValue);
	return(0);
}

int CfgAidTLVFind(char *sAid, long Tag, char* sValue)
{
	CfgGroup *pGrp = NULL;
	CfgAid *pAid = NULL;
	CfgAidTlv *pTLV = NULL;
	unsigned int grpno = 0;

	if(sAid==NULL||strlen(sAid)==0) return(0);

	pAid = CfgAidFind(sAid, &grpno);
	if(pAid == NULL) {
		return(-1);
	}
	pTLV = pAid->TLVs;

	if(pTLV==NULL) {
		return(-1);
	}

	while(pTLV) {
		if(pTLV->tag==Tag)
			break; //found
		else pTLV = pTLV->next;
	}
	if(pTLV != NULL) {
		strcpy(sValue,pTLV->val);
		return(0);
	} else {

	}

	pGrp = g_cfgGroups;
	pTLV = NULL;
	while(pGrp) {
		if(pGrp->groupno == grpno) {
			pTLV = pGrp->TLVs;
			break;
		} else {
			pGrp = pGrp->next;
		}
	}

	while(pTLV) {
		if(pTLV->tag==Tag)
			break; //found
		else pTLV = pTLV->next;
	}
	if(pTLV != NULL) {
		strcpy(sValue,pTLV->val);
		return(0);
	} else {

	}

	return(0);
}

int CfgParseTag(unsigned long lTag,char *sTag,char *sTagDesc)
{
	unsigned long tag = lTag;
	unsigned char *p = (unsigned char *)&tag;
	unsigned char sDesc[1024]="";
	unsigned char stmp[10];

	sprintf(stmp,"%2.2x%2.2x%2.2x%2.2x", *p,*(p+1),*(p+2),*(p+3));
	p= stmp;
	while(*p =='0') p++;
	strcpy(sTag,p);

	switch(lTag) {
	case	0x5f2a:
				strcpy(sDesc,"Trans Currency Code");
				break;
	case	0x9f1a:
				strcpy(sDesc,"Trans country Code");
				break;
	case	0xFFFD:
				strcpy(sDesc,"Term ActionCode-Online ");
				break;
	case	0xFFFE:
				strcpy(sDesc,"Term ActionCode-Default");
				break;
	case	0xFFFF:
				strcpy(sDesc,"Term ActionCode-Denial ");
				break;
	case	0x9f35:
				strcpy(sDesc,"Terminal type");
				break;
	case	0x9f33:
				strcpy(sDesc,"Terminal capabilities");
				break;
	case	0x9f40:
				strcpy(sDesc,"Addi Terminal capa");
				break;
	case	0x1ff5:
				strcpy(sDesc,"Floor limit Domes-HEX");
				break;
	case	0x1ff9:
				strcpy(sDesc,"Floor limit inter-HEX");
				break;
	case	0xfff1:
				strcpy(sDesc,"CTLS Transaction Limit");
				break;
	default:
			break;
	}

	if(strlen(sDesc))	sprintf(sTagDesc,"(%s)",sDesc);
	else strcpy(sTagDesc,"");
	return(0);
}

int CfgPrtCfg(char *sAid)
{
	CfgGroup *pGrp = g_cfgGroups;
	CfgAid *pAid = NULL;
	CfgAidTlv *pTLV = NULL;
	unsigned int grpno = 0;
	char stmp[10240];

	strcpy(stmp,"\033k042CTLS config\n");
	if(true) {
		while(pGrp){
			pAid = pGrp->AIDs;
			while(pAid) {
				if(sAid && strlen(sAid) && strcmp(sAid,pAid->sAid)!=0) {
					pAid = pAid ->next;
					continue;
				}
				sprintf(&stmp[strlen(stmp)],"\nAID=%s\n",pAid->sAid);
				//print Group TLVs
				pTLV = pGrp->TLVs;
				while(pTLV) {
					char sTag[10],sDesc[100];
					CfgParseTag(pTLV->tag,sTag,sDesc);
					sprintf(&stmp[strlen(stmp)]," %s%s=%s\n",sTag,sDesc,pTLV->val);
					pTLV=pTLV->next;
				}

				//print AID TLVs
				pTLV = pAid->TLVs;
				while(pTLV) {
					char sTag[10],sDesc[100];
					CfgParseTag(pTLV->tag,sTag,sDesc);
					sprintf(&stmp[strlen(stmp)]," %s%s=%s\n",sTag,sDesc,pTLV->val);
					pTLV=pTLV->next;
				}
				pAid = pAid->next;
			}
			pGrp = pGrp->next;
		}
		strcat(stmp,"\n\n\n\n");
		PrtPrintBuffer(strlen(stmp), (uchar *)stmp, E_PRINT_END);//E_PRINT_END
	}

}

int CtlsGetCfg(char *sAid,unsigned long lTag,char *sVal)
{
	CfgGroup *pGrp = g_cfgGroups;
	CfgAid *pAid = NULL;
	char sValue[1024] = "";

	if(sAid && strlen(sAid) && lTag ) {
		CfgAidTLVFind(sAid, lTag, sValue);
		strcpy(sVal,sValue);
	} else {
		//CfgPrtCfg(NULL);
		CfgPrtCfg(NULL);
		//print all
	}
	return(0);
}
