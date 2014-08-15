/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       comms.c
**
** DATE CREATED:    10 July  2007
**
** AUTHOR:          Tareq Hafez
**
** DESCRIPTION:     This module contains the communication functions
**-----------------------------------------------------------------------------
*/

/*
**-----------------------------------------------------------------------------
** Include Files
**-----------------------------------------------------------------------------
*/

#ifdef __VMAC
#include "logsys.h"
#endif

/*
** Standard include files.
*/
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <ctype.h>

/*
** KEF Project include files.
*/
#include <svc.h>

#include <errno.h>


/*
** Local include files
*/
#include "auris.h"
#include "alloc.h"
#include "display.h"
#include "timer.h"
#include "comms.h"


/*
**-----------------------------------------------------------------------------
** Constants
**-----------------------------------------------------------------------------
*/

/*
**-----------------------------------------------------------------------------
** Module variable definitions and initialisations.
**-----------------------------------------------------------------------------
*/

static uint wSerialPortHandle[2] = {0xFFFF, 0xFFFF};

char model[13]="";
char partNo[13];

bool uart_open[2] = {false, false};

int iphandle = -1;

//#define DispText2(a,b,c,d,e,f)	LOG_PRINTF((a));
#define DispText2(a,b,c,d,e,f)	;


/*
**-----------------------------------------------------------------------------
** FUNCTION   : CommsSerialDataAvailable
**
** DESCRIPTION: Check if receive data is available
** 
** PARAMETERS:	None
**
** RETURNS:	RECEIVE_EMPTY		if receive buffer is empty
**			RECEIVE_READY		if receive buffer has data
**			ERR_COMMS_CONNECT_FAILURE	if connection is not up.
**
**-----------------------------------------------------------------------------
*/
static uint CommsSerialDataAvailable(int port)
{
	char four[4];

	if (wSerialPortHandle[port] == 0xFFFF || get_port_status(wSerialPortHandle[port], four) < 0)
		return 0;

	if (four[0])
		return 1;

	return 2;
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : CommsTranslateError
**
** DESCRIPTION: Translate the local error to an FPOZ error
**
** PARAMETERS:	NONE
**
** RETURNS:		NONE
**
**-----------------------------------------------------------------------------
*/

uchar TranslateBaudRate(uchar bBaudRate)
{
	int i;
	static struct
	{
		uchar fromBaudRate;
		uchar toBaudRate;
	} baudTable[] = {	{COMMS_300, Rt_300},	{COMMS_600, Rt_600},	{COMMS_1200, Rt_1200},	{COMMS_2400, Rt_2400},
						{COMMS_4800, Rt_4800},	{COMMS_9600, Rt_9600},	{COMMS_12000, Rt_12000},{COMMS_14400, Rt_14400},
						{COMMS_19200, Rt_19200},{COMMS_28800, Rt_28800},{COMMS_38400, Rt_38400},{COMMS_57600, Rt_57600},
						{COMMS_115200, Rt_115200}
					};

	// Find the true baud rate number for Verifone first
	for (i = 0; i < COMMS_BAUDRATE_MAX_COUNT; i++)
	{
		if (bBaudRate == baudTable[i].fromBaudRate)
			return baudTable[i].toBaudRate;
	}

	return Rt_115200;
}


/*
**-----------------------------------------------------------------------------
** FUNCTION   : CommsSetSerial
**
** DESCRIPTION: Changes parameters of an open communication port
**
** PARAMETERS:	T_COMMS * psComms
**
** RETURNS:		NONE
**
**-----------------------------------------------------------------------------
*/

static uint CommsSetSerial(T_COMMS * psComms)
{
	uchar format;
	struct Opn_Blk Com1ob;

	// Set the correct data bits, parity bits and stop bits
	if (psComms->bDataBits == 7)
	{
		if (psComms->tParityBit == 'E')
			format = Fmt_A7E1;
		else if (psComms->tParityBit == 'O')
			format = Fmt_A7O1;
		else 
			format = Fmt_A7N1;
	}
	else
	{
		if (psComms->tParityBit == 'E')
			format = Fmt_A8E1;
		else if (psComms->tParityBit == 'O')
			format = Fmt_A8O1;
		else 
			format = Fmt_A8N1;
	}

	Com1ob.rate = TranslateBaudRate(psComms->bBaudRate);
    Com1ob.format = format;
	Com1ob.protocol = P_char_mode;
	Com1ob.parameter = 0;
	set_opn_blk(psComms->wHandle, &Com1ob);

	return ERR_COMMS_NONE;
}


/*
**-----------------------------------------------------------------------------
** FUNCTION   : CommsConnect
**
** DESCRIPTION: Start a new file session
**				Assumes a connection has already been established with the host
**
** PARAMETERS:	NONE
**
** RETURNS:		NONE
**
**-----------------------------------------------------------------------------
*/
uint CommsConnect(T_COMMS * psComms)
{
	int serialPort = -1;

	switch (psComms->eConnectionType)
	{
		case E_CONNECTION_TYPE_UART_1:
			serialPort = 0;
			wSerialPortHandle[0] = 0xFFFF;

			if (uart_open[0])
				return ERR_COMMS_NONE;
			psComms->wHandle = open(DEV_COM1, 0);
			uart_open[0] = true;

			CommsSetSerial(psComms);
			wSerialPortHandle[serialPort] = psComms->wHandle;
			break;

		case E_CONNECTION_TYPE_UART_2:
			serialPort = 1;
			wSerialPortHandle[1] = 0xFFFF;

			if (uart_open[1])
				return ERR_COMMS_NONE;
#ifdef __VX670
			psComms->wHandle = open(DEV_COM6, 0);
#else
			psComms->wHandle = open(DEV_COM2, 0);
#endif
			uart_open[1] = true;

			CommsSetSerial(psComms);
			wSerialPortHandle[serialPort] = psComms->wHandle;
			break;

		case E_CONNECTION_TYPE_IP:
			{
				int nRetVal = inCeTcpConnect(psComms);
				if (nRetVal > 0)
				{
					psComms->wHandle = nRetVal;
					return ERR_COMMS_NONE;
				} else return ERR_COMMS_GENERAL;

			}


		default:
			return ERR_COMMS_CONNECT_NOT_SUPPORTED;
	}

	return ERR_COMMS_NONE;
}


/*
**-----------------------------------------------------------------------------
** FUNCTION   : CommsSend
**
** DESCRIPTION: Send data via the modem
** 
** PARAMETERS:	None
**
** RETURNS:	ERR_MDM_CONNECTION_FAILED if the modem connection is lost.
**		ERR_MDM_SEND_FAILED if transmiting the data failed.
**
**-----------------------------------------------------------------------------
*/
uint CommsSend( T_COMMS * psComms )
{
	int nRetVal;
#ifndef __VX670
	short retVal;
#endif
	char four[4];

	// If length = ZERO, just return with SUCCESS
	if (psComms->wLength == 0) return ERR_COMMS_NONE;

	if (psComms->eConnectionType == E_CONNECTION_TYPE_IP)
	{
		char * data = (char *) psComms->pbData;

			nRetVal = inSendTCPCommunication(psComms);
			if(nRetVal > 0)
			{
			}
			else
			{
				return ERR_COMMS_GENERAL;
			}

		if (data != (char *) psComms->pbData) my_free(data);

		DispText2("SEND:Done", 1, 0, true, false, true);
		return ERR_COMMS_NONE;
	}

	// Write the data
	write(psComms->wHandle, (char *) psComms->pbData, psComms->wLength);

	// Wait for it to transmit completely
	while(get_port_status(psComms->wHandle, four) != 0)
	{
		SVC_WAIT(200);
	}

    return ERR_COMMS_NONE;
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : CommsReceive
**
** DESCRIPTION: Receive data via the modem
** 
** PARAMETERS:	None
**
** RETURNS:	ERR_MDM_CONNECTION_FAILED if the modem connection is lost.
**		ERR_MDM_RECEIVE_TIMEOUT	if the response does not arrive in time.
**		ERR_MDM_RECEIVE_FAILED if data reception fails
**
**-----------------------------------------------------------------------------
*/
uint CommsReceive( T_COMMS * psComms, bool fFirstChar)
{
	short retVal=0;
	TIMER_TYPE myTimer;

	// If the length = 0, return with no error
	if (psComms->wLength == 0) {
		return ERR_COMMS_NONE;
	}

	if (psComms->eConnectionType == E_CONNECTION_TYPE_IP)
	{
		//Dwarika .. not used for CE Lib
		/*	
		struct timeval timeout;
		*/
		int length = psComms->wLength;
		char *pdata = (char *)psComms->pbData;
		{
			retVal = inReceiveTCPCommunication(psComms);
			if(retVal > 0)
			{
				pdata = pdata + retVal;
				length = length - retVal;
			}
			else if (retVal == -235)
			{
				return ERR_COMMS_RECEIVE_TIMEOUT;
			}
			else
			{
				return ERR_COMMS_GENERAL;
			}

		}

		psComms->wLength = retVal;
	
		return ERR_COMMS_NONE;
		
	}
	else
	{
		char four[4];
		// Arm the timers
		TimerArm(&myTimer, fFirstChar?psComms->bResponseTimeout * 10000L:psComms->dwInterCharTimeout * 10);

		for(;;)
		{
			int iret = 0;
			iret = get_port_status(psComms->wHandle, four) ;
			if(iret>0) {
				SVC_WAIT(100);
				continue;
			}
			else if (iret<0)
			{
				return ERR_COMMS_RECEIVE_FAILURE;
				
			}

			// Check if there are some data available
			if (four[0] > 0)
			{
				int length;

				length = read(psComms->wHandle, (char *) psComms->pbData, psComms->wLength);

				if (length > 0) {
					psComms->wLength = length;
					return ERR_COMMS_NONE;
				}
				else if (length < 0)
					return ERR_COMMS_RECEIVE_FAILURE;
				else if(length == 0) {
					if(!fFirstChar) 
						return ERR_COMMS_RECEIVE_TIMEOUT;
				}
			}

			// Check that the timer has not expired
			if (TimerExpired(&myTimer) == true)
			{
				psComms->wLength = 0;
				return ERR_COMMS_RECEIVE_TIMEOUT;
			}

			SVC_WAIT(100);
		}
	}
	return ERR_COMMS_NONE;

}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : Comms
**
** DESCRIPTION: Main Handler external communication functions
** 
** PARAMETERS:	None
**
** RETURNS:		Error code
**
**-----------------------------------------------------------------------------
*/
uint Comms(E_COMMS_FUNC eFunc, T_COMMS * psComms)
{
	uint retCode = ERR_COMMS_NONE;
	uchar header[64];
	uchar headerIndex;
	uchar headerLength;
	uchar * data;
	uint dataLength;
	uint msgLength = 0;

	if (model[0] == '\0')
	{
		SVC_INFO_MODELNO(model);
		SVC_INFO_PARTNO(partNo);
	}

	switch (eFunc)
	{
		case E_COMMS_FUNC_CONNECT:
			//Dwarika .. not used for CE Lib
			return CommsConnect(psComms);

		case E_COMMS_FUNC_SEND:
			// Check if a header must be sent first
			switch (psComms->eHeader)
			{
					
				default:
				case E_HEADER_NONE:
				case E_HEADER_SSL:
				case E_HEADER_HTTP:
				case E_HEADER_HTTPS:
					headerLength = 0;
					break;
				case E_HEADER_LENGTH:
                case E_HEADER_SSL_LENGTH:
					header[0] = psComms->wLength / 256;
					header[1] = psComms->wLength % 256;
					//header[0] = (psComms->wLength + 2) / 256;
					//header[1] = (psComms->wLength + 2) % 256;
					headerLength = 2;
					break;
                case E_HEADER_CLNP:
                    headerLength = 0x15; //fixed length:21
                    header[0] = 0x81;
                    header[1] = headerLength;
                    header[2] = 0x01; //version
                    header[3] = 0x13; //lifetime
                    header[4] = 0x1C; //type
                    header[5] = ( psComms->wLength + headerLength)/ 256;
                    header[6] = ( psComms->wLength + headerLength)% 256;
                    header[7] = 0; //checksum
                    header[8] = 0; //checksum
                    //memcpy( &header[9], "\x05\x49\x80\x00\x00\x05",6); //dest address NSAP
                    memcpy( &header[9], "\x05\x49\x80\x00\x00\x03",6); //dest address NSAP DEV1
                    memcpy( &header[15],"\x05\x49\xFF\x8B\x2B\x01",6); //source address NSAP
                    break;
                case E_HEADER_TPDU_20:
                    memcpy( header,"\x60\x00\x20\x00\x00",5);
                    header[5] = psComms->wLength / 256;
                    header[6] = psComms->wLength % 256;
                    headerLength = 7;
					break;
                case E_HEADER_TPDU_30:
                    memcpy( header,"\x60\x00\x30\x00\x00",5);
                    header[5] = psComms->wLength / 256;
                    header[6] = psComms->wLength % 256;
                    headerLength = 7;
					break;
                case E_HEADER_TPDU_WESTPAC:
                    memcpy( header,"\x60\x00\x10\x00\x00",5);
                    headerLength = 5;
					break;
			}


			// If a header must be sent first, send it
			if (headerLength)
			{
				// Shift the main data and add the header data
					memmove(psComms->pbData+headerLength, psComms->pbData, psComms->wLength);
					memcpy(psComms->pbData, header, headerLength);
					psComms->wLength += headerLength;
				return CommsSend(psComms);
			}
			else
			{
				return CommsSend(psComms);
			}

		case E_COMMS_FUNC_RECEIVE:
			// Preserve the maximum number of bytes expected
			dataLength = psComms->wLength;

			// Preserve the start of the receive buffer
			data = psComms->pbData;

			// Set the header length according to the header type
			if (psComms->eHeader == E_HEADER_LENGTH || psComms->eHeader == E_HEADER_SSL_LENGTH )
				headerLength = 2;
			else if (psComms->eHeader == E_HEADER_TPDU_20 || psComms->eHeader == E_HEADER_TPDU_30) 
				headerLength = 7;
			else if (psComms->eHeader == E_HEADER_CLNP) {
				headerLength = 21;
			}
			else
				headerLength = 0;


			// Initialisation. The buffer size must be big enough to receive the header as well
			headerIndex = 0;
			psComms->wLength = dataLength + headerLength;

			// Start receiving characters
			//if( psComms->wLength && retCode == ERR_COMMS_NONE )
			if(psComms->eHeader == E_HEADER_CLNP) {
				retCode = CommsReceive(psComms, true);
				if(retCode==ERR_COMMS_NONE) {
					memmove(psComms->pbData,&psComms->pbData[headerLength],psComms->wLength-headerLength);
					psComms->wLength = psComms->wLength - headerLength ;
				}
				return(retCode);
			}

			// only receive once
			retCode = CommsReceive(psComms, true);
				// Are we still expecting header data?
			for(; psComms->wLength && retCode == ERR_COMMS_NONE; retCode = CommsReceive(psComms, false))
			{
				if (headerLength)
					{
							// Is the header data complete?
						if (psComms->wLength >= headerLength)
						{
								msgLength = psComms->wLength - headerLength;
								memcpy(&header[headerIndex], psComms->pbData, headerLength);
								memmove(data, &psComms->pbData[headerLength], msgLength);

								// Reduce the maximum length if the header has some length information
								if ((psComms->eHeader == E_HEADER_LENGTH || psComms->eHeader == E_HEADER_SSL_LENGTH) && dataLength > (header[0] * 256 + header[1]))
									dataLength = header[0] * 256 + header[1];
								if ((psComms->eHeader == E_HEADER_TPDU_20 || psComms->eHeader == E_HEADER_TPDU_30) && dataLength >(header[5]*256+header[6])  )
									dataLength = header[5] * 256 + header[6];
								if (psComms->eHeader == E_HEADER_CLNP ) {
									dataLength = header[5] * 256 + header[6];
									dataLength = dataLength - headerLength;
								}
								if (dataLength < msgLength) dataLength = msgLength;
								headerIndex = headerLength = 0;
						}
						else
						{
							memcpy(&header[headerIndex], psComms->pbData, psComms->wLength);
							headerIndex += psComms->wLength;
							headerLength -= psComms->wLength;
							psComms->pbData = &data[headerIndex];
						}
					}
				else
					msgLength += psComms->wLength;

				// Adjust the receiving length and receive pointer
				psComms->pbData = &data[headerIndex + msgLength];
				psComms->wLength = dataLength + headerLength - msgLength;
			}


			psComms->pbData = data;
			psComms->wLength = msgLength;
			if (msgLength && retCode == ERR_COMMS_RECEIVE_TIMEOUT)
				retCode = ERR_COMMS_NONE;


			// Limit the data if the length of data > header length
			if ((psComms->eHeader == E_HEADER_LENGTH || psComms->eHeader == E_HEADER_SSL_LENGTH) && psComms->wLength > (header[0] * 256 + header[1]))
				psComms->wLength = header[0] * 256 + header[1];

			return retCode;

		case E_COMMS_FUNC_DISCONNECT:
			return ERR_COMMS_NONE;
		case E_COMMS_FUNC_SERIAL_DATA_AVAILABLE:
			return CommsSerialDataAvailable(0);

		case E_COMMS_FUNC_SERIAL2_DATA_AVAILABLE:
			return CommsSerialDataAvailable(1);

		case E_COMMS_FUNC_SET_SERIAL:
			return ERR_COMMS_NONE;
		case E_COMMS_FUNC_DISP_GPRS_STS:
			return ERR_COMMS_NONE;
		case E_COMMS_FUNC_SIGNAL_STRENGTH:
			return (inCheckGPRSStatus(5,5));	
		//	return 0;
	}

	return ERR_COMMS_FUNC_NOT_SUPPORTED;
}
