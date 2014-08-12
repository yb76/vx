//
//-----------------------------------------------------------------------------
// PROJECT:			iRIS
//
// FILE NAME:       prtean128.c
//
// DATE CREATED:    22 January 2009
//
// DESCRIPTION:     This module has barcode EAN-128 printing support
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
#include <string.h>
#include <stdio.h>

//
// Project include files.
//
#include <auris.h>

//
// Local include files
//
#include "prtean128.h"

//
//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
//
const long ean128CharValue[108] =
{
	212222L,	// 0
	222122L,	// 1
	222221L,	// 2
	121223L,	// 3
	121322L,	// 4
	131222L,	// 5
	122213L,	// 6
	122312L,	// 7
	132212L,	// 8
	221213L,	// 9
	221312L,	// 10
	231212L,	// 11
	112232L,	// 12
	122132L,	// 13
	122231L,	// 14
	113222L,	// 15
	123122L,	// 16
	123221L,	// 17
	223211L,	// 18
	221132L,	// 19
	221231L,	// 20
	213212L,	// 21
	223112L,	// 22
	312131L,	// 23
	311222L,	// 24
	321122L,	// 25
	321221L,	// 26
	312212L,	// 27
	322112L,	// 28
	322211L,	// 29
	212123L,	// 30
	212321L,	// 31
	232121L,	// 32
	111323L,	// 33
	131123L,	// 34
	131321L,	// 35
	112313L,	// 36
	132113L,	// 37
	132311L,	// 38
	211313L,	// 39
	231113L,	// 40
	231311L,	// 41
	112133L,	// 42
	112331L,	// 43
	132131L,	// 44
	113123L,	// 45
	113321L,	// 46
	133121L,	// 47
	313121L,	// 48
	211331L,	// 49
	231131L,	// 50
	213113L,	// 51
	213311L,	// 52
	213131L,	// 53
	311123L,	// 54
	311321L,	// 55
	331121L,	// 56
	312113L,	// 57
	312311L,	// 58
	332111L,	// 59
	314111L,	// 60
	221411L,	// 61
	431111L,	// 62
	111224L,	// 63
	111422L,	// 64
	121124L,	// 65
	121421L,	// 66
	141122L,	// 67
	141221L,	// 68
	112214L,	// 69
	112412L,	// 70
	122114L,	// 71
	122411L,	// 72
	142112L,	// 73
	142211L,	// 74
	241211L,	// 75
	221114L,	// 76
	413111L,	// 77
	241112L,	// 78
	134111L,	// 79
	111242L,	// 80
	121142L,	// 81
	121241L,	// 82
	114212L,	// 83
	124112L,	// 84
	124211L,	// 85
	411212L,	// 86
	421112L,	// 87
	421211L,	// 88
	212141L,	// 89
	214121L,	// 90
	412121L,	// 91
	111143L,	// 92
	111341L,	// 93
	131141L,	// 94
	114113L,	// 95
	114311L,	// (FNC3	/	FNC3	/	96)
	411113L,	// (FNC2	/	FNC2	/	97)
	411311L,	// (SHIFT	/	SHIFT	/	98)
	113141L,	// (CODE C	/	CODE C	/	99)
	114131L,	// (CODE B	/	FNC4	/	CODE B) =100
	311141L,	// (FNC4	/	CODE A	/	CODE A) =101
	411131L,	// (FNC1	/	FNC1	/	FNC1)	=102
	211412L,	// START A = 103	[ From 0x20 ' ' to 0x5F '_', then from 0x00 to 0x1F. Hence is includes the upper case letter but not the lower case ones, then it includes the control characters].
	211214L,	// START B = 104	[ From 0x20 to 0x7F which includes upper and lower case letters] 
	211232L,	// START C = 105
	233111L,	// STOP = 106
	200000L,	// STOP2 = 107
};

#define	SHIFT	98

#define START_A	103
#define START_B	104
#define START_C	105

#define	CODE_A	101
#define	CODE_B	100
#define	CODE_C	99

#define	STOP	106
#define	STOP2	107

static int numDigits(uchar * data)
{
	int i;

	for (i = 0; data[i]; i++)
	{
		if (data[i] < '0' || data[i] > '9')
			break;
	}

	return i;
}

static int aORbFirst(uchar * data)
{
	do
	{
		if (*data < ' ')
			return CODE_A;
		else if (*data > '_')
			return CODE_B;
		data++;
	} while(*data);

	return CODE_B;
}

static char fillCodeAorB(uchar data)
{
	if (data >= ' ')
		return data - ' ';
	else
		return data + '_' + 1;
}

static void fillCodeC(char * out, uchar * data, int size)
{
	int i, j;

	for (i = 0, j = 0; i < size; i += 2, j++)
	{
		int charValue = (data[i] - '0') * 10 + data[i+1] - '0';
		out[j] = charValue;
	}
}

int BuildEan128BarCode(uchar * data, uchar * out, uint size, int height)
{
	int code = CODE_C;
	char paddedData[100];
	int i = 0;
	int j = 0;

	int weightedTotal;
	int weightValue;
	int bytes = 0;
	int bits = 0;

	// Initialisation
	memset(out, 0, sizeof(out));

	// Initial checks
	if(data == NULL || strlen((char *)data) == 0)
		return 1;

	// Determine the start character
	// 1 - If data consists of 2 digits, then Start Character C
	// 2 - If data starts with four or more digits, then start character C
	// 3 - Otherwise, use start character A if we first find a character < ' ' before finding a character > '_'
	// 4 - Otherwise, use code B.
	if ((strlen((char *)data) == 2 && numDigits(data) == 2) || numDigits(data) >= 4)
		paddedData[j++] = START_C;
	else
	{
		code = aORbFirst(data);
		paddedData[j++] = (code == CODE_B? START_B:START_A);
	}

	// Prepare the padded string
	while(data[i])
	{
		int m;
		int k = numDigits(&data[i]);

		if (code != CODE_C)
		{
			if (k >= 4)
				paddedData[j++] = code = CODE_C;
			else if (code == CODE_A)
			{
				if (data[i] > '_')
				{
					if (data[i+1] <= '_')
					{
						paddedData[j++] = SHIFT;
						paddedData[j++] = fillCodeAorB(data[i++]);
					}
					else
						paddedData[j++] = code = CODE_B;
				}
				else paddedData[j++] = fillCodeAorB(data[i++]);
			}
			else
			{
				if (data[i] < ' ')
				{
					if (data[i+1] >= ' ')
					{
						paddedData[j++] = SHIFT;
						paddedData[j++] = fillCodeAorB(data[i++]);
					}
					else
						paddedData[j++] = code = CODE_A;
				}
				else paddedData[j++] = fillCodeAorB(data[i++]);
			}
		}
		else
		{
			// Only code an even number of digits
			if (k & 0x01)
				m = k - 1;
			else m = k;

			// Add the appropriate code values
			fillCodeC(&paddedData[j], &data[i], m);
			i += m, j += (m/2);

			// Add the next code (A or B)
			if (data[i])
				paddedData[j++] = code = aORbFirst(&data[i]);
		}
	}

	// Calculate the Symbol Check Character
	for (weightValue = 1, i = 1, weightedTotal = paddedData[0]; i < j; i++)
	{
		weightedTotal += paddedData[i] * weightValue;
		weightValue++;
	}

	// Fill the Symbol Check Character and end with the Stop Character Value
	paddedData[i++] = weightedTotal % 103;
	paddedData[i++] = STOP;
	paddedData[i++] = STOP2;

	// Now create the output buffer
	for (j = 0; j < i; j++)
	{
		int modules;
		int k;
		long divider = 100000;

		for (k = 0; k < 6; k++, divider /= 10)
		{
			if (divider == 0) divider = 100000L;

			for (modules = ean128CharValue[paddedData[j]] / divider % 10; modules; modules--)
			{
				out[bytes] <<= 1;
				if ((k & 0x01) == 0)
					out[bytes] |= 0x01;
				if (++bits == 8)
					bits = 0, bytes++;
			}
		}
	}

	// Make sure we have shifted the last byte completely
	if (bits > 0)
	{
		while(bits < 8)
			out[bytes] <<= 1, ++bits;
		bytes++;
	}

	// Now make it the appropriate height
	if (bytes <= 24)
		height /= 2;

	for (i = 1; i < height; i++)
		memcpy(&out[i*bytes], out, bytes);

	return bytes;
}
