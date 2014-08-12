/*
**-----------------------------------------------------------------------------
** PROJECT:			iRIS
**
** FILE NAME:       security.c
**
** DATE CREATED:    14 January 2008
**
** AUTHOR:          Tareq Hafez
**
** DESCRIPTION:     This module supports security functions
**-----------------------------------------------------------------------------
*/

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
#include <eoslog.h>
//
// Project include files.
//
#include <auris.h>
#include <svc.h>
#include <svc_sec.h>

/*
** Local include files
*/
#include "utility.h"
#include "security.h"
#include "iris.h"
#include "irisfunc.h"
#include "emv.h"

/*
**-----------------------------------------------------------------------------
** Constants
**-----------------------------------------------------------------------------
*/
#define	C_SCRIPT_ID		7

// Script macro numbers
#define	M_WRITE			80
#define	M_WRITE_3DES	81
#define	M_ERASE			82
#define	M_ERASE_3DES	83
#define	M_RANDOM		84
#define	M_RANDOM_3DES	85
#define	M_KVC			86
#define	M_KVC_3DES		87

#define	M_COPY			90
#define	M_COPY_3DES		91
#define	M_MOVE			92
#define	M_MOVE_3DES		93
#define	M_XOR			94
#define	M_XOR_3DES		95
#define	M_XOR_3DES_DATA	96

#define	M_CLRIV			100
#define	M_SETIV			101

#define	M_ENC			110
#define	M_ENC_3DES		111
#define	M_ENCV			112
#define	M_ENCV_3DES		113
#define	M_DEC			114
#define	M_DEC_3DES		115
#define	M_DECV			116
#define	M_DECV_3DES		117
#define	M_OFB			118
#define	M_OFB_3DES		119

#define	M_INJECT		130
#define	M_INJECT_3DES	131
#define	M_3INJECT_3DES	132

#define	M_OWF			140
#define	M_OWF_3DES		141
#define	M_OWFV_3DES		142
#define	M_OWF_DATA		143
#define	M_OWF_3DES_DATA	144

#define	M_RSA_WRITE		150
#define	M_RSA_CRYPT		151
#define	M_RSA_INJECT	152
#define	M_RSA_3INJECT	153
#define	M_RSA_RSAINJECT	154
#define	M_RSA_WRAP_3DES	155
#define	M_3DES_RSAINJECT 159

#define	M_PIN			160
#define	M_PIN_3DES		161
#define	M_PINV			162
#define	M_PINV_3DES		163
#define	M_GETKEY_3DES	164
#define	M_PINV_3DES_EMV	166
#define	M_RSA_WRAP_3DES_ASN	167



#define	C_NO_OF_KEYS	256
#define	C_NO_OF_RSA		6
#define	C_APPNAME_MAX	50

/*
**-----------------------------------------------------------------------------
** Module variable definitions and initialisations.
**-----------------------------------------------------------------------------
*/
uchar iris_ktk3[17] = "\x00\xFA\x14\x4D\x82\xB1\x5A\x47\xE1\xB7\xB1\x53\xA0\xFF\x55\x6C";

static const char * fileName = "s1.dat";
static int handle = -1;
int cryptoHandle = -2;

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityInit
**
** DESCRIPTION:	Initialise the security application data
**
** PARAMETERS:	None
**
** RETURNS:		None
**-------------------------------------------------------------------------------------------
*/
void SecurityInit(void)
{
	// Open the crypto device
	cryptoHandle = open("/dev/crypto", 0);

	// Install the iRIS security script
	//iPS_InstallScript("f:irissec.vso");
	iPS_InstallScript("i:irissec.vso");

	// Open the application name key associations file
	// Initialise if file does not exist
	if ((handle = open(fileName, O_RDWR)) == -1)
	{
		int i;
		char appName[C_APPNAME_MAX];

		handle = open(fileName, O_CREAT | O_TRUNC | O_RDWR);

		memset(appName, 0, sizeof(appName));

		// Write an empty application name for all DES and RSA keys
		for (i = 0; i < (C_NO_OF_KEYS + C_NO_OF_RSA); i++)
			write(handle, appName, sizeof(appName));
	}
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityWriteKey
**
** DESCRIPTION:	Write a DES / 3-DES key
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**				key			<=	The key value
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityWriteKey(char * appName, uchar location, uchar keySize, uchar * key)
{
	uchar data[17];
	unsigned short outLen;

	// Make sure the application owns or can claim the key

	// Setup the input parameters
	data[0] = location;
	memcpy(&data[1], key, keySize);

	// Perform the instruction
	if (iPS_ExecuteScript(	C_SCRIPT_ID, keySize == 8?M_WRITE:M_WRITE_3DES, keySize + 1, data, 0, &outLen, data))
		return false;

	return true;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityEraseKey
**
** DESCRIPTION:	Erase a DES / 3-DES key
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityEraseKey(char * appName, uchar location, uchar keySize)
{
	uchar data[1];
	unsigned short outLen;


	// Setup the input parameters
	data[0] = location;

	// Perform the instruction
	if (iPS_ExecuteScript(	C_SCRIPT_ID, keySize == 8?M_ERASE:M_ERASE_3DES, sizeof(data), data, 0, &outLen, data))
		return false;

	lseek(handle, C_APPNAME_MAX * location, SEEK_SET);
	write(handle, "", 1);

	// Release the following key if a double length key
	if (keySize != 8)
	{
		lseek(handle, C_APPNAME_MAX * (location+1), SEEK_SET);
		write(handle, "", 1);
	}

	return true;
}


/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityGenerateKey
**
** DESCRIPTION:	Generate a random DES / 3-DES key
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityGenerateKey(char * appName, uchar location, uchar keySize)
{
	uchar data;
	unsigned short outLen;


	// Setup the input parameters
	data = location;

	// Perform the instruction
	if (iPS_ExecuteScript(	C_SCRIPT_ID, keySize == 8?M_RANDOM:M_RANDOM_3DES, sizeof(data), &data, 0, &outLen, &data))
		return false;

	return true;
}


/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityKVCKey
**
** DESCRIPTION:	Get the KVC (Key Verification Check) of a DES / 3-DES key
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**				kvc			<=	Place to store the KVC
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityKVCKey(char * appName, uchar location, uchar keySize, uchar * kvc)
{
	uchar data;
	unsigned short outLen;
	short mode = 0;

	// Make sure the application owns or can claim the key

	// Setup the input parameters
	data = location;

	if(keySize==8) mode = M_KVC;
	else mode = M_KVC_3DES;

	// Perform the instruction
	if (iPS_ExecuteScript(	C_SCRIPT_ID, mode, sizeof(data), &data, 3, &outLen, kvc)) {
		return false;
	}

	return true;
}


/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityCopyKey
**
** DESCRIPTION:	Copy a KEK type DES / 3-DES key into another KEK type key.
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**				to			<=	The destination location index
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityCopyKey(char * appName, uchar location, uchar keySize, uchar to)
{
	uchar data[2];
	unsigned short outLen;


	// Setup the input parameters
	data[0] = location;
	data[1] = to;

	// Perform the instruction
	if (iPS_ExecuteScript(	C_SCRIPT_ID, keySize == 8?M_COPY:M_COPY_3DES, sizeof(data), data, 0, &outLen, data))
		return false;

	return true;
}


/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityMoveKey
**
** DESCRIPTION:	Move a KEK type DES / 3-DES key into another KEK type key.
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**				to			<=	The destination location index
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityMoveKey(char * appName, uchar location, uchar keySize, uchar to)
{
	uchar data[2];
	unsigned short outLen;


	// Setup the input parameters
	data[0] = location;
	data[1] = to;

	// Perform the instruction
	if (iPS_ExecuteScript(	C_SCRIPT_ID, keySize == 8?M_MOVE:M_MOVE_3DES, sizeof(data), data, 0, &outLen, data))
		return false;

	return true;
}


/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityXorKey
**
** DESCRIPTION:	XOR a KEK type DES / 3-DES key with another of the same type and store the result into another KEK type key.
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**				with		<=	The other location index to XOR it with
**				to			<=	The destination location index
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityXorKey(char * appName, uchar location, uchar keySize, uchar with, uchar to)
{
	uchar data[3];
	unsigned short outLen;


	// Setup the input parameters
	data[0] = location;
	data[1] = with;
	data[2] = to;

	// Perform the instruction
	if (iPS_ExecuteScript(	C_SCRIPT_ID, keySize == 8?M_XOR:M_XOR_3DES, sizeof(data), data, 0, &outLen, data))
		return false;

	return true;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityXorKeyWithData
**
** DESCRIPTION:	XOR a KEK type DES / 3-DES key with another of the same type and store the result into another KEK type key.
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**				data		<=	The data to be xored (8bytes)
**				to			<=	The destination location index
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityXorKeyWithData(char * appName, uchar location, uchar keySize, uchar *xdata, uchar to)
{
	uchar data[10];
	unsigned short outLen;

	if( xdata == NULL ) return false;

	// Setup the input parameters
	data[0] = location;
	data[1] = to;
	memcpy( &data[2], xdata, 8 );

	// Perform the instruction
	if ( iPS_ExecuteScript(	C_SCRIPT_ID, M_XOR_3DES_DATA, sizeof(data), data, 0, &outLen, data))
		return false;

	return true;
}


/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecuritySetIV
**
** DESCRIPTION:	Clears/Sets the Initial Vector used for subsequent CBC encryption/decryption operations
**
** PARAMETERS:	iv			<=	The Initial Vector 8-byte array. If NULL, then clear the IV.
**
** RETURNS:		None
**-------------------------------------------------------------------------------------------
*/
bool SecuritySetIV(uchar * iv)
{
	unsigned short outLen;

	// Clear the IV if the parameter is NULL
	if (iv == NULL)
	{
		if (iPS_ExecuteScript(	C_SCRIPT_ID, M_CLRIV, 0, NULL, 0, &outLen, NULL))
			return false;
	}

	// Set the IV otherwise
	else
	{
		if (iPS_ExecuteScript(	C_SCRIPT_ID, M_SETIV, 8, iv, 0, &outLen, NULL))
			return false;
	}

	return true;
}


bool getset_ivmode( int mode, bool iv_input)
{
	static bool iv = true;
	if(mode == 1) iv = iv_input;
	return iv;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityCrypt
**
** DESCRIPTION:	CBC or OFB Encrypt/Decrypt a data block.
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**				eDataSize	<=	The data block size.
**				eData		<=	The data.
**				decrypt		<=	If true, decrypt. Otherwise, encrypt.
**				ofb			<=	If true, OFB encryption/decryption. Otherwise, CBC encryption/decryption.
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityCrypt(char * appName, uchar location, uchar keySize, int eDataSize, uchar * eData, bool decrypt, bool ofb)
{
	int i;
	uchar data[9];
	unsigned short outLen;
	bool ivmode = true;
	bool ret = true;


	// Setup the input parameters
	data[0] = location;

	//ivmode = getset_ivmode( 0,true );
	for (i = 0; i < eDataSize; i += 8)
	{
		// Place the encrypted data in the input parameters buffer
		memset(&data[1], 0, 8);
		memcpy(&data[1], &eData[i], (eDataSize-i) < 8? (eDataSize-i):8);

		// Perform the instruction
		if ( iPS_ExecuteScript( C_SCRIPT_ID, ofb?(keySize == 8?M_OFB:M_OFB_3DES):(decrypt?(keySize == 8?M_DEC:M_DEC_3DES):(keySize == 8?M_ENC:M_ENC_3DES)), 9, data, 8, &outLen, &data[1])) {
			ret = false;
		} else
			memcpy(&eData[i], &data[1], (eDataSize-i) < 8? (eDataSize-i):8);
	}

	//if(!ivmode) getset_ivmode(1,true);
	return ret;
}


/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : _getVariant
**
** DESCRIPTION:	Transforms a single variant byte into an array of bytes. The number of bytes
**				is equal to the key size
**
** PARAMETERS:	data		<=	Buffer where the full variant is stored
**				index		<=	Starting offset within the buffer
**				keySize		<=	The key size (8 for single DES keys or 16 for double length 3-DES keys)
**				variant		<=	Points to the initial 2 variant bytes used to vary the keys
**
** RETURNS:		An index pointing to a location within "data" just after the full variant.
**-------------------------------------------------------------------------------------------
*/
int _getVariant(uchar * data, uchar keySize, uchar * variant)
{
	int i = 0;

	while (i < keySize)
	{
		data[i++] = variant[0];
		data[i++] = variant[1];
	}

	return i;
}


/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityCryptWithVariant
**
** DESCRIPTION:	CBC Encrypt/Decrypt a data block with a variant
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**				dataSize	<=	The data block size.
**				data		<=	The data. This must be right filled with ZEROs if the dataSize is not a multiple of 8 bytes.
**				variant		<=	The variant initial 2 bytes.
**				decrypt		<=	If true, decrypt. Otherwise, encrypt.
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityCryptWithVariant(char * appName, uchar location, uchar keySize, int eDataSize, uchar * eData, uchar * variant, bool decrypt)
{
	int i;
	uchar data[25];
	int inLen;
	unsigned short outLen;


	// Setup the input parameters
	data[0] = location;
	//inLen = 9 + _getVariant(&data[9], keySize, variant?variant:"\x00\x00");
	inLen = 9 + keySize;
	memset(&data[9], '\0', keySize);
	if( variant ) memcpy(&data[9], variant,keySize);

	for (i = 0; i < eDataSize; i += 8)
	{
		// Place the encrypted data in the input parameters buffer
		memset(&data[1], 0, 8);
		memcpy(&data[1], &eData[i], (eDataSize-i) < 8? (eDataSize-i):8);

		// Perform the instruction
		if (iPS_ExecuteScript( C_SCRIPT_ID, decrypt?(keySize == 8?M_DECV:M_DECV_3DES):(keySize == 8?M_ENCV:M_ENCV_3DES), inLen, data, 8, &outLen, &data[1]))
			return false;

		// Store the encrypted result back
		memcpy(&eData[i], &data[1], (eDataSize-i) < 8? (eDataSize-i):8);
	}

	return true;
}




/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityMAB
**
** DESCRIPTION:	Calculate a MAB block
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**				eDataSize	<=	The data block size. This does not have to be a multiple of 8 bytes
**				eData		<=	The data.
**				mab			=>	An 8-byte array buffer containing the MAB of the data block on output if successful
**
** RETURNS:		TRUE if successful and mab will contain the MAB of the data block
**				FALSE if not. MAB array remains untouched.
**-------------------------------------------------------------------------------------------
*/
bool SecurityMAB(char * appName, uchar location, uchar keySize, int eDataSize, uchar * eData, uchar * variant, uchar * mab)
{
	int i;
	uchar data[25];
	uint inLen = 9;
	unsigned short outLen;
	short macroid = 0;

	// Setup the input parameters
	data[0] = location;
	if (variant) {
		inLen += _getVariant(&data[9], keySize, variant?variant:"\x00\x00");
		if( keySize == 8) macroid = M_ENCV;
		else macroid = M_ENCV_3DES;
	} else {
		if( keySize == 8) macroid = M_ENC;
		else macroid = M_ENC_3DES;
	}

	for (i = 0; i < eDataSize; i += 8)
		{
		  memset(&data[1], 0, 8);
		  memcpy(&data[1], &eData[i], (eDataSize-i) < 8? (eDataSize-i):8);
		// Perform the instruction
		  if ( iPS_ExecuteScript( C_SCRIPT_ID, macroid, inLen, data, 8, &outLen, &data[1])) {
			return false;
		  }
		}

	// Return the MAB value
	memcpy(mab, &data[1], 8);

	return true;
}



/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityInjectKey
**
** DESCRIPTION:	Inject a KEK encrypted key
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**				to			<=	The injected key location
**				eKeySize	<=	The encrypted key size (8 = single DES, 16 = double length 3-DES key)
**				eKey		<=	Encrypted key
**				variant		<=	The variant initial 2 bytes.
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityInjectKey(char * appName, uchar location, uchar keySize, char * appName2, uchar to, uchar eKeySize, uchar * eKey, uchar * variant)
{
	uchar data[34];
	unsigned short outLen;

	// Setup the input parameters
	data[0] = location;
	data[1] = to;
	memcpy(&data[2], eKey, eKeySize);
	outLen = eKeySize + 2 + _getVariant(&data[eKeySize+2], keySize, variant?variant:"\x00\x00");


	// Perform the instruction
	if ( iPS_ExecuteScript( C_SCRIPT_ID, keySize == 8?M_INJECT:(eKeySize == 8?M_INJECT_3DES:M_3INJECT_3DES), outLen, data, 0, &outLen, NULL)) {
		return false;
	}

	return true;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityOWFKey
**
** DESCRIPTION:	OWF a KEK encrypted key
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**				eKeySize	<=	The encrypted key size (8 = single DES, 16 = double length 3-DES key)
**				eKey		<=	Encrypted key
**				variant		<=	If TRUE, then the key is first varied using the PPASN key value.
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityOWFKey(char * appName, uchar location, uchar keySize, uchar to, uchar ppasn, bool variant)
{
	uchar data[3];
	unsigned short outLen;

	// Setup the input parameters
	data[0] = location;
	data[1] = to;
	data[2] = ppasn;

	// Perform the instruction
	if ( iPS_ExecuteScript( C_SCRIPT_ID, keySize == 8?M_OWF:(variant?M_OWFV_3DES:M_OWF_3DES), 3, data, 0, &outLen, NULL)) {
		return false;
	}

	return true;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityOWFKeyWithData
**
** DESCRIPTION:	OWF a KEK encrypted key
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**				to			<=	Encrypted key
**				variant		<=	If TRUE, then the key is first varied using the PPASN key value.
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityOWFKeyWithData(char * appName, uchar location, uchar keySize, uchar to, uchar * variant)
{
	uchar data[18];
	unsigned short outLen;


	// Setup the input parameters
	data[0] = location;
	data[1] = to;
	if (keySize == 8)
		memcpy(&data[2], variant, 8);
	else
		memcpy(&data[2], variant, 16);

	// Perform the instruction
	if (iPS_ExecuteScript( C_SCRIPT_ID, keySize == 8?M_OWF_DATA:M_OWF_3DES_DATA, keySize == 8?10:18, data, 0, &outLen, NULL))
		return false;

	return true;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : Security3DESWriteRSA
**
** DESCRIPTION:	Inject a 3DES encrypted RSA key
**
** PARAMETERS:	appName		<=	The application name				
**				location	<=	The 3DES key location index
**				appName2	<=	Owner of the stored RSA key
**				to			<=	RSA key location
**				blockLen	<=	Length of the RSA block to inject
**				block		<=	The encrypted RSA block
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool Security3DESWriteRSA(char * appName, uchar location, char * appName2, uchar to, int blockLen, uchar * block)
{
	uchar data[521];
	unsigned short outLen;


	// Setup the input parameters
	memset(data, 0, sizeof(data));	// Set all to ZERO including the first bytes [2-8] used for EMV.
	data[0] = location;
	data[1] = to;
	memcpy(&data[9], block, blockLen);


	// Perform the instruction
	if (iPS_ExecuteScript( C_SCRIPT_ID, M_3DES_RSAINJECT, 521, data, 0, &outLen, NULL))
		return false;

	return true;
}


/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityWriteRSA
**
** DESCRIPTION:	Write an RSA key
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				modLen		<=	Modulus length in bytes. 0 = 2048 bits.
**				modulus		<=	Modulus data
**				expLen		<=	Exponent length in bytes. For location 0-3, a maximum of 3 bytes apply.
**				exponent	<=	Exponent data
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityWriteRSA(char * appName, uchar location, int blockLen, uchar * block)
{
	uchar data[512];
	unsigned short outLen;


	// Setup the input parameters
	memset(data, 0, sizeof(data));	// Set all to ZERO including the first 7 bytes used for EMV.
	data[0] = location;
	memcpy(&data[8], block, blockLen);

	// Perform the instruction
	if (iPS_ExecuteScript( C_SCRIPT_ID, M_RSA_WRITE, 512, data, 0, &outLen, NULL))
		return false;

	return true;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityClearRSA
**
** DESCRIPTION:	Clears the ownership of the RSA block for the owner or iRIS
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityClearRSA(char * appName, uchar location)
{

	lseek(handle, C_APPNAME_MAX * (location + C_NO_OF_KEYS), SEEK_SET);
	write(handle, "", 1);

	return true;
}


/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityCryptRSA
**
** DESCRIPTION:	Sign/Decrypt an RSA block
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				eDataSize	<=	Size of the data block to crypt
**				eData		<=	Encrypted/Clear data block
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityCryptRSA(char * appName, uchar location, int eDataSize, uchar * eData)
{
	uchar data[257];
	unsigned short outLen;


	// Setup the input parameters
	memset(data, 0, sizeof(data));	// Set all to ZEROs
	data[0] = location;
	memcpy(&data[1], eData, eDataSize);

	// Perform the instruction
	if ( iPS_ExecuteScript( C_SCRIPT_ID, M_RSA_CRYPT, sizeof(data), data, sizeof(data), &outLen, data)) {
		return false;
	}

	memcpy(eData, data, eDataSize);
	return true;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityRSAInjectKey
**
** DESCRIPTION:	Inject a DES / 3DES key stored within an RSA encrypted block
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				to			<=	The injected key location
**				eKeySize	<=	The encrypted key size (8 = single DES, 16 = double length 3-DES key)
**				eDataSize	<=	Size of the data block to decrypt
**				eData		<=	Encrypted data block
**
** RETURNS:		TRUE if successful. The keys will not be returned but the rest of the block will
**				in case the application wants to check a signature, etc. The application can alway
**				erase the key if it does not like the signature.
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityRSAInjectKey(char * appName, uchar location, char * appName2, uchar to, uchar eKeySize, int eDataSize, uchar * eData)
{
	uchar data[258];
	unsigned short outLen;


	// Setup the input parameters
	data[0] = location;
	data[1] = to;
	memcpy(&data[2], eData, eDataSize);

	// Perform the instruction
	if (iPS_ExecuteScript( C_SCRIPT_ID, eKeySize == 8?M_RSA_INJECT:M_RSA_3INJECT, eDataSize+2, data, eDataSize, &outLen, eData))
		return false;

	return true;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityRSAInjectRSA
**
** DESCRIPTION:	Inject a RSA key stored within an RSA encrypted block
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The RSA key location index
**				to			<=	The injected RSA key location
**				eDataSize	<=	Size of the data block to decrypt
**				eData		<=	Encrypted data block
**
** RETURNS:		TRUE if successful.
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityRSAInjectRSA(char * appName, uchar location, char * appName2, uchar to, int eDataSize, uchar * eData)
{
	uchar data[512];
	unsigned short outLen;


	// Setup the input parameters
	memset(data, 0, sizeof(data));	// Set all to ZERO including the first 7 bytes (ie. 2->8) used for EMV.
	data[0] = location;
	data[1] = to;
	memcpy(&data[9], eData, eDataSize);

	// Perform the instruction
	if (iPS_ExecuteScript( C_SCRIPT_ID, M_RSA_RSAINJECT, eDataSize+9, data, 0, &outLen, NULL))
		return false;

	return true;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityRSAWrap3Des
**
** DESCRIPTION:	Inject a RSA key stored within an RSA encrypted block
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The RSA key location index
**				to			<=	The injected RSA key location
**				eDataSize	<=	Size of the data block to decrypt
**				eData		<=	Encrypted data block
**
** RETURNS:		TRUE if successful.
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityRSAWrap3Des(char * appName, uchar location, uchar to, int *eDataSize, uchar * eData)
{
	uchar data[300];
	unsigned short outLen;
	uchar hsm[10];

	*eDataSize = 0 ;
	// Make sure the application owns the RSA location and owns or can claim the destination location

	// Setup the input parameters
	memset(data, 0, sizeof(data));
	data[0] = location;
	data[1] = to;

	memcpy( &data[2],"\x00\x02", 2 );
	memset( &data[4],'\x34', 109 );

	memset(hsm,0,sizeof(hsm));
	get_env("HSM", hsm, sizeof(hsm));
	if(strncmp(hsm,"THALES",6) == 0)
	{
		char asnstr[50];
		char hex[17];
		asnstr[0] = '\x30';
		asnstr[1] = 36;
		asnstr[2] = '\x04';
		asnstr[3] = '\x10';
		SecurityGetKey(currentObjectGroup, (uchar) data[1], hex);
		memcpy( &asnstr[4], hex,16);
		asnstr[20] = '\x04';
		asnstr[21] = '\x10';
		memset( &asnstr[22], 0x00,16);

		memcpy(&data[4+88], asnstr, 38);
		data[4+88-1] = 0x00;

	}
	// Perform the instruction
	if ( iPS_ExecuteScript( C_SCRIPT_ID, (strncmp(hsm,"THALES",6) == 0)?M_RSA_WRAP_3DES_ASN:M_RSA_WRAP_3DES, 128+2, data, 128, &outLen, data)) {
		return false;
	}

	//data..0   1   2   3   4  ...  240 241    242 - 257
	//      rsa des 00  02  33 33 33 33 00     00... 00
	// Perform the instruction

	memcpy( eData, data, outLen );
	*eDataSize = outLen;
	return true;
}


/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityPINBlock
**
** DESCRIPTION:	Get the PIN Block
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**				pan			<=	The PAN as an ASCII string
**				ePinBlock	=>	The PIN block will be stored here if successful.
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityPINBlock(char * appName, uchar location, uchar keySize, char * pan, uchar * ePinBlock)
{
	uchar data[9];
	unsigned short outLen;
	char myPan[20];
	int iret,index, j;
	char stmp[20];

	// Only get the last 12 digits of the pan excluding the last check digit.
	for (j = 0, index = strlen(pan) - 13; index < (int) (strlen(pan)-1); j++, index++)
	{
		if (index < 0) myPan[j] = '0';
		else myPan[j] = pan[index];
	}
	myPan[j] = '\0';
	sprintf( stmp, "0000%s",myPan);
	strcpy( myPan, stmp);

	// Setup the input parameters
	data[0] = location;
	UtilStringToHex(myPan, 16, &data[1]);

	// Perform the instruction
	if (iret = iPS_ExecuteScript( C_SCRIPT_ID, keySize == 8?M_PIN:M_PIN_3DES, sizeof(data), data, 8, &outLen, ePinBlock)) {
		return false;
	}

	return true;
}


/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityPINBlockWithVariant
**
** DESCRIPTION:	Get the PIN Block after varying KPP to a KPE
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**				pan			<=	The PAN as an ASCII string
**				stan		<=	The transaction stan number - First part of the variant.
**				amount		<=	The transaction amount - Second part of the variant.
**				ePinBlock	=>	The PIN block will be stored here if successful.
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityPINBlockWithVariant(char * appName, uchar location, uchar keySize, char * pan, char * stan, char * amount, bool emv,uchar * ePinBlock)
{
	uchar data[33];
	unsigned short outLen;
	char myPan[20];
	int index, j;
	int inlen = 25;
	unsigned char mode = 0;

	// Only get the last 12 digits of the pan excluding the last check digit.
	for (j = 0, index = strlen(pan) - 13; index < (int) (strlen(pan)-1); j++, index++)
	{
		if (index < 0) myPan[j] = '0';
		else myPan[j] = pan[index];
	}
	myPan[j] = '\0';

	// Setup the input parameters
	memset(data, 0, sizeof(data));
	data[0] = location;
	UtilStringToHex(myPan, 16, &data[1]);
	UtilStringToHex(stan, 6, &data[9]);
	UtilStringToHex(amount, 16, &data[17]);

	// Perform the instruction
#ifdef __EMV
	if(false) {
		char pin[10];
		char stmp[17];
		int GetSetEmvPin( unsigned short action,unsigned char *pin);

		inlen += 8;
		mode = M_PINV_3DES_EMV;
		GetSetEmvPin(0,(unsigned char*)pin);
		if(strlen(pin)==0) return false;
		data[25] = strlen(pin);

		memset(stmp,0,sizeof(stmp));	
		memset(stmp,'F',14);	
		memcpy( stmp,pin,strlen(pin));
		UtilStringToHex(stmp, 14,&data[26] );
	}
	else 
#endif

	if(keySize==8) mode = M_PINV;
	else {
		mode = M_PINV_3DES;
	}

	if ( iPS_ExecuteScript( C_SCRIPT_ID, mode, inlen , data, 8, &outLen, ePinBlock)) {
		return false;
	}

	return true;
}

/*
**-------------------------------------------------------------------------------------------
**
**-------------------------------------------------------------------------------------------
*/
bool SecurityPINBlockCba(uchar* location_1, uchar *location_2,char * pan, bool emv,uchar * ePinBlock)
{
	bool ok = true;
	unsigned char data[20];
	int iret = 0;

	memset(data,0,sizeof(data));
	// Perform the instruction
#ifdef __EMV
	if (!gEmv.onlinepin ) //if(emv)
	{
		char myPan[20];
		int index, j;
		char pin[13];
		char stmp[17];
		char stmp1[17];
		char stmp2[17];
		unsigned short outLen = 0;

		memset(stmp, 0, sizeof(stmp));
		memset(stmp1, 0, sizeof(stmp1));
		memset(stmp2, 0, sizeof(stmp2));
		memset(pin, 0, sizeof(pin));

		// Only get the last 12 digits of the pan excluding the last check digit.
		for (j = 0, index = strlen(pan) - 13; index < (int) (strlen(pan)-1); j++, index++)
		{
			if (index < 0) myPan[j] = '0';
			else myPan[j] = pan[index];
		}
		myPan[j] = '\0';
		sprintf( stmp, "0000%s",myPan);
		strcpy( myPan, stmp);
		UtilStringToHex(myPan, 16, (unsigned char *)stmp1);

		GetSetEmvPin(0,(unsigned char*)pin);

		memset(stmp,'F',16);	
		if(strlen(pin)) {
			memcpy( stmp+2,pin,strlen(pin));
		}
		UtilStringToHex(stmp, 16,(unsigned char *)stmp2 );
		stmp2[0] = strlen(pin);
		if(strlen(pin) ==0) { memcpy( stmp2,"\x80\xff\xff\xff\xff\xff\xff\xff",8); }

		for( j = 0 ; j < 8 ; j ++ ) {
			stmp1[j] ^=  stmp2[j];
		}

		memcpy( data+1, stmp1, 8);
		data[0] = (unsigned char)atoi((const char*)location_1);

		SecuritySetIV(NULL);
		if ( iret = iPS_ExecuteScript( C_SCRIPT_ID, M_ENC_3DES, 9, data, 8, &outLen, &data[1]))  ok = false;

		if(ok) {
			data[0] = (unsigned char)atoi((const char*)location_2);
			SecuritySetIV(NULL);
			if ( iPS_ExecuteScript( C_SCRIPT_ID, M_ENC_3DES, 9, data, 8, &outLen, &data[1]))  ok = false;
			if(ok) UtilHexToString((const char*)&data[1], 8, (char *)ePinBlock);
		}
	}
	else
#endif
	{
		int iret = 0;
		unsigned char epinb1[9] ;
		unsigned short outLen;

		memset(data,0,sizeof(data));
		memset( epinb1,0,9);
		SecuritySetIV(NULL);
		ok = SecurityPINBlock("iris", (unsigned char) atoi((const char *)location_1), 16, pan, epinb1);
		if( ok ) {
			memcpy( data+1, epinb1, 8 );
			LOG_PRINTFF(0x00000001L,"pinblock step1 = [%02x%02x%02x%02x%02x%02x%02x%02x]",data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);
			data[0] = (unsigned char)atoi((const char*)location_2);
			SecuritySetIV(NULL);
			if ( iret = iPS_ExecuteScript( C_SCRIPT_ID, M_ENC_3DES, 9, data, 8, &outLen, &data[1]))  ok = false;
			if(ok) UtilHexToString((const char *)&data[1],8,(char *)ePinBlock);
			LOG_PRINTFF(0x00000001L,"pinblock step2 = [%02x%02x%02x%02x%02x%02x%02x%02x]",data[1],data[2],data[3],data[4],data[5],data[6],data[7],data[8]);
		}
	}


	return ok;
}

/*
**-------------------------------------------------------------------------------------------
** FUNCTION   : SecurityGetKey
**
** DESCRIPTION:	Get a random DES / 3-DES key
**
** PARAMETERS:	appName		<=	The application name
**				location	<=	The key location index
**				keySize		<=	The key size (8 = single DES, 16 = double length 3-DES key)
**
** RETURNS:		TRUE if successful
**				FALSE if not
**-------------------------------------------------------------------------------------------
*/
bool SecurityGetKey(char * appName, uchar location, uchar* value)
{
	uchar data;
	unsigned short outLen;
	uchar temp[17];


	// Setup the input parameters
	data = location;

	// Perform the instruction
	if (iPS_ExecuteScript(	C_SCRIPT_ID, M_GETKEY_3DES, sizeof(data), &data, 16, &outLen, temp))
		return false;

	memcpy( value, temp, 16 );

	return true;
}
