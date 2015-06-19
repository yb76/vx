//
//-----------------------------------------------------------------------------
// PROJECT:			i-RIS - jsonSign
//
// FILE NAME:       main.c
//
// DATE CREATED:    12 March 2008
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
#include <windows.h>

//
// Project include files.
//
#include "cryptoki.h"
#include "sha1.h"
#include "crypto.h"

//
// Local include files
//

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

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : Main Program Entry
//
// DESCRIPTION:	Signs json files using an encrypted SHA1 and the i-RIS secret key
//
// PARAMETERS:	Program arguments (file name)
//
// RETURNS:		Error number
//-------------------------------------------------------------------------------------------
//
void signJSON(char * fileName)
{
	int i;
	sha1_context ctx;
	unsigned char digest[24];
	char signedName[100];
	char keep;

	char * data;
	unsigned long size;
	FILE * fp;
	char * ptr;

	// Open JSON object file which contains one object in a single line (output from jsonImport.exe) into memory
	if ((fp = fopen(fileName, "r+b")) == NULL)
	{
		fprintf(stderr, "Error reading file: %s\n", fileName);
		exit(-1);
	}

	// Read the object into memory
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	data = malloc(size+1);
	fseek(fp, 0, SEEK_SET);
	fgets(data, size+1, fp);

	// Look for label "SIGN:"
	if ((ptr = strstr(data, "SIGN:")) == NULL)
	{
		fprintf(stderr, "Object does not have a a SIGN string. Add one first.\n");
		return;
	}

	// Change all 40 characters afterwards to ZEROs - Just in case they are not
	memset(ptr+5, '0', 40);

	// Terminate at the end of the object. Again assuming one object in the file or the result will be wrong
	while(data[strlen(data)-1] != '}') data[strlen(data)-1] = '\0';

	// Calculate SHA1 over the entire object
	sha1_starts(&ctx);
	sha1_update(&ctx, data, strlen(data));
	memset(digest, 0, sizeof(digest));
	sha1_finish(&ctx, digest);

	// Encrypt SHA1
	Crypto_SKEncrypt(digest, sizeof(digest));

	// Store the encrypted SHA1 in the memory buffer instead of the ZEROs.
	keep = ptr[45];
	for (i = 0; i < 20; i++)
		sprintf(&ptr[i*2+5], "%02X", digest[i]);
	ptr[45] = keep;

	// Save the file as filename.s (Typically object.json.s) back into the same directory.
	// Key injection personnel are to send his back to RIS to store in the i-RIS server...
	fclose(fp);
	sprintf(signedName, "%s.s", fileName);
	fp = fopen(signedName, "w+b");
	fwrite(data, 1, strlen(data), fp);
	fclose(fp);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : Main Program Entry
//
// DESCRIPTION:	Signs json files using an encrypted SHA1 and the i-RIS secret key
//
// PARAMETERS:	Program arguments (file name)
//
// RETURNS:		Error number
//-------------------------------------------------------------------------------------------
//
int main(int argc, char ** argv)
{
	HANDLE fh;

	// Initialise cryptoki with slot 0 - SGB slot
	printf("Initialising cryptoki...\n");
	Crypto_Init((argc >= 2)?atol(argv[1]):0);

	// A file name must be supplied
	if (argc < 3)
	{
		WIN32_FIND_DATA * fd;
		fd = malloc(sizeof(WIN32_FIND_DATA));
		if ((fh = FindFirstFile("*.json", fd)) != INVALID_HANDLE_VALUE)
		{
			do
			{
				printf("Processing file: %s\n", fd->cFileName);
				signJSON(fd->cFileName);
			} while(FindNextFile(fh, fd));
		}
	}
	else signJSON(argv[2]);

	// close Crypto. No longer needed
	Crypto_Done();
}
