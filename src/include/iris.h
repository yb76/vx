#ifndef __IRIS_H
#define __IRIS_H

/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       iris.h
**
** DATE CREATED:    31 January 2008
**
** AUTHOR:			Tareq Hafez
**
** DESCRIPTION:     Header file for the iris JSON object support functions
**-----------------------------------------------------------------------------
*/

#include "input.h"


/*
**-----------------------------------------------------------------------------
** Constants
**-----------------------------------------------------------------------------
*/
extern char irisGroup[];

/*
**-----------------------------------------------------------------------------
** Type definitions
**-----------------------------------------------------------------------------
*/
typedef struct
{
	char * group;
	char * name;
	char * value;
	uchar flag;		// bit 0: Read access by other groups allowed. (.r)
					// bit 1: Write access by other groups allowed (.w)
					// bit 2: Byte Array. (.h)
					// bit 8: temp. Automatically deleted when IDLE display is current. (.t)
} T_TEMPDATA;

extern char * currentObject;
extern char * currentObjectData;
extern uint currentObjectLength;
extern char * currentObjectVersion;
extern char * currentObjectGroup;
extern char * currentEvent;
extern char * currentEventValue;
extern char * newEventValue;

extern char * prevObject;
extern char * nextObject;

extern char * forceNextObject;

extern int stackIndex;
extern uchar stackLevel;

extern int mapIndex;

extern ulong displayTimeout;
extern int displayTimeoutMultiplier;
extern int displayTimeoutMultiplierFull;

extern unsigned char lastKey;
extern bool vmacRefresh;

extern char * currentTableName;
extern char * currentTableData;
extern bool currentTableValid;
extern char * currentTablePromptValue;

// This is defined within irismain.c
char * getLastKeyDesc(unsigned char key, T_KEYBITMAP * keyBitmap);


/*
**-----------------------------------------------------------------------------
** Functions
**-----------------------------------------------------------------------------
*/

// Add data to be uploaded to the host at the next session
void IRIS_AppendToUpload(const char * addition);

// Store an object data buffer into a file.
void IRIS_PutNamedObjectData(const char * objectData, uint length, const char * name);

// Store an object data buffer into a file. The file name will be the value of the string "NAME" in the object
void IRIS_PutObjectData(const char * objectData, uint length);

// Get the object data buffer from a file. The file name is the object name. Files are stored by their object names.
char * IRIS_GetObjectData(const char * objectName, unsigned int * length);

char * IRIS_GetObjectData_all(const char * objectName, unsigned int * length, int flag);

// Given an object data, search for a string and return its value in an allocated tree strcutures "array of arrays where the leaves are simple values"
char * IRIS_GetStringValue(const char * data, int size, const char * name );

// Deallocate the allocate tree structure structure
void IRIS_DeallocateStringValue(char * value);

// Given a fully qualified string name and value, store the value in the string
void IRIS_StoreData(const char * fullName, const char* tag, const char * value, bool deleteFlag);

char * IRIS_GetObjectTagValue(const char * objectName, const char * tag);

#endif /* __IRIS_H */
