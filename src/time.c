/*
**-----------------------------------------------------------------------------
** PROJECT:         
**
** FILE NAME:       time.c
**
** DATE CREATED:    31 January 2008
**
** AUTHOR:          Tareq Hafez
**
** DESCRIPTION:     Own implementation of time.c system functions
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
#include "stdio.h"
#include "string.h"

/*
** KEF Project include files.
*/
#include <auris.h>
#include <svc.h>

/*
** Local include files
*/
#include "irisfunc.h"
#include "my_time.h"

/*
**-----------------------------------------------------------------------------
** Constants
**-----------------------------------------------------------------------------
*/
static const int monthDays[2][12] = {{31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
									 {31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}};

/*
**-----------------------------------------------------------------------------
** Module variable definitions and initialisations.
**-----------------------------------------------------------------------------
*/
static struct tm myTime;

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : my_gmtime
**
** DESCRIPTION:	Break a given time into its components
**
** PARAMETERS:	someTime	<=	A time in seconds
**
** RETURNS:		struct tm *
**	None
**-------------------------------------------------------------------------------------------
*/
struct tm * my_gmtime(time_t_ris * someTime) //KK changed
{
	long days;
	uchar leapYear = 0;

	// Initialise
	memset(&myTime, 0, sizeof(myTime));

	myTime.tm_sec = *someTime % 60;
	myTime.tm_min = *someTime / 60 % 60;
	myTime.tm_hour = *someTime / 60 / 60 % 24;
	myTime.tm_yday = days = *someTime / 60 / 60 / 24;
	myTime.tm_wday = (days + 4) % 7;

	// This should work if someTime was created from a year = 0, so leave it as is....days will be still <= 365 and year 70 is not a leap year. so calculation back and forth is good.
	if (*someTime > (60UL * 60UL * 24UL * 365UL * 2UL))
	{
		for (myTime.tm_year = 70; ; myTime.tm_year++)
		{
			leapYear = (myTime.tm_year % 4 == 0)?1:0;
			myTime.tm_yday = days;
			days -= 365;
			if (leapYear) days--;
			if (days < 0) break;
		}
	}

	myTime.tm_mday = ++myTime.tm_yday;

	for (myTime.tm_mon = 0; (myTime.tm_yday > monthDays[leapYear][myTime.tm_mon]) && myTime.tm_mon < 11; myTime.tm_mon++)
	{
		if (myTime.tm_yday > monthDays[leapYear][myTime.tm_mon] && myTime.tm_yday <= monthDays[leapYear][myTime.tm_mon+1])
			myTime.tm_mday = myTime.tm_yday - monthDays[leapYear][myTime.tm_mon];
	}

	return &myTime;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : my_mktime
**
** DESCRIPTION:	Returns the current time in seconds
**
** PARAMETERS:	someTime	<=	Time broken down in as a "struct tm"
**
** RETURNS:		supplied time in seconds
**
**-------------------------------------------------------------------------------------------
*/
time_t_ris my_mktime(struct tm * someTime) //kk changed
{
	time_t_ris timeInSeconds = 0;	//kk changed
	uchar leapYear = (someTime->tm_year >= 73 && someTime->tm_year % 4 == 0)?1:0;

	// Store number of days until the beginning of the year. Also add 1972 extra day since it is a leap year
	// Note that this will not work for years 1970 to 1972, but that's OK. This is the 21st century fox!!!
	if (someTime->tm_year >= 73)
		timeInSeconds = (someTime->tm_year - 70) * 365 + 1;

	// Adjust for leap years.
	if (someTime->tm_year >= 73)
		timeInSeconds += (someTime->tm_year - 73) / 4;

	// Add the days of this year taking into account if it is a leap year
	if (someTime->tm_mon)
		timeInSeconds += monthDays[leapYear][someTime->tm_mon-1];

	// Add the days of the year but adjust to start from zero since 1 January for example carries no days in it and 2 January has only one day and so on
	if (someTime->tm_mday)
		timeInSeconds += (someTime->tm_mday - 1);

	// Adjust the days to seconds
	timeInSeconds *= 24UL * 60UL * 60UL;

	// Now add the time of the current day but adjust all to seconds
	timeInSeconds += someTime->tm_hour * 60UL * 60UL;
	timeInSeconds += someTime->tm_min * 60UL;
	timeInSeconds += someTime->tm_sec;

	return timeInSeconds;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : timeSet
**
** DESCRIPTION:	Set a new time
**
** PARAMETERS:	newTime	<=	Broken down time. Only year, month, day, hour, minute and second matter.
**
** RETURNS:		None
**
**-------------------------------------------------------------------------------------------
*/
void timeSet(struct tm * newTime)
{
	char rtc[15];
	int handle;
	
	handle = open(DEV_CLOCK, 0);
	sprintf(rtc, "%04d%02d%02d%02d%02d%02d", newTime->tm_year+1900, newTime->tm_mon+1, newTime->tm_mday, newTime->tm_hour, newTime->tm_min, newTime->tm_sec);
	write(handle, rtc, 14);
	close(handle);
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : timeFromString
**
** DESCRIPTION:	Converts a time string to a tm structure
**
** PARAMETERS:	string	<=	The time in a string
**
** RETURNS:		Converted time in struct tm
**
**-------------------------------------------------------------------------------------------
*/
struct tm * timeFromString(char * string)
{
	myTime.tm_year = ((string[0] - '0') * 1000 + (string[1] - '0') * 100 + (string[2] - '0') * 10 + string[3] - '0') - 1900;
	myTime.tm_mon = ((string[4] - '0') * 10 + string[5] - '0') - 1;
	myTime.tm_mday = (string[6] - '0') * 10 + string[7] - '0';
	myTime.tm_hour = (string[8] - '0') * 10 + string[9] - '0';
	myTime.tm_min = (string[10] - '0') * 10 + string[11] - '0';
	myTime.tm_sec = (string[12] - '0') * 10 + string[13] - '0';

	return &myTime;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : time
**
** DESCRIPTION:	Returns the current time in seconds since 1970
**
** PARAMETERS:	now	<=	This will contain the current time in seconds
**
** RETURNS:		Current itime in seconds since 1970
**
**-------------------------------------------------------------------------------------------
*/
time_t_ris my_time(time_t_ris * now)	//KK changed
{
	time_t_ris timeInSeconds;	//KK changed
	char rtc[15];

	// Read the clock
	read_clock(rtc);

	timeInSeconds = my_mktime(timeFromString(rtc));

	if (now) *now = timeInSeconds;

	return timeInSeconds;
}

unsigned long systicks()
{
	return read_ticks();
}
