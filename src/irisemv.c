
#ifdef __EMV

//
//-----------------------------------------------------------------------------
// PROJECT:			iRIS
//
// FILE NAME:       irisemv.c
//
// DATE CREATED:    27 October 2010
//
// AUTHOR:          Katerina Phibbs
//
// DESCRIPTION:     This module contains iris emv functions called by the payment application
//-----------------------------------------------------------------------------
//

//
// Standard include files.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <auris.h>
#include <svc.h>

#include "iris.h"
#include "irisfunc.h"
#include "emv.h"

bool _emv_pack_tlv(const char* tag,char* results)
{
	char* tagind = 0;
	unsigned short btag;
	int status;
	unsigned char value[512];
	char result[1024+10]="";
	char string[1024]="";
	unsigned short uTagLen;

	if(tag == NULL || results==NULL) return(false);
	strcpy(results,"");

	for(tagind = (char *)tag; *tagind && strlen(tagind)>=4; tagind += 4 ){

			char stag[5] ="";
			memcpy(stag, tagind, 4 );
			stag[4] = '\0';
			// trim the last 00 

			btag = 0 ;
			ascii_to_binary((char *)&btag, stag, 4);
			if( stag[2] == '0' && stag[3] == '0') stag[2] = '\0';
			memset(string,0,sizeof(string));

			status = usEMVGetTLVFromColxn(btag, value, &uTagLen);
			if (status == EMV_SUCCESS) {
				UtilHexToString((const char *)value , uTagLen , string);
				sprintf( result,"%s%02X%s", stag,uTagLen, string);
			} else {
			    uTagLen = 0;
				sprintf( result,"%s00", stag);
			}
			strcat( results,result);
	}
	 
	return (true);
}


#endif
