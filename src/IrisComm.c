/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       IrisComm.c
**
** DATE CREATED:    
**
** AUTHOR:          
**
** DESCRIPTION:     This module contains the communication functions
**-----------------------------------------------------------------------------
*/

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <svc.h>
#include <svc_net.h>

// for EOS log
#include <eoslog.h>
#include <ceif.h>
#include <vos_ddi_ini.h>
#include <vos_ddi_app_evt.h>
#include "IrisComm.h"
#include "comms.h"
#include "printer.h"
#include "inputmacro.h"


#define MAX_EVENT_NAME_SIZE			30
#define	RET_SUCCESS					0
#define	RET_FAILED					-1
#define TO_CE_OPEN					120000L
#define TO_START					60000L
#define TO_STOP						60000L
#define TO_ITERATION				200
#define RESPONSE_TIMEOUT	1000	
#define	COMMS_E_TIMEOUT		-235
#define	COMMS_E_NO_RESPONSE	-236

int g_inConnected = 0;
int g_inAttached = 0;
int g_inClosed = 1;
static int g_inErrno = 0;
char chSignalPercent [21 + 1] = {'0', '%', 0};
char chSignaldBm [21 + 1] = {'0', 'd', 'B', 'm', 0};
char chSignalRSSI [21 + 1] = {'0', 'R', 'S', 'S', 'I', 0};
unsigned char g_ucExtEventData [CEIF_EVT_DATA_SZ];
int g_NICount;						// Global for number of NWIF supported
unsigned char* g_NIInfo;
int g_GPRSHandle = -1;
static int gSocketHandle = -1;
static stNIInfo g_currMediaInfo;			// Current media stNIInfo
static int g_signal=0;
static int g_nowait=0;

static int DebugPrint (const char*template,...) ;
static int DebugPrint2 (const char*template,...) ;
unsigned int inManageCEEvents (void);
void voTranslateCEevt(int inCEEVT, char* chTranslatedEvent);
int connect_nonblock(int sockHandle, struct sockaddr* paddr,int timeout);

static void ErrorDesc( int errorcode, char **errmsg)
{
	if( errmsg ) {//TODO
		switch( errorcode ) {
			case (COMMS_E_NO_RESPONSE):
				*errmsg = "NO_RESPONSE";
				break;
			case (NO_SIM_CARD):
				*errmsg = "NO SIM CARD";
				break;
			case (COMMS_E_TIMEOUT):
				*errmsg = "TIMEOUT";
				break;
			default:
				*errmsg = "COMMS ERROR";
				break;
		}
	}

}

static void ListNWIF (stNIInfo stArray[], int arrayCount)
{
	int cntr = 0;
	for (cntr = 0; cntr < arrayCount; cntr++) {
		if(((strcmp(stArray[cntr].niDeviceName,"/DEV/COM2")) == 0) && ((strcmp(stArray[cntr].niCommTech,"GPRS")) == 0))
		{
			g_GPRSHandle = stArray[cntr].niHandle;
			break;
		}
	}
}

static int SocketConnect (int* pSocketHandle,char * tHIP, int tPort, int conn_timeout)
{
	int sockHandle = -1;
	int sockType;
	int retVal;

	struct linger myLinger;
	struct sockaddr_in	sockHost;
	struct timeval timeout;

	
	memset (&sockHost, 0, sizeof (struct sockaddr_in));
	memset (&timeout, 0, sizeof (struct timeval));

	LOG_PRINTF ( "[%s] SocketConnect: g_inConnected %d", __FUNCTION__, g_inConnected);
	LOG_PRINTF ( "[%s] SocketConnect: g_inAttached %d", __FUNCTION__, g_inAttached);

	sockHost.sin_family = AF_INET;
	sockHost.sin_addr.s_addr = htonl (inet_addr (tHIP));
	sockHost.sin_port = htons (tPort);
	sockType = SOCK_STREAM;

	sockHandle = socket (AF_INET, sockType, 0);
	if (sockHandle < 0) {
		LOG_PRINTF ( "[%s] Socket creation FAILED: %d. errno: %d", __FUNCTION__, sockHandle, errno);
		return RET_FAILED;
	}

	*pSocketHandle = sockHandle;

	LOG_PRINTF ( "[%s] SocketConnect: tHIP %s", __FUNCTION__, tHIP);
	retVal = connect_nonblock (sockHandle, (struct sockaddr*)&sockHost, 15);
	if (retVal != 0)  {

		LOG_PRINTF ( "[%s] Socket creation FAILED: %d. errno: %d", __FUNCTION__, retVal, errno);
		socketclose (sockHandle);
		return RET_FAILED;
	}

	return RET_SUCCESS;
}

int inCheckGPRSStatus(int x,int y)
{
	char stmp[20];
	char *p = NULL;

	inManageCEEvents ();
	
	strcpy(stmp,chSignalRSSI);
	p = strchr(stmp, 'R');
	if(p) {
		*p = 0;
	}
	g_signal = atoi(stmp);
	if(g_signal == 99) return 0 ; 
	else return g_signal;
}

static vdDisplayBmp(int x,int y, char *bmpfile)
{
	int charmode = false;
	if(get_display_coordinate_mode() == CHARACTER_MODE) {
		charmode = true;
		set_display_coordinate_mode(PIXEL_MODE);
	}
	put_BMP_at(x,y,bmpfile);
	if(	charmode )
		set_display_coordinate_mode(CHARACTER_MODE);
}

void vdDisplayGPRSSignalStrength(int inSignalStatus, int inNetStatus,int x,int y)
{

	if ((inSignalStatus <= 0) || (inNetStatus <0))
		{
			vdDisplayBmp(x,y,"F:GSM4.bmp");
		}
	if ((inSignalStatus >= 25) && (inSignalStatus <= 31))
		{
			vdDisplayBmp(x,y,"F:GSM0.bmp");
		}
	if ((inSignalStatus >= 17) && (inSignalStatus <= 24))
		{
			vdDisplayBmp(x,y,"F:GSM1.bmp");
		}
	if ((inSignalStatus >= 9) && (inSignalStatus <= 16))
		{
			vdDisplayBmp(x,y,"F:GSM2.bmp");
		}
	if ((inSignalStatus >= 1) && (inSignalStatus <= 8))
		{
			vdDisplayBmp(x,y,"F:GSM3.bmp");
		}
}

static int inStartCE_OPEN (void)
{
   unsigned int uiValueLength;
   int inRetVal = RET_FAILED;
   stNI_NWIFState ceNWIF;
   unsigned int event;

   if(g_inErrno) {
	   LOG_PRINTF( " %s:g_inErrno[%d]", __FUNCTION__, g_inErrno );
	   return(g_inErrno);
   }

	inRetVal = ceStartNWIF( g_GPRSHandle , CE_OPEN);
	LOG_PRINTF( "ceStart %d errno%d", inRetVal, errno );

	if (inRetVal == RET_SUCCESS)
	{
	    event = inManageCEEvents ();
		LOG_PRINTF( "inStartCE_OPEN  event %d", event);

		if (event == CE_EVT_START_OPEN)
		{
			inRetVal = RET_SUCCESS;

			do
			{
				inRetVal = ceGetNWParamValue(g_GPRSHandle, NWIF_STATE, &ceNWIF, sizeof (stNI_NWIFState), &uiValueLength);

				if (inRetVal >= 0)
				{
					if (ceNWIF.nsCurrentState == NWIF_CONN_STATE_OPEN)
					{
						inRetVal = RET_SUCCESS;
					}
					else
						SVC_WAIT (TO_ITERATION);
				}
			} while (ceNWIF.nsCurrentState != NWIF_CONN_STATE_OPEN);
		}
		else
		{					
			LOG_PRINTF( "inWaitForThisEvent returned error: %d", inRetVal);
		}
	}
	return (inRetVal);
}

static int inStartCE_NETWORK (void)
{
    int inRetVal = RET_FAILED;
    unsigned long ulTime, ulTimeOut;
    unsigned int event=0;
	int retryCounter;
	unsigned int uiValueLength;
	stNI_NWIFState ceNWIF;

   if(g_inErrno) {
		   LOG_PRINTF( " %s:g_inErrno[%d]", __FUNCTION__, g_inErrno );
		   return(g_inErrno);
   }

	retryCounter = 0;
	for(;;)
	{
		LOG_PRINTF( "inStartCE_NETWORK ***********ceStartNWIF " );
		inRetVal = ceStartNWIF(g_GPRSHandle, CE_CONNECT);
		LOG_PRINTF( "inStartCE_NETWORK ***********inRetVal %d ", inRetVal);

		switch(inRetVal)
		{
			case 0:
				LOG_PRINTF( "inStartCE_NETWORK ***********inRetVal %d ", inRetVal);
				retryCounter = 31;
				ulTimeOut = read_ticks() + TO_STOP;
				LOG_PRINTF( "inStartCE_NETWORK ***********ulTimeOut %d ", ulTimeOut);
				do
				{
					event = inManageCEEvents ();

					LOG_PRINTF( "inStartCE_NETWORK ***********event %d ", event);
					LOG_PRINTF ( "inStartCE_NETWORK g_inConnected %d ", g_inConnected );
					LOG_PRINTF ( "inStartCE_NETWORK g_inAttached %d ", g_inAttached );

					ulTime = read_ticks();

				}while((event != CE_EVT_NET_UP) && (ulTime < ulTimeOut) && !g_inErrno);  

				break;

			case ECE_START_STOP_BUSY:
				LOG_PRINTF( "inStartCE_NETWORK ***********ECE_START_STOP_BUSY" );
				SVC_WAIT(500);
				break;

			case ECE_EVENTS_AGAIN:
				LOG_PRINTF( "inStartCE_NETWORK ***********ECE_EVENTS_AGAIN" );
				return RET_FAILED;

			case ECE_STATE_ERR:
				
				LOG_PRINTF( "inStartCE_NETWORK ***********ECE_STATE_ERR" );
				inRetVal = ceGetNWParamValue(g_GPRSHandle, NWIF_STATE, &ceNWIF, sizeof (stNI_NWIFState), &uiValueLength);

				LOG_PRINTF( "inStartCE_NETWORK ***********inRetVal %d ", inRetVal);
				LOG_PRINTF( "inStartCE_NETWORK ***********ceNWIF.nsEvent %d ", ceNWIF.nsEvent);
				LOG_PRINTF( "inStartCE_NETWORK ***********ceNWIF.nsTargetState %d ", ceNWIF.nsTargetState);
				LOG_PRINTF( "inStartCE_NETWORK ***********ceNWIF.nsCurrentState %d ", ceNWIF.nsCurrentState);
				LOG_PRINTF( "inStartCE_NETWORK ***********ceNWIF.nsErrorCode %d ", ceNWIF.nsErrorCode);
				LOG_PRINTF( "inStartCE_NETWORK ***********ceNWIF.nsErrorString %s ", ceNWIF.nsErrorString);

				switch (ceNWIF.nsCurrentState)
				{
					case NWIF_CONN_STATE_INIT:		// 0 DDI Driver is not yet loaded.
						LOG_PRINTF( "inStartCE_NETWORK NWIF_CONN_STATE_INIT");
						
						inRetVal = inStartCE_OPEN();
						if (inRetVal == RET_FAILED)
							return RET_FAILED;
						break;

					case NWIF_CONN_STATE_OPEN:		// 1 NWIF has now been initialized.
						LOG_PRINTF( "inStartCE_NETWORK NWIF_CONN_STATE_OPEN");
						break;

					case NWIF_CONN_STATE_LINK:		// 2 NWIF has established data link connection.
						LOG_PRINTF( "inStartCE_NETWORK NWIF_CONN_STATE_LINK");

						inRetVal = ceStartNWIF(g_GPRSHandle, CE_LINK);
						ulTimeOut = read_ticks() + TO_STOP;
						LOG_PRINTF( "inStartCE_NETWORK ***********ulTimeOut %d ", ulTimeOut);
						do
						{
							event = inManageCEEvents ();
							ulTime = read_ticks();
						}while((event != CE_EVT_START_LINK) && (ulTime < ulTimeOut));  

						LOG_PRINTF( "inStartCE_NETWORK ***********event %d ", event);
						if (event != CE_EVT_START_LINK)
						{
							return RET_FAILED;
						}
						break;
					case NWIF_CONN_STATE_NET:		// 3 TCP/IP is up and running.
						LOG_PRINTF( "inStartCE_NETWORK NWIF_CONN_STATE_NET");

						retryCounter = 31;
						inRetVal = ceStartNWIF(g_GPRSHandle, CE_NETWORK);

						ulTimeOut = read_ticks() + TO_STOP;
						LOG_PRINTF( "inStartCE_NETWORK ***********ulTimeOut %d ", ulTimeOut);
						do
						{
							event = inManageCEEvents ();
							ulTime = read_ticks();
						}while((event != CE_EVT_NET_UP) && (ulTime < ulTimeOut));  

						LOG_PRINTF( "inStartCE_NETWORK ***********event %d ", event);
						if (event != CE_EVT_NET_UP)
						{
							return RET_FAILED;
						}
						
						break;

					case NWIF_CONN_STATE_ERR:		// -1 An error has occurred
						LOG_PRINTF( "inStartCE_NETWORK NWIF_CONN_STATE_ERR");
						break;
				}

				break;

			case ECE_PARAM_INVALID:
				LOG_PRINTF( "inStartCE_NETWORK ***********ECE_PARAM_INVALID" );
				return RET_FAILED;

			case ECE_DEVOWNER:
				LOG_PRINTF( "inStartCE_NETWORK ***********ECE_DEVOWNER" );
				return RET_FAILED;

			case ECE_DEVBUSY:
				LOG_PRINTF( "inStartCE_NETWORK ***********ECE_DEVBUSY" );
				return RET_FAILED;

			case ECE_INIT_FAILURE:
				LOG_PRINTF( "inStartCE_NETWORK ***********ECE_INIT_FAILURE" );
				return RET_FAILED;

			default:
				LOG_PRINTF( "inStartCE_NETWORK *********** -1" );
				return RET_FAILED;
		}

		retryCounter++;
		LOG_PRINTF( "inStartCE_NETWORK ***********retryCounter %d ", retryCounter);
		if (retryCounter > 30 )
			break;
	}

	LOG_PRINTF( "inStartCE_NETWORK ***********event %d ", event);

	if (event != CE_EVT_NET_UP) //(inRetVal >= 0)
	{
		LOG_PRINTF( "ceStart Last Event Received  %d errno%d", event, errno );
	}
	else 
	   inRetVal = RET_SUCCESS;

	LOG_PRINTF( "inStartCE_NETWORK ***********inRetVal %d ", inRetVal);

	return (inRetVal);
}


static int inStopCE_NETWORK (void)
{
    int inRetVal = RET_FAILED;
    unsigned int event;
    unsigned long ulTime, ulTimeOut;

    if(g_inErrno) {
 	   LOG_PRINTF( " %s:g_inErrno[%d]", __FUNCTION__, g_inErrno );
 	   return(g_inErrno);
    }

	LOG_PRINTF( "inStopCE_NETWORK ***********ceStopNWIF CE_NETWORK " );
	inRetVal = ceStopNWIF(g_GPRSHandle, CE_NETWORK);	
	LOG_PRINTF( "inStopCE_NETWORK ***********inRetVal %d ", inRetVal );

	switch(inRetVal)
	{
		case 0:
			LOG_PRINTF( "inStopCE_NETWORK ***********inRetVal %d ", inRetVal);
			//retryCounter = 31;

			ulTimeOut = read_ticks() + TO_STOP;
			do
			{
				event = inManageCEEvents ();
				ulTime = read_ticks();
				}while((event != CE_EVT_STOP_NW) && (ulTime < ulTimeOut));  

			if(event == CE_EVT_STOP_NW) //(inRetVal == RET_SUCCESS)
			{
				inRetVal = ceStopNWIF(g_GPRSHandle, CE_LINK);
				LOG_PRINTF( "inStopCE_NETWORK **********CE_LINK *inRetVal %d ", inRetVal);
				if (inRetVal == RET_SUCCESS)
				{
					LOG_PRINTF( "ceStop %d errno%d", inRetVal, errno );
			
					ulTimeOut = read_ticks() + TO_STOP;
					do
					{
						event = inManageCEEvents ();
						ulTime = read_ticks();
					}while((event != CE_EVT_STOP_LINK) && (ulTime < ulTimeOut));  
	               
					if(event == CE_EVT_STOP_LINK)
						inRetVal = RET_SUCCESS;
					else
						inRetVal = RET_FAILED;  
					LOG_PRINTF( "inStopCE_NETWORK ***********inRetVal %d ", inRetVal);
				}
			}

			break;

		case ECE_START_STOP_BUSY:
			LOG_PRINTF( "inStopCE_NETWORK ***********ECE_START_STOP_BUSY" );
			SVC_WAIT(500);
			break;

		case ECE_EVENTS_AGAIN:
			LOG_PRINTF( "inStopCE_NETWORK ***********ECE_EVENTS_AGAIN" );
			return RET_FAILED;

		case ECE_STATE_ERR:
			LOG_PRINTF( "inStopCE_NETWORK ***********ECE_STATE_ERR" );

			inRetVal = ceStopNWIF(g_GPRSHandle, CE_LINK);
			LOG_PRINTF( "inStopCE_NETWORK ********CE_LINK***inRetVal %d", inRetVal );
			if (inRetVal == RET_SUCCESS)
			{
				LOG_PRINTF( "ceStop %d errno%d", inRetVal, errno );
			
				ulTimeOut = read_ticks() + TO_STOP;
				do
				{
					event = inManageCEEvents ();
					ulTime = read_ticks();
				}while((event != CE_EVT_STOP_LINK) && (ulTime < ulTimeOut));  
	               
				if(event == CE_EVT_STOP_LINK)
					inRetVal = RET_SUCCESS;
				else
					inRetVal = RET_FAILED;  

				LOG_PRINTF( "inStopCE_NETWORK inRetVal %d", inRetVal);
			}
			else
			{
				LOG_PRINTF( "inStopCE_NETWORK CE_DISCONNECT inRetVal %d", inRetVal);
				inRetVal = ceStopNWIF(g_GPRSHandle, CE_DISCONNECT);
				LOG_PRINTF( "inStopCE_NETWORK CE_DISCONNECT inRetVal %d", inRetVal);
				ulTimeOut = read_ticks() + TO_STOP;
				do
				{
					event = inManageCEEvents ();
					ulTime = read_ticks();
				}while((event != CE_EVT_STOP_LINK) && (ulTime < ulTimeOut));  
	               
				if(event == CE_EVT_STOP_LINK)
					inRetVal = RET_SUCCESS;
				else
					inRetVal = RET_FAILED;  
				LOG_PRINTF( "inStopCE_NETWORK CE_DISCONNECT inRetVal %d", inRetVal);

			}
				
			break;

		case ECE_PARAM_INVALID:
			LOG_PRINTF( "inStopCE_NETWORK ***********ECE_PARAM_INVALID" );
			return RET_FAILED;

		case ECE_DEVOWNER:
			LOG_PRINTF( "inStopCE_NETWORK ***********ECE_DEVOWNER" );
			return RET_FAILED;

		case ECE_DEVBUSY:
			LOG_PRINTF( "inStopCE_NETWORK ***********ECE_DEVBUSY" );
			return RET_FAILED;

		default:
			LOG_PRINTF( "inStopCE_NETWORK *********** -1" );
			return RET_FAILED;
	}

	LOG_PRINTF( "inStopCE_NETWORK inRetVal %d", inRetVal);
	return (inRetVal);
}

static int inCEStartGPRSNetwork(char *sAPN)
{
	static int firsttime = 1;
	int inRetVal = RET_FAILED;

   if(g_inErrno) {
		   LOG_PRINTF( " %s:g_inErrno[%d]", __FUNCTION__, g_inErrno );
		   return(g_inErrno);
   }

	LOG_PRINTF( "inCEStartGPRSNetwork ******* firsttime %d", firsttime);
	LOG_PRINTF( "inCEStartGPRSNetwork ******* sAPN %s", sAPN);

	if(firsttime ) {
		
		int reconnect = 1;
		char model[20]="";
		
		SVC_INFO_MODELNO(model);
		if( strstr(model,"3G")!= NULL) {
			int hs_network = 1;
			ceSetDDParamValue(g_GPRSHandle, INI_HS_NETWORK, &hs_network, sizeof(hs_network));
		}
		ceSetDDParamValue(g_GPRSHandle, AUTO_RECONNECT, &reconnect, sizeof(reconnect));

		ceSetDDParamValue(g_GPRSHandle, INI_GPRS_APN, sAPN, strlen (sAPN) + 1);

		firsttime = 0;

		inRetVal = inStartCE_OPEN();

		LOG_PRINTF( "inCEStartGPRSNetwork inStartCE_OPEN inRetVal %d", inRetVal);
		//3G
		SVC_WAIT(2000);
		if (inRetVal != RET_SUCCESS) 
		{
			LOG_PRINTF( "inCEStartGPRSNetwork inStartCE_OPEN inRetVal %d", inRetVal);
			return(inRetVal);
		}
	}

	LOG_PRINTF( "inCEStartGPRSNetwork  inRetVal %d", inRetVal);

	inRetVal = inStartCE_NETWORK ();

	return(inRetVal);
}


static int connect_nonblock(int sockHandle, struct sockaddr* paddr,int timeout)
{
	int res; 
	int valopt; 
	int result;
	unsigned long ulTimeout;

	result = socketerrno(SOCKET_ERROR);
	LOG_PRINTF( "connect_nonblock  result %d", result );

	// Set non-blocking 
	valopt = 1; 
	if( (res = socketioctl(sockHandle, FIONBIO, &valopt)) < 0) {
		LOG_PRINTF( "connect_nonblock  errno%d", errno );
		return(-1); 
	}

	LOG_PRINTF( "connect_nonblock  timeout%d", timeout );
	ulTimeout = read_ticks() + RESPONSE_TIMEOUT * timeout;

	// Trying to connect with timeout 
	res = connect(sockHandle, (struct sockaddr *)paddr, sizeof(struct sockaddr_in)); 
	
	LOG_PRINTF ( "[%s] Socket creation FAILED: %d. errno: %d", __FUNCTION__, res, errno);

	if (res != 0) {
		do { 
			result = socketerrno(sockHandle);
			
			LOG_PRINTF( "connect_nonblock  result%d", result );

			if (result == 0)
				break;

			if((result != 0) && (result != EINPROGRESS) && (result != EISCONN))
				return(-1);

			if (read_ticks()>=ulTimeout) 
			{
				LOG_PRINTF( "connect_nonblock  Timeout in select res %d", res );
				return(-2);
			}

		} while (1); 
	} 
	else { 
		LOG_PRINTF( "Error connecting  errno%d", errno );
        return(-1);
	} 

  valopt = 0; 
  socketioctl(sockHandle, FIONBIO, &valopt);
  return(0);
}

static int inStartCeConnection(char * sAPN)
{
	int retVal = 0;

   if(g_inErrno) {
		   LOG_PRINTF( " %s:g_inErrno[%d]", __FUNCTION__, g_inErrno );
		   return(g_inErrno);
   }

	LOG_PRINTF( "inStartCeConnection  sAPN %s", sAPN );

	retVal = inCEStartGPRSNetwork(sAPN);
	return retVal;
}

static int ceForceReconnect( stNI_NWIFState ceNWIF,char *newAPN)
{
	int inRetVal = -1;
	unsigned long ulTime, ulTimeOut;
	unsigned int event;

	LOG_PRINTF ( "ceForceReconnect ceNWIF.nsCurrentState: %d", ceNWIF.nsCurrentState);
	LOG_PRINTF ( "ceForceReconnect ceNWIF.nsEvent: %d", ceNWIF.nsEvent);

	inRetVal = RET_SUCCESS;

	switch (ceNWIF.nsEvent)
	{
		case CE_EVT_PROCESSING_NET_UP:

			LOG_PRINTF ( "ceForceReconnect CE_EVT_PROCESSING_NET_UP" );
			ulTimeOut = read_ticks() + TO_STOP;
			do
			{
				event = inManageCEEvents ();
				ulTime = read_ticks();
			}while((event != CE_EVT_NET_UP) && (ulTime < ulTimeOut));  
			break;
		default:
			switch (ceNWIF.nsCurrentState)
			{
				case NWIF_CONN_STATE_INIT:	//	0	DDI Driver is not yet loaded.
					LOG_PRINTF ( "ceForceReconnect ceNWIF.nsCurrentState: %d", ceNWIF.nsCurrentState);
					inRetVal = inStartCE_OPEN();
					LOG_PRINTF ( "ceForceReconnect inRetVal: %d", inRetVal);
					if (inRetVal == RET_FAILED)
					{
						LOG_PRINTF ( "ceForceReconnect inRetVal: %d", inRetVal);
						return RET_FAILED;
					}
					break;
				case NWIF_CONN_STATE_OPEN:	//	1	NWIF has now been initialized.
					LOG_PRINTF ( "ceForceReconnect ceNWIF.nsCurrentState: %d", ceNWIF.nsCurrentState);
					inRetVal = inStartCE_NETWORK ();
					LOG_PRINTF ( "ceForceReconnect inRetVal: %d", inRetVal);
					if (inRetVal == RET_FAILED)
					{
						LOG_PRINTF ( "ceForceReconnect inRetVal: %d", inRetVal);
						return RET_FAILED;
					}
					break;

				case NWIF_CONN_STATE_LINK:	//	2	NWIF has established data link connection.
				case NWIF_CONN_STATE_NET:	//	3	TCP/IP is up and running.

					LOG_PRINTF ( "ceForceReconnect ceNWIF.nsCurrentState: %d", ceNWIF.nsCurrentState);
					inRetVal = inStopCE_NETWORK();
					LOG_PRINTF ( "ceForceReconnect inRetVal: %d", inRetVal);
					if (inRetVal == RET_FAILED)
					{
						LOG_PRINTF ( "ceForceReconnect inRetVal: %d", inRetVal);
						return RET_FAILED;
					}

					if(newAPN && strlen(newAPN)) {
						ceSetDDParamValue(g_GPRSHandle, INI_GPRS_APN, newAPN, strlen (newAPN) + 1);
					}

					inRetVal = inStartCE_NETWORK ();
					LOG_PRINTF ( "ceForceReconnect inRetVal: %d", inRetVal);
					if (inRetVal == RET_FAILED)
					{
						return RET_FAILED;
					}
					break;

				case NWIF_CONN_STATE_ERR:	//	-1	An error has occurred.
					LOG_PRINTF ( "ceForceReconnect ceNWIF.nsCurrentState: %d", ceNWIF.nsCurrentState);
					return RET_FAILED;
			}
		break;
	}

	LOG_PRINTF ( "ceForceReconnect inRetVal: %d", inRetVal);
	return inRetVal;
}

int inCeTcpConnect(T_COMMS * psComms)
{
	static char sAPN[20] = "";
	static char sHOSTIP[30] = "";
	static int nPort=0;
	bool changeAPN = false,change = false;
	int inRetVal = -1;
	int NWParamValue;
	unsigned int uiValueLength;
	stNI_NWIFState ceNWIF;
    unsigned long ulStartTime;
    unsigned long ulTotalTime;
	char chBuffer [21 + 1];
	static int firsttime = 1;

	if( strcmp( sAPN, psComms->ownIpAddress)) {
		strcpy( sAPN, psComms->ownIpAddress);
		change = true;
		changeAPN = true;
	}

	if( strcmp( sHOSTIP, psComms->ipAddress)) {
		strcpy( sHOSTIP, psComms->ipAddress);
		change = true;
	}

	if( nPort != psComms->wPortNumber) {
		nPort = psComms-> wPortNumber ;
		change = true;
	}

	if (firsttime) {
		int inRetVal = inStartCeConnection(psComms->ownIpAddress);
		firsttime = 0;
		change = false;
		changeAPN = false;
	}
	if(g_inErrno) {
			ErrorDesc(g_inErrno, &psComms->pErrmsg);
			LOG_PRINTF( " %s:g_inErrno[%d]", __FUNCTION__, g_inErrno );
			return(RET_FAILED);
	}

	if (change) {}

	if (gSocketHandle > 0) 
		socketclose (gSocketHandle);

	gSocketHandle = -1;
	
	LOG_PRINTF( "inCeTcpConnect  sHOSTIP %s", sHOSTIP );
	LOG_PRINTF( "inCeTcpConnect  nPort %d", nPort );

	// if APN is not changed
	if(  !changeAPN ) {
		inRetVal = SocketConnect (&gSocketHandle,sHOSTIP,nPort,psComms->bConnectionTimeout);
		LOG_PRINTF( "inCeTcpConnect  retVal %d", inRetVal );
		if(inRetVal == 0) {
			return 1;
		}
	}else{

	}

	ulStartTime = read_ticks();

	inManageCEEvents ();
	/* Check signal, no signal no point trying  */
	if (0 == g_inAttached )
		return RET_FAILED;

	NWParamValue = ceGetNWParamValue(g_GPRSHandle, NWIF_STATE, &ceNWIF, sizeof (stNI_NWIFState), &uiValueLength);

	LOG_PRINTF( "inCeTcpConnect  ceNWIF.nsErrorString %s", ceNWIF.nsErrorString );

	LOG_PRINTF( "inCeTcpConnect ceGetNWParamValue errorcode= %d,current=%d,target=%d\n", ceNWIF.nsErrorCode, ceNWIF.nsCurrentState, ceNWIF.nsTargetState );

	LOG_PRINTF( "inCeTcpConnect  NWParamValue %d", NWParamValue );
	switch (NWParamValue)
	{
		case 0:
			break;

		case ECE_NOTREG:
		case ECE_PARAM_INVALID:
		case ECE_SIGNAL_FAILURE:
		default:
			return RET_FAILED;
	}

	LOG_PRINTF( "inCeTcpConnect  ceNWIF.nsEvent %d", ceNWIF.nsEvent );
	switch (ceNWIF.nsEvent)
	{
		case CE_EVT_NET_POOR:
			return RET_FAILED;

		case CE_EVT_NET_FAILED:
		case CE_EVT_STOP_CLOSE:
		case CE_EVT_NET_DN:
		case CE_EVT_NET_OUT:
			break;

		case CE_EVT_PROCESSING_NET_DN:
			return RET_FAILED;

		case CE_EVT_RECONNECT_PENDING:
			break;

		case CE_EVT_PROCESSING_NET_UP:
			break;

		case CE_EVT_NET_UP:
		case CE_EVT_NET_RES:


			break;

		default:
			return RET_FAILED;
	 }

	LOG_PRINTF( "inCeTcpConnect  ceNWIF.nsErrorCode %d", ceNWIF.nsErrorCode );
	switch (ceNWIF.nsErrorCode)
	{
		case 0:
			//voDisplayText("RECONNECTING", 2, J_CENTER, NO_DELAY, 0);

			LOG_PRINTF( "inCeTcpConnect  ceNWIF.nsErrorCode %d", ceNWIF.nsErrorCode );
			inRetVal = ceForceReconnect(ceNWIF, changeAPN?sAPN:NULL);
	
			LOG_PRINTF( "inCeTcpConnect inRetVal %d", inRetVal );
			break;

		case ECE_PPP_TIMEOUT_FAILURE:
		case ECE_STATE_ERR:
		case ECE_ETHERNET_REBIND_TIMEOUT:
		case ECE_RESTORE_DEFAULT_FAILURE:
		case ECE_TCPIP_STACK_FAILURE:
		case ECE_PPP_AUTH_FAILURE:

			LOG_PRINTF( "inCeTcpConnect  ceNWIF.nsErrorCode %d", ceNWIF.nsErrorCode );
			
			//voDisplayText("RECONNECTING", 2, J_CENTER, NO_DELAY, 0);
			inRetVal = ceForceReconnect(ceNWIF, changeAPN?sAPN:NULL);
			LOG_PRINTF( "inCeTcpConnect inRetVal %d", inRetVal );
			break;

		case ECE_INIT_FAILURE:
		case ECE_SIGNAL_FAILURE:
		case ECE_CONNECTION_LOST:
		case ECE_BATTERY_THRESHOLD_FAILURE:
		default:
			return RET_FAILED;
	}  

	LOG_PRINTF( "inCeTcpConnect inRetVal %d", inRetVal );

	ulTotalTime = (read_ticks() - ulStartTime) / TICKS_PER_SEC;

	//clrscr();
	//voDisplayText("CE_NETWORK SUCCESS", 2, J_CENTER, NO_DELAY, 0);
	//voDisplayText("TOTAL TIME:", 4, J_CENTER, NO_DELAY, 0);
	sprintf (chBuffer, "%lu SECONDS", ulTotalTime);
	LOG_PRINTF( "CE_NETWORK SUCCESS TOTAL TIME: %lu",ulTotalTime );
	//voDisplayText(chBuffer, 5, J_CENTER, NO_DELAY, 0);
							
	//voDisplayText("PRESS A KEY", 7, J_CENTER, IDLE_TIMER, 1);

	inRetVal = SocketConnect (&gSocketHandle,sHOSTIP,nPort,psComms->bConnectionTimeout);
	if(inRetVal == RET_SUCCESS) 
	{
		//return RET_SUCCESS;
		return 1;
	}

	return RET_FAILED;
}

int inCeTcpDisConnectIP()
{
	if(gSocketHandle>0) socketclose (gSocketHandle);
	gSocketHandle = -1;
	return(0);
}

int inSendTcpRaw( char *pchBuff,int ilen)
{
	int retVal = send(gSocketHandle, pchBuff, ilen, 0);
	return(retVal);
}

int inSendTCPCommunication(T_COMMS * psComms)
{
    int  retVal;
	char szTransmitBuffer[4096+1];
    
	char sHOSTIP[18];
	char *pchSendBuff =(char*) psComms->pbData;
	int inSendSize = psComms->wLength;

	if(g_inErrno) {
		ErrorDesc(g_inErrno, &psComms->pErrmsg);
		LOG_PRINTF( " %s:g_inErrno[%d]", __FUNCTION__, g_inErrno );
		return(RET_FAILED);
   }

	memset(szTransmitBuffer, 0x00, sizeof(szTransmitBuffer));
	memcpy(szTransmitBuffer, pchSendBuff, inSendSize);

	strcpy(sHOSTIP, psComms->ipAddress);
	//nPort = psComms->wPortNumber;
	
	retVal = send(gSocketHandle, szTransmitBuffer, inSendSize, 0);
	LOG_PRINTF( "inSendTCPCommunication sock:%d,sent:%d, retVal %d", gSocketHandle,inSendSize,retVal );

	return(retVal);

}

int inRecvNext(char *pchReceiveBuff, int len)
{
    int  retVal = 0;
    struct timeval  mytimeval;

	if(gSocketHandle > 0) {
    	mytimeval.tv_sec = 5;
    	mytimeval.tv_usec = 100;
    	setsockopt(gSocketHandle, SOL_SOCKET, SO_RCVTIMEO, (char*) &mytimeval, sizeof(mytimeval) );
		retVal = recv(gSocketHandle,pchReceiveBuff,len,0);
	}

	return(retVal);
}

int inReceiveTCPCommunication(T_COMMS * psComms)
{
    int  retVal = 0;
    struct timeval  mytimeval;
	char *pchReceiveBuff = (char *)psComms->pbData;
	unsigned long MaxBufLen = 8192;

	if(g_inErrno) {
		ErrorDesc(g_inErrno, &psComms->pErrmsg);
		LOG_PRINTF( " %s:g_inErrno[%d]", __FUNCTION__, g_inErrno );
		return(RET_FAILED);
   }

	psComms->pErrmsg = NULL;
	errno = 0;
    if(MaxBufLen< psComms->wLength) {
    	MaxBufLen = psComms->wLength;
        setsockopt(gSocketHandle, SOL_SOCKET, SO_RCVBUF, (unsigned long*) &MaxBufLen, sizeof(MaxBufLen) );
    }
    mytimeval.tv_sec = psComms->bResponseTimeout;
    mytimeval.tv_usec = 100;
    setsockopt(gSocketHandle, SOL_SOCKET, SO_RCVTIMEO, (char*) &mytimeval, sizeof(mytimeval) );

	retVal = recv(gSocketHandle, pchReceiveBuff, MaxBufLen, 0);

	LOG_PRINTF( "inReceiveTCPCommunication retVal %d", retVal );

	if(retVal<=0 )
	{
		if(errno == ETIMEOUT) {
			ErrorDesc(COMMS_E_TIMEOUT, &psComms->pErrmsg);
			return(COMMS_E_TIMEOUT); //TIMEOUT
		}
		else if(retVal == 0) {
			ErrorDesc(COMMS_E_NO_RESPONSE, &psComms->pErrmsg);
			return(-1);
		}else
			return(retVal);

	}
	else
	{
		return(retVal);
	}
}


static int DebugPrint2 (const char*template,...) 
{
    va_list ap;
	char stmp[200];
	static int debugflag = -1;

	if(debugflag == -1) {
		char flag[30] = "";
		memset(flag,0,sizeof(flag));
		get_env("RISGPRSDEBUG", flag, sizeof(flag));
		if(strncmp(flag,"YES",3) == 0)
				debugflag = 1;
		else debugflag = 0;
	}
	if(debugflag == 0) return 0;

    memset(stmp,0,sizeof(stmp));
    va_start (ap, template);
    vsnprintf (stmp,128, template, ap);
    strcat(stmp,"\n");
  PrtPrintBuffer(strlen(stmp), (uchar *)stmp, E_PRINT_END);
  return(0);
}

static int DebugPrint (const char*template,...) {

    va_list ap;
    char stmp[1024];
	char s_debug[30] = "\033k042GPRS_DEBUG:";
	stNI_NWIFState nwState;
	int pLen;
	char now[30];
	static int debugflag = -1;


	if(debugflag == -1) {
		char flag[30] = "";
		memset(flag,0,sizeof(flag));
		get_env("RISGPRSDEBUG", flag, sizeof(flag));
		if(strncmp(flag,"YES",3) == 0)
				debugflag = 1;
		else debugflag = 0;
	}
	if(debugflag == 0) return 0;

	memset(now,0,sizeof(now));
	read_clock(now);

    memset(stmp,0,sizeof(stmp));
    va_start (ap, template);
    vsnprintf (stmp,128, template, ap);

  ceGetNWParamValue (g_currMediaInfo.niHandle, NWIF_STATE, &nwState, sizeof (stNI_NWIFState), (unsigned int *)&pLen);
  PrtPrintBuffer(strlen(s_debug), (uchar *)s_debug, 0);
  PrtPrintBuffer(strlen(stmp), (uchar *)stmp, 2);//E_PRINT_END
  PrtPrintBuffer(1, "\n", E_PRINT_END);//E_PRINT_END

  sprintf(stmp,"current_state=%d,newstate=%d,event=%d,errorcode=%d %s,time=%s",nwState.nsCurrentState,nwState.nsTargetState,nwState.nsEvent,nwState.nsErrorCode,nwState.nsErrorString,now);
  strcat(stmp,"\n");
  PrtPrintBuffer(strlen(stmp),(uchar *)stmp, 2);//E_PRINT_END
  return 0;
}


unsigned int inManageCEEvents (void)
{
	stNI_NWIFState ceNWIF;
	stceNWEvt ceEvt;
	int retVal = 0;
	unsigned int uiValueLength;
	int evtDataLen =0;
	char chBuffer [MAX_EVENT_NAME_SIZE + 1];
	char chDispBuffer [100 + 1];

	memset (&ceEvt, 0, sizeof (stceNWEvt));
	memset (g_ucExtEventData, 0, CEIF_EVT_DATA_SZ);
    
	while ( ceGetEventCount() )
	{
		        LOG_PRINTF( "%s:Reading CE event", __FUNCTION__ );

	            memset (&ceEvt, 0, sizeof (stceNWEvt));
	            memset (g_ucExtEventData, 0, CEIF_EVT_DATA_SZ);
				
				retVal = ceGetEvent (&ceEvt, CEIF_EVT_DATA_SZ, (void*) g_ucExtEventData, &evtDataLen);

				LOG_PRINTF ("[%s] Received CE Event ID [%d] from NWIF Handle [%d] Event Data Size [%d]", 
			                 __FUNCTION__, ceEvt.neEvt, ceEvt.niHandle, evtDataLen);
                
                voTranslateCEevt(ceEvt.neEvt, chBuffer);
		        LOG_HEX_PRINTF ("EVENT DATA", (char*) g_ucExtEventData, evtDataLen);
		        if ( strlen(g_ucExtEventData) && evtDataLen>0 ) {
					if( ceEvt.neParam1 == NO_SIM_CARD) {
						g_inErrno = NO_SIM_CARD;
						return(0);
					}
		        }
				if ( 0 == retVal)
				{
					LOG_PRINTF( "%s:CEevent niHandle:%d evtID:0x%02X", __FUNCTION__, ceEvt.niHandle, ceEvt.neEvt);
					
					if (ceEvt.niHandle != g_GPRSHandle)
		            {
		                LOG_PRINTF ("No GPRS Conection!");
			            return 0;
                    }
				    sprintf (chDispBuffer, "Last Event: %s", chBuffer);

					switch(ceEvt.neEvt)
					{
							char chCEEvent [22] ;
							// Following events will trigger an UI update but will continue
							// waiting for further event notifications
							case CE_EVT_NET_OUT:		
								LOG_PRINTF ("NET OUT");
				                strcpy (chCEEvent, "NETWORK OUT");
				                g_inConnected = 0;				
								break;
								
							case CE_EVT_NET_RES:		
								LOG_PRINTF ("NET RESTORED");
				                strcpy (chCEEvent, "NETWORK RESTORED");
				                g_inConnected = 1;					
								break;
								
							case CE_EVT_NET_POOR:		
								LOG_PRINTF ("NETWORK POOR");
				                strcpy (chCEEvent, "NETWORK POOR");
				                g_inConnected = 0;									
								break;
								
							case CE_EVT_NET_DN:			
								LOG_PRINTF ("NET DOWN");
				                strcpy (chCEEvent, "NETWORK DOWN");
				                g_inConnected = 0;
								break;
								
							case CE_EVT_NET_FAILED:		
						        LOG_PRINTF ("NET FAILED");
				                strcpy (chCEEvent, "NETWORK FAILED");
				                g_inConnected = 0;
								break;

							case CE_EVT_PROCESSING_NET_DN:
							case CE_EVT_PROCESSING_NET_UP:
								LOG_PRINTF ("NET IN TRANSITION");
				                strcpy (chCEEvent, "NETWORKT IN TRANSITION");
				                g_inConnected = 0;			
								break;
								
						    case CE_EVT_SIGNAL:				
				                 LOG_PRINTF ("NET SIGNAL");
				                 sprintf (chSignalPercent, "%d%%", ceEvt.neParam1);
				                 sprintf (chSignaldBm, "%ddBm", ceEvt.neParam2);
				                 sprintf (chSignalRSSI, "%dRSSI", ceEvt.neParam3);

				                 retVal = ceGetNWParamValue(g_GPRSHandle, NWIF_STATE, &ceNWIF, sizeof (stNI_NWIFState), &uiValueLength);

				                 if (retVal >= 0)
				                 {
					                if (ceNWIF.nsCurrentState == NWIF_CONN_STATE_OPEN ||
						                ceNWIF.nsCurrentState == NWIF_CONN_STATE_LINK ||
						                ceNWIF.nsCurrentState == NWIF_CONN_STATE_NET)
						                   g_inAttached = 1;
					                    else
						                   g_inAttached = 0;
				                 }
				                 break;		

							// Following events are treated as final response
							case CE_EVT_NET_UP:
				                 LOG_PRINTF ("NET UP");
				                 strcpy (chCEEvent, "NETWORK UP");
				                 g_inConnected = 1;
				                 return CE_EVT_NET_UP;
				                 
				           case CE_EVT_STOP_NW:
				                 LOG_PRINTF ("STOP NETWORK");
				                 strcpy (chCEEvent, "STOP NETWORK");
				                 return CE_EVT_STOP_NW;
								
							case CE_EVT_STOP_CLOSE:
						         LOG_PRINTF ("STOP CLOSE");
				                 strcpy (chCEEvent, "CLOSED - RESTART NW");
				                 g_inAttached = 0;
				                 g_inClosed = 1;
								 return CE_EVT_STOP_CLOSE;
								 
						   case CE_EVT_START_OPEN:		
						         LOG_PRINTF ("START OPEN");
				                 strcpy (chCEEvent, "START OPEN");
				                 g_inAttached = 0;
				                 g_inClosed = 0;
								 return CE_EVT_START_OPEN; 
				                 
				           case CE_EVT_STOP_FAIL:
				                LOG_PRINTF ("STOP FAILED");
				                strcpy (chCEEvent, "STOP FAILED");
				                return CE_EVT_STOP_FAIL;
				                
				           case CE_EVT_START_LINK:
				                LOG_PRINTF ("START LINK");
				                strcpy (chCEEvent, "START LINK");
				                return CE_EVT_START_LINK;     
				                
				           case CE_EVT_STOP_LINK:
				                LOG_PRINTF ("STOP LINK");
				                strcpy (chCEEvent, "STOP LINK");
				                return CE_EVT_STOP_LINK;

						   default:
								LOG_PRINTF( "%s:Correct niHandle but unexpected CE Event", __FUNCTION__ );
								break;
						}

					} //if get event was success
			} //while there are events present
	return(0);
}

void voTranslateCEevt(int inCEEVT, char* chTranslatedEvent)
{	
	switch (inCEEVT)
	{	
		case CE_EVT_NO_EVENT:
			strcpy (chTranslatedEvent, "CE_EVT_NO_EVENT");
			break;
			
		case CE_EVT_NET_UP:
			strcpy (chTranslatedEvent, "CE_EVT_NET_UP");
			break;
			
		case CE_EVT_NET_DN:
			strcpy (chTranslatedEvent, "CE_EVT_NET_DN");
			break;
			
		case CE_EVT_NET_FAILED:
			strcpy (chTranslatedEvent, "CE_EVT_NET_FAILED");
			break;
			
		case CE_EVT_NET_OUT:
			strcpy (chTranslatedEvent, "CE_EVT_NET_OUT");
			break;
			
		case CE_EVT_NET_RES:
			strcpy (chTranslatedEvent, "CE_EVT_NET_RES");
			break;
			
		case CE_EVT_SIGNAL:
			strcpy (chTranslatedEvent, "CE_EVT_SIGNAL");
			break;
			
		case CE_EVT_START_OPEN:
			strcpy (chTranslatedEvent, "CE_EVT_START_OPEN");
			break;
			
		case CE_EVT_START_LINK:
			strcpy (chTranslatedEvent, "CE_EVT_START_LINK");
			break;
			
		case CE_EVT_START_NW:
			strcpy (chTranslatedEvent, "CE_EVT_START_NW");
			break;
			
		case CE_EVT_START_FAIL:
			strcpy (chTranslatedEvent, "CE_EVT_START_FAIL");
			break;
			
		case CE_EVT_STOP_NW:
			strcpy (chTranslatedEvent, "CE_EVT_STOP_NW");
			break;
			
		case CE_EVT_STOP_LINK:
			strcpy (chTranslatedEvent, "CE_EVT_STOP_LINK");
			break;
			
		case CE_EVT_STOP_CLOSE:
			strcpy (chTranslatedEvent, "CE_EVT_STOP_CLOSE");
			break;
			
		case CE_EVT_STOP_FAIL:
			strcpy (chTranslatedEvent, "CE_EVT_STOP_FAIL");
			break;
			
		case CE_EVT_DDI_APPL:
			strcpy (chTranslatedEvent, "CE_EVT_DDI_APPL");
			break;
			
		case CE_EVT_START_DIAL:
			strcpy (chTranslatedEvent, "CE_EVT_START_DIAL");
			break;
			
		case CE_EVT_NET_POOR:
			strcpy (chTranslatedEvent, "CE_EVT_NET_POOR");
			break;
			
		case CE_EVT_PROCESSING_NET_UP:
			strcpy (chTranslatedEvent, "CE_EVT_PROCESSING_NET_UP");
			break;
			
		case CE_EVT_PROCESSING_NET_DN:
			strcpy (chTranslatedEvent, "CE_EVT_PROCESSING_NET_DN");
			break;
			
		case CE_EVT_RECONNECT_PENDING:
			strcpy (chTranslatedEvent, "CE_EVT_RECONNECT_PENDING");
			break;

		default:
			strcpy (chTranslatedEvent, "UNKNOWN");
			break;
	}
}


int InitComEngine_Register (void)
{
	// Register this appication with CommEngine
	int retVal = 0;
	int inTryAgainFlag = 1;

	LOG_PRINTF ("**** InitComEngine Start ****");

	while (inTryAgainFlag == 1)
	{
		retVal = ceRegister ();

		LOG_PRINTF ("ceRegister retVal: %d", retVal);

		if (retVal < 0) 
		{
			switch (retVal) 
			{
				case ECE_REGAGAIN:
					LOG_PRINTF ("**** ceRegister failed try again ****");
					ceUnregister ();	// Unregister first before
					SVC_WAIT(200);
				break;

				default:
					retVal = RET_FAILED;
					inTryAgainFlag = 0;
					break;
			}
		}
		else
			inTryAgainFlag = 0;
	}
	return (retVal);
}

int InitCEEvents (void)
{	
	int retVal = 0;

	LOG_PRINTF ("**** InitCEEvents Start ****");

	// Subscribe notification events from CommEngine
	retVal = ceEnableEventNotification ();
	LOG_PRINTF ("[%s] ceEnableEventNotification retVal: %d", __FUNCTION__, retVal);

	if (retVal < 0) {
		LOG_PRINTF ("[%s] ceEnableEventNotification FAILED.  retVal: %d", __FUNCTION__, retVal);
		return retVal;
	}

	// Handle Signal Events
	retVal = ceSetSignalNotification (CE_SF_ON);
	LOG_PRINTF ("[%s] ceSetSignalNotificationFreq retVal: %d", __FUNCTION__, retVal);
	return 0;
}


// Checks how many network interfaces (NWIF) are available
// and will be stored in the stNIInfo structure array
// Returns the number of supported NWIF

int InitNWIF (void)
{
	int retVal = 0;
	unsigned int nwInfoCount = 0;

	LOG_PRINTF ("**** InitNWIF Start ****");

	// Get the total number of network interface from this terminal (NWIF)
	g_NICount = ceGetNWIFCount ();

	LOG_PRINTF ("ceGetNWIFCount retVal: %d", g_NICount);

	if (g_NICount < 0) {
		LOG_PRINTF ("ceGetNWIFCount FAILED.  retVal: %d", retVal);
		return -1;
	}

	g_NIInfo = (unsigned char*) malloc (g_NICount * sizeof (stNIInfo));

	retVal = ceGetNWIFInfo ( (stNIInfo*) g_NIInfo, g_NICount, &nwInfoCount);

	LOG_PRINTF ("ceGetNWIFInfo retVal: %d", retVal);

	if (retVal < 0) {
		LOG_PRINTF ("ceGetNWIFInfo FAILED.  retVal: %d", retVal);
		return -1;
	}

	ListNWIF ((stNIInfo*)g_NIInfo, g_NICount);
	return retVal;
}

void GetMediaInfo (stNIInfo stArray[], int arrayCount)
{
	int i = 0;	

	for (i=0; i < arrayCount; i++) 
	{
		ceSetNWIFStartMode( stArray [i].niHandle, CE_SM_MANUAL);

		if (strcmp (stArray [i].niCommTech, CE_COMM_TECH_GPRS) == 0) 
		{
			g_GPRSHandle = stArray[i].niHandle;
		}
	}
}

int InitComEngine(void)
{
	stNI_IPConfig ip_config;
	stNI_PPPConfig ppp_config;
	int auto_reconnect;

	LOG_INIT( "RUN_AURIS", LOGSYS_COMM, 0xFFFFFFFF );

	InitComEngine_Register();

	InitCEEvents ();

	// Get all NWIF
	InitNWIF ();	

	GetMediaInfo ( (stNIInfo*) g_NIInfo, g_NICount);

	/* use DHCP */
	memset(&ip_config,0,sizeof(ip_config));
	ip_config.ncDHCP = 1; 
	ceSetNWParamValue( g_GPRSHandle, IP_CONFIG, &ip_config, sizeof(ip_config));

	memset(&ppp_config,0,sizeof(ppp_config));
	ppp_config.ncAuthType = PPP_AUTH_NONE;
	ceSetNWParamValue( g_GPRSHandle, PPP_CONFIG, &ppp_config, sizeof(ppp_config));

	auto_reconnect = 1;
	ceSetDDParamValue( g_GPRSHandle, AUTO_RECONNECT, &auto_reconnect, sizeof(auto_reconnect));

	return(0);

}
