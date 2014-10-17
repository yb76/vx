//
//-----------------------------------------------------------------------------
// PROJECT:			iRIS
//
// FILE NAME:       iris.c
//
// DATE CREATED:    5 March 2008
//
// AUTHOR:          Tareq Hafez
//
// DESCRIPTION:     This module supports iris object resolutions
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
#include "utility.h"
#include "irisfunc.h"
#include "security.h"
#include "iris.h"

//
//-----------------------------------------------------------------------------
// Module variable definitions and initialisations.
//-----------------------------------------------------------------------------
//

#define C_MAX_STACK         100

char irisGroup[] = "iRIS";
static char * upload = NULL;


//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
// IRIS FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

//
//-----------------------------------------------------------------------------
// FUNCTION   : IRIS_AppendToUpload
//
// DESCRIPTION:	Add an upload message to the next remote session
//
// PARAMETERS:	addition	<=	New data to add
//
// RETURNS:		None
//
//-----------------------------------------------------------------------------
//
void IRIS_AppendToUpload(const char * addition)
{
	if (upload)
	{
		upload = my_realloc(upload, strlen(upload) + strlen(addition) + 1);
		strcat(upload, addition);
	}
	else
	{
		upload = my_malloc(strlen(addition) + 1);
		strcpy(upload, addition);
	}
}

bool FileExist(const char *filename)
{
	int handle;
	handle = open(filename, FH_RDONLY);
	if (FH_ERR(handle))
		return (false);

	close(handle);
	return (true);	
}

bool FileRemove(const char *filename)
{
		int ret = 0;
		ret = _remove(filename);
		if(ret == 0 ) return(true);
		else return(false);
}

bool FileRename(const char *ofilename,const char *nfilename)
{
		int ret = 0;
		ret = _rename(ofilename,nfilename);
		if(ret == 0 ) return(true);
		else return(false);
}


void IRIS_StoreData(const char * objectName, const char *tag, const char * value, bool deleteFlag)
{
	char * objectData;
	uint objectLength;


	if(objectName == NULL ) return;

	if (deleteFlag)
		{
			(bool)FileRemove(objectName);
			return;
		}

	if( tag == NULL ) return;

	// Get the object data
	if ((objectData = IRIS_GetObjectData(objectName, &objectLength)) == NULL )
	{
		objectData = my_malloc	(50 + strlen(objectName) + strlen(tag) + strlen(value));

		sprintf(objectData, "{TYPE:DATA,NAME:%s,GROUP:%s,VERSION:%s,%s:%s}", objectName, irisGroup, "1",tag,value);

		IRIS_PutNamedObjectData((const char *)objectData, strlen(objectData), objectName);
		UtilStrDup(&objectData, NULL);
		return;
	}


	// Obtain the group information but not for objectless temporary data

	// The object must have a simple value group for restricted access. If no group, my_free access.
		// If it is a temporary data area, store/update the value
	{
					char * ptr, * ptr2;
					char search[100];

					{
						char * newTagValue = NULL;

						// Create the new tag value pair
						if ( value)
						{
							newTagValue = my_malloc(strlen(tag)+strlen(value)+3);
							sprintf(newTagValue, ",%s:%s", tag, value);
						}

						// Look for the beginning of the string
						sprintf(search, ",%s:", tag);
						ptr = strstr(objectData, search);
						if(ptr == NULL) {
							sprintf(search, "{%s:", tag);
							ptr = strstr(objectData, search);
						}

						// If an existing value exists, find the end of the existing tag:value
						if (ptr)
						{
						    ptr++;
							if ((ptr2 = strchr(ptr+1,',')) == NULL)	// This will not work with complex value. It must be simple
								ptr2 = strchr(ptr+1,'}');
							if (!newTagValue && *ptr != ',') ptr2++;

							// If they are exactly the same value, do not attempt to write anything....skip this.
							if (newTagValue && (strlen(&newTagValue[1]) == (uint) (ptr2 - ptr)) && strncmp(&newTagValue[1], ptr, ptr2-ptr) == 0)
							{
#ifdef _DEBUG
								printf("Skipping storing same PERM data ====> %s:%s\n", objectName, value);								
#endif
								ptr = NULL;
							}
						}
						// If not found, add the new tag:value it to the end of the file as long as we have a new tag:value
						else if (newTagValue)
							ptr2 = ptr = strrchr(objectData,'}');

						// Replace with the new tag:value
						if (ptr)
						{
							FILE_HANDLE handle;

							// Open the object file for an update
							handle = open(objectName, FH_RDWR);
							lseek(handle, ptr - objectData, SEEK_SET);

							if (newTagValue && (strlen(&newTagValue[1]) == (uint) (ptr2 - ptr)))
								write(handle, &newTagValue[1], ptr2 - ptr);
							else
							{
								int offset = 0;
								if (ptr != ptr2)
								{
									_delete_(handle, ptr2 - ptr, ptr, objectData, objectLength);
									offset = 1;
								}
								if (newTagValue)
									_insert(handle, &newTagValue[offset], strlen(&newTagValue[offset]), ptr, objectData, objectLength);
							}

							close(handle);
						}
						UtilStrDup(&newTagValue, NULL);
					}
	}

	// Clean up
	UtilStrDup(&objectData, NULL);
}
//
//-----------------------------------------------------------------------------
// FUNCTION   : IRIS_PutObjectData
//
// DESCRIPTION:	Store the object data in the specified file
//
// PARAMETERS:	objectData	<=	The object data to store
//
// RETURNS:		None
//
//-----------------------------------------------------------------------------
//
void IRIS_PutNamedObjectData(const char * objectData, uint length, const char * name)
{
	FILE_HANDLE handle;

	handle = open(name, FH_NEW);
	write(handle, objectData, length);
	close(handle);
}

void IRIS_PutObjectData(const char * objectData, uint length)
{
	char * name;

	name = IRIS_GetStringValue(objectData, length, "NAME");

	// Create the object within the terminal
	if (name)
		IRIS_PutNamedObjectData(objectData, length, name);

	// Deallocate the object value used
	IRIS_DeallocateStringValue(name);
}

//
//-----------------------------------------------------------------------------
// FUNCTION   : 
//
// DESCRIPTION:	
//			
//
// PARAMETERS:	
//			
//
// RETURNS:		
//			
//
//-----------------------------------------------------------------------------
//
bool remoteTms()
{
	char * ptr=NULL;
	char * ptrtmp=NULL;
	char serialNo[20];
	char manufacturer[20];
	char model[20];

	char tx[1000];
	char * dataString;
	char * data;
	char iv[20];
	char kvc[10];
	char proof[40];

	int retry;
	char temp[50];
	char timeout_s[10]="20";
	char itimeout_s[10];
	static char * commstype = NULL;
	static char iConnectType = 0; //1-IP,2:PSTN
	short restart = 0;
	short maxTry = 1;
	char *objdata=NULL;
	unsigned int objlength=0;
	static char key_master[10]="4";
	static char key_kek[10]="6";
	static char key_mkr[10]="103";
	static char key_mks[10]="101";
	static char key_ppasn[10]="50";
	static bool key_inited = false;
	int bufferSize = 2000;

	temp[0] = '\0';

	// Establish a TCP connection
	for (retry = 0; retry < maxTry; retry++)
	{
		char *errmsg=NULL;

		static char * bufSize = NULL;
		static char * interCharTimeout = NULL;
		static char * timeout = NULL;
		static char * port = NULL;
		static char * hip = NULL;
		static char * sdns = NULL;
		static char * pdns = NULL;
		static char * gw = NULL;
		static char * oip = NULL;
		static char * header = NULL;

		static char * phoneno = NULL;

		if( commstype == NULL) {
			objdata = IRIS_GetObjectData("IRIS_CFG",&objlength);
			commstype = objdata?IRIS_GetStringValue(objdata,objlength,"COMMS_TYPE"):NULL;
			if(commstype == NULL || objdata == NULL) {
				if (objdata) UtilStrDup(&objdata,NULL);
				if (upload) UtilStrDup(&upload, NULL);
				return false;
			}

			if( strcmp(commstype, "IP")==0 ) {
				header = "1";				// Length header
				oip = IRIS_GetStringValue(objdata,objlength,"OIP");
#ifdef __VX670
				gw="R";
#else
				gw = IRIS_GetStringValue(objdata,objlength,"GW");
#endif
				pdns = IRIS_GetStringValue(objdata,objlength,"PDNS");
				sdns = IRIS_GetStringValue(objdata,objlength,"SDNS");
				hip = IRIS_GetStringValue(objdata,objlength,"HIP");
				port = IRIS_GetStringValue(objdata,objlength,"PORT");
				timeout = IRIS_GetStringValue(objdata,objlength,"TIMEOUT");
				strcpy(timeout_s,timeout);
				interCharTimeout = IRIS_GetStringValue(objdata,objlength,"ITIMEOUT");
				strcpy(itimeout_s,interCharTimeout);
				bufSize = IRIS_GetStringValue(objdata,objlength,"BUFLEN");
				iConnectType = 1;

			} else if (strcmp(commstype , "DIAL") == 0 ) {
				phoneno = IRIS_GetStringValue(objdata,objlength,"PHONENO");
				iConnectType = 2;
			}
			IRIS_DeallocateStringValue(objdata);
		}
					
		if( iConnectType == 1 )  {
			__tcp_disconnect_completely();
			__tcp_connect(bufSize,interCharTimeout,timeout,port,hip,sdns,pdns,gw,oip,header,&errmsg);
		} else if(iConnectType == 2) {
			errmsg =(char*) __pstn_connect("4096","1200","1000","45",phoneno,""/*pabs*/,"1"/* fastconnect */,
									"0"/* blienddial */,"1"/* dialtype */,"1"/*sync*/,"0"/* predial */, "8");
		}

		if (errmsg && strcmp(errmsg, "NOERROR") == 0)
		{
			break;
		}
		SVC_WAIT(1000);
	}


	if (retry == maxTry  )
	{
		if (upload) UtilStrDup(&upload, NULL);
		return false;
	}

	data = my_malloc(bufferSize);

	// Get the terminal configuration
	__serial_no(serialNo);
	__manufacturer(manufacturer);
	__model(model);

	if ( !key_inited ) {
	 	objdata = IRIS_GetObjectData("IRIS_CFG",&objlength);
	 	if(objdata) {
	 		char *kek = IRIS_GetStringValue(objdata,objlength,"KEK");
	 		char *master = IRIS_GetStringValue(objdata,objlength,"MASTER");
	 		char *mkr = IRIS_GetStringValue(objdata,objlength,"MKr");
	 		char *mks = IRIS_GetStringValue(objdata,objlength,"MKs");
	 		char *ppasn = IRIS_GetStringValue(objdata,objlength,"PPASN");
			if (kek) strcpy( key_kek,kek);
			if (master) strcpy( key_master,master);
			if (mkr) strcpy( key_mkr,mkr);
			if (mks) strcpy( key_mks,mks);
			if (ppasn) strcpy( key_ppasn,ppasn);
			IRIS_DeallocateStringValue(kek);
			IRIS_DeallocateStringValue(master);
			IRIS_DeallocateStringValue(mkr);
			IRIS_DeallocateStringValue(mks);
			IRIS_DeallocateStringValue(ppasn);
	 		IRIS_DeallocateStringValue(objdata);
		}
		key_inited = true;
	 }

	 do {
			int iLength = 0;

			restart = 0;
			// Generate a random iv via a random key
			SecurityGenerateKey(irisGroup, 0, 8);
			SecuritySetIV(NULL);
			UtilStrDup(&ptrtmp,NULL);
			ptr = (char*)__crypt("0123456789ABCDEF","8","0","",false,false);


			strcpy(iv,ptr);
			UtilStrDup(&ptr,NULL);

			// Encrypt the proof which is IV | signature
			SecuritySetIV(NULL);
			sprintf(temp, "%s%s", iv, "CAFEBABEDEAFF001");
			ptr = (char*)__crypt(temp,"",key_mkr,"",false,false);
			strcpy(proof,ptr);
			UtilStrDup(&ptr,NULL);

			// Send KVC of KEK as well
			__kvc("",key_kek,kvc);

			// Add the authentication object
			sprintf(tx, "{TYPE:IDENTITY,SERIALNUMBER:%s,MANUFACTURER:%s,MODEL:%s}{TYPE:AUTH,PROOF:%s,KVC:%s}", serialNo, manufacturer, model, proof,kvc);

			// Add the event object and GETOBJECTs
			data[0] = '\0';

			// Add objects required for upload
			if (upload )
			{
				iLength = strlen(data) + strlen(upload) + strlen(tx) ;
				if( iLength > bufferSize) data = my_realloc(data, iLength);
				strcat(data, upload);
			}

			// Set IV
			__iv_set(iv);

			// OFB
			dataString = my_malloc(strlen(data)*2 + 1);
			UtilHexToString((const char *) data, strlen(data), dataString);
			ptr = (char*)__crypt(dataString, "",key_mkr,"",false,true);
			my_free(dataString);
			__iv_set(NULL);

			// Set the transmit buffer
			iLength = strlen(tx) *2 + strlen(ptr) + 1;
			if( iLength > bufferSize ) data = my_realloc(data, iLength);
			UtilHexToString((const char *) tx, strlen(tx), data);
			strcat(data, ptr);
			UtilStrDup(&ptr,NULL);

			// Send the data
			strcpy(temp,"");
			if(iConnectType == 1) {
			  __tcp_send(data,temp);
			} else {
			  __pstn_send(data,temp);
			}

			if (strcmp(temp, "NOERROR"))
			{
				// Clean up
				if (upload) UtilStrDup(&upload, NULL);
				my_free(data);
				UtilStrDup(&ptrtmp,NULL);
				return false;
			}

			// Get the response
			if(iConnectType == 1) {
		  		__tcp_recv( &ptr,"2000",timeout_s,temp);
			}
			else {
		  		__pstn_recv( &ptr,"7000","45",temp);
			}
			ptrtmp = ptr;

			if (strcmp(temp, "NOERROR"))
			{
				// Clean up
				if (upload) UtilStrDup(&upload, NULL);
				my_free(data);
				UtilStrDup(&ptrtmp,NULL);
				return false;
			}

			// Examine the authenticate object
			strcpy(data, "{TYPE:AUTH,RESULT:");
			UtilHexToString((const char *) data, strlen(data), tx);
			if (ptr && strncmp(ptr, tx, strlen(tx)) == 0)
			{
				// If we are granted access, get and store the objects received from the host
				strcpy(data, "YES GRANTED");
				UtilHexToString((const char *) data, strlen(data), tx);
				if (strncmp(&ptr[36], tx, strlen(tx)) == 0)
				{
					unsigned char ofb = ptr[61];
					int nextpack = 0;
					char *ptr_rcvdata = NULL;
					int i=0,j=0;
					char * msgptr;
					char header[10] = "";
					int rcvlen = 0;
					char *response =NULL;

					ptr += 62;

					// If OFB encrypted, decrypt first
					if (ofb == '1')
					{
						__iv_set(iv);
						ptr = (char*)__crypt(ptr, "",key_mks,"",true,true);
						__iv_set(NULL);
					}

					ptr_rcvdata = ptr ;
					nextpack = 0;
					__store_objects(1,ptr_rcvdata, &nextpack, &response);
					if(response) {
							int hlen = strlen(response)-2;
							response[0] = hlen / 256; response[1] = hlen % 256;
							inSendTcpRaw(response,hlen+2); 
							UtilStrDup(	&response , NULL);
					} else if(nextpack) {
							inSendTcpRaw( "\x00\x01\x06", 3 );
					}

					for(i=0;nextpack;i++) {
						int iSent = -1;

						__tcp_recv( &msgptr,"2000","10",temp);
						if(strcmp(temp,"NOERROR") ) {
							break;
						}

						__store_objects(1,msgptr, &nextpack,&response);
						if(response) {
							int hlen = strlen(response)-2;
							response[0] = hlen / 256; response[1] = hlen % 256;
							iSent = inSendTcpRaw(response,hlen+2); 
						} else {
							iSent = inSendTcpRaw( "\x00\x01\x06", 3 );
						}
						UtilStrDup(	&response , NULL);
						UtilStrDup(	&msgptr , NULL);
						if(iSent == -1) break;
					}

					UtilStrDup(	&response , NULL);
					if (ofb == '1') UtilStrDup(&ptr,NULL);


				}

				// If this is a new session, update the session keys...
				else
				{
					uchar key[65];

					strcpy(data, "NEW SESSION");
					UtilHexToString((const char *) data, strlen(data), tx);
					if (strncmp(&ptr[36], tx, strlen(tx)) == 0)
					{
						ptr += 60;

						// OWF KEK
						SecurityOWFKey(irisGroup, atoi(key_kek), 16, atoi(key_kek), atoi(key_ppasn), false);

						// If we have a new KEK...
						strcpy(data, "KEK:");
						UtilHexToString((const char *) data, strlen(data), tx);
						if (strncmp(ptr, tx, strlen(tx)) == 0)
						{
							memcpy(key, &ptr[8], 64);
							key[64] = '\0';

							UtilStringToHex((char *) key, 64, (uchar *) tx); tx[32] = '\0';
							UtilStringToHex(tx, 32, key);

							SecurityInjectKey(irisGroup, atoi(key_master), 16, irisGroup, atoi(key_kek), 16, key, "\x82\xC0");
							ptr += 74;
						}

						// If we have a new PPASN...
						strcpy(data, "PPASN:");
						UtilHexToString((const char *) data, strlen(data), tx);
						if (strncmp(ptr, tx, strlen(tx)) == 0)
						{
							memcpy(key, &ptr[12], 32);
							key[32] = '\0';

							UtilStringToHex((char *) key, 32, (uchar *) tx); tx[16] = '\0';
							UtilStringToHex(tx, 16, key);

							SecurityInjectKey(irisGroup, atoi(key_master), 16, irisGroup, atoi(key_ppasn), 8, key, "\x88\x88");
							ptr += 46;

							// OWF Variant of KEK
							SecurityOWFKey(irisGroup, atoi(key_kek), 16, atoi(key_kek), atoi(key_ppasn), true);
						}

						// If we have a new MKs...
						strcpy(data, "MKs:");
						UtilHexToString((const char *) data, strlen(data), tx);
						if (strncmp(ptr, tx, strlen(tx)) == 0)
						{
							memcpy(key, &ptr[8], 64);
							key[64] = '\0';

							UtilStringToHex((char *) key, 64, (uchar *) tx); tx[32] = '\0';
							UtilStringToHex(tx, 32, key);

							SecurityInjectKey(irisGroup, atoi(key_kek), 16, irisGroup, atoi(key_mks), 16, key, "\x22\xC0");
							ptr += 74;
						}

						// If we have a new MKr...
						strcpy(data, "MKr:");
						UtilHexToString((const char *) data, strlen(data), tx);
						if (strncmp(ptr, tx, strlen(tx)) == 0)
						{
							memcpy(key, &ptr[8], 64);
							key[64] = '\0';

							UtilStringToHex((char *) key, 64, (uchar *) tx); tx[32] = '\0';
							UtilStringToHex(tx, 32, key);

							SecurityInjectKey(irisGroup, atoi(key_kek), 16, irisGroup, atoi(key_mkr), 16, key, "\x44\xC0");
						}

						restart = 1;
					}
				}
			}
	 }while(restart);


	if(iConnectType == 1) {
		inCeTcpDisConnectIP(); //close socket
	} else {
		__pstn_disconnect();
	}

	// Clean up
	if (upload) UtilStrDup(&upload, NULL);
	my_free(data);
	UtilStrDup(&ptrtmp,NULL);
	return true;
}

//
//-----------------------------------------------------------------------------
// FUNCTION   : IRIS_GetObjectData
//
// DESCRIPTION:	Look for an object in the file system.
//				If found, read the object into an allocated memory
//
// PARAMETERS:	objectName	<=	The name of the file (object)
//				length		=>	Updated with the object length on exit
//
// RETURNS:		Pointer to an allocated memory structure representing file
//				object data in memory
//
//-----------------------------------------------------------------------------
//

char * IRIS_GetObjectData_all(const char * objectName, unsigned int * length, int jsonfile)
{
	char temp = 0;
	char * data;
	FILE_HANDLE handle;

	if (objectName == NULL || length == NULL) 
		return NULL;

	// Open the file
	handle = open(objectName, FH_RDONLY);
	if (FH_ERR(handle))
	{
		handle = open(objectName, FH_RDONLY);
		if (FH_ERR(handle))
			return NULL;
	}

	// Do a simple check first
	if(jsonfile) {
		read(handle, &temp, 1);
		if (temp != '{') {
			*length = 0;			  // not json file
			close(handle);
			return NULL;
		}
	}
				

	// Read the entire object
	*length = lseek(handle, 0, SEEK_END);
#ifdef _DEBUG
	*length = ftell(handle);
#endif
	lseek(handle, 0, SEEK_SET);

	data = my_malloc((*length)+1);
	read(handle, data, *length);
	if(jsonfile) {
		data[*length] = '\0';
		*length = strlen(data);
	}

	// Just in case there are added data towards the end durign inserts and deletion operations....

	close(handle);

	return data;
}
char * IRIS_GetObjectData(const char * objectName, unsigned int * length)
{
	return(IRIS_GetObjectData_all(objectName, length, true));
}

char * IRIS_GetObjectTagValue(const char *data, const char * intag)
{
	char *str_st = NULL;
	char *str_end = NULL;
	short ok = false;
	short taglen =0;
	char *value=NULL;
	short valuelen;
	char tag[100];
	bool cont = false;
	char *ptr = NULL;

	if (data == NULL || intag == NULL) return NULL;
	ptr = (char*)data;

	sprintf(tag,"%s:", intag);
    taglen = strlen( tag ) ;

	do {
		cont = false;
		str_st = strstr( ptr, tag );
		if(str_st != NULL) {
			char pre_char = *(str_st-1);
			if( (pre_char != ',') && (pre_char != '{')) {
				cont = true;
				ptr += taglen;
			}
	   	 	else {
				char *stmp1 = NULL; char* stmp2 = NULL;
				str_end = NULL;
		 		str_st += taglen  ;
				stmp1 = strchr(str_st,',');
				stmp2 = strchr(str_st,'}');

				if(stmp1 == NULL && stmp2 == NULL ) str_end = NULL;
				else if( stmp2 == NULL ) str_end = stmp1;
				else if( stmp1 == NULL ) str_end = stmp2;
				else if( stmp2 < stmp1) str_end = stmp2;
				else if( stmp1 < stmp2) str_end = stmp1;

		  		if(str_end != NULL && str_end != str_st) ok = true;
			}
		} 
		else break;
	} while(cont);

	if(!ok) {
		return(NULL);
	}


	valuelen = str_end - str_st;
	value = my_malloc( valuelen + 1 );
	if (value) {
		memcpy(value, str_st, valuelen );
		value[valuelen] = '\0';
	}

	return value;
}

//
//-----------------------------------------------------------------------------
// FUNCTION   : IRIS_DeallocateStringValue
//
// DESCRIPTION:	Deallocate the string value completely including its nested arrays
//
//				Examine the buffer pointer to by value and by nested pointers of value...
//				If the buffer ends with a null pointer (ie FOUR zeros), then this is
//				an array of pointers, so traverse one level down first. If not, just
//				deallocate and go back
//
//				THIS IS A RECURSIVE FUNCTION
//
// PARAMETERS:	value	<=	The array or element to my_free
//
// RETURNS:		None
//
//-----------------------------------------------------------------------------
//
void IRIS_DeallocateStringValue(char * value)
{
	// Make sure value is already allocated and not NULL
	if (value == NULL)
		return;

	my_free(value);

}

//
//-----------------------------------------------------------------------------
// FUNCTION   : IRIS_GetStringValue
//
// DESCRIPTION:	Look inside the object passed in 'data' for an string with the name
//				'name' and return its data stored within an allocated memory block
//				for easy parsing.
////
// PARAMETERS:	data	<=	The main object
//				name	<=	The 'string' part of the object.
//
// RETURNS:		Pointer to an allocated memory structure representing the 'value'
//				of the object in question.
//
//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
// FUNCTION   : IRIS_GetStringValue
//
// DESCRIPTION:	Look inside the object passed in 'data' for an string with the name
//				'name' and return its data stored within an allocated memory block
//				for easy parsing.
////
// PARAMETERS:	data	<=	The main object
//				name	<=	The 'string' part of the object.
//
// RETURNS:		Pointer to an allocated memory structure representing the 'value'
//				of the object in question.
//
//-----------------------------------------------------------------------------
//
char * IRIS_GetStringValue(const char * data, int size, const char * name)
{
	char *value= (char *)IRIS_GetObjectTagValue(data, name);
	return value;
}

int IRIS_GetFileInfo(const char *filename, char *gettype, int *filesize, char *filedate, char **filedata)
{
	int iret = 0;	
	*filesize = 0;
	strcpy(filedate,"");
	iret = dir_get_file_date(filename,filedate);
	if(iret ==0 )  {
		*filesize = dir_get_file_size(filename);
		if(filedata) {
			char *pData = IRIS_GetObjectData_all( filename, filesize, false );
			if(strcmp(gettype,"HEX")==0) {
				*filedata = my_calloc( *filesize*2 + 1 );
				UtilHexToString((const char *) pData, *filesize, *filedata);
				my_free(pData);
			} else {
				*filedata = pData ;
			}
			
		}
		return(0);
	}

	return(-1);
}

int IRIS_GetDirInfo(const char *driver,const char *group,const char *filename_p, char **flist)
{
	char l_driver[30]="I:";
	char fname[64]="";
	int iret = 0;
	int i = 0;
	int max_len = 4096;

	if(!flist) return -1;

	if(driver&&strlen(driver)) strcpy(l_driver,driver);
	iret = dir_get_first(l_driver);
	if(iret == 0) {
		*flist = my_malloc(4096);
		**flist = '\0';
		strcpy(fname,l_driver);
		if(filename_p==NULL || strlen(filename_p)==0)
			strcat(*flist,l_driver);
		else if( strncmp(fname,filename_p,strlen(filename_p)==0))
			strcat(*flist,l_driver);
		for(i=0;;i++) {
			iret = dir_get_next(fname);
			if(iret) break;
			if(filename_p==NULL || strlen(filename_p)==0) {
				strcat(*flist,",");
				strcat(*flist,fname);
			}
			else if( strncmp(fname,filename_p,strlen(filename_p))==0) {
				strcat(*flist,",");
				strcat(*flist,fname);
			}
		}
	}

	return(0);
}
