/*
**-----------------------------------------------------------------------------
** PROJECT:         AURIS
**
** FILE NAME:       emvmisc.c
**
** DATE CREATED:    13 September 2010
**
** AUTHOR:			Katerina Phibbs    
**
** DESCRIPTION:     This module handles all EMV callback functionality
**-----------------------------------------------------------------------------
*/

/*
**-----------------------------------------------------------------------------
** Include Files
**-----------------------------------------------------------------------------
*/


#ifdef __EMV
/*
** Standard include files.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "alloc.h"
#include "emv.h"
#include "iris.h"
#include "utility.h"
#include "irisfunc.h"

/*!
** 	int	EmvSetTagData(const unsigned char* pcTag,
**									unsigned char* pbDataBuf,
**									int iDataBufLen);
**
** 	Set/update the value corresponding to an EMV tag.
**
** 	
**	pcTag	Buffer (const unsigned char) containing the EMV Tag whose value is to be
**				set/updated.
**
**	
**	pbDataBuf	Buffer (const unsigned char*) containing the new tag data.
**
**	iDataBufLen	Length (bytes) of the buffer containing the new tag data.
**
**  returns:
**	#SUCCESS				Operation completed successfully.
**	#ERR_EMV_INVALID_PARAMETER	Either the tag supplied is invalid,
**										or the buffer containing the tag data
**										(or it's length) is NULL or otherwise
**										invalid.
**	#ERR_EMV_FAILURE			Tag addition/modification failed.
*/
int	EmvSetTagData(const unsigned char* pcTag,const unsigned char* pbDataBuf, int iDataBufLen)
{
	unsigned short	usVxTag, usTagDataLen;
	int				status = 99;


	/*
	** Sanity checks
	*/
	if ((pcTag == NULL) || (pbDataBuf == NULL) || (iDataBufLen < 0))
	{
		return(ERR_EMV_INVALID_PARAMETER);
	}

	usTagDataLen = (unsigned short)iDataBufLen;

	memcpy( (char*)&usVxTag, pcTag,2 );


	/*
	** TVR/TSI tag data is updated differently to all other tag data
	*/
	status = (int)usEMVUpdateTLVInCollxn(usVxTag, (byte*)pbDataBuf, usTagDataLen);

	if (status != EMV_SUCCESS)
	{
		return(ERR_EMV_FAILURE);
	}


	return(SUCCESS);
}


/*
** 	int		EmvGetTagData(const unsigned char* pcTag, int iTagLen,
**						unsigned char* pbDataBuf, int iDataBufLen);
**
**	Return the value corresponding to a supplied EMV Tag.
**
**	NOTE: if the requested tag data is not found the function will simply return
**	0 as the returned data length and will not return an error.
**
**	pcTag	Buffer (const unsigned char*) containing the EMV Tag whose value is required.
**
**	pbDataBuf	Buffer (unsigned char*) supplied for storage of the returned tag data.
**
**	iDataBufLen	Length (bytes) of the buffer supplied for storage of the returned tag
**				data.
**
**	retval	>= 0							Operation completed successfully. Value returned
**											is the length (bytes) of the requested EMV tag data.
**	retval	(-#ERR_EMV_INVALID_PARAMETER)	Either of the buffers supplied (or their lengths)
**											is invalid.
**	retval	(-#ERR_EMV_FAILURE)				Tag requested could not be found.
*/

int EmvGetTagDataRaw( unsigned short pcTag, char *databuf)
{
	int status;
	uchar hex[1024];
	unsigned short valuelen=0;

	strcpy(databuf,"");
	memset(hex,0,sizeof(hex));

	status = usEMVGetTLVFromColxn(pcTag, hex, &valuelen);
	if(status != (int)EMV_SUCCESS) return(ERR_EMV_FAILURE);

	UtilHexToString((const char *)hex,valuelen,databuf);
 	return(0);
}

int		EmvGetTagData(const unsigned char* pcTag, unsigned char* pbDataBuf, int *piTagLength, int iDataBufLen)
{
	unsigned short	usVxTag, usTagDataLen;
	int				status;


	/*
	** Sanity checks
	*/
	if ((pcTag == NULL) || (pbDataBuf == NULL) || (iDataBufLen <= 0))
	{
		return(-((int)ERR_EMV_INVALID_PARAMETER));
	}


	/*
	** Return required tag data.
	** NOTE: if the requested tag data is not found the function will simply
	** return 0 as the returned data length and will not return an error.
	*/
	/* Convert EMV tag into Verifone format */
	usVxTag = EmvTag2VxTag(pcTag);
	if (usVxTag == 0)
	{
		return(-((int)ERR_EMV_INVALID_PARAMETER));
	}

	/* Intialise tag data buffer */
	memset(pbDataBuf, 0, (iDataBufLen * sizeof(unsigned char)));
	usTagDataLen = 0;


	/* Extract required tag data */
	status = (int)usEMVGetTLVFromColxn(usVxTag, pbDataBuf, &usTagDataLen);
	if (status == (int)EMV_SUCCESS)
	{
		status = (int)usTagDataLen;
		*piTagLength = status;


	}
	else
	{
		if (status == E_TAG_NOTFOUND)
		{
			status = 0;
			memset(pbDataBuf, 0, (iDataBufLen * sizeof(unsigned char)));
			*piTagLength = 0;

		}
		else
		{
			status = -((int)ERR_EMV_FAILURE);
		}
		


	}

	return(status);
}

/*
** 	unsigned short	EmvTag2VxTag(const unsigned char* pcTag,int iTagLen);
**
** 	
**	Convert an EMV Tag from our format into
**	the format used by the Verifone EMV Library/Module.
**
** 	
**	pcTag	Buffer (const FPEMV_BYTE*) containing the EMV Tag in the
**				format used by iris
**
** 	\retval	> 0	Operation completed successfully. The returned value is the
**				value of the supplied tag in the format used by the Verifone
**				EMV Library/Module
**	\retval	0	Tag is malformed or it's length is invalid.
*/
unsigned short	EmvTag2VxTag(const unsigned char* pcTag)
{
	unsigned short	usVxTag, usTagLen;


	/*
	** Sanity checks
	*/
	usTagLen = EmvTagLen(pcTag);
	if ((usTagLen == 0) || (usTagLen > 2))
	{
		return((unsigned short)0);
	}

	/*
	** Convert tag format
	*/
	usVxTag = ((unsigned short)pcTag[0] << 8) & 0xff00;
	if (usTagLen == 2)
	{
		usVxTag |= (((unsigned short)pcTag[1]) & 0x00ff);
	}

	return(usVxTag);
}

/*!
** 	int		EmvTagLen(const unsigned char* pcTag);
**
** 	
**	Return the expected length (bytes) of an EMV tag
**	(ie: the actual tag-value, not the data corresponding to the tag)
**
** 	param
**	pcTag	Buffer (const unsigned char*) containing the EMV Tag whose
**				length is to be determined in the format used by us
**
**	\retval		Returns the length (bytes) of the supplied EMV tag.
*/
unsigned short	EmvTagLen(const unsigned char* pcTag)
{
	unsigned short	i;


	/*
	** Sanity checks
	*/
	if (pcTag == NULL)
	{
		return(0);
	}

	/*
	** Length of the tag is determined as follows:
	**	-	If the lowest 5-bits of the 1st byte of the tag < 31
	**		then the length of the tag is 1 byte
	**	- 	If the lowest 5-bits of the 1st byte of the tag == 31
	**		then subsequent tag bytes follow
	**	-	If the most significant bit of a subsequent byte of
	**		the tag == 1 then another tag byte follows
	**	-	If the most significant bit of a subsequent byte of
	**		the tag == 1 then this byte is the last byte of the
	**		tag
	*/
	if ((pcTag[0] & 0x1f) < 0x1f)
	{
		return(1);
	}

	for (i = 2;; i++)
	{
		if ((pcTag[i - 1] & 0x80) == 0)
		{
			break;
		}
	}

	return(i);
}

/*
** 	int		EmvMiscVerixVEmvErr2RisEmvErr(int iVerixVEmvErr);
**
**	Convert a Verix-V EMV-Module error code into the equivalent RIS err code
**
**	iVerixVEmvErr	Error code returned by the Verix-V EMV-Module.
**
**	Returns our EMV Library error code equivalent to the supplied Verix-V EMV-Module error code.
*/
int	EmvMiscVerixVEmvErr2RisEmvErr(int iVerixVEmvErr)
{
	int		iRisEmvErr;

	/*
	** Translate Verifone Verix-V EMV Module error into our error
	*/
	switch (iVerixVEmvErr)
	{
		case APPL_BLOCKED:
		case E_APP_BLOCKED:
			/*
			** Application is blocked
			*/
			iRisEmvErr = ERR_EMV_APPLICATION_BLOCKED;
			break;

		case APPL_NOT_AVAILABLE:
			/*
			** Unable to start selected application
			*/
			iRisEmvErr = ERR_EMV_NO_RETRY_FAILURE;
			break;

		case BAD_DATA_FORMAT:
			/*
			** Invalid data returned by the ICC
			*/
			iRisEmvErr = ERR_EMV_CARD_FAILURE;
			break;

		case BAD_ICC_RESPONSE:
			/*
			** Invalid response was received from the ICC
			*/
			iRisEmvErr = ERR_EMV_CARD_FAILURE;
			break;

		case CANDIDATELIST_EMPTY:
			/*
			** Unable to find an application supported by the terminal
			** and the card
			*/
			iRisEmvErr = ERR_EMV_CANDIDATE_LIST_EMPTY;
			break;

		case CARD_BLOCKED:
		case E_ICC_BLOCKED:
			/*
			** Card is blocked
			*/
			iRisEmvErr = ERR_EMV_CARD_BLOCKED;
			break;

		case CARD_REMOVED:
			/*
			** Card removed before the transaction was completed
			*/
			iRisEmvErr = ERR_EMV_CARD_REMOVED;
			break;

		case CHIP_ERROR:
			/*
			** Communication with the ICC failed
			*/
			iRisEmvErr = ERR_EMV_CARD_FAILURE;
			break;

		case E_EXPLICIT_SELEC_REQD:
			/*
			** Application requires explicit selection
			*/
			iRisEmvErr = ERR_EMV_EXPLICIT_SELECT_REQD;
			break;

		case EMV_FAILURE:
			/*
			** Operation failed
			*/
			iRisEmvErr = ERR_EMV_FAILURE;
			break;

		case ICC_DATA_MISSING:
			/*
			** Mandatory data objects missing
			*/
			iRisEmvErr = ERR_EMV_REQUIRED_DATA_MISSING;
			break;

		case INVALID_PARAMETER:
			/*
			** Operation failed because an invalid parameter was supplied
			*/
			iRisEmvErr = ERR_EMV_INVALID_PARAMETER;
			break;

		case TRANS_CANCELLED:
			/*
			** Transaction cancelled by the customer/terminal-operator
			*/
			iRisEmvErr = ERR_EMV_CLIENT_ABORTED;
			break;

		case TAG_ALREADY_PRESENT:
		case USE_MAG_CARD:
		case 0x6985:
		case 0x9481:
			/*
			** Transaction failed but may be retried as a mag-swipe transaction
			*/
			iRisEmvErr = ERR_EMV_NO_RETRY_FAILURE;
			break;

		default:
			/*
			** Unknown/invalid/unsupported Verix-V EMV-Module error code
			*/
			iRisEmvErr = ERR_EMV_CARD_ERROR;
			break;
	}

	return((int)iRisEmvErr);
}

/*
** 	int		EmvMiscTagLenLen(const FPEMV_BYTE* pcTagLenBytes);
**
**	Return the length of the length-field within a tag.
**
**	pcTagLenBytes	Buffer contining the length field of the tag.
**
**	retval		Returns the length (bytes) of the supplied EMV tag
**				length.
*/
unsigned short	EmvMiscTagLenLen(const unsigned char* pcTagLenBytes)
{
	/*
	** Sanity checks
	*/
	if (pcTagLenBytes == NULL)
	{
		return(0);
	}

	/*
	** Length of the tag length-field is determined as follows:
	**	- If the most significant bit of the 1st byte is zero,
	**	  then the number of bytes used to represent the tag
	**	  length is 1
	**	- If the most significant bit of the 1st byte is 1, then
	**	  the remaining bits of the 1st byte represent the
	**	  number of bytes remaining in the tag-length field
	*/
	if ((pcTagLenBytes[0] & 0x80) == 0x80)
    {
        return((unsigned short)((pcTagLenBytes[0] & 0x7F) + 1));
    }
    else
    {
        return(1);
    }
}


/*
** 	int		EmvMiscTagDataLen(const FPEMV_BYTE* pcTagLenBytes);
**
**	Return the length of the data within a tag.
**
**	pcTagLenBytes	Buffer contining the length field of the tag.
**
**	Returns the length (bytes) of the data within the tag.
*/
unsigned short	EmvMiscTagDataLen(const unsigned char* pcTagLenBytes)
{
	unsigned short	wTagLenLen, wTagDataLen;
	int				i;


	/*
	** Sanity checks
	*/
	if (pcTagLenBytes == NULL)
	{
		return(0);
	}

	/*
	** Length of the tag data field is determined as follows:
	**	- If the most significant bit of the 1st byte is zero,
	**	  then the number of bytes used to represent the tag
	**	  length is 1, & the actual length of the data within
	**	  the tag is the number of bytes represented by the
	**	  value of the length byte.
	**	- If the most significant bit of the 1st byte is 1, then
	**	  the remaining bits of the most 1st byte represent the
	**	  number of bytes remaining in the tag-length field, and
	**	  the actual length of the data within the tag is equal
	**	  to the integer represented by these remaining bytes.
	*/
	wTagLenLen = EmvMiscTagLenLen(pcTagLenBytes);
	wTagDataLen = 0;

	if (wTagLenLen > 1)
    {
	    for(i = 1; i < wTagLenLen; i++)
        {
            wTagDataLen <<= 8;
            wTagDataLen += (unsigned short)(pcTagLenBytes[i]);
        }
    }
    else
    {
	    wTagDataLen = pcTagLenBytes[0];
    }

    return(wTagDataLen);
}

/*--------------------------------------------------------------------------
    Function:		ascii_to_binary
    Description:	Packs a string of ASCII hex digits into the 
                    destination buffer in binary form
    Parameters:		void
    Returns:		
	Notes:          
--------------------------------------------------------------------------*/

void ascii_to_binary(char *dest, const char *src, int length)
{	
	/* length = length of source */
	short   i=0;
	char    digit_val;

	if (length & 0x0001)
	{	// length is odd, so do some adjustment to put a zero at the start
        if ((src[0] - '0') > 9)
		{
            if ((src[0] - 'A') > 5)
                digit_val = (src[0] - 'a' + 10);
            else
                digit_val = (src[0] - 'A' + 10);
		}
		else
            digit_val = (src[0] - '0');

        dest[0] = 0;
        dest[0] |= (digit_val & 0x0F);
        dest++;
        src++;
        length--;
	}

    while(i < length)
	{
        if ((src[i] - '0') > 9)
		{
            if ((src[0] - 'A') > 5)
                digit_val = (src[i] - 'a' + 10);
            else
                digit_val = (src[i] - 'A' + 10);
		}
        else
            digit_val = (src[i] - '0');

        if (i & 0x0001)
            dest[i/2] |= (digit_val & 0x0F);
        else
		{
            dest[i/2] = 0;
            dest[i/2] |= ((digit_val << 4) & 0xF0);
		}

        i++;
	}

    return;
}

short getNextTLVObject(unsigned short *tag, short *length, byte *value, const byte *buffer);
// Find a tag and return the data part of that tlv object in the value parameter and the length of the data part
short findTag(unsigned short tag, byte *value, short *length, const byte *buffer,short bufLen)
{
    byte            *ptr;
    unsigned short  nextTag = 0;
    short           found = 0;
    short           len = 0;
    short           bytesRead = 0;
    
	ptr =(byte *) buffer;

 // Start of data section
    found = 0;
    bytesRead = 0;
    do
    {
        bytesRead += getNextTLVObject(&nextTag, &len, value, ptr + bytesRead);
        if (nextTag == tag)
        {
            found = 1;
            *length = len;
            break;
        }
    } while (bytesRead < bufLen);

    if (found)
        return (1);

    return (0);
}

/*--------------------------------------------------------------------------
    Function:		getNextRawTLVData
    Description:	Gets the next tag and the raw tlv data
    Parameters:		unsigned short, byte *, const byte *
    Returns:		No. of bytes read
	Notes:          
--------------------------------------------------------------------------*/

short getNextRawTLVData(unsigned short *tag, byte *data, const byte *buffer)
{
    short    bytesRead = 0;
    byte     *ptr;
    byte     tagByte1 = 0x00;
	byte     tagByte2 = 0x00;
    short    numTagBytes = 0;
    byte     dataByte = 0x00;
    short    dataLength = 0;
    short    numLengthBytes = 0;
    short    i = 0;

    // Get the tag
    ptr = (byte*) buffer;
    tagByte1 = *ptr;

    //    EMV specification says, any tag with == 0x1F (31) must be treated as two byte tags.

    if ((tagByte1 & 31) == 31)                                        // Bit pattern must be 10011111 or greater
    {
        ptr++;
        tagByte2 = *ptr;
        *tag = (int) ((tagByte1 << 8) + tagByte2);
        numTagBytes = 2;
    }
    else
    {
        *tag = (int) tagByte1;
        numTagBytes = 1;
    }

    // Get the data
    ptr++;
    dataByte = *ptr;
    if (dataByte & 128)                                                 // If last bit is set
    {
        dataLength = 0;
        numLengthBytes = (int) dataByte & 127;                          // b7 - b1 represent the number of subsequent length bytes
        ptr++;
        for (i = 0; i < numLengthBytes; i++)
        {
            dataLength = (dataLength << 8) + (int) *ptr;
            ptr++;
        }   
    }
    else                                                                // Length field consists of 1 byte max value of 127
    {
        numLengthBytes = 1;
        dataLength = (int) *ptr;
        ptr++;
    }

    bytesRead = numTagBytes + numLengthBytes + dataLength;

    ptr = (byte*) buffer;
    for (i = 0; i < bytesRead; i++)
    {
        data[i] = *ptr;
        ptr++;
    }

    return (bytesRead);
}

/*--------------------------------------------------------------------------
    Function:		getNextTLVObject
    Description:	Gets next tlv object separated into tag, length and value
    Parameters:		unsigned short *, short *, byte *, const byte *
    Returns:		No. of bytes read
	Notes:          
--------------------------------------------------------------------------*/

short getNextTLVObject(unsigned short *tag, short *length, byte *value, const byte *buffer)
{
    byte       *ptr;
    byte       tagByte1 = 0, tagByte2 = 0;
    byte       dataByte = 0;
    short      numLengthBytes = 0;
    short      dataLength = 0;
    short      i = 0;
    short      numTagBytes = 0;
    short      bytesRead = 0;

    ptr = (byte *)buffer;   

    tagByte1 = *ptr;

    //    EMV specification says, any tag with == 0x1F (31) must be treated as two byte tags.
    // 
    if ((tagByte1 & 31) == 31)                                        // Bit pattern must be 10011111 or greater
    {
        ptr++;
        tagByte2 = *ptr;
        *tag = (short) ((tagByte1 << 8) + tagByte2);
        numTagBytes = 2;
    }
    else
    {
        *tag = (short) tagByte1;
        numTagBytes = 1;
    }

    // Get the data
    ptr++;
    dataByte = *ptr;
    if (dataByte & 128)                                                 // If last bit is set
    {
        dataLength = 0;
        numLengthBytes = (short) dataByte & 127;                          // b7 - b1 represent the number of subsequent length bytes
        ptr++;
        for (i = 0; i < numLengthBytes; i++)
        {
            dataLength = (dataLength << 8) + (short) *ptr;
            ptr++;
        }   
    }
    else                                                                // Length field consists of 1 byte max value of 127
    {
        numLengthBytes = 1;
        dataLength = (short) *ptr;
        ptr++;
    }

    *length = dataLength;

    // ptr should now be pointing at the data
    for (i = 0; i < dataLength; i++)
    {
        value[i] = *ptr;
        ptr++;
    }

    bytesRead = numTagBytes + numLengthBytes + dataLength;

    return (bytesRead);
}

int createScriptFiles(byte *scriptBuf, short bufLen, byte* pScript71, unsigned short *usScript71Len, byte* pScript72, unsigned short *usScript72Len )
{
    byte             *ptr;
    byte             data[1024] = {0};
    unsigned short   tag = 0;
    //short            bufLen = 0;
    short            bytesRead = 0;
    short            bytes = 0;
    short            numScripts = 0;
    
    //bufLen = scriptBuf[10];
	
    ptr = &scriptBuf[0];              
	*usScript71Len = 0;
	*usScript72Len = 0;
    bytes = 0;
    bytesRead = 0;
    numScripts = 0;
    do
    {
		memset(data,0x00,sizeof(data));
        bytes = getNextRawTLVData(&tag, data, ptr + bytesRead);
	    bytesRead += bytes;

		if ( (tag == 0x71) && (pScript71) )
			{
		        numScripts++;			
				memcpy((char *)pScript71,(char *) data, bytes); 			
				pScript71 += bytes;
				*usScript71Len += bytes;
			}
		else if ( (tag == 0x72) && (pScript72) )
			{
		        numScripts++;						
				memcpy((char *)pScript72,(char *) data, bytes); 						
				*usScript72Len += bytes;				
				pScript72 += bytes;
			}
    } while (bytesRead < bufLen);

    return (numScripts);
}

int EmvGetTacIac(char * tac_df, char * tac_dn, char *tac_ol, char * iac_df, char *iac_dn, char *iac_ol)
{
	char szTACDefault[30]="";
	char szTACDenial[30]="";
	char szTACOnline[30]="";
	char szIACDefault[30]="";
	char szIACDenial[30]="";
	char szIACOnline[30]="";

	//vdGetMVTTermActionCodes(szTACDefault, szTACDenial, szTACOnline);
	
	if(strlen(gEmv.tac_default)) strcpy(tac_df,gEmv.tac_default); else strcpy(tac_df,szGetEMVTACDefault());
	if(strlen(gEmv.tac_denial)) strcpy(tac_dn,gEmv.tac_denial); else strcpy(tac_dn,szGetEMVTACDenial());
	if(strlen(gEmv.tac_online)) strcpy(tac_ol,gEmv.tac_online); else strcpy(tac_ol,szGetEMVTACOnline());

	EmvGetTagDataRaw(TAG_9F0D_IAC_DEFAULT,szIACDefault);
	EmvGetTagDataRaw(TAG_9F0E_IAC_DENIAL,szIACDenial);
	EmvGetTagDataRaw(TAG_9F0F_IAC_ONLINE,szIACOnline);
	strcpy(iac_df,szIACDefault);
	strcpy(iac_dn,szIACDenial);
	strcpy(iac_ol,szIACOnline);

	return(0);
}

bool emv_tlv_replace( char *newtlvs,short* bufLen,unsigned short btag)
{
	char *ptr = newtlvs ;
	unsigned short nextTag;
	unsigned short rawTag;
	unsigned short status;
	unsigned short len,newlen;
	unsigned char value[1024];
	unsigned char newvalue[1024];
	short bytesRead = 0;
	short taglen = 1;
	char ctmp;
	char *ptmp;

	if(ptr == NULL) return false;

	ptmp = (char*) &btag;
	ctmp = *ptmp;
    if ((ctmp & 31) == 31) { rawTag = btag; taglen=2; }                                     // Bit pattern must be 10011111 or greater
	else { rawTag = btag >> 8; taglen = 1;}

	status = usEMVGetTLVFromColxn(rawTag, newvalue, &newlen);
	if(status != EMV_SUCCESS) {
		newlen = 0;
		strcpy((char*)newvalue,"");
	}

 // Start of data section
    bytesRead = 0;
    do
    {
		char *ptrtmp = ptr + bytesRead;
		bytesRead += getNextTLVObject(&nextTag, (short *)&len, value, (const unsigned char *)ptrtmp );
        if (nextTag == btag)
        {
			if( newlen != len  || memcmp( newvalue,value,len) != 0) {
				if(newlen == len) {
					ptrtmp += taglen;
					ptrtmp ++;
					memcpy( ptrtmp, newvalue, newlen);
				} else {
					//memcpy( ptrtmp, btag, taglen);
					ptrtmp += taglen;
					*ptrtmp = newlen ;
					ptrtmp ++;
					memmove( ptrtmp+newlen, ptr+bytesRead, *bufLen - bytesRead );
					memcpy( ptrtmp, newvalue, newlen);
					ptrtmp += newlen;
					*bufLen += ( newlen - len );	
				}
			}
            break;
        }
    } while (bytesRead < *bufLen);


	return true;
}

#endif
