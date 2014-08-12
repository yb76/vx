/*
**-----------------------------------------------------------------------------
** PROJECT:         PrinterModems
**
** FILE NAME:       PRTEAN13.c
**
** DATE CREATED:    10 August 2007
**
** AUTHORS:         Tareq Hafez
**
** DESCRIPTION:     Module to handle the EAN 13 bar code font.
**-----------------------------------------------------------------------------
*/

#ifndef __PRTEAN13_H
#define __PRTEAN13_H

// start column for EAN 13 barcode
#define EAN13_START_COLUMN  50

// EAN 13 barcode raster bitmap dimensions
#define EAN13_PRINT_HEIGHT	128
#define EAN13_PRINT_WIDTH  384

int BuildEan13BarCode ( uchar * bpChar, uchar * abPrintBlock, uint wSizePrintBlock);

#endif
