/*
**-----------------------------------------------------------------------------
** PROJECT:			iRIS
**
** FILE NAME:       utility.c
**
** DATE CREATED:    30 January 2008
**
** AUTHOR:          Tareq Hafez
**
** DESCRIPTION:     This module has various utility functions used by other modules
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
#include "utility.h"
#include "alloc.h"

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

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : UtilBinToBCD
**
** DESCRIPTION:	Transforms an unsigned long to a BCD array
**
** PARAMETERS:	data		<=	The unsigned long variable
**				bcd			=>	Array to store the converted bcd amount
**				length		=>	Number of BCD digits required. This must be an even number.
**
** RETURNS:		None
**-------------------------------------------------------------------------------------------
*/
void UtilBinToBCD(uchar data, uchar * bcd, uchar length)
{
	ulong divider = 1;

	if (!bcd) return;

	// Adjust the divider to the correct value
	while (--length) divider *= 10;

	// Prepare the STAN variant in the correct format
	for (; divider >= 1; bcd++, divider /= 100)
	{
		*bcd = (uchar) ((data / divider % 10) << 4);
		if (divider == 1) break;
		*bcd |= data / (divider / 10) % 10;
	}
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : UtilHexToString
**
** DESCRIPTION:	Transforms hex byte array to a string. Each byte is simply split in half.
**				The minimum size required is double that of the hex byte array
**
** PARAMETERS:	hex			<=	Array to store the converted hex data
**				string		=>	The number string.
**				length		<=	Length of hex byte array
**
** RETURNS:		The converted string
**-------------------------------------------------------------------------------------------
*/
char * UtilHexToString(const char * hex, int length, char * string)
{
	int i;

	if (string)
	{
		string[0] = '\0';

		if (hex)
		{
			for (i = 0; i < length; i++)
				sprintf(&string[i*2], "%02X", hex[i]);
		}
	}

	return string;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : UtilStrDup
**
** DESCRIPTION:	frees the existing pointer if it already exists, then performs a strdup()
**				to allocate and transfers to it the source string but only if the source string
**				exists
**
** PARAMETERS:	dest		=>	The new string to (re)allocate
**				source		<=	The string source that holds the string value
**
** RETURNS:		dest
**-------------------------------------------------------------------------------------------
*/
char * UtilStrDup(char ** dest, const char * source)
{
	if (!dest) return NULL;

	if (*dest)
	{
		my_free(*dest);
		*dest = NULL;
	}

	if (source)
	{
		*dest = (char *)my_malloc(strlen(source) + 1);
		strcpy(*dest, source);
	}

	return *dest;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : UtilStringToHex
**
** DESCRIPTION:	Transforms an ASCII HEX string into a hex byte array. The string can only contain
**				0 to 9, : to ?, and A to F tp get the correct results. Otherwise the values are
**				wrong.
**
** PARAMETERS:	string		<=	The ASCII HEX string
**				length		<=	Number of ASCII characters to convert from the right
**				hex			=>	Array to store the converted hex bytes
**
** RETURNS:		The converted hex byte array
**-------------------------------------------------------------------------------------------
*/
int UtilStringToHex(const char * string, int length, uchar * hex)
{
	int i;
	int index;

	// Handle error conditions
	if (!string || !hex)
		return 0;

	// Calculate the number of bytes required
	index = length / 2;
	if (length & 0x01) index++;
	length = index;

	// Initialise to all ZEROs
	memset(hex, 0, length);

	for (i = strlen(string)-1, index--; i >= 0  && index >= 0; index--, i--)
	{
		hex[index] = (string[i]>='a'&& string[i]<='f' ? (string[i]-'a'+0x0A): (string[i] >= 'A'&&string[i] <= 'F' ? (string[i] - 'A' + 0x0A):(string[i] - '0')));
		if (--i >= 0)
			hex[index] |= (string[i]>='a'&&string[i]<='f'?(string[i]-'a'+0x0A):(string[i] >= 'A'&&string[i]<='F'? (string[i] - 'A' + 0x0A):(string[i] - '0'))) << 4;
	}

	return length;
}

long UtilStringToNumber(const char * string)
{
	bool negative = false;
	uint i;
	long result;
	char * ptr = (char *)string;

	if (!string || ptr[0] == '\0')
		return -1;

	if (ptr[0] == '-')
		negative = true, ptr++;
	else if (ptr[0] == '+')
		ptr++;

	while (*ptr == '0' || *ptr == ' ') ptr++;

	if (strlen(ptr) >= 10)
		return -1;

	for (i = 0; ptr[i]; i++)
	{
		if (ptr[i] < '0' || ptr[i] > '9')
			return -1;
	}

	result = atol(ptr);

	if (negative)
		return -result;
	else
		return result;
}

