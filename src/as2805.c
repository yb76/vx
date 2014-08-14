/*
**-----------------------------------------------------------------------------
** PROJECT:			iRIS
**
** FILE NAME:       as2805.c
**
** DATE CREATED:    30 January 2008
**
** AUTHOR:          Tareq Hafez
**
** DESCRIPTION:     This module supports security functions
**-----------------------------------------------------------------------------
*/

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

/*
** Local include files
*/
#include "alloc.h"
#include "utility.h"
#include "my_time.h"
#include "as2805.h"

/*
**-----------------------------------------------------------------------------
** Type Definitions
**-----------------------------------------------------------------------------
*/
typedef struct
{
	uchar format;
	int size;
} T_FIELD_FORMAT;

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
static uchar * buffer = NULL;
static uint fieldsStart;
static uint fieldsIndex;

static bool leftOver = false;
static int leftOverValue = 0;

static bool bcdLength = true; // false;

static uint maxField = 53;
static uint addField = 0;

static const T_FIELD_FORMAT fieldType[] =
{
	{	C_BCD,		4},		// 0, Message ID

	{	C_BITMAP,	64},	// 1, Secondary bitmap (b64)
	{	C_LLNVAR,	19},	// 2, Primary account number (PAN) (LLVAR n..19)
	{	C_BCD,		6},		// 3, Processing code (n6)
	{	C_BCD,		12},		// 4, Amount, transaction (n12)

	{	C_BCD,		12},	// 5, Amount, settlement (n12)
	{	C_BCD,		12},	// 6, Amount, cardholder billing fee (n12)
	{	C_MMDDhhmmss,10},	// 7, Transmission date and time (n10)
	{	C_BCD,		8},		// 8, Amount, cardholder billing fee (n8)

	{	C_BCD,		8},		// 9, Conversion rate, settlement (n8)
	{	C_BCD,		8},		// 10, Conversion rate, cardholder billing (n8)
	{	C_BCD,		6},		// 11, STAN (n6)

	/**
	{	C_hhmmss,	6},		// 12, Time, local transactions (n6)
	{	C_MMDD,		4},		// 13, Date, local transactions (n4)
	{	C_YYMM,		4},		// 14, Date, expiry (n4)
	{	C_MMDD,		4},		// 15, Date, settlement (n4)
	{	C_MMDD,		4},		// 16, Date, conversion (n4)
	{	C_MMDD,		4},		// 17, Date, capture (n4)
	*/
	{	C_BCD,		6},		// 12, Time, local transactions (n6)
	{	C_BCD,		4},		// 13, Date, local transactions (n4)
	{	C_BCD,		4},		// 14, Date, expiry (n4)
	{	C_BCD,		4},		// 15, Date, settlement (n4)
	{	C_BCD,		4},		// 16, Date, conversion (n4)
	{	C_BCD,		4},		// 17, Date, capture (n4)

	{	C_BCD,		4},		// 18, Merchant's type (n4)
	{	C_BCD,		3},		// 19, Acquiring Institution Country Code (n3)
	{	C_BCD,		3},		// 20, PAN extended, country code (n3)

	{	C_BCD,		3},		// 21, Forwarding institution country code (n3)
	{	C_BCD,		3},		// 22, POS Entry Mode (n3)
	{	C_BCD,		3},		// 23, Card sequence number (n3)
	{	C_BCD,		3},		// 24, NII (n3)

	{	C_BCD,		2},		// 25, POS condition code (n2)
	{	C_BCD,		2},		// 26, POS PIN service code (n2)
	{	C_BCD,		1},		// 27, Auth Identification response length (n1)
	{	C_AMOUNT,	8},		// 28, Amount, transaction fee (x + n8)

	{	C_AMOUNT,	8},		// 29, Amount, settlement fee (x + n8)
	{	C_AMOUNT,	8},		// 30, Amount, transaction processing fee (x + n8)
	{	C_AMOUNT,	8},		// 31, Amount, settlement processing fee (x + n8)
	{	C_LLNVAR,	11},	// 32, AIIC (LLVAR n..11)

	{	C_LLNVAR,	11},	// 33, FIIC (LLVAR n..11)
	{	C_LLAVAR,	28},	// 34, PAN extended (LLVAR ns..28)
	{	C_LLNVAR,	37},	// 35, Track 2 (LLVAR z..37)
	{	C_LLLNVAR,	104},	// 36, Track 3 (LLLVAR z...104)

	{	C_STRING,	12},	// 37, RRN (an12)
	{	C_STRING,	6},		// 38, Auth Identification Response (an6)
	{	C_STRING,	2},		// 39, Response code (an2)
	{	C_STRING,	3},		// 40, Service Restriction Code (an3)

	{	C_STRING,	8},		// 41, CATID (ans8)
	{	C_STRING,	15},	// 42, CAIC (ans15)
	{	C_STRING,	40},	// 43, Card Acceptor Name/Location (ans40)
	{	C_LLAVAR,	25},	// 44, Additional response data (LLVAR ans..25)

	{	C_LLAVAR,	76},	// 45, Track 1 (LLVAR ans..76)
	{	C_LLLVAR,	0},		// 46, Additional data - ISO (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 47, Additional data - National (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 48, Additional data - Private (LLLVAR ans...999)

	{	C_BCD,		3},		// 49, Currency code - transaction (n3)
	{	C_BCD,		3},		// 50, Currency code - settlement (n3)
	{	C_BCD,		3},		// 51, Currency code - cardholder billing (n3)
	{	C_BITMAP,	64},	// 52, PIN Block (b64)

	{	C_BCD,		16},	// 53, Security related control information (n16)
	{	C_LLLVAR,	120},	// 54, Additional Amounts (LLLVAR an...120)
	{	C_LLLVAR,	0},		// 55, Reserved for ISO use (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 56, Reserved for ISO use (LLLVAR ans...999)

	{	C_BCD,		12},	// 57, Amount cash (n12)
	{	C_BCD,		12},	// 58, Ledger balance (n12)
	{	C_BCD,		12},	// 59, Account balance, cleared funds (n12)
	{	C_LLLVAR,	0},		// 60, Reserved private (LLLVAR ans...999)

	{	C_LLLVAR,	0},		// 61, Reserved private (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 62, Reserved private (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 63, Reserved private (LLLVAR ans...999)
	{	C_BITMAP,	64},	// 64, MAC (b64)




	{	C_BITMAP,	64},	// 65, Reserved for ISO use (b64)
	{	C_BCD,		1},		// 66, Settlement code (n1)
	{	C_BCD,		2},		// 67, Extended payment code (n2)
	{	C_BCD,		3},		// 68, Receiving institution country code (n3)

	{	C_BCD,		3},		// 69, Settlement institution country code (n3)
	{	C_BCD,		3},		// 70, NMIC (n3)
	{	C_BCD,		4},		// 71, Message number (n4)
	{	C_BCD,		4},		// 72, Message number, last (n4)

	{	C_YYMMDD,	6},		// 73, Date, action (n6)
	{	C_BCD,		10},	// 74, Credits, number (n10)
	{	C_BCD,		10},	// 75, Credits, reversal number (n10)
	{	C_BCD,		10},	// 76, Debits, number (n10)

	{	C_BCD,		10},	// 77, Debits, reversal number (n10)
	{	C_BCD,		10},	// 78, Transfer, number (n10)
	{	C_BCD,		10},	// 79, Transfer, reversal number (n10)
	{	C_BCD,		10},	// 80, Inquiries, number (n10)

	{	C_BCD,		10},	// 81, Auth, number (n10)
	{	C_BCD,		12},	// 82, Credits, processing fee amount (n12)
	{	C_BCD,		12},	// 83, Credits, transaction fee amount (n12)
	{	C_BCD,		12},	// 84, Debits, processing fee amount (n12)

	{	C_BCD,		12},	// 85, Debits, transaction fee amount (n12)
	{	C_BCD,		16},	// 86, Credits, amount (n16)
	{	C_BCD,		16},	// 87, Credits, reversal amount (n16)
	{	C_BCD,		16},	// 88, Debits, amount (n16)

	{	C_BCD,		16},	// 89, Debits, reversal amount (n16)
	{	C_BCD,		42},	// 90, Original data elements (n42)
	{	C_STRING,	1},		// 91, File update code (an1)
	{	C_STRING,	2},		// 92, File security code (an2)

	{	C_STRING,	5},		// 93, Response indicator (an5)
	{	C_STRING,	7},		// 94, Service indicator (an7)
	{	C_STRING,	42},	// 95, Replacement amounts (an42)
	{	C_BITMAP,	64},	// 96, Message Security Code (b64)

	{	C_AMOUNT,	16},	// 97, Amount, net settlement (x + n16)
	{	C_LLAVAR,	25},	// 98, Payee (LLVAR ans..25)
	{	C_LLNVAR,	11},	// 99, SIIC (LLVAR n..11)
	{	C_LLNVAR,	11},	// 100, RIIC (LLVAR n..11)

	{	C_LLAVAR,	17},	// 101, File Name (LLVAR ans..17)
	{	C_LLAVAR,	28},	// 102, Account Identification 1 (LLVAR ans..28)
	{	C_LLAVAR,	28},	// 103, Account Identification 2 (LLVAR ans..28)
	{	C_LLLVAR,	100},	// 104, Transaction description (LLLVAR ans...100)

	{	C_LLLVAR,	0},		// 105, Reserved for ISO use (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 106, Reserved for ISO use (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 107, Reserved for ISO use (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 108, Reserved for ISO use (LLLVAR ans...999)

	{	C_LLLVAR,	0},		// 109, Reserved for ISO use (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 110, Reserved for ISO use (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 111, Reserved for ISO use (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 112, Reserved for national use (LLLVAR ans...999)

	{	C_LLLVAR,	0},		// 113, Reserved for national use (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 114, Reserved for national use (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 115, Reserved for national use (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 116, Reserved for national use (LLLVAR ans...999)

	{	C_STRING,	2},		// 117, Card status update code (an2)
	{	C_BCD,		10},	// 118, Cash. total number (n10)
	{	C_BCD,		16},		// 119, Cash, total amount (n16)
	{	C_LLLVAR,	0},		// 120, Reserved private (LLLVAR ans...999)

	{	C_LLLVAR,	0},		// 121, Reserved private (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 122, Reserved private (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 123, Reserved private (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 124, Reserved private (LLLVAR ans...999)

	{	C_LLLVAR,	0},		// 125, Reserved private (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 126, Reserved private (LLLVAR ans...999)
	{	C_LLLVAR,	0},		// 127, Reserved private (LLLVAR ans...999)
	{	C_BITMAP,	64},	// 128, MAC (b64)
};

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : AS2805SetBit
**
** DESCRIPTION:	Set a bit to indicate the presense of a field in the AS2805 message
**
** PARAMETERS:	None
**
** RETURNS:		None
**-------------------------------------------------------------------------------------------
*/
void AS2805SetBit(uchar field)
{
	uchar octet;
	uchar bit;

	// If we are using the secondary bitmap, then adjust...
	if (field > 64 && (buffer[2] & 0x80) == 0)
	{
		// Shift the fields already filled by 8 bytes (the size of the secondary bitmap)
		memmove(&buffer[18], &buffer[10], fieldsIndex - fieldsStart);

		// Clear the secondary bitmap
		memset(&buffer[10], 0, 8);

		// The fields data now start at byte 16 allowing for both primary and secondary bitmaps
		fieldsStart = 18;
		fieldsIndex += 8;

		// Indicate that the secondary bitmap is now used
		buffer[2] |= 0x80;
	}

	// Set the corresponsing bit within either the primary or secondary bitmap
	octet = (field-1) / 8;
	bit = (field-1) % 8;
	buffer[2+octet] |= (0x80 >> bit);
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : _bcdToNumber
**
** DESCRIPTION:	Convert a bcd array to a number.
**				If A-F are encountered, they are simply ignored. This works well with variable length bcd values
**
** PARAMETERS:	bcd		<=	Array to convert
**				size	<=	Number of BCD digits in the array
**
** RETURNS:		The converted number
**-------------------------------------------------------------------------------------------
*/
static ulong _bcdToNumber(uchar * bcd, uint * index, uint size, uchar format)
{
	uint number;

	// Adjust for left over nibble already accounted for.
	if (leftOver) size--;
	leftOver = false;

	// Convert the length to octets (bytes)
	if (size & 0x01)
	{
		size = size / 2 + 1;
		if (format == C_BCD_LINK) leftOver = true;
	}
	else
		size /= 2;

	for (number = leftOverValue; size; size--, (*index)++)
	{
		uchar digit = bcd[*index] >> 4;
		number *= 10;
		if (digit <= 9) number += digit;

		digit = bcd[*index] & 0x0f;
		number *= 10;
		if (digit <= 9) number += digit;
	}

	if (leftOver)
	{
		leftOverValue = number % 10;
		number /= 10;
	}
	else leftOverValue = 0;

	return number;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : _bcdToString
**
** DESCRIPTION:	Convert a bcd array to a number.
**				If A-F are encountered, they are simply ignored. This works well with variable length bcd values
**
** PARAMETERS:	bcd		<=	Array to convert
**				size	<=	Number of BCD digits in the array
**
** RETURNS:		The converted number
**-------------------------------------------------------------------------------------------
*/
static char * _bcdToString(uchar * bcd, uint * index, uint size, uchar format, char * string)
{
	static bool leftOver = false;
	static char leftOverValue = '\0';

	// Adjust for left over nibble already accounted for.
	if (leftOver) size--;
	leftOver = false;

	// Convert the length to octets (bytes)
	if (size & 0x01)
	{
		size = size / 2 + 1;
		if (format == C_BCD_LINK) leftOver = true;
	}
	else
		size /= 2;

	// Start with the leftover value if available
	string[0] = leftOverValue;
	string[1] = '\0';

	for (; size; size--, (*index)++)
		sprintf(&string[strlen(string)], "%02X", bcd[*index]);

	if (leftOver)
	{
		leftOverValue = string[strlen(string)-1];
		string[strlen(string)-1] = '\0';
	}
	else leftOverValue = '\0';

	return string;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : _adjustForVarBCD
**
** DESCRIPTION:	Inserts a BCD field
**
** PARAMETERS:	None
**
** RETURNS:		None
**-------------------------------------------------------------------------------------------
*/
static void _adjustForVarBCD(char * temp)
{
	int size = strlen(temp);

	if (size & 0x01)
	{
		temp[size+1] = '\0';
		temp[size] = 0x3F;
	}
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : AS2805Init
**
** DESCRIPTION:	Initialise the AS2805 buffer
**
** PARAMETERS:	None
**
** RETURNS:		A pointer to the AS2805 buffer area
**-------------------------------------------------------------------------------------------
*/
uchar * AS2805Init(uint size)
{
	// Allocate an approprate buffer for an AS2805 message and fill it with ZEROs
	if (buffer)
		UtilStrDup((char **) &buffer, NULL);
	buffer = my_calloc(size);

	// Assume initially that the secondary bitmap is not used. Hence, the fields data start at position # 10
	fieldsStart = fieldsIndex = 10;
	leftOver = false;
	leftOverValue = 0;

	return buffer;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : AS2805close
**
** DESCRIPTION:	Release the memory used by AS2805 packets
**
** PARAMETERS:	None
**
** RETURNS:		None
**-------------------------------------------------------------------------------------------
*/
void AS2805Close()
{
	// Deallocate the buffer in an AS2805 message and clean up
	if (buffer)
		UtilStrDup((char **) &buffer, NULL);

	leftOver = false;
	leftOverValue = 0;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : AS2805Position
**
** DESCRIPTION:	Returns the current position of the buffer and address
**
** PARAMETERS:	length	=>	Upodated to contaion current length of AS2805 packet
**
** RETURNS:		Address of buffer
**-------------------------------------------------------------------------------------------
*/
uchar * AS2805Position(uint * length)
{
	*length = fieldsIndex;
	return buffer;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : AS2805Position
**
** DESCRIPTION:	Returns the current position of the buffer and address
**
** PARAMETERS:	length	=>	Upodated to contaion current length of AS2805 packet
**
** RETURNS:		Address of buffer
**-------------------------------------------------------------------------------------------
*/
void AS2805BcdLength(bool state)
{
	bcdLength = state;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : AS2805BufferPack
**
** DESCRIPTION:	Packs a data field or sub-field into a buffer.
**
** PARAMETERS:	data			<=	data used for packing
**				format			<=	Format of field / sub-field
**				size		<=	Size of of field / subfield.
**									except for amount where it is the number of octets.
**				buffer			<=	Buffer used to append the field / sub-field data to.
**				index			<=>	Current index for subfield. It will be updated on output
**
** RETURNS:		None
**
**-------------------------------------------------------------------------------------------
*/
void AS2805BufferPack(char * data, uchar format, uint size, uchar * buffer, uint * index)
{
	char temp[1024];
	static char link = '\0';
	time_t_ris sometime;	//KK changed
	struct tm myTime;

	if(format == C_LLVAR) format = bcdLength ? C_LLNVAR: C_LLAVAR;

	switch (format)
	{
		case C_BCD:
		case C_BCD_LINK:
			if (link != '\0')
			{
				sprintf(temp, "%c%0*s", link, size, data);
				temp[size+1] = '\0';
			}
			else
			{
				sprintf(temp, "%0*s", size, data);
				temp[size] = '\0';
			}

			if (format == C_BCD_LINK && (strlen(temp) & 0x01))
			{
				link = temp[strlen(temp)-1];
				temp[strlen(temp)-1] = '\0';
			}
			else link = '\0';

			if (temp[0] != '\0')
				*index += UtilStringToHex(temp, strlen(temp), &buffer[*index]);
			break;
		case C_MMDDhhmmss:
		case C_YYMMDD:
		case C_YYMM:
		case C_MMDD:
		case C_hhmmss:
			sometime = atol(data);
			myTime = *my_gmtime(&sometime);

			if (format == C_MMDDhhmmss)
				sprintf(temp, "%0.2d%0.2d%0.2d%0.2d%0.2d", myTime.tm_mon+1, myTime.tm_mday, myTime.tm_hour, myTime.tm_min, myTime.tm_sec);
			else if (format == C_YYMMDD)
				sprintf(temp, "%0.2d%0.2d%0.2d", myTime.tm_year % 100, myTime.tm_mon + 1, myTime.tm_mday);
			else if (format == C_YYMM)
				sprintf(temp, "%0.2d%0.2d", myTime.tm_year % 100, myTime.tm_mon + 1);
			else if (format == C_MMDD)
				sprintf(temp, "%0.2d%0.2d", myTime.tm_mon + 1, myTime.tm_mday);
			else if (format == C_hhmmss)
				sprintf(temp, "%0.2d%0.2d%0.2d", myTime.tm_hour, myTime.tm_min, myTime.tm_sec);

			*index += UtilStringToHex(temp, size, &buffer[*index]);
			break;
		case C_BITMAP:
			UtilStringToHex(data, strlen(data), &buffer[*index]);
//				memcpy(&buffer[*index], data, size/8);
			*index += size/8;
			break;
		case C_AMOUNT:
			if (data[0] == '-')
			{
				buffer[(*index)++] = 'D';
				data++;
			}
			else buffer[(*index)++] = 'C';
			*index += UtilStringToHex(data, size, &buffer[*index]);
			break;
		case C_STRING:
			sprintf((char *) &buffer[*index], "%-*s", size, data);
			*index += size;
			break;
		case C_LLNVAR:
			*index += UtilStringToHex(ltoa(strlen(data), temp, 10), 1, &buffer[*index]);
			strcpy(temp, data);
			_adjustForVarBCD(temp);
			*index += UtilStringToHex(temp, strlen(temp), &buffer[*index]);
			break;
		case C_LLAVAR:
			sprintf((char *) &buffer[*index], "%02d%s", strlen(data), data);
			*index += strlen(data) + 2;
			break;
		case C_LLLVAR:
			if (bcdLength)
				*index += UtilStringToHex(ltoa(strlen(data)/2, temp, 10), 3, &buffer[*index]);
			else
			{
				sprintf((char *) &buffer[*index], "%03d", strlen(data)/2);
				*index += 3;
			}
			*index += UtilStringToHex(data, strlen(data), &buffer[*index]);
			break;
	}
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : AS2805Pack
**
** DESCRIPTION:	Packs a field into an AS2805 buffer
**
** PARAMETERS:	field		<=	The field number
**				data		<=	The data used for packing
**
** RETURNS:		Current size of buffer
**
**-------------------------------------------------------------------------------------------
*/
uint AS2805Pack(uchar field, char * data)
{
	// Initialise the AS2805 message buffer and controling variables if it has not been called already...
	// This is just a precaution since it must still be called from the application
	if (buffer == NULL)
		AS2805Init(2000);

	// Store the message ID at the beginning of the message
	if (field == 0)
		UtilStringToHex(data, 4, buffer);
	else
	{
		// Set the appropraite primary or secondary bit
		AS2805SetBit(field);

		// Pack the field data into the allocated AS2805 buffer
		AS2805BufferPack(data, fieldType[field].format, fieldType[field].size, buffer, &fieldsIndex);
	}

	return fieldsIndex;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : AS2805BufferUnPack
**
** DESCRIPTION:	Packs a field into an AS2805 buffer
**
** PARAMETERS:	field		<=	The field number
**				data		=>	The output is stored here as a string
**
** RETURNS:		None
**
**-------------------------------------------------------------------------------------------
*/
void AS2805BufferUnpack(char * data, uchar format, uint size, uchar * buffer, uint * index)
{
	uchar length=0;
	struct tm myTime;
	int i = 0;

	memset(&myTime, 0, sizeof(myTime));

	if(format == C_LLVAR) format = bcdLength ? C_LLNVAR: C_LLAVAR;

	switch (format)
	{
		case C_BCD:
		case C_BCD_LINK:
			_bcdToString(buffer, index, size, format, data);
			break;
		case C_AMOUNT:
			if (buffer[(*index)++] == 'D')
				data[0] = '-';
			else data[0] = '0';
			_bcdToString(buffer, index, size, format, &data[1]);
			break;
		case C_YYMM:
		case C_YYMMDD:
			myTime.tm_year = _bcdToNumber(buffer, index, 2, C_BCD) + 100;
		case C_MMDD:
		case C_MMDDhhmmss:
			myTime.tm_mon = _bcdToNumber(buffer, index, 2, C_BCD) - 1;
			if (format == C_YYMM)
			{
				ltoa(my_mktime(&myTime), data, 10);
				break;
			}
			myTime.tm_mday = _bcdToNumber(buffer, index, 2, C_BCD);
			if (format == C_YYMMDD || format == C_MMDD)
			{
				ltoa(my_mktime(&myTime), data, 10);
				break;
			}
		case C_hhmmss:
			myTime.tm_hour = _bcdToNumber(buffer, index, 2, C_BCD);
			myTime.tm_min = _bcdToNumber(buffer, index, 2, C_BCD);
			myTime.tm_sec = _bcdToNumber(buffer, index, 2, C_BCD);
			ltoa(my_mktime(&myTime), data, 10);
			break;
		case C_BITMAP:
			UtilHexToString((const char *)&buffer[*index], size/8, data);
			*index += size/8;
			break;
		case C_STRING:
			memcpy(data, &buffer[*index], size);
			*index += size;
			while (data[size-1] == ' ') size--;
			data[size] = '\0';
			break;
		case C_LLNVAR:
		case C_LLLNVAR:
			if (format == C_LLNVAR) length = 2;
			if (format == C_LLLNVAR) length = 3;
			size = _bcdToNumber(buffer, index, length, C_BCD);
			for (i = 0; i < size; i++)
			{
				if (i & 0x01)
					data[i] = (buffer[(*index)++] & 0x0f) + 0x30;
				else
					data[i] = (buffer[(*index)] >> 4) + 0x30;
			}
			if (size & 0x01) (*index)++;
			data[i] = '\0';
			break;
		case C_LLAVAR:
			size = (buffer[*index] - '0') * 10 + buffer[*index+1] - '0';
			*index += 2;
			memcpy(data, &buffer[*index+2], size);
			data[size] = '\0';
			*index += size ;
			break;
		case C_LLLVAR:
			if (bcdLength)
				size = _bcdToNumber(buffer, index, 3, C_BCD);
			else
			{
				size = (buffer[*index] - '0') * 100 + (buffer[(*index)+1] - '0') * 10 + buffer[(*index)+2] - '0';
				(*index) += 3;
			}
			UtilHexToString((const char *)&buffer[*index], size, data);
			*index += size;
			break;
		default:
			break;
	}
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : AS2805UnPack
**
** DESCRIPTION:	Packs a field into an AS2805 buffer
**
** PARAMETERS:	field		<=	The field number
**				data		=>	The packed data as a string
**
** RETURNS:		None
**
**-------------------------------------------------------------------------------------------
*/
void AS2805Unpack(uchar field, char * data, uchar * buffer, uint length)
{
	uchar bitMap = 0x80;
	uint index = 0;
	uchar currentField = 1;

	// Initialisation
	data[0] = '\0';

	if( field > 128) return ;

	if (field == 0)
		sprintf(data, "%d", _bcdToNumber(buffer, &index, 4, C_BCD));
	else index = 2;

	for (fieldsIndex = 10; currentField <= field && fieldsIndex < length; currentField++, bitMap >>= 1)
	{
		if (bitMap == 0)
		{
			bitMap = 0x80;
			index++;
		}

		if (buffer[index] & bitMap)
		{
			uchar format = fieldType[currentField].format;
			int size = fieldType[currentField].size;

			if(format == C_LLVAR) format = bcdLength ? C_LLNVAR: C_LLAVAR;

			if (field != currentField)
			{
				switch (format)
				{
					case C_LLNVAR:
					case C_LLLNVAR:
						{
							int len = _bcdToNumber(buffer, &fieldsIndex, (format == C_LLNVAR)?2:3, format);
							if (len & 0x01) len = len / 2 + 1; else len /= 2;
							fieldsIndex = fieldsIndex + len ;
						}
						break;
					case C_LLAVAR:
						fieldsIndex += (buffer[fieldsIndex] - '0') * 10 + buffer[fieldsIndex+1] - '0' + 2;
						break;
					case C_LLLVAR:
						if (bcdLength) {
							//fieldsIndex += _bcdToNumber(buffer, &fieldsIndex, 3, C_BCD);
							int len = _bcdToNumber(buffer, &fieldsIndex, 3, C_BCD);
							fieldsIndex += len ;
						}
						else
							fieldsIndex += (buffer[fieldsIndex] - '0') * 100 + (buffer[fieldsIndex+1] - '0') * 10 + buffer[fieldsIndex+2] - '0' + 3;
						break;
					case C_AMOUNT:
						fieldsIndex++;
					case C_BCD:
					case C_MMDDhhmmss:
					case C_YYMMDD:
					case C_YYMM:
					case C_MMDD:
					case C_hhmmss:
						fieldsIndex += 	size / 2;
						if (size & 0x01) fieldsIndex++;
						break;
					case C_BITMAP:
						fieldsIndex += 	size / 8;
						break;
					default:
						fieldsIndex += 	size;
				}
			}
			else
			{
				AS2805BufferUnpack(data, format, size, buffer, &fieldsIndex);
				break;
			}
		}
	}
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : AS2805OFBAdjust
**
** DESCRIPTION:	Transfers the clear fields, as specified in AS2805.9, from a source stream
**				to a destination stream to complete message encryption / decryption.
**
**				The fields transferred, if available, according to the primary bitmap are:
**				- Field 0 - Message ID.
**				- Primary bitmap.
**				- Field 39	- Response Code
**				- Field 41	- Terminal ID
**				- Field 42	- Merchant ID
**				- Field 53 - Security Related Control Information
**				- The length portion of any variable length field of it exists up to field 53.
**				  This includes fields 2,32-36, 44-48 as defined in the above table
**
**				
** PARAMETERS:	source		<=	Source AS2805 stream where clear data are stored.
**				dest		=>	Destination AS2805 stream - Must be a replica of source stream
**								In terms of field existence and size. The data in the other
**								fields are left untouched.
**				length		<=	Length of the buffer
**
** RETURNS:		None
**
**-------------------------------------------------------------------------------------------
*/
void AS2805OFBAdjust(uchar * source, uchar * dest, uint length)
{
	uchar bitMap = 0x80;
	uint index = 2;
	uchar currentField = 1;
	const uchar field[] = {39, 41, 42, 53};
	int i = 0;

	// Transfer field 0 = Message ID and primary bitmap
	memcpy(dest, source, 10);

	for (fieldsIndex = 10; currentField <= maxField && fieldsIndex < length; currentField++, bitMap >>= 1)
	{
		if (bitMap == 0)
		{
			bitMap = 0x80;
			index++;
		}

		if (source[index] & bitMap)
		{
			uchar format = fieldType[currentField].format;
			int size = fieldType[currentField].size;

			if(format == C_LLVAR) format = bcdLength ? C_LLNVAR: C_LLAVAR;

			if (currentField != field[i] && currentField != addField)
			{
				switch (format)
				{
					case C_LLNVAR:
					case C_LLLNVAR:
						memcpy(&dest[fieldsIndex], &source[fieldsIndex], (format == C_LLNVAR)?1:2);
						{
							int len = _bcdToNumber(source, &fieldsIndex, (format == C_LLNVAR)?2:3, format);
							if (len & 0x01) len = len / 2 + 1;
							else len /= 2;
							fieldsIndex += len;
						}
						break;
					case C_LLAVAR:
						memcpy(&dest[fieldsIndex], &source[fieldsIndex], 2);
						fieldsIndex += (source[fieldsIndex] - '0') * 10 + source[fieldsIndex+1] - '0' + 2;
						break;
					case C_LLLVAR:
						if (bcdLength)
						{
							memcpy(&dest[fieldsIndex], &source[fieldsIndex], 2);
							fieldsIndex += _bcdToNumber(buffer, &fieldsIndex, 3, C_BCD);
						}
						else
						{
							memcpy(&dest[fieldsIndex], &source[fieldsIndex], 3);
							fieldsIndex += (source[fieldsIndex] - '0') * 100 + (source[fieldsIndex+1] - '0') * 10 + source[fieldsIndex+2] - '0' + 3;
						}
						break;
					case C_AMOUNT:
						fieldsIndex++;
					case C_BCD:
					case C_MMDDhhmmss:
					case C_YYMMDD:
					case C_YYMM:
					case C_MMDD:
					case C_hhmmss:
						fieldsIndex += 	size / 2;
						if (size & 0x01) fieldsIndex++;
						break;
					case C_BITMAP:
						fieldsIndex += 	size / 8;
						break;
					default:
						fieldsIndex += 	size;
				}
			}
			else
			{
				memcpy(&dest[fieldsIndex], &source[fieldsIndex], format == C_STRING?size:size/2);
				fieldsIndex += (format == C_STRING?size:size/2);
			}
		}

		if (currentField >= field[i])
			i++;
	}
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : AS2805OFBVariation
**
** DESCRIPTION:	Allows varying the maximum field number to include during OFB encryption.
**				It also allows to add an additional field to leave unencrypted - usually the STAN
**				which can be used as part of the IV.
**				
** PARAMETERS:	my_maxField		<=	New maximum field.
**				my_addField		<=	New additional field.
**
** RETURNS:		None
**
**-------------------------------------------------------------------------------------------
*/
void AS2805OFBVariation(int my_maxField, int my_addField)
{
	maxField = my_maxField;
	addField = my_addField;
}
