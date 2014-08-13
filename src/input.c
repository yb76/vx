/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       input.c
**
** DATE CREATED:    10 July 2007
**
** AUTHOR:			Tareq Hafez    
**
** DESCRIPTION:     This module handles smartcard, magstripe and keyboard entry
**-----------------------------------------------------------------------------
*/

/*
**-----------------------------------------------------------------------------
** Include Files
**-----------------------------------------------------------------------------
*/

/*
** Standard include files.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
** Project include files.
*/
#include <svc.h>
#include <svc_sec.h>
#include <errno.h>

//dwarika .. for CTLS
#include <ctlsinterface.h>
#include "Contactless.h"

// for EOS log
#include <eoslog.h>

//KK added, include files for Verifone build

#ifndef _DEBUG
#include <cardslot.h>
#endif

#ifdef __VMAC
#	include "eeslapi.h"
#	include "logsys.h"
#	include "devman.h"
#	include "vmac.h"
#	include "version.h"
#endif

/*
** Local include files
*/
#include "auris.h"
#include "input.h"
#include "display.h"
#include "timer.h"
#include "irisfunc.h"
#include "iris.h"
#include "comms.h"
#include "security.h"
#include "alloc.h"
#ifdef __EMV
	#include "emv.h"
#endif
#include "printer.h"

#include "LibCtls.h"

#include "ctlsmod.h"

extern T_MenuTch MenuTch;

/*
**-----------------------------------------------------------------------------
** Constants
**-----------------------------------------------------------------------------
*/
const struct
{
	T_KEYBITMAP	tKeyBitmap;
	uchar bKeyCode;
} sKey[] = 
{   {KEY_0_BIT, KEY_0}, {KEY_1_BIT, KEY_1}, {KEY_2_BIT, KEY_2}, 
	{KEY_3_BIT, KEY_3}, {KEY_4_BIT, KEY_4}, {KEY_5_BIT, KEY_5}, 
	{KEY_6_BIT, KEY_6}, {KEY_7_BIT, KEY_7}, {KEY_8_BIT, KEY_8}, 
	{KEY_9_BIT, KEY_9},
	{KEY_FUNC_BIT, KEY_FUNC}, {KEY_CNCL_BIT, KEY_CNCL}, {KEY_ASTERISK_BIT, KEY_ASTERISK},
	{KEY_OK_BIT, KEY_OK}, {KEY_CLR_BIT, KEY_CLR},  {KEY_LCLR_BIT, KEY_LCLR},
	{KEY_SK1_BIT, KEY_SK1}, {KEY_SK2_BIT, KEY_SK2}, {KEY_SK3_BIT, KEY_SK3},  {KEY_SK4_BIT, KEY_SK4},
	{KEY_F0_BIT, KEY_F0}, {KEY_F1_BIT, KEY_F1}, {KEY_F2_BIT, KEY_F2},  {KEY_F3_BIT, KEY_F3}, {KEY_F4_BIT, KEY_F4},  {KEY_F5_BIT, KEY_F5},
	{KEY_NO_BITS, KEY_NONE}
};

const char HiddenString[] = "********************";

/*
**-----------------------------------------------------------------------------
** Module variable definitions and initialisations.
**-----------------------------------------------------------------------------
*/
static bool Override;
static ulong Number;
static char String[MAX_COL*3+1];
static bool HiddenAttribute;

extern int prtHandle;
extern int conHandle;
extern int cryptoHandle;
int mcrHandle = 0;
bool numberEntry = false;

#ifdef __VMAC

	int timerID = -1;
	int wait_10003 = -1;
	int active = -1;
	int firstInit = -1;

	typedef struct
	{
		char logicalName[17];
		char lowPAN[20];
		char highPAN[20];
	} typeRedirectPAN;
	
	typeRedirectPAN * redirectPAN = NULL;

	int maxRedirectPAN = 0;

	int hotKey = -1;
	char hotKeyApp[30];
	int hotKeyEvent;

#else
	#define	LOG_PRINTF(...)
#endif

int ser_line0;
int ser_line1;

//
//-----------------------------------------------------------------------------
// FUNCTION   : InpSetOverride
//
// DESCRIPTION:	Sets the initial override condition of an input
//
// PARAMETERS:	override	<=	boolean.
//
// RETURNS:		NONE
//	
//-----------------------------------------------------------------------------
//
void InpSetOverride(bool override)
{
	Override = override;
}


//
//-----------------------------------------------------------------------------
// FUNCTION   : InpSetString
//
// DESCRIPTION:
//	Sets the entry string to a value. This can be used to
//	initialise the string or set it to a default value.
//
// PARAMETERS:
//	value	<=  The value to set the string to.
//	hidden	<=  Indicates if the string entry should be hidden
//
// RETURNS:
//	NONE
//-----------------------------------------------------------------------------
//
void InpSetString(char * value, bool hidden, bool override)
{
	numberEntry = false;

	memset(String, 0, sizeof(String));
	strcpy(String, value);

	HiddenAttribute = hidden;

	Override = override;
}

//
//-----------------------------------------------------------------------------
// FUNCTION   : InpGetString
//
// DESCRIPTION:
//	Returns the last string entered using a String Entry field.
//
// PARAMETERS:
//	NONE
//
// RETURNS:
//	(char *) The string entered
//-----------------------------------------------------------------------------
//
char * InpGetString(void)
{
	if (numberEntry)
		return ltoa(Number, String, 10);
	else
		return String;
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : InpSetNumber
**
** DESCRIPTION:
**	Sets the entry number to a value. This can be used to
**	initialise the number or set it to a default value.
**
** PARAMETERS:
**	value		<=  The value to set the number to.
**	override	<=	Whether the first numeric entry should override the number (true)
**					or just append to it (false)
**
** RETURNS:
**	NONE
**-----------------------------------------------------------------------------
*/
void InpSetNumber(ulong value, bool override)
{
	numberEntry = true;
	Number = value;
	HiddenAttribute = false;
	Override = override;
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : InpGetNumber
**
** DESCRIPTION:
**	Returns the last number entered using an Number Entry field.
**
** PARAMETERS:
**	NONE
**
** RETURNS:
**	(ulong) The number entered
**-----------------------------------------------------------------------------
*/
ulong InpGetNumber(void)
{
    return Number;
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : InpTurnOn
**
** DESCRIPTION:	Turns on all open resources
**
**
** PARAMETERS:	None
**
**
** RETURNS:		
**
**-----------------------------------------------------------------------------
*/
void InpTurnOn(void)
{
	DispInit();

	if (ser_line0)
		__ser_reconnect(0);

	if (ser_line1)
		__ser_reconnect(1);

	ser_line0 = ser_line1 = -1;

	if (cryptoHandle == -2)
		SecurityInit();
	else if (cryptoHandle == -1)
		cryptoHandle = open("/dev/crypto", 0);
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : InpTurnOff
**
** DESCRIPTION:	Turns off all open resources
**
**
** PARAMETERS:	None
**
**
** RETURNS:		
**
**-----------------------------------------------------------------------------
*/
int InpTurnOff(bool serial0)
{
	int released = -1;

	// Just in case it was left connected
	__pstn_disconnect();

	// Disconnect TCP and ethernet ports if open
	__tcp_disconnect_completely();

	if (conHandle != -1)
	{
		close(conHandle);
		LOG_PRINTF(("Releasing console..."));
		conHandle = -1;
		released = 1;
	}

	if (mcrHandle)
	{
		close(mcrHandle);
		LOG_PRINTF(("Releasing mag reader..."));
		mcrHandle = 0;
		released = 1;
	}
	
	if (ser_line0 == -1)
	{
		if (serial0)
		{
			ser_line0 = ____ser_disconnect(0);
			if (ser_line0)
			{
				LOG_PRINTF(("Releasing com1..."));
				released = 1;
			}
		}
		else ser_line0 = 0;
	}

	if (ser_line1 == -1)
	{
		ser_line1 = ____ser_disconnect(1);
		if (ser_line1)
		{
			LOG_PRINTF(("Releasing com2..."));
			released = 1;
		}
	}

	if (prtHandle != -1)
	{
		close(prtHandle);
		LOG_PRINTF(("Releasing printer (COM4)..."));
		prtHandle = -1;
		released = 1;
	}

	if (cryptoHandle > -1)
	{
		close(cryptoHandle);
		LOG_PRINTF(("Releasing crypto (CRYPTO)..."));
		cryptoHandle = -1;
		released = 1;
	}

#ifndef __VX670
	CommsReInitPSTN();
#endif

	return released;
}


/*
**-----------------------------------------------------------------------------
** FUNCTION   : InpGetKeyEvent
**
** DESCRIPTION:
**	Returns the next key or event as requested.
**
** PARAMETERS:	NONE
**
** RETURNS:
**		NOERROR
**-----------------------------------------------------------------------------
*/
uchar InpGetKeyEvent(T_KEYBITMAP keyBitmap, T_EVTBITMAP * evtBitmap, T_INP_ENTRY inpEntry, ulong timeout, bool largeFont, bool flush, bool * keyPress)
{
	int myTimer = -1;
	uchar keyCode;
	uchar retKeyCode = KEY_NONE;
	char entry[MAX_COL*3+1];
	char inpStr[MAX_COL*3+1];
	T_EVTBITMAP myEvtBitmap = EVT_NONE;
	int lastPinStatus = 0;
	char numPinPress = 0;
//	int activeStatus;
	char curr_symbol[20];
	unsigned int curr_decimal=2;

    uchar index;
    bool displayEntry = true;
    bool override = Override;

	char retVal;
	int ret;

	char sPrntBuff[50];
	char sPressKeyCode[5];
	char * pKeyCode;

	int nLoop;
	int alpha_press = 0;

    uchar lastKeyIndex = 0;
    uchar lastKey = 12;	    /* Dummy out-of-range value */
	unsigned long expiry;
    const struct
    {
		uchar sequence[7];
    } keys[12] = {  {{'0','-',' ','+','_','!','@'}},
					{{'1','Q','Z','.','q','z','/'}},
					{{'2','A','B','C','a','b','c'}},
					{{'3','D','E','F','d','e','f'}},
					{{'4','G','H','I','g','h','i'}},
					{{'5','J','K','L','j','k','l'}},
					{{'6','M','N','O','m','n','o'}},
					{{'7','P','R','S','p','r','s'}},
					{{'8','T','U','V','t','u','v'}},
					{{'9','W','X','Y','w','x','y'}},
					{{'*',',','\'','"','`','$','%'}},
					{{'#','^','&','(',')','[',']'}}};


    /* If we did not request any keys or events, go back */
    if (keyBitmap == KEY_NO_BITS && (evtBitmap == NULL || (evtBitmap && *evtBitmap == EVT_NONE)))
		return KEY_NONE;

	/* Adjust the last key and last key index to reflect the last character in the string if the string not empty */
	if (strlen(String))
	{
		for (lastKey = 0; lastKey < 12; lastKey++)
		{
			for (lastKeyIndex = 0; lastKeyIndex < 7; lastKeyIndex++)
			{
				if (String[strlen(String)-1] == keys[lastKey].sequence[lastKeyIndex])
					break;
			}
			if (lastKeyIndex < 7) break;
		}
	}

	if (flush)
	{
		//flush screen touch until pen up
		flush_touch();

		// Flush the current keyboard entries
		while(read(conHandle, (char *) &keyCode, 1) == 1);

		/* Enable or disable any MCR tracks */
		 if (evtBitmap && (*evtBitmap & EVT_MCR))
		{
			char temp[6];
			if (mcrHandle == 0) mcrHandle = open(DEV_CARD, 0);
			read(mcrHandle, temp, sizeof(temp));
		}
		else
		{
			if (mcrHandle)
			{
				close(mcrHandle);
				mcrHandle = 0;
			}
		}
	}


    // If timeout is requested, arm the default timer
    if (evtBitmap && (*evtBitmap & EVT_TIMEOUT))
		myTimer = set_timer(timeout, EVT_TIMER);

	if (inpEntry.type == E_INP_PIN)
	{
		PINPARAMETER param;
		uchar data[20];
		int iret = 0;

		memset(&param, 0, sizeof(param));
		// Set the next printing location

		//Dwarika .. Vx680 ..Change the coordinate mode
	if(get_display_coordinate_mode() == PIXEL_MODE)
		set_display_coordinate_mode(CHARACTER_MODE);

		param.ucMin = inpEntry.row - 1;
		param.ucMax = inpEntry.col + 1;

		DispText(inpEntry.length? "ENTER PIN AND OK": "ENTER PIN OR OK", 8, 7, false, true, false);
		DispText(" ", 11, 9, false, true, false);
		if(param.ucMin < 4 ) param.ucMin = 4;
		if(param.ucMax >12 ) param.ucMax = 12;

		param.ucEchoChar = '*';
		param.ucDefChar = ' ';
		if (keyBitmap & KEY_CLR_BIT)
			param.ucOption |= 0x10;
		if (inpEntry.length == 0)
			param.ucOption |= 0x02;

		iPS_SetPINParameter(&param);
		iPS_SelectPINAlgo(0x0B);
		iret = iPS_RequestPINEntry(0, data);
		if(iret == E_KM_ACCESS_DENIED) {
			DispClearScreen();
			DispText("ACCESS DENIED", 3, 0, false, true, false);
			SVC_WAIT(3000);
			return(KEY_CNCL);
		}
	}

	if (inpEntry.type == E_INP_AMOUNT_ENTRY)
	{
		strcpy(curr_symbol, "$ ");
		curr_decimal = 2;
	}

    do {
		long evt = 0;

		if (inpEntry.type == E_INP_PIN)
		{
			int status;
			PINRESULT result;

			iPS_GetPINResponse(&status, &result);
			if (status == 0)
				retKeyCode = KEY_OK;
			else if (status == 6)
				retKeyCode = KEY_NO_PIN;
			else if (status == 5) {
				retKeyCode = KEY_CNCL;
			}
			else if (status == 0x0A)
				retKeyCode = KEY_CLR;
			else
			{
				if (status != lastPinStatus || result.encPinBlock[0] != numPinPress)
				{
					lastPinStatus = status;
					numPinPress = result.encPinBlock[0];
					if (myTimer>0) {
						clr_timer(myTimer);
						myTimer = set_timer(timeout,EVT_TIMER);
					}
				}
			}
			
			if(retKeyCode != KEY_NONE) {
				break;
			}
		}
		else
		{
			/* If a number or amount display is requested, display it */
			if (displayEntry == true && inpEntry.type != E_INP_NO_ENTRY)
			{
				uchar entryOffset = 0;

				memset(entry, ' ', inpEntry.length);
				if (inpEntry.type == E_INP_AMOUNT_ENTRY)
				{
					int i;
					int multiplier = 1;
					for (i = 0; i < curr_decimal; i++) multiplier *= 10;
					sprintf(inpStr, "%s%ld", curr_symbol, Number/multiplier);
					if (curr_decimal)
						sprintf(&inpStr[strlen(inpStr)], "%s%0*ld", ".", curr_decimal, Number%multiplier);
				}
				else if (inpEntry.type == E_INP_PERCENT_ENTRY)
					sprintf(inpStr, "%ld.%02ld%%", Number/100, Number%100);
				else if (inpEntry.type == E_INP_DECIMAL_ENTRY)
				{
					int i;
					unsigned long divider = 1;

					for (i = 0; i < inpEntry.decimalPoint; i++)
						divider *= 10;

					sprintf(inpStr, "%ld.%0*ld", Number/divider, inpEntry.decimalPoint, Number%divider);
				}
				else if (inpEntry.type == E_INP_DATE_ENTRY)
					sprintf(inpStr, "%c%c/%c%c/%c%c",	String[0]?String[0]:'D', String[1]?String[1]:'D',
														String[2]?String[2]:'M', String[3]?String[3]:'M',
														String[4]?String[4]:'Y', String[5]?String[5]:'Y');
				else if (inpEntry.type == E_INP_MMYY_ENTRY)
					sprintf(inpStr, "%c%c/%c%c",	String[0]?String[0]:'M', String[1]?String[1]:'M',
													String[2]?String[2]:'Y', String[3]?String[3]:'Y');
				else if (inpEntry.type == E_INP_TIME_ENTRY)
					sprintf(inpStr, "%c%c:%c%c:%c%c",	String[0]?String[0]:'H', String[1]?String[1]:'H',
														String[2]?String[2]:'M', String[3]?String[3]:'M',
														String[4]?String[4]:'S', String[5]?String[5]:'S');
				else
					strcpy(inpStr, String);

				if (inpEntry.type == E_INP_AMOUNT_ENTRY || inpEntry.type == E_INP_PERCENT_ENTRY  || inpEntry.type == E_INP_DECIMAL_ENTRY)
					entryOffset = inpEntry.length - strlen(inpStr);

				if (HiddenAttribute == true) {
					memcpy(&entry[entryOffset], HiddenString, strlen(inpStr));
				}
				else
					memcpy(&entry[entryOffset], inpStr, strlen(inpStr));
				entry[inpEntry.length] = '\0';

				if(HiddenAttribute || inpEntry.type == E_INP_AMOUNT_ENTRY) {
					int col = (inpEntry.col==255)?(240 - inpEntry.length*8)/2: inpEntry.col;
					int pos_x1 = col - 10;
					int pos_y1 = inpEntry.row - 10;
					int pos_x2 = col - 10 + inpEntry.length * 8 + 20;
					int pos_y2 = inpEntry.row + 26;

					draw_line( pos_x1,pos_y1,pos_x2,pos_y1 ,1, 4523 );
					draw_line( pos_x1,pos_y1,pos_x1,pos_y2 ,1, 4523 );
					draw_line( pos_x2,pos_y1,pos_x2,pos_y2 ,1, 4523 );
					draw_line( pos_x1,pos_y2,pos_x2,pos_y2 ,1, 4523 );
				}

				DispText(entry, inpEntry.row, inpEntry.col, false, largeFont, false);
				displayEntry = false;
			}

		}

		if (inpEntry.type == E_INP_PIN ) {
			if (evtBitmap && (*evtBitmap & EVT_TIMEOUT))
				evt = read_evt( EVT_TIMER);
			else continue;
		}
#ifdef __EMV
		else if (myEvtBitmap == EVT_NONE && evtBitmap && *evtBitmap & EVT_SCT_IN && EmvIsCardPresent()) {
			evt = EVT_ICC1_INS;
		}
#endif
	
	
		//else evt = wait_event();
		else evt = read_event();

		//Dwarika .. For touch
		keyCode = 0x00;

		if(MenuTch.cnt) {
			if(alpha_press) {
				flush_touch();
				alpha_press = 0;
			}
			expiry = read_ticks() + 16 * 5;
			while (read_ticks() < expiry)
			{
				int penDown =0, new_touch_x = 0, new_touch_y = 0;
				penDown = get_touchscreen(&new_touch_x, &new_touch_y);

				if(penDown >0 && new_touch_x > 0 && new_touch_x < 240 && new_touch_y > 0 && new_touch_y < 320)
				{
					int nAlphaBtn=0;
					for(nLoop = 0; nLoop <10 && MenuTch.buttons[nLoop].x2+MenuTch.buttons[nLoop].y2>0 ; nLoop++)
					{
						if((MenuTch.buttons[nLoop].x1 <= new_touch_x) && (MenuTch.buttons[nLoop].y1 <= new_touch_y) 
										&& (MenuTch.buttons[nLoop].x2 >= new_touch_x) && (MenuTch.buttons[nLoop].y2 >= new_touch_y))
							{
								if( MenuTch.buttons[nLoop].isAlpha ) {
									evt = EVT_KBD;
									keyCode = KEY_ALPHA;
									alpha_press = 1;
								} else {
									evt = EVT_TOUCH;
									myEvtBitmap = EVT_TOUCH;
									MenuTch.pressed = nLoop ;
								}
								sound(58, 80); // 1500Hz 500msec
								break;
							}
					}		

					break;
				}					
				
			 }
		}
		//Dwarika .. For touch

		if((keyCode == 0x00)&& (evt & EVT_KBD))
		{
			read(conHandle, (char *) &keyCode, 1);
		}

		/* If key press is requested and a key is pressed... */
		//if (evt & EVT_KBD && inpEntry.type != E_INP_PIN && keyBitmap != KEY_NO_BITS && read(conHandle, (char *) &keyCode, 1) == 1)
		if ((evt & EVT_KBD) && inpEntry.type != E_INP_PIN && keyBitmap != KEY_NO_BITS && keyCode)
		{
			if(keyCode != KEY_ALPHA)
				keyCode &= 0x7F;

			//__tcp_disconnect_extend();

			/* If timeout is requested, re-arm the default timer */
			if (myTimer>0)
			{
				clr_timer(myTimer);
				myTimer = set_timer(timeout,EVT_TIMER);
			}

			/* Check if the key press should be returned to the caller */
			for (index = 0; sKey[index].bKeyCode != KEY_NONE; index++)
			{
				if (sKey[index].bKeyCode == keyCode && (sKey[index].tKeyBitmap & keyBitmap))
				{
					
					if (keyCode == KEY_CLR && inpEntry.type != E_INP_NO_ENTRY)
					{
						if (inpEntry.type == E_INP_AMOUNT_ENTRY || inpEntry.type == E_INP_PERCENT_ENTRY || inpEntry.type == E_INP_DECIMAL_ENTRY)
						{
							if (Number) break;
						}
						else if (strlen(String)) break;
					}
					else if (keyCode == KEY_OK && inpEntry.type != E_INP_NO_ENTRY)
					{
						if (inpEntry.type == E_INP_AMOUNT_ENTRY || inpEntry.type == E_INP_PERCENT_ENTRY || inpEntry.type == E_INP_DECIMAL_ENTRY)
						{
							if (Number < inpEntry.minLength)	// This is actually minimum amount and not minimum length
								{
  								InpBeep(2, 20, 5);
								break;
								}
						}
						else if (inpEntry.type == E_INP_DATE_ENTRY || inpEntry.type == E_INP_TIME_ENTRY)
						{
							if (strlen(String) < 6) {
  								InpBeep(2, 20, 5);
								break;
							}
						}
						else if (inpEntry.type == E_INP_MMYY_ENTRY)
						{
							if (strlen(String) < 4) {
  								InpBeep(2, 20, 5);
								break;
							}
						}
						else if (strlen(String) < inpEntry.minLength) {
  							InpBeep(2, 20, 5);
							break;
						}
					}
					retKeyCode = keyCode;
					break;
				}
			}
			if(retKeyCode != KEY_NONE) break; // second break 

			/* Now check if the key press should be consumed by an entry field */
			if ((keyCode >= '0' && keyCode <= '9') || keyCode == KEY_CLR ||
				(keyCode == KEY_ASTERISK && (inpEntry.type == E_INP_STRING_ENTRY || inpEntry.type == E_INP_STRING_ALPHANUMERIC_ENTRY)) ||
				(keyCode == KEY_FUNC && (inpEntry.type == E_INP_STRING_ENTRY || inpEntry.type == E_INP_STRING_ALPHANUMERIC_ENTRY)) )
			{
				if (inpEntry.type == E_INP_AMOUNT_ENTRY || inpEntry.type == E_INP_PERCENT_ENTRY || inpEntry.type == E_INP_DECIMAL_ENTRY)
				{
					if (keyCode >= '0' && keyCode <= '9' && (strlen(inpStr) < inpEntry.length || override || Number == 0))
					{
						if (override)
							Number = keyCode - '0';
						else
							Number = Number * 10 + keyCode - '0';
					}
					else if (keyCode == KEY_CLR)
						Number /= 10;

					override = false;
					displayEntry = true;
				}
				else
				{

					if (override == true)
					{

					//	String[0] = '\0'; //Dwarika .. for testing CLR CHAR
						override = false;
						displayEntry = true;
					}

					if (keyCode == KEY_CLR)
					{

						if (HiddenAttribute)
						{
							if (strlen(String))
							{
								String[strlen(String)] = '\0';
								displayEntry = true;
							}
							else
							{
								String[0] = '\0';
								displayEntry = true;
							}
						}

						if (strlen(String))
						{

							String[strlen(String)-1] = '\0';
							displayEntry = true;
						}
						/* Adjust the last key and last key index to reflect the last character in the string if string not empty */
						if (strlen(String))
						{

							for (lastKey = 0; lastKey < 12; lastKey++)
							{
								for (lastKeyIndex = 0; lastKeyIndex < 7; lastKeyIndex++)
								{
									if (String[strlen(String)-1] == keys[lastKey].sequence[lastKeyIndex])
										break;
								}
								if (lastKeyIndex < 7) break;
							}
						}
						else 
						{

							lastKey = 12;
						}
					}
					else if (inpEntry.type == E_INP_DATE_ENTRY)
					{
						int position = strlen(String);

						if ((position == 0 && keyCode >= '0' && keyCode <= '3') ||

							(position == 1 && String[0] == '0' && keyCode >= '1' && keyCode <= '9') ||
							(position == 1 && (String[0] == '1' || String[0] == '2') && keyCode >= '0' && keyCode <= '9') ||
							(position == 1 && String[0] == '3' && keyCode >= '0' && keyCode <= '1') ||

							(position == 2 && keyCode >= '0' && keyCode <= '1') ||

							(position == 3 && (	(String[2] == '0' && keyCode >= '1' && keyCode <= '9') &&
												(String[0] != '3' || keyCode != '2') &&
												(String[0] != '3' || String[1] != '1' || (keyCode != '4' && keyCode != '6' && keyCode != '9')) )) ||
							(position == 3 && ( (String[2] == '1' && keyCode >= '0' && keyCode <= '2') &&
												(String[0] != '3' || String[1] != '1' || keyCode != '1') )) ||

							position == 4 ||

							(position == 5 && ((((String[4]-'0')*10 + keyCode -'0') % 4) == 0 || String[0] != '2' || String[1] != '9' || String[2] != '0' || String[3] != '2')))
						{
							String[position+1] = '\0';
							String[position] = keyCode;
							displayEntry = true;
						}
					}
					else if (inpEntry.type == E_INP_MMYY_ENTRY)
					{
						int position = strlen(String);

						if ((position == 0 && keyCode >= '0' && keyCode <= '1') ||

							(position == 1 && String[0] == '0' && keyCode >= '1' && keyCode <= '9') ||
							(position == 1 && String[0] == '1' && keyCode >= '0' && keyCode <= '2') ||

							position == 2 ||

							position == 3)
						{
							String[position+1] = '\0';
							String[position] = keyCode;
							displayEntry = true;
						}
					}
					else if (inpEntry.type == E_INP_TIME_ENTRY)
					{
						int position = strlen(String);

						if ((position == 0 && keyCode >= '0' && keyCode <= '2') ||

							(position == 1 && String[0] != '2' && keyCode >= '0' && keyCode <= '9') ||
							(position == 1 && String[0] == '2' && keyCode >= '0' && keyCode <= '3') ||

							((position == 2 || position == 4) && keyCode >= '0' && keyCode <= '5') ||
							((position == 3 || position == 5) && keyCode >= '0' && keyCode <= '9'))
						{
							String[position+1] = '\0';
							String[position] = keyCode;
							displayEntry = true;
						}
					}
					else if (inpEntry.type == E_INP_STRING_ENTRY)
					{
						if (strlen(String) < inpEntry.length)
						{
							if (HiddenAttribute)
							{
								if (keyCode >= '0' && keyCode <= '9')
								{
									String[strlen(String)+1] = '\0';
									String[strlen(String)] = keyCode;
									displayEntry = true;
								}
							}
							else
							{
								if (keyCode >= '0' && keyCode <= '9')
								{
									String[strlen(String)+1] = '\0';
									String[strlen(String)] = keyCode;
									displayEntry = true;
								}
							}
						}
					}
					else if (inpEntry.type == E_INP_STRING_ALPHANUMERIC_ENTRY || inpEntry.type == E_INP_STRING_HEX_ENTRY) //Dwarika.. For No special char .. 20120816
					{

						if (strlen(String) < inpEntry.length)
						{

							if ((keyCode != KEY_ASTERISK) && (keyCode != KEY_FUNC))
							{

								lastKey = keyCode - '0';
								lastKeyIndex = 0;
								String[strlen(String)+1] = '\0';
								String[strlen(String)] = keyCode;
								displayEntry = true;
							}
						}
					}
					
				}
			}
			else if (lastKey < 12 && (inpEntry.type == E_INP_STRING_ALPHANUMERIC_ENTRY || inpEntry.type == E_INP_STRING_HEX_ENTRY))
			{
				if (keyCode == KEY_ALPHA)
				{
					if (++lastKeyIndex == 7) lastKeyIndex = 0;
					if (inpEntry.type == E_INP_STRING_HEX_ENTRY)
					{
						if (lastKey == 2 || lastKey == 3)
						{
							if (lastKeyIndex == 4)
								lastKeyIndex = 0;
						}
						else lastKeyIndex = 0;
					}
					String[strlen(String)-1] = keys[lastKey].sequence[lastKeyIndex];
					displayEntry = true;
				}
			}
		}
		// If the comms event is requested, return if data available
		if (evtBitmap && (*evtBitmap & EVT_SERIAL_DATA) && (evt & EVT_COM1))
			myEvtBitmap = EVT_SERIAL_DATA;

		// If the comms event is requested, return if data available
		else if (evtBitmap && (*evtBitmap & EVT_SERIAL2_DATA) && (evt & EVT_COM2))
			myEvtBitmap = EVT_SERIAL2_DATA;

		/* If the screen timeout expires, return */
		else if (evtBitmap && (*evtBitmap & EVT_TIMEOUT) && (evt & EVT_TIMER))
			myEvtBitmap = EVT_TIMEOUT;

		else if (myEvtBitmap == EVT_NONE && evtBitmap && (*evtBitmap & EVT_MCR) && (evt & EVT_MAG))
			myEvtBitmap = EVT_MCR;

#ifdef __EMV
		else if (myEvtBitmap == EVT_NONE && evtBitmap && *evtBitmap & EVT_SCT_IN && evt & EVT_ICC1_INS)
		{
                myEvtBitmap = EVT_SCT_IN;
		}

		else if (myEvtBitmap == EVT_NONE && evtBitmap && *evtBitmap & EVT_SCT_OUT && evt & EVT_ICC1_REM)
		{
                myEvtBitmap = EVT_SCT_OUT;
		}
#endif


    } while (myEvtBitmap == EVT_NONE);

	//__tcp_disconnect_check();

	clr_timer(myTimer);
	
	// Transfer the result to the caller
	if (evtBitmap) *evtBitmap = myEvtBitmap;

	// Indicate to the caller that no key was detected
	if (retKeyCode == KEY_NONE && inpEntry.type == E_INP_PIN) iPS_CancelPIN();

	if( inpEntry.type == E_INP_PIN ) {
			if( retKeyCode == KEY_OK) gEmv.onlinepin = true;
			else gEmv.onlinepin = false;
	}

	return retKeyCode;
}



/*
**-----------------------------------------------------------------------------
** FUNCTION   : InpBeep
**
** DESCRIPTION:
**	Beeps for the specified count and durations
**
** PARAMETERS:
**	count	    <=  Number of beeps required
**	onDuration  <=	The ON duration in 10msec
**	offDuration <=	The OFF duration in 10msec
**
** RETURNS:
**	None
**-----------------------------------------------------------------------------
*/
void InpBeep(uchar count, uint onDuration, uint offDuration)
{
	uchar index;

	for (index = 0; index < count; index++)
	{
		if (onDuration >= 10)
			error_tone();	// 100 msec beep
		else
			normal_tone();	// 50 msec beep
		TimerArm(NULL, (offDuration+onDuration) * 100);
		while(TimerExpired(NULL) == false);
	}
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : InpGetMCRTracks
**
** DESCRIPTION:	Returns the MCR tracks detected
**
** PARAMETERS:
**				ptTrack1	=>	A buffer to store track1 data (if not NULL).
**				pbTrack1Len	<=>	On entry contains the maximum bytes to read from track1
**								On exit contains the actual number of bytes read.
**				ptTrack2	=>	A buffer to store track2 data (if not NULL).
**				pbTrack2Len	<=>	On entry contains the maximum bytes to read from track2
**								On exit contains the actual number of bytes read.
**				ptTrack3	=>	A buffer to store track3 data (if not NULL).
**				pbTrack3Len	<=>	On entry contains the maximum bytes to read from track3
**								On exit contains the actual number of bytes read.
**
** RETURNS:		NOERROR if all operations are successful
**-----------------------------------------------------------------------------
*/
bool InpGetMCRTracks(	char * ptTrack1, uchar * pbTrack1Length,
						char * ptTrack2, uchar * pbTrack2Length,
						char * ptTrack3, uchar * pbTrack3Length)
{
	char buffer[2+79+2+40+2+107];	// allowing for (count + status + data) for three tracks
	if (read(mcrHandle, buffer, sizeof(buffer)))
	{
		int i;
		int index = 0;
		char * track[3];
		uchar * trackLen[3];

		track[0] = ptTrack1, trackLen[0] = pbTrack1Length;
		track[1] = ptTrack2, trackLen[1] = pbTrack2Length;
		track[2] = ptTrack3, trackLen[2] = pbTrack3Length;

		for (i = 0; i < 3; i++, index += buffer[index])
		{
			if (buffer[index] > 2)
			{
				if (track[i] != NULL)
				{
					if (buffer[index] < *trackLen[i]) *trackLen[i] = buffer[index];
					memcpy(track[i], &buffer[index+2], (*trackLen[i])-2);
					track[i][(*trackLen[i])-2] = '\0';
				}
			}
			else if (track[i] && trackLen[i]) *trackLen[i] = 0;
		}

	}

	return true;
}

#ifdef __VMAC
/*
**-----------------------------------------------------------------------------
** FUNCTION   : VMACInactive
**
** DESCRIPTION:	Waits for an event in a VMAC loop until activated
**
** PARAMETERS:	NONE
**
** RETURNS:		Activation status
**
**-----------------------------------------------------------------------------
*/
void VMACHotKey(char * appName, int event)
{
	strcpy(hotKeyApp, appName);
	hotKeyEvent = event;
	hotKey = 1;
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : VMACDeactivate
**
** DESCRIPTION:	Waits for an event in a VMAC loop until activated
**
** PARAMETERS:	NONE
**
** RETURNS:		Activation status
**
**-----------------------------------------------------------------------------
*/
void VMACDeactivate(void)
{
	unsigned short event = 15001;

	LOG_PRINTF(("Handling DE_ACT EVENT (deactivate)"));

	if (InpTurnOff(false) == 1)
		EESL_send_event("DEVMAN", RES_COMPLETE_EVENT, (unsigned char*) &event, sizeof(short));
}
/*
**-----------------------------------------------------------------------------
** FUNCTION   : VMACLoop
**
** DESCRIPTION:	Waits for an event in a VMAC loop until activated
**
** PARAMETERS:	NONE
**
** RETURNS:		Activation status
**
**-----------------------------------------------------------------------------
*/
int VMACLoop(void)
{
	int status = 0;
	long event;
	static unsigned char firstInit = -1;

	if (false && firstInit == -1)
	{
		unsigned short event = 15001;
		firstInit = 1;

		LOG_PRINTF(("Handling DE_ACT EVENT (first)"));
		InpTurnOff(true);
		EESL_send_event("DEVMAN", RES_COMPLETE_EVENT, (unsigned char*) &event, sizeof(short));
	}

	if (hotKey != -1 )
	{
		VMACDeactivate();
		EESL_send_event(hotKeyApp, hotKeyEvent, NULL, 0);
		hotKey = -1;
	}

	//disable_hot_key();
	do
	{
		event = wait_event();

		if (event & EVT_ACTIVATE) {
			LOG_PRINTF(("IRIS ACTIVATE EVENT %d",event));
		}
		if (event & EVT_DEACTIVATE) {
			LOG_PRINTF(("IRIS DEACTIVATE EVENT %d",event));
			VMACDeactivate();
		}
		if (event & EVT_PIPE || EESL_queue_count())
		{
			unsigned short evtID;
			unsigned char data2[300];
			unsigned short dataSize;
			char senderName[21];

			LOG_PRINTF(("IRIS PIPE EVENT %d",event));

			do
			{
				memset(senderName, 0, sizeof(senderName));
				evtID = EESL_read_cust_evt(data2, sizeof(data2), &dataSize, senderName);
				LOG_PRINTF(("IRIS PIPE EVENT - cust_evt %d",evtID));

				if (evtID == APPLICATION_INIT_EVENT)
				{
					LOG_PRINTF(("VMAC IRIS INIT using %s",GetVMACLibVersion()));
					EESL_send_devman_event("IRIS", 15001, NULL, 0);
				}
				else if (evtID == CONSOLE_AVAILABLE_EVENT)
				{
					LOG_PRINTF(("Handling Console Available EVENT"));
				}
				else if (evtID == APP_RES_SET_UNAVAILABLE)
				{
					LOG_PRINTF(("Handling RES SET UNAVAILABLE EVENT"));
				}
				else if (evtID == CONSOLE_REQUEST_EVENT || evtID == MAG_READER_REQUEST_EVENT || evtID == BEEPER_REQUEST_EVENT || evtID == CLOCK_REQUEST_EVENT ||
						evtID == COMM_1_REQUEST_EVENT || evtID == COMM_2_REQUEST_EVENT || evtID == COMM_3_REQUEST_EVENT || evtID == COMM_4_REQUEST_EVENT || evtID == ETH_1_REQUEST_EVENT)
				{
					unsigned short event = 15001;
					{

						LOG_PRINTF(("Handling DE_ACT EVENT"));

						if (InpTurnOff(false) == 1)
							EESL_send_event("DEVMAN", RES_COMPLETE_EVENT, (unsigned char*) &event, sizeof(short));

					}
				}
				else if (evtID == 10001)
				{
					int i,j, k;
					char temp[300];

					LOG_PRINTF(("VMAC IRIS PAN RANGE using %s",GetVMACLibVersion()));
					sprintf(temp, "%*s", dataSize, data2);
					temp[dataSize] = '\0';
					LOG_PRINTF(("VMAC IRIS PAN REGISTRATION: %s", temp));

					// Store the logical name
					for (i = 0; i < dataSize && data2[i] && data2[i] != ','; i++)
						temp[i] = data2[i];
					temp[i++] = '\0';

					// Process the data
					for(;i < dataSize;)
					{
						// Find an empty pan range slot. Stop if none found
						for (j = 0; j < maxRedirectPAN; j++)
						{
							if (redirectPAN[j].logicalName[0] == '\0')
								break;
						}
						if (j == maxRedirectPAN)
						{
							maxRedirectPAN++;
							if (redirectPAN == NULL)
								redirectPAN = my_malloc(sizeof(typeRedirectPAN));
							else
								redirectPAN = my_realloc(redirectPAN, maxRedirectPAN * sizeof(typeRedirectPAN));
						}
						strcpy(redirectPAN[j].logicalName, temp);
						LOG_PRINTF(("VMAC IRIS PAN STORED: %d", j));

						// Now store the low pan range
						for (k = 0; k < 20 && data2[i] != '-' && i < dataSize; k++, i++)
							redirectPAN[j].lowPAN[k]  = data2[i];
						redirectPAN[j].lowPAN[k] = '\0';
						LOG_PRINTF(("VMAC IRIS LOW PAN RANGE (%d): %s", j, redirectPAN[j].lowPAN));
						if (data2[i] == '-') i++;
						if (i == dataSize) break;

						// Now store the high pan range
						for (k = 0; k < 20 && data2[i] != ',' && i < dataSize; k++, i++)
							redirectPAN[j].highPAN[k]  = data2[i];
						redirectPAN[j].highPAN[k] = '\0';
						LOG_PRINTF(("VMAC IRIS HIGH PAN RANGE (%d): %s", j, redirectPAN[j].highPAN));
						if (data2[i] == ',') i++;
					}
				}
				else if (evtID == 10007)
				{
					int i,j;
					char temp[300];

					LOG_PRINTF(("VMAC IRIS PAN RANGE DELETION using %s",GetVMACLibVersion()));
					sprintf(temp, "%*s", dataSize, data2);
					temp[dataSize] = '\0';
					LOG_PRINTF(("VMAC IRIS PAN DELETION: %s", temp));

					// Store the logical name
					for (i = 0; i < dataSize && data2[i] && data2[i] != ','; i++)
						temp[i] = data2[i];
					temp[i++] = '\0';

					// Now erase any existing pan ranges for this logical name
					for (j = 0; j < maxRedirectPAN; j++)
					{
						if (strcmp(redirectPAN[j].logicalName, temp) == 0)
						{
							redirectPAN[j].logicalName[0] = '\0';
							LOG_PRINTF(("VMAC IRIS PAN CLEANUP: %d", j));
						}
					}
				}
				else if (evtID == 15001)
				{
					LOG_PRINTF(("VMAC IRIS ACTIVATE using %s", GetVMACLibVersion()));
					InpTurnOn();
					active = 1;
					status = 1;
				}
				else if (evtID == 10003 )
				{
					LOG_PRINTF(("VMAC IRIS MCR ACK [%d] using %s",data2[0],GetVMACLibVersion()));
				}
				else if (evtID == 10005)
				{
					char message[100] = "";
					const char *objname = "iPAY_CFG";
					char *jsondata=NULL;
					char *jsonvalue=NULL;
					unsigned int objlength = 0;

					LOG_PRINTF(("VMAC IRIS TID/MID REQUEST using %s", GetVMACLibVersion()));


					// Get the TID
					jsondata = (char*)IRIS_GetObjectData( objname, &objlength);
        			if(jsondata) jsonvalue = (char*)IRIS_GetObjectTagValue( (const char *)jsondata, "TID" );
					strcat(message, jsonvalue?jsonvalue:"");
					strcat(message, ",");
          			UtilStrDup(&jsonvalue , NULL);

					// Get the MID
        			if(jsondata) jsonvalue = (char*)IRIS_GetObjectTagValue( (const char *)jsondata, "MID" );
					strcat(message, jsonvalue?jsonvalue:"");
          			UtilStrDup(&jsonvalue , NULL);

					LOG_PRINTF(("Sending event 10006 to %s with DATA=\"%s\"", senderName, message));
					EESL_send_event(senderName, 10006, (unsigned char *) message, strlen(message));
				}
			} while (evtID != 0);
		}
	} while (active == -1);

	return status;
}
#endif


int flush_touch(void)
{
	int penDown = 1,touch_x = 0, touch_y = 0;
	while(penDown){
		penDown = get_touchscreen (&touch_x, &touch_y);
	}
	return(0);
}


int ctlsCall( int acc, long amt, int nosaf, char *tr1,char *tr2,char *tlvs,char *emvres)
{
	int ret;
	int i;
	int rc = 0;
	_ctlsStru ctlsStru;
	
	set_display_color(CURRENT_PALETTE,1);
	set_display_color(BACKGROUND_COLOR,65535);

	memset( &ctlsStru,0,sizeof(ctlsStru));
	sprintf(ctlsStru.sAmt,"%012d",amt);
	ctlsStru.nosaf = nosaf;
	 
	if(EmvIsCardPresent()) ret = -1002;// chip card inserted
	else ret = AcquireCard('1', 50000,&ctlsStru);

	if(ret>=0) {
		ProcessCard();
	}
	
	if(ctlsStru.nTLvLen > 5)
	{
		memcpy(tlvs,ctlsStru.sTLvData,ctlsStru.nTLvLen);
	}

	{
		LOG_PRINTFF(0x00000001L,"sTrackTwo" );
		strcpy(tr1,ctlsStru.sTrackOne);
		strcpy(tr2,ctlsStru.sTrackTwo);
	}
	
	if(emvres) {
		if(ret<0) sprintf(emvres,"%d", ret);
		else strcpy(emvres,ctlsStru.TxnStatus);
	}
	
	CancelAcquireCard(0);
	ctlsReset();

	SVC_WAIT(200);
		
	return(0);
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : ()LOW_POWER
**
** DESCRIPTION: Turns the screen saver ON and puts the device in low power mode
**
** PARAMETERS:	None
**
** RETURNS:		None
**
**-----------------------------------------------------------------------------
*/
void __lowPower(int event,int tcpcheck, long timeout, long timeout_fail,long shutdowntm)
{
	int keyCode = 0;
	long wait=0;
	int timerID = 0;
	char *retmsg;
	long tm = timeout;
	int evt = 0;
	int tcpchk = 1;
	int interval = 1000;
	long long tm_total = 0;

//	timeout : 20 mins

	GetSetTcpChkFlg(1, &tcpchk);
	set_backlight(0);
	if(event) {
		while(1) {
			if (timerID >= 0) clr_timer(timerID);
			timerID = set_timer(20*6000,EVT_TIMER);
			evt = wait_evt(EVT_KBD|EVT_TIMER|EVT_PIPE);
			if(evt & EVT_PIPE) inManageCEEvents();
			if(evt & EVT_KBD) break;
			if(evt & EVT_TIMER) {
				__ip_connect_check("10",&retmsg);
			}
		}
	} else {
		while(1) {
			int	evt = wait_evt(EVT_PIPE|EVT_KBD);
			if(evt & EVT_PIPE) { inManageCEEvents(); continue; }
			if(evt & EVT_KBD) break;

			if(read(conHandle, (char *) &keyCode, 1) >0) break;
			SVC_WAIT(interval);
			if(shutdowntm) {
				tm_total += interval;
				//if(tm_total > 1000 * 60 * 60 * 5 ) SVC_SHUTDOWN()	;
				if(tm_total > shutdowntm ) SVC_SHUTDOWN()	;
			}	
			/*  
			if (timerID >= 0) clr_timer(timerID);
			timerID = set_timer(60000,EVT_TIMER);
			evt = wait_evt(EVT_KBD|EVT_TIMER);
			if(evt & EVT_KBD) break;
			if(evt & EVT_TIMER && tm && tcpcheck) {
			*/

			if(tm && tcpcheck) {
				wait += interval;
				if(wait>=tm && tcpcheck) {
					char stmp[64];
					char stmp2[64];

					__ip_connect_check("10",&retmsg);
					if(strcmp(retmsg,"NOERROR")!=0) {
						SVC_WAIT(1000);
						set_backlight(1);
						sprintf(stmp,"LARGE,THIS,CONNECTION NOT OK,2,C;");
						sprintf(stmp2,"LARGE,THIS,%s,3,C;",retmsg);
						strcat(stmp,"LARGE,THIS,WILL RECHECK,4,C;");
						DisplayObject(stmp,0,EVT_TIMEOUT,2000,NULL,NULL);
						tm = timeout_fail;
					} else {
						tm = timeout;
					}
					set_backlight(0);
					wait = 0;
				}
			}
		}
	}
	set_backlight(1);
	if (timerID >= 0) clr_timer(timerID);
	tcpchk = 0;
	GetSetTcpChkFlg(1, &tcpchk);
}

int GetSetTcpChkFlg(int mode, int* flag)
{
	static int connflag = 0;

	if(mode ==0) {
		//get
		*flag = connflag;
	} else if(mode ==1) {
		//set
		connflag = *flag;
	}
	return(0);
}
