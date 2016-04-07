//
//-----------------------------------------------------------------------------
// PROJECT:			iRIS
//
// FILE NAME:       iris_io.c
//
// DATE CREATED:    5 March 2008
//
// AUTHOR:          Tareq Hafez
//
// DESCRIPTION:     This module supports I/O functions including
//					- Keyboard input results
//					- Printing
//					- Beeper
//					- Magnetic card reads.
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

//
// Local include files
//
#include "alloc.h"
#include "input.h"
#include "printer.h"
#include "utility.h"
#include "iris.h"
#include "irisfunc.h"

//
//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
//
static const char banner[] = "**** GRAPHICS START ****";
static const char banner2[] = "**** GRAPHICS END   ****";

static const struct
{
	char * search;
	char * replace;
} special[5] = {	{"**** COMMA ****", ","}, {"**** WIGGLY ****", "{"}, {"**** YLGGIW ****", "}"}, {"**** BRACKET ****", "["}, {"**** TEKCARB ****", "]"}	};


//
//-----------------------------------------------------------------------------
// Module variable definitions and initialisations.
//-----------------------------------------------------------------------------
//


//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
// I/O FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////


//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()PRINT
//
// DESCRIPTION:	Prints formatted data
//
//
// PARAMETERS:	None
//
// RETURNS:		status of printing is updated but nothing returned
//-------------------------------------------------------------------------------------------
//
static void __add_print_data(char ** dump, char * data, int size)
{
	char * n;

	do
	{
		int i;
		char * banner="";
		char * p;

		for (n = NULL, i = 0; i < 5; i++)
		{
			if ((p = strchr(data, special[i].replace[0])) != NULL && (n == NULL || p < n))
				n = p, banner = special[i].search;
		}

		if (n)
		{
			size += (strlen(banner) - 1);	// Comma banner string minus the comma itself
			*dump = my_realloc(*dump, size);

			*n = '\0';
			strcat(*dump, data);
			strcat(*dump, banner);
			data = n + 1;
		}
		else
			strcat(*dump, data);
	} while (n);
}

static char * ____printDump(char * dump, char * data, int graphics)
{
	int size;

	// Calculate the new size
	size = strlen(data) + (graphics?(strlen(banner)+strlen(banner2)):0) + (dump?strlen(dump):0) + 1;

	// Reallocate enough memory
	if (dump)
		dump = my_realloc(dump, size);
	else
		dump = my_calloc(size);

	// Add the header banner if graphics
	if (graphics)
		strcat(dump, banner);

	// Add the print data
	__add_print_data(&dump, data, size);

	// Add the trailer banner if graphics and lose what we have allocated to store the graphics
	if (graphics)
		strcat(dump, banner2);

	// Return the dump
	return dump;
}

char * __print_cont(const char * data,bool endflag)
{
	int i;
	bool skip = false;
	bool centre = false;
	char leftPart[200];
	char line[200];
	char output[200];
	int maxLine = 24;
	int largeFont = 24;
	bool doubleWidth = false;
	bool doubleHeight = false;
	bool inverse = false;
	char * dump = NULL;
	uint cpl = 0;

	// Initialisation
	memset(line, 0, sizeof(line));
	memset(leftPart, 0, sizeof(leftPart));

	for (i = 0; data && data[i]; i++)
	{
			// Process the print data
			if (data[i] == '\\')
				skip = !skip;
			else if (skip && data[i] == 'C')
			{
				centre = true;
				skip = false;
				continue;
			}
			else if (skip && data[i] == 'c')
			{
				cpl = (data[i+1] - '0') * 10 + data[i+2] - '0';
				i += 2;
				skip = false;
				continue;
			}
			else if (skip && data[i] == 'R')
			{
				strcpy(leftPart, line);
				memset(line, 0 , sizeof(line));
				centre = skip = false;
				continue;
			}
			else if (skip && data[i] == 'n')
			{
				if (leftPart[0])
					sprintf(output, "%s%s%s%s%s%*s\n",	largeFont == 24? "\033k999":(largeFont == 32?"\033k032":"\033k042"),
														doubleWidth?"\033G":"", doubleHeight?"\033h":"", inverse?"\033i":"", leftPart, maxLine-strlen(leftPart), line);
				else if (centre)
					sprintf(output, "%s%s%s%s%*s\n",		largeFont == 24? "\033k999":(largeFont == 32?"\033k032":"\033k042"),
														doubleWidth?"\033G":"", doubleHeight?"\033h":"", inverse?"\033i":"", (maxLine+strlen(line))/2, line);
				else
					sprintf(output, "%s%s%s%s%s\n",		largeFont == 24? "\033k999":(largeFont == 32?"\033k032":"\033k042"),
														doubleWidth?"\033G":"", doubleHeight?"\033h":"",  inverse?"\033i":"",line);

				memset(line, 0, sizeof(line));
				memset(leftPart, 0, sizeof(leftPart));

				PrtPrintBuffer((uint) strlen(output), (uchar *) output, E_PRINT_APPEND);
				dump = ____printDump(dump, output, 0);

				centre = skip = false;
				cpl = 0;
				continue;
			}
			else if (skip && data[i] == 'f')
			{
				largeFont = maxLine = 24;
				doubleHeight = doubleWidth = inverse = false;
				skip = false;
				cpl = 0;
				continue;
			}
			else if (skip && data[i] == 'F')
			{
				largeFont = 32;
				doubleHeight = inverse = false;
				doubleWidth = true;
				maxLine = 16;
				skip = false;
				cpl = 0;
				continue;
			}
			else if (skip && data[i] == '3')
			{
				largeFont = maxLine = 32;
				doubleHeight = doubleWidth = inverse = false;
				skip = false;
				cpl = 0;
				continue;
			}
			else if (skip && data[i] == '4')
			{
				largeFont = maxLine = 42;
				doubleHeight = doubleWidth = inverse = false;
				skip = false;
				cpl = 0;
				continue;
			}
			else if (skip && data[i] == 'H')
			{
				doubleHeight = true;
				skip = false;
				continue;
			}
			else if (skip && data[i] == 'h')
			{
				doubleHeight = false;
				skip = false;
				continue;
			}
			else if (skip && data[i] == 'i')
			{
				inverse = true;
				skip = false;
				continue;
			}
			else if (skip && data[i] == 'W')
			{
				if (doubleWidth == false)
				{
					doubleWidth = true;
					maxLine /= 2;
				}
				skip = false;
				continue;
			}
			else if (skip && data[i] == 'w')
			{
				if (doubleWidth == true)
				{
					doubleWidth = false;
					maxLine *= 2;
				}
//				maxLine /= 2;		// ****** This needs to be deleted.....wrong....TH.
				skip = false;
				continue;
			}
			else if (skip && data[i] == 'B')
			{
				char barcode[20];
				sprintf(barcode, "\033k901%.12s", &data[i+1]);
				PrtPrintBuffer((uint) strlen(barcode), (uchar *) barcode, E_PRINT_APPEND);
				dump = ____printDump(dump, barcode, 0);
				i += 12;
				skip = false;
				continue;
			}
			else if (skip && data[i] == '*')
			{
				int size = 0;
				char barcode[120];
				char * end = strchr(&data[i+1], '\\');
				if (end) size = end - &data[i+1];
				if (size > 0 && size < 100)
				{
					sprintf(barcode, "\033k902%.*s\033k903", size, &data[i+1]);
					PrtPrintBuffer((uint) strlen(barcode), (uchar *) barcode, E_PRINT_APPEND);
					dump = ____printDump(dump, barcode, 0);
					i += size;
				}
				skip = false;
				continue;
			}
			else if (skip && data[i] == 'g')
			{
				int j;
				char name[50];
				unsigned int length;
				char * objectData;

				for (i++, j = 0; data[i] != '\\'; i++, j++)
					name[j] = data[i];
				name[j] = '\0', i--;

				// Obtain the graphics object locally or go to the host if not available
				objectData = IRIS_GetObjectData(name, &length);

				if (objectData)
				{
					char * image = IRIS_GetStringValue(objectData, length, "IMAGE");

					if (image)
					{
							int width, height;
							unsigned char * imageb = my_malloc(strlen(image) / 2 + 1);

							UtilStringToHex(image, strlen(image), imageb);

							PrtPrintBuffer(1, "\n", E_PRINT_END);

							width = imageb[0] * 256 + imageb[1];
							height = imageb[2] * 256 + imageb[3];
							PrtPrintGraphics(width, height, &imageb[4], true, (uchar) ((height >= 120 || width > 192)? 1:2));
							dump = ____printDump(dump, (char *) image, 1);

							my_free(imageb);

						IRIS_DeallocateStringValue(image);
					}

					my_free(objectData);
				}

				skip = false;
				continue;
			}

			// Loop start, set the index variables and remember them for counting later
			else if (skip && data[i] == 'L')
			{
				// Remember the beginning of the loop block
				skip = false;
				continue;
			}
			else
				skip = false;

			// Add the printing character to the current line
			if (skip == false)
			{
				if (data[i] != '\r' && data[i] != '\n')
					line[strlen(line)] = data[i];
				if (data[i] == '\n' || (cpl && (strlen(leftPart) + strlen(line)) >= cpl))
				{
					if (leftPart[0])
						sprintf(output, "%s%s%s%s%s%*s\n",	largeFont == 24? "\033k999":(largeFont == 32?"\033k032":"\033k042"),
															doubleWidth?"\033G":"", doubleHeight?"\033h":"", inverse?"\033i":"", leftPart, maxLine-strlen(leftPart), line);
					else if (centre)
						sprintf(output, "%s%s%s%s%*s\n",		largeFont == 24? "\033k999":(largeFont == 32?"\033k032":"\033k042"),
															doubleWidth?"\033G":"", doubleHeight?"\033h":"", inverse?"\033i":"", (maxLine+strlen(line))/2, line);
					else
						sprintf(output, "%s%s%s%s%s\n",		largeFont == 24? "\033k999":(largeFont == 32?"\033k032":"\033k042"),
															doubleWidth?"\033G":"", doubleHeight?"\033h":"",  inverse?"\033i":"",line);

					memset(line, 0, sizeof(line));
					memset(leftPart, 0, sizeof(leftPart));

					PrtPrintBuffer((uint) strlen(output), (uchar *) output, E_PRINT_APPEND);
					dump = ____printDump(dump, output, 0);

					centre = false;
				}	
			}
	}

    if(endflag) PrtPrintFormFeed();
	return(dump);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()SHUTDOWN
//
// DESCRIPTION:	Turns of the terminal. This only works with a Vx670 terminal that is not externally powered.
//
// PARAMETERS:	None
//
// RETURNS:		None
//-------------------------------------------------------------------------------------------
//
void __shutdown(void)
{
	if (SVC_SHUTDOWN() == 0)
		SVC_WAIT(10000);	// Make sure we do not start any operation while shuting down.
}


void __print_err(char *result)
{
	strcpy(result, PrtGetErrorText(PrtStatus(true)));
}

int DebugPrint2 (char *timeflag,const char*template,...)
{
    va_list ap;
	char stmp[200];
	static int debugflag = -1;

    memset(stmp,0,sizeof(stmp));
    va_start (ap, template);
    vsnprintf (stmp,128, template, ap);
    //strcat(stmp,"\n");
    if(timeflag&&strlen(timeflag)) {
    	char dt[30]="";
    	__time_real(timeflag,dt);
    	strcat(stmp,dt);
    }
  PrtPrintBuffer(strlen(stmp), (uchar *)stmp, E_PRINT_END);
  return(0);
}
