#ifndef __PRINTER_H_
#define __PRINTER_H_

/*
**-----------------------------------------------------------------------------
** PROJECT:			EZIPIN
**
** FILE NAME:		Printer.h
**
** DATE CREATED:	28/02/01
**
** AUTHOR:			Tareq Hafez
**
** DESCRIPTION:		Printer Interface Functions
**-----------------------------------------------------------------------------
*/

/*
**-----------------------------------------------------------------------------
**
** Include Files
**
**-----------------------------------------------------------------------------
*/

/*
** ......general include files such a standard C library header files.......
*/

/*
** KEF Project include files.
*/
#include <svc.h>
#include <auris.h>

/*
** Local include files
*/

/*
**-----------------------------------------------------------------------------
** Module variable definitions and initialisations.
**-----------------------------------------------------------------------------
*/

typedef enum
{
	E_PRT_ERR_NONE = 0,
	E_PRT_ERR_CONNECTION_FAILED,
	E_PRT_ERR_SEND_FAILED,
	E_PRT_ERR_PAPER_OUT,
	E_PRT_ERR_GRAPHICS,
	E_PRT_ERR_MECHANISM,
	E_PRT_ERR_FIRMWARE,
	E_PRT_ERR_GENERAL
} E_PRT_ERR;

typedef enum
{
	E_PRINT_START,
	E_PRINT_APPEND,
	E_PRINT_END
} E_PRINT;

/*
**-----------------------------------------------------------------------------
** Function Declarations
**-----------------------------------------------------------------------------
*/
void PrtInit(void);
char * PrtGetErrorText(E_PRT_ERR eError);

E_PRT_ERR PrtPrintBuffer(uint wLength, uchar * pbData, int ePrintCommand);
E_PRT_ERR PrtPrintFormFeed(void);
E_PRT_ERR PrtPrintGraphics(uint wWidth, uint wHeight, uchar * pbData, bool fCenter, uchar bMultiplier);

E_PRT_ERR PrtStatus(bool fStatusCheck);

void DPPrintDebug(char * data);
void DPPrintDebug1(char * data, int len);
void Comm1Debug(char * data);
#endif /*__PRINTER_H_*/
