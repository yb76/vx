#ifndef __TIMER_H
#define __TIMER_H

/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       timer.h
**
** DATE CREATED:    10 July 2007
**
** AUTHOR:			Tareq Hafez
**
** DESCRIPTION:     Describes the constants, type definitions and function
**					declarations used within AURIS
**-----------------------------------------------------------------------------
*/

typedef ulong TIMER_TYPE;

bool TimerArm( TIMER_TYPE * timer, ulong timeout );
bool TimerExpired(TIMER_TYPE * timer);
ulong TimerElapsed(TIMER_TYPE * timer);

#endif /* __TIMER_H */
