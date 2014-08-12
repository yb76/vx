#ifndef __INPUT_H
#define __INPUT_H

/*
**-----------------------------------------------------------------------------
** PROJECT:        AURIS
**
** FILE NAME:      Input.h
**
** DATE CREATED:   10 July 2007
**
** AUTHOR:         Tareq Hafez
**
** DESCRIPTION:    Key and Event Definitions
**-----------------------------------------------------------------------------
*/

//
//-----------------------------------------------------------------------------
// Include Files.
//-----------------------------------------------------------------------------
//
#include "timer.h"
#include "inputmacro.h"

//
//-----------------------------------------------------------------------------
// Type Definitions.
//-----------------------------------------------------------------------------
//

/*
** The following type definitions  are used to access the keypad input congfiguration
*/

typedef struct
{
    T_KEYBITMAP tKeyBitmap;
    T_EVTBITMAP tEvtBitMap;
} T_INPUTBITMAP;

typedef enum
{
	E_INP_NO_ENTRY = 0,
	E_INP_AMOUNT_ENTRY,
	E_INP_STRING_ENTRY,
	E_INP_STRING_ALPHANUMERIC_ENTRY,
	E_INP_STRING_ALPHANUMERIC_ENTRY_1,
	E_INP_STRING_HEX_ENTRY,
	E_INP_PIN,
	E_INP_DATE_ENTRY,
	E_INP_PERCENT_ENTRY,
	E_INP_DECIMAL_ENTRY,
	E_INP_MMYY_ENTRY,
	E_INP_TIME_ENTRY
} E_INP_ENTRY_TYPE;

typedef struct
{
    E_INP_ENTRY_TYPE type;
    uchar row;
    uchar col;
    uchar length;
	ulong minLength;
	uchar decimalPoint;
} T_INP_ENTRY;


typedef struct
{
	int x1,y1,x2,y2;
	char sEvtCode[30];
	bool isAlpha;
}T_MenuTchDtl;

typedef struct
{
	
	int cnt;
	int pressed ;
	T_MenuTchDtl buttons[10];
}T_MenuTch;

//
//-----------------------------------------------------------------------------
// Function Declaration
//-----------------------------------------------------------------------------
//
void InpTurnOn(void);
int InpTurnOff(bool serial0);

void VMACInactive(void);
void VMACHotKey(char * appName, int event);
void VMACDeactivate(void);
int VMACLoop(void);

void InpSetOverride(bool override);
void InpSetString(char * value, bool hidden, bool override);
char * InpGetString(void);

void InpSetNumber(ulong value, bool override);
ulong InpGetNumber(void);

uchar InpGetKeyEvent(T_KEYBITMAP keyBitmap, T_EVTBITMAP * evtBitmap, T_INP_ENTRY inpEntry, ulong timeout, bool largeFont, bool flush, bool * keyPress);
//uchar InpGetKeyEvent(T_KEYBITMAP keyBitmap, T_EVTBITMAP * evtBitmap, T_INP_ENTRY inpEntry, ulong timeout, bool largeFont, bool flush, bool * keyPress, int ** ppX1, int ** ppX2, int ** ppY1, int ** ppY2);
void InpBeep(uchar count, uint onDuration, uint offDuration);
bool InpGetMCRTracks(	char * ptTrack1, uchar * pbTrack1Length,
						char * ptTrack2, uchar * pbTrack2Length,
						char * ptTrack3, uchar * pbTrack3Length);

int DisplayObject(const char *lines,T_KEYBITMAP keyBitmap,T_EVTBITMAP keepEvtBitmap,int timeout, char* outevent, char* outinput);
int InitCTLS(void);
#endif /* __INPUT_H */
