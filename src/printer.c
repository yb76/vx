/*
**-----------------------------------------------------------------------------
** PROJECT:			IRIS
**
** FILE NAME:		Printer.c
**
** DATE CREATED:	10 July 2007
**
** AUTHOR:			Tareq Hafez
**
** DESCRIPTION:		Printer Interface Functions
**-----------------------------------------------------------------------------
*/

/*
**-----------------------------------------------------------------------------
**
** Include Files
**
**-----------------------------------------------------------------------------
*/

/*
** Standard include files
*/
#include <string.h>
#include <stdlib.h>

/*
** KEF Project include files.
*/
#include <stdio.h>
#include <auris.h>
#include <svc.h>

/*
** Local include files
*/
#include "printer.h"
#include "prtean13.h"
#include "prtean128.h"

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
static uchar heap[20000];

bool fGraphicsStart = false;
ulong dwGraphicsIndex;
ulong dwGraphicsSize;
uint wGraphicsHeight;
uint wGraphicsWidth;
uchar bGraphicsMultiplier;

uchar bEscapeIndex;
uchar bBarcodeIndex;
uchar abBarcode[100];

uint waitSize = 0;

typedef enum
{
	E_PRINT_STATE_DEFAULT = 0,
	E_PRINT_STATE_ESCAPE,

	E_PRINT_STATE_K,
	E_PRINT_STATE_FIRST_9,
	E_PRINT_STATE_FIRST_0,
	E_PRINT_STATE_SECOND_99,
	E_PRINT_STATE_SECOND_90,
	E_PRINT_STATE_SECOND_03,
	E_PRINT_STATE_SECOND_04,

	E_PRINT_STATE_BARCODE_EAN13,
	E_PRINT_STATE_BARCODE_EAN128,
} E_PRINT_STATE;

E_PRINT_STATE eState = E_PRINT_STATE_DEFAULT;

int prtHandle = -1;

static E_PRT_ERR MyPrtConnect(void)
{
#ifndef _DEBUG
	if (prtHandle < 0)
	{
		open_block_t parm;				// structure to fill comm parameters for com port
		prtHandle = open(DEV_COM4, 0);
		if (prtHandle == -1)
			return E_PRT_ERR_CONNECTION_FAILED;

		//Set the Comm Parameters
		memset(&parm,0,sizeof(parm));
		parm.rate      = Rt_19200;			// Santana ITP is always set to 19200 baud
		parm.format    = Fmt_A8N1 | Fmt_auto | Fmt_RTS;	// Santana ITP is always set at 8N1
		parm.protocol  = P_char_mode;
		parm.parameter = 0;
		set_opn_blk( prtHandle, &parm);
	}
#endif

	return E_PRT_ERR_NONE;
}

E_PRT_ERR PrtStatus(bool fStatusCheck)
{
	char status = 0;
#ifndef _DEBUG
	char four[4];

	// Wait until the buffer empties
	while(get_port_status(prtHandle, four));
#endif

	if (fStatusCheck)
	{
#ifndef _DEBUG
		// Request the printer status
		write(prtHandle, "\033d", 2);
		while(read(prtHandle, &status, 1) <= 0);
#else
		//*********************** TESTING ONLY ***********************
//		status = 0x02;
#endif

		if (status & 0x40)	
			return E_PRT_ERR_MECHANISM;
		else if (status & 0x04)	
			return E_PRT_ERR_FIRMWARE;
		else if (status & 0x02)
			return E_PRT_ERR_PAPER_OUT;
	}

	return E_PRT_ERR_NONE;
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : MyPrtSend
**
** DESCRIPTION:	Sends the data to the printer
**
** PARAMETERS:
**
** RETURNS:		None
**
**-----------------------------------------------------------------------------
*/
static E_PRT_ERR MyPrtPrint(uchar * pbData, uint size)
{
	if (pbData)
	{
		memcpy(&heap[waitSize], pbData, size);
		waitSize += size;
	}

	if (waitSize > 4000 || (waitSize && pbData == NULL))
	{
#ifdef _DEBUG
		printf(heap, waitSize);
		memset(heap, 0, sizeof(heap));
#else
		if (write(prtHandle, (char *) heap, waitSize) == -1)
		{
			waitSize = 0;
			return E_PRT_ERR_SEND_FAILED;
		}
#endif

		PrtStatus(false);
		waitSize = 0;
	}

	return E_PRT_ERR_NONE;
}

static E_PRT_ERR MyPrtSend(uchar * pbData, uint wLength)
{
	E_PRT_ERR err;

	for (; wLength; pbData++, wLength--)
	{
		switch(eState)
		{
			case E_PRINT_STATE_ESCAPE:
				if (*pbData == 'G')			// Double width
				{
					if ((err = MyPrtPrint("\036", 1)) != E_PRT_ERR_NONE)
						return err;
					eState = E_PRINT_STATE_DEFAULT;
				}
				else if (*pbData == 'H')	// Normal width
				{
					if ((err = MyPrtPrint("\037", 1)) != E_PRT_ERR_NONE)
						return err;
					eState = E_PRINT_STATE_DEFAULT;
				}
				else if (*pbData == 'h')	// Double height
				{
					if ((err = MyPrtPrint("\021", 1)) != E_PRT_ERR_NONE)
						return err;
					eState = E_PRINT_STATE_DEFAULT;
				}
				else if (*pbData == 'i')	// Inverse printing
				{
					if ((err = MyPrtPrint("\022", 1)) != E_PRT_ERR_NONE)
						return err;
					eState = E_PRINT_STATE_DEFAULT;
				}
				else if (*pbData == 'k')
					eState = E_PRINT_STATE_K;
				break;

			// 'ESC k' processing...
			case E_PRINT_STATE_K:
				if (*pbData == '9')
					eState = E_PRINT_STATE_FIRST_9;
				else if (*pbData == '0')
					eState = E_PRINT_STATE_FIRST_0;
				else eState = E_PRINT_STATE_DEFAULT;
				break;

			// 'ESC k 9' processing...
			case E_PRINT_STATE_FIRST_9:
				if (*pbData == '9')
					eState = E_PRINT_STATE_SECOND_99;
				else if (*pbData == '0')
					eState = E_PRINT_STATE_SECOND_90;
				else eState = E_PRINT_STATE_DEFAULT;
				break;
			case E_PRINT_STATE_SECOND_99:
				if (*pbData == '9')
				{
					if ((err = MyPrtPrint("\030\037\033F24;", 7)) != E_PRT_ERR_NONE)
						return err;
				}
				eState = E_PRINT_STATE_DEFAULT;
				break;
			case E_PRINT_STATE_SECOND_90:
				if (*pbData == '1' || *pbData == '2')
				{
					bBarcodeIndex = 0;
					memset(abBarcode, 0, sizeof(abBarcode));
					if (*pbData == '1')
						eState = E_PRINT_STATE_BARCODE_EAN13;
					else
						eState = E_PRINT_STATE_BARCODE_EAN128;
				}
				else
				{
					// End of EAN128 barcode so print it...
					if (*pbData == '3')
					{
						int width;

						// Empty any normal text first
						if ((err = MyPrtPrint(NULL, 0)) != E_PRT_ERR_NONE)
							return err;

						memset(heap, 0, sizeof(heap));
						if ((width = BuildEan128BarCode(abBarcode, heap, sizeof(heap), 80)) != -1)
						{
							E_PRT_ERR err;

							if ((err = PrtPrintGraphics(width * 8, (width <= 24)? 40:80, heap, true, (uchar) ((width <= 24)? 2:1))) != E_PRT_ERR_NONE)
								return err;
						}
					}

					eState = E_PRINT_STATE_DEFAULT;
				}
				break;

			// 'ESC k 0' processing...
			case E_PRINT_STATE_FIRST_0:
				if (*pbData == '3')
					eState = E_PRINT_STATE_SECOND_03;
				else if (*pbData == '4')
					eState = E_PRINT_STATE_SECOND_04;
				else eState = E_PRINT_STATE_DEFAULT;
				break;
			case E_PRINT_STATE_SECOND_03:
				if (*pbData == '2')
				{
					if ((err = MyPrtPrint("\030\037\033F32;", 7)) != E_PRT_ERR_NONE)
						return err;
				}
				eState = E_PRINT_STATE_DEFAULT;
				break;
			case E_PRINT_STATE_SECOND_04:
				if (*pbData == '2')
				{
					if ((err = MyPrtPrint("\030\037\033F42;", 7)) != E_PRT_ERR_NONE)
						return err;
				}
				eState = E_PRINT_STATE_DEFAULT;
				break;

			// Barcode processing - EAN13...
			case E_PRINT_STATE_BARCODE_EAN13:
				abBarcode[bBarcodeIndex++] = *pbData;
				if (bBarcodeIndex == 12)
				{
					uint size = 5 + (EAN13_PRINT_WIDTH/8) * EAN13_PRINT_HEIGHT;

					// Empty any normal text first
					if ((err = MyPrtPrint(NULL, 0)) != E_PRT_ERR_NONE)
						return err;

					memset(heap, 0, size);
					if (BuildEan13BarCode(abBarcode, heap, size) != -1)
					{
						E_PRT_ERR err;

						uint width = heap[4] * 8;
						uint height = (heap[0] + heap[1] * 256) / heap[4];
						if ((err = PrtPrintGraphics(width, height, &heap[5], true, 1)) != E_PRT_ERR_NONE)
							return err;
					}
					eState = E_PRINT_STATE_DEFAULT;
				}
				break;

			// Barcode processing - EAN128 - STARTING...
			case E_PRINT_STATE_BARCODE_EAN128:
				if (*pbData == '\033')
					eState = E_PRINT_STATE_ESCAPE;
				else
					abBarcode[bBarcodeIndex++] = *pbData;
				break;

			case E_PRINT_STATE_DEFAULT:
			default:
				if (*pbData == '\033')
					eState = E_PRINT_STATE_ESCAPE;
				else if (*pbData == '\021')
				{
					if ((err = MyPrtPrint("\037", 1)) != E_PRT_ERR_NONE)
						return err;
				}
				else
				{
					if ((err = MyPrtPrint(pbData, 1)) != E_PRT_ERR_NONE)
						return err;
				}
				break;
		}
	}

	return E_PRT_ERR_NONE;
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : PrtPrint
**
** DESCRIPTION:
**
** Connects to the printer device, prints the data then disconnects from the printer device
**
** PARAMETERS:
**				wLength	<=	Length of data to print...
**				pbData	<=	Pointer to data for printing...
**
** RETURNS:		None
**
**-----------------------------------------------------------------------------
*/
static E_PRT_ERR PrtPrint(uchar * pbData, uint wLength, E_PRINT ePrintCommand)
{
	E_PRT_ERR myPrintErr;
	uchar retry;
	E_PRT_ERR statusErr;

	// Connect to the printer and send the data. Retry connection failures 3 times
	for (retry = 0; retry < 3; retry++)
	{
		myPrintErr = MyPrtSend(pbData, wLength);
		if (myPrintErr != E_PRT_ERR_CONNECTION_FAILED)
			break;
	}

	// Perform a status check
	if (myPrintErr == E_PRT_ERR_NONE)
	{
		MyPrtPrint(NULL, 0);
		if ((statusErr = PrtStatus(true)) != E_PRT_ERR_NONE)
			myPrintErr = statusErr;
	}

	return myPrintErr;
}

/*
**-------------------------------------------------------------------------------------
** FUNCTION   : PrtGetErrorText
**
** DESCRIPTION: Get the error text
**
** PARAMETERS:	None
**
** RETURNS:		(char *)
**
**-----------------------------------------------------------------------------
*/
char * PrtGetErrorText(E_PRT_ERR eError)
{
	uchar i;
	const struct
	{
		int err;
		char * ptDesc;
	} error[] = {	{E_PRT_ERR_CONNECTION_FAILED,	"CONNECTION FAILURE"},
			{E_PRT_ERR_SEND_FAILED,					"SEND FAILED"},
			{E_PRT_ERR_PAPER_OUT,					"PAPER OUT"},
			{E_PRT_ERR_GRAPHICS,					"GRAPHICS"},
			{E_PRT_ERR_FIRMWARE,					"FIRMWARE"},
			{E_PRT_ERR_MECHANISM,					"MECHANISM FAILURE"},
			{E_PRT_ERR_GENERAL,						"GENERAL"},
			{E_PRT_ERR_NONE,						"OK"}};

	// Search for the error text...
	for(i = 0; error[i].err != eError && error[i].err != E_PRT_ERR_NONE; i++);

	// Return the error text...
	return error[i].ptDesc;
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : PrintBuffer
**
** DESCRIPTION:
**				Buffers the data before printing. Requires a flush at the end
**				Limitation: New buffer must be less than 200 bytes
** PARAMETERS:
**				None
** RETURNS:
**				NOERROR if receipt printed OK.
**-----------------------------------------------------------------------------
*/
E_PRT_ERR PrtPrintBuffer(uint wLength, uchar * pbData, E_PRINT ePrintCommand)
{
	static uchar printBuffer[700];
	static uint printBufferIndex;
	uint index = 0;
	E_PRT_ERR err;
	uint length;

	// Connect if not already connected
	if ((err = MyPrtConnect()) != E_PRT_ERR_NONE)
		return err;

	if (ePrintCommand == E_PRINT_START)
	{
		printBufferIndex = 0;
		eState = E_PRINT_STATE_DEFAULT;
	}

	while (wLength)
	{
		if (wLength >= (sizeof(printBuffer) - printBufferIndex))
			length = sizeof(printBuffer) - printBufferIndex;
		else length = wLength;
		wLength -= length;

		memmove(&printBuffer[printBufferIndex], &pbData[index], length);
		index += length;
		printBufferIndex += length;

		if (printBufferIndex > 500 || ePrintCommand == E_PRINT_END)
		{
			E_PRT_ERR eError;

			if ((eError = PrtPrint(printBuffer, printBufferIndex, ePrintCommand)) != E_PRT_ERR_NONE)
			{
				printBufferIndex = 0;
				return eError;
			}
			printBufferIndex = 0;
		}
	}

	return E_PRT_ERR_NONE;
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : PrintFormFeed
**
** DESCRIPTION:
**				Print the current date and time
** PARAMETERS:
**				None
** RETURNS:
**				NOERROR if receipt printed OK.
**-----------------------------------------------------------------------------
*/
E_PRT_ERR PrtPrintFormFeed()
{
	return PrtPrintBuffer(12, "\033k999\n\n\n\n\n\n", E_PRINT_END);
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : PrtPrintGraphics
**
** DESCRIPTION:	Print the graphics image centred
**				
** PARAMETERS:	None
**				
** RETURNS:		NOERROR if receipt printed OK.
**				
**-----------------------------------------------------------------------------
*/
E_PRT_ERR PrtPrintGraphics(uint wWidth, uint wHeight, uchar * pbData, bool fCenter, uchar bMultiplier)
{
#ifndef _DEBUG
	E_PRT_ERR err;
	uint origWidth = wWidth;
	uint origHeight = wHeight;
	uint fill;
	uchar line[48];
	uchar buffer[500];
	int offset = 0;
	uint bufIndex = 0;
	uint bufLine = 0;

	// If there is some text to print, flush it now
	if (waitSize)
	{
		if ((err = MyPrtPrint(NULL, 0)) != E_PRT_ERR_NONE)
			return err;
	}

	// Adjust the width and height according to the multiplier
	wWidth *= bMultiplier;
	wHeight *= bMultiplier;
	fill = (384 - wWidth) / 16;

	if (fCenter)
		offset = fill * 8;
	fill = 0;

	// Connect if not already connected
	if ((err = MyPrtConnect()) != E_PRT_ERR_NONE)
		return err;

	sprintf((char *) buffer, "\033GL0,0,%d,%d;", wWidth, wHeight);
	write(prtHandle, (char *) buffer, strlen((char *)buffer));

	// Center the graphics if required
	{
		uint i, j, k, bitmap, bitmap2, origFill = fill;

		for (i = 0; i < origHeight; i++)
		{
			bitmap2 = 0x80;
			fill = origFill;
			memset(line, 0, sizeof(line));
			for (j = 0; j < origWidth/8; j++)
			{
				for (bitmap = 0x80; bitmap; bitmap >>= 1)
				{
					for (k = 0; k < bMultiplier; k++, bitmap2 >>= 1)
					{
						if (bitmap2 == 0) bitmap2 = 0x80, fill++;
						if (pbData[(i*origWidth/8)+j] & bitmap) line[fill] |= bitmap2;
					}
				}
			}

			for (k = 0; k < bMultiplier; k++)
			{
				memcpy(&buffer[bufIndex], line, fill+1);
				bufIndex += (fill + 1);
				bufLine++;
				if (bufIndex > (sizeof(buffer)-50))
				{
					write(prtHandle, (char *) buffer, bufIndex);
					if ((err = PrtStatus(false)) != E_PRT_ERR_NONE)
						return err;
					bufIndex = bufLine = 0;
				}
			}
		}

		if (bufIndex)
		{
			write(prtHandle, (char *) buffer, bufIndex);
			if ((err = PrtStatus(false)) != E_PRT_ERR_NONE)
				return err;
		}

	}

	sprintf((char *) buffer, "\033GP0,%d;", offset);
	write(prtHandle, (char *) buffer, strlen((char *) buffer));
	if ((err = PrtStatus(true)) != E_PRT_ERR_NONE)
		return err;
#else
	DispGraphics2(pbData, wWidth, wHeight);
#endif
	return E_PRT_ERR_NONE;
}
