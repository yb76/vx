//
//-----------------------------------------------------------------------------
// PROJECT:			iRIS
//
// FILE NAME:       iris2805.c
//
// DATE CREATED:    5 March 2008
//
// AUTHOR:          Tareq Hafez
//
// DESCRIPTION:     This module supports AS2805 functions
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
#include "AS2805.h"
#include "security.h"
#include "utility.h"
#include "iris.h"
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
int AS2805_Error = -1;
//static uchar iv_ofb[8] = "\x01\x23\x45\x67\x89\xAB\xCD\xEF";

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
// AS2805 FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////


//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()AS2805_MAKE
//
// DESCRIPTION:	Creates an AS2805 stream
//
// PARAMETERS:	arrayOfArrays	<=	An simple or complex value that needs to be resolved to a simple value
//									The simple value must finally resolve to an array of arrays. Each array
//									consists of [element {= AS2805 field Number}, element {= AS2805 field value}]
//
//									Each element within the array can be complex or simple again
//
// RETURNS:		The request
//-------------------------------------------------------------------------------------------
//
void __as2805_make(T_AS2805FIELD **flds, char **pmsg)
{
	int i;
	uint length = 0;
	uchar * request;
	char * string;
	T_AS2805FIELD *field = flds[0];
	char macvalue[17]="0000000000000000";

	// Set the AS2805 error to none
	AS2805_Error = -1;

	SecuritySetIV(NULL);

	// Resolve element to a single value string
	if (flds)
	{
		// Initialise the AS2805 buffer with maximum of 1000 bytes.....This should be plenty.
		request = AS2805Init(1000);

		for (i=0;field;i++)
		{

			uchar fieldNum ;
			unsigned char * fieldValue ;

            field = flds[i];
			if(field==NULL) break;
			fieldNum = field->fieldnum;
			fieldValue = field->data;

			// If the field number is 64 or 128 (MAC location), then set the bit to ensure MAC calculation is OK.
			if (fieldNum == 64 || fieldNum == 128) {
				if( strncmp((const char*)fieldValue ,"KEY=",4) == 0) {
					uchar *skey = fieldValue + 4;
					uint as_length;
					uchar * as_request ;

					AS2805SetBit(fieldNum);
					as_request = AS2805Position(&as_length);
					__mac( (const char *)as_request,"",(const char *)skey,as_length,true, macvalue );
					fieldValue = (unsigned char *)macvalue;
				}

			}

			// Pack the value
			length = AS2805Pack(fieldNum, (char *)fieldValue);
		}
	}

	if (length == 0)
	{
		*pmsg = NULL;
		return;
	}

	// Convert the request buffer to ASCII Hex
	string = my_malloc(length * 2 + 1);
	UtilHexToString((const char *)request, length, string);

	// We got our message now, release internal buffers
	AS2805Close();

	// return with the request buffer as a string
	*pmsg = string;
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()AS2805_MAKE_CUSTOM
//
// DESCRIPTION:	Creates a custom AS2805 stream - no bitmaps
//
// PARAMETERS:	element	<=	An simple of complex element that needs to be resolved to a simple element
//							The simple element must finally resolve to an array of arrays. Each array
//							consists of [element {= AS2805 field Number}, element {= AS2805 field value}]
//
//							Each element within the array can be complex or simple again
//
// RETURNS:		The request
//-------------------------------------------------------------------------------------------
//

void __as2805_make_custom(T_AS2805FIELD **flds, char **pmsg)
{
	int i;
	uint length = 0;
	uchar * request = NULL;
	char * string;
	T_AS2805FIELD *field = flds[0];

	// Set the AS2805 error to none
	AS2805_Error = -1;

	// Resolve element to a single value string
	if (flds)
	{

		// Initialise the custom AS2805 buffer with maximum of 1000 bytes.....This should be plenty.
		request = my_malloc(1000);

		// For each array within the main array
		for (i=0;field;i++)
		{
			uchar formatType=0;
			uint fieldSize;
			uchar * fieldValue;
			char * format;

            field = flds[i];
			if(field==NULL) break;

			format = field->type;
			fieldSize = field->fieldnum;
			fieldValue = field->data;

			// Get the field number

			if (strncmp(format, "a", 1) == 0) formatType = C_STRING;

			//else if (strcmp(format, "x+n") == 0) formatType = C_AMOUNT;
			else if (strcmp(format, "m") == 0) formatType = C_AMOUNT;
			else if (format[0] == 'z') formatType = C_LLNVAR;
			else if (format[0] == 'n') formatType = C_BCD;
			else if (format[0] == 'N') formatType = C_BCD_LINK;
			else if (format[0] == 'b') formatType = C_BITMAP;

			// Pack the value
			AS2805BufferPack((char *)fieldValue, formatType, fieldSize, request, &length);

		}

	}

	// Handle an empty message request situation
	if (length == 0)
	{
		*pmsg = NULL;
		return;
	}

	// Convert the request buffer to ASCII Hex
	string = my_malloc(length * 2 + 1);
	UtilHexToString((const char *)request, length, string);
	if (request) my_free(request);

	// We got our message now, release internal buffers
	AS2805Close();

	// return with the request buffer as a string
	*pmsg = string;
}


//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()AS2805_BREAK
//
// DESCRIPTION:	Creates an AS2805 stream
//
// PARAMETERS:	arrayOfArrays	<=	An simple or complex value that needs to be resolved to a simple value
//									The simple value must finally resolve to an array of arrays. Each array
//									consists of [element {= AS2805 field Number}, element {= AS2805 field value}]
//
//									Each element within the array can be complex or simple again
//
// RETURNS:		The request
//-------------------------------------------------------------------------------------------
//
void __as2805_break(int fldcnt,const char *msg,T_AS2805FIELD *flds , int *errfld)
{
	int i;
	uint length;
	uchar * response;
	const char * data = msg;

	// Convert the response to hex

	if(msg==NULL || strlen(data)==0 ||flds==NULL) {
		*errfld = 1;
		return;
	}

	length = strlen(data)/2;
	response = my_malloc(length + 200);		// To cover ill-formatted messages
	UtilStringToHex(data, strlen(data), response);

	// Set the AS2805 error to none
	AS2805_Error = -1;

	// Resolve element to a single value string
	for (i=0;i<fldcnt;i++)
	{
			uchar fieldNum;
			uchar * fieldValue = NULL;
			int operation;

			operation = flds[i].action;
			fieldNum = flds[i].fieldnum;

			// Allocate enough space for the field value
			if (operation == C_GET || operation == C_GETS   )
			{
				fieldValue = my_malloc(2000);
				fieldValue[0] = '\0';
				AS2805Unpack(fieldNum, (char *)fieldValue, response, length);
				flds[i].data = fieldValue;
			}
	}

	// We got our message now, release internal buffers
	AS2805Close();
	my_free(response);

	// Return the result. If -1, no error. If a positive number then this is the field number that had the error during a CHK operation
	*errfld = AS2805_Error;
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()AS2805_BREAK_CUSTOM
//
// DESCRIPTION:	Creates a custom AS2805 stream - no bitmaps
//
// PARAMETERS:	element	<=	An simple of complex element that needs to be resolved to a simple element
//							The simple element must finally resolve to an array of arrays. Each array
//							consists of [element {= AS2805 field Number}, element {= AS2805 field value}]
//
//							Each element within the array can be complex or simple again
//
// RETURNS:		The request
//-------------------------------------------------------------------------------------------
//

void __as2805_break_custom(const char *msg,T_AS2805FIELD **flds, int *errfld)
{
	int i;
	uint respLength;
	uint length = 0;
	char temp[10];
	uchar * response;
	const char * data = msg;
	bool tooLong = false;
	T_AS2805FIELD *field = flds[0];

	// Convert the response to hex
	respLength = strlen(data) / 2;
	response = my_malloc(respLength + 200);		// To cover ill formatted messages
	UtilStringToHex(data, strlen(data), response);

	// Set the AS2805 error to none
	AS2805_Error = -1;

	// Resolve element to a single value string
	if (flds)
	{
		for (i=0;!tooLong && AS2805_Error==-1 && field;i++)
		{
			uchar formatType=0;
			int fieldSize;
			char * fieldValue = NULL;
			char * format;
			int operation;

            field = flds[i];
			if(field==NULL) break;

			operation = field->action;
			format = field->type;
			fieldSize = field->fieldnum;

			// Get the field number
			if (strcmp(format, "LLNVAR") == 0) formatType = C_LLNVAR;
			else if (strcmp(format, "LLAVAR") == 0) formatType = C_LLAVAR;
			else if (strcmp(format, "LLLNVAR") == 0) formatType = C_LLLNVAR;
			else if (strcmp(format, "LLLVAR") == 0) formatType = C_LLLVAR;
			else if (strncmp(format, "an", 2) == 0) formatType = C_STRING;
			else if (strncmp(format, "a", 1) == 0) formatType = C_STRING;
			else if (strcmp(format, "ns") == 0) formatType = C_STRING;
			else if (strcmp(format, "x+n") == 0) formatType = C_AMOUNT;
			else if (format[0] == 'z') formatType = C_LLNVAR;
			else if (format[0] == 'n') formatType = C_BCD;
			else if (format[0] == 'N') formatType = C_BCD_LINK;
			else if (format[0] == 'b') formatType = C_BITMAP;

			// Allocate enough space for the field value
			fieldValue = my_malloc(2000);
			fieldValue[0] = '\0';

			// Unpack the value
			// fetch the left message
			if(fieldSize == -1) fieldSize = respLength - length;
			AS2805BufferUnpack(fieldValue, formatType, fieldSize, response, &length);

			// Make sure we have not gone past the response field
			if (length > respLength) {
				tooLong = true;
				if(fieldValue) my_free(fieldValue);
			}
			else if (operation == C_GET || operation == C_GETS )
			{
				field->data = (unsigned char *)fieldValue;
			}
		}
	}


	// We got our message now, release internal buffers
	AS2805Close();
	my_free(response);

	// Return the result. If -1, no error. If a positive number then this is the field number that had the error during a CHK operation
	sprintf(temp, "%d", AS2805_Error);
	*errfld = AS2805_Error;
}
