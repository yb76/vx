

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <svc.h>
#include <svc_sec.h>

// for EOS log
#include <eoslog.h>

#include "auris.h"
#include "alloc.h"
#include "display.h"
#include "input.h"
#include "printer.h"

#include "as2805.h"
#include "security.h"
#include "utility.h"
#include "iris.h"
#include "irisfunc.h"

#ifdef __EMV
#include "emv.h"
#endif 

#include "comms.h"


int conHandle = -1;

T_MenuTch MenuTch;

// We must define these as they are expected by iRIS to define the scope of object and string resolution
char * currentObject = NULL;
char * currentObjectData = NULL;
char * currentObjectGroup = NULL;
char * nextObject = NULL;

char * getLastKeyDesc(unsigned char key, T_KEYBITMAP * keyBitmap)
{
	int i;

	static struct
	{
		uchar key;
		T_KEYBITMAP keyBitmap;
		char * event;
	} key_string[] =
	{
	{KEY_ALL,		KEY_ALL_BITS,			"KEY_ALL"},
	{KEY_NUM,		KEY_NUM_BITS,			"KEY_NUM"},
	{KEY_NO_PIN,	KEY_NO_PIN_BIT,			"KEY_NO_PIN"},
	{KEY_0,			KEY_0_BIT,				"KEY_0"},
	{KEY_1,			KEY_1_BIT,				"KEY_1"},
	{KEY_2,			KEY_2_BIT,				"KEY_2"},
	{KEY_3,			KEY_3_BIT,				"KEY_3"},
	{KEY_4,			KEY_4_BIT,				"KEY_4"},
	{KEY_5,			KEY_5_BIT,				"KEY_5"},
	{KEY_6,			KEY_6_BIT,				"KEY_6"},
	{KEY_7,			KEY_7_BIT,				"KEY_7"},
	{KEY_8,			KEY_8_BIT,				"KEY_8"},
	{KEY_9,			KEY_9_BIT,				"KEY_9"},
	{KEY_SK1,		KEY_SK1_BIT,			"KEY_SK1"},
	{KEY_SK2,		KEY_SK2_BIT,			"KEY_SK2"},
	{KEY_SK3,		KEY_SK3_BIT,			"KEY_SK3"},
	{KEY_SK4,		KEY_SK4_BIT,			"KEY_SK4"},
	{KEY_F0,		KEY_F0_BIT,				"KEY_F0"},
	{KEY_F1,		KEY_F1_BIT,				"KEY_F1"},
	{KEY_F2,		KEY_F2_BIT,				"KEY_F2"},
	{KEY_F3,		KEY_F3_BIT,				"KEY_F3"},
	{KEY_F4,		KEY_F4_BIT,				"KEY_F4"},
	{KEY_F5,		KEY_F5_BIT,				"KEY_F5"},
	{KEY_CNCL,		KEY_CNCL_BIT,			"CANCEL"},
	{KEY_CLR,		KEY_CLR_BIT,			"KEY_CLR"},
	{KEY_LCLR,		KEY_LCLR_BIT,			"KEY_LCLR"},
	{KEY_OK,		KEY_OK_BIT,				"KEY_OK"},
	{KEY_ASTERISK,	KEY_ASTERISK_BIT,		"KEY_ASTERISK"},
	{KEY_FUNC,		KEY_FUNC_BIT,			"KEY_FUNC"},
	{KEY_NONE,		0,						NULL}
	};

	// Convert the key pressed to a bitmap
	for (i = 0; key_string[i].event; i++)
	{
		if (key == key_string[i].key)
		{
			if (keyBitmap)
				*keyBitmap = key_string[i].keyBitmap;
			break;
		}
	}

	return key_string[i].event;
}

static bool getEvent(T_EVTBITMAP setEvtBitmap, T_KEYBITMAP keepKeyBitmap, T_INP_ENTRY inpEntry, ulong timeout, bool ignoreTimeout, bool largeFont, bool * flush,T_EVTBITMAP *pevtBitmap,T_KEYBITMAP *pkeyBitmap,uchar *pkey, char** pevent)
{
	int i;
	bool keyPress = false;
	char sPrntBuff[50];
	int nCount = 0;
	//Dwarika.. for touch screen
		char sTmpBuff[100];
		int nloopct = 0;
	//Dwarika.. for touch screen

	static struct
	{
		T_EVTBITMAP evtBitmap;
		char * event;
	} event_string[] = 
	{
	{EVT_TIMEOUT,		"TIME"},
	{EVT_MCR,			"MCR"},
	{EVT_SERIAL_DATA,	"SER_DATA"},
	{EVT_SERIAL2_DATA,	"SER2_DATA"},
	{EVT_SCT_IN,		"CHIP_CARD_IN"},
	{EVT_SCT_OUT,		"CHIP_CARD_OUT"},
	{EVT_CTLS_CARD,		"CTLS_CARD"},
	{EVT_TOUCH,			"TOUCH_EVT"},		
	{EVT_NONE,			NULL}
	};

	// Wait for an event to occur
	*pkeyBitmap = KEY_NO_BITS;
	*pevtBitmap = setEvtBitmap;
	*pkey = InpGetKeyEvent(keepKeyBitmap, pevtBitmap, inpEntry, timeout , largeFont, *flush, &keyPress);
	*flush = false;

/*Dwarika .. Touch Handle*/
	if(*pkey == KEY_NONE)
	{
			if(*pevtBitmap == EVT_TOUCH)
			{
				*pevent = (char *)MenuTch.buttons[MenuTch.pressed].sEvtCode;
				return (true);
			}
		

		if (ignoreTimeout && *pevtBitmap == EVT_TIMEOUT) return false;
		else		
		{
			for (i = 0; event_string[i].event; i++)
			{
				if (*pevtBitmap == event_string[i].evtBitmap)
				{
					*pevent = event_string[i].event;

					if(event_string[i].evtBitmap == EVT_CTLS_CARD)
					{
					}

					
					break;
				}
			}
		}
	}
	else
	{

			if(*pkey!=KEY_NONE) *pevent = getLastKeyDesc(*pkey, pkeyBitmap);
			else {
				for (i = 0; event_string[i].event; i++)
				{
					if (*pevtBitmap == event_string[i].evtBitmap)
					{
							
						*pevent = event_string[i].event;

						if(event_string[i].evtBitmap == EVT_CTLS_CARD)
						{
						}

						break;
					}
				}
			}
	}
	/*Dwarika .. Touch Handle*/

#if 0	
	// Do not generate a timeout event if we are ignoring timeouts...This is if we are updating elements within the screen
	//if (ignoreTimeout && *pkey == KEY_NONE && *pevtBitmap == EVT_TIMEOUT) return false;

	if(*pkey!=KEY_NONE) *pevent = getLastKeyDesc(*pkey, pkeyBitmap);
	else {
		for (i = 0; event_string[i].event; i++)
		{
			if (*pevtBitmap == event_string[i].evtBitmap)
			{
				*pevent = event_string[i].event;
				break;
			}
		}
	}

	#endif

	// Check if an event has occurred
	return (true);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : processDisplayObject
//
// DESCRIPTION:	Obtains the current object data:
//					If not found, it returns with a failure
//					If found, it authenticates the object using the signing key.
//						If authentication succeeds, it processes the object and returns with a success.
//						If authentication fails, it displays an auth error and awaits a CANCEL
//							 or CLR key and returns with a failure.
//
// PARAMETERS:	objectName	<=	The name of the object to retrieve
//
// RETURNS:
//-------------------------------------------------------------------------------------------
//
static void processDisplayObject()
{
	lua_main(currentObject);
}

static void processObjectLoop()
{
	
	while(1)
	{
		// Process the current object
		processDisplayObject();
		if (nextObject) UtilStrDup(&currentObject, nextObject);
	}
}


//
//-------------------------------------------------------------------------------------------
// FUNCTION   : Main function
//
// DESCRIPTION:	Sets the current object to boot and then let's it all roll!!!
//
//				It also has a list of internal objects to use if they are not externally overriden
//				relating to booting, configuring and contacting the iRIS host.
//
//				The external signed object takes precedence first, then the internal objects. If
//				none are found. The terminal will be get out and back to the previous object. The 
//				user can then select a different action or try again.
//
// PARAMETERS:	None
//
// RETURNS:		Track 1 string if available or an empty string
//-------------------------------------------------------------------------------------------
//
main(int argc, char * argv[])
{
	SecurityInit();
	__pstn_init();
	__ser_init();
	UtilStrDup(&currentObjectGroup, irisGroup);

	// Set the initial object to BOOT which is the boot loader
	UtilStrDup(&currentObject, "BOOT.lua");
	UtilStrDup(&nextObject, "BOOT.lua");


#ifdef __EMV
	EMVPowerOn();
#endif
	processObjectLoop();
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : DisplayObject
//
// DESCRIPTION:	Rolls through the display object
//
// PARAMETERS:	objectName	<=	The name of the object to retrieve
//
// RETURNS:		Track 1 string if available or an empty string
//-------------------------------------------------------------------------------------------
//
int DisplayObject(const char *lines,T_KEYBITMAP keyBitmap,T_EVTBITMAP keepEvtBitmap,int timeout, char* outevent, char* outinput)
{
	T_INP_ENTRY inpEntry;
	bool inpLargeFont = false;

	char * event=NULL;
	bool firstRound = true;
	bool eventOccurred = false;
	bool flush = true;
	int item_num = 0;
	int i=0,j=0;
	char liness[1024];
	char *str = liness;
	//char *scrfield[40][9];
	char *scrfield[40][10];
	int color_background = 40575; //153,204,255
	int color_darkblue = 4523;
	int color_button = 10998;//45,93,181

	//Dwarika.. for touch screen
		char sPrntBuff[50];
	char fEndcCol[40];
	char sBMPFileNm[20];
	char sBMPEVT[20];
	char sDispBMPFile[30];
	int CTLS_Handle = -1;
	int lastrow = 0;

	int ret;
	int nWideLbl = 0;
	int nCoord;
	//Dwarika.. for touch screen

	// Initialisation
	memset(&inpEntry, 0, sizeof(inpEntry));
	memset(scrfield,0,sizeof(scrfield));

	memset(&MenuTch,0x00,sizeof(MenuTch));
	memset(fEndcCol,0x00,sizeof(fEndcCol));

	//Dwarika .. Vx680 ..Change the coordinate mode
	if(get_display_coordinate_mode() == CHARACTER_MODE)
		set_display_coordinate_mode(PIXEL_MODE);

	write_pixels (0, 0, 240, 320, color_background );

	set_display_color(CURRENT_PALETTE,1);
	set_display_color(BACKGROUND_COLOR,color_background );//61695  255
	set_display_color(FOREGROUND_COLOR,65535);//61695  255


 


	strcpy(liness,lines);
	while(*str) {
	    char *pstart = str;
		char endc;

		while(*str && *str!=','&& *str!= ';') ++str;
		scrfield[i][j] = pstart;
		endc = *str;
		if(endc == ';'||endc == '\0' ) 
		{
			if(j != 9)
				fEndcCol[i] = '1';
			else
				fEndcCol[i] = '0';

			 i++;j=0;
		  if(endc == '\0') break;
		} else if(endc == ',')  
			{
			//Dwarika .. for Vx 680
			//if(j==8){ i++;j=0;}
			if(j==9)
			{ 
				fEndcCol[i] = '0';
				i++;j=0;
			}
			else j++;
		}
		*str = 0;
		++str;
	}
	item_num = i;
	//DispClearScreen();

	// Keep looping waiting for events to occur
	for(; eventOccurred == false ; firstRound = false)
	{
		for(i = 0; firstRound && i<item_num; i++)
		{
			int index;
			int	operation = 0;
			bool inverse = false;
			bool largeFont = false;
			char * search = NULL;
			char * referenceName = NULL;
			char * tableresult = NULL;
			int row = 0, col = 0;
			int inputMaxLength = 0;
			char * value;
			bool skipDisplay = false;
			bool hidden = false;
			bool button = false;
			bool b_hidden = false;

			nWideLbl = 0;
			memset(sBMPEVT,0x00,sizeof(sBMPEVT));


			for(index = 0; index<=9; index++)
			{
				value = scrfield[i][index];
				if(value && *value==0) value=NULL;

				switch(index)
				{
					// Pick up the type of the animation GRAPH or TEXT
					case 0:
						if (value)
						{
							if (strcmp(value, "GRAPH") == 0)
								operation = 1;
							else if (strcmp(value, "TEXT") == 0)
								operation = 0;
							else if (strcmp(value, "INV") == 0)
								inverse = true;
							else if (strcmp(value, "LARGE") == 0)
								largeFont = true;
							else if (strcmp(value, "WIDELBL") == 0) {
								//largeFont = true;
								nWideLbl = 1;
							}
							else if (strncmp(value, "BUTTON",6) == 0) {
								//if(strncmp(value,"BUTTONL",7)==0) largeFont = true;
								button = true;
								strcpy(sBMPEVT, value);
							}
							else if (strcmp(value, "BATTERY") == 0)
								operation = 2;
							else if (strcmp(value, "SIGNAL") == 0)
								operation = 3;
							else if (strcmp(value, "TIMEDISP") == 0)
								operation = 4;
							else if (strcmp(value, "BITMAP") == 0) {
								operation = 5;
							}
							else if (strcmp(value, "LINV") == 0)
							{
								largeFont = true;
								inverse = true;
							}
							else if (strcmp(value, "AMOUNT") == 0)
							{
								inpEntry.type = E_INP_AMOUNT_ENTRY;
								inputMaxLength = 21;
							}
							else if (strcmp(value, "PERCENT") == 0)
							{
								inpEntry.type = E_INP_PERCENT_ENTRY;
								inputMaxLength = 21;
							}
							else if (strcmp(value, "DECIMAL") == 0)
							{
								inpEntry.type = E_INP_DECIMAL_ENTRY;
								inputMaxLength = 21;
							}
							else if (strcmp(value, "DATE") == 0)
							{
								inpEntry.type = E_INP_DATE_ENTRY;
								inputMaxLength = 21;
							}
							else if (strcmp(value, "MMYY") == 0)
							{
								inpEntry.type = E_INP_MMYY_ENTRY;
								inputMaxLength = 21;
							}
							else if (strcmp(value, "TIME") == 0)
							{
								inpEntry.type = E_INP_TIME_ENTRY;
								inputMaxLength = 21;
							}
							else if (strcmp(value, "NUMBER") == 0 || strcmp(value, "HIDDEN") == 0)
							{
								if (strcmp(value, "HIDDEN") == 0) hidden = true;
								inpEntry.type = E_INP_STRING_ENTRY;
								inputMaxLength = 21;
							}
							else if (strcmp(value, "STRING") == 0)
							{
								inpEntry.type = E_INP_STRING_ALPHANUMERIC_ENTRY;
								inputMaxLength = 21;
							}
							else if (strcmp(value, "HEX") == 0)
							{
								inpEntry.type = E_INP_STRING_HEX_ENTRY;
								inputMaxLength = 21;
							}
							else if (strcmp(value, "LAMOUNT") == 0)
							{
								inpLargeFont = largeFont = true;
								inpEntry.type = E_INP_AMOUNT_ENTRY;
								inputMaxLength = 16;
							}
							else if (strcmp(value, "LPERCENT") == 0)
							{
								inpLargeFont = largeFont = true;
								inpEntry.type = E_INP_PERCENT_ENTRY;
								inputMaxLength = 16;
							}
							else if (strcmp(value, "LDECIMAL") == 0)
							{
								inpLargeFont = largeFont = true;
								inpEntry.type = E_INP_DECIMAL_ENTRY;
								inputMaxLength = 16;
							}
							else if (strcmp(value, "LDATE") == 0)
							{
								inpLargeFont = largeFont = true;
								inpEntry.type = E_INP_DATE_ENTRY;
								inputMaxLength = 16;
							}
							else if (strcmp(value, "LMMYY") == 0)
							{
								inpLargeFont = largeFont = true;
								inpEntry.type = E_INP_MMYY_ENTRY;
								inputMaxLength = 16;
							}
							else if (strcmp(value, "LTIME") == 0)
							{
								inpLargeFont = largeFont = true;
								inpEntry.type = E_INP_TIME_ENTRY;
								inputMaxLength = 16;
							}
							else if (strcmp(value, "LNUMBER") == 0 || strcmp(value, "LHIDDEN") == 0)
							{
								if (strcmp(value, "LHIDDEN") == 0) hidden = true;
								inpLargeFont = largeFont = true;
								inpEntry.type = E_INP_STRING_ENTRY;
								inputMaxLength = 16;
							}
							else if (strcmp(value, "LSTRING") == 0)
							{
								inpLargeFont = largeFont = true;
								inpEntry.type = E_INP_STRING_ALPHANUMERIC_ENTRY;
								inputMaxLength = 16;
							}
							else if (strcmp(value, "LHEX") == 0)
							{
								inpLargeFont = largeFont = true;
								inpEntry.type = E_INP_STRING_HEX_ENTRY;
								inputMaxLength = 16;
							}
							else if (strcmp(value, "PIN") == 0)
							{
								inpLargeFont = largeFont = true;
								inpEntry.type = E_INP_PIN;
								inputMaxLength = 16;
							}
						}
						break;

					case 1:
						// If this is an input entry, get the default/current value
						if (inputMaxLength)
						{
							if (inpEntry.type == E_INP_AMOUNT_ENTRY || inpEntry.type == E_INP_PERCENT_ENTRY || inpEntry.type == E_INP_DECIMAL_ENTRY)
								InpSetNumber(value?atol(value):0, true);
							else
								InpSetString(value?value:"", (bool) (hidden?true:false), true);
							break;
						}

						// If the table name is not defined, then assign it a default name
						if (!value || value[0] == '\0')
							value = "DEFAULT_T";

						UtilStrDup(&referenceName, value);

						break;
					// Pick up the index within the table to use
					case 2:
						if (inputMaxLength && value && value[0] == '1')
							InpSetOverride(false);
						
						if (value)
							UtilStrDup(&search, value);
						break;
					// Pick up the row. If 255, then centre
					case 3:
						if (value)
						{
							if (value[0] == 's')
							{
								#ifdef __VX670
									largeFont = 0;
									//row = atoi(&value[1]) * 3;
									//Dwarika .. vx680
									row = atoi(&value[1]) + 1 * 40;
								#else
									row = atoi(&value[1]) - 1;
									if (!largeFont)
										row = (atoi(&value[1]) - 1) * 2;
								#endif
							}
							else if (value[0] == 'B')
							{
								#if defined(__VX670) 
								{
									//row = 15;
									//Dwarika .. vx680
									row = 257;
								}
								#else
									row = 7;
								#endif
							}
							else if (value[0] == 'P')
							{
								row = atoi( &value[1]);
							}
							else
							{
								//row = atoi(value) - 1;
								//Dwarika .. Vx680
								row = atoi(value) * 20;

								#if defined(__VX670)
									if (nWideLbl)
									{
										row += 40;
									}
									else
									{
										if (largeFont)
										{
											//row += 2;
											//Dwarika .. Vx680
											row += 30;
										}
										else
										{
											//row += 4;
											//Dwarika .. Vx680
											row += 40;
										}
									}
								#endif
							}
						}
						if (inputMaxLength)
							inpEntry.row = row;
						break;
					// Pick up the column. If 255, then centre
					case 4:
						if (value)
						{
							if (value[0] == 'P')
							{
								col = atoi( &value[1]);
							}
							else if (value[0] == 'C')
								col = 255;
							else if (value[0] == 'R')
								col = 254;
							else
							{
								//col = atoi(value) - 1;
								//Dwarika .. Vx680
								col = (atoi(value) + 1) * 5;
							}
						}

						if (inputMaxLength)
						{
							inpEntry.col = col;
							//Dwarika .. Vx680
							//inpEntry.col = col + 5; // ???
							inpEntry.length = inputMaxLength - col;
						}
						break;
					// Pickup the initial display loop value
					case 5:
						if (value && value[0] != '\0')
						{
							if (inputMaxLength) 
								inpEntry.length = atoi(value);
						}
						break;
					case 6:
						if (value && value[0] != '\0')
						{
							if(button && value[0] == '1') b_hidden = true;
							if (inputMaxLength) 
								inpEntry.minLength = atoi(value);
						}
						break;
					case 7:
						break;
					case 8:
						break;
					case 9:
						break;
					default:
						break;
				}
			}

			// Input definitions do not have any associated displays
			if (inputMaxLength) skipDisplay = true;

			if (skipDisplay == false)
			{
				// Initialisation
				value = NULL;
				// Get the text table or graphics image matching entry
				if (search)
				{
					if (strcmp(referenceName, "THIS") && operation == 0)
					{
						__text_table( search,referenceName,&tableresult);
						value = tableresult;
					}
					else value = search;

				}

				// Display the text
				if (value)
				{
					if (operation == 0)
					{
						{
							int nRetVal,nTmpCol;
							int len=0;
							int button_type = 0;
							int button_w= 0;
							int button_h = 0;
							int pos_x = 0;
							int pos_y = 0;

							if(strlen(value) > 1)
								len = strlen(value);

							// 8 * 16 font size
							if (col == 255)
								nTmpCol = ((largeFont?MAX_COL_LARGE_FONT:MAX_COL) - (len * 8))  / 2;
							else if  (col == 254)
							{
								nTmpCol = (largeFont?MAX_COL_LARGE_FONT:MAX_COL) - (len * 8);
							}
							else
								nTmpCol = col;

							if(button) {
								int adjlen = 0;
								if(  strncmp( sBMPEVT,"BUTTONL" ,7) == 0)	{
									// LARGE button 200*53
									button_type = 1;
									button_h = 53;
									button_w = 200;
									pos_x = 20;
									pos_y = row - 18;
								}else if( strncmp( sBMPEVT,"BUTTONM" ,7) == 0) {
									// MEDIUM button 200*35
									button_type = 2;
									button_h = 35;
									button_w = 200;
									pos_x = 20;
									pos_y = row - 9;
								}else if( strncmp( sBMPEVT,"BUTTONS" ,7) == 0) {
									// SMALL button 70*53
									button_type = 3;
									button_h = 53;
									button_w = 70;
									adjlen = (int)((button_w - len * 8 ) /2.0);
									pos_x = nTmpCol - adjlen;
									pos_y = row - 18;
								}else if( strncmp( sBMPEVT,"BUTTONA" ,7) == 0) {
									// ALPHA button 70*53
									button_type = 4;
									button_h = 53;
									button_w = 70;
									adjlen = (int)((button_w - len * 8 ) /2.0);
									pos_x = nTmpCol - adjlen;
									pos_y = row - 18;
								}
								if(b_hidden) {pos_x = nTmpCol; pos_y = row;}
							}
							

							// touch button
							if((strlen(sBMPEVT) > 4) && MenuTch.cnt < 10)
							{
								sprintf(MenuTch.buttons[MenuTch.cnt].sEvtCode,"%s",sBMPEVT);
								if( button_type == 4) MenuTch.buttons[MenuTch.cnt].isAlpha = true;
								MenuTch.buttons[MenuTch.cnt].x1 = pos_x;
								MenuTch.buttons[MenuTch.cnt].y1 = pos_y;
								MenuTch.buttons[MenuTch.cnt].x2 = pos_x + button_w;
								MenuTch.buttons[MenuTch.cnt].y2 = pos_y + button_h;

								MenuTch.cnt++;

							}

							//nRetVal = put_BMP_at(30,row-10,sDispBMPFile);
							if(nWideLbl)
							{
								int toprow = row -12;
								if (toprow < lastrow) toprow = lastrow;
								write_pixels (1, toprow, 238, row+28, color_darkblue);
								lastrow = row + 28;
							}
							else if(button && !b_hidden)
							{
							 	char btnfile[30];
								sprintf( btnfile, "f:btn%d.bmp",button_type);
								put_BMP_at(pos_x,pos_y,btnfile);
							}

							set_display_color(CURRENT_PALETTE,1);
							if(button) set_display_color(BACKGROUND_COLOR,color_button);
							else if(nWideLbl) set_display_color(BACKGROUND_COLOR,color_darkblue);//61695  255
							else set_display_color(BACKGROUND_COLOR,color_background);

							if(nWideLbl || button) set_display_color(FOREGROUND_COLOR,65535);//white
							else set_display_color(FOREGROUND_COLOR,color_darkblue);//dark blue

						}
						
						memset(sDispBMPFile,0x00,sizeof(sDispBMPFile));	

						DispText(value, row, col, false /* Clear Line */, largeFont, inverse);
					}
					else if (operation == 2)
					{
#ifdef __VX670
						set_display_color(BACKGROUND_COLOR,color_background);
						set_display_color(FOREGROUND_COLOR,color_darkblue);
						DispUpdateBattery(row, col);
#endif
					}
					else if (operation == 3)
					{
#ifdef __VX670
						DispSignal(row, col);
#endif
					}
					else if (operation == 4)
					{
						char timestr[30] = "";
						__time_real( "DD/MM/YY hh:mm", timestr);
						if(timestr)
						{
							memset(sDispBMPFile,0x00,sizeof(sDispBMPFile));	

							set_display_color(CURRENT_PALETTE,1);
							set_display_color(BACKGROUND_COLOR,color_background );
							set_display_color(FOREGROUND_COLOR,color_darkblue);
							//DispText(timestr, row, col, false /* Clear Line */, false/*large*/, false /*inverse*/);
							DispText(timestr, row, col, false /* Clear Line */, false/*large*/, false /*inverse*/);
						}
					}
					else if (operation == 1)
					{
						unsigned int length;
						char * objectData;
						static unsigned char * imageb = NULL;
						static char bmpname[50] = "";
						
						if( imageb && ( strcmp(bmpname,value) == 0 )) {
							DispGraphics((const char *)imageb, row, col);
						} else {
							strcpy(bmpname,value);
							// Obtain the graphics object locally or go to the host if not available
							objectData = IRIS_GetObjectData(value, &length);

							if (objectData)
							{
								char *type= IRIS_GetStringValue(objectData, length, "TYPE" );

								if (type)
								{
									if (strcmp(type, "IMAGE") == 0)
									{
										char * image = IRIS_GetStringValue(objectData, length, "IMAGE");

										if (image)
										{
											if(imageb) my_free(imageb);
											imageb = my_malloc(strlen(image) / 2 + 1);
											UtilStringToHex(image, strlen(image), imageb);
											DispGraphics((const char *)imageb, row, col);

											IRIS_DeallocateStringValue(image);
										}
									}

									IRIS_DeallocateStringValue(type);
								}

								my_free(objectData);
							}
						}
					}
					else if (operation == 5)
					{
						char bmpfile[30];
						bool charmode = false;
						if(get_display_coordinate_mode() == CHARACTER_MODE) {
							charmode = true;
							set_display_coordinate_mode(PIXEL_MODE);
						}
						sprintf( bmpfile, "f:%s",value);
						put_BMP_at(col,row,bmpfile);
						if(charmode) set_display_coordinate_mode(CHARACTER_MODE);
					}
					
				}
				else
				{
								
				}
			}
			else
			{
					set_display_color(CURRENT_PALETTE,1);
					set_display_color(BACKGROUND_COLOR,color_background);
					set_display_color(FOREGROUND_COLOR,color_darkblue);
			}
			// my_free allocated data
			UtilStrDup(&search, NULL);
			UtilStrDup(&referenceName, NULL);
			UtilStrDup(&tableresult, NULL);
		}

		// Display timeout procesing
		if (eventOccurred == false)
		{
			uchar key;
			T_KEYBITMAP newKeyBitmap;
			T_EVTBITMAP newEvtBitmap = EVT_NONE;
			set_display_color(CURRENT_PALETTE,1);
			set_display_color(BACKGROUND_COLOR,color_background );
			set_display_color(FOREGROUND_COLOR,color_darkblue);
			eventOccurred = getEvent(keepEvtBitmap,keyBitmap,inpEntry,timeout,false,inpLargeFont,&flush,&newEvtBitmap,&newKeyBitmap,&key,&event );
		}
	}

	if(event) {
		if(outinput) strcpy( outinput,InpGetString());
		if(outevent) strcpy( outevent, event );
	}

	return(0);

}

int set_nextObj( const char* nextObj)
{
	UtilStrDup(&nextObject, nextObj);
	return(0);
}

char * currentScreen( int getset, char* scrname)
{
	static char sname[64]="";

	if(scrname == NULL)	return (NULL);

	if(getset==0) // get
		strcpy(scrname, sname);
	else if(getset==1) //set
		strcpy(sname, scrname);

	return(scrname);
}

int inGetApplVer(char* ver)
{
 	char sApplVer[10]="1.0";
	
	memset(sApplVer,0x00,sizeof(sApplVer));
	get_env("APPLVER",sApplVer,6);
	strcpy(ver,sApplVer);
	return(0);
 }
