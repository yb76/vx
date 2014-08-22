/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       emvCallback.c
**
** DATE CREATED:    13 September 2010
**
** AUTHOR:			Katerina Phibbs    
**
** DESCRIPTION:     This module handles all EMV callback functionality
**-----------------------------------------------------------------------------
*/

#ifdef __EMV

/*
**-----------------------------------------------------------------------------
** Include Files
**-----------------------------------------------------------------------------
*/

/*
** Standard include files.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "inputmacro.h"
#include "alloc.h"
#include "emv.h"
#include "irisfunc.h"
#include "Input.h"

#include "include\printer.h"

#define	EMVCB_BLACK_CARDS_FILE	"BlackListedCards.txt"


/* Variables */

/*
** Storage for a  PIN entered by a user
*/
extern char		acEmvCbFnOfflinePin[EMV_MAX_PIN_LENGTH + 1];
extern unsigned short emvChipApp;

/*
** 	int		EmvSelectAppMenu(char **pcMenuItems, int iMenuItemsTotal)
**
** 	
**	This function provides an implementation of the Verix-V EMV Library
**	inMenuFunc() callback function using our "pMenuFunc" callback function.
**
**
**	This function is used to display a list of available options
**	on the screen in the form of a menu, prompt the user to enter a
**	selection from the menu and return the user's selection.
**
**	pcMenuItems		Buffer containing a list of menu-items to be
**					displayed. Each menu-item is formatted as a
**					NULL-terminated string.
**
**	iMenuItemsTotal	Total number of menu-items supplied.
**
** 	retval	>= 1			Operation completed successfully. The
**							value returned is the index of the
**							menu item selected by the user (where
**							1 corresponds to the first item shown
**							on the menu).
**	retval TRANS_CANCELLED	Transaction has been cancelled by the
**							user
*/
int	EmvCallbackFnSelectAppMenu(char **pcMenuItems, int iMenuItemsTotal)
{
	int selected = 0;
	int i = 0;
	int last_item = 0;
  	char jsontag[30] = "";
  	char *jsonvalue=NULL;
  	unsigned int objlength;
  	static char *data = NULL;
	char cardname_upper[60]="";
	int itemtotal = iMenuItemsTotal;
	char namearr[20][30];

	if(data == NULL) data =(char*)IRIS_GetObjectData( "CARD_NAME", &objlength);

	for(i=0;i<iMenuItemsTotal;i++) {
	  jsonvalue = NULL;
	  memset(cardname_upper,0,sizeof(cardname_upper));
	  strnupr (cardname_upper, pcMenuItems[i], strlen(pcMenuItems[i]));
      if(data) jsonvalue = (char*)IRIS_GetObjectTagValue( (const char *)data, cardname_upper );
      if(jsonvalue && strncmp(jsonvalue,"DISABLE",7)==0) {
		 strcpy( pcMenuItems[i],"");
		 itemtotal -= 1;
	  } else {
		last_item = i+1;
	  }
    }

    UtilStrDup(&jsonvalue , NULL);

	//if(itemtotal == 1) return(last_item);
	if(itemtotal != iMenuItemsTotal ) {
		int j = 0;
		memset(namearr,0,sizeof(namearr));
		for(i=0;i<iMenuItemsTotal;i++) {
			if(strlen(pcMenuItems[i])>0) {
				strcpy( namearr[j] , pcMenuItems[i]);
				j++;
			}
		}
		selected = DispArray(30000, (char **)namearr,j);
	} else {
		int iTo = 30000;
		if(itemtotal == 1) iTo = 5000;
		//gEmv.appsTotal = iMenuItemsTotal;
		selected = DispArray(iTo, pcMenuItems,iMenuItemsTotal);
	}

	if(selected > 0) return(selected);
	else if(itemtotal == 1 && (selected == -1 * EVT_TIMEOUT)) return(1);

	return(0);
}

/*!
** 	void	EmvFnPromptManager(unsigned short usPromptId)
**
**	This function provides an implementation of the Verix-V EMV Library
**	vdPromptManager() callback function using our
**	"pPromptFunc" callback function.
**
**	This function conforms to the specification of the Verifone
**	vdPromptManager() callback function
**
**	This function displays a specified prompt obtained from a prompt
**	table on the terminal screen.
**
**	usPromptId	Prompt index indicating which prompt is to be
**				displayed
**
*/
void	EmvCallbackFnPromptManager(unsigned short usPromptId)
{
	char stmp[100];
	char stmp2[100];
	char *szDspMsg="";
	char outevent[200];
	unsigned long keys =KEY_CNCL_BIT|KEY_OK_BIT|KEY_CLR_BIT;
	
	
	/*
	** Map Verifone Prompt-ID to our Prompt-ID
	*/
      
    switch (usPromptId)
    {
		case E_CAPK_FILE_NOT_FOUND:    
            //szDspMsg = "CAPK FILE Missing";
			break;
    	case E_INVALID_CAPK:		
    	case E_CAPK_FILE_EXPIRED:		
		case E_NO_CAPK_DATA:		
		//Return without displaying any messages
			break;
        case E_ICC_BLOCKED:
            szDspMsg =  "CARD BLOCKED";
            break;
        case E_PIN_REQD:
			gEmv.pinentry = false;
            szDspMsg =  "PIN Required";
            break;
        case E_LAST_PIN_TRY:
			gEmv.pinentry = false;
            szDspMsg = "Last PIN Try";
            break;
        case E_PIN_TRY_LT_EXCEED:
			gEmv.pinentry = false;
            szDspMsg = "PIN Try Exceeded";
            break;        	
		case E_USR_ABORT:
		case E_USR_PIN_CANCELLED:
			gEmv.pinentry = false;
 			szDspMsg = "PIN Cancelled" ;
            break;
        case E_USR_PIN_BYPASSED:
            //szDspMsg = "PIN Bypassed" ;
            break;        
        case E_PIN_BLOCKED:
			gEmv.pinentry = false;
            szDspMsg =  "PIN Blocked";
            break;
        case EMV_PIN_SESSION_IN_PROGRESS:
             //szDspMsg = "PIN Progress" ;
             break;
        case EMV_PIN_SESSION_COMPLETE:
            //szDspMsg = "PIN Complete" ;
            break;
		case E_APP_EXPIRED:
			/*  szDspMsg = "CARD ERROR";*/
			break;
		case E_INVALID_PIN:
			gEmv.pinentry = false;
            szDspMsg =  "Incorrect PIN";
            break;
		///****************************
        case TRANS_APPROVED:
            szDspMsg  = "TRANS APPROVED" ;
            break;
        case TRANS_DECLINED:
            szDspMsg = "TRANS DECLINED" ;
            break;
        case OFFLINE_APPROVED:
            szDspMsg = "OFFLINE APPROVED" ;
            break;
        case OFFLINE_DECLINED:
            szDspMsg = "OFFLINE DECLINED" ;
            break;
        case OFFLINE_NOTALLOWED:
            szDspMsg = "OFFLINE NOT ALLOWED" ;
            break;
        case FAILURE:
		case EMV_FAILURE:
        case EMV_CHIP_ERROR:
        case CHIP_ERROR:
            szDspMsg =  "CHIP ERROR";
            break;
        case CARD_REMOVED:
            szDspMsg = "CARD REMOVED" ;
            break;
        case CARD_BLOCKED:
            szDspMsg = "CARD BLOCKED BY ISS" ;
            break;
        case APPL_BLOCKED:
            szDspMsg = "APPLICATION BLOCKED" ;
            break;
        case CANDIDATELIST_EMPTY:
            szDspMsg = "CANDIDATELIST EMPTY" ;
            break;
        case BAD_DATA_FORMAT:
            szDspMsg = "BAD DATA FORMAT" ;
            break;
        case APPL_NOT_AVAILABLE:
            szDspMsg = "APP NOT AVAIL" ;
            break;
        case TRANS_CANCELLED:
            szDspMsg = "TRANS CANCELLED" ;
            break;
        case EASY_ENTRY_APPL:
            szDspMsg = "EASY ENTRY APPL" ;
            break;
        case ICC_DATA_MISSING:
            szDspMsg = "ICC DATA MISSING" ;
            break;
        case CONFIG_FILE_NOT_FOUND:
            szDspMsg = "CONFIG FILE NOT FOUND" ;
            break;
        case FAILED_TO_CONNECT:
            szDspMsg = "FAILED TO CONNECT";
            break;
        case SELECT_FAILED:
            szDspMsg = "SELECT FAILED" ;
            break;
        case USE_CHIP:
            szDspMsg = "USE CHIP READER" ;
            break;
        case REMOVE_CARD:
            szDspMsg = "REMOVE CARD" ;
            break;
        case BAD_ICC_RESPONSE:
            szDspMsg = "BAD ICC RESPONSE" ;
            break;
		case USE_MAG_CARD:
            szDspMsg = "SWIPE CARD" ;
            break;
		case E_TAG_NOTFOUND:
            szDspMsg = "TAG NOT FOUND";
            break;
			
		case INVALID_CAPK_FILE:
            szDspMsg = "INVALID KEY FILE";
			break;
	
        case INVALID_ATR:
            szDspMsg = "INVALID ATR" ;
			break ;
		case 0x6985: 
			szDspMsg = "OFFLINE NOT ALLOWED" ;
			break;
		/*
		case E_INVALID_MODULE: 
			szDspMsg = MSG_INVALID_MODULE ;
			break;*/
		
        default:
            szDspMsg = "Chip Error" ;
            break;
   	}

	if(strlen( szDspMsg) == 0 ) return;

	sprintf(stmp,"WIDELBL,THIS,%s,2,C;",szDspMsg);
	DisplayObject(stmp,keys,EVT_TIMEOUT,2000,outevent,stmp2);
	return;
}


/*
** 	unsigned short	EmvFnAmtEntry(unsigned long *pulTxnAmt)
**
**	This function provides an implementation of the Verix-V EMV Library
**	usAmtEntryFunc() callback function using our
**	"pGetAmountFunc" callback function.
**
**
**	This function is used to prompt the user to enter an amount,
**	and collect the amount entered by the user.
**
**	\note
**	Despite what the documentation for Verifone Verix-V EMV Module
**	states, this callback function does \b not appear to be called
**	by the Verifone Verix-V inVXEMVAPSelectApplication() function,
**	and must be called explicitly by the calling application.
**
**	pulTxnAmt	Buffer (unsigned long*) supplied for the storage
**				of the amount entered by the user.
**
** 	return:	>= 1			Operation completed successfully. 
**	retrun:	EMV_SUCCESS		Amount was successfully entered by the
**							user
**	return: E_INVALID_PARAM	Buffer supplied for the storage of the
**							entered amount is NULL or otherwise
**							invalid.
**	retrun: TRANS_CANCELLED	Amount entry was cancelled by the user.
*/
unsigned short	EmvFnAmtEntry(unsigned long *pulTxnAmt)
{
	char AccountType = 0x00;
	int cashamt= 0;
	char tmp[13];

	EmvSetAmt(gEmv.amt,cashamt);

	if(strlen(gEmv.acct)) {
		if(strcmp(gEmv.acct,"SAVINGS")==0) {
			AccountType = 0x10;
		}
		else if(strcmp(gEmv.acct,"CHEQUE")==0) {
			AccountType = 0x20;
		}
		else if(strcmp(gEmv.acct,"CREDIT")==0) {
			AccountType = 0x30;
		}
		usEMVAddTLVToCollxn(TAG_5F57_ACCOUNT_TYPE, (byte *)&AccountType, 1);
	}

	*pulTxnAmt = 0;
	return(EMV_SUCCESS);
}


/*
** unsigned short	EmvGetUsrPin(unsigned char *ucPin);
**
**	This function provides an implementation of the Verix-V EMV Library
**	getUsrPin() callback function (VerixV EMV function-ID: GET_USER_PIN).
**
**	This function conforms to the specification of the Verifone getUsrPin()
**	callback function, as defined in the following document:
**
**		Author: 	Verifone Inc
**		Title:		Verix/Verix-V EMV Module
**					Reference Manual
**
**	This function performs offline PIN-entry. The function prompts the
**	user to enter a PIN & returns it in the supplied buffer.
**
**	ucPin	Buffer (unsigned char*) provided for the storage of the
**			entered PIN.
**
**	return:
**  > 0					Operation completed successfully. Value returned is the length of the PIN (characters)
**	E_USR_PIN_BYPASSED	PIN-entry has been bypassed
**	E_USR_PIN_CANCELLED	PIN-entry has been cancelled by the customer.
**	EMV_FAILURE			Operation aborted due to some other error.
*/
unsigned short	EmvGetUsrPin(unsigned char *ucPin)
{
	char scrlines[1024] ;
	char outevent[100];
	char outpin[15]="";//ID 0000039
	char sAmt[30];
	
	sprintf(sAmt, "$%.2f", gEmv.amt/100.0);
	sprintf( scrlines, "WIDELBL,THIS,TOTAL:           %9s,2,3;", sAmt);
	strcat( scrlines,"LARGE,THIS,ENTER PIN OR OK,5,C;");
	strcat( scrlines,"LHIDDEN,,0,7,C,12;");
	strcpy( (char *)ucPin,"");

	GetSetEmvPin(1,"");
	while(1) {

		DisplayObject(scrlines,KEY_OK_BIT+KEY_CNCL_BIT,EVT_SCT_OUT+EVT_TIMEOUT,30000,outevent,outpin);

		if(strcmp(outevent,"KEY_OK")==0) {
			if( strlen(outpin) == 0) {
				if( !gEmv.pinbypass_enable ) {
					char * nopinbypass = "WIDELBL,THIS,PIN BYPASS,2,C;WIDELBL,THIS,NOT ALLOWED,3,C;";
					DisplayObject(nopinbypass,KEY_OK_BIT+KEY_CNCL_BIT,EVT_TIMEOUT,2000,outevent,outpin);
					continue;
				} else {
					return(E_USR_PIN_BYPASSED);
				}
			}
			else if(( strlen(outpin) >= 4) && ( strlen(outpin) <= 12)){
				strcpy( (char *)ucPin, outpin );
				GetSetEmvPin(1,(unsigned char *)outpin);
				return(strlen((const char *)ucPin));
			}
			else {
				char event[64]="";
				char input[256]="";
				char srcline1[1024]="WIDELBL,THIS,INVALID PIN,2,C;WIDELBL,THIS,PLEASE RETRY,4,C;";
				DisplayObject(srcline1,0,EVT_TIMEOUT,2000,event,input);
				continue;
			}
			//END ..Dwarika .. to fix bug id 3346 from bash report by CBA
		} else {
			return(E_USR_ABORT);
		}
	}
}

void EmvDispUsrPin()
{
	char scrlines[1024] ="";
	char outevent[100];
	char outpin[15]="";
	char sAmt[30];
	
	sprintf(sAmt, "$%.2f", gEmv.amt/100.0);
	sprintf( scrlines, "WIDELBL,THIS,TOTAL:           %9s,2,3;", sAmt);
	strcat( scrlines,gEmv.pinbypass_enable?"LARGE,THIS,ENTER PIN OR OK,5,C;":"LARGE,THIS,ENTER PIN AND OK,5,C;");
	
	DisplayObject(scrlines,0,0,0,outevent,outpin);

}

int GetSetEmvPin( unsigned short action,unsigned char *pin)
{
	if(action==0) {
		strcpy( (char *)pin,(const char *)gEmv.pin);	
	} else if(action==1){
		strcpy( (char *)gEmv.pin,(const char *)pin);
	}
	return(0);
}

/*
** 	unsigned short	EmvSignature(void)
**
** 	This function provides an implementation of the Verix-V EMV Library
**	usEMVPerformSignature() callback function (VerixV EMV function-ID:
**	PERFORM_SIGNATURE) using RIS's callback function.
**
**	This function conforms to the specification of the Verifone
**	usEMVPerformSignature() callback function, as defined in the
**	following document:
**
**		Author: 	Verifone Inc
**		Title:		Verix/Verix-V EMV Module
**					Reference Manual
**
**	This function is used to prompt the terminal operator to obtain
**	the customer's signature and verify it.
**
**	return:
** 	#EMV_SUCCESS		Operation completed successfully. Customer's signature has been verified.
**	#TRANS_CANCELLED	Transaction has been cancelled by the customer/terminal-operator.
**	#EMV_FAILURE		Customer's signature is invalid
**
** Note: in our case we always confirm the signature as OK. If signature is mismatched, we handle it through the payment
** application by generating reversal.
*/
unsigned short	EmvSignature(void)
{
	gEmv.sign = true;
	return(EMV_SUCCESS);
}

unsigned short EmvFnOfflinePin(unsigned char *pin)
{
	int iret = 0;
	char usrpin[14] = "";//ID 0000039

	gEmv.onlinepin = false;
	gEmv.offlinepin = false;
	gEmv.pinentry = true;

	vSetPinParams(gEmv.pinbypass_enable);//SECURE_PIN_MODULE changes
	inVXEMVAPSetDisplayPINPrompt(&EmvDispUsrPin);

	return(E_PERFORM_SECURE_PIN);

}

unsigned short EmvFnOnlinePin(void)
{
	int iret = 0;
	char usrpin[14] = "";	//ID 0000039
	char outevent[100];
	char outpin[15]="";
	
	gEmv.onlinepin = false;
	gEmv.offlinepin = false;
	gEmv.pinentry = false;

	vSetPinParams(gEmv.pinbypass_enable);//SECURE_PIN_MODULE changes
	inVXEMVAPSetDisplayPINPrompt(NULL);

	{
	char scrlines[1024] ;
	char sAmt[30];
	
	sprintf(sAmt, "$%.2f", gEmv.amt/100.0);
	sprintf( scrlines, "WIDELBL,THIS,TOTAL:           %9s,2,3;", sAmt);
	strcat( scrlines,gEmv.pinbypass_enable?"PIN,,,P5,P11,0;":"PIN,,,P5,P11,1;");
	DisplayObject(scrlines,KEY_OK_BIT+KEY_CNCL_BIT+( gEmv.pinbypass_enable?KEY_NO_PIN_BIT:0),EVT_SCT_OUT+EVT_TIMEOUT,30000,outevent,outpin);
	}

	if (strcmp(outevent ,"KEY_NO_PIN")==0) {
		return(E_USR_PIN_BYPASSED);
	} else if (strcmp(outevent ,"KEY_OK")==0) { 
		gEmv.pinentry = true;
		gEmv.onlinepin = true;
		return(EMV_SUCCESS);
	} else {
		return(E_USR_ABORT);
	}
}

/*
** 	int	EmvCallbackFnInit(void);
**
**	Setup/register EMV callback functions provided by the calling application.
**
**
**	return:
** 	#SUCCESS			Operation completed successfully.
**	#ERR_EMV_CONFIG_ERROR	EMV kernel configuration operation failed.
*/
int	EmvCallbackFnInit(void)
{

	inVxEMVAPSetFunctionality(GET_LAST_TXN_AMT, (void*)&EmvFnLastAmtEntry);
	inVxEMVAPSetFunctionality(GET_USER_PIN, (void*)&EmvFnOfflinePin);
	inVxEMVAPSetFunctionality(DISPLAY_ERROR_PROMPT, (void*)&EmvCallbackFnPromptManager);
	inVxEMVAPSetFunctionality(PERFORM_SIGNATURE, (void*)&EmvSignature);
	inVxEMVAPSetFunctionality(PERFORM_ONLINE_PIN, (void*)&EmvFnOnlinePin);

	inVxEMVAPSetFunctionality(PERFORM_ISS_ACQ_CVM, NULL);
	inVxEMVAPSetFunctionality(IS_PARTIAL_SELECT_ALLOWED, NULL);
	inVxEMVAPSetFunctionality(GET_CAPK_DATA, NULL);
	inVxEMVAPSetFunctionality(GET_TERMINAL_PARAMETER, NULL);
	return(SUCCESS);
}

/*
** 	EMVBoolean	EmvCallbackIsCardBlackListed(unsigned char *pucPan,
**											   unsigned short usPanLen,
**											   unsigned char *pucPanSeqNo,
**											   unsigned short usPanSeqLen)
**
**	This function provides an implementation of the Verix-V EMV Library
**	bIsCardBlackListed() callback function.
**
**	This function conforms to the specification of the Verifone
**	bIsCardBlackListed() callback function, as defined in the
**	following document:
**
**		Author: 	Verifone Inc
**		Title:		Verix/Verix-V EMV Module
**					Reference Manual
**
**
**	This function is used to determine if a card (as defined by the supplied
**	PAN and PAN-Sequence Number) is in the list of blacklisted cards.
**
**	pucPan			Buffer (unsigned char*) containing the PAN of the
*					card in question
**
**	usPanLen		PAN length (digits)
**
**	pucPanSeqNo		Buffer (unsigned char*) containing the PAN-Sequence
**					Number of the card in question.
**
**	usPanSeqLen		Length of the PAN Sequence Number (digits)
**
** 	EMV_TRUE	Supplied card details DO appear in the list
**						of blacklisted cards
**	EMV_FALSE	Supplied card details DO NOT appear in the list
**						of blacklisted cards
*/
EMVBoolean	EmvCallbackIsCardBlackListed(unsigned char *pucPan,
									   unsigned short usPanLen,
									   unsigned char *pucPanSeqNo,
									   unsigned short usPanSeqLen)
{
	EMVBoolean	fPanBlackListed;
	int			iFd;
	char		acFileBuf[25];
	char		*pcPan, *pcPanSeqNo;
	char		acPanStr[21], acPanSeqNoStr[3];
	int			iBytesRead, iPanEnd, iEol, iFdAdj;
	int			i;


	/*
	** Sanity checks
	*/
	if ((pucPan == NULL) || (usPanLen <= 0) || ((usPanLen * 2) > sizeof(acPanStr)) ||
		(pucPanSeqNo == NULL) || (usPanSeqLen <= 0) || ((usPanSeqLen * 2) > sizeof(acPanSeqNoStr)))
	{
		return(EMV_FALSE);
	}

	/*
	** Open the Blacklisted-Cards file.
	** Return harmlessly if it doesn't exist or can't be opened
	*/
#ifndef _DEBUG
	iFd = open(EMVCB_BLACK_CARDS_FILE, O_RDONLY);
	if (iFd < 0)
	{
		return(EMV_FALSE);
	}
#endif

	/*
	** Convert supplied PAN & PAN-Seq No into ascii
	*/
	memset(acPanStr, 0, sizeof(acPanStr));
	for (i = 0; i < (int)usPanLen; i++)
	{
		sprintf(&(acPanStr[strlen(acPanStr)]), "%02.2x", pucPan[i]);
	}

	memset(acPanSeqNoStr, 0, sizeof(acPanSeqNoStr));
	for (i = 0; i < (int)usPanSeqLen; i++)
	{
		sprintf(&(acPanSeqNoStr[strlen(acPanSeqNoStr)]), "%02.2x", pucPanSeqNo[i]);
	}

	/*
	** Check supplied PAN & PAN Seq No against those in the Blacklisted-Cards file
	*/
	fPanBlackListed = EMV_FALSE;
	for (;;)
	{
		/*
		** Find the start of the next record in the Blacklisted-Cards file.
		*/
		memset(acFileBuf, 0, sizeof(acFileBuf));
		for (;;)
		{
			iBytesRead = read(iFd, &(acFileBuf[0]), 1);
			if (iBytesRead != 1)
			{
				break;
			}

			if ((acFileBuf[0] >= '0') && (acFileBuf[0] <= '9'))
			{
				break;
			}
		}

		if (iBytesRead != 1)
		{
			break;
		}

		/*
		** Read in next line from the Blacklisted-Cards file.
		** Each line in the Blacklisted-Cards file contains up to 24-characters,
		** formatted as follows:
		**	- PAN (up to 19 characters)
		**	- Comma (ie: "," character)
		**	- PAN Seq No (2 characters)
		**	- Carriage-Return (may or may-not be present, depending on file-format)
		**	- Line-feed
		*/
		iBytesRead = read(iFd, &(acFileBuf[1]), (sizeof(acFileBuf) - 2));
		if (iBytesRead <= 0)
		{
			break;
		}

		/* Take into account the 1st character of the record that was read in separately */
		iBytesRead += 1;

		/*
		** Find the end of the record/line read in.
		** As each line may be terminated by either a CR+LF or just a LF character,
		** the end-of-line is found by searching for the comma in the record &
		** going on from there.
		*/
		iPanEnd = -1;
		for (i = 0; i < iBytesRead; i++)
		{
			if (acFileBuf[i] == ',')
			{
				iPanEnd = i;
				break;
			}
		}

		/* Check that a complete PAN was read in & that the PAN is <= 19-characters long */
		if ((iPanEnd < 0) || (iPanEnd > 19))
		{
			break;
		}

		/* Check that at least 1 complete record was read in */
		iEol = iPanEnd + 2;
		if (iEol >= iBytesRead)
		{
			break;
		}

		/*
		** Skip over the PAN-Seq No & check what character(s) was/were used to
		** terminate the record. If the start of the next record has already been
		** read in, adjust the file-descriptor so that it is pointing to the start
		** of the next record in the file ready for the next read operation.
		*/
		iEol += 1;
		if (iEol < iBytesRead)
		{
			/*
			** Find the start of the next record within the data that
			** we've already read in
			*/
			for (i = iEol; i < iBytesRead; i++)
			{
				if ((acFileBuf[i] != '\r') && (acFileBuf[i] != '\n'))
				{
					break;
				}
			}

			/*
			** Adjust the file-descriptor so that it is pointing to the start
			** of the next record in the file ready for the next read operation.
			*/
			iFdAdj = iBytesRead - i;
			if (iFdAdj > 0)
			{
				if (lseek(iFd, -iFdAdj, SEEK_CUR) < 0)
				{
					break;
				}
			}
		}

		/*
		** Complete record has been read in from the Blacklisted-Cards file.
		** Compare the supplied PAN with the PAN from the current record in
		** the Blacklisted-Cards file.
		*/
		pcPan = &(acFileBuf[0]);
		acFileBuf[iPanEnd] = '\0';

		pcPanSeqNo = &(acFileBuf[iPanEnd + 1]);
		*(pcPanSeqNo + 2) = '\0';

		if ((strcmp(acPanStr, pcPan) == 0) && (strcmp(acPanSeqNoStr, pcPanSeqNo) == 0))
		{
			fPanBlackListed = EMV_TRUE;
			break;
		}
	}

	close(iFd);
	return(fPanBlackListed);
}


unsigned short	EmvFnLastAmtEntry(unsigned long *pulTxnAmt)
{
	unsigned long	dwAmtAuth;

	/*
	** Sanity checking
	*/

	if (pulTxnAmt == NULL)
	{
		return(E_INVALID_PARAM);
	}

	dwAmtAuth = 0;

	*pulTxnAmt = (unsigned long)dwAmtAuth;

	return(EMV_SUCCESS);
}

int vSetPinParams(bool pinbypass) //SECURE_PIN_MODULE changes
{
    short   shRetVal                =   0;
    srPINParams psParams;

    psParams.ucMin = 4;
    psParams.ucMax = 12;
    psParams.ucEchoChar = '*';
    psParams.ucDefChar = '-';
    psParams.ucDspLine = 160;
    psParams.ucDspCol = 100;
   	psParams.ucOption = 0x00;

    psParams.ulFirstKeyTimeOut = 0;
    psParams.ulInterCharTimeOut = 0;
    psParams.ulWaitTime = 0;

    psParams.ucPINBypassKey = ENTER_KEY_PINBYPASS;
    if ( pinbypass ) {
		psParams.ucSubPINBypass = EMV_TRUE;
    	psParams.ucOption = 0x02;
	}
    else psParams.ucSubPINBypass = EMV_FALSE;
    usEMVSetPinParams(&psParams);
}

#endif

