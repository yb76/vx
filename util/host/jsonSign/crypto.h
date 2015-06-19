#ifndef __CRYPTO_H
#define __CRYPTO_H

/*
**-----------------------------------------------------------------------------
** PROJECT:         i-RIS (Key Injection)
**
** FILE NAME:       crypto.h
**
** DATE CREATED:    1 February 2008
**
** AUTHOR:			Tareq Hafez
**
** DESCRIPTION:     Header file for creating cryptographics functions
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

CK_RV Crypto_Init(CK_SLOT_ID slotId);
CK_RV Crypto_Done(void);


CK_RV Crypto_FindKey(CK_CHAR * keyName, CK_OBJECT_HANDLE * phKey);

CK_RV Crypto_SKEncrypt(CK_CHAR_PTR data, CK_ULONG length);

#endif /* __CRYPTO_H */
