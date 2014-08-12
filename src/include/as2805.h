#ifndef __AS2805_H
#define __AS2805_H

/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       as2805.h
**
** DATE CREATED:    1 February 2008
**
** AUTHOR:			Tareq Hafez
**
** DESCRIPTION:     Header file for AS2805 module
**
**-----------------------------------------------------------------------------
*/

/*
**-----------------------------------------------------------------------------
** Constants
**-----------------------------------------------------------------------------
*/
#define	C_BITMAP			0
#define	C_BCD				1
#define	C_AMOUNT			2
#define	C_STRING			3
#define	C_BCD_LINK			4

#define	C_MMDDhhmmss		10
#define	C_MMDD				11
#define	C_YYMM				12
#define	C_hhmmss			13
#define	C_YYMMDD			14

#define	C_LLNVAR			20
#define	C_LLAVAR			21

#define	C_LLLNVAR			30
#define	C_LLLVAR			31
#define	C_LLVAR				32

// Used when making or breaking an AS2805 stream
// Must not conflioct with C_LOOP and C_END_LOOP above
#define	C_LOOP				98
#define	C_END_LOOP			99


// Used when breaking an AS2805 stream
// Must not conflioct with C_LOOP and C_END_LOOP above
#define	C_GET				1000
#define	C_CHK				1001
#define	C_IGN				1002
#define	C_PUT				1003	// This is used during ()NEW_OBJECT,but postentially can be used for AS2805. Currently ()AS2805_MAKE assumes a C_PUT and does not use it
#define	C_ADD				1004
#define	C_GETS				1005


typedef struct
{
    short action;
    char type[5];
    short fieldnum;
    uchar *data;
} T_AS2805FIELD;

/*
**-----------------------------------------------------------------------------
** Type definitions
**-----------------------------------------------------------------------------
*/

/*
**-----------------------------------------------------------------------------
** Functions
**-----------------------------------------------------------------------------
*/

uchar * AS2805Init(uint size);
void AS2805Close(void);

uchar * AS2805Position(uint * length);

void AS2805BcdLength(bool state);

void AS2805SetBit(uchar field);

void AS2805BufferPack(char * data, uchar format, uint size, uchar * buffer, uint * index);
uint AS2805Pack(uchar field, char * data);
void AS2805BufferUnpack(char * data, uchar format, uint maxOctets, uchar * buffer, uint * index);
void AS2805Unpack(uchar field, char * data, uchar * buffer, uint length);
void AS2805OFBAdjust(uchar * source, uchar * dest, uint length);
void AS2805OFBVariation(int my_maxField, int my_additionalField);

#endif /* __AS2805_H */
