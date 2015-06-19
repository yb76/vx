//
//-----------------------------------------------------------------------------
// PROJECT:			i-RIS
//
// FILE NAME:       crypto.c
//
// DATE CREATED:    20 March 2008
//
// AUTHOR:          Tareq Hafez
//
// DESCRIPTION:     This module supports cryptographics functions with the
//					Protect Server Gold HSM
//-----------------------------------------------------------------------------
//


//
//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------
//

//
// Standard include files.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// Project include files
//
#include "cryptoki.h"
#include "ctvdef.h"
#include "ctutil.h"
#include "genmacro.h"

//
// Local include files
//
#include "crypto.h"

//
//-----------------------------------------------------------------------------
// Constants & Macros
//-----------------------------------------------------------------------------
//
#define CHECK_CK_RV(rv, string)				\
	if (Crypto_Error(rv, string) != CKR_OK)	\
		return rv;

//
//-----------------------------------------------------------------------------
// Module variable definitions and initialisations.
//-----------------------------------------------------------------------------
//
static CK_SESSION_HANDLE hSession = CK_INVALID_HANDLE;

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : Crypto_Error
//
// DESCRIPTION:	Displays an error message if detected
//
// PARAMETERS:	None
//
// RETURNS:		CK_RV
//-------------------------------------------------------------------------------------------
//
static CK_RV Crypto_Error(CK_RV rv, CK_CHAR * string)
{
	if (rv != CKR_OK)
	{
		fprintf(stderr, "Crypto error occured : %s (0x%lx - %s)\n", string, rv, strError(rv));
		Crypto_Done();
	}

	return rv;
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : Crypto_Init
//
// DESCRIPTION:	Initiates a cryptographics session
//
// PARAMETERS:	None
//
// RETURNS:		CK_RV
//-------------------------------------------------------------------------------------------
//
CK_RV Crypto_Init(CK_SLOT_ID slotId)
{
    CK_RV rv;

	// Initialise the cryptoki API
    rv = C_Initialize(NULL);
	CHECK_CK_RV(rv, "Initialise");

	// Obtain a session so we can perform cryptoki operations */
	rv = C_OpenSession(slotId, CKF_RW_SESSION, NULL, NULL, &hSession);
	CHECK_CK_RV(rv, "Open Session");

	// Login so we can perform cryptoki operations
	rv = C_Login(hSession, CKU_USER, (CK_CHAR_PTR)"0000", (CK_SIZE) 4);
	CHECK_CK_RV(rv, "Login");

	return rv;
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : Crypto_Done
//
// DESCRIPTION:	Initiates a cryptographics session
//
// PARAMETERS:	None
//
// RETURNS:		CK_RV
//-------------------------------------------------------------------------------------------
//
CK_RV Crypto_Done(void)
{
    CK_RV rv;

	// Close the session if open
	if (hSession != CK_INVALID_HANDLE)
	{
		if ((rv = C_CloseSession(hSession)) != CKR_OK)
		{
			hSession = CK_INVALID_HANDLE;
			fprintf(stderr, "Crypto error occured : %s (0x%lx - %s)\n", "Close Session", rv, strError(rv));
			return rv;
		}
	}

	if ((rv = C_Finalize(NULL)) != CKR_OK)
	fprintf(stderr, "Crypto error occured : %s (0x%lx - %s)\n", "Finalise", rv, strError(rv));

	return rv;
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : Crypto_FindKey
//
// DESCRIPTION:	Finds a key given its label
//
// PARAMETERS:	keyName				<=	The key name
//				phKey				=>	The handle of the key will be stored here.
//
// RETURNS:		CK_RV
//-------------------------------------------------------------------------------------------
//
CK_RV Crypto_FindKey(CK_CHAR * keyName, CK_OBJECT_HANDLE * phKey)
{
	CK_RV rv;

	CK_COUNT objectCount = 0;
	CK_CHAR error[100];

	CK_ATTRIBUTE template[] =
	{
		{CKA_LABEL,				keyName,	strlen(keyName)}
	};

	// Prepare to find the requested key
	sprintf(error, "FindObjectsInit %s", keyName);
	rv = C_FindObjectsInit(hSession, template, 1);
	CHECK_CK_RV(rv, error);

	// Find the requested key
	sprintf(error, "FindObjects %s", keyName);
	rv = C_FindObjects(hSession, phKey, 1, &objectCount);
	CHECK_CK_RV(rv, error);

	C_FindObjectsFinal(hSession);

	if (objectCount != 1)
	{
		sprintf(error, "Key %s not found", keyName);
		CHECK_CK_RV(CKR_VENDOR_DEFINED, error);
	}

	return rv;
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : Crypto_KVCKey
//
// DESCRIPTION:	Calculates the KVC of a key
//
// PARAMETERS:	data	<=> The data to encrypt. It is updated on output
//
// RETURNS:		CK_RV
//-------------------------------------------------------------------------------------------
//
CK_RV Crypto_SKEncrypt(CK_CHAR_PTR data, CK_ULONG length)
{
	CK_RV rv;

	int len = length;
	CK_BYTE iv[8];
	CK_MECHANISM mechanism = {CKM_DES3_CBC, NULL, 0};
	CK_OBJECT_HANDLE hKey = CK_INVALID_HANDLE;

	// Prepare the IV
	memset(iv, 0, sizeof(iv));
	mechanism.pParameter = iv;
	mechanism.parameterLen = sizeof(iv);

	// Find the secret key
	rv = Crypto_FindKey("SK", &hKey);
	CHECK_CK_RV(rv, "SK not found");

	// Prepare for the encryption of SHA1
	rv = C_EncryptInit(hSession, &mechanism, hKey);
	CHECK_CK_RV(rv, "encryptInit for SHA1");

	// Go ahead and encrypt it
	rv = C_Encrypt(hSession, data, length, data, &len);
	CHECK_CK_RV(rv, "encrypting SHA1");

	// Return the result
	return rv;
}

