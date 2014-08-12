/*
**-----------------------------------------------------------------------------
** PROJECT:			AURIS
**
** FILE NAME:       timer.c
**
** DATE CREATED:    10 July 2007
**
** AUTHOR:          Tareq Hafez
**
** DESCRIPTION:     This module supports timer functions
**-----------------------------------------------------------------------------
*/

//
//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------
//

//
// Standard include files.
//

//
// Project include files.
//
#include <auris.h>
#include <svc.h>

/*
** Local include files
*/
#include "timer.h"

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
static TIMER_TYPE DefaultTimer;

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : TimerArm
**
** DESCRIPTION:	Arm / Reset the specified timer
**
** PARAMETERS:	None
**
** RETURNS:		None
**-------------------------------------------------------------------------------------------
*/
bool TimerArm(TIMER_TYPE * timer, ulong timeout)
{
	TIMER_TYPE myTimer = read_ticks();
	myTimer += timeout * TICKS_PER_SEC / 10000;	// timeout unit = 1/10 of a millisecond...adjust

	/* Assign it to the proper timer */
	if (timer)
		*timer = myTimer;
	else
		DefaultTimer = myTimer;

	return true;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : TimerExpired
**
** DESCRIPTION:	Check if the timer has expired
**
** PARAMETERS:	None
**
** RETURNS:		None
**-------------------------------------------------------------------------------------------
*/
bool TimerExpired(TIMER_TYPE * timer)
{
	ulong tick;
	TIMER_TYPE myTimer;

	/* Get either the specified or default timer */
	if (timer)
		myTimer = *timer;
	else
		myTimer = DefaultTimer;

	tick = read_ticks();
	if (tick > myTimer)
		return true;

	/* Otherwise, indicate that it has not expired */
	return false;
}
