////////////////////////////////////////////////////////////////////////////////
//
// Project    : Contactless Library (libmsr)
//              Provides higher level functions for reading Contactless data.
//
// File Name  : ctlsmod.c
//
// Author(s)  : 
//
// Version History
//
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <svc.h>
//#include <aclconio.h>
//#include <actutil.h>
//#include <logsys.h>

#include <vxemvapdef.h>
#include <emvsizes.h>
#include <est.h>
#include <emvTags.hpp>

#include <ctlsinterface.h>

// for EOS log
#include <eoslog.h>
#include "ctlsmod.h"
#include "Contactless.h"
#include "auris.h"
#include "input.h"

#include "printer.h"


///////////////////////////////////////////////////////////////////////////////

// C++ like...
#define null  NULL

#define dbprintf(...)

///////////////////////////////////////////////////////////////////////////////

//
// ViVOtech defintions, commands and prototypes
//

typedef enum
{
	SET_BAUD = 0, PING, PASSTHRU, SET_LED, SET_POLL, ACTIVATE_TRAN, SET_DISP, UPDATE_BAL, GET_EMV_CONFIG, SET_EMV_CONFIG,
	GET_VER, GET_LCD, STORE_LCD, SET_LCD, GET_ALL_AIDS, SET_CFG_AID, SET_CAPK, DEL_ALL_CAPK, GET_PRODUCT_TYPE,
	CANCEL_TRANSACTION, DEL_CFG_GROUP, SET_CFG_GROUP, DEL_CFG_AID, GET_CFG_AID, GET_CFG_GRP, GET_ALL_GRPS
} CTLS_VIVOTECH_CMD_TYPE;

typedef enum {LITTLE_ENDIAN = 0, BIG_ENDIAN} CRC_TYPE;

typedef struct
{
	char command;
	char subcommand;
	int datalength;
	CRC_TYPE crctype;
} CTLS_VIVOTECH_CMD_VALUES;

CTLS_VIVOTECH_CMD_VALUES ctls_vivotech_cmd[] =
{
	{0x30, 0x01,  1, LITTLE_ENDIAN},		// Set Baud Rate
	{0x18, 0x01,  0, LITTLE_ENDIAN},		// Ping
	{0x2C, 0x01,  1, BIG_ENDIAN},			// Pass Thru
	{0x0A, 0x02,  2, BIG_ENDIAN},			// Set LED
	{0x01, 0x01,  1, LITTLE_ENDIAN},		// Set Polling
	{0x02, 0x01, -1, LITTLE_ENDIAN},		// Activate Transaction
	{0x01, 0x01, -1, LITTLE_ENDIAN},		// Set Display
	{0x03, 0x03, -1, LITTLE_ENDIAN},		// Update Balance
	{0x03, 0x02,  0, LITTLE_ENDIAN},		// Get EMV Configuration
	{0x04, 0x00, -1, LITTLE_ENDIAN},		// Set EMV Configuration
	{0x09, 0x00,  1, LITTLE_ENDIAN},		// Get Version
	{0x01, 0x04,  1, LITTLE_ENDIAN},		// Get LCD
	{0x01, 0x03, -1, LITTLE_ENDIAN},		// Store LCD
	{0x01, 0x02,  4, LITTLE_ENDIAN},		// Set LCD
	{0x03, 0x05,  0, LITTLE_ENDIAN},		// Get all AIDS
	{0x04, 0x02, -1, LITTLE_ENDIAN},		// Set Configurable AID
	{0x24, 0x01, -1, LITTLE_ENDIAN},		// Set CAPK
	{0x24, 0x03,  0, LITTLE_ENDIAN},		// Delete All CAPK
	{0x09, 0x01,  0, LITTLE_ENDIAN},		// Get Product Type
	{0x05, 0x01,  0, LITTLE_ENDIAN},		// Cancel Transaction
	{0x04, 0x05, -1, LITTLE_ENDIAN},		// Delete Configurable Group
	{0x04, 0x03, -1, LITTLE_ENDIAN},		// Set Configurable Group
	{0x04, 0x04, -1, LITTLE_ENDIAN},		// Delete Configurable AID
	{0x03, 0x04, -1, LITTLE_ENDIAN},		// Get Configurable AID
	{0x03, 0x06, -1, LITTLE_ENDIAN},		// Get Configurable Group
	{0x03, 0x07, -1, LITTLE_ENDIAN}			// Get All Groups
};

typedef enum
{
	DEFAULT = 0, QX1000, QX120, VX810_CTLS
} PRODUCT_TYPE;

typedef struct
{
	int tag;
	int taglen;
	char tagdata[100];
} EMV_TAG_REC;

typedef struct
{
	unsigned char track1[100];
	int track1len;
	unsigned char track2[100];
	int track2len;
	int clearingreclen;
	int clearingrecindex;
	int datatagindex;
	EMV_TAG_REC clearingrectag[20];
	EMV_TAG_REC datatag[100];
} CARD_DATA;

CARD_DATA CardData;

int ViVOtechGenerateCommand(CTLS_VIVOTECH_CMD_TYPE cmd, unsigned char *sendCmd, int *sendCmdlen, unsigned char *data, int datalen);
int ViVOtechGenerateKeyMgmtCommand(CTLS_VIVOTECH_CMD_TYPE cmd, unsigned char *sendCmd, int *sendCmdlen, unsigned char *data, int datalen,
								   unsigned char *reqdata1, int *reqdata1len, unsigned char *reqdata2, int *reqdata2len);
void ViV0techGenerateKeyDataMsgs(unsigned char *reqdata, int *reqdatalen, unsigned char *data, int datalen);
int ViVOpayCheckResponse(unsigned char *data, int length);
PRODUCT_TYPE ViVOpayGetProductType(void);
int ViVOpayLoadMessages(PRODUCT_TYPE ProdType);
int ViVOpayGetMessages(void);
int ViVOpayLoadConfig(void);
int ViVOpayGetConfig(void);
int ViVOpayProcessAIDTable(void);
int ViVOpayGetAllConfigAIDs(void);
int ViVOpayGetAllConfigGroups(void);


///////////////////////////////////////////////////////////////////////////////

//
// Wave3 defintions and commands
//

typedef enum
{
	POLL = 0, ECHO, RESET, INITCOMMS, MUTUAL_AUTH, GET_KEYS, INV_READER, SWITCH_ADMIN,
	GET_CAPA, SET_CAPA, SALE_TRAN, SHOW_STS, GET_DATETIME, SET_DATETIME, GET_PARAMS,
	GET_VISAPK, SET_VISAPK, GET_EMVTAGS, SET_EMVTAGS, GET_CVMCAPA
} CTLS_WAVE3_CMD_TYPE;

typedef struct
{
	int command;
	int datalength;
} CTLS_WAVE3_CMD_VALUES;

CTLS_WAVE3_CMD_VALUES ctls_wave3_cmd[20] = 
{
	{0x07, 0},			// Poll
	{0x08, -1},			// Echo
	{0x31, 0 },			// Reset
	{0x20, 10},			// Initialise Communications
	{0x21, 10},			// Mutual Authenticate
	{0x22, 18},			// Generate Key
	{0x23, 0},			// Invalidate Reader
	{0x40, 1},			// Switch to Admnistrative Mode
	{0x41, -1},			// Get Capability
	{0x42, -1},			// Set Capability
	{0x30, 6},			// Ready for Sales Transaction
	{0x32, -1},			// Show Status
	{0x43, 0},			// Get Date and Time
	{0x44, 7},			// Set Date and Time
	{0x45, 2},			// Get Parameters
	{0x50, 1},			// Get Visa Public Key
	{0x51, -1},			// Set Visa Public Key
	{0x56, -1},			// Get EMV Tags Values
	{0x57, -1},			// Set EMV Tags Values
	{0x5A, 1}			// Get CVM Capability
};
  
unsigned int ctls_seqnumber = 0;

int Wave3GenerateCommand(CTLS_WAVE3_CMD_TYPE cmdType, unsigned char *req, int *reqlen, unsigned char *data, int datalen);
unsigned int Wave3IncreaseSeqNumber(void);
int Wave3GoodSeqNumber(unsigned int seqnumber);
int Wave3CheckResponse(unsigned char *data, int length);
int Wave3SetEMVTag(void);

///////////////////////////////////////////////////////////////////////////////
int ctlsCommand(int vivopay, int timeout, unsigned char *req, int reqlen, unsigned char *resp, int *resplen);
int ctlsPending(void);
int ctlsSend(unsigned char *req, int reqlen);
int ctlsRead(int vivopay, unsigned char *resp, int *resplen);
void ClearCardData(void);
int readCtlsRecord(int fHandle , char *buf, int AllowNewLine);
//int nProcessESTFile(void);

///////////////////////////////////////////////////////////////////////////////
//
// private variables
//

int ctls_reader = false;
int ctls_on = false;
int ctls_port = 0;
int ctls_evo = false;
int ctls_read_timer;
int ctls_online_auth_req = false;
ctls_read_cb ctls_read_callback;
int ctls_timeout = 100;
int ctls_wait_tap = false;
int ctls_firsttime = false;
int ctls_maxtranslimit = 100;

char sPrntBuff[200];

////////////////////////////////////////////////////////////////////////////////
//

void ctls_ui_stop_handler(void* notUsed)
{
}

int ctlsInitialise(int port)
{
	int i = 0;
	int ret;
	CTLSUIFuncs ctlsuistruct;

	// assume no ctls reader at this stage
	ctls_on = false;
	ctls_evo = true;

	if(ctls_reader) return(0);

	{

		// initialise CTLS
		ret = CTLSInitInterface(20000);
		// dbprintf("ctlsInitialise: CTLSInitInterface ret %d", ret);
		if (ret != 0)
		{
			// dbprintf("ctlsInitialise: error initialising CTLS reader");
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
			// dbprintf("ctlsInitialise: error opening CTLS reader");
			return -1;
		}

		// configure the CTLS app
		ret = CTLS_GREEN_STYLE_LED;
		CTLSClientUIParamHandler(UI_LED_STYLE_PARAM, &ret, sizeof(ret));
		CTLSClientUISetCardLogoBMP("N:/CTLSMV.bmp");

	}

	if ((ret = CTLSGetUI(&ctlsuistruct)) == 0)
	{
			//ctlsuistruct.startUIHandler = ctls_ui_start_handler;
			//ctlsuistruct.stopUIHandler = ctls_ui_stop_handler;
			//ctlsuistruct.ledHandler = ctls_ui_led_handler;
			//ctlsuistruct.textHandler = ctls_ui_text_handler;
			//ctlsuistruct.clearDisplayHandler = ctls_ui_clear_text_handler;
			//ret = CTLSSetUI(&ctlsuistruct);
	}

	if (ViVOpayLoadConfig() != 0)
	{
		return -1;
	}

	if (ViVOpayProcessAIDTable() != 0)
	{
		return -1;
	}

	ViVOpayGetMessages();
	ViVOpayGetConfig();
	ViVOpayGetAllConfigAIDs();
	ViVOpayGetAllConfigGroups();

	// ctls reader is present and communications is established
	ctls_reader = true;

	return 0;
}

////////////////////////////////////////////////////////////////////////////////

void ctlsEnableCallback(int timer)
{
	unsigned char resp[1024];
	int resplen = sizeof(resp);
	
	char sTmpBuff[1024]="",sTmpBuff1[2048];
	char *pTmpBuff,*pTmpBuff1;
	int i,j,nRetVal,length;
	char buf = 0;

	if (ctlsPending())
	{
		memset(resp,0,sizeof(resp));
		if (ctlsRead(true, resp, &resplen) < 0)
		{
			SVC_WAIT(1000);
			return;
		}

		{//DEBUG
		memset(sTmpBuff,0,sizeof(sTmpBuff));
		for(i=0;i<resplen;i++) { sprintf( sTmpBuff1,"%02x", resp[i]); strcat( sTmpBuff,sTmpBuff1); }
		}
		
		if (resp[10] == ctls_vivotech_cmd[ACTIVATE_TRAN].command)
		{
			// perform the required tones
			// TODO: what should we do for 0x23 Online Required? Need to see what QX1000 does...
			//

			if (resp[11] == 0x00) // success
			{
					/*{
						sprintf( sTmpBuff1, "\\4**DATAlen**%d\\n",resplen );
						__print_cont( sTmpBuff1, false );
						for(i=0;i<resplen*2;i+=100){
								sprintf( sTmpBuff1, "\\4%100.100s\\n",sTmpBuff+i );
								__print_cont( sTmpBuff1, false );
						}
					}
					*/
				#ifdef __PRNTDEBUG
					memset(sPrntBuff,0x00,sizeof(sPrntBuff));
					sprintf(sPrntBuff,"ctlsEnableCallback Success resplen:%d \n",resplen);
					////DPPrintDebug(sPrntBuff);
					////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);
			#endif
				// play sound of success
				sound(58, 500); // 1500Hz 500msec
				SVC_WAIT(550);

			#ifdef __PRNTDEBUG
				memset(sTmpBuff,0x00,sizeof(sTmpBuff));
				pTmpBuff = sTmpBuff;
				for(i=13;i<60;i++)
				{
					pTmpBuff+=sprintf(pTmpBuff,"%02x",resp[i]);
				}				
				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n sTmpBuff:%s \n",sTmpBuff);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				length = strlen(sTmpBuff);
				memset(sTmpBuff1,0x00,sizeof(sTmpBuff1));
				pTmpBuff1 = sTmpBuff1;
				for(i = 0; i < length; i++)
				{
						if(i % 2 != 0)
						{
								pTmpBuff1+=sprintf(pTmpBuff1,"%c ",hex_to_ascii(buf, sTmpBuff[i]));
						}
						else
						{
								buf = sTmpBuff[i];
						}
				}

				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n ASCII:%s \n",sTmpBuff1);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);
						
				memset(sTmpBuff,0x00,sizeof(sTmpBuff));
				pTmpBuff = sTmpBuff;
				for(i=61;i<100;i++)
				{
					pTmpBuff+=sprintf(pTmpBuff,"%02x",resp[i]);
				}				
				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n sTmpBuff:%s \n",sTmpBuff);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				length = strlen(sTmpBuff);
				memset(sTmpBuff1,0x00,sizeof(sTmpBuff1));
				pTmpBuff1 = sTmpBuff1;
				for(i = 0; i < length; i++)
				{
						if(i % 2 != 0)
						{
								pTmpBuff1+=sprintf(pTmpBuff1,"%c ",hex_to_ascii(buf, sTmpBuff[i]));
						}
						else
						{
								buf = sTmpBuff[i];
						}
				}

				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n ASCII:%s \n",sTmpBuff1);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				memset(sTmpBuff,0x00,sizeof(sTmpBuff));
				pTmpBuff = sTmpBuff;
				for(i=101;i<150;i++)
				{
					pTmpBuff+=sprintf(pTmpBuff,"%02x",resp[i]);
				}				
				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n sTmpBuff:%s \n",sTmpBuff);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				length = strlen(sTmpBuff);
				memset(sTmpBuff1,0x00,sizeof(sTmpBuff1));
				pTmpBuff1 = sTmpBuff1;
				for(i = 0; i < length; i++)
				{
						if(i % 2 != 0)
						{
								pTmpBuff1+=sprintf(pTmpBuff1,"%c ",hex_to_ascii(buf, sTmpBuff[i]));
						}
						else
						{
								buf = sTmpBuff[i];
						}
				}

				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n ASCII:%s \n",sTmpBuff1);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				memset(sTmpBuff,0x00,sizeof(sTmpBuff));
				pTmpBuff = sTmpBuff;
				for(i=151;i<200;i++)
				{
					pTmpBuff+=sprintf(pTmpBuff,"%02x",resp[i]);
				}				
				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n sTmpBuff:%s \n",sTmpBuff);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				length = strlen(sTmpBuff);
				memset(sTmpBuff1,0x00,sizeof(sTmpBuff1));
				pTmpBuff1 = sTmpBuff1;
				for(i = 0; i < length; i++)
				{
						if(i % 2 != 0)
						{
								pTmpBuff1+=sprintf(pTmpBuff1,"%c ",hex_to_ascii(buf, sTmpBuff[i]));
						}
						else
						{
								buf = sTmpBuff[i];
						}
				}

				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n ASCII:%s \n",sTmpBuff1);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				memset(sTmpBuff,0x00,sizeof(sTmpBuff));
				pTmpBuff = sTmpBuff;
				for(i=201;i<resplen;i++)
				{
					pTmpBuff+=sprintf(pTmpBuff,"%02x",resp[i]);
				}				
				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n sTmpBuff:%s \n",sTmpBuff);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				length = strlen(sTmpBuff);
				memset(sTmpBuff1,0x00,sizeof(sTmpBuff1));
				pTmpBuff1 = sTmpBuff1;
				for(i = 0; i < length; i++)
				{
						if(i % 2 != 0)
						{
								pTmpBuff1+=sprintf(pTmpBuff1,"%c ",hex_to_ascii(buf, sTmpBuff[i]));
						}
						else
						{
								buf = sTmpBuff[i];
						}
				}

				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n ASCII:%s \n",sTmpBuff1);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				SVC_WAIT(500);

			#endif

			}
			else if (resp[11] == 0x23) // Request Online Authorisation
			{
				#ifdef __PRNTDEBUG
					////DPPrintDebug("Online Authorisation required");
				////LOG_PRINTFF (0x00000001L,"Online Authorisation required \n");
			#endif
				// play sound of success
				//sound(58, 500); // 1500Hz 500msec
				//SVC_WAIT(550);
			}
			else if (resp[11] != 0x08) // failure (except for timeout)
			{
				#ifdef __PRNTDEBUG
					////DPPrintDebug("Fail");
					////LOG_PRINTFF (0x00000001L,"Fail \n");
			#endif
				switch (resp[14])
				{
					case 0x20:	// Card returned Error Status
					case 0x21:	// Collision Error
					case 0x30:	// Card did not respond
						// no sound
						break;

					case 0x23:	//  Request Online Authorisation
					case 0x43: // Card Generated ARQC
						// play sound of success
						//sound(58, 500); // 1500Hz 500msec
						//SVC_WAIT(550);
						break;

					case 0x42:	// Card Generated AAC	
						// beep later
						break;

					case 0x02:  // Go to Contact Interface
					case 0x22:  // Amount over Maximum Offline Limit
					default:
						// play sound of failure
						sound(45, 200); // 750Hz 200msec
						SVC_WAIT(400);
						sound(45, 200); // 750Hz 200msec
						SVC_WAIT(250);
						break;
				}
			}

			ctls_wait_tap = false;

			// make the callback - providing transaction result data without message header/trailer
			// TODO: for Wave3 need to include decryption and different data extraction
			if (true || ctls_read_callback != null) //TEST
			{
			unsigned int cardtype;
			//	ctls_read_callback(ctls_timeout, ctls_firsttime, &resp[10], resplen - 12);
			//int ctlsParseData(unsigned char *cardtype, long amount, unsigned char *data, int datalen)
			
					cardtype = 0;
					nRetVal = ctlsParseData(&cardtype, 10, &resp[10], resplen - 12);

				#ifdef __PRNTDEBUG
					memset(sPrntBuff,0x00,sizeof(sPrntBuff));
					sprintf(sPrntBuff,"\n ctlsParseData nRetVal:%d \n",nRetVal);
					////DPPrintDebug(sPrntBuff);
					////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);
				#endif
			}
		}
		else if (resp[10] == ctls_vivotech_cmd[CANCEL_TRANSACTION].command)
		{
			ctls_wait_tap = false;
			ctlsReset();
		}
	}
	else
	{
		// create a new timer and continue waiting
		//Dwarika .. for testing
	//	if (!ctls_wait_tap || ((ctls_read_timer = timerCreate(100, ctlsEnableCallback)) < 0))
			if (!ctls_wait_tap)
		{
			// TODO: ctls off? callback in error?
			return;
		}
	}

	/**Print data****/
	#ifdef __PRNTDEBUG
				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n CardData.clearingreclen:%d \n",CardData.clearingreclen);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n CardData.clearingrecindex:%d \n",CardData.clearingrecindex);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n CardData.datatagindex:%d \n",CardData.datatagindex);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n CardData.track1len:%d \n",CardData.track1len);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				memset(sTmpBuff,0x00,sizeof(sTmpBuff));
				pTmpBuff = sTmpBuff;
				for(i=0;i<50;i++)
				{
					pTmpBuff+=sprintf(pTmpBuff,"%02x",CardData.track1[i]);
				}				
				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n CardData.track1 :%s \n",sTmpBuff);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				length = strlen(sTmpBuff);
				memset(sTmpBuff1,0x00,sizeof(sTmpBuff1));
				pTmpBuff1 = sTmpBuff1;
				for(i = 0; i < length; i++)
				{
						if(i % 2 != 0)
						{
								pTmpBuff1+=sprintf(pTmpBuff1,"%c ",hex_to_ascii(buf, sTmpBuff[i]));
						}
						else
						{
								buf = sTmpBuff[i];
						}
				}

				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n ASCII:%s \n",sTmpBuff1);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				memset(sTmpBuff,0x00,sizeof(sTmpBuff));
				pTmpBuff = sTmpBuff;
				for(i=51;i<100;i++)
				{
					pTmpBuff+=sprintf(pTmpBuff,"%02x",CardData.track1[i]);
				}				
				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n CardData.track1 :%s \n",sTmpBuff);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				length = strlen(sTmpBuff);
				memset(sTmpBuff1,0x00,sizeof(sTmpBuff1));
				pTmpBuff1 = sTmpBuff1;
				for(i = 0; i < length; i++)
				{
						if(i % 2 != 0)
						{
								pTmpBuff1+=sprintf(pTmpBuff1,"%c ",hex_to_ascii(buf, sTmpBuff[i]));
						}
						else
						{
								buf = sTmpBuff[i];
						}
				}

				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n ASCII:%s \n",sTmpBuff1);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n CardData.track2len:%d \n",CardData.track2len);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				memset(sTmpBuff,0x00,sizeof(sTmpBuff));
				pTmpBuff = sTmpBuff;
				for(i=0;i<50;i++)
				{
					pTmpBuff+=sprintf(pTmpBuff,"%02x",CardData.track2[i]);
				}				
				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n CardData.track2 :%s \n",sTmpBuff);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				length = strlen(sTmpBuff);
				memset(sTmpBuff1,0x00,sizeof(sTmpBuff1));
				pTmpBuff1 = sTmpBuff1;
				for(i = 0; i < length; i++)
				{
						if(i % 2 != 0)
						{
								pTmpBuff1+=sprintf(pTmpBuff1,"%c ",hex_to_ascii(buf, sTmpBuff[i]));
						}
						else
						{
								buf = sTmpBuff[i];
						}
				}

				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n ASCII:%s \n",sTmpBuff1);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				memset(sTmpBuff,0x00,sizeof(sTmpBuff));
				pTmpBuff = sTmpBuff;
				for(i=51;i<100;i++)
				{
					pTmpBuff+=sprintf(pTmpBuff,"%02x",CardData.track2[i]);
				}				
				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n CardData.track2 :%s \n",sTmpBuff);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				length = strlen(sTmpBuff);
				memset(sTmpBuff1,0x00,sizeof(sTmpBuff1));
				pTmpBuff1 = sTmpBuff1;
				for(i = 0; i < length; i++)
				{
						if(i % 2 != 0)
						{
								pTmpBuff1+=sprintf(pTmpBuff1,"%c ",hex_to_ascii(buf, sTmpBuff[i]));
						}
						else
						{
								buf = sTmpBuff[i];
						}
				}

				memset(sPrntBuff,0x00,sizeof(sPrntBuff));
				sprintf(sPrntBuff,"\n ASCII:%s \n",sTmpBuff1);
				////DPPrintDebug(sPrntBuff);
				////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

				for(i = 0; i < 50; i++)
				{
					memset(sPrntBuff,0x00,sizeof(sPrntBuff));
					sprintf(sPrntBuff,"\n tag[%d]:%d %x \n",i,CardData.datatag[i].tag,CardData.datatag[i].tag);
					////DPPrintDebug(sPrntBuff);
					////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

					memset(sPrntBuff,0x00,sizeof(sPrntBuff));
					sprintf(sPrntBuff,"\n taglen[%d]:%d \n",i,CardData.datatag[i].taglen);
					////DPPrintDebug(sPrntBuff);
					////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);

					if(CardData.datatag[i].taglen)
					{
						pTmpBuff = sTmpBuff;
						for(j = 0; j < CardData.datatag[i].taglen; i++)
						{
							pTmpBuff+=sprintf(pTmpBuff,"%02x",CardData.datatag[i].tagdata[j]);
						}
						memset(sPrntBuff,0x00,sizeof(sPrntBuff));
						sprintf(sPrntBuff,"\n sTmpBuff:%s \n",sTmpBuff);
						////DPPrintDebug(sPrntBuff);
						////LOG_PRINTFF (0x00000001L,"sPrntBuff:%s: \n",sPrntBuff);
					}

				}

				SVC_WAIT(500);

			#endif
	/**Print data****/

	return;
}

////////////////////////////////////////////////////////////////////////////////

int ctlsEnable( int timeout, int trantype, unsigned long amount, ctls_read_cb callback)
{
	unsigned char req[1024];
	int reqlen = 0;
	unsigned char resp[1024];
	int resplen = sizeof(resp);
	char buffer[16];
	int tries = 0;
	int nRetVal = 0;
	static int firsttime = 1;
	unsigned char data[] =
	{
		0x1E,
		0x9F, 0x02, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x5F, 0x2A, 0x02, 0x00, 0x36,
		0x9A, 0x03, 0x00, 0x00, 0x00,
		0x9F, 0x21, 0x03, 0x00, 0x00, 0x00,
		0x9F, 0x1A, 0x02, 0x00, 0x36,
		0x9C, 0x01, 0x00 };

	if (!ctls_reader) 
	{
		return -1;
	}

	LOG_PRINTFF(0x00000001L,"ctlsEnable");
	
	// check if ctls already enabled - assume all ok
	if (ctls_on)
	{
		return 0;
	}

	if (amount >= ctls_maxtranslimit)
	{
		//return -2; //TEST
	}

	ctls_timeout = 0;
	ctls_online_auth_req = false;
	ctls_wait_tap = false;
	ctls_firsttime = firsttime;
	ClearCardData();

	LOG_PRINTFF(0x00000001L,"ctlsEnable 1");
	
	// send poll on demand
	// dbprintf("ctlsEnable: SET_POLL Command");
	if( firsttime) {
		LOG_PRINTFF(0x00000001L,"ctlsEnable 2");
		firsttime = 0;
		if (ViVOtechGenerateCommand(SET_POLL, req, &reqlen, "\x01", 1) != 0)
		{
			return -1;
		}
		
		LOG_PRINTFF(0x00000001L,"ctlsEnable 3");

		while (ctlsCommand(true, 2, req, reqlen, resp, &resplen) != 0)
		{
			if (resplen == 0 && tries <= 2)
			{
				// timeout, wait and try again
				SVC_WAIT(500); 
				resplen = sizeof(resp);
				tries++;
				// dbprintf("ctlsEnable: Resending SET_POLL Command");
			}
			else
			{
				return -1;
			}
		}
		
		LOG_PRINTFF(0x00000001L,"ctlsEnable 4");
		
		if (ctls_vivotech_cmd[SET_POLL].command != resp[10])
		{
			return -1;
		}
	}
	
	LOG_PRINTFF(0x00000001L,"ctlsEnable 5");

	// prepare the data for the activate transaction command
	data[0] = (unsigned char)timeout;
	sprintf(buffer, "%012lu", amount);
	// dbprintf("ctlsEnable: amount is %s", buffer);
	SVC_DSP_2_HEX(buffer, (char *)&data[4], 6);

	// TODO; change currency? or can use default?
	/*
	if (SVC_CLOCK(0, buffer, 15) != 15)
	{	
		return -1;
	}
	*/
	SVC_DSP_2_HEX(&buffer[2], (char *)&data[17], 3);
	SVC_DSP_2_HEX(&buffer[8], (char *)&data[23], 3);
	data[33] = (unsigned char)trantype;
	
	LOG_PRINTFF(0x00000001L,"ctlsEnable 6");

	// send activate transaction command
	// dbprintf("ctlsEnable: ACTIVATE_TRAN Command");
	if(0)
	{
		LOG_PRINTFF(0x00000001L,"ctlsEnable 7");
		if (ViVOtechGenerateCommand(GET_LCD, req, &reqlen, "\x06", 1) != 0)
		{
			return -1;
		}

		while (ctlsCommand(true, 2, req, reqlen, resp, &resplen) != 0)
		{
			if (resplen == 0 && tries <= 2)
			{
				// timeout, wait and try again
				SVC_WAIT(500); 
				resplen = sizeof(resp);
				tries++;
				// dbprintf("ctlsEnable: Resending SET_POLL Command");
			}
			else
			{
				return -1;
			}
		}

		if (ctls_vivotech_cmd[GET_LCD].command != resp[10])
		{
			return -1;
		}

	}
	LOG_PRINTFF(0x00000001L,"ctlsEnable 8");
	if (ViVOtechGenerateCommand(ACTIVATE_TRAN, req, &reqlen, data, sizeof(data)) != 0)
	{
		return -1;
	}
	LOG_PRINTFF(0x00000001L,"ctlsEnable 9");
	if (ViVOtechGenerateCommand(ACTIVATE_TRAN, req, &reqlen, data, sizeof(data)) != 0)
	{
		return -1;
	}
	LOG_PRINTFF(0x00000001L,"ctlsEnable 10");
	if (ctlsSend(req, reqlen) != 0)
	{
		return -1;
	}
	else
	{
	}
	LOG_PRINTFF(0x00000001L,"ctlsEnable 11");
	// save the callback
	ctls_wait_tap = true;
	ctls_timeout = timeout;
	ctls_read_callback = callback;

	// create a timer for checking on the response
	//Dwarika .. for testing
	//if ((ctls_read_timer = timerCreate(100, ctlsEnableCallback)) < 0)
	// return -1;
	ctlsEnableCallback(100);
	LOG_PRINTFF(0x00000001L,"ctlsEnable 12");
	// all ready - ctls is on
	ctls_on = true;

	return 0;
}

int ctlsDisable(void)
{
	unsigned char req[1024];
	int reqlen = 0;
	unsigned char resp[1024];
	int resplen = sizeof(resp);

	if (!ctls_on)
		return 0;

	ctls_on = false;
	ctls_wait_tap = false;

	// cancel any outstanding transaction
	// dbprintf("ctlsDisable: CANCEL_TRANSACTION Command");
	if (ViVOtechGenerateCommand(CANCEL_TRANSACTION, req, &reqlen, null, 0) != 0)
		return -1;
	if (ctlsCommand(true, 2, req, reqlen, resp, &resplen) != 0)
		return -1;
	if (ctls_vivotech_cmd[CANCEL_TRANSACTION].command != resp[10])
		return -1;

	ctlsReset();
	return 0;
}

int ctlsParseData(unsigned int *cardtype, long amount, unsigned char *data, int datalen)
{
	int index;
	int extract_data = false;
	int respcode = 0;
	char RIDList[][10 + 1] = {"A000000003",  "A000000004"};
	unsigned char buffer[32 + 1];
	int CIDPresent = false;
	int refund = false;
	int CardGeneratedAAC = false;

	ctls_online_auth_req = false;
	if (data[0] == ctls_vivotech_cmd[ACTIVATE_TRAN].command)
	{		
		if	(data[1] == 0 || data[1] == 0x23) //Successful or Req Online Auth
		{
			extract_data = true;

			index = 4;
			CardData.track1len = data[index++];
			if (CardData.track1len > 0)
			{
				memcpy(CardData.track1, &data[index], CardData.track1len);
				index += CardData.track1len;
//				vdPrintLargeHexData("CardData.track1:", CardData.track1, CardData.track1len);
			}

			CardData.track2len = data[index++];
			if (CardData.track2len > 0)
			{
				memcpy(CardData.track2, &data[index], CardData.track2len);
				index += CardData.track2len;
//				vdPrintLargeHexData("CardData.track2:", CardData.track2, CardData.track2len);
			}

			if (data[index] == 1)
			{	
				////LOG_PRINTFF(0x00000001L,"got clearing rec data");
				CardData.clearingreclen = data[index++];
				index += CardData.clearingreclen;
			}

			if (data[1] == 0x23)
				ctls_online_auth_req = true;

			////LOG_PRINTFF(0x00000001L,"data[1] = %02X, ctls_online_auth_req = %d", data[1], ctls_online_auth_req);
		}
		else if (data[1] == 0x08) // timeout
		{
			respcode = TIMED_OUT;

		}
		else if (data[1] == 0x0A) // Failed
		{
			int resp_error = data[4];
			
			index = 7;
			switch (resp_error)
			{
				case 0x02: // Go to Contact Interface
				case 0x22: // Amount over Maximum Offline Limit
				case 0x27: // Unsupported Card
					respcode = INSERT_CARD;
					break;

				case 0x42:	// Card Generated AAC	
					////LOG_PRINTFF(0x00000001L,"Card Generated AAC");
					CardGeneratedAAC = true;
					extract_data = true;
					break;

				case 0x23: // Require Online Authorisation
				case 0x43: // Card Generated ARQC
					extract_data = true;
					ctls_online_auth_req = true;
					respcode = 0;
					break;

				case 0x20: // Card returned error status
					if (data[5] == 0x90 && data[6] == 0x00) //SW1SW2
						respcode = -1;
					else if ((data[5] == 0x69 && data[6] == 0x84) || //SW1SW2 - Invalid data
						(data[5] == 0x69 && data[6] == 0x85)) //SW1SW2
						respcode = INSERT_CARD;
					else
						respcode = CTLS_READ_RETRY;

					if (respcode != CTLS_READ_RETRY)
					{
						// play sound of failure
						sound(45, 200); // 750Hz 200msec
						SVC_WAIT(400);
						sound(45, 200); // 750Hz 200msec
						SVC_WAIT(250);
					}
					break;

				case 0x21: // Collision Error
				case 0x30: // Card did not respond
					respcode = CTLS_READ_RETRY;
					break;

				case 0x25: // Card Blocked
				case 0x26: // Card Expired
				case 0x04: // Read Record Error
				default:
					respcode = -1;
					break;
			}
		}

		if (extract_data)
		{
			index++;
			while (index < datalen)
			{
				CardData.datatag[CardData.datatagindex].tag = data[index] << 8;

				if (data[index] == 0x1f || data[index] == 0x5f || data[index] ==  0x9f
					|| data[index] == 0xe3 || data[index] == 0xff || data[index] == 0xdf)
				{
					index++;
					CardData.datatag[CardData.datatagindex].tag += data[index];
				}

				index++;
				CardData.datatag[CardData.datatagindex].taglen = data[index++];
				if(CardData.datatag[CardData.datatagindex].taglen)
					memcpy(CardData.datatag[CardData.datatagindex].tagdata, &data[index], 
						CardData.datatag[CardData.datatagindex].taglen);
//DebugPrint ("parse...6 tag=%d,len=%d",CardData.datatag[CardData.datatagindex].tag,CardData.datatag[CardData.datatagindex].taglen);

				////LOG_PRINTFF(0x00000001L,"CardData.datatag, index=%d, tag=0x%04X",CardData.datatagindex, CardData.datatag[CardData.datatagindex].tag);

//				vdPrintLargeHexData("CardData.datatag:", 
//					(unsigned char *)CardData.datatag[CardData.datatagindex].tagdata,
//					CardData.datatag[CardData.datatagindex].taglen);

				if (CardData.datatag[CardData.datatagindex].tag == 0x9F27) // CID
					CIDPresent = true;
				else if (CardData.datatag[CardData.datatagindex].tag == 0x9F06)  // AID
				{
					SVC_HEX_2_DSP(CardData.datatag[CardData.datatagindex].tagdata, (char *)buffer, 
						CardData.datatag[CardData.datatagindex].taglen);
					buffer[CardData.datatag[CardData.datatagindex].taglen * 2] = 0;
	
					if (memcmp(RIDList[0], (char *)buffer, strlen(RIDList[0])) == 0)
						*cardtype = 5; // VISA ctls emv
					else if (memcmp((char *)RIDList[1], (char *)buffer, strlen(RIDList[1])) == 0)
						*cardtype = 3; // Mastercard ctls emv
					else
						*cardtype = 10;

					if (!CardGeneratedAAC && *cardtype != 10 && (CardData.clearingreclen == 0))
					{
						(*cardtype)++;  // VISA or MasterCard ctls MSD
						ctls_online_auth_req = true;
					}
				}
				else if (CardData.datatag[CardData.datatagindex].tag == 0x9C00)  // Transaction Type
				{
					if ((CardData.datatag[CardData.datatagindex].taglen == 1) &&
						(CardData.datatag[CardData.datatagindex].tagdata[0] == 0x20))
						refund = true;
				}

				index += CardData.datatag[CardData.datatagindex].taglen;
				CardData.datatagindex++;
			}

			if (data[1] == 0 || ctls_online_auth_req) //Successful or Require Online Authorisation
			{
				if (*cardtype == 3 || *cardtype == 5)  // VISA or MasterCard CTLS EMV
				{
					////LOG_PRINTFF(0x00000001L,"ctlsParseData: for ctls emv cards amount=%ld", amount);
					if ((strlen((char*)buffer) > 0) && (amount >= ctlsGetLimit(2,(char*)buffer)))
					{
						respcode = INSERT_CARD;	

						// play sound of failure
						sound(45, 200); // 750Hz 200msec
						SVC_WAIT(400);
						sound(45, 200); // 750Hz 200msec
						SVC_WAIT(250);
					}
				}
				else if (*cardtype == 4 || *cardtype == 6)  // VISA or MasterCard CTLS MS
				{
					////LOG_PRINTFF(0x00000001L,"ctlsParseData: for ctls ms cards amount=%ld", amount);
					if ((strlen((char*)buffer) > 0) && (amount >= ctlsGetLimit(2,(char*)buffer)))
					{
						//respcode = USE_MAG_CARD;

						// play sound of failure
						//sound(45, 200); // 750Hz 200msec
						//SVC_WAIT(400);
						//sound(45, 200); // 750Hz 200msec
						//SVC_WAIT(250);
					}
				}

				if (respcode == 0)
				{
					// play sound of success
					sound(58, 500); // 1500Hz 500msec
					SVC_WAIT(550);
				}

				if (!CIDPresent && respcode == 0)
				{
					// Add CID
					CardData.datatag[CardData.datatagindex].tag = 0x9F27;
					CardData.datatag[CardData.datatagindex].taglen = 1;

					if (!ctls_online_auth_req)
					{
						CardData.datatag[CardData.datatagindex].tagdata[0] = 0x40;
					}
					else
					{
						CardData.datatag[CardData.datatagindex].tagdata[0] = 0x80;
					}

					////LOG_PRINTFF(0x00000001L,"CID Not Present.  Added CID (9F27) = 0x%02X", CardData.datatag[CardData.datatagindex].tagdata[0]);

					CardData.datatagindex++;
				}
			}

			if (CardGeneratedAAC) // Card Generated AAC
			{
				if (refund && *cardtype == 3)
				{
					// play sound of success
					sound(58, 500); // 1500Hz 500msec
					SVC_WAIT(550);
				}
				else
				{	
					// play sound of failure
					sound(45, 200); // 750Hz 200msec
					SVC_WAIT(400);
					sound(45, 200); // 750Hz 200msec
					SVC_WAIT(250);
					respcode = -1;
				}
			}

			CardData.datatagindex--;
		}
	}

	return respcode;
}

int ctlsGetEMVTag(int EMVTag, unsigned char *data, int *datalen)
{
	int index;

	for (index = 0; index <= CardData.datatagindex; index++)
	{
		if (EMVTag == CardData.datatag[index].tag)
		{
			*datalen = CardData.datatag[index].taglen;
			if (*datalen > 0)
			{
				memcpy(data, CardData.datatag[index].tagdata, *datalen);
				return true;
			}
			else
				break;
		}
	}

	return false;
}

int ctlsSetEMVTag(int EMVTag, unsigned char *data, int datalen)
{
	int index;

	for (index = 0; index <= CardData.datatagindex; index++)
	{
		if (EMVTag == CardData.datatag[index].tag)
		{
			if (datalen == CardData.datatag[index].taglen)
			{
				memcpy(CardData.datatag[index].tagdata, data, datalen);
				return true;
			}
			else
				break;
		}
	}

	return false;
}

int ctlsGetTrackData(int trackno, unsigned char *trackdata, int *tracklen)
{
	*tracklen = 0;
	switch (trackno)
	{
		case 1:
			*tracklen = CardData.track1len;
			memcpy(trackdata, CardData.track1, *tracklen);
			break;
		case 2:
			*tracklen = CardData.track2len;
			memcpy(trackdata, CardData.track2, *tracklen);
			break;
		default:
			break;
	}

	if ((*tracklen) > 0)
	{
		if ((*tracklen) % 2 != 0)
		{
			//trackdata[*tracklen] = 'F';
			//(*tracklen)++;
		}
		////LOG_PRINTFF(0x00000001L,"CtlsGetTrackData: Track %d=%s, len=%d", trackno, trackdata, (*tracklen));
		return 1;
	}

	return 0;
}

int ctlsOnlineAuthReq(void)
{
	return (ctls_online_auth_req);
}

void ctlsReset(void)
{
		reset_ctls();
}

///////////////////////////////////////////////////////////////
//  Wave3 Functions
//
//g
int Wave3GenerateCommand(CTLS_WAVE3_CMD_TYPE cmdType, unsigned char *req, int *reqlen, unsigned char *data, int datalen)
{
	char buffer[1024] = {0x02, 0x00, 0x0B, 0x01, 0x0E, 0x01};
	int bufflen = datalen;
	unsigned short crc = 0;
	
	buffer[1] = 0;

	buffer[6] = ctls_wave3_cmd[cmdType].command;
	if (data == null)
		bufflen = 0;

	if (bufflen != ctls_wave3_cmd[cmdType].datalength && ctls_wave3_cmd[cmdType].datalength >= 0)
		return -1;

	buffer[7] = bufflen / 256;
	buffer[8] = bufflen % 256;

	if (bufflen > 0)
	{
		memcpy(&buffer[9], data, bufflen);
	}

	bufflen += 9;
	crc = SVC_CRC_CALC(2, &buffer[1], bufflen - 1);

	buffer[bufflen++] = crc % 256;
	buffer[bufflen++] = crc / 256;
	buffer[bufflen++] = 0x03;  // ETX
    *reqlen = bufflen;
	memcpy(req, buffer, *reqlen);
	
	return 0;
}

int Wave3CheckResponse(unsigned char *data, int length)
{
	unsigned short crcresp;
	unsigned short len;

	// check header - allowing for versions 1 and 2 of the protocol
	if (length < 9)
		return -1;

	len = data[7] * 256 + data[8];
	if (len + 12 != length)
		return - 1;
	
	if (data[0] != 0x02 || data[1] > 0xff) //|| !Wave3GoodSeqNumber(data[1]))
		return -1;

	if (data[2] != 0x0e || data[3] != 0x01)  // bad sender id
		return -1;

	if (data[4] != 0x0b || data[5] != 0x01)  // bad receiver id
		return -1;

	crcresp = SVC_CRC_CALC(2, (char *)&data[1], length - 4);

	if ((data[length - 3] != (crcresp % 256)) && (data[length - 2] != (crcresp / 256)))
		return -1; // error in response message - invalid crc

	return true;
}

unsigned int Wave3IncreaseSeqNumber(void)
{
	if (ctls_seqnumber == 0xff)
		ctls_seqnumber = 0x1;
	return (ctls_seqnumber++);
}

int Wave3GoodSeqNumber(unsigned int seqnumber)
{
	if (ctls_seqnumber++ == seqnumber)
		return 1;

	return 0;
}

int Wave3SetEMVTag(void)
{
	unsigned char req[1024], resp[1024];
	int reqlen, resplen;
	unsigned char data[1024];
	unsigned char EMVTags[] = {0x01, 0x9F, 0x35, 0x01, 0x00};  //9F35 - Terminal Type

	// Switch to Admin Mode
	memset(data, 0, sizeof(data));
	reqlen = sizeof(req);

	// dbprintf("Wave3SetEMVTag: SWITCH_ADMIN Command");
	if (Wave3GenerateCommand(SWITCH_ADMIN, req, &reqlen, data, 1) == 0)
	{
		resplen = sizeof(resp);
		if (ctlsCommand(false, 5, req, reqlen, resp, &resplen) == 0)
		{
			if (ctls_wave3_cmd[SWITCH_ADMIN].command != resp[6] &&
				resp[9] != 0)  //RC_SUCCESS
			{
				return -1;
			}
		}
	}

	// set EMV Tags
	reqlen = sizeof(req);
	// dbprintf("Wave3SetEMVTag: SET_EMVTAGS Command");
	if (Wave3GenerateCommand(SET_EMVTAGS, req, &reqlen, EMVTags, sizeof(EMVTags)) == 0)
	{
		resplen = sizeof(resp);
		if (ctlsCommand(false, 5, req, reqlen, resp, &resplen) == 0)
		{
			if (ctls_wave3_cmd[SET_EMVTAGS].command != resp[6] &&
				resp[9] != 0)  //RC_SUCCESS
			{
				return -1;
			}
		}
	}

	// Switch back to Normal Mode
	memset(data, 0, sizeof(data));
	data[0] = 1; // Normal Mode
	reqlen = sizeof(req);
	// dbprintf("Wave3ChangeEMVTag: SWITCH_ADMIN Command");
	if (Wave3GenerateCommand(SWITCH_ADMIN, req, &reqlen, data, 1) == 0)
	{
		resplen = sizeof(resp);
		if (ctlsCommand(false, 5, req, reqlen, resp, &resplen) == 0)
		{
			if (ctls_wave3_cmd[SWITCH_ADMIN].command != resp[6] &&
				resp[9] != 0)  //RC_SUCCESS
			{
				return -1;
			}
		}
	}

	return 0;
}


///////////////////////////////////////////////////////////////

//
// ViVOtech Functions
//
//

int ViVOtechGenerateCommand(CTLS_VIVOTECH_CMD_TYPE cmd, unsigned char *sendCmd, int *sendCmdlen, unsigned char *data, int datalen)
{
	int datalength;
	unsigned short crc;

	if (cmd != DEL_ALL_CAPK)
	{
		// set the Vivotech header
		strcpy((char *)sendCmd, "ViVOtech2");
	}
	else
	{
		// set the Vivotech header
		strcpy((char *)sendCmd, "ViVOtech");
		sendCmd[9] = 'C';
	}

	// set command and subcommand
	sendCmd[10] = ctls_vivotech_cmd[cmd].command;
	sendCmd[11] = ctls_vivotech_cmd[cmd].subcommand;

	// validate length of data provided for command
	datalength = datalen;
	if (data == null) datalength = 0;
	if (datalength != ctls_vivotech_cmd[cmd].datalength && ctls_vivotech_cmd[cmd].datalength >= 0)
		return -1;

	// set the data length
	sendCmd[12] = datalength / 256;
	sendCmd[13] = datalength % 256;

	// copy in the data provided
	if (datalength > 0)
		memcpy(&sendCmd[14], data, datalength);

	// set the crc
	crc = SVC_CRC_CCITT_M(sendCmd, 14 + datalength, 0xFFFF);
	if (ctls_vivotech_cmd[cmd].crctype == LITTLE_ENDIAN)
	{
		sendCmd[14 + datalength] = crc / 256;
		sendCmd[15 + datalength] = crc % 256;
	}
	else
	{
		sendCmd[14 + datalength] = crc % 256;
		sendCmd[15 + datalength] = crc / 256;
	}

	// set the buffer length for return
	*sendCmdlen = 16 + datalength;

	return 0;
}

int ViVOtechGenerateKeyMgmtCommand(CTLS_VIVOTECH_CMD_TYPE cmd, unsigned char *sendCmd, int *sendCmdlen, unsigned char *data, int datalen,
								   unsigned char *reqdata1, int *reqdata1len, unsigned char *reqdata2, int *reqdata2len)
{
	int datalength;
	unsigned short crc;
	int length1, length2;

	// set the Vivotech header
	strcpy((char *)sendCmd, "ViVOtech");
	sendCmd[9] = 'C';

	// set command and subcommand
	sendCmd[10] = ctls_vivotech_cmd[cmd].command;
	sendCmd[11] = ctls_vivotech_cmd[cmd].subcommand;

	// validate length of data provided for command
	datalength = datalen;
	if (data == null) datalength = 0;
	if (datalength != ctls_vivotech_cmd[cmd].datalength && ctls_vivotech_cmd[cmd].datalength >= 0)
		return -1;

	// set the data length
	length1 = datalength / 244;  //12 for header
	length2 = datalength % 244;

	if (length1 == 0)
	{
		sendCmd[12] = 0;
		sendCmd[13] = length2;
		length1 = length2;
		length2 = 0;
	}
	else
	{
		sendCmd[12] = length2;
		length1 = sendCmd[13] = 244;
	}

	crc = SVC_CRC_CCITT_M(sendCmd, 14, 0xFFFF);
	sendCmd[14] = crc / 256;
	sendCmd[15] = crc % 256;
	*sendCmdlen = 16;

	// set up the data messages
	ViV0techGenerateKeyDataMsgs(reqdata1, reqdata1len, data, length1);
	if (length2 > 0)
		ViV0techGenerateKeyDataMsgs(reqdata2, reqdata2len, &data[length1], length2);

	return 0;
}

void ViV0techGenerateKeyDataMsgs(unsigned char *reqdata, int *reqdatalen, unsigned char *data, int datalen)
{
	unsigned short crc;

	strcpy((char *)reqdata, "ViVOtech");
	reqdata[9] = 'D';
	memcpy(&reqdata[10], data, datalen);
	crc = SVC_CRC_CCITT_M(reqdata, 10 + datalen, 0xFFFF);
	reqdata[10 + datalen] = crc / 256;
	reqdata[11 + datalen] = crc % 256;
	*reqdatalen = 12 + datalen;
}

int ViVOpayCheckResponse(unsigned char *data, int length)
{
	char *header = "ViVOtech";
	int datalength;
	int i;
	unsigned short crcresp, crc;

	// check header - allowing for versions 1 and 2 of the protocol
	if (length < 10)
		return false;
	for (i = 0; i < 8; ++i)
		if (data[i] != header[i])
			return -1; // error in response message - invalid header

	// check the CRC
	if (length < 14)
		return false;
	datalength = data[12] * 256 + data[13];
	if (length < 16 + datalength)
		return false;
	crcresp = (data[15 + datalength] << 8) + data[14 + datalength];
	crc = SVC_CRC_CCITT_M(data, 14 + datalength, 0xFFFF);
	if (crc != crcresp)
		return -1; // error in response message - invalid crc
	return true;
}

PRODUCT_TYPE ViVOpayGetProductType(void)
{
	unsigned char req[1024];
	int reqlen = 0;
	unsigned char resp[1024];
	int resplen = sizeof(resp);

	PRODUCT_TYPE ProductType = DEFAULT;

	// dbprintf("ViVOpayGetProductType: GET_PRODUCT_TYPE Command");
	if (ViVOtechGenerateCommand(GET_PRODUCT_TYPE, req, &reqlen, null, 0) == 0)
	{
		if (ctlsCommand(true, 5, req, reqlen, resp, &resplen) == 0)
		{
			// check response is okay
			if (ctls_vivotech_cmd[GET_PRODUCT_TYPE].command == resp[10] && resp[11] == 0)
			{
				switch (resp[5])
				{
					case 1:
						ProductType = QX1000;
						break;
					case 2:
						ProductType = QX120;
						break;
					case 3:
						ProductType = VX810_CTLS;
						break;
					default:
						break;
				}
			}
		}
	}

	return ProductType;
}


int ViVOpayLoadMessages(PRODUCT_TYPE ProdType)
{
	char FileName[100];
	int fh;
	unsigned char sendCmd[1024];
	int sendCmdLength;
	unsigned char respCmd[1024];
	int respCmdLength;
	unsigned char data[1024];
	int datalen;
	int MsgIndex;
	char Msg[100];
	int i;
	int InMsg;
	char buffer[1024];

	// TODO:... remove
	ProdType = QX120;

	// dbprintf("ViVOpayLoadMessages: product %d", ProdType);

	//sendCmd[0] = 0xFE;
	//sendCmdLength = 1;
	//respCmdLength = sizeof(respCmd);
	//if (GenerateViVOtechCommand(STORE_LCD, sendCmd, &sendCmdLength))
	//	if (TransmitCommand(true, sendCmd, sendCmdLength, respCmd, &respCmdLength))
	//	{
	//		if (!(ctls_vivotech_cmd[STORE_LCD].command == respCmd[0] &&
	//			respCmd[1] == 0))
	//			return 0;
	//	}

	switch (ProdType)
	{
		case QX1000:
			strcpy(FileName, "ctlslcdmsg_vivotech_QX1000.txt");
			break;
		case QX120:
			strcpy(FileName, "ctlslcdmsg_vivotech_QX120.txt");
			break;
		case VX810_CTLS:
		case DEFAULT:
		default:
			strcpy(FileName, "ctlslcdmsg_vivotech_default.txt");
			break;
	}

	// dbprintf("ViVOpayLoadMessages: %s", FileName);
	if ((fh = open(FileName, O_RDONLY)) == -1)
	{
		return 0;
	}

	while (readCtlsRecord(fh, buffer, true) >= 0)
	{
		if (strlen(buffer) == 0)
			continue;

		if (!(buffer[0] == '/' && buffer[1] =='/'))
		{
			memset(sendCmd, 0, sizeof(sendCmd));
			strncpy(Msg, buffer, 2);
			MsgIndex = atoi(Msg);
			data[0] = MsgIndex;
			InMsg = false;

			for (i = 3, datalen = 1; i < (strlen(buffer) - 1); i++)
			{
				if (InMsg)
				{
					if (buffer[i] != '\"')
						data[datalen++] = buffer[i];
					else
						break;
				}
	
				if (!InMsg && buffer[i] == '\"')
					InMsg = true;
			}

			data[1] = datalen - 5;
			
			// dbprintf("ViVOpayLoadMessages: STORE_LCD Command");
			if (ViVOtechGenerateCommand(STORE_LCD, sendCmd, &sendCmdLength, data, datalen) == 0)
			{
				respCmdLength = sizeof(respCmd);
				if (ctlsCommand(true, 2, sendCmd, sendCmdLength, respCmd, &respCmdLength) == 0)
				{
					if (!(ctls_vivotech_cmd[STORE_LCD].command == respCmd[10] && respCmd[11] == 0))
					{
						// dbprintf("ViVOpayLoadMessages: failed");
						return -1;
					}
				}
			}
		}
	}

	// dbprintf("ViVOpayLoadMessages: messages set");
	return 0;
}

int ViVOpayGetMessages(void)
{
	return 1;
}

int ViVOpayLoadConfig(void)
{
	char FileName[100];
	int fh;
	unsigned char sendCmd[1024];
	int sendCmdLength;
	unsigned char respCmd[1024];
	int respCmdLength;
	unsigned char data[1024];
	int datalen;
	int MsgScheme;
	char buffer[1024];
	int i;

	// dbprintf("ViVOpayLoadConfig: in...");

	// TODO: change parameter to CTLSMODE? do we need to have VIVO and WAVE3 configs now as we have to do both anyway
	//Dwarika .. for testing
	//MsgScheme = intGetEnv("CTLSVIVOPAY", 0);
	MsgScheme = 0;
	switch (MsgScheme)
	{
		case 0:
			strcpy(FileName, "ctlsemvconfig_vivotech.txt");
			break;
		case 1:
			strcpy(FileName, "ctlsemvconfig_wave3.txt");
			break;
		default:
			return 0;
	}

	// dbprintf("ViVOpayLoadConfig: scheme %d, %s", MsgScheme, FileName);

	// TODO: errors returning 0 and not -1???
	if ((fh = open(FileName, O_RDONLY)) == -1) {
		return 0;
	}

	while (readCtlsRecord(fh, buffer, false) >= 0)
	{
		// find the end of the data
		for (i = 0; i < strlen(buffer) && isxdigit(buffer[i]); i++) ;
		buffer[i] = 0;

		// do not process blank lines or comment lines
		if (strlen(buffer) == 0)
			continue;

		// convert tag and data to hex
		memset(data, 0, sizeof(data));
		datalen = strlen(buffer)/2;
		SVC_DSP_2_HEX(buffer, (char *)data, datalen);

		// dbprintf("ViVOpayLoadConfig: SET_EMV_CONFIG command");
		if (ViVOtechGenerateCommand(SET_EMV_CONFIG, sendCmd, &sendCmdLength, data, datalen) == 0)
		{
			respCmdLength = sizeof(respCmd);
			if (ctlsCommand(true, 2, sendCmd, sendCmdLength, respCmd, &respCmdLength) == 0)
			{
				if (!(ctls_vivotech_cmd[SET_EMV_CONFIG].command == respCmd[10] && respCmd[11] == 0))
				{
					// failed
					// dbprintf("ViVOpayLoadConfig: load failed");
					return -1;
				}
			}
		}
	}

	// dbprintf("ViVOpayLoadConfig: config set");
	return 0;
}

int ViVOpayGetConfig(void)
{
//#ifdef LOGSYS_FLAG
#if 1
	unsigned char sendCmd[1024];
	int sendCmdLength = 0;
	unsigned char respCmd[1024];
	int respCmdLength = sizeof(respCmd);
	int offset;
	int tag, taglength;


	char buffer[2048] = "EMV CONFIG:";
	char tagresult[1024], tagname[10];
	int bufferlen = strlen(buffer);

	// dbprintf("ViVOpayGetConfig: GET_EMV_CONFIG Command");
	if (ViVOtechGenerateCommand(GET_EMV_CONFIG, sendCmd, &sendCmdLength, null, 0) == 0)
	{
		if (ctlsCommand(true, 5, sendCmd, sendCmdLength, respCmd, &respCmdLength) == 0)
		{
			//Check response is okay
			if ((ctls_vivotech_cmd[GET_EMV_CONFIG].command == respCmd[10]) &&
				(respCmd[11] == 0))
			{
				offset = 14;
				while (offset < (respCmdLength - 2))
				{
					unsigned char *Pos;

					tag = respCmd[offset];
					Pos = &respCmd[offset++];

					if (tag == 0x1F || tag == 0x5F || tag == 0x9F ||tag == 0xDF || tag == 0xFF)
						tag  = (tag << 8) + respCmd[offset++];
					
					taglength = respCmd[offset++];

					if (tag == 0x9F1B)
						taglength = 4;
					
					if (tag > 0xFF)
						SVC_HEX_2_DSP((char *)Pos, tagname, 2);
					else
						SVC_HEX_2_DSP((char *)Pos, tagname, 1);

					SVC_HEX_2_DSP((char *)&respCmd[offset], tagresult, taglength);
					tagresult[taglength * 2] = 0;
					sprintf(&buffer[bufferlen], "[%s]: [%s]", tagname, tagresult);
					bufferlen = strlen(buffer);
					offset += taglength;
				}
				////LOG_PRINTFF(0x00000001L,"ViVOpayGetConfig: %s", buffer);
				return 1;
			}
		}
	}
#endif
	return 0;
}


int ViVOpayGetAllConfigAIDs(void)
{
#if 1
	unsigned char sendCmd[1024];
	int sendCmdLength;
	unsigned char respCmd[1024];
	int respCmdLength;
	unsigned char tagresult[2048];

	// dbprintf("ViVOpayLoadAIDS: GET_ALL_AIDS Command");
	if (ViVOtechGenerateCommand(GET_ALL_AIDS, sendCmd, &sendCmdLength, null, 0) == 0)
	{
		respCmdLength = sizeof(respCmd);
		if (ctlsCommand(true, 2, sendCmd, sendCmdLength, respCmd, &respCmdLength) == 0)
		{
			if (!(ctls_vivotech_cmd[GET_ALL_AIDS].command == respCmd[10] && respCmd[11] == 0))
			{
				// failed
				return -1;
			}
			//SVC_HEX_2_DSP((char *)respCmd, tagresult, respCmdLength);
		}
	}
#endif
	return 0;
}

int ViVOpayGetAllConfigGroups(void)
{
//#ifdef LOGSYS_FLAG
#if 1
	unsigned char sendCmd[1024];
	int sendCmdLength;
	unsigned char respCmd[1024];
	unsigned char tagresult[2048];
	int respCmdLength;

	// dbprintf("ViVOpayLoadAIDS: GET_ALL_GRPS Command");
	if (ViVOtechGenerateCommand(GET_ALL_GRPS, sendCmd, &sendCmdLength, null, 0) == 0)
	{
		respCmdLength = sizeof(respCmd);
		if (ctlsCommand(true, 2, sendCmd, sendCmdLength, respCmd, &respCmdLength) == 0)
		{
			if (!(ctls_vivotech_cmd[GET_ALL_GRPS].command == respCmd[10] && respCmd[11] == 0))
			{
				// failed
				return -1;
			}
			//SVC_HEX_2_DSP((char *)respCmd, tagresult, respCmdLength);
		}
	}
#endif
	return 0;
}


int ViVOpayProcessAIDTable(void)
{
	#if 0
	unsigned char sendCmd[1024];
	int sendCmdLength = 0;
	unsigned char respCmd[1024];
	int respCmdLength = sizeof(respCmd);
	char *pos;
	int offset;
	int tag, taglength;
	int TLVLength;
	AID_DATA AIDList[15];
	unsigned char RIDList[10][15];
	unsigned char CAPKList[100][20];
	int MaxAIDs, MaxRIDs, MaxCAPKs;
	int index = 0, index2, index3;
	unsigned char data[1024];
	int datalen;
	char AIDFileBuffer[5120];
	long AIDFileLen;
	int found = false;
	char temp[1024], temp2[1024], temp3[1024];

	// dbprintf("ViVOpayProcessAIDTable: GET_ALL_AIDS Command");
	if (ViVOtechGenerateCommand(GET_ALL_AIDS, sendCmd, &sendCmdLength, null, 0) == 0)
	{
		if (ctlsCommand(true, 5, sendCmd, sendCmdLength, respCmd, &respCmdLength) == 0)
		{
			//Check response is okay
			if ((ctls_vivotech_cmd[GET_ALL_AIDS].command == respCmd[10]) &&
				(respCmd[11] == 0))
			{
				offset = 14;
				index = 0;
				TLVLength = (respCmd[12] * 256) + respCmd[13];

				while (offset < TLVLength)
				{
					tag = respCmd[offset++];

					if (tag == 0x1F || tag == 0x5F || tag == 0x9F ||tag == 0xDF || tag == 0xFF)
						tag  = (tag << 8) + respCmd[offset++];
					
					taglength = respCmd[offset++];

					if (tag == 0x9F1B)
						taglength = 4;
					
					if (tag == 0xFFE4)
					{
						AIDList[index].GroupNo = respCmd[offset];
					}

					if (tag == 0x9F06)
					{
						memcpy(&AIDList[index].AID[0], &respCmd[offset], taglength);
						AIDList[index].AIDLength = taglength;
//						vdPrintLargeHexData("Found AID:", AIDList[index].AID, AIDList[index].AIDLength);
						index++;
					}

					offset += taglength;
				}
			}
		}
	}

	MaxAIDs = index;

	// Disable All AIDS
	for (index = 0; index < MaxAIDs; index++)
	{
		unsigned char SetCfgAIDMsgStart[] = {0xFF, 0xE4, 0x01, 0x00, 0x9F, 0x06, 0x00};
		unsigned char SetCfgAIDMsgEnd[] = {0xFF, 0xE6, 0x01, 0x80};

		// set group to 0
		memset(data, 0x00, sizeof(data));
		memcpy(data, SetCfgAIDMsgStart, sizeof(SetCfgAIDMsgStart));
		data[6] = AIDList[index].AIDLength;
		memcpy(&data[7], AIDList[index].AID, AIDList[index].AIDLength);
		datalen = 7 + AIDList[index].AIDLength;
		memcpy(&data[datalen], SetCfgAIDMsgEnd, sizeof(SetCfgAIDMsgEnd));
		datalen += sizeof(SetCfgAIDMsgEnd);
		AIDList[index].Enabled = false;

		// dbprintf("ViVOpayProcessAIDTable: SET_CFG_AID Command");
		if (ViVOtechGenerateCommand(SET_CFG_AID, sendCmd, &sendCmdLength, data, datalen) == 0)
		{
			respCmdLength = sizeof(respCmd);
			if (ctlsCommand(true, 5, sendCmd, sendCmdLength, respCmd, &respCmdLength) == 0)
			{
				//Check response is okay
				if ((ctls_vivotech_cmd[SET_CFG_AID].command != respCmd[10]) ||
					(respCmd[11] != 0))
				{
					continue;
				}
			}
		}
	}

	// dbprintf("ViVOpayProcessAIDTable: Search for AIDS in xml file");
	// Search for AIDs in AID s List
	// ensure that the card prefix table has been signed
	if ((dir_get_attributes("ctlsaidtable.xml") & ATTR_NOT_AUTH) != 0)
		return -1;

	AIDFileLen = sizeof(AIDFileBuffer);
	//Dwarika .. for testing
	if (read_file("ctlsaidtable.xml", AIDFileBuffer, &AIDFileLen) < 0)
		return -1;

	// find the root tag and check this is the CTLS AID Table file
	//Dwarika .. for testing
	//if ((pos = xmlRootElementTag(AIDFileBuffer)) == NULL)
	if (xmlRootElementTag(AIDFileBuffer) == 0)
		return -1;

	if (memcmp(pos, "CTLSAIDTable", 12) != 0)
		return -1;

	pos += 13;
	while (true)
	{	
		found = false;
		//Dwarika .. for testing
		//if ((pos = xmlFindElementTag("CTLS", pos)) == null)
		if (xmlFindElementTag("CTLS", pos) == 0)
			break; // reached end

		pos += 4;
		//Dwarika .. for testing
		if (!xmlGetDataStr("AID", pos, temp, sizeof(temp)))
			break; // format error

		pos += strlen(temp);
		SVC_DSP_2_HEX(temp, temp2, strlen(temp));

		for (index = 0; index < MaxAIDs && !found; index++)
		{
			if (memcmp(temp2, AIDList[index].AID, AIDList[index].AIDLength) == 0)
			{
				found = true;
				AIDList[index].Enabled = true;
				AIDList[index].TranLimitExists = false;
				AIDList[index].FloorLimitExists = false;
				AIDList[index].CVMReqLimitExists = false;
				AIDList[index].TermCapCVMReqExists = false;
				if (xmlGetDataStr("GroupID", pos, temp, sizeof(temp)))
				{
					pos += strlen(temp);
					AIDList[index].GroupNo = atoi(temp);
				}

				if (xmlGetDataStr("TranLimit", pos, temp, sizeof(temp)))
				{
					char padding[] = "000000000000";
					int translimit;
					
					translimit = atoi(temp);
					if (translimit > ctls_maxtranslimit)
						ctls_maxtranslimit = translimit;

					pos += strlen(temp);
					strcpy(&padding[strlen(padding) - strlen(temp)], temp);

					SVC_DSP_2_HEX(padding, temp3, sizeof(padding));
					memcpy((char *)AIDList[index].TranLimit, temp3, sizeof(padding)/2);
					AIDList[index].TranLimitExists = true;
				}

				if (xmlGetDataStr("FloorLimit", pos, temp, sizeof(temp)))
				{
					char strFloorLimit[8 + 1];

					sprintf(strFloorLimit, "%08X", atoi(temp));
					pos += strlen(temp);
					SVC_DSP_2_HEX(strFloorLimit, temp3, strlen(strFloorLimit));
					memcpy((char *)AIDList[index].FloorLimit, temp3, strlen(strFloorLimit)/2);
					AIDList[index].FloorLimitExists = true;
				}

				if (xmlGetDataStr("CVMRequiredLimit", (char *)pos, temp, sizeof(temp)))
				{
					char padding[] = "000000000000";
					
					pos += strlen(temp);
					strcpy(&padding[strlen(padding) - strlen(temp)], temp);
					SVC_DSP_2_HEX(padding, temp3, sizeof(padding));
					memcpy((char *)AIDList[index].CVMReqLimit, temp3, sizeof(padding)/2);
					AIDList[index].CVMReqLimitExists = true;
				}

				if (xmlGetDataStr("TermCapabilities", (char *)pos, temp, sizeof(temp)))
				{
					pos += strlen(temp);
					SVC_DSP_2_HEX(temp, temp3, strlen(temp));
					memcpy((char *)AIDList[index].TermCap, temp3, strlen(temp)/2);
				}

				if (xmlGetDataStr("TermCapabilitiesNoCVMReq", pos, temp, sizeof(temp)))
				{
					pos += strlen(temp);
					SVC_DSP_2_HEX(temp, temp3, strlen(temp));
					memcpy((char *)AIDList[index].TermCapNoCVMReq, temp3, strlen(temp)/2);

					if (xmlGetDataStr("TermCapabilitiesCVMReq", (char *)pos, temp, sizeof(temp)))
					{
						pos += strlen(temp);
						SVC_DSP_2_HEX(temp, temp3, strlen(temp));
						memcpy((char *)AIDList[index].TermCapCVMReq, temp3, strlen(temp)/2);
						AIDList[index].TermCapCVMReqExists = true;
						// dbprintf("ViVOpayProcessAIDTable: found TermCapabilitiesNoCVMReq");
					}
				}

				if (xmlGetDataStr("AddTermCapabilities", (char *)pos, temp, sizeof(temp)))
				{
					pos += strlen(temp);
					SVC_DSP_2_HEX(temp, temp3, strlen(temp));
					memcpy((char *)AIDList[index].AddTermCap, temp3, strlen(temp)/2);
				}

				if (xmlGetDataStr("DefaultTDOL", (char *)pos, temp, sizeof(temp)))
				{
					pos += strlen(temp);
					SVC_DSP_2_HEX(temp, temp3, strlen(temp));
					AIDList[index].DefaultTDOLLen = strlen(temp) / 2 ;
					memcpy((char *)AIDList[index].DefaultTDOL, temp3, AIDList[index].DefaultTDOLLen);
				}

				if (xmlGetDataStr("TTQ", (char *)pos, temp, sizeof(temp)))
				{
					pos += strlen(temp);
					SVC_DSP_2_HEX(temp, temp3, strlen(temp));
					memcpy((char *)AIDList[index].TTQ, temp3, strlen(temp)/2);
				}

				if (xmlGetDataStr("TACOnline", (char *)pos, temp, sizeof(temp)))
				{
					pos += strlen(temp);
					SVC_DSP_2_HEX(temp, temp3, strlen(temp));
					memcpy((char *)AIDList[index].TACOnline, temp3, strlen(temp)/2);
				}

				if (xmlGetDataStr("TACDefault", (char *)pos, temp, sizeof(temp)))
				{
					pos += strlen(temp);
					SVC_DSP_2_HEX(temp, temp3, strlen(temp));
					memcpy((char *)AIDList[index].TACDefault, temp3, strlen(temp)/2);
				}

				if (xmlGetDataStr("TACDenial", (char *)pos, temp, sizeof(temp)))
				{
					pos += strlen(temp);
					SVC_DSP_2_HEX(temp, temp3, strlen(temp));
					memcpy(AIDList[index].TACDenial, temp3, strlen(temp)/2);
				}

//				vdPrintLargeHexData("AID data:", AIDList[index].AID, 250);
			}
		}
	}
	
	// Delete All Configurable Group
	// dbprintf("ViVOpayProcessAIDTable: Delete All Configuration Groups");
	for (index = 1; index < 7; index++)
	{
		unsigned char DelCfgGroupTag[] = {0xFF, 0xE4, 0x01, 0x00};
		memset(data, 0x00, sizeof(data));
		memcpy(data, DelCfgGroupTag, sizeof(DelCfgGroupTag));
		data[3] = index;
		datalen = sizeof(DelCfgGroupTag);

		// dbprintf("ViVOpayProcessAIDTable: DEL_CFG_GROUP Command");
		if (ViVOtechGenerateCommand(DEL_CFG_GROUP, sendCmd, &sendCmdLength, data, datalen) == 0)
		{
			respCmdLength = sizeof(respCmd);
			if (ctlsCommand(true, 5, sendCmd, sendCmdLength, respCmd, &respCmdLength) == 0)
			{
				//Check response is okay
				if ((ctls_vivotech_cmd[DEL_CFG_GROUP].command != respCmd[10]) ||
					(respCmd[11] != 0))
				{
					// dbprintf("ViVOpayProcessAIDTable: DEL_CFG_GROUP Command Error");
					continue;
				}
			}
		}
	}

	// Create Group and set tags
	// dbprintf("ViVOpayProcessAIDTable: Create Configuration Groups");
	for (index = 0; index < MaxAIDs; index++)
	{
		// dbprintf("ViVOpayProcessAIDTable: index = %d, Create GroupNo = %d", index, AIDList[index].GroupNo);
	
		if ((AIDList[index].GroupNo != 0) && AIDList[index].Enabled)
		{
			unsigned char SetCfgGrpMsgStart[] = {0xFF, 0xE4, 0x01, 0x00};
			unsigned char TransLimitTag[] = {0xFF, 0xF1, 0x6};
			unsigned char FloorLimitTag[] = {0x9F, 0x1B, 0x4};
			unsigned char CVMReqLimitTag[] = {0xFF, 0xF5, 0x06};
			unsigned char TermCapTag[] = {0x9F, 0x33, 0x03};
			unsigned char TermCapNoCVMReqTag[] = {0xDF, 0x28, 0x03};
			unsigned char TermCapCVMReqTag[] = {0xDF, 0x29, 0x03};
			unsigned char AddTermCapTag[] = {0x9F, 0x40, 0x05};
			unsigned char DefaultTDOLTag[] = {0x97, 0x00};
			unsigned char TTQTag[] = {0x9F, 0x66, 0x04};
			unsigned char TACOnlineTag[] = {0xFF, 0xFD, 0x05};
			unsigned char TACDefaultTag[] = {0xFF, 0xFE, 0x05};
			unsigned char TACDenialTag[] = {0xFF, 0xFF, 0x05};

			memset(data, 0x00, sizeof(data));
			memcpy(data, SetCfgGrpMsgStart, sizeof(SetCfgGrpMsgStart));
			data[3] = AIDList[index].GroupNo;
			datalen = sizeof(SetCfgGrpMsgStart);

			if (AIDList[index].TranLimitExists)
			{
				memcpy(&data[datalen], TransLimitTag, sizeof(TransLimitTag));
				memcpy(&data[datalen + sizeof(TransLimitTag)], AIDList[index].TranLimit, 6);
				datalen += 9;
			}

			if (AIDList[index].FloorLimitExists)
			{
				memcpy(&data[datalen], FloorLimitTag, sizeof(FloorLimitTag));
				memcpy(&data[datalen + sizeof(FloorLimitTag)], AIDList[index].FloorLimit, 4);
				datalen += 7;
			}

			if (AIDList[index].CVMReqLimitExists)
			{
				memcpy(&data[datalen], CVMReqLimitTag, sizeof(CVMReqLimitTag));
				memcpy(&data[datalen + sizeof(CVMReqLimitTag)], AIDList[index].CVMReqLimit, 6);
				datalen += 9;
			}

			memcpy(&data[datalen], TermCapTag, sizeof(TermCapTag));
			memcpy(&data[datalen + sizeof(TermCapTag)], AIDList[index].TermCap, 3);
			datalen += 6;

			if (AIDList[index].TermCapCVMReqExists)
			{
				memcpy(&data[datalen], TermCapNoCVMReqTag, sizeof(TermCapNoCVMReqTag));
				memcpy(&data[datalen + sizeof(TermCapNoCVMReqTag)], AIDList[index].TermCapNoCVMReq, 3);
				datalen += 6;

				memcpy(&data[datalen], TermCapCVMReqTag, sizeof(TermCapCVMReqTag));
				memcpy(&data[datalen +  sizeof(TermCapCVMReqTag)], AIDList[index].TermCapCVMReq, 3);
				datalen += 6;
			}

			memcpy(&data[datalen], AddTermCapTag, sizeof(AddTermCapTag));
			memcpy(&data[datalen + sizeof(AddTermCapTag)], AIDList[index].AddTermCap, 5);
			datalen += 8;

			memcpy(&data[datalen], DefaultTDOLTag, sizeof(DefaultTDOLTag));
			data[datalen + sizeof(DefaultTDOLTag) - 1] = AIDList[index].DefaultTDOLLen;
			memcpy(&data[datalen + sizeof(DefaultTDOLTag)], AIDList[index].DefaultTDOL,
				AIDList[index].DefaultTDOLLen);
			datalen += (sizeof(DefaultTDOLTag) + AIDList[index].DefaultTDOLLen);

			memcpy(&data[datalen], TTQTag, sizeof(TTQTag));
			memcpy(&data[datalen + sizeof(TTQTag)], AIDList[index].TTQ, 4);
			datalen += 7;
				
			memcpy(&data[datalen], TACOnlineTag, sizeof(TACOnlineTag));
			memcpy(&data[datalen + sizeof(TACOnlineTag)], AIDList[index].TACOnline, 5);
			datalen += 8;
				
			memcpy(&data[datalen], TACDefaultTag, sizeof(TACDefaultTag));
			memcpy(&data[datalen + sizeof(TACDefaultTag)], AIDList[index].TACDefault, 5);
			datalen += 8;

			memcpy(&data[datalen], TACDenialTag, sizeof(TACDenialTag));
			memcpy(&data[datalen + sizeof(TACDenialTag)], AIDList[index].TACDenial, 5);
			datalen += 8;

			// dbprintf("ViVOpayProcessAIDTable: SET_CFG_GROUP Command");
			if (ViVOtechGenerateCommand(SET_CFG_GROUP, sendCmd, &sendCmdLength, data, datalen) == 0)
			{
				respCmdLength = sizeof(respCmd);
				if (ctlsCommand(true, 5, sendCmd, sendCmdLength, respCmd, &respCmdLength) == 0)
				{
					//Check response is okay
					if ((ctls_vivotech_cmd[SET_CFG_GROUP].command != respCmd[10]) ||
						(respCmd[11] != 0))
					{
						return -1;
					}
				}
			}
		}
	}

	// dbprintf("ViVOpayProcessAIDTable: Enable AIDs");

	// Enable AIDs in System AID
	index2 = 0;
	for (index = 0; index < MaxAIDs; index++)
	{
		if (AIDList[index].Enabled)
		{
			unsigned char SetAIDMsgStart[] = {0xFF, 0xE4, 0x01, 0x00, 0x9F, 0x06, 0x00};
			unsigned char ExtraTags[] = 
				{0xFF, 0xE5, 0x01, 0x10,	// Maximum AID Length
				0xFF, 0xE1, 0x01, 0x01};	// Partial Selection Allowed

			memset(data, 0x00, sizeof(data));
			memcpy(data, SetAIDMsgStart, sizeof(SetAIDMsgStart));
			data[3] = AIDList[index].GroupNo;
			data[6] = AIDList[index].AIDLength;
			memcpy(&data[sizeof(SetAIDMsgStart)], AIDList[index].AID, AIDList[index].AIDLength);
			datalen = sizeof(SetAIDMsgStart) + AIDList[index].AIDLength;

			memcpy(&data[datalen], ExtraTags, sizeof(ExtraTags));
			datalen += sizeof(ExtraTags);

			// dbprintf("ViVOpayProcessAIDTable: SET_CFG_AID Command");
			if (ViVOtechGenerateCommand(SET_CFG_AID, sendCmd, &sendCmdLength, data, datalen) == 0)
			{
				respCmdLength = sizeof(respCmd);
				if (ctlsCommand(true, 5, sendCmd, sendCmdLength, respCmd, &respCmdLength) == 0)
				{
					//Check response is okay
					if ((ctls_vivotech_cmd[SET_CFG_AID].command != respCmd[10]) ||
						(respCmd[11] != 0))
					{
						// dbprintf("ViVOpayProcessAIDTable: SET_CFG_AID Command Error");
						continue;
					}
					else
					{
						// Get the RID and check it does not already exist
						SVC_HEX_2_DSP((char *)AIDList[index].AID, temp, 5);
						found = false;
						for (index3 = 0; index3 < index2 && !found; index3++)
						{
							if (strcmp((char *)RIDList[index3], temp) == 0)
								found = true;
						}

						if (!found);
						{
							strcpy((char *)RIDList[index2], temp);
							index2++;
						}
						// dbprintf("ViVOpayProcessAIDTable: set rid[%d]=%s", index2, temp);
					}
				}
			}
		}
	}

	MaxRIDs = index2;

	// Delete All CPAK Command
	// dbprintf("ViVOpayProcessAIDTable: DEL_ALL_CAPK Command");
	if (ViVOtechGenerateCommand(DEL_ALL_CAPK, sendCmd, &sendCmdLength, null, 0) == 0)
	{
		respCmdLength = sizeof(respCmd);
		if (ctlsCommand(true, 5, sendCmd, sendCmdLength, respCmd, &respCmdLength) == 0)
		{
			//Check response is okay
			if ((ctls_vivotech_cmd[DEL_ALL_CAPK].command != respCmd[10]) ||
				(respCmd[9] != 'A') || (respCmd[11] != 0))
			{
				// dbprintf("ViVOpayManageCAPKs: DEL_ALL_CAPK - error");
			}
		}
	}
	
	// dbprintf("ViVOpayProcessAIDTable: Get all CAPKs, MaxRIDs = %d", MaxRIDs);
	// Get all CAPKs from the EST file using the RID List
	index = 0;
	index3 = 0;
	while (inLoadESTRec(index) == 0)
	{
		szGetESTRID(temp);
		// dbprintf("ViVOpayProcessAIDTable: Loaded inLoadESTRec[%d]=%s", index, temp);
		for (index2 = 0; index2 < MaxRIDs; index2++)
		{
			// dbprintf("ViVOpayProcessAIDTable: Compare RIDList[%d]=%s",index2, RIDList[index2]);
			if (strcmp(temp, (char *)RIDList[index2]) == 0)
			{
				szGetESTCAPKFile1(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE1 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile2(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE2 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile3(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE3 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile4(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE4 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile5(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE5 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile6(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE6 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile7(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE7 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile8(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE8 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile9(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE9 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile10(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE10 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile11(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE11 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile12(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE12 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile13(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE13 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile14(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE14 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile15(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE15 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}
			}
		}
		index++;
	}

	MaxCAPKs = index3;

	// Add all the desired CAPKs
	// dbprintf("ViVOpayProcessAIDTable: Add all desired CAPKs, MaxCAPs = %d", MaxCAPKs);
	for (index = 0; index < MaxCAPKs; index++)
	{
		int fp;
		long filesize;
		unsigned char rid[10], idx[10], mod[1000], exp[10], chk[100];
		unsigned int modlen, explen;
		unsigned char capkdata[1000];
		unsigned char reqdata1[300], reqdata2[300];
		int reqdata1len, reqdata2len;

		reqdata1len = 0;
		reqdata2len = 0;
		memset(temp, 0x00, sizeof(temp));
		memset(temp2, 0x00, sizeof(temp));
		memset(temp3, 0x00, sizeof(temp));
		// dbprintf("ViVOpayProcessAIDTable: LoadingCAPk[%d]=%s", index, CAPKList[index]);
		
		strcpy(temp, (char*)CAPKList[index]);
		if ((fp = open(temp, O_RDONLY)) == -1)
			continue;
		if (get_file_size(fp, &filesize) != 0)
			return -1;
		if ((filesize = read(fp, temp2, filesize)) == -1)
			return -1;
		temp2[filesize] = 0;
		close(fp);

		// extract all required data
		SVC_DSP_2_HEX(temp, (char *)rid, strlen(temp) - 3);
		SVC_DSP_2_HEX(&temp[strlen(temp) - 2], (char *)idx, 2);
		
		strncpy(temp3, temp2, 3);
		temp3[3] = 0;
		modlen = atoi(temp3);

		SVC_DSP_2_HEX(&temp2[3], (char *)mod, modlen * 2);
		strncpy(temp3, &temp2[3 + modlen * 2], 2);
		temp3[2] = 0;
		explen = atoi(temp3);

		SVC_DSP_2_HEX(&temp2[5 + modlen * 2], (char *)exp, explen * 2);
		SVC_DSP_2_HEX(&temp2[5 + modlen * 2 + explen * 2], (char *)chk, 40);

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
		
		if (ViVOtechGenerateKeyMgmtCommand(SET_CAPK,  sendCmd, &sendCmdLength, capkdata, 34 + modlen,
			reqdata1, &reqdata1len, reqdata2, &reqdata2len) == 0)
		{
			// dbprintf("ViVOpayProcessAIDTable: SET_CAPK Command");
			respCmdLength = sizeof(respCmd);
			if (ctlsCommand(true, 5, sendCmd, sendCmdLength, respCmd, &respCmdLength) == 0)
			{
				//Check response is okay
				if ((ctls_vivotech_cmd[SET_CAPK].command != respCmd[10]) ||
					(respCmd[9] != 'A') || (respCmd[11] != 0))
				{
					continue;
				}
			}

			// dbprintf("ViVOpayProcessAIDTable: SET_CAPK Data 1 Command");
			respCmdLength = sizeof(respCmd);
			if (ctlsCommand(true, 5, reqdata1, reqdata1len, respCmd, &respCmdLength) == 0)
			{
				//Check response is okay
				if ((ctls_vivotech_cmd[SET_CAPK].command != respCmd[10]) ||
					(respCmd[9] != 'A') || (respCmd[11] != 0))
				{
					continue;
				}
			}

			if (reqdata2len > 0)
			{
				// dbprintf("ViVOpayProcessAIDTable: SET_CAPK Data 2 Command");
				respCmdLength = sizeof(respCmd);
				if (ctlsCommand(true, 5, reqdata2, reqdata2len, respCmd, &respCmdLength) == 0)
				{
					//Check response is okay
					if ((ctls_vivotech_cmd[SET_CAPK].command != respCmd[10]) ||
						(respCmd[9] != 'A') || (respCmd[11] != 0))
					{
						continue;
					}
				}
			}
		}
	}
	#endif

	return 0;
}


////////////////////////////////////////////////////////////////////////////////

void  dbprintf_hexdump(char *prefix, unsigned char *data, int length)
{
	char buffer[4096];

	if (strlen(prefix) + length * 2 >= 4096)
		length = (4096 - strlen(prefix) - 1)/2;

	strcpy(buffer, prefix);
	//hex_to_ascii(data, &buffer[strlen(prefix)], length);

	// dbprintf(buffer);
}

//
//
//

int ctlsCommand(int vivopay, int timeout, unsigned char *req, int reqlen, unsigned char *resp, int *resplen)
{
	int ret;
	unsigned long expiry;
	int complete = false;
	int length = 0;
	int totallength = 0;
	char buffer[1024];

	// validate request command
	if (req == null || reqlen == 0)
		return -1;

	// validate reponse buffer
	if (resp == null || resplen == null)
		return -1;

	ret = CTLSSend((char *)req, reqlen);
	if (ret < 0)
	{
		// failed to send command
		return ret;
	}

	expiry = read_ticks() + timeout * 1000;
	while (!complete && read_ticks() < expiry)
	{
		// read response data
		totallength = *resplen - length;

		ret = CTLSReceive((char *)&resp[length], totallength);
		if (ret < 0)
		{
			return ret;
		}
		length += ret;

		// check if command is complete
		if (vivopay)
			complete = ViVOpayCheckResponse(resp, length);
		else
			if (length != 0 && resp[length - 1] == 0x03)
				complete = Wave3CheckResponse(resp, length);

		if (complete < 0)
		{
			// failed to read valid response
			return -1;
		}

	//Dwarika .. for testing
	//	process_events(100);
	}

	if (length <= 0)
	{
		// failed to obtain response
		// dbprintf("No response from CTLS");
		*resplen = 0;
		return -1;
	}
	// dbprintf_hexdump("RESP FROM CTLS: ", resp, length);
	*resplen = length;
	return 0;
}

int ctlsPending(void)
{
	return 1;
}

int ctlsSend(unsigned char *req, int reqlen)
{
	int ret;
	char buffer[1024];
	LOG_PRINTFF(0x00000001L,"ctlsSend ");
	// validate request command
	if (req == null || reqlen == 0)
		return -1;

	// ensure there are no messages in the port
	while(CTLSReceive((char *)&buffer, sizeof(buffer)) > 0);
	LOG_PRINTFF(0x00000001L,"ctlsSend 1");
	ret = CTLSSend((char *)req, reqlen);
	LOG_PRINTFF(0x00000001L,"ctlsSend 2");
	if (ret <= 0)
	{
		// failed to send command
		return -1;
	}

	return 0;
}


int nProcessESTFile(void)
{
	unsigned char sendCmd[1024];
	int sendCmdLength = 0;
	unsigned char respCmd[1024];
	int respCmdLength = sizeof(respCmd);
	char *pos;
	int offset;
	int tag, taglength;
	int TLVLength;
	AID_DATA AIDList[15];
	unsigned char RIDList[10][15];
	unsigned char CAPKList[100][20];
	int MaxAIDs, MaxRIDs, MaxCAPKs;
	int index = 0, index2, index3;
	unsigned char data[1024];
	int datalen;
	char AIDFileBuffer[5120];
	long AIDFileLen;
	int found = false;
	char temp[1024], temp2[1024], temp3[1024];
	
	index = 0;
	index3 = 0;
	MaxRIDs = 4;
	while (inLoadESTRec(index) == 0)
	{
		LOG_PRINTFF(0x00000001L,"nProcessESTFile 1");
		szGetESTRID(temp);
		// dbprintf("ViVOpayProcessAIDTable: Loaded inLoadESTRec[%d]=%s", index, temp);
		LOG_PRINTFF(0x00000001L,"Loaded inLoadESTRec[%d]=%s", index, temp);
		for (index2 = 0; index2 < MaxRIDs; index2++)
		{
			// dbprintf("ViVOpayProcessAIDTable: Compare RIDList[%d]=%s",index2, RIDList[index2]);
			LOG_PRINTFF(0x00000001L,"Compare RIDList[%d]=%s",index2, RIDList[index2]);
			//if (strcmp(temp, (char *)RIDList[index2]) == 0)
			{
				szGetESTCAPKFile1(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE1 = %s", temp);
					LOG_PRINTFF(0x00000001L,"CAPKFILE1 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile2(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE2 = %s", temp);
					LOG_PRINTFF(0x00000001L,"CAPKFILE2 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile3(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE3 = %s", temp);
					LOG_PRINTFF(0x00000001L,"CAPKFILE3 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile4(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE4 = %s", temp);
					LOG_PRINTFF(0x00000001L,"CAPKFILE4 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile5(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE5 = %s", temp);
					LOG_PRINTFF(0x00000001L,"CAPKFILE5 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile6(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE6 = %s", temp);
					LOG_PRINTFF(0x00000001L,"CAPKFILE6 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile7(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE7 = %s", temp);
					LOG_PRINTFF(0x00000001L,"CAPKFILE7 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile8(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE8 = %s", temp);
					LOG_PRINTFF(0x00000001L,"CAPKFILE8 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile9(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE9 = %s", temp);
					LOG_PRINTFF(0x00000001L,"CAPKFILE9 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile10(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE10 = %s", temp);
					LOG_PRINTFF(0x00000001L,"CAPKFILE10 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile11(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE11 = %s", temp);
					LOG_PRINTFF(0x00000001L,"CAPKFILE11 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile12(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE12 = %s", temp);
					LOG_PRINTFF(0x00000001L,"CAPKFILE12 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile13(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE13 = %s", temp);
					LOG_PRINTFF(0x00000001L,"CAPKFILE13 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile14(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE14 = %s", temp);
					LOG_PRINTFF(0x00000001L,"CAPKFILE14 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}

				szGetESTCAPKFile15(temp);
				if (strlen(temp)> 0)
				{
					// dbprintf("ViVOpayProcessAIDTable: CAPKFILE15 = %s", temp);
					LOG_PRINTFF(0x00000001L,"CAPKFILE15 = %s", temp);
					strcpy((char *)CAPKList[index3++], temp);
				}
			}
		}
		index++;
	}

	MaxCAPKs = index3;
LOG_PRINTFF(0x00000001L,"nProcessESTFile 2 MaxCAPKs:%d,index3:%d",MaxCAPKs,index3);
	// Add all the desired CAPKs
	// dbprintf("ViVOpayProcessAIDTable: Add all desired CAPKs, MaxCAPs = %d", MaxCAPKs);
	for (index = 0; index < MaxCAPKs; index++)
	{
		int fp;
		long filesize;
		unsigned char rid[10], idx[10], mod[1000], exp[10], chk[100];
		unsigned int modlen, explen;
		unsigned char capkdata[1000];
		unsigned char reqdata1[300], reqdata2[300];
		int reqdata1len, reqdata2len;

		reqdata1len = 0;
		reqdata2len = 0;
		memset(temp, 0x00, sizeof(temp));
		memset(temp2, 0x00, sizeof(temp));
		memset(temp3, 0x00, sizeof(temp));
		// dbprintf("ViVOpayProcessAIDTable: LoadingCAPk[%d]=%s", index, CAPKList[index]);
		LOG_PRINTFF(0x00000001L,"nProcessESTFile 3");
		LOG_PRINTFF(0x00000001L,"LoadingCAPk[%d]=%s", index, CAPKList[index]);
		strcpy(temp, (char*)CAPKList[index]);
		if ((fp = open(temp, O_RDONLY)) == -1)
			continue;
		if (get_file_size(fp, &filesize) != 0)
			return -1;
		if ((filesize = read(fp, temp2, filesize)) == -1)
			return -1;
		temp2[filesize] = 0;
		close(fp);

		// extract all required data
		SVC_DSP_2_HEX(temp, (char *)rid, strlen(temp) - 3);
		SVC_DSP_2_HEX(&temp[strlen(temp) - 2], (char *)idx, 2);
		
		strncpy(temp3, temp2, 3);
		temp3[3] = 0;
		modlen = atoi(temp3);

		SVC_DSP_2_HEX(&temp2[3], (char *)mod, modlen * 2);
		strncpy(temp3, &temp2[3 + modlen * 2], 2);
		temp3[2] = 0;
		explen = atoi(temp3);

		SVC_DSP_2_HEX(&temp2[5 + modlen * 2], (char *)exp, explen * 2);
		SVC_DSP_2_HEX(&temp2[5 + modlen * 2 + explen * 2], (char *)chk, 40);

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
		
		if (ViVOtechGenerateKeyMgmtCommand(SET_CAPK,  sendCmd, &sendCmdLength, capkdata, 34 + modlen,
			reqdata1, &reqdata1len, reqdata2, &reqdata2len) == 0)
		{
			// dbprintf("ViVOpayProcessAIDTable: SET_CAPK Command");
			LOG_PRINTFF(0x00000001L,"nProcessESTFile 4");
			respCmdLength = sizeof(respCmd);
			if (ctlsCommand(true, 5, sendCmd, sendCmdLength, respCmd, &respCmdLength) == 0)
			{
				//Check response is okay
				LOG_PRINTFF(0x00000001L,"nProcessESTFile 5");
				if ((ctls_vivotech_cmd[SET_CAPK].command != respCmd[10]) ||
					(respCmd[9] != 'A') || (respCmd[11] != 0))
				{
					continue;
				}
			}

			// dbprintf("ViVOpayProcessAIDTable: SET_CAPK Data 1 Command");
			LOG_PRINTFF(0x00000001L,"nProcessESTFile 6");
			respCmdLength = sizeof(respCmd);
			if (ctlsCommand(true, 5, reqdata1, reqdata1len, respCmd, &respCmdLength) == 0)
			{
				//Check response is okay
				LOG_PRINTFF(0x00000001L,"nProcessESTFile 7");
				if ((ctls_vivotech_cmd[SET_CAPK].command != respCmd[10]) ||
					(respCmd[9] != 'A') || (respCmd[11] != 0))
				{
					continue;
				}
			}

			if (reqdata2len > 0)
			{
				// dbprintf("ViVOpayProcessAIDTable: SET_CAPK Data 2 Command");
				LOG_PRINTFF(0x00000001L,"nProcessESTFile 8");
				respCmdLength = sizeof(respCmd);
				if (ctlsCommand(true, 5, reqdata2, reqdata2len, respCmd, &respCmdLength) == 0)
				{
					//Check response is okay
					LOG_PRINTFF(0x00000001L,"nProcessESTFile 9");
					if ((ctls_vivotech_cmd[SET_CAPK].command != respCmd[10]) ||
						(respCmd[9] != 'A') || (respCmd[11] != 0))
					{
						continue;
					}
				}
			}
		}
	}
	LOG_PRINTFF(0x00000001L,"nProcessESTFile 10");
	return 0;
}

	
int ctlsRead(int vivopay, unsigned char *resp, int *resplen)
{
	int ret;
	unsigned long expiry;
	int complete = false;
	int length = 0;
	int totallength = 0;
	char stmp[100];

	// validate reponse buffer
	if (resp == null || resplen == null)
	{
		return -1;
	}

	//expiry = read_ticks() + 250; // allow 250 msec to read the entire command
	expiry = read_ticks() + 16000; // allow 250 msec to read the entire command

	while (!complete && read_ticks() < expiry)
	{
		unsigned char key = 0;
		// read response data
		totallength = *resplen - length;
		ret = CTLSReceive((char *)&resp[length], totallength);

		if (ret < 0)
		{
			// failed to read response
			
			return ret;
		}
    	if(read(STDIN, &key, 1) == 1){
			key &= 0x7F;
			if( key == KEY_CNCL) return(-1);
		}
		length += ret;

		// check if command is complete
		if (vivopay)
			complete = ViVOpayCheckResponse(resp, length);

		if (complete < 0)
		{
			return -1;
		}

		//process_events(100);
	}

	if (length <= 0)
	{
		return -1;
	}
//	vdPrintLargeHexData("RESP FROM CTLS 2:", resp, length);
	*resplen = length;
	return 0;
}


int readCtlsRecord(int fHandle , char *buf, int AllowNewLine)
{
	char readBytes = 0;
	int retVal;
	int i = 0;

	while (readBytes != '\r')
	{
		retVal = read(fHandle, &readBytes, 1);
		if(retVal == 0)
		{
			return -1;
		}
		
		if ((readBytes != '\n' && readBytes != '\r') ||
			(AllowNewLine && readBytes == '\n'))
		{
			if (!(readBytes == '\n' && i==0))   //ignore newline from previous line
				buf[i++] = readBytes;
		}

		if(retVal < 0)
		{
			close(fHandle);
			return -1;
		}
	}

	buf[i] = 0;
	return i;
}

void ClearCardData(void)
{
	memset(&CardData,0,sizeof(CardData));
	/*
	memset(CardData.track1, 0x00, sizeof(CardData.track1));
	memset(CardData.track2, 0x00, sizeof(CardData.track2));
	CardData.clearingreclen = 0;
	CardData.clearingrecindex = 0;
	CardData.datatagindex = 0;
	CardData.track1len = 0;
	CardData.track2len = 0;
	*/
}

long ctlsGetLimit(int LimitType, char *AID)
{
		return (10000);//DEBUG
	#if 0
	char buffer[5120]; // 5K
	long length = sizeof(buffer);
	char temp[128];
	char *pos;
	int AIDlen;

	// dbprintf("ctlsGetLimit: AID is %s", AID);

	// CVM Limit or CTLS Trans Limit
	if (LimitType != 1 && LimitType != 2)
		return -1;

	// ensure that the file is signed
	if ((dir_get_attributes("ctlsaidtable.xml") & ATTR_NOT_AUTH) != 0)
		return -1;

	// load the aids file
	//Dwarika .. for testing
	if (read_file("ctlsaidtable.xml", buffer, &length) < 0)
		return -1;

	// find the root tag and check this is the language file
	//dwarika .. for testing
	//if ((pos = xmlRootElementTag(buffer)) == NULL)
	if (xmlRootElementTag(buffer) == 0)
		return -1;

	if (memcmp(pos, "CTLSAIDTable", 12) != 0)
		return -1;

	pos += 13;

	// find the AID
	while (true)
	{
		//dwarika .. for testing
		//if ((pos = xmlFindElementTag("CTLS", pos)) == null)
		if (xmlFindElementTag("CTLS", pos) == 0)
			return -1; // reached end

		pos += 4;
		if (!xmlGetDataStr("AID", pos, temp, sizeof(temp)))
			return -1; // format error

		AIDlen = strlen(temp);
		if (memcmp(AID, temp, AIDlen) == 0)
		{
			if (LimitType == 1)
			{
				if (!xmlGetDataStr("CVMRequiredLimit", pos, temp, sizeof(temp)))
					return -1; // format error
			}
			else if (LimitType == 2)
			{
				if (!xmlGetDataStr("TranLimit", pos, temp, sizeof(temp)))
					return -1; // format error
			}
			break;
		}
		pos += AIDlen;
	}

	// return field value
	// dbprintf("ctlsGetLimit: Limit is %s", temp);
	return atol(temp);
	#endif
}
////////////////////////////////////////////////////////////////////////////////

int hex_to_int(char c){
        int first = c / 16 - 3;
        int second = c % 16;
        int result = first*10 + second;
        if(result > 9) result--;
        return result;
}

int hex_to_ascii(char c, char d){
        int high = hex_to_int(c) * 16;
        int low = hex_to_int(d);
        return high+low;
}

int InitCTLS(void)
{
	return(ctlsInitialise(4));
}
