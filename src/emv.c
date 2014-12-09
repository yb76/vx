
#ifdef __EMV
/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       emv.c
**
** DATE CREATED:    8 September 2010
**
** AUTHOR:			
**
** DESCRIPTION:     This module handles all EMV functionality
**
**The functions that can be called from the payment application in the following order:
*/
// for EOS log
#include <eoslog.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <cardslot.h>
#include "alloc.h"
#include "emv.h"
#include "iris.h"
#include "irisfunc.h"
#include "display.h"

#include "ctlsmod.h"
#include "include\printer.h"

//Dwarika .. commented due to compilation error
//enum cryptogram{ GEN_AAR = 0xC0, GEN_ARQC = 0x80, GEN_TC = 0x40, GEN_AAC = 0x00, GEN_CDDATC = 0x50, GEN_CDDAARQC = 0x90};


/************* GLOBAL VARIABLES ***********/
EMV_GLOBAL gEmv;

static	struct _emvcfg_list {
		int total;
		char *filename[15];
		int current;
} emvcfg_list;

int EmvGetPTermID(char *ptid)
{
    char    tempPTID [EMV_TERM_ID_SIZE + 2] = {0};
    SVC_INFO_PTID (tempPTID);
    strncpy (ptid, tempPTID+1, EMV_TERM_ID_SIZE);
    ptid[EMV_TERM_ID_SIZE+1] = '\0';
    return (EMV_SUCCESS);
}

/**
** 	int	EMVPowerOn(void) - the first call from the payment application. 
**	Needs to be called only once at start up.
** 	
**	This function performs all processing required during terminal power-on.
**	This function performs the following tasks: 
**			Initialises all internal data structures
**			Sets up all callback functions required by the calling application
**			Initialises the underlying EMV kernel
**	
** returns:
** 	#SUCCESS				Operation completed successfully.
**	#ERR_EMV_DATA_NOT_AVAILIBLE	Unable to read EMV kernel configuration details.
**	#ERR_EMV_CONFIG_ERROR		EMV kernel configuration operation failed, or callback functions could not be set up.
**	#ERR_EMV_FAILURE			EMV kernel intialisation failed.
**
**	TO DO: 
**	
**	2. AID data restore - get data from TMS, store it in a file, then make sure the file is present here??
**
*/
int	EMVPowerOn(void)
{
	char	atEmvKernelVersion[22];
	int		status = 0;
	static int started = 0;
	char	acAidEst[20]="";
	char	emvCfgfile[40]="";
	srAIDList aidList;
	int i = 0;

	if( started ) return(0);
	DispInit();

	/*
	** Normal terminal power-on processing
	*/
	/* Retrieve & store underlying EMV Kernel version details */
	memset(atEmvKernelVersion, 0, sizeof(atEmvKernelVersion));
	status = usVXEMVAPGetVersion(atEmvKernelVersion);
	if (status != SUCCESS)
	{
		return(ERR_EMV_FAILURE);
	}

	/* EMV Kernel Initial/Power-On Configuration */
	/*
	** Load initial/default configuration from EST.DAT & MVT.DAT files
	*/
	status = inLoadMVTRec(0);
	if (status != SUCCESS)
	{
		return(ERR_EMV_CONFIG_ERROR);
	}

	/* Setup callback functions - Verifone kernel will be calling these functions when needed. Functions are supplied by thin client */
	status = EmvCallbackFnInit();
	if (status != SUCCESS)
	{
		return(ERR_EMV_CONFIG_ERROR);
	}

	/* Underlying EMV Kernel initialisation */
	status = inVXEMVAPSCInit();
	if (status != SUCCESS)
	{
		return(ERR_EMV_CONFIG_ERROR);
	}

	memset(&emvcfg_list,0,sizeof(emvcfg_list));
	emvcfg_list.total = 0;

	status = inGetESTSupportedAIDs(&aidList);
	for(i=0;i<aidList.countAID;i++) {
			if(aidList.arrAIDList[i].lenOfAID) {
				memset(acAidEst,0,sizeof(acAidEst));
				strncpy(acAidEst,(const char *)aidList.arrAIDList[i].stAID,aidList.arrAIDList[i].lenOfAID);

				UtilHexToString((const char*)aidList.arrAIDList[i].stAID, aidList.arrAIDList[i].lenOfAID, acAidEst);
				sprintf(emvCfgfile,"EMVCFG%s", acAidEst);
				if(FileExist(emvCfgfile)) {
					emvcfg_list.filename[emvcfg_list.total] = malloc ( strlen(emvCfgfile) +5 );
					strcpy(emvcfg_list.filename[emvcfg_list.total], emvCfgfile);
					emvcfg_list.total ++;
				}
			}
	}


	started = 1;
	return(SUCCESS);
}

int	EMVTransInit(void)
{
	int				status;
	char transType;
	char serialBuf[20];

	EMVPowerOn();
	// init global
	memset( &gEmv,0,sizeof(gEmv));
	emvcfg_list.current = 0;

	/*
	** Sanity check
	*/
	if(!EmvIsCardPresent()) return(ERR_EMV_CARD_REMOVED);

	/**********************************************************************
	** Transaction Initialisation.
	** This needs to be called at the start of each transaction.
	***********************************************************************/
	status = inVXEMVAPTransInit(0, NULL);
	if (status != SUCCESS)
	{
		return(ERR_EMV_CONFIG_ERROR);
	}

	// set transaction type : sale(default) / cashback/ adjustment/ sale with cashback
	transType = 0x00; //sale
	usEMVAddTLVToCollxn(TAG_9C00_TRAN_TYPE, &transType, 1);

	SVC_INFO_SERLNO (serialBuf) ;
	if(!isalnum(serialBuf[0])) strcpy(serialBuf,"111111111");
	usEMVAddTLVToCollxn(TAG_9F1E_IFD_SER_NUM , serialBuf, 8);

	status = inVXEMVAPCardInit();
	if (status != SUCCESS)
	{
		if(status == NO_ATR || status==CHIP_ERROR || status==INVALID_ATR) gEmv.techfallback = true;
		return(status);
	}

	return(SUCCESS);
}

/*
**	CALLED BY PAYMENT APP
**
** 	E_EMV	EMVSelectApplication(void) - called by the payment application at the start of a new transaction
**	in order for application selection to be processed
**
**	Perform all steps for select EMV application
**
**   return:
** 	 #SUCCESS	Operation completed successfully.
**					Calling application may examine EMV tag
**					data to determine the status of the transaction.
**	 #ERR_EMV_CLIENT_ABORTED Transaction was cancelled by the customer/terminal-operator
**	 #ERR_EMV_CARD_REMOVED	Card was removed prematurely from the ICC
**							Operation failed. Return code is the error code returned by the called
**							function that failed.
**	TO DO:
**	The callback functions need to be finished/done
**
*/
int	EMVSelectApplication(const long amt, const long acc)
{
	int				iAutoSelect=inGetAutoSelectApplnFlag();
	unsigned int 	usDefaultRecordRFU1 = inGetShortRFU1();
	int				iStatus;
	unsigned long 	pullTxnAmt=0;

	/*************************************************************
	** Application selection

	inVXEMVAPSelectApplication() EMV kernel API performs both application
	selection and GPO, unless specified to split the functionality to perform only
	application selection, by setting the MVT short RFU1 field's bit 8. If the
	application has chosen to perform application selection and GPO separately,
	then the application can perform the language selection prior calling
	inVXEMVAPPerformGPO() API.

	*************************************************************/

	if (amt > 0) gEmv.amt = amt;

	iStatus = inVXEMVAPSelectApplication(iAutoSelect, EmvCallbackFnSelectAppMenu, EmvCallbackFnPromptManager, EmvFnAmtEntry, EmvFnCandListModify);
	if( (usDefaultRecordRFU1 & 0x0080) && (iStatus == SUCCESS) ) {
		iStatus = inVXEMVAPPerformGPO();
	}

	if (iStatus != SUCCESS)
	{	/*
		** No applications being available can either mean that there are actually
		** no applications on the card, or that there are one or more applications
		** on the card but they're blocked
		*/
		if (iStatus == CANDIDATELIST_EMPTY)
		{
			unsigned short cnt = 21;
			unsigned short	usAidBlockedTotal=0;
			srAIDListStatus	sAidListStatus[21];

			if(gEmv.appsTotal >0) cnt = gEmv.appsTotal;
			usEMVGetAllAIDStatus(sAidListStatus, &cnt, &usAidBlockedTotal);
			if (usAidBlockedTotal > 0)
			{
				iStatus = APPL_BLOCKED;
			}
		}

		if(iStatus == CARD_BLOCKED || iStatus == CARD_REMOVED||iStatus == TRANS_CANCELLED||iStatus == INVALID_PARAMETER|| iStatus ==APPL_BLOCKED )
			gEmv.techfallback = false ;
		else
			gEmv.techfallback = true ;
		return(iStatus);
	}


	return SUCCESS;
}

void getEMVAids(int indx,char *sAID)
{
	int i;
	long flrlimit = 0L;
	long flrlimit_I = 0L;
	char			acTacOnTmp[(2 * EMV_TAC_LENGTH) + 1];
	char			acTacDfTmp[(2 * EMV_TAC_LENGTH) + 1];
	char			acTacDnTmp[(2 * EMV_TAC_LENGTH) + 1];
	char			acTacOnTmp_I[(2 * EMV_TAC_LENGTH) + 1];
	char			acTacDfTmp_I[(2 * EMV_TAC_LENGTH) + 1];
	char			acTacDnTmp_I[(2 * EMV_TAC_LENGTH) + 1];
	char PrntBuff[150];

	ushort	iLen;
	char emvCfgfile [50];
	char *cfgdata=NULL;
	bool emvcfg = false;

	if(sAID) {
		sprintf(emvCfgfile,"EMVCFG%s", sAID);
  		cfgdata = (char*)IRIS_GetObjectData( emvCfgfile, (unsigned int*)&iLen);
		if(cfgdata) {
			emvcfg = true;
		} 
	}

	{

		char tag[128]="";
		char tag2[128]="";
		i = indx;
		//MVT
		inLoadMVTRec(i);
		
		flrlimit = lnGetEMVFloorLimit();
		flrlimit_I = flrlimit;
		if(emvcfg) {
        	char *stmp = (char*)IRIS_GetObjectTagValue( cfgdata, "D_FLOORLIMIT" );
			if(stmp) flrlimit = atol(stmp);
    		UtilStrDup(&stmp , NULL);

        	stmp = (char*)IRIS_GetObjectTagValue( cfgdata, "I_FLOORLIMIT" );
			if(stmp) flrlimit_I = atol(stmp);
    		UtilStrDup(&stmp , NULL);

		}

		{
			sprintf(PrntBuff,"FLOOR LIMIT_D:\\R%.2f\n",flrlimit/100.0);
  			__print_cont( PrntBuff, 0 );
			sprintf(PrntBuff,"FLOOR LIMIT_I:\\R%.2f\n",flrlimit_I/100.0);
  			__print_cont( PrntBuff, 0 );
		}
		
		memset(acTacOnTmp, 0, sizeof(acTacOnTmp));
		memset(acTacOnTmp_I, 0, sizeof(acTacOnTmp_I));
		szGetMVTEMVTACOnline(acTacOnTmp);
		strcpy(acTacOnTmp_I,acTacOnTmp);

		if(emvcfg) {
        	char *stmp = (char*)IRIS_GetObjectTagValue( cfgdata, "D_TAC_ONLINE" );
			if(stmp) strcpy( acTacOnTmp, stmp);
    		UtilStrDup(&stmp , NULL);

        	stmp = (char*)IRIS_GetObjectTagValue( cfgdata, "I_TAC_ONLINE" );
			if(stmp) strcpy( acTacOnTmp_I, stmp);
    		UtilStrDup(&stmp , NULL);
		}
		if(strlen(acTacOnTmp) > 0)
		{
			sprintf(PrntBuff,"TAC ONLINE_D:\\R%s\n",acTacOnTmp);
  			__print_cont( PrntBuff, 0 );
			sprintf(PrntBuff,"TAC ONLINE_I:\\R%s\n",acTacOnTmp_I);
  			__print_cont( PrntBuff, 0 );
		}
		
		memset(acTacDfTmp, 0, sizeof(acTacDfTmp));
		memset(acTacDfTmp_I, 0, sizeof(acTacDfTmp_I));
		szGetMVTEMVTACDefault(acTacDfTmp);
		strcpy(acTacDfTmp_I,acTacDfTmp);
		if(emvcfg) {
        	char *stmp = (char*)IRIS_GetObjectTagValue( cfgdata, "D_TAC_DEFAULT" );
			if(stmp) strcpy( acTacDfTmp, stmp);
    		UtilStrDup(&stmp , NULL);

        	stmp = (char*)IRIS_GetObjectTagValue( cfgdata, "I_TAC_DEFAULT" );
			if(stmp) strcpy( acTacDfTmp_I, stmp);
    		UtilStrDup(&stmp , NULL);
		}
		if(strlen(acTacDfTmp) > 0)
		{
			sprintf(PrntBuff,"TAC DEFAULT_D:\\R%s\n",acTacDfTmp);
  			__print_cont( PrntBuff, 0 );
			sprintf(PrntBuff,"TAC DEFAULT_I:\\R%s\n",acTacDfTmp_I);
  			__print_cont( PrntBuff, 0 );
		}
		
		memset(acTacDnTmp, 0, sizeof(acTacDnTmp));
		memset(acTacDnTmp_I, 0, sizeof(acTacDnTmp_I));
		szGetMVTEMVTACDenial(acTacDnTmp);
		strcpy(acTacDnTmp_I,acTacDnTmp);
		if(emvcfg) {
        	char *stmp = (char*)IRIS_GetObjectTagValue( cfgdata, "D_TAC_DENIAL" );
			if(stmp) strcpy( acTacDnTmp, stmp);
    		UtilStrDup(&stmp , NULL);

        	stmp = (char*)IRIS_GetObjectTagValue( cfgdata, "I_TAC_DENIAL" );
			if(stmp) strcpy( acTacDnTmp_I, stmp);
    		UtilStrDup(&stmp , NULL);
		}
		LOG_PRINTFF (0x00000001L, " acTacDnTmp:%s:",acTacDnTmp);
		if(strlen(acTacDnTmp) > 0)
		{
			sprintf(PrntBuff,"TAC DENIAL_D:\\R%s\n",acTacDnTmp);
  			__print_cont( PrntBuff, 0 );
			sprintf(PrntBuff,"TAC DENIAL_I:\\R%s\n",acTacDnTmp_I);
  			__print_cont( PrntBuff, 0 );
		}


		memset(tag, 0, sizeof(tag));
		if(emvcfg) {
        	char *stmp = (char*)IRIS_GetObjectTagValue( cfgdata, "D_RSTHRESHOLD" );
			if(stmp) strcpy( tag, stmp);
    		UtilStrDup(&stmp , NULL);
		}
		if( strlen(tag)==0) { 
			short rs = lnGetEMVRSThreshold();
			if(rs>0) sprintf(tag,"%d",rs);
		}
		if(strlen(tag) > 0)
		{
			sprintf(PrntBuff,"RS Threadhold:\\R%.2f\n",atol(tag)/100.0);
  			__print_cont( PrntBuff, 0 );
		}
		
		memset(tag, 0, sizeof(tag));
		if(emvcfg) {
        	char *stmp = (char*)IRIS_GetObjectTagValue( cfgdata, "D_RSTARGET" );
			if(stmp) strcpy( tag, stmp);
    		UtilStrDup(&stmp , NULL);
		}
		if( strlen(tag)==0) { 
			short rs = inGetEMVTargetRSPercent();
			if(rs>0) sprintf(tag,"%d",rs);
		}
		if(strlen(tag) > 0)
		{
			sprintf(PrntBuff,"RS Target:\\R%s%%\n",tag);
  			__print_cont( PrntBuff, 0 );
		}
		
		
		memset(tag, 0, sizeof(tag));
		if(emvcfg) {
        	char *stmp = (char*)IRIS_GetObjectTagValue( cfgdata, "D_RSMAX" );
			if(stmp) strcpy( tag, stmp);
    		UtilStrDup(&stmp , NULL);
		}
		if( strlen(tag)==0) { 
			short rs = inGetEMVMaxTargetRSPercent();
			if(rs>0) sprintf(tag,"%d",rs);
		}
		if(strlen(tag) > 0)
		{
			sprintf(PrntBuff,"RS Max Target:\\R%s%%\n",tag);
  			__print_cont( PrntBuff, 0 );
		}
		
		memset(tag, 0, sizeof(tag));
		memset(tag2, 0, sizeof(tag2));
		strcpy(tag,"NO");
		strcpy(tag2,"NO");
		if(emvcfg) {
        	char *stmp = (char*)IRIS_GetObjectTagValue( cfgdata, "D_PINBYPASS" );
			if(stmp) strcpy( tag, stmp);
    		UtilStrDup(&stmp , NULL);

        	stmp = (char*)IRIS_GetObjectTagValue( cfgdata, "I_PINBYPASS" );
			if(stmp) strcpy( tag2, stmp);
    		UtilStrDup(&stmp , NULL);
		}
		if(strlen(tag) > 0)
		{
			sprintf(PrntBuff,"PINBYPASS_D:\\R%s\n",tag);
  			__print_cont( PrntBuff, 0 );
			sprintf(PrntBuff,"PINBYPASS_I:\\R%s\n",tag2);
  			__print_cont( PrntBuff, 0 );
		}

		memset(PrntBuff,0x00,sizeof(PrntBuff));
		sprintf(PrntBuff,"\n\n");
  		__print_cont( PrntBuff, 0 );

	}
    UtilStrDup(&cfgdata , NULL);

}

void vPrintEMVAllAids(void)
{
	int retval = 0;
	int i;
	char			acAidEst[(2 * EMV_MAX_AID_LENGTH) + 1];
	char PrntBuff[50];
	char sAID[10][50];
	
	LOG_PRINTFF (0x00000001L, " vPrintEMVAllAids ");
	retval = inGetMVTRecNumber();
	LOG_PRINTFF (0x00000001L, " inGetMVTRecNumber retval:%d:",retval);
	
		
	sprintf(PrntBuff,"CONTACT EMV DETAILS \n");
  	__print_cont( PrntBuff, 0 );
	
	sprintf(PrntBuff,"----------------------\n \n");
  	__print_cont( PrntBuff, 0 );

	for(i = 1; i<5; i++)
	{
		short j=0;
		short iLen = 0;
		char *cfgdata = NULL;
		char emvCfgfile[30] = "";
		bool found = false;
		////EST
		strcpy(sAID[0],"");
		strcpy(sAID[1],"");
		strcpy(sAID[2],"");
		strcpy(sAID[3],"");
		strcpy(sAID[4],"");
		strcpy(sAID[5],"");
	
		retval = inLoadESTRec(i-1);
		LOG_PRINTFF (0x00000001L, " inLoadESTRec(%d) retval:%d:",i-1,retval);
		if(retval) break;

		memset(acAidEst, 0, sizeof(acAidEst));
		szGetESTSupportedAID1((char*)acAidEst);
		LOG_PRINTFF (0x00000001L, "\n acAidEst 1:%s:",acAidEst);
		if(strlen(acAidEst) > 0)
		{
			strcpy(sAID[0],acAidEst);
			sprintf(emvCfgfile,"EMVCFG%s", acAidEst);
  			cfgdata = (char*)IRIS_GetObjectData( emvCfgfile, (unsigned int*)&iLen);
			if(cfgdata) {
				sprintf(PrntBuff,"AID:\\R%s\n",acAidEst);
  				__print_cont( PrntBuff, 0 );
				getEMVAids(i,acAidEst);
    			UtilStrDup(&cfgdata , NULL);
				strcpy(sAID[0],"");
			}
		}

		memset(acAidEst, 0, sizeof(acAidEst));
		szGetESTSupportedAID2((char*)acAidEst);
		LOG_PRINTFF (0x00000001L, "\n acAidEst 2:%s:",acAidEst);
		if(strlen(acAidEst) > 0)
		{
			strcpy(sAID[1],acAidEst);
			sprintf(emvCfgfile,"EMVCFG%s", acAidEst);
  			cfgdata = (char*)IRIS_GetObjectData( emvCfgfile, (unsigned int*)&iLen);
			if(cfgdata) {
				sprintf(PrntBuff,"AID:\\R%s\n",acAidEst);
  				__print_cont( PrntBuff, 0 );
				getEMVAids(i,acAidEst);
    			UtilStrDup(&cfgdata , NULL);
				strcpy(sAID[1],"");
			}
		}
		
		memset(acAidEst, 0, sizeof(acAidEst));
		szGetESTSupportedAID3((char*)acAidEst);
		LOG_PRINTFF (0x00000001L, "\n acAidEst 3:%s:",acAidEst);
		if(strlen(acAidEst) > 0)
		{
			strcpy(sAID[2],acAidEst);
			sprintf(emvCfgfile,"EMVCFG%s", acAidEst);
  			cfgdata = (char*)IRIS_GetObjectData( emvCfgfile, (unsigned int*)&iLen);
			if(cfgdata) {
				sprintf(PrntBuff,"AID:\\R%s\n",acAidEst);
  				__print_cont( PrntBuff, 0 );
				getEMVAids(i,acAidEst);
    			UtilStrDup(&cfgdata , NULL);
				strcpy(sAID[2],"");
			}
		}
		
		memset(acAidEst, 0, sizeof(acAidEst));
		szGetESTSupportedAID4((char*)acAidEst);
		LOG_PRINTFF (0x00000001L, "\n acAidEst 4:%s:",acAidEst);
		if(strlen(acAidEst) > 0)
		{
			strcpy(sAID[3],acAidEst);
			sprintf(emvCfgfile,"EMVCFG%s", acAidEst);
  			cfgdata = (char*)IRIS_GetObjectData( emvCfgfile, (unsigned int*)&iLen);
			if(cfgdata) {
				sprintf(PrntBuff,"AID:\\R%s\n",acAidEst);
  				__print_cont( PrntBuff, 0 );
				getEMVAids(i,acAidEst);
    			UtilStrDup(&cfgdata , NULL);
				strcpy(sAID[3],"");
			}
		}
		
		memset(acAidEst, 0, sizeof(acAidEst));
		szGetESTSupportedAID5((char*)acAidEst);
		LOG_PRINTFF (0x00000001L, "\n acAidEst 5:%s:",acAidEst);
		if(strlen(acAidEst) > 0)
		{
			strcpy(sAID[4],acAidEst);
			sprintf(emvCfgfile,"EMVCFG%s", acAidEst);
  			cfgdata = (char*)IRIS_GetObjectData( emvCfgfile, (unsigned int*)&iLen);
			if(cfgdata) {
				sprintf(PrntBuff,"AID:\\R%s\n",acAidEst);
  				__print_cont( PrntBuff, 0 );
				getEMVAids(i,acAidEst);
    			UtilStrDup(&cfgdata , NULL);
				strcpy(sAID[4],"");
			}
		}
		
		memset(acAidEst, 0, sizeof(acAidEst));
		szGetESTSupportedAID6((char*)acAidEst);
		LOG_PRINTFF (0x00000001L, "\n acAidEst 6:%s:",acAidEst);
		if(strlen(acAidEst) > 0)
		{
			strcpy(sAID[5],acAidEst);
			sprintf(emvCfgfile,"EMVCFG%s", acAidEst);
  			cfgdata = (char*)IRIS_GetObjectData( emvCfgfile, (unsigned int*)&iLen);
			if(cfgdata) {
				sprintf(PrntBuff,"AID:\\R%s\n",acAidEst);
  				__print_cont( PrntBuff, 0 );
				getEMVAids(i,acAidEst);
    			UtilStrDup(&cfgdata , NULL);
				strcpy(sAID[5],"");
			}
		}

		for(j=0;j<=5;j++) {
		  if(strlen(sAID[j])) {
			sprintf(PrntBuff,"AID:\\R%s\n",sAID[j]);
  			__print_cont( PrntBuff, 0 );
			found = true;
		  }
		}
		if(found) getEMVAids(i,NULL);
	
	}
	
	sprintf(PrntBuff,"\n----------------------\n \n");
  	__print_cont( PrntBuff, 1 );
}

int inPrintCTLSEmvPrm()
{
	char stmp[20];
	char PrntBuff[150];
	int i = 0;

	AID_DATA *AIDlist = getCtlsAIDlist();

	strcat(PrntBuff,"\3CONTACTLESS EMV DETAILS \n");
	strcat(PrntBuff,"----------------------\n \n");
	strcat(PrntBuff,"\n");
  	__print_cont( PrntBuff, 0 );

	for(i=0;;++i)
	{

		if(AIDlist[i].GroupNo == 0|| AIDlist[i].AID[0] == 0x00) break;

		strcpy(stmp,(const char *)AIDlist[i].AID);
		stmp[0] = 'A';
		sprintf(PrntBuff,"AID : \\R%s\n",stmp);
  		__print_cont( PrntBuff, 0 );

		sprintf(PrntBuff,"PRIORITY: \\R%s\n",AIDlist[i].TermPriority);
  		__print_cont( PrntBuff, 0 );

		sprintf(PrntBuff,"APPL Ver :\\R%s\n",AIDlist[i].AppVer);
  		__print_cont( PrntBuff, 0 );

		sprintf(PrntBuff,"THRESHOLD:\\R%s\n",AIDlist[i].ThresholdD);
  		__print_cont( PrntBuff, 0 );

		sprintf(PrntBuff,"TARGET PERCENTAGE:\\R%s\n",AIDlist[i].TargetPerD);
  		__print_cont( PrntBuff, 0 );

		sprintf(PrntBuff,"MAX TARGET PERCENTAGE:\\R%s\n",AIDlist[i].MaxTargetD);
  		__print_cont( PrntBuff, 0 );

		UtilHexToString( (const char *)AIDlist[i].TACOnline , 5 , stmp);
		sprintf(PrntBuff,"TAC ONLINE :\\R%s\n",stmp);
  		__print_cont( PrntBuff, 0 );

		UtilHexToString( (const char *)AIDlist[i].TACDefault , 5 , stmp);
		sprintf(PrntBuff,"TAC DEFAULT:\\R%s\n",stmp);
  		__print_cont( PrntBuff, 0 );

		UtilHexToString( (const char *)AIDlist[i].TACDenial , 5 , stmp);
		sprintf(PrntBuff,"TAC DENIAL :\\R%s\n",stmp);
  		__print_cont( PrntBuff, 0 );

		sprintf(PrntBuff,"TXN LIMIT :\\R %.2f\n",AIDlist[i].TranLimitExists/100.0);
  		__print_cont( PrntBuff, 0 );

		UtilHexToString((const char *)AIDlist[i].FloorLimit,4,stmp);
		sprintf(PrntBuff,"FLOOR LIMIT:\\R %.2f\n",Hex_To_Dec(stmp)/100.0);
  		__print_cont( PrntBuff, 0 );

		sprintf(PrntBuff,"CVM LIMIT:\\R %.2f\n",AIDlist[i].CVMReqLimitExists/100.0);
  		__print_cont( PrntBuff, 0 );

		UtilHexToString((const char *)AIDlist[i].TermCapNoCVMReq,3,stmp);
		sprintf(PrntBuff,"TCC NO CVM REQ:\\R %s\n",stmp);
  		__print_cont( PrntBuff, 0 );

		UtilHexToString((const char *)AIDlist[i].TermCapCVMReq,3,stmp);
		sprintf(PrntBuff,"TCC CVM REQ:\\R%s\n\n",stmp);
  		__print_cont( PrntBuff, 0 );
		
	}	
	sprintf(PrntBuff,"----------------------\n\n\n");
  	__print_cont( PrntBuff, 1 );
	return(0);
}


/*
**	CALLED BY PAYMENT APP
**
** 	int	EMVTransPrepare(void) - called after select application is finished.
**	in order to perform reading from the card and basic data authentication
**
**	Perform all steps for preparing transaction for processing
**
**   return:
** 	 #SUCCESS				Operation completed successfully. Calling application may examine EMV tag data to determine the status of the transaction.
**	 #ERR_EMV_CLIENT_ABORTED	Transaction was cancelled by the customer/terminal-operator
**	 #ERR_EMV_CARD_REMOVED		Card was removed prematurely from the ICC
**								Operation failed. Return code is the error code returned by the called
**								function that failed.
*/
int	EMVReadAppData(void)
{
	int			status;

	/*
	** Sanity checks
	*/
	if(!EmvIsCardPresent()) return(ERR_EMV_CARD_REMOVED);
	
	/*
	** Load configuration data required for the transaction
	** NOTE: The TAG_ALREADY_PRESENT error code is benign (it's just informing
	** the calling application that a tag is already present) & therefore can
	** be ignored (it can actually occur when cards with proprietary tags are
	** used).
	*/
	status = inVXEMVAPGetCardConfig(-1, -1);
	if ((status != SUCCESS) && (status != TAG_ALREADY_PRESENT))
	{
		return(ERR_EMV_CONFIG_ERROR);
	}

	/*
	** Read application data from the card
	** NOTE: The TAG_ALREADY_PRESENT error code is benign (it's just informing
	** the calling application that a tag is already present) & therefore can
	** be ignored (it can actually occur when cards with proprietary tags are
	** used).
	*/
	status = inVXEMVAPProcessAFL();
	if ((status != SUCCESS) && (status != TAG_ALREADY_PRESENT))
	{
		if(status == APPL_BLOCKED || status == CARD_BLOCKED || status == 0x6985 )
				gEmv.techfallback = false;
		else
				gEmv.techfallback = true;
	}
    return(status);
}

int EmvFindCfgFile(char *aid,char *cfgfile)
{
		char emvCfgfile [50];
		int i = 0;
		bool found = false;

		strcpy(cfgfile,"");
		sprintf(emvCfgfile,"EMVCFG%s", aid);

		for(i=0;i<emvcfg_list.total;i++) {
			if(emvcfg_list.filename[i] ==NULL) break;
			// compare EMVCFGA0000002501 and AID A00000002501nnnn
			if(strncmp(emvCfgfile, emvcfg_list.filename[i],strlen(emvcfg_list.filename[i]))==0)
			{
				found = true ;
				strcpy(cfgfile,emvcfg_list.filename[i]);
				break;
			}
		}
		return(found);
}

int EMVCheckLocalCfgFile()
{
		uchar aid_hex[17]="";
		char aid[40]="";
		ushort iLen;
		char *cfgdata=NULL;
		char emvCfgfile [50];
		int i = 0;
		bool found = false;

		usEMVGetTLVFromColxn(TAG_4F_AID, aid_hex, &iLen);
		UtilHexToString((const char *)aid_hex,iLen,aid);
		sprintf(emvCfgfile,"EMVCFG%s", aid);

		for(i=0;i<emvcfg_list.total;i++) {
			if(emvcfg_list.filename[i] ==NULL) break;
			// compare EMVCFGA0000002501 and AID A00000002501nnnn
			if(strncmp(emvCfgfile, emvcfg_list.filename[i],strlen(emvcfg_list.filename[i]))==0)
			{
				found = true ;
				break;
			}
		}

		if(found) {
			emvcfg_list.current = i+1;

			strcpy(emvCfgfile, emvcfg_list.filename[i]);
  			cfgdata = (char*)IRIS_GetObjectData( emvCfgfile, (unsigned int*)&iLen);
			if(cfgdata) {
        		char *acct = (char*)IRIS_GetObjectTagValue( cfgdata, "ACCT" );
        		char *acct_cr = (char*)IRIS_GetObjectTagValue( cfgdata, "ACCT_CR" );
				if(acct) strcpy( gEmv.acct, acct);
				if(acct_cr && strcmp(acct_cr,"NO") == 0) gEmv.acct_nocr = true;
    			UtilStrDup(&acct , NULL);
    			UtilStrDup(&acct_cr , NULL);
    			UtilStrDup(&cfgdata , NULL);
			}
		}

	return SUCCESS;
}

int EMVDataAuth(void)
{
	int		iStatus;

	if(!EmvIsCardPresent()) return(ERR_EMV_CARD_REMOVED);

	iStatus = inVXEMVAPDataAuthentication(NULL);
	if (iStatus != SUCCESS)
	{
        return(iStatus);
	}

	return(SUCCESS);
}

int EMVProcessingRestrictions (void)
{
	int	iStatus;
	uchar aid_hex[17]="";
	char aid[40]="";
	uchar termCtry[3];
	uchar cardCtry[3];
	ushort	iLen;
	char emvCfgfile [50];
	char *cfgdata=NULL;

	if(!EmvIsCardPresent()) return(ERR_EMV_CARD_REMOVED);

	usEMVGetTLVFromColxn(TAG_4F_AID, aid_hex, &iLen);
	UtilHexToString((const char *)aid_hex,iLen,aid);

	if(emvcfg_list.current ) {
		strcpy(emvCfgfile, emvcfg_list.filename[emvcfg_list.current-1]);
  		cfgdata = (char*)IRIS_GetObjectData( emvCfgfile, (unsigned int*)&iLen);
	}
	if(cfgdata) 
	{
		char *sfloorlimit = NULL;
		char *sRSThreshold= NULL; 
		char *sRSTarget= NULL;
		char *sRSMax= NULL;
		unsigned long ulFloorLimit=0;
		unsigned long ulRSThreshold; 
		int inRSTarget;
		int inRSMax;
		char *s_tac_df = NULL;
		char *sPinBypass = NULL;
		char *sAcct = NULL;


		usEMVGetTLVFromColxn(TAG_9F1A_TERM_COUNTY_CODE, termCtry, &iLen);
		usEMVGetTLVFromColxn(TAG_5F28_ISSUER_COUNTY_CODE, cardCtry, &iLen);
		if(memcmp( termCtry, cardCtry, 2 ) == 0 ) {
        	sfloorlimit = (char*)IRIS_GetObjectTagValue( cfgdata, "D_FLOORLIMIT" );
        	sRSThreshold = (char*)IRIS_GetObjectTagValue( cfgdata, "D_RSTHRESHOLD" );
        	sRSTarget = (char*)IRIS_GetObjectTagValue( cfgdata, "D_RSTARGET" );
        	sRSMax = (char*)IRIS_GetObjectTagValue( cfgdata,"D_RSMAX"  );
			if(( s_tac_df = (char*)IRIS_GetObjectTagValue(cfgdata,"D_TAC_DEFAULT")) != NULL ) {
					char *s_tac_dn = (char*)IRIS_GetObjectTagValue(cfgdata,"D_TAC_DENIAL");
					char *s_tac_on = (char*)IRIS_GetObjectTagValue(cfgdata,"D_TAC_ONLINE");
					if(s_tac_df && s_tac_dn && s_tac_on) {
						unsigned char h_tac_df[30],h_tac_on[30],h_tac_dn[30];
						int iret = 0;
						UtilStringToHex(s_tac_df,strlen(s_tac_df), h_tac_df);
						UtilStringToHex(s_tac_dn,strlen(s_tac_dn), h_tac_dn);
						UtilStringToHex(s_tac_on,strlen(s_tac_on), h_tac_on);
						iret = inVXEMVAPSetTACs( h_tac_dn,h_tac_on,h_tac_df);
						if(iret==0) {
							strcpy(gEmv.tac_default,s_tac_df);
							strcpy(gEmv.tac_denial,s_tac_dn);
							strcpy(gEmv.tac_online,s_tac_on);
						}
					}
    				UtilStrDup(&s_tac_df , NULL);
    				UtilStrDup(&s_tac_dn , NULL);
    				UtilStrDup(&s_tac_on , NULL);
			}
			sPinBypass= (char*)IRIS_GetObjectTagValue(cfgdata,"D_PINBYPASS");
			if(sPinBypass && strcmp(sPinBypass,"YES")==0) gEmv.pinbypass_enable = true;
		} else {
        	sfloorlimit = (char*)IRIS_GetObjectTagValue( cfgdata, "I_FLOORLIMIT" );
        	sRSThreshold = (char*)IRIS_GetObjectTagValue( cfgdata, "I_RSTHRESHOLD" );
        	sRSTarget = (char*)IRIS_GetObjectTagValue( cfgdata, "I_RSTARGET" );
        	sRSMax = (char*)IRIS_GetObjectTagValue( cfgdata,"I_RSMAX"  );
			if(( s_tac_df = (char*)IRIS_GetObjectTagValue(cfgdata,"I_TAC_DEFAULT")) != NULL ) {
					char *s_tac_dn = (char*)IRIS_GetObjectTagValue(cfgdata,"I_TAC_DENIAL");
					char *s_tac_on = (char*)IRIS_GetObjectTagValue(cfgdata,"I_TAC_ONLINE");
					if(s_tac_df && s_tac_dn && s_tac_on) {
						unsigned char h_tac_df[30],h_tac_on[30],h_tac_dn[30];
						int iret= 0;
						UtilStringToHex(s_tac_df,strlen(s_tac_df), h_tac_df);
						UtilStringToHex(s_tac_dn,strlen(s_tac_dn), h_tac_dn);
						UtilStringToHex(s_tac_on,strlen(s_tac_on), h_tac_on);
						iret = inVXEMVAPSetTACs( h_tac_dn,h_tac_on,h_tac_df);
						if(iret==0) {
							strcpy(gEmv.tac_default,s_tac_df);
							strcpy(gEmv.tac_denial,s_tac_dn);
							strcpy(gEmv.tac_online,s_tac_on);
						}
					}
    				UtilStrDup(&s_tac_df , NULL);
    				UtilStrDup(&s_tac_dn , NULL);
    				UtilStrDup(&s_tac_on , NULL);
			}
			sPinBypass= (char*)IRIS_GetObjectTagValue(cfgdata,"I_PINBYPASS");
			if(sPinBypass && strcmp(sPinBypass,"YES")==0) gEmv.pinbypass_enable = true;
		}
		if(sfloorlimit) {
			if(sfloorlimit && sRSThreshold && sRSTarget && sRSMax) {
				ulFloorLimit = atol( sfloorlimit);
				ulRSThreshold = atol( sRSThreshold);
	   			inRSTarget = atoi( sRSTarget);
				inRSMax = atoi(sRSMax);
				if (ulFloorLimit > 0) {
					iStatus = inVXEMVAPSetROSParams(ulFloorLimit,ulRSThreshold,inRSTarget,inRSMax);
				}
			}
		}

		if(( sAcct = (char*)IRIS_GetObjectTagValue(cfgdata,"CR")) != NULL) {
			if(strcmp(sAcct,"NO") == 0) gEmv.cracct_disable = true;
		}


    	UtilStrDup(&sfloorlimit , NULL);
    	UtilStrDup(&sRSThreshold , NULL);
    	UtilStrDup(&sRSTarget , NULL);
    	UtilStrDup(&sRSMax , NULL);
    	UtilStrDup(&sPinBypass , NULL);
    	UtilStrDup(&sAcct , NULL);
    	UtilStrDup(&cfgdata , NULL);
	}

	iStatus = inVXEMVAPProcRisk(NULL);
	if (iStatus != SUCCESS)
	{
        return(iStatus);
	}

	//usEMVGetTxnStatus(&TVRTSI);
	//if( TVRTSI.stTVR[1] & 0x40 ) return(E_APP_EXPIRED);	

	return(SUCCESS);
}

/*
** 	int	EmvCardholderVerify(void)
**
**	EMV cardholder verification
**
**	return:
** 	#SUCCESS			Operation completed successfully.
**	#ERR_EMV_CARD_REMOVED	Card was removed from the ICC reader.
**	#ERR_EMV_CBFN_ERROR		Callback function returned an error.
**	#ERR_EMV_CLIENT_ABORTED	Operation was cancelled by the customer/terminal-operator
**	Other					Error returned during reading or authentication of the card data.
*/
int	EmvCardholderVerify(void)
{
	int	iStatus;
	
	LOG_PRINTFF (0x00000001L, " EmvCardholderVerify");

	if(!EmvIsCardPresent()) 
	{
		return(ERR_EMV_CARD_REMOVED);
	}

	/*
	** EMV Kernel Cardholder Verification
	*/
	iStatus = inVXEMVAPVerifyCardholder(NULL);
	if(!EmvIsCardPresent()) 
	{
		return(ERR_EMV_CARD_REMOVED);
	}
	else if (iStatus != 0)
	{
	//	LOG_PRINTFF (0x00000001L, " EmvCardholderVerify 1.4");
        return(iStatus);
	}

	return(SUCCESS);
}

/*
** 	int	EmvProcess1stAC(void)
**
**	Perform all remaining tasks required to complete EMV
**	transaction processing. This involves:
**		- Generate 1st AC
**		- Perform transaction online if required
**		- Generate 2nd AC
**		- Process issuer scripts received via the transaction host
**		- Issuer authentication
**
**	return:
** 	#SUCCESS					Operation completed successfully.
**	#ERR_EMV_CARD_REMOVED			Card was removed from the ICC reader.
**	#ERR_EMV_APPLICATION_BLOCKED	Card application is blocked
**	#ERR_EMV_DATA_NOT_AVAILIBLE		Unable to obtain data required to complete the operation.
**	#ERR_EMV_NO_RETRY_FAILURE		Operation failed due to error, but
**											fallback processing may be used by
**											the calling application to complete
**											the transaction.
**	#ERR_EMV_FAILURE				Operation failed due to error.
*/
int	EmvProcess1stAC(void)
{
	uchar	sAuthRespCode[2];
	unsigned short	iTagDataLen;
	int	iStatus;
	uchar cryptInfoData;

	/*
	** Sanity check
	*/
	if(!EmvIsCardPresent()) return(ERR_EMV_CARD_REMOVED);
	
	/*
	** Generate 1st Application Cryptogram (AC)
	*/
	iStatus = inVXEMVAPFirstGenerateAC(gEmv.termDesicion);
	if (iStatus != SUCCESS)
	{	
		switch(iStatus)
		{
			case APPL_BLOCKED:
			case CARD_REMOVED:
			case CARD_BLOCKED:
				return(iStatus);
			case E_CDA_ARQC_FAILED:
				/*
				** CDA failed on 1st AC generation.
				** Generate a 2nd AC to decline the transaction without connecting
				** to the host, & then continue on with the remaining transaction
				** processing.
				*/

				inVXEMVAPSecondGenerateAC(DECLINE_CDA_ARQC_FAILED);
				break;

			case TRANS_APPROVED:
			case TRANS_DECLINED:
			case OFFLINE_APPROVED:
			case OFFLINE_DECLINED:
			case OFFLINE_NOTALLOWED:
				/*
				** Transaction was selected for offline processing & offline
				** processing has either been completed or is not allowed.
				*/
				return(iStatus);
			default:
				gEmv.techfallback = true;
				return(iStatus);
		}
	}

	/*
	** Examine the AC data returned by the EMV card to determine how to proceed from here.
	** Check validity of the cryptogram returned by the card first.
	*/
	iStatus = usEMVGetTLVFromColxn(TAG_9F27_CRYPT_INFO_DATA, &cryptInfoData, &iTagDataLen);
	if(iStatus != EMV_SUCCESS) return(iStatus);
	usEMVGetTLVFromColxn(TAG_8A_AUTH_RESP_CODE, sAuthRespCode, &iTagDataLen);

    if(memcmp(sAuthRespCode,AAC_GEN1AC_DECLINE, 2)==0)
	 {
			return(OFFLINE_DECLINED);
	 }

	switch(cryptInfoData & 0xC0)
	{
		case GEN_AAC:
			if((cryptInfoData & 0x07) == 0x01)	return(OFFLINE_NOTALLOWED);
			else return (OFFLINE_DECLINED);
		case GEN_TC:
			if((cryptInfoData & 0x07) == 0x01)	return(OFFLINE_NOTALLOWED);
			else return (OFFLINE_APPROVED);
		case GEN_AAR:
			return(BAD_DATA_FORMAT);
		case GEN_ARQC:
			if((cryptInfoData & 0x07) == 0x01)	return(OFFLINE_NOTALLOWED);
			else return (ONLINE_REQST);
	}

	return(SUCCESS);
}

/*
**
*/

int EmvUseHostData(int hostDecision,const char *hexdata,short len)
{   
    uchar buffer[1024];
    uchar buffer1[1024];
    uchar buffer2[1024];
    ushort len1=0;
    ushort len2=0;
    ushort taglen = 0;
	int numScripts = 0;
	uchar issuerScriptResults[1024];
	short iret=0;
	uchar cid ;
	
    if (len && findTag(0x91, buffer, (short *)&taglen, (const byte *)hexdata,len))   {           // Issuer auth data
    	usEMVUpdateTLVInCollxn(0x9100, buffer, taglen);
	}

    memset(buffer,0,sizeof(buffer));
    memset(buffer1,0,sizeof(buffer1));
    memset(buffer2,0,sizeof(buffer2));

    //verifone fix
    //Check for failed to connect, check if Offline Data Authentication not
    // performed bit is set in TVR, check if CDA failed bit is not set in TVR, check if the terminal and the card support CDA
    if (hostDecision == FAILED_TO_CONNECT) {
    	srTxnResult  srTVRTSI;
    	char ucAIP[256] = "";
    	char ucTermCap[256] = "";
    	unsigned short usAIPLen=0;
    	unsigned short usTermCapLen = 0;

    	usEMVGetTxnStatus(&srTVRTSI); //Get TVR & TSI
    	usEMVGetTLVFromColxn(TAG_8200_APPL_INTCHG_PROFILE, &ucAIP[0], &usAIPLen); // Get AIP
    	usEMVGetTLVFromColxn(TAG_9F33_TERM_CAP, &ucTermCap[0], &usTermCapLen); //Get Terminal Capabilities
    	if( (	(srTVRTSI.stTVR[0] & 0x80) == 0x80)
    		&& ((srTVRTSI.stTVR[0] & 0x04) == 0x00)
    		&& (((ucTermCap[2] & 0x08) == 0x08)
    		&& ((ucAIP[0] & 0x01) == 0x01)))
    	{
    		srTVRTSI.stTVR[0] &= 0x7F;
        	usEMVSetTxnStatus(&srTVRTSI);
    	}
    	//Reset the offline data authentication not performed bit
    }

    //end

    if(len) numScripts = createScriptFiles((byte *)hexdata,len,buffer1,&len1,buffer2,&len2);

    iret = inVXEMVAPUseHostData(hostDecision, issuerScriptResults, &numScripts,buffer1,len1,buffer2,len2);
	//DebugDisp("inVXEMVAPUseHostData,hostDecision:%d,ret = %d",hostDecision,iret );

	if(iret) return(iret);
	
	usEMVGetTLVFromColxn(0x9f27, &cid, &taglen);
    if ((hostDecision == HOST_AUTHORISED) && ((cid & 0xC0) == 0x40))  {    // Host authorised but card declined 
		return(SUCCESS);
	}
	else if( hostDecision == FAILED_TO_CONNECT && ((cid &0xC0) == 0x40)) {
		return(SUCCESS);
	}
    else {
		return(TRANS_DECLINED) ;
	}
}

/*
** 	int	EmvCardPowerOff(void);
**
**	Perform all tasks required in order to (gracefully) complete interaction
**	with the EMV card. This involves:
**		- Powering down the card
**
**	return:
** 	#SUCCESS			Operation completed successfully. Card may now be removed from the terminal ICC reader.
**	#ERR_EMV_CARD_REMOVED	Card was removed before the operation could be completed.
**	#ERR_EMV_FAILURE		Operation failed.
*/
int	EmvCardPowerOff(void)
{
	
	memset( &gEmv,0,sizeof(gEmv));
	emvcfg_list.current = 0;

	/*
	** Check that the card is still in the ICC reader
	*/
	if(!EmvIsCardPresent()) return(ERR_EMV_CARD_REMOVED);
	
	/*
	** Power-down the card
	*/
	if (Terminate_CardSlot(CUSTOMER_CARD, SWITCH_OFF_CARD) == CARDSLOT_SUCCESS)
	{
		return(SUCCESS);
	}
	else
	{
		return(ERR_EMV_FAILURE);
	}
}

/*
**	INTERNAL
** 	int	EmvIsCardPresent(void)
** 	Check if an ICC card is currently in the ICC reader.
**	return
*/
bool EmvIsCardPresent(void)
{
	int		status;
	status = inVXEMVAPCardPresent();
	if(status==0) return(true);

	if (status == SLOT_INITIALIZE_ERROR) {
		//inVXEMVAPInitCardslot();
		Init_CardSlot(CUSTOMER_CARD);
		SVC_WAIT(10);
		status = inVXEMVAPCardPresent(); 
	}
	if(status==0) return(true);
	else return(false);	



	//status = Get_Card_State (CUSTOMER_CARD)	;
	//if(status == CARD_PRESENT) return (true);
	//else return(false);
}

EMVBoolean bIsCardBlackListed(byte * pan, unsigned short panLen, byte * panSeqNo, unsigned short panSeqLen)
{
	return(EMV_FALSE);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : EmvGetAmtCallback
//
// DESCRIPTION:	This function is called from within the thin client (actually called from the EMV module
//				when it requests a callback to be provided) in order to execute a callback
//				to the payment application - we need to transaction amount to pass it on to
//				the EMV kernel
//
// PARAMETERS:	none
//
// RETURNS:		the amount as passed from the payment application
//-------------------------------------------------------------------------------------------
//
int EmvGetAmtCallback(void)
{
	//callback to the payment application to provide the amount
	//executeCallbackObjects("SGB_EMV_GET_AMT");
	ulong	emvAmount;
	emvAmount = 0;
	return emvAmount;
}

int EmvSetAmt(long emvAmount,long emvCash)
{

	if( emvAmount >= 0 )  {
		char tmp[13];
		char ucharBuf[7];

		usEMVUpdateTLVInCollxn(TAG_81_AMOUNT_AUTH, (byte *) &emvAmount, 4);

		usEMVAddAmtToCollxn( emvAmount); //9F02

		if ( emvCash >= 0 ) 
		{
			usEMVUpdateTLVInCollxn(TAG_9F04_AMT_OTHER_BIN, (byte *) &emvCash, 4);
			memset(tmp,0x00,sizeof(tmp));
			sprintf(tmp, "%012lu", emvCash);
			ascii_to_binary(ucharBuf, (const char *) tmp, 12);
			usEMVUpdateTLVInCollxn(TAG_9F03_AMT_OTHER_NUM, ucharBuf, 6);
		}
	}
	return 0;
}

int EmvSetAccount(unsigned char emvAcc)
{
	int iret = usEMVUpdateTLVInCollxn (TAG_5F57_ACCOUNT_TYPE, (unsigned char *)&emvAcc, 1);
	return(0);
}


long Hex_To_Dec(char *s )
{
   int i,a[20];
   unsigned long int h=0,m=1;
  // char s[20]={"000001F4"};

   // clrscr();
   for(i=0;s[i]!='\0';i++)
   {
    switch(s[i])
     {
      case '0':
       a[i]=0;
       break;
      case '1':
       a[i]=1;
       break;
      case '2':
       a[i]=2;
       break;
      case '3':
       a[i]=3;
       break;
      case '4':
       a[i]=4;
       break;
      case '5':
       a[i]=5;
       break;
      case '6':
       a[i]=6;
       break;
      case '7':
       a[i]=7;
       break;
      case '8':
       a[i]=8;
       break;
      case '9':
       a[i]=9;
       break;
      case 'a':
      case 'A':
       a[i]=10;
       break;
      case 'b':
      case 'B':
       a[i]=11;
       break;
      case 'c':
      case 'C':
       a[i]=12;
       break;
      case 'd':
      case 'D':
       a[i]=13;
       break;
      case 'e':
      case 'E':
       a[i]=14;
       break;
      case 'f':
      case 'F':
       a[i]=15;
       break;
      default:
       break;
     }
   }
   i--;
   for(;i>=0;i--)
   {
    h=h+a[i]*m;
    m=m*16;
   }
   
  // sprintf(buff,"%ld ",h);
  // printf(buff)  ;
   //Ing_Tracer(buff);
   return h ;
   //strcpy ( d , buff ) ;
}

#endif
