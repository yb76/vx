//
//-----------------------------------------------------------------------------
// PROJECT:			iRIS
//
// FILE NAME:       irispstn.c
//
// DATE CREATED:    5 March 2008
//
// AUTHOR:          Tareq Hafez
//
// DESCRIPTION:     This module supports PSTN communication functions
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
#include "comms.h"
#include "utility.h"
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
#ifndef __VX670
static T_COMMS comms;
static int bufLen = 300;
static int retVal = ERR_COMMS_NONE;
#endif


//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
// PSTN FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

void __pstn_init(void)
{
#ifndef __VX670
	comms.wHandle = 0xFFFF;
#endif
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()PSTN_CONNECT
//
// DESCRIPTION:	Connect the PSTN / Modem
//
// PARAMETERS:	None
//
// RETURNS:		Sets the return value in retVal
//-------------------------------------------------------------------------------------------
//

char* __pstn_connect(const char *bufSize,const char *baud,const char *interCharTimeout,const char *timeout,const char *phoneNo,const char *pabx,const char *fastConnect,const char *blindDial,const char *dialType,const char *sync,const char *preDial,const char* header)
{

#ifndef __VX670
	// If we are already connected, disconnect first
	if (comms.wHandle != 0xFFFF)
	{
		__tcp_disconnect_extend();
		Comms(E_COMMS_FUNC_DISCONNECT, &comms);
		comms.wHandle = 0xFFFF;
	}

	// Set the communication structure...
	memset(&comms, 0, sizeof(comms));
	comms.wHandle = 0xFFFF;
	comms.eConnectionType = E_CONNECTION_TYPE_PSTN;


	if( header && header[0] )
		comms.eHeader = atoi(header);
	else
		comms.eHeader = E_HEADER_NONE;

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

	// Set the baud rate
	if (baud)
	{
		if (strcmp(baud, "2400") == 0)
			comms.bBaudRate = COMMS_2400;
		else if (strcmp(baud, "4800") == 0)
			comms.bBaudRate = COMMS_4800;
		else if (strcmp(baud, "9600") == 0)
			comms.bBaudRate = COMMS_9600;
		else if (strcmp(baud, "19200") == 0)
			comms.bBaudRate = COMMS_19200;
		else if (strcmp(baud, "28800") == 0)
			comms.bBaudRate = COMMS_28800;
		else if (strcmp(baud, "57600") == 0)
			comms.bBaudRate = COMMS_57600;
		else if (strcmp(baud, "115200") == 0)
			comms.bBaudRate = COMMS_115200;
		else
			comms.bBaudRate = COMMS_1200;
	}
	else
		comms.bBaudRate = COMMS_1200;

	// Set the synchronous mode
	if (sync && sync[0] == '1')
		comms.fSync = true;

	// Set the dial type
	if (dialType && dialType[0] == '1')
		comms.tDialType = 'T';
	else
		comms.tDialType = 'P';

	// Set fast connect (quick dial)
	if (fastConnect && fastConnect[0] == '1')
		comms.fFastConnect = true;

	// Set blind dial
	if (blindDial && blindDial[0] == '1')
		comms.fBlindDial = true;

	// Set PABX
	strcpy( comms.ptDialPrefix , pabx?pabx:"");

	// Set the phone number
	strcpy(comms.ptPhoneNumber , phoneNo?phoneNo:"");

	// Set the predial option
	if (preDial && preDial[0] == '1')
		comms.fPreDial = true;

	// Pre-size the receiving buffer as requested by the application
	if (bufSize && bufSize[0])
		bufLen = atol(bufSize);
	if (bufLen < 300) bufLen = 300;

	// Initiate the connection
	__tcp_disconnect_extend();
	retVal = Comms(E_COMMS_FUNC_CONNECT, &comms);
	return(CommsErrorDesc(retVal));
#else
	return("");
#endif
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()PSTN_SEND
//
// DESCRIPTION:	Sends the supplied buffer via PSTN / Modem
//
// PARAMETERS:	None
//
// RETURNS:		Sets the return value in retVal
//-------------------------------------------------------------------------------------------
//

void __pstn_send(const char* data,char*retMsg)
{
#ifndef __VX670
	__tcp_disconnect_extend();
	IRIS_CommsSend(data,&comms, &retVal,retMsg);
#endif
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()PSTN_RECV
//
// DESCRIPTION:	Receive data from the PSTN / Modem
//
// PARAMETERS:	None
//
// RETURNS:		The current time in strunc tm format
//-------------------------------------------------------------------------------------------
//
void __pstn_recv(char **pmsg,const char* interCharTimeout, const char *timeout, char * errmsg)
{
#ifndef __VX670
	__tcp_disconnect_extend();
	IRIS_CommsRecv(&comms, interCharTimeout,timeout, bufLen, &retVal, pmsg, errmsg);
#endif

}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()PSTN_DISCONNECT
//
// DESCRIPTION:	Disconnect the PSTN / Modem
//
// PARAMETERS:	None
//
// RETURNS:		The current time in strunc tm format
//-------------------------------------------------------------------------------------------
//
//

void __pstn_disconnect(void)
{
#ifndef __VX670
	if (comms.wHandle != 0xFFFF)
	{
		__tcp_disconnect_extend();
		Comms(E_COMMS_FUNC_DISCONNECT, &comms);	
		comms.wHandle = 0xFFFF;
	}
#endif
}


//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()PSTN_WAIT
//
// DESCRIPTION:	Wait for the PSTN connection
//
// PARAMETERS:	None
//
// RETURNS:		The current time in strunc tm format
//-------------------------------------------------------------------------------------------
//

char * __pstn_wait(void)
{
#ifndef __VX670
	retVal = Comms(E_COMMS_FUNC_PSTN_WAIT, &comms);
	return(CommsErrorDesc(retVal));
#else
	return("");
#endif
}
