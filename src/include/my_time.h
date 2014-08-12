#ifndef __TIME_H
#define __TIME_H

/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       my_time.h
**
** DATE CREATED:    31 January 2008
**
** AUTHOR:			Tareq Hafez
**
** DESCRIPTION:     Header file for the "time" module
**-----------------------------------------------------------------------------
*/

/*
**-----------------------------------------------------------------------------
** Constants
**-----------------------------------------------------------------------------
*/

/*
**-----------------------------------------------------------------------------
** Type definitions
**-----------------------------------------------------------------------------
*/
struct tm
{
	int tm_sec;		// seconds after the minute (0-59)
	int tm_min;		// minutes after the hour (0-59)
	int tm_hour;	// hours since midnight (0-23)
	int tm_mday;	// day of the month (1-31)
	int tm_mon;		// months since January (0-11)
	int tm_year;	// years since 1900
	int tm_wday;	// days since sunday (0-6)
	int tm_yday;	// days since January 1st (0-365/366)
//	int tm_isdst;	// daylight saving time flag
};

// KK changed the name to time_t_ris to avoid clashes with other time_t implementations
typedef unsigned long time_t_ris;

/*
**-----------------------------------------------------------------------------
** Functions
**-----------------------------------------------------------------------------
*/
struct tm * my_gmtime(time_t_ris * someTime);
time_t_ris my_mktime(struct tm * someTime);
void timeSet(struct tm * newTime);
struct tm * timeFromString(char * string);
time_t_ris my_time(time_t_ris * now);
unsigned long systicks(void);

#endif /* __TIME_H */
