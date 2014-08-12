//
//-----------------------------------------------------------------------------
// PROJECT:			iRIS
//
// FILE NAME:       iriscomms.c
//
// DATE CREATED:    5 March 2008
//
// AUTHOR:          Tareq Hafez
//
// DESCRIPTION:     This module supports generic communication functions
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
#include "alloc.h"
#include "comms.h"
#include "utility.h"
#include "iris.h"
#include "iriscomms.h"

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

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
// COMMS FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : IRIS_CommsSend
//
// DESCRIPTION:	Sends the supplied buffer via the communication port
//
// PARAMETERS:	comms	<=	A communication structure
//				retVal	=>	The result is stored here
//
// RETURNS:		Sets the return value in retVal
//-------------------------------------------------------------------------------------------
//

void IRIS_CommsSend(const char *data,T_COMMS * comms, int * retVal,char*retMsg)
{

	short HeaderLenMax = 50;

	if(retMsg) strcpy(retMsg, "COMM ERROR");
	// If not connected, return an error
	if (comms->wHandle == 0xFFFF)
		*retVal = ERR_COMMS_CONNECT_FAILURE;

	else if (data && strlen(data) >= 2)
	{
		// Get the buffer length in hex bytes
		comms->wLength = strlen(data) / 2;

		// Allocate and fill up the hex buffer
		comms->pbData = my_malloc(comms->wLength+HeaderLenMax);
		UtilStringToHex(data, comms->wLength * 2, comms->pbData);

		
		if ((*retVal = Comms(E_COMMS_FUNC_SEND, comms)) != ERR_COMMS_NONE)
		{
			//Dwarika ..
			//Comms(E_COMMS_FUNC_DISCONNECT, comms);
		}
		else
		{
			if(retMsg) strcpy(retMsg, "NOERROR");
		}
			

		// Release the transmit buffer as it is not needed now
		UtilStrDup((char **) &comms->pbData, NULL);
	} else {
		*retVal = ERR_COMMS_GENERAL;
	}

	//if(retMsg) strcpy(retMsg, CommsErrorDesc(*retVal)); //dwarika 680
	// Clear the stack and return the error description
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : IRIS_CommsRecv
//
// DESCRIPTION:	Receive data from the communication port
//
// PARAMETERS:	None
//
// RETURNS:		The current time in strunc tm format
//-------------------------------------------------------------------------------------------
//

void IRIS_CommsRecv(T_COMMS * comms, const char* interCharTimeout, const char* timeout, const int bufLen, int * retVal,char** pstring,char* retMsg)
{
	// If not connected, return an error
	if (comms->wHandle == 0xFFFF)
	{
		*retVal = ERR_COMMS_CONNECT_FAILURE;

		// Clear the stack and return
		//if(retMsg) strcpy(retMsg, CommsErrorDesc(*retVal)); //Dwarika 680
		return;
	}

	// Override the timeout if requested
	if (timeout && timeout[0])
		comms->bResponseTimeout = atoi(timeout);

	// Update the intercharacter timeout if requested
	if (interCharTimeout && interCharTimeout[0])
		comms->dwInterCharTimeout = atol(interCharTimeout);

	// Allocate the receive buffer and set the maximum receive length
	comms->pbData = my_malloc(bufLen);
	comms->wLength = bufLen;

	// Go ahead..receive...
	
	if ((*retVal = Comms(E_COMMS_FUNC_RECEIVE, comms)) != ERR_COMMS_NONE)
	{
		if (*retVal != ERR_COMMS_RECEIVE_TIMEOUT) {
			Comms(E_COMMS_FUNC_DISCONNECT, comms);
		}
	}
	

	// Return the received data or empty if error
	if (*retVal == ERR_COMMS_NONE && comms->wLength)
	{
		*pstring = my_malloc(comms->wLength * 2 + 1);
		UtilHexToString((const char *)comms->pbData, comms->wLength, *pstring);
		//*pstring[comms->wLength * 2] = 0;
	}
	//else *pstring = NULL;
	if( comms->pbData) my_free(comms->pbData);

	// Release the receive buffer as it is not needed now
	//Dwarika .. Vx680
	//if(retMsg) strcpy(retMsg, CommsErrorDesc(*retVal));
    if (retMsg) {
		if( *retVal == ERR_COMMS_NONE) strcpy(retMsg, "NOERROR");
		else if( *retVal == ERR_COMMS_RECEIVE_TIMEOUT) strcpy(retMsg, "TIMEOUT");
		else strcpy(retMsg,"COMM ERROR");
	}

}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : IRIS_CommsDisconnect
//
// DESCRIPTION:	Disconnect the communication port
//
// PARAMETERS:	None
//
// RETURNS:		The current time in strunc tm format
//-------------------------------------------------------------------------------------------
//
void IRIS_CommsDisconnect(T_COMMS * comms, int retVal)
{
	 Comms(E_COMMS_FUNC_DISCONNECT, comms);
}
