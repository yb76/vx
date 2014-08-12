#ifndef __AURIS_H
#define __AURIS_H

/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       auris.h
**
** DATE CREATED:    10 July 2007
**
** AUTHOR:			Tareq Hafez
**
** DESCRIPTION:     This contains general constants and type definitions used
**					throughout the project
**-----------------------------------------------------------------------------
*/

//
//-----------------------------------------------------------------------------
// Constant Definitions.
//-----------------------------------------------------------------------------
//
#define	false					0
#define	true					1

//
//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------
//
typedef	unsigned char	uchar;
typedef	unsigned int	uint;
typedef	unsigned long	ulong;
typedef	unsigned char	bool;
typedef	unsigned short	ushort;

#ifndef __int8_t_defined
#define __int8_t_defined
typedef signed char int8_t;
typedef short int16_t;
//typedef int int32_t;
typedef long long int64_t;
#endif

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
#ifndef __uint32_t_defined
#define __uint32_t_defined
//typedef unsigned int uint32_t;
#endif
typedef unsigned long long uint64_t;


//Defs
#define getch _getch
#define itoa _itoa


#define	FH_RDONLY				O_RDONLY
#define	FH_NEW					(O_CREAT | O_TRUNC | O_WRONLY | O_APPEND)
#define	FH_RDWR					O_RDWR
#define FH_NEW_RDWR				(O_CREAT | O_TRUNC | O_RDWR)

#define	FH_OK(h)				(h != -1)
#define	FH_ERR(h)				(h == -1)

#define	_delete_(h,c,x,y,z)		delete_(h,c)
#define	_insert(h,d,c,x,y,z)	insert(h,d,c)

#define printf(a,...)
#define	FILE_HANDLE				int


#ifdef __DEBUGDISP
	#include <stdarg.h>
	char DebugDisp (const char *template, ...);
#else
	#define	DebugDisp( a, ... ) 	
#endif

	
#endif /* __AURIS_H */
