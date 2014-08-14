//
//-----------------------------------------------------------------------------
// PROJECT:			iRIS
//
// FILE NAME:       iriscrypt.c
//
// DATE CREATED:    5 March 2008
//
// AUTHOR:          Tareq Hafez
//
// DESCRIPTION:     This module supports security functions
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
// Project include files.
//
#include <auris.h>
#include <svc.h>

//
// Local include files
//
#include "alloc.h"
#include "security.h"
#include "utility.h"
#include "iris.h"
#include "irisfunc.h"

//
//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------
//

//
//-----------------------------------------------------------------------------
// Module variable definitions and initialisations.
//-----------------------------------------------------------------------------
//
//uchar iris_ktk2[17] = "\xDF\x45\xC7\x34\x8F\x51\x76\x6D\x9D\x20\xE5\x2C\x0D\x67\x40\xA7";


//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
// CRYPTOGRAPHY FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()DES_STORE
//
// DESCRIPTION:	Stores a DES or 3DES key depending on the key size
//
// PARAMETERS:	None
//
// RETURNS:		
//-------------------------------------------------------------------------------------------
//

bool __des_store(const char *data, const char* keySize, const char* key)
{
	uchar * hex;
	bool result = false;

	if (data && key && keySize)
	{
		int size = strlen(data) / 2 + 1;

		// Change the ASCII data to hex digits.
		hex = my_malloc(size);
		size = UtilStringToHex(data, strlen(data), hex);

		// Perform the key storage
		result = SecurityWriteKey(currentObjectGroup, (uchar) atoi(key), (uchar) atoi(keySize), hex);

		my_free(hex);
	}

	return (result) ;
}


//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()DES_RANDOM
//
// DESCRIPTION:	Generate a random DES or 3DES key depending on the key size
//
// PARAMETERS:	None
//
// RETURNS:		
//-------------------------------------------------------------------------------------------
//
bool __des_random(const char* keySize, const char *key, char *value)
{
	bool result = false;
	char temp[33];
	if (key && keySize) result = SecurityGenerateKey(currentObjectGroup, (uchar) atoi(key), (uchar) atoi(keySize));
	if (result) {
		uchar hex[17];
		result = SecurityGetKey(currentObjectGroup, (uchar) atoi(key), hex);
		UtilHexToString( (const char *)hex, 16, temp);
		temp[32] = '\0';
		strcpy( value, temp);
	}

	return(result);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()IV_SET
//
// DESCRIPTION:	Sets the IV for the next Crypt or MAC operation
//
// PARAMETERS:	None
//
// RETURNS:		
//-------------------------------------------------------------------------------------------
//
bool __iv_set(const char* iv)
{
	bool result = false;

	uchar * hex;

	if (iv)
	{
		int size = strlen(iv) / 2 + 1;

		// Change the ASCII data to hex digits.
		hex = my_malloc(size);
		size = UtilStringToHex(iv, strlen(iv), hex);

		result = SecuritySetIV(hex);

		my_free(hex);
	} else {
		result = SecuritySetIV(NULL);
	}

	return(result);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()ENC / ()DEC / )OFB
//
// DESCRIPTION:	Perform encryption
//
// PARAMETERS:	None
//
// RETURNS:		
//-------------------------------------------------------------------------------------------
//

char* __crypt(const char* data, const char *keySize, const char* key, const char *variant, bool decrypt, bool ofb)
{
	uchar * hex;
	char * string = NULL;
	bool ok = false;
	int size = 0;

	//SecuritySetIV(NULL);
	if (data && key )
	{
		size = strlen(data) / 2 + 1;
		if (size/8*8 != size) size = size/8*8+8;

		// Change the ASCII data to hex digits.
		hex = my_calloc(size);
		size = UtilStringToHex(data, strlen(data), hex);

		// Perform the encryption
		if (variant && ( strlen(variant) == 4 ))
		{
			uchar hexVariant[17];
			uchar stmp[17];

			// Change the ASCII variant to hex digits.
			memset(hexVariant,0,sizeof(hexVariant));
			memset(stmp,0,sizeof(stmp));
			UtilStringToHex(variant, strlen(variant), hexVariant);

			if(strlen(variant) == 4) {
				int keylen = atoi(keySize);
				_getVariant(stmp,keylen,hexVariant);
				memcpy( hexVariant,stmp,keylen);
			}

			ok = SecurityCryptWithVariant(currentObjectGroup, (uchar) atoi(key), (uchar) ((keySize && keySize[0] == '8')?8:16), size, hex, hexVariant, decrypt);
		}
		else
			ok = SecurityCrypt(currentObjectGroup, (uchar) atoi(key), (uchar) ((keySize && keySize[0] == '8')?8:16), size, hex, decrypt, ofb);

		// Change back to ASCII data
	}
	if(ok) {
		string = my_malloc(size*2 + 1);
		UtilHexToString((const char *)hex, size, string);
		my_free(hex);
		return(string);
	}
	else
	{
		// Send the data back as is if an error occurs
		UtilStrDup(&string, data);
		return(string);
	}

}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()MAC
//
// DESCRIPTION:	Perform MAC calculation
//
// PARAMETERS:	None
//
// RETURNS:		
//-------------------------------------------------------------------------------------------
//

char* __mac(const char* data, const char* variant, const char *key , int length, bool dataIsHex, char * mac_value)
{
	uchar * hex;
	uchar hexVariant[2];
	uchar hexMAC[8];
	char mac[17];
	static int maclen = 0;

	strcpy(mac, "0000000000000000");

	SecuritySetIV(NULL);
	if (data && variant && key)
	{
		int size ;

		if( dataIsHex ) {
			size = length;
			hex = my_malloc(size + 1);
			memcpy( hex, data , size);
		} else {
			size = strlen(data) / 2;
			hex = my_malloc(size + 1);
			size = UtilStringToHex(data, strlen(data), hex);
		}

		// Change the ASCII variant to hex digits.
		if( strlen(variant) == 4 ) {
			UtilStringToHex(variant, 4, hexVariant);
		}

		// Perform the MAC calculation
		SecurityMAB(currentObjectGroup, (uchar) atoi(key), 16, size, hex, strlen(variant) == 4?hexVariant:NULL, hexMAC);

		my_free(hex);

		// Clear the last 4 bytes since this is a MAC
		//WBC  4bytes len
		//CBA  8bytes len
		if (maclen == 0)
		{
			unsigned int objlength = 0;
			char *objdata = NULL;
			char *tagvalue = NULL;
			
			objdata = IRIS_GetObjectData( "IRIS_CFG",&objlength);
			if(objdata) {
				tagvalue = IRIS_GetObjectTagValue( objdata,"MAC_LEN");
			}
		
			if(tagvalue && *tagvalue == '4') maclen = 4;
			else maclen = 8;
			UtilStrDup(&tagvalue,NULL);
			UtilStrDup(&objdata,NULL);				
		}
		if (maclen==4)
			hexMAC[4] = hexMAC[5] = hexMAC[6] = hexMAC[7] = 0;

		// Change back to ASCII data
		UtilHexToString((const char*)hexMAC, sizeof(hexMAC), mac);
	}

	// Push the result onto the stack and clean up
	strcpy(mac_value, mac);
	return(mac_value);
}


//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()KVC
//
// DESCRIPTION:	Perform KVC Check
//
// PARAMETERS:	None
//
// RETURNS:		
//-------------------------------------------------------------------------------------------
//

void __kvc(const char* keySize, const char * key, char *kvc_)
{
	uchar hexKVC[3];
	char kvc[7]="000000";

	if (key)
	{
		// Perform the encryption
		if (SecurityKVCKey(currentObjectGroup, (uchar) atoi(key), (uchar) ((keySize && atoi(keySize) == 8)?8:16), hexKVC))
			// Change to ASCII hex data
			UtilHexToString((const char *)hexKVC, sizeof(hexKVC), kvc);
	}
	strcpy(kvc_,kvc);

}


//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()OWF
//
// DESCRIPTION:	Perform One Way Function
//
// PARAMETERS:	None
//
// RETURNS:		
//-------------------------------------------------------------------------------------------
//

bool __owf(const char* ppasn, const char* to, const char* from, bool variant, const char *data )
{
	bool result = false;

	// Perform the encryption
	if ( data == NULL && ppasn && to && from )
		result = SecurityOWFKey(currentObjectGroup, (uchar) atoi(from), 16, (uchar) atoi(to), (uchar) atoi(ppasn), variant);
	else if ( ppasn == NULL && data && to && from )
		result = SecurityOWFKeyWithData(currentObjectGroup, (uchar) atoi(from), 16, (uchar) atoi(to), (uchar*)data);

	return(result);
}


//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()DERIVE_3DES, ()DERIVE_DES
//
// DESCRIPTION:	Perform 3DES key derivation
//
// PARAMETERS:	None
//
// RETURNS:		
//-------------------------------------------------------------------------------------------
//

bool __derive_3deskey(const char* data, const char* variant, const char* key, const char* kek)
{
	uchar * hex;
	uchar hexVariant[2];
	bool result = false;

	if (data && variant && key  && kek )
	{
		int size = strlen(data) / 2;

		// Change the ASCII data to hex (binary).
		hex = my_malloc(size + 1);
		size = UtilStringToHex(data, strlen(data), hex);

		// Change the ASCII variant to hex digits.
		if(variant) UtilStringToHex(variant, 4, hexVariant);

		// The application name of the new key must match the current group, or the current group must be iRIS
		result = SecurityInjectKey(currentObjectGroup, (uchar) atoi(kek), 16, currentObjectGroup, (uchar) atoi(key), 16, hex, (variant&&strlen(variant)==4)?hexVariant:NULL);

		// Clean up
		my_free(hex);
	}

	return(result);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()RSA_STORE
//
// DESCRIPTION:	Perform RSA Storage
//
// PARAMETERS:	None
//
// RETURNS:		
//-------------------------------------------------------------------------------------------
//
bool __rsa_store(const char *data, const char* rsa)
{
	uchar * hex;
	bool result = false;

	// char *data
	// Format:	FIRST BYTE =  modulus length in bytes (i.e. 2 = 16 bits, 0 = 2096 bits, 128 = 1024 bits).
	//			SECOND BYTE = exponent length in bytes (i.e. 2 = 16 bits, 0 = 2096 bits, 128 = 1024 bits).
	//			then..........modulus bytes.
	//				..........exponent bytes.
	// If RSA location (ie rsa) is 0 to 3, then modulus length has maximum value of 252 bytes (= 2016 bits) and exponent length has maximum of 3 bytes (= 24 bits).

	if (data && rsa)
	{
		int size = strlen(data) / 2;

		// Change the ASCII data to hex (binary).
		hex = my_malloc(size + 1);
		size = UtilStringToHex(data, strlen(data), hex);

		// Perform RSA storage
		result = SecurityWriteRSA(currentObjectGroup, (uchar) atoi(rsa), size, hex);

		// Clean up
		my_free(hex);
	}

	return(result);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()RSA_CLEAR
//
// DESCRIPTION:	Perform RSA Storage
//
// PARAMETERS:	None
//
// RETURNS:		
//-------------------------------------------------------------------------------------------
//
bool __rsa_clear(const char *rsa)
{
	bool result = false;
	if (rsa) result = SecurityClearRSA(currentObjectGroup, (uchar) atoi(rsa));
	return(result);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()RSA_CRYPT
//
// DESCRIPTION:	Perform an RSA cryption
//
// PARAMETERS:	None
//
// RETURNS:		
//-------------------------------------------------------------------------------------------
//
char * __rsa_crypt(const char *data, const char *rsa)
{
	uchar * hex;
	char * string = NULL;
	bool ok = false;

	if (data && rsa)
	{
		int size = strlen(data) / 2;

		// Change the ASCII data to hex (binary).
		hex = my_calloc(size + 1);
		size = UtilStringToHex(data, strlen(data), hex);

		// Perform RSA storage
		ok = SecurityCryptRSA(currentObjectGroup, (uchar) atoi(rsa), size, hex);

		// Change back to ASCII data
		string = my_malloc(size*2 + 1);
		UtilHexToString((const char *)hex, size, string);
		my_free(hex);
	}

	// Push the result onto the stack
	if (ok) {
		return(string);
	} else {
		return(NULL);
	}

}


//
//-------------------------------------------------------------------------------------------
// RETURNS:		
//-------------------------------------------------------------------------------------------
//
bool __rsa_wrap_3des(const char *rsa_key, const char *des_key,char * result)
{
	uchar hex[256+1];
	char string[512+1] ="";
	int len;
	bool ok = false;

	if (rsa_key && des_key)
	{
		// Perform RSA storage
		ok = SecurityRSAWrap3Des(currentObjectGroup, (uchar) atoi(rsa_key), (uchar) atoi(des_key),&len, hex);
		if (ok) {
		    memset( string,0,sizeof(string));
			UtilHexToString((const char*)hex, len, string);
			strcpy(result,string);
		}
	}

	// Push the result onto the stack
	return(ok);
}


//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()DFORMAT1
//
// DESCRIPTION:	APCA format
//
// PARAMETERS:	None
//
// RETURNS:		The decoded payload
//-------------------------------------------------------------------------------------------
//
void __dformat1(char *data, int int_blockSize, char *result)
{
	char * string = NULL;

	strcpy( result , "");

	if (data && int_blockSize)
	{
		int i;
		uchar * hex;
		uchar * block;
		int size = strlen(data) / 2;
		int padCount;
		signed short checksum = 0;

		// Change the ASCII data to hex (binary).
		hex = my_calloc(size + 1);
		size = UtilStringToHex(data, strlen(data), hex);
		padCount = int_blockSize - size - 5;

		block = my_malloc(int_blockSize);

		if (padCount)
			block[0] = 0x03;			// Block format 3 and padding included.
		else
			block[0] = 0x02;
		block[1] = 0x00;			// Always ZERO.
		block[2] = int_blockSize/8;	// Number of 8-byte blocks of the modulus or block size.
									// Skip the checksum bytes data[3] and data[4]
		memcpy(&block[int_blockSize - padCount - size], hex, size);
		my_free(hex);

		if (padCount)
		{
			for (i = int_blockSize - padCount; i < int_blockSize; i++)
				block[i] = 0x55;
			block[i-1] = padCount;
		}

		for (i = 5; i < int_blockSize; i++)
			checksum = (checksum << 1) + ((checksum & 0x8000)?1:0) + block[i];

		block[3] = checksum >> 8;
		block[4] = checksum & 0xFF;

		// Change back to ASCII data
		string = my_malloc(int_blockSize*2 + 1);
		UtilHexToString((const char *)block, int_blockSize, string);
		my_free(block);

		string[ int_blockSize*2] = '\0';
		strcpy( result, string );
		my_free(string);
	}

}


//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()3DES_RSA_STORE
//
// DESCRIPTION:	Perform RSA Storage
//
// PARAMETERS:	None
//
// RETURNS:		
//-------------------------------------------------------------------------------------------
//
bool __3des_rsa_store(char *data, int rsakey, int kek)
{
	uchar * hex = NULL;
	bool result = false;

	// data Format:	FIRST BYTE =  modulus length in bytes (i.e. 2 = 16 bits, 0 = 2096 bits, 128 = 1024 bits).
	//			SECOND BYTE = exponent length in bytes (i.e. 2 = 16 bits, 0 = 2096 bits, 128 = 1024 bits).
	//			then..........modulus bytes.
	//				..........exponent bytes.
	// If RSA location (ie rsa) is 0 to 3, then modulus length has maximum value of 252 bytes (= 2016 bits) and exponent length has maximum of 3 bytes (= 24 bits).
	//

	if (data)
	{
		int size = strlen(data) / 2;

		// Change the ASCII data to hex (binary).
		hex = my_malloc(size + 1);
		size = UtilStringToHex(data, strlen(data), hex);

		// Perform RSA storage
		result = Security3DESWriteRSA(currentObjectGroup, (uchar) kek, currentObjectGroup, (uchar) rsakey, size, hex);

		// Clean up
		my_free(hex);
	}

	return(result);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : 
//
// DESCRIPTION:	Perform Key Xor Function
//
// PARAMETERS:	None
//
// RETURNS:		
//-------------------------------------------------------------------------------------------
//

bool __key_xor( const char* to, const char* from, const char* with_key, const char * with_data )
{
	bool result = false;

	if( to == NULL || from == NULL ) return (false);

	// Perform the encryption
	if ( with_key == NULL && with_data!= NULL && to && from )
		result = SecurityXorKeyWithData(currentObjectGroup, (uchar) atoi(from), 16, (uchar *) (with_data), (uchar) atoi(to) );
	else if ( with_data == NULL && with_key!= NULL && to && from )
		result = SecurityXorKey(currentObjectGroup, (uchar) atoi(from), 16, (uchar) atoi(with_key), (uchar) atoi(to) );

	return(result);
}

bool __get_key(int key,char *value)
{
	bool result = false;
	char temp[33];
	uchar hex[17];

	result = SecurityGetKey(currentObjectGroup, (uchar) key, hex);
	if( result ) {
		UtilHexToString( (const char *)hex, 16, temp);
		temp[32] = '\0';
		strcpy( value, temp);
	}

	return(result);
}
