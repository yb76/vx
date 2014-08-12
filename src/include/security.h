#ifndef __SECURITY_H
#define __SECURITY_H

/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       security.h
**
** DATE CREATED:    29 January 2008
**
** AUTHOR:			Tareq Hafez
**
** DESCRIPTION:     Cryptographic Security Functions
**
**-----------------------------------------------------------------------------
*/

//
//-----------------------------------------------------------------------------
// Constant Definitions.
//-----------------------------------------------------------------------------
//

//
//-----------------------------------------------------------------------------
// Type Definitions
//-----------------------------------------------------------------------------
//
extern uchar iris_ktk2[17];
extern uchar iris_ktk3[17];

//
//-----------------------------------------------------------------------------
// Function Definitions
//-----------------------------------------------------------------------------
//
void SecurityInit(void);

bool SecurityWriteKey(char * appName, uchar location, uchar keySize, uchar * key);
bool SecurityEraseKey(char * appName, uchar location, uchar keySize);
bool SecurityGenerateKey(char * appName, uchar location, uchar keySize);

bool SecurityKVCKey(char * appName, uchar location, uchar keySize, uchar * kvc);

bool SecurityCopyKey(char * appName, uchar location, uchar keySize, uchar to);
bool SecurityMoveKey(char * appName, uchar location, uchar keySize, uchar to);
bool SecurityXorKey(char * appName, uchar location, uchar keySize, uchar with, uchar to);
bool SecurityXorKeyWithData(char * appName, uchar location, uchar keySize, uchar *xdata, uchar to);

bool SecuritySetIV(uchar * iv);

bool SecurityCrypt(char * appName, uchar location, uchar keySize, int eDataSize, uchar * eData, bool decrypt, bool ofb);
bool SecurityCryptWithVariant(char * appName, uchar location, uchar keySize, int eDataSize, uchar * eData, uchar * variant, bool decrypt);
bool SecurityMAB(char * appName, uchar location, uchar keySize, int eDataSize, uchar * eData, uchar * variant, uchar * mab);

bool SecurityInjectKey(char * appName, uchar location, uchar keySize, char * appName2, uchar to, uchar eKeySize, uchar * eKey, uchar * variant);
bool SecurityOWFKey(char * appName, uchar location, uchar keySize, uchar to, uchar ppasn, bool variant);
bool SecurityOWFKeyWithData(char * appName, uchar location, uchar keySize, uchar to, uchar * variant);

bool Security3DESWriteRSA(char * appName, uchar location, char * appName2, uchar to, int blockLen, uchar * block);
bool SecurityWriteRSA(char * appName, uchar location, int blockLen, uchar * block);
bool SecurityClearRSA(char * appName, uchar location);
bool SecurityCryptRSA(char * appName, uchar location, int eDataSize, uchar * eData);
bool SecurityRSAWrapKey(char * appName, uchar location, char * appName2, uchar from, uchar * eData);
bool SecurityRSAInjectKey(char * appName, uchar location, char * appName2, uchar to, uchar eKeySize, int eDataSize, uchar * eData);
bool SecurityRSAInjectRSA(char * appName, uchar location, char * appName2, uchar to, int eDataSize, uchar * eData);

bool SecurityPINBlock(char * appName, uchar location, uchar keySize, char * pan, uchar * ePinBlock);
bool SecurityPINBlockWithVariant(char * appName, uchar location, uchar keySize, char * pan, char * stan, char * amount, bool emv,uchar * ePinBlock);


#endif /* __SECURITY_H */
