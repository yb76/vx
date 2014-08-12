#ifndef __IRISCOMMS_H
#define __IRISCOMMS_H

/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       iriscomms.h
**
** DATE CREATED:    31 January 2008
**
** AUTHOR:			Tareq Hafez
**
** DESCRIPTION:     Header file for the iris generic communication functions
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

/*
**-----------------------------------------------------------------------------
** Functions
**-----------------------------------------------------------------------------
*/

void IRIS_CommsSend(const char *data, T_COMMS * comms, int * retVal, char *retmsg);

void IRIS_CommsRecv(T_COMMS * comms, const char * ito,const char *to,const int bufLen, int * retVal, char ** result,char *retmsg);

void IRIS_CommsDisconnect(T_COMMS * comms, int retVal);

void IRIS_CommsErr(int retVal);


#endif /* __IRISCOMMS_H */
