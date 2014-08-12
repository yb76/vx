#ifndef __ALLOC_H
#define __ALLOC_H

/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       alloc.h
**
** DATE CREATED:    29 January 2008
**
** AUTHOR:			Tareq Hafez
**
** DESCRIPTION:     Allocation functions forcing a 4-byte boundry
**
**-----------------------------------------------------------------------------
*/

//
//-----------------------------------------------------------------------------
// Constant Definitions.
//-----------------------------------------------------------------------------
//

#define	C_TINY_BLOCK_SIZE	32
#define	C_MAX_TINY_BLOCKS	4000

#define	C_MED_BLOCK_SIZE	1024
#define	C_MAX_MED_BLOCKS	60

#define	C_BIG_BLOCK_SIZE	32 * 1024
#define	C_MAX_BIG_BLOCKS	7

//
//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------
//

typedef struct
{
	void * block[C_TINY_BLOCK_SIZE / sizeof(void *)];
} T_TINY_HEAP;

typedef struct
{
	void * block[C_MED_BLOCK_SIZE / sizeof(void *)];
} T_MED_HEAP;

typedef struct
{
	unsigned char block[C_BIG_BLOCK_SIZE];
} T_BIG_HEAP;


extern T_TINY_HEAP iris_tiny_heap[];
extern T_MED_HEAP iris_med_heap[];
extern T_BIG_HEAP iris_big_heap[];

//
//-----------------------------------------------------------------------------
// Function Definitions
//-----------------------------------------------------------------------------
//

void * my_calloc(unsigned int size);
void * my_malloc(unsigned int size);
void my_free(void * ptr);
void * my_realloc(void * ptr, unsigned int size);

#endif /* __ALLOC_H */
