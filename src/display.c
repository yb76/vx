/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       display.c
**
** DATE CREATED:    10 July 2007
**
** AUTHOR:          Tareq Hafez
**
** DESCRIPTION:     This module contains the display functions
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
#include <stdarg.h>
#include <string.h>

/*
** KEF Project include files.
*/
#include <svc.h>

/*
** Local include files
*/
#include "auris.h"
#include "comms.h"
#include "display.h"
#include "Input.h"
#include "printer.h"

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
extern int conHandle;

/*
**-----------------------------------------------------------------------------
** FUNCTION   : DispInit
**
** DESCRIPTION: Initialise the display module
**
** PARAMETERS:	None
**
** RETURNS:		None
**
**-----------------------------------------------------------------------------
*/
void DispInit(void)
{
	if (conHandle == -1)
		conHandle = open(DEV_CONSOLE, 0);
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : DispClearScreen
**
** DESCRIPTION: Initialise the display module
**
** PARAMETERS:	None
**
** RETURNS:		None
**
**-----------------------------------------------------------------------------
*/
void DispClearScreen(void)
{
	// Initialisation
	DispInit();

	clrscr();

}

int parse_display_line(const char *intext, char *newline, int *color )
{
	//eg. "<color_red>EFG"
	int i = 0;
	int j = 0;
	char text[256];
	char *p= text;
	char *q = NULL;
	char curr_t[64];
	char *color_tag = "<color_";
	int skip = 0;
	unsigned int color_red = 0xf800; //255,0,0
	unsigned int color_yellow = 0xffe0; //255,255,0
	unsigned int color_green = 2016; //0,255,0

	*color = 0;
	strcpy(text,intext);
	strcpy(newline,intext);

	p = strstr(text,color_tag) ;
	if(p!=NULL) q = strstr(p+1,">");
	if(p ==NULL || q==NULL) return 0;

	p = p+strlen(color_tag);

	if(strncmp(p,"red",3)==0) {
		*color = color_red;
	}else	if(strncmp(p,"yellow",6)==0) {
		*color = color_yellow;
	}else	if(strncmp(p,"green",3)==0) {
		*color = color_green;
	}else	if(*p >='0' && *p<='9') {
		*q = 0;
		*color = atoi(p);
	}

	p = q+1;
	strcpy(newline,p);
	return(1);
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : DispText
**
** DESCRIPTION: Write the text on the LCD screen,
**
** PARAMETERS:	text		<=	Text to write
				row			<=	Row to display it at
**				col			<=	Column to display it at
**				largeFont	<=	Font size: Large or small
**				inverse		<=	Indicates if reverse video is required
**
** RETURNS:		None
**
**-----------------------------------------------------------------------------
*/
void DispText(const char * intext, uint row, uint col, bool clearLine, bool largeFont, bool inverse)
{
	const char * emptyLine = "                     ";
	int len = 0;
    long color,old_color;
    char text[256];

	if(intext == NULL) return;


	// Initialisation
	DispInit();
	parse_display_line(intext, text, &color );
	len = strlen(text);

	// If centre requested
	//if (col == 255) col = ((largeFont?MAX_COL_LARGE_FONT:MAX_COL) - (len * 7)) / 2;
	if (col == 255) {
		col = (MAX_COL - (len * 8)) / 2;
	}

	// If right justificatino requested
	else if  (col == 254)
	{
		//col = (largeFont?MAX_COL_LARGE_FONT:MAX_COL) - (len * 7);
		col = MAX_COL - (len * 8);
	}

	if(color) {
		old_color = get_display_color(FOREGROUND_COLOR);
		if(color!= old_color) set_display_color(FOREGROUND_COLOR,color);
	}

	if (!largeFont)
	{
		inverse?setfont("f:asc8x21i.vft"):setfont("");
		if (row == 9999)
			write(conHandle, text, strlen(text));
		else
		{
			if (clearLine) write_at(emptyLine, 21, 1, row+1);
			write_at(text, strlen(text), col+1, row+1);
		}
	}
	else
	{
		inverse?setfont("f:ir8x16i.vft"):setfont("f:ir8x16.vft");
		if (row == 9999)
			write(conHandle, text, strlen(text));
		else
		{
			if (clearLine) write_at(emptyLine, 16, 1, row+1);
			write_at(text, strlen(text), col+1, row+1);
		}
	}

	if(color!= old_color) set_display_color(FOREGROUND_COLOR,old_color);

}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : DispWriteGraphics
**
** DESCRIPTION: Write the text on the LCD screen,
**
** PARAMETERS:	text		<=	Text to write
				row			<=	Row to display it at
**				col			<=	Column to display it at
**				largeFont	<=	Font size: Large or small
**				inverse		<=	Indicates if reverse video is required
**
** RETURNS:		None
**
**-----------------------------------------------------------------------------
*/
void DispGraphics(const char * graphics, uint row, uint col)
{
	uchar bitmap;
	int i, j, k, m;
	int width = graphics[1]/8;
	int widthchar = graphics[1]/6;
	int height = graphics[3]/8;

	// Initialisation
	DispInit();

	// Set font to 6x8 to achieve the best resolution. The other 2 are 8x16 or 16x16 which will
	// give us a bad column resolution
	setfont("");

	// Do the conversion and output. It must be multiples of 8 pixels in both directions unfortunately
	// Paint as many rows as requested
	for (m = 0; m < height; m++)
	{
		char data[126];

		gotoxy((21 - widthchar)/2+1, row+1+m);
		memset(data, 0, sizeof(data));

		for (k = 0; k < width; k++)
		{
			for (bitmap = 0x80, j = 0; j < 8 && (j+k*8) < sizeof(data); j++, bitmap >>= 1)
			{
				for (i = 0; i < 8; i++)
				{
					if (graphics[4 + i*width + k + m*width*8] & bitmap)
						data[j + k*8] += (1 << i);
				}
			}
		}
		putpixelcol(data, k*8/6*6);
	}
}

/* DispArray  */
int	DispArray(int timeout,char **pcMenuItems, int iMenuItemsTotal)
{
	char scrlines[1024]="";
	char stmp[64]="";
	char linestr[64]="";
	char outevent[30];
	int line_max=4;
	int i=0,j=0;
	long keys=KEY_1_BIT|KEY_2_BIT|KEY_3_BIT|KEY_4_BIT|KEY_5_BIT|KEY_6_BIT;
	int selected = 0;


	line_max=6;
	keys = keys|KEY_CNCL_BIT|KEY_OK_BIT|KEY_CLR_BIT;
	while(1) {
		strcpy(scrlines,"WIDELBL,THIS,PLEASE SELECT,-1,C;");
		for(i=0;i<line_max;i++) {
			strcpy(stmp," ");
			if( i+j<iMenuItemsTotal && pcMenuItems[i+j] && strlen( pcMenuItems[i+j]) ) {
			  strcpy(stmp,pcMenuItems[i+j]);
		      sprintf( linestr,",THIS,%d>  %s,%i,5;",i+1,stmp,(i+1)*2);
			}
			strcat(scrlines,linestr);
		}
		DisplayObject(scrlines,keys,timeout?EVT_TIMEOUT:0,timeout,outevent,stmp);
		if(!strcmp(outevent, "CANCEL") || !strcmp(outevent,"TIME")) break;
		else if(!strcmp(outevent, "KEY_OK")){
			if(j+line_max<iMenuItemsTotal) j+=line_max;
			else InpBeep(2, 20, 5);
		}	
		else if(!strcmp(outevent, "KEY_CLR")){
			if(j>=line_max) j-=line_max;
			else InpBeep(2, 20, 5);
		}
		else if(!strncmp(outevent,"KEY_1",5)||!strncmp(outevent,"KEY_2",5)||!strncmp(outevent,"KEY_3",5)||!strncmp(outevent,"KEY_4",5)||
					!strncmp(outevent,"KEY_5",5)||!strncmp(outevent,"KEY_6",5)) {
			int func = outevent[4]-'1';
			if(j+func<iMenuItemsTotal && pcMenuItems[j+func]) {
				selected = j+func+1;
				break;
			}else InpBeep(2, 20, 5);
		}
	}
	if(selected > 0) return(selected);
	else if(!strcmp(outevent,"TIME")) {
		long iret = - EVT_TIMEOUT;
		return(iret);
	} else
		return(0);
}

#ifdef __VX670
/*
**-----------------------------------------------------------------------------
** FUNCTION   : DispSignal
**
** DESCRIPTION: Updates the signal strength
**
** PARAMETERS:	value		<=	A value from 0 to 31
				row			<=	Row to display it at
**				col			<=	Column to display it at
**
** RETURNS:		None
**
**-----------------------------------------------------------------------------
*/
void DispSignal(uint row, uint col)
{
	int value;

	// Get the signal value
	value = Comms(E_COMMS_FUNC_SIGNAL_STRENGTH, NULL);
	
	vdDisplayGPRSSignalStrength(value, 0,col+1 ,row+1);
}

/*
**-----------------------------------------------------------------------------
** FUNCTION   : DispUpdateBattery
**
** DESCRIPTION: Update the battery display
**
** PARAMETERS	row			<=	Row to display it at
**				col			<=	Column to display it at
**
** RETURNS:		None
**
**-----------------------------------------------------------------------------
*/
void DispUpdateBattery(uint row, uint col)
{
	char temp[30];
	static const char * table[20] =
	{
		"\x81\x84\x84\x8A",
		"\x81\x84\x84\x89",
		"\x81\x84\x84\x88",
		"\x81\x84\x84\x87",
		"\x81\x84\x84\x86",
		"\x81\x84\x84\x85",
		"\x81\x84\x8A\x85",
		"\x81\x84\x89\x85",
		"\x81\x84\x88\x85",
		"\x81\x84\x87\x85",
		"\x81\x84\x86\x85",
		"\x81\x84\x85\x85",
		"\x81\x8A\x85\x85",
		"\x81\x89\x85\x85",
		"\x81\x88\x85\x85",
		"\x81\x87\x85\x85",
		"\x81\x86\x85\x85",
		"\x81\x85\x85\x85",
		"\x82\x85\x85\x85",
		"\x83\x85\x85\x85"
	};
	long fullcharge;
	long remainingcharge;

	if ((fullcharge = get_battery_value(FULLCHARGE)) != -1 &&
		(remainingcharge = get_battery_value(REMAININGCHARGE)) != -1 &&
		fullcharge > 0)
	{
		int index = remainingcharge * 20 / fullcharge;
		sprintf(temp, "%3d%% %s", remainingcharge * 100 / fullcharge, (char *) table[index>19?19:index]);
		DispText(temp, row, col, false, false, false);
	}
	else {
		DispText("        ", row, col, false, false, false);
	}
}

void GetBatteryRemaining(uint* battcharing,uint *batt)
{
	long fullcharge;
	long remainingcharge;

	get_battery_value(CHARGERSTATUS);

	if ((fullcharge = get_battery_value(FULLCHARGE)) != -1 &&
		(remainingcharge = get_battery_value(REMAININGCHARGE)) != -1 &&
		fullcharge > 0)
	{
		*batt = remainingcharge *100/fullcharge;
	}

}

#endif



#ifdef __DEBUGDISP

char DebugDisp (const char *template, ...)
{
    char stmp[128];
    char key;
    va_list ap;
	int old_mode = 0;
     
    memset(stmp,0,sizeof(stmp));
    va_start (ap, template);
    vsnprintf (stmp,128, template, ap);
    va_end (ap);

	if(get_display_coordinate_mode() == PIXEL_MODE) {
		old_mode = 1;
		set_display_coordinate_mode(CHARACTER_MODE);
	}

    DispClearScreen();

	setfont("");
    write_at(stmp, strlen(stmp), 1, 0);
    while(read(STDIN, &key, 1) != 1);

	if(old_mode)
		set_display_coordinate_mode(PIXEL_MODE);

	key &= 0x7F;
	return(key);
}

int DebugPrint (const char*template,...) {

    va_list ap;
    char stmp[200];
	char s_debug[30] = "\033k042DEBUG:";

    memset(stmp,0,sizeof(stmp));
    va_start (ap, template);
    vsnprintf (stmp,128, template, ap);

    strcat(stmp,"\n");
	PrtPrintBuffer(strlen(s_debug), (uchar *)s_debug, E_PRINT_START);
	PrtPrintBuffer(strlen(stmp), (uchar *)stmp, E_PRINT_END);//E_PRINT_END
	return 0;
}


#endif
