//
//-----------------------------------------------------------------------------
// PROJECT:			iRIS
//
// FILE NAME:       iristime.c
//
// DATE CREATED:    5 March 2008
//
// AUTHOR:          Tareq Hafez
//
// DESCRIPTION:     This module supports security functions
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
#include <svc.h>

//
// Local include files
//
#include "my_time.h"
#include "iris.h"
#include "irisfunc.h"

//
//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
//
#define	C_YEAR		0
#define	C_MON		1
#define	C_DAY		2
#define	C_HOUR		3
#define	C_MIN		4
#define	C_SEC		5

#define	C_FMT_MAX	6

//
//-----------------------------------------------------------------------------
// Module variable definitions and initialisations.
//-----------------------------------------------------------------------------
//
static unsigned long ticks[10]={0,0,0,0,0,0,0,0,0,0};

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
// TIME FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()NOW
//
// DESCRIPTION:	Returns the current time
//
// PARAMETERS:	None
//
// RETURNS:		The current time in strunc tm format
//-------------------------------------------------------------------------------------------
//
void __now(char *result)
{
	sprintf(result, "%lu", my_time(NULL));
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()TIME_SET
//
// DESCRIPTION:	Sets the current time
//
// PARAMETERS:	value	<=	 time to set clock to. Make sure it is populated with YYMMDDhhmmss at least
//
// RETURNS:		None
//-------------------------------------------------------------------------------------------
//
void __time_set(const char *newTime,char* adjust)
{

	if (newTime && strlen(newTime)==14)
	{
		int handle;
		char timeoffset[30]="";

		handle = open(DEV_CLOCK, 0);

		if(adjust && strlen(adjust) && atol(adjust)) {
			struct tm currtm;
			time_t_ris timeinsec;

		   	memcpy( &currtm, timeFromString((char *)newTime),sizeof(currtm));
			timeinsec = my_mktime(&currtm);
			timeinsec += atol(adjust) * 60 ; // minutes
		   	memcpy( &currtm, my_gmtime(&timeinsec),sizeof(currtm));
			timeSet(&currtm);
		}else
			write(handle, newTime, 14);
		close(handle);
	}
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()TIME
//
// DESCRIPTION:	converta a time structure to a formatted string
//
// PARAMETERS:	format	<=	Maps the value provided to ne of the elements within "struct tm"
//				value	<=	 time to set clock to. Make sure it is populated with YYMMDDhhmmss at least
//
// RETURNS:		None
//-------------------------------------------------------------------------------------------
//
void ____time(char * value, int * which, char * output, int * j)
{
	int k;
	time_t_ris sometime = atol(value);			//KK changed
	struct tm * myTime = my_gmtime(&sometime);

	// Process year 4 digits format first
	while (which[C_YEAR] >= 4)
	{
		sprintf(&output[*j],"%0.4d", myTime->tm_year + 1900);
		(*j) += 4;
		which[C_YEAR] -= 4;
	}

	// Process month 3 digit format second
	while (which[C_MON] >= 3)
	{
		static const char * months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
		sprintf(&output[*j],"%s", months[myTime->tm_mon]);
		(*j) += 3;
		which[C_MON] -= 3;
	}

	// Process the 2-digit formats including the year now
	for (k = 0; k < C_FMT_MAX; k++)
	{
		while (which[k] >= 2)
		{
			sprintf(&output[*j],"%0.2d",	k == C_YEAR? (myTime->tm_year%100):
											(k == C_MON? (myTime->tm_mon + 1):
											(k == C_DAY? myTime->tm_mday:
											(k == C_HOUR? myTime->tm_hour:
											(k == C_MIN? myTime->tm_min:myTime->tm_sec)))));
			(*j) += 2;
			which[k] -= 2;
		}

		if (which[k])
		{
			output[(*j)++] = (k == C_YEAR?'Y':(k == C_MON?'M':(k == C_DAY?'D':(k == C_HOUR?'h':(k == C_MIN?'m':'s')))));
			which[k]--;
		}
	}
}

char*  __time_real(const char* format,char *output)
{
	char now[30];

	if(output == NULL || format == NULL || !strlen(format))
	{
		output = NULL;
	} else {
	//YYYYMMDDhhmmssW
		char *p = (char *)format;
		char *s = output;
		read_clock(now);
		while(*p) {
			if( *p == 'Y' && *(p+1) == 'Y' && *(p+2)=='Y' && *(p+3)=='Y') {
				memcpy(s,now,4);
				p = p+4; s = s+4;
			}
			else if( *p == 'Y' && *(p+1) == 'Y' ) {
				*s = now[2];	*(s+1) = now[3];	
				p = p+2; s = s+2;
			}
			else if( *p == 'M' && *(p+1) == 'M' ) {
				*s = now[4];	*(s+1) = now[5];	
				p = p+2; s = s+2;
			}
			else if( *p == 'D' && *(p+1) == 'D' ) {
				*s = now[6];	*(s+1) = now[7];	
				p = p+2; s = s+2;
			}
			else if( *p == 'h' && *(p+1) == 'h' ) {
				*s = now[8];	*(s+1) = now[9];	
				p = p+2; s = s+2;
			}
			else if( *p == 'm' && *(p+1) == 'm' ) {
				*s = now[10];	*(s+1) = now[11];	
				p = p+2; s = s+2;
			}
			else if( *p == 's' && *(p+1) == 's' ) {
				*s = now[12];	*(s+1) = now[13];	
				p = p+2; s = s+2;
			}
			else if( *p == 'W' ) {
				*s = now[14];
				p = p+1; s = s+1;
			} else {
				*s = *p;
				p = p+1; s = s+1;
			}
		}
		*s = '\0';
	}
	return(output);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()TIMER_START
//
// DESCRIPTION:	Starts a timer in millisecond resolution. Returns the current ticks in milliseconds
//
// PARAMETERS:	None
//
// RETURNS:		None
//-------------------------------------------------------------------------------------------
//
void __timer_start(int timer_idx)
{
	int idx = timer_idx;
	if(timer_idx >10) idx = 0;
	ticks[idx] = read_ticks();
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()TIMER_STOP
//
// DESCRIPTION:	Stops the timer. Returns the time difference in milliseconds from the start of the timer.
//
// PARAMETERS:	None
//
// RETURNS:		None
//-------------------------------------------------------------------------------------------
//
long __timer_stop(int timer_idx)
{
	unsigned long ticks2 = read_ticks();
	unsigned long diff;
	int idx = timer_idx;
	if(timer_idx >10) idx = 0;

	if (ticks2 < ticks[idx])
		diff = 0xFFFFFFFF - ticks[idx] + ticks2 + 1;
	else
		diff = ticks2 - ticks[idx];
	return(diff);
}

bool __timer_started(int timer_idx)
{
	int idx = timer_idx;
	if(timer_idx >10) idx = 0;
	if( ticks[idx] > 0) return true;
	else return false;
}
