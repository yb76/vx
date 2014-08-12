//
//-----------------------------------------------------------------------------
// PROJECT:			iRIS
//
// FILE NAME:       prtean13.c
//
// DATE CREATED:    5 March 2008
//
// DESCRIPTION:     This module has barcode printing support
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
#include "prtean13.h"

//
//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
//
#define EAN_NORMAL_WIDTH    21      /* 7 with times 3 scale */
#define LEFT_GUARD_WIDTH    9       /* 3 with x 3 scale */
#define RIGHT_GUARD_WIDTH   9       /* 3 with x 3 scale */
#define CENTRE_GUARD_WIDTH  15      /* 5 with x 3 scale */


/* turn a numeric literal into a hex constant
(avoids problems with leading zeroes)
8-bit constants max value 0x11111111, always fits in unsigned long
*/
#define HEX__(n) 0x##n##LU

/* for upto 8-bit binary constants */
#define B8(d) ((unsigned char)B8__(HEX__(d)))

/* 8-bit conversion function */
#define B8__(x) ((x&0x0000000FLU)?1:0) \
+((x&0x000000F0LU)?2:0) \
+((x&0x00000F00LU)?4:0) \
+((x&0x0000F000LU)?8:0) \
+((x&0x000F0000LU)?16:0) \
+((x&0x00F00000LU)?32:0) \
+((x&0x0F000000LU)?64:0) \
+((x&0xF0000000LU)?128:0)


static void RotateEan13( uchar, uchar * );
static const uchar sFontEAN13Table[] =
{
	B8(00000000),B8(01111110),B8(00111000), //; Odd (A) 0    0001101 ");
	B8(00000011),B8(11110000),B8(00111000), //; Odd (A) 1    0011001 ");
	B8(00000011),B8(10000001),B8(11111000), //; Odd (A) 2    0010011 ");
	B8(00011111),B8(11111110),B8(00111000), //; Odd (A) 3    0111101 ");
	B8(00011100),B8(00000001),B8(11111000), //; Odd (A) 4    0100011 ");
	B8(00011111),B8(10000000),B8(00111000), //; Odd (A) 5    0110001 ");
	B8(00011100),B8(01111111),B8(11111000), //; Odd (A) 6    0101111 ");
	B8(00011111),B8(11110001),B8(11111000), //; Odd (A) 7    0111011 ");
	B8(00011111),B8(10001111),B8(11111000), //; Odd (A) 8    0110111 ");
	B8(00000000),B8(01110001),B8(11111000), //; Odd (A) 9    0001011 ");
	B8(00011100),B8(00001111),B8(11111000), //; Even (B) 0   0100111 ");
	B8(00011111),B8(10000001),B8(11111000), //; Even (B) 1   0110011 ");
	B8(00000011),B8(11110001),B8(11111000), //; Even (B) 2   0011011 ");
	B8(00011100),B8(00000000),B8(00111000), //; Even (B) 3   0100001 ");
	B8(00000011),B8(11111110),B8(00111000), //; Even (B) 4   0011101 ");
	B8(00011111),B8(11110000),B8(00111000), //; Even (B) 5   0111001 ");
	B8(00000000),B8(00001110),B8(00111000), //; Even (B) 6   0000101 ");
	B8(00000011),B8(10000000),B8(00111000), //; Even (B) 7   0010001 ");
	B8(00000000),B8(01110000),B8(00111000), //; Even (B) 8   0001001 ");
	B8(00000011),B8(10001111),B8(11111000), //; Even (B) 9   0010111 ");
	B8(11111111),B8(10000001),B8(11000000), //; Right Hand 0 1110010 ");
	B8(11111100),B8(00001111),B8(11000000), //; Right Hand 1 1100110 ");
	B8(11111100),B8(01111110),B8(00000000), //; Right Hand 2 1101100 ");
	B8(11100000),B8(00000001),B8(11000000), //; Right Hand 3 1000010 ");
	B8(11100011),B8(11111110),B8(00000000), //; Right Hand 4 1011100 ");
	B8(11100000),B8(01111111),B8(11000000), //; Right Hand 5 1001110 ");
	B8(11100011),B8(10000001),B8(00000000), //; Right Hand 6 1010000 ");
	B8(11100000),B8(00001110),B8(00000000), //; Right Hand 7 1000100 ");
	B8(11100000),B8(01110000),B8(00000000), //; Right Hand 8 1001000 ");
	B8(11111111),B8(10001110),B8(00000000)  //; Right Hand 9 1110100 ");
};


const uchar LeftGuard[2] =	{B8(11100011),B8(10000000)};//{0xC7, 0x01};
const uchar RightGuard[2] =	{B8(11100011),B8(10000000)};//{0xC7, 0x01};
const uchar CentreGuard[2] = {B8(00011100),B8(01110000)};//{0x38, 0x0E};


/*
** The following is a table that indicates which code set is to be used
** for a given barcode, as defined by the first character of the barcode
** The type codes give an offset into the font table (note that the character
** needs to be multiplied by three for the font table as the character width
** is three bytes. (21 bits actually)
*/

#define TYPE_A  0
#define TYPE_B  30
#define TYPE_C  60


const uchar sCodeSets[10][6] = {
{TYPE_A, TYPE_A, TYPE_A, TYPE_A, TYPE_A, TYPE_A},
{TYPE_A, TYPE_A, TYPE_B, TYPE_A, TYPE_B, TYPE_B},
{TYPE_A, TYPE_A, TYPE_B, TYPE_B, TYPE_A, TYPE_B},
{TYPE_A, TYPE_A, TYPE_B, TYPE_B, TYPE_B, TYPE_A},
{TYPE_A, TYPE_B, TYPE_A, TYPE_A, TYPE_B, TYPE_B},
{TYPE_A, TYPE_B, TYPE_B, TYPE_A, TYPE_A, TYPE_B},
{TYPE_A, TYPE_B, TYPE_B, TYPE_B, TYPE_A, TYPE_A},
{TYPE_A, TYPE_B, TYPE_A, TYPE_B, TYPE_A, TYPE_B},
{TYPE_A, TYPE_B, TYPE_A, TYPE_B, TYPE_B, TYPE_A},
{TYPE_A, TYPE_B, TYPE_B, TYPE_A, TYPE_B, TYPE_A}
};


#define LEFT_GUARD      10
#define CENTRE_GUARD    11
#define RIGHT_GUARD     12

//
//-----------------------------------------------------------------------------
// Module variable definitions and initialisations.
//-----------------------------------------------------------------------------
//

uchar    PrintCharLine[9];


//-----------------------------------------------------------------------------
//
//          BuildEan13BarCode
//
//      This function will take a numeric string and create a barcode from it
//      as per the EAN 13 specification.
//      This involves also calculating the checksum and possible zero insertion
//      The input string will be alphanumeric with null termination.
//
//-----------------------------------------------------------------------------
int BuildEan13BarCode ( uchar * bpChar, uchar * abPrintBlock, uint wSizePrintBlock)
{
	uchar    PrintCharLine[4];
    uchar    bBarCode[17];
    int     i, n;
    int     row;
    bool    fError;
    uchar    bCount;
    uchar    bCount2;
    uchar    bLeftType;
    uint    wWidth;
    uchar    bIndex;
    uchar    bChecksum;
    uchar    bOffset;
    uint    wTheColumn;		/* pixel column counter in bits */

	memset(abPrintBlock, 0, wSizePrintBlock);

	/* Number of bytes to hold the entire image */
	abPrintBlock[0] = (wSizePrintBlock - 5) & 0xff;	// LOW(bmpSize)
	abPrintBlock[1] = (wSizePrintBlock - 5) >> 8;	// HIGH(bmpSize)

	/* Number of bytes for each raster line. */
	abPrintBlock[4] = (EAN13_PRINT_WIDTH/8);

	/* Set up the start column */
    wTheColumn = EAN13_START_COLUMN;

    /*
    ** The first thing to do is to count the number of characters in the
    ** input string.  If it is not valid then nothing will be printed,
    */

    fError = false;
    for ( n = 0 ; bpChar[n] > 0x20 ; n++ )
    {
        if ( (bpChar[n] & 0xF0) != 0x30 )
        {
            fError = true;
        }
    }

    if (( n > 13 ) || (n < 11 ) ||
        ( fError == true ))
    {
        /*
        ** This is an invalid barcode request
        */

        return -1;
    }

    /*
    ** OK, the barcode looks validish. Lets build the final string
    */

    bBarCode[0] = LEFT_GUARD;       /* insert the left guard */

    if ( n == 11 )
    {
        bLeftType = 0;
        bCount2 = 0;
    }
    else
    {
        bLeftType = bpChar[0] & 0x0F;
        bCount2 = 1;
    }


    for ( bCount = 1 ; bCount < 7 ; bCount2++ )
    {
        bBarCode[bCount++] = bpChar[bCount2] & 0x0F;
    }

    bBarCode[bCount++] = CENTRE_GUARD;      /* insert the middle guard */

    for ( ; bCount2 < n ; bCount2++ )
    {
        bBarCode[bCount++] = bpChar[bCount2] & 0x0F;
    }

    bChecksum = bLeftType + (bBarCode[1] * 3) +
                bBarCode[2] + (bBarCode[3] * 3) +
                bBarCode[4] + (bBarCode[5] * 3) +
                bBarCode[6] + (bBarCode[8] * 3) +
                bBarCode[9] + (bBarCode[10] * 3) +
                bBarCode[11] + (bBarCode[12] * 3);

    bChecksum = bChecksum%10;
    bBarCode[13] = (10 - (bChecksum%10))%10;

    bBarCode[14] = RIGHT_GUARD;             /* insert the right hand guard */

    /*
    ** We now have the string to turn into a barcode
    */

    for ( i = 0 ; i < 15 ; i++ )
    {
        switch (bBarCode[i])
        {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
                /*
                ** This is a normal numerics - but is it left or right?
                */

                if ( i < 7 )
                {
                    /*
                    ** we are on the left of the barcode therefore the position
                    ** is used to define which table is used
                    */

                    bIndex = sCodeSets[bLeftType][i-1];
                }
                else
                {
                    /*
                    ** Otherwise it is always the third table
                    */

                    bIndex = TYPE_C;
                }

                bIndex += (bBarCode[i] * 3);    /* now the tanle offset */

                PrintCharLine[0] = sFontEAN13Table[bIndex++];
                PrintCharLine[1] = sFontEAN13Table[bIndex++];
                PrintCharLine[2] = sFontEAN13Table[bIndex];
                PrintCharLine[3] = 0x00;
                wWidth = EAN_NORMAL_WIDTH;

                break;

            case LEFT_GUARD:
                /*
                ** This is the left-hand guard character
                */

                PrintCharLine[0] = LeftGuard[0];
                PrintCharLine[1] = LeftGuard[1];
                PrintCharLine[2] = 0x00;
                PrintCharLine[3] = 0x00;
                wWidth = LEFT_GUARD_WIDTH;

                break;

            case RIGHT_GUARD:
                /*
                ** This is the right-hand guard character
                */

                PrintCharLine[0] = RightGuard[0];
                PrintCharLine[1] = RightGuard[1];
                PrintCharLine[2] = 0x00;
                PrintCharLine[3] = 0x00;
                wWidth = RIGHT_GUARD_WIDTH;

                break;

            case CENTRE_GUARD:
                /*
                ** This is the centre guard character
                */

                PrintCharLine[0] = CentreGuard[0];
                PrintCharLine[1] = CentreGuard[1];
                PrintCharLine[2] = 0x00;
                PrintCharLine[3] = 0x00;
                wWidth = CENTRE_GUARD_WIDTH;

                break;

            default:
                /*
                ** There is something wrong here
                */

                return -1;
        }

        bOffset = (uchar)(wTheColumn & 0x07);

        if ( bOffset != 0 )
        {
            /*
            ** If we are not on a clean boundary then it must be rotated
            */

            RotateEan13( bOffset, PrintCharLine );
        }

		for (row=0; row < EAN13_PRINT_HEIGHT; row += 1)
		{
			for ( bCount=0 ; bCount < 4 ; bCount++ )
			{
				int columnOffset = (wTheColumn / 8) + bCount + (row * (EAN13_PRINT_WIDTH/8));
				abPrintBlock[5 + columnOffset] |= PrintCharLine[bCount];
			}
		}

        wTheColumn += wWidth;       /* move on to the next position */
    }
	return n;
}

static void RotateEan13(uchar n, uchar * PrintCharLine)
{
	ulong e = (PrintCharLine[0] << 24) | (PrintCharLine[1] << 16) | (PrintCharLine[2] << 8) | PrintCharLine[3];
	e >>= n;
	PrintCharLine[0] = (uchar)(e >> 24);
	PrintCharLine[1] = (uchar)(e >> 16);
	PrintCharLine[2] = (uchar)(e >> 8);
	PrintCharLine[3] = (uchar)e;
}
