//
//-----------------------------------------------------------------------------
// PROJECT:			iRIS
//
// FILE NAME:       iristcp.c
//
// DATE CREATED:    5 March 2008
//
// AUTHOR:          Tareq Hafez
//
// DESCRIPTION:     This module supports TCP/IP communication functions
//-----------------------------------------------------------------------------
//

//
//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------
//

//
// Standard include files.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Project include files.
//
#include <auris.h>
#include <svc.h>

//
// Local include files
//
#include "my_time.h"
#include "comms.h"
#include "display.h"
#include "iris.h"
#include "iriscomms.h"
#include "irisfunc.h"

//
//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
//

//
//-----------------------------------------------------------------------------
// Module variable definitions and initialisations.
//-----------------------------------------------------------------------------
//
static T_COMMS comms;
static int retVal;
static int bufLen = 300;

static char currOwnIPAddress[50]="";
static char currGateway[50]="";
static char currPDNS[50]="";
static char currSDNS[50]="";
static char currIPAddress[50]="";
static char currHeader[5]="";
static unsigned int currPortNumber=0;
static unsigned int currHandle=0xFFFF;
//static unsigned long connTime = 0;

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
// TCP FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
void __tcp_disconnect_do(void);

void __ip_connect_check(char *tm,char **retmsg)
{
	char scurrPortnumber[20];
	sprintf( scurrPortnumber,"%d", currPortNumber);

	// Get the parameters off the iRIS stack
	if(strlen(scurrPortnumber))
		__tcp_connect("1000", "40", "10", scurrPortnumber, currIPAddress , currSDNS, currPDNS, currGateway, currOwnIPAddress , currHeader, retmsg);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()IP_CONNECT
//
// DESCRIPTION:	Establish an IP connection
//
// PARAMETERS:	None
//
// RETURNS:		None
//-------------------------------------------------------------------------------------------
//
//

void __ip_connect(const char* timeout, const char* sdns, const char *pdns, const char *gw, const char * oip, char **retmsg)
{
	// Get the parameters off the iRIS stack
	bool change = false;

	// Set the communication structure...
	memset(&comms, 0, sizeof(comms));
	comms.wHandle = 0xFFFF;
	comms.eConnectionType = E_CONNECTION_TYPE_IP_SETUP;

	// Set the connection timeout
	if (timeout && timeout[0])
		comms.bConnectionTimeout = comms.bResponseTimeout = atoi(timeout);
	else
		comms.bConnectionTimeout = comms.bResponseTimeout = 30;

	// Set the IP Address
	strcpy(comms.ownIpAddress , oip?oip:"");
	if (strcmp(comms.ownIpAddress, currOwnIPAddress)) change = true;
	strcpy(comms.gateway , gw?gw:"");
	if (strcmp(comms.gateway, currGateway)) change = true;
	strcpy(comms.pdns , pdns?pdns:"");
	if (strcmp(comms.pdns, currPDNS)) change = true;
	strcpy(comms.sdns , sdns?sdns:"");
	if (strcmp(comms.sdns, currSDNS)) change = true;

	// If already connected...
	if (change)
	{
		if (currHandle != 0xFFFF)
		{
			// If the parameters have changed, disconnect first then connect
			comms.wHandle = currHandle;
			__tcp_disconnect_do();
		}
#ifdef __VX670
		else if (currOwnIPAddress[0] && strcmp(comms.ownIpAddress, currOwnIPAddress))
			__tcp_disconnect_do();
#endif
		retVal = Comms(E_COMMS_FUNC_CONNECT, &comms);
		//__timer_start(1);
	}

//	if (retVal == ERR_COMMS_NONE)
	{
		strcpy(currOwnIPAddress, comms.ownIpAddress);
		strcpy(currGateway, comms.gateway);
		strcpy(currPDNS, comms.pdns);
		strcpy(currSDNS, comms.sdns);
	}

	if (retVal == ERR_COMMS_NONE)
		currHandle = comms.wHandle;

	// Clear the stack and return the error description
	//Dwarika .. Westpack Vx680
	//*retmsg = CommsErrorDesc(retVal);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()TCP_CONNECT
//
// DESCRIPTION:	Connect the PSTN / Modem
//
// PARAMETERS:	None
//
// RETURNS:		Sets the return value in retVal
//-------------------------------------------------------------------------------------------
//

void __tcp_connect(const char * bufSize, const char * interCharTimeout, const char * timeout, const char * port, const char * hip, const char *sdns, const char * pdns, const char *gw, const char* oip , const char * header, char **perrmsg)
{

	bool change = false;

	// Set the communication structure...
	memset(&comms, 0, sizeof(comms));
	comms.wHandle = 0xFFFF;
	comms.eConnectionType = E_CONNECTION_TYPE_IP;
	if (header && header[0] >= '1' && header[0] <= '9')
		comms.eHeader = atoi(header);

	// Set the connection timeout
	if (timeout && timeout[0])
		comms.bConnectionTimeout = comms.bResponseTimeout = atoi(timeout);
	else
		comms.bConnectionTimeout = comms.bResponseTimeout = 30;

	// Set the inter-character timeout
	if (interCharTimeout && interCharTimeout[0])
		comms.dwInterCharTimeout = atol(interCharTimeout);
	else
		comms.dwInterCharTimeout = 3000;

	// Set own IP Address or leave as is
	strcpy(comms.ownIpAddress , oip?oip:"");
	if (strcmp(comms.ownIpAddress, currOwnIPAddress)) change = true;

	// Set gateway or leave as is
	strcpy(comms.gateway , gw?gw:"");
	if (strcmp(comms.gateway, currGateway)) change = true;

	// Set up primary DNS or leave as is
	strcpy(comms.pdns , pdns?pdns:"");
	if (strcmp(comms.pdns, currPDNS)) change = true;

	// Set up secondary DNS or leave as is
	strcpy(	comms.sdns , sdns?sdns:"");
	if (strcmp(comms.sdns, currSDNS)) change = true;

	// Set up the host IP address
	strcpy(comms.ipAddress , hip?hip:"");
	if (strcmp(comms.ipAddress, currIPAddress)) change = true;

	// Set the tcp port number
	if (port)
	{
		comms.wPortNumber = atoi(port);
		if (comms.wPortNumber != currPortNumber) change = true;
	}

	// Set the buffer size
	if (bufSize && bufSize[0])
		bufLen = atol(bufSize);
	if (bufLen < 300) bufLen = 300;

	// If already connected...
	if (currHandle != 0xFFFF)
	{
		// If the parameters have changed, disconnect first then connect
		long connTime_ip = __timer_stop(0) -  60000;
		bool started = __timer_started(1);

		if ( connTime_ip > 0 || !started ) { //TODO
			comms.wHandle = currHandle;
#ifdef __VX670
			__tcp_disconnect_ip_only();
#else
			__tcp_disconnect_do();
#endif
			retVal = Comms(E_COMMS_FUNC_CONNECT, &comms);
			__timer_start(0);
		}
		else if (change || (retVal != ERR_COMMS_NONE && retVal != ERR_COMMS_RECEIVE_TIMEOUT))
		{
			comms.wHandle = currHandle;
			// No need to disconnect if already in error since we have already disconnected
			if (retVal == ERR_COMMS_NONE || retVal == ERR_COMMS_RECEIVE_TIMEOUT)
				__tcp_disconnect_do();
			retVal = Comms(E_COMMS_FUNC_CONNECT, &comms);
			__timer_start(0);
		}

		// If no change, report all is connected OK.
		else
		{
			retVal = ERR_COMMS_NONE;
			comms.wHandle = currHandle;
		}
	}
	// Initiate the connection
	else
	{
#ifdef __VX670
	//Dwarika .. Westpack Vx680
		//__tcp_disconnect_do();
#endif
		retVal = Comms(E_COMMS_FUNC_CONNECT, &comms);
		__timer_start(0);
//		retVal = ERR_COMMS_RECEIVE_TIMEOUT;
	}

	// Arm the timer
	if (retVal == ERR_COMMS_NONE)
	{
		strcpy(currOwnIPAddress, comms.ownIpAddress);
		strcpy(currGateway, comms.gateway);
		strcpy(currPDNS, comms.pdns);
		strcpy(currSDNS, comms.sdns);
		strcpy(currIPAddress, comms.ipAddress);
		if(comms.eHeader) sprintf(currHeader,"%d", comms.eHeader);
		currPortNumber = comms.wPortNumber;
		currHandle = comms.wHandle;
		*perrmsg = "NOERROR";
	}
	else if(comms.pErrmsg)
		*perrmsg = comms.pErrmsg;
	else 
		*perrmsg = "comm error";

	//Dwarika .. Westpack Vx680
	//*perrmsg = CommsErrorDesc(retVal);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()TCP_SEND
//
// DESCRIPTION:	Sends the supplied buffer via TCP
//
// PARAMETERS:	None
//
// RETURNS:		Sets the return value in retVal
//-------------------------------------------------------------------------------------------
//


void __tcp_send(const char* data,char*retMsg)
{
	IRIS_CommsSend(data,&comms, &retVal,retMsg);

#ifdef __VX670
	if (retVal != ERR_COMMS_NONE) __tcp_disconnect_completely();
#endif
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()TCP_RECV
//
// DESCRIPTION:	Receive data from the PSTN / Modem
//
// PARAMETERS:	None
//
// RETURNS:		The current time in strunc tm format
//-------------------------------------------------------------------------------------------
//

void __tcp_recv(char **pmsg,const char* interCharTimeout, const char *timeout, char * errmsg)
{
	IRIS_CommsRecv(&comms, interCharTimeout,timeout, bufLen, &retVal, pmsg, errmsg);

#ifdef __VX670
	if (retVal != ERR_COMMS_NONE ) __tcp_disconnect_completely();
#endif
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()TCP_DISCONNECT
//
// DESCRIPTION:	Disconnect the TCP/IP communication port
//
// PARAMETERS:	None
//
// RETURNS:		The current time in strunc tm format
//-------------------------------------------------------------------------------------------
//
void __tcp_disconnect(void)
{
	if (retVal != ERR_COMMS_NONE && retVal != ERR_COMMS_RECEIVE_TIMEOUT)
		__tcp_disconnect_do();

#ifdef __VX670
	comms.fFastConnect = true;
#endif

}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()TCP_DISCONNECT_NOW
//
// DESCRIPTION:	Disconnect the TCP/IP communication port
//
// PARAMETERS:	None
//
// RETURNS:		The current time in strunc tm format
//-------------------------------------------------------------------------------------------
//
void __tcp_disconnect_now(void)
{
	__tcp_disconnect_do();

}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : __tcp_disconnect_do
//
// DESCRIPTION:	Actually disconnect the TCP/IP communication port
//
// PARAMETERS:	None
//
// RETURNS:		The current time in strunc tm format
//-------------------------------------------------------------------------------------------
//
void __tcp_disconnect_do(void)
{
	Comms(E_COMMS_FUNC_DISCONNECT, &comms);
	currHandle = comms.wHandle = 0xFFFF;
}

#ifdef __VX670
//
//-------------------------------------------------------------------------------------------
// FUNCTION   : __tcp_disconnect_ip_only
//
// DESCRIPTION:	Actually disconnect the TCP/IP communication port
//
// PARAMETERS:	None
//
// RETURNS:		The current time in strunc tm format
//-------------------------------------------------------------------------------------------
//
int __tcp_disconnect_ip_only(void)
{
	// Indicate that we need a faster connection next time.... This will keep the GPRS connection alive but shut down the IP connection.
	comms.fFastConnect = true;

	__tcp_disconnect_do();
	return(0);
}
#endif

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : __tcp_disconnect_ip_only
//
// DESCRIPTION:	Actually disconnect the TCP/IP communication port
//
// PARAMETERS:	None
//
// RETURNS:		The current time in strunc tm format
//-------------------------------------------------------------------------------------------
//
int __tcp_disconnect_completely(void)
{
	// Indicate that we want to netdisconnect as well....This is for VMAC deactivation purposes....
	comms.fSync = true;

	__tcp_disconnect_do();
	return(0);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : __tcp_disconnect_check
//
// DESCRIPTION:	Check if we should actually disconnect the TCP/IP communication port
//
// PARAMETERS:	None
//
// RETURNS:		None
//-------------------------------------------------------------------------------------------
//
int __tcp_disconnect_check(void)
{
#ifdef __VX670
	__tcp_disconnect_ip_only();
#endif
	return(0);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : __tcp_disconnect_extend
//
// DESCRIPTION:	Extends the disconnection timer. Used when performing PSTN sessions in the middle
//				of a TCP session.
//
// PARAMETERS:	None
//
// RETURNS:		None
//-------------------------------------------------------------------------------------------
//
int __tcp_disconnect_extend(void)
{
	return(0);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()TCP_GPRS_STS
//
// DESCRIPTION:	Returns a description of the current TCP error
//
// PARAMETERS:	None
//
// RETURNS:		Error description
//-------------------------------------------------------------------------------------------
//
void __tcp_gprs_sts(void)
{
	Comms(E_COMMS_FUNC_DISP_GPRS_STS, &comms);
}
