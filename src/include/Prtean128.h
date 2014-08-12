/*
**-----------------------------------------------------------------------------
** PROJECT:         PrinterModems
**
** FILE NAME:       PRTEAN128.c
**
** DATE CREATED:    22 January 2009
**
** AUTHORS:         Tareq Hafez
**
** DESCRIPTION:     Module to handle the EAN 128 barcode font
**-----------------------------------------------------------------------------
*/

#ifndef __PRTEAN128_H
#define __PRTEAN128_H

int BuildEan128BarCode ( uchar * bpChar, uchar * abPrintBlock, uint wSizePrintBlock, int height );

#endif
