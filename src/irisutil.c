//
//-----------------------------------------------------------------------------
// PROJECT:			iRIS
//
// FILE NAME:       irisutil.c
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
#include <errno.h>

#ifdef __VMAC
#	include "eeslapi.h"
#	include "logsys.h"
#	include "devman.h"
#	include "vmac.h"
#	include "version.h"
#endif


//
// Local include files
//
#include "alloc.h"
#include "zlib.h"
#include "as2805.h"
#include "security.h"
#include "utility.h"
#include "iris.h"
#include "irisfunc.h"
#include "display.h"

#ifdef __EMV
#include "emv.h"
#endif

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

char lastFaultyFileName[100];
char lastFaultyGroup[50];

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
// GENERAL iRIS UTILITY FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
void __tcp_disconnect_do(void);

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()LOCATE
//
// DESCRIPTION:	Locates and updates the array index
//
// PARAMETERS:	index	=>	The index to update starting from zero until count
//				first	<=	The first string to compare its value to
//				second	<=	The second string to compare its value to
//				wildcard<=	Skip n character from the "second" at the end of the "second" string.
//							If wildcard is "CPAT", then the second is treated as "from-to-length".
//							If wildcard is "MIN", then return the one with the least value
//							If wildcard is "MAX", then return the one with the least value
//
// RETURNS:		None
//-------------------------------------------------------------------------------------------
//

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()TEXT_TABLE
//
// DESCRIPTION:	Retrieves the text string from the text table
//
// PARAMETERS:	name	<=	Text table object name. This must be of type: TEXT TABLE.
//				string	<=	A string whose value will be compared to the first value of each array
//							When a match occurs, the search stops and the second value is 
//							returned from the array
//
// RETURNS:		The second value from the array but only if found
//-------------------------------------------------------------------------------------------


void __text_table(const char* search, const char* textTableObjectName, char** pfound)
{
	long num1 = UtilStringToNumber(search);
	static char * currentTableName = NULL;
	static char * currentTableData = NULL;
	static uint currentTableLength;
	char *value = NULL;

	if (search && textTableObjectName)
	{
		// Obtain the object data if not already obtained
		if (currentTableData == NULL || currentTableName == NULL || strcmp(textTableObjectName, currentTableName))
		{
			UtilStrDup(&currentTableName, textTableObjectName);
			UtilStrDup(&currentTableData, NULL);
			currentTableData = IRIS_GetObjectData(currentTableName, &currentTableLength);
		}

		// Given an object data
		if (currentTableData)
		{
			// Search for string "TYPE" and return its value in an allocated tree structures "array of arrays where the leaves are simple values"
			value = IRIS_GetStringValue(currentTableData, currentTableLength, search );
			UtilStrDup(pfound, value);
			IRIS_DeallocateStringValue(value);
		}
	}

	// Push the result onto the stack if available
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()DECOMPRESS
//
// DESCRIPTION:	Decompress the data supplied, frees it and returns an uncompressed buffer
//
// PARAMETERS:	None
//
// RETURNS:		None
//-------------------------------------------------------------------------------------------
//
static unsigned char * decompress(unsigned char * input, unsigned long outSize, unsigned long size)
{
	unsigned char * output;
	z_stream d_stream;
	int err;

	// Allocate an output buffer
	output = my_malloc(outSize+50);	// Extra memory just in case the decompression uses more memory becuase of a fault

	// Initialisation
	memset(&d_stream, 0, sizeof(d_stream));
	d_stream.next_in = input;
	d_stream.next_out = output;
	d_stream.zalloc = (alloc_func)0;
	d_stream.zfree = (free_func)0;
	d_stream.opaque = (voidpf)0;
	d_stream.avail_in = 0;
	err = inflateInit(&d_stream);

	// Process the compressed stream
	while (d_stream.total_in < size)
	{
		d_stream.avail_in = d_stream.avail_out = 1; /* force small buffers */
		err = inflate(&d_stream, Z_NO_FLUSH);
		if (err == Z_STREAM_END) break;
		if (err != Z_OK) break;
	}

	// Finalise the decompression
	err = inflateEnd(&d_stream);

	// Return the uncompressed output
	return output;
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()STORE_OBJECTS
//
// DESCRIPTION:	A privilaged function. It can only be used by the iRIS group
//
// PARAMETERS:	None
//
// RETURNS:		None
//-------------------------------------------------------------------------------------------
//
static uint getNextObject(char * objects, uint * index, uint objectsLength)
{
	char data;
	unsigned int length = 0;
	int marker = 0;

	for (;(*index+length) < objectsLength;)
	{
		// Identify the byte we are examining
		data = objects[*index+length];

		// Detect the start of the object
		if (marker == 0 && data != '{')
			(*index)++;

		// If we detect the start of an object, increment the object marker
		if (data == '{')
			marker++;

		// If the marker is detected, start accounting for the json object
		if (marker)
			length++;

		// If we detect the end of an object, decrement the marker
		if (data == '}')
		{
			marker--;
			if (marker < 0)
			{
				// Incorrectly formatted JSON object. Found end of object without finding beginning of object
				return 0;
			}
			if (marker == 0)
				break;
		}
	}

	if (length)
		return length;

	return 0;
}

void DoTmsCmd(const char *jsonobj)
{
	uint length;
	char *objects = NULL;
	const char *jsonfile = "TMS_CMDTODO";


	if(jsonobj && strlen(jsonobj)) objects = (char *)jsonobj;
	else objects = IRIS_GetObjectData(jsonfile, &length);

	if(objects ==NULL) {
		return ;
	}

	_remove(jsonfile);
	//it might SHUTDOWN() in the following store process
	__store_objects(0,objects,NULL,NULL);

	if(jsonobj == NULL || strlen(jsonobj)==0) {
		my_free(objects);
	}
}

void __store_objects(int unzip,char *objects,int* nextmsg,char **response)
{
	uint index;
	uint count = 0;
	char temp[10];
	uint length;
	unsigned long size, outSize;
	char * objects_hex;
	bool endflag = false;

	// Handle null errors
	if ( !objects || strcmp(currentObjectGroup, irisGroup))
	{
		return;
	}

	// Convert from string to hex
	if(unzip) {
		size = strlen(objects)/2;
		objects_hex = my_calloc(size + 1);
		UtilStringToHex(objects, strlen(objects), (unsigned char *) objects_hex);
		objects = objects_hex + 8;
		size -= 8;
		outSize = atol(objects_hex);

	// Decompress the objects first
		objects = (char *) decompress((unsigned char *) objects, outSize, size);
		my_free(objects_hex);
	}

	// Get one object at a time
    //endflag = ( strstr(objects,"{END_OBJ}") != NULL) ;
    endflag = true;
	for (index = 0; endflag && (length = getNextObject(objects, &index, strlen(objects))) != 0; index += length, count++)
	{
		// Get the object type
		char * type = IRIS_GetStringValue(&objects[index], length, "TYPE" );

		// If type does not exist, reject the object
		if (type == NULL)
			continue;

		// If type is a display object, then authenticate the signature of the file
		if (strcmp(type, "DISPLAY") == 0 || strcmp(type, "TEXT TABLE") == 0 || strcmp(type, "IMAGE") == 0 || strcmp(type, "PROFILE") == 0)
		{
			IRIS_PutObjectData(&objects[index], length);
		}

		// If this is a configuration or data object, store it without further authentication. It is not executable.
		// An IMAGE to print that is not signed should be of TYPE:DATA.
		else if (strncmp(type, "CONFIG", 6) == 0 || strcmp(type, "DATA") == 0)
		{
			// Store the object
			IRIS_PutObjectData(&objects[index], length);
		}
		else if (strncmp(type, "EMV", 3) == 0)
		{

		}

		// If this is an EXPIRY command, then lose the object or group of objects
		else if (strcmp(type, "EXPIRE") == 0)
		{
			char * objectName;
			char * groupName, * groupValue, * groupType;

			// Get the object name then remove it from the terminal
			if ((objectName = IRIS_GetStringValue(&objects[index], length, "NAME" )) != NULL)
			{
					_remove(objectName);
				IRIS_DeallocateStringValue(objectName);
			}

			// Get the group name, then remove all objects of that belong to the specified group
			if ((groupName = IRIS_GetStringValue(&objects[index], length, "GROUPNAME" )) != NULL)
			{
				if ((groupValue = IRIS_GetStringValue(&objects[index], length, "GROUPVALUE" )) != NULL)
				{
					{
						int i, j;
						char fileName[50];
						char oldFileName[50];
						char * data;
						uint len;
						char * ptr;

						groupType = IRIS_GetStringValue(&objects[index], length, "GROUPTYPE" );

						for (j = 0; j < 2; j++)
						{
							strcpy(fileName, j?"I:":"F:");
							strcpy(oldFileName, j?"I:":"F:");
							for (i = dir_get_first(fileName); i == 0; i = dir_get_next(fileName))
							{
								if ((data = IRIS_GetObjectData(fileName, &len)) != NULL)
								{
									if (data[0] == '{' && data[len-1] == '}' && (ptr = strstr(data, groupName)) != NULL &&
										(ptr[-1] == ',' || ptr[-1] == '{') && strncmp(&ptr[strlen(groupName)+1], groupValue, strlen(groupValue)) == 0 &&
										(groupType == NULL || groupType[0] != '\0' || ((ptr = strstr(data, "TYPE:")) != NULL &&
										(ptr[-1] == ',' || ptr[-1] == '{') && strncmp(&ptr[5], groupType, strlen(groupType)) == 0)))
									{
										_remove(fileName);
										strcpy(fileName, oldFileName);
									}
									else strcpy(oldFileName, fileName);
									my_free(data);
								}
							}
						}

						if (groupType) IRIS_DeallocateStringValue(groupType);
					}

					IRIS_DeallocateStringValue(groupValue);
				}

				IRIS_DeallocateStringValue(groupName);
			}
		}

		// If this is an SHUTDOWN command, then reboot the terminal
		else if (strcmp(type, "SHUTDOWN") == 0)
#ifdef _DEBUG
			exit(0);
#else
		{
			__tcp_disconnect_do();
			SVC_WAIT(500);
			SVC_RESTART("");
		}
#endif

		// If this an environment variable setting, then set the environment variable
		else if (strcmp(type, "ENV") == 0)
		{
			char * key;
			char * value;
			char * gid;
			int gid_orig = -1;

			// Get the file name
			if ((key = IRIS_GetStringValue(&objects[index], length, "KEY" )) != NULL)
			{
				// Get the file data and create or override the file
				if ((value = IRIS_GetStringValue(&objects[index], length, "VALUE" )) != NULL)
				{
						if ((gid = IRIS_GetStringValue(&objects[index], length, "GID" )) != NULL)
						{
#ifndef _DEBUG
							{
								gid_orig = get_group();
								set_group(atoi(gid));
							}
#endif
							IRIS_DeallocateStringValue(gid);
						}
#ifndef _DEBUG
						put_env(key, value, strlen(value));
						if (gid_orig != -1)
							set_group(gid_orig);
#endif

					IRIS_DeallocateStringValue(value);
				}

				IRIS_DeallocateStringValue(key);
			}
		}

		// If this is an binary file upgrade, then store store or override the file
		else if (strcmp(type, "FILE") == 0)
		{
			char * fileName;
			char * fileData;

			// Get the file name
			if ((fileName = IRIS_GetStringValue(&objects[index], length, "NAME" )) != NULL)
			{
				// Get the file data and create or override the file
				if ( (fileData = IRIS_GetStringValue(&objects[index], length, "DATA" )) != NULL)
				{
#ifdef _DEBUG
					FILE * fp;
#else
					int handle;
#endif
					char * hexData;
					unsigned int hexSize;

					{
						// Convert the file data to hex and store it in the file
						hexSize = strlen(fileData) / 2 + 1;
						hexData = my_malloc(hexSize);
						hexSize = UtilStringToHex(fileData, strlen(fileData), (uchar *) hexData);

#ifdef _DEBUG
						// Get a file pointer to store the new file
						if ((fp = fopen(fileName, "w+b")) != NULL)
						{
							fwrite(hexData, 1, hexSize, fp);
							fclose(fp);
						}
#else
						if ((handle = open(fileName, O_CREAT | O_TRUNC | O_WRONLY | O_APPEND | O_CODEFILE)) != -1)
						{
							write(handle, hexData, hexSize);
							close(handle);
						}
#endif

						my_free(hexData);
					}

					IRIS_DeallocateStringValue(fileData);
				}

				IRIS_DeallocateStringValue(fileName);
			}
		}

		// If this is a binary file upgrade, then store store or override the file
		else if (strcmp(type, "MERGE") == 0 || strcmp(type, "COMBINE_FILE") == 0)
		{
			char * fileName;
			char * fileMax;


			// Get the proper file name
			if ((fileName = IRIS_GetStringValue(&objects[index], length, "NAME" )) != NULL)
			{
				uint len;

				// Make sure it is a simple file name value
				if ((fileMax = IRIS_GetStringValue(&objects[index], length, "TOTAL")) != NULL)
				{
					int i;
					int indexlen = 3;
#ifdef _DEBUG
#else
					int whandle;
					char *s_indexlen = IRIS_GetStringValue(&objects[index], length, "INDEXLEN");
					char temp[50]="";

					if (s_indexlen) indexlen = atoi(s_indexlen);

					// We must have all the files or we do not merge
					for (i = 0; i < atoi(fileMax); i++)
					{
						sprintf(temp, "%s_%0*d", fileName, indexlen , i);
						if (dir_get_file_sz(temp) == -1)
							break;
					}
					if( i< atoi(fileMax) && response ) {
						char stmp[256] = "";
						sprintf(stmp, "00{TYPE:RESPONSE,ACTION:COMBINE_FILE,RESULT:NOK,ERRORCODE:%d,ERRSTR:FILE %s NOT FOUND}",i,temp);
						*response = my_malloc(1024);
						strcpy(*response,stmp);
					}

					if (i == atoi(fileMax))
					{
						char wfilename[64];
						char *drive = NULL;

						if ((drive = IRIS_GetStringValue(&objects[index], length, "DRIVE")) != NULL) {
							sprintf(wfilename,"%s:%s", drive, fileName );
							IRIS_DeallocateStringValue(drive);
						} else 
							sprintf(wfilename,"%s", fileName );

						whandle = open(wfilename, O_CREAT | O_TRUNC | O_WRONLY | O_APPEND | O_CODEFILE);


						// Join the files together
						for (i = 0; whandle != -1; i++)
						{
							int handle;
							char temp[50];
							unsigned char * data;
							sprintf(temp, "%s_%0*d", fileName, indexlen, i);

							// Read the partial file
							if (i == atoi(fileMax) || (handle = open(temp, O_RDONLY)) == -1)
							{
								char stmp[256] = "";
								sprintf(stmp, "00{TYPE:RESPONSE,ACTION:COMBINE_FILE,RESULT:OK,FILENAME:%s}",wfilename);
								*response = my_malloc(1024);
								strcpy(*response,stmp);
								close(whandle);
								break;
							}

							// Get the file size
							len = dir_get_file_sz(temp);

							// Read the partial file
							data = my_malloc(len);
							read(handle, (char *) data, len);
							close(handle);
							_remove(temp);

							if(data && strncmp((const char *)data,"{TYPE",5)==0) {
								char *sData = IRIS_GetStringValue((const char *)data,len,"DATA");
								char *sIndex = IRIS_GetStringValue((const char *)data,len,"INDEX");
								char *sEncode = IRIS_GetStringValue((const char *)data,len,"ENCODE");

								if(sEncode == NULL) {
									write(whandle, (char *) data, len);
								} else if(strcmp(sEncode ,"BASE24")==0) { //TODO
									write(whandle, (char *) data, len);
								} else if(strcmp(sEncode ,"HEX")==0) {
									char *sData_x = my_malloc( strlen(sData) / 2 + 1 );
									int hexlen = UtilStringToHex( sData,strlen(sData), (uchar *)sData_x);
								   	write(whandle, (char *) sData_x, hexlen);
									my_free(sData_x);
								} else if(strcmp(sEncode ,"PUREASCII")==0) {
								   	write(whandle, (char *) sData, strlen(sData)+1);
								}

								IRIS_DeallocateStringValue(sData);
								IRIS_DeallocateStringValue(sIndex);
								IRIS_DeallocateStringValue(sEncode);
							} else 
								write(whandle, (char *) data, len);
							my_free(data);
						}
					}
#endif
					IRIS_DeallocateStringValue(fileMax);
				}

				IRIS_DeallocateStringValue(fileName);
			}
		}
		else if (strcmp(type, "REPLACE") == 0)
		{
			char *obj = my_malloc(length+10);
			char * ptr;
			char * ptmp;
			char *position_f = "<POSITION>";
			char *delete_f = "<DELETE>";
			char *insert_s = "<INSERT_START>:";
			char *insert_e = "<INSERT_END>";
			char *fileName;
			char fname[30] = "";
			FILE_HANDLE handle;
			bool hex = false;


			memset(obj,0,sizeof(obj));
			memcpy(obj,&objects[index],length);

			fileName = IRIS_GetStringValue(&objects[index], length, "NAME" );
			if(fileName==NULL) {
				IRIS_DeallocateStringValue(type);
				my_free(obj);
				continue;
			}
			strcpy(fname,fileName);
			IRIS_DeallocateStringValue(fileName);

			ptmp = IRIS_GetStringValue(&objects[index], length, "HEX" );
			if(ptmp && strncmp(ptmp,"YES",3) == 0)
				hex = true;

			handle = open(fname, O_RDWR);
			if(handle == 0 ){
				IRIS_DeallocateStringValue(type);
				my_free(obj);
				continue;
			}
			
			ptr = obj;

			while(1) {
				long del_cnt = 0;
				char* insert_ptr = NULL;
				char* insertend_ptr = NULL;
				char *ptmp2;
				char keep ;
				ptmp = NULL;

				if( (ptmp2 = strstr(ptr, position_f))!=NULL) {
					ptmp = (char*)IRIS_GetObjectTagValue(ptr, position_f);
				} else break;
				ptr = ptmp2 + strlen(position_f);

				if(strcmp(ptmp,"END") == 0)
				  lseek( handle, 0, SEEK_END);
				else
				  lseek( handle, atol(ptmp), SEEK_SET);
				my_free(ptmp);

				if((ptmp2 = strstr(ptr, delete_f))!=NULL) {
					ptmp = (char*)IRIS_GetObjectTagValue(ptr, delete_f);
					if(ptmp && strcmp(ptmp,"ALL")==0){
						long fsize = 0;
						get_file_size(handle,&fsize);
						del_cnt = fsize;
					}
					else if(ptmp) del_cnt = atol(ptmp);

					my_free(ptmp);
					if(del_cnt) delete_(handle,del_cnt);
					ptr = ptmp2 + strlen( delete_f);
				}

				if((ptmp2 = strstr(ptr, insert_s))!=NULL) {
					insert_ptr = ptmp2 + strlen( insert_s);
					if((ptmp2 = strstr(ptr, insert_e))!=NULL) {
						insertend_ptr = ptmp2;
						keep = *insertend_ptr ; // should be '<'
						*insertend_ptr = '\0';
						if(!hex) {
							insert(handle, insert_ptr, strlen(insert_ptr));
						}
						else{
							uchar *hex_s = my_malloc(strlen(insert_ptr)/2 + 1);
							int len = UtilStringToHex( insert_ptr, strlen(insert_ptr),hex_s);
							insert(handle, (const char *)hex_s, len);
							my_free(hex_s);
						}
						*insertend_ptr = keep;
					}
					ptr = ptmp2 ;
				}
			}

			close(handle);
			my_free(obj);

		}
		else if (strcmp(type, "SETJSON") == 0)
		{
			char *jname = IRIS_GetStringValue(&objects[index], length, "NAME" );
			char *jtag = IRIS_GetStringValue(&objects[index], length, "TAG" );
			char *jvalue = IRIS_GetStringValue(&objects[index], length, "VALUE" );
			if(jname && jtag && jvalue) {
				IRIS_StoreData(jname,jtag, jvalue, 0/* false */);
			}
			IRIS_DeallocateStringValue(jname);
			IRIS_DeallocateStringValue(jtag);
			IRIS_DeallocateStringValue(jvalue);
		}
		else if (strcmp(type, "FILE_PART") == 0)
		{
			// The name should include index
			char *s_name = IRIS_GetStringValue(&objects[index], length, "NAME" );
			char *s_index = IRIS_GetStringValue(&objects[index], length, "INDEX" );
			char stmp[64]="";

			if(s_name && s_index ) {
				sprintf(stmp, "%s_%s", s_name,s_index);
				// Store the object
				IRIS_PutNamedObjectData(&objects[index], length,stmp);
			}
			IRIS_DeallocateStringValue(s_name);
			IRIS_DeallocateStringValue(s_index);
		}
		else if (strcmp(type, "DISPMSG") == 0)
		{
			char tag_start[] = "<DISPLINE_START>";
			char tag_end[] = "<DISPLINE_END>";
			// The name should include index
			char *s_start = strstr(&objects[index], tag_start);
			char *s_end =  strstr(&objects[index], tag_end);

			if(s_start && s_end ) {
				char stmp[1024]="";
				s_start = s_start + strlen(tag_start);
				strncpy(stmp,s_start,s_end-s_start);
				stmp[s_end-s_start] = '\0';
				DisplayObject(stmp,0,0,0,NULL,NULL);
			}
		}
		else if (strcmp(type, "RUN_LUA_STRING") == 0)
		{
			char tag_start[] = "<LUA_START>";
			char tag_end[] = "<LUA_END>";
			// The name should include index
			char *s_start = strstr(&objects[index], tag_start);
			char *s_end =  strstr(&objects[index], tag_end);
			char *respfile = NULL;
			int respfile_len = 0;
			char respfile_name[] = "TMS_RESPFILE";

			if(s_start && s_end ) {
				int luaDoString( char *slua);
				char *stmp = NULL;

				s_start = s_start + strlen(tag_start);
				stmp = my_malloc( s_end-s_start + 1 );
				strncpy(stmp,s_start,s_end-s_start);
				stmp[s_end-s_start] = '\0';

				luaDoString(stmp);
				my_free(stmp);
			}

			if( response ) {
					respfile = IRIS_GetObjectData( respfile_name,(uint *) &respfile_len);
					if(respfile) {
							int max_len = 4096;
							char stmp[4096];
							memset(stmp,0,sizeof(stmp));
							*response = my_malloc(max_len + 2);
							if(respfile_len<max_len) strcpy( stmp, respfile);
							else { strncpy(stmp,respfile,max_len-5); strcat(stmp,"..}");}

							sprintf(*response,"00%s",stmp);
							IRIS_DeallocateStringValue(respfile);
					}
			}
		}

		if(nextmsg) {
			char *s_nextmsg = IRIS_GetStringValue(&objects[index], length, "NEXTMSG" );
			if(s_nextmsg && atoi(s_nextmsg) == 1) {
				*nextmsg = 1;
			} else *nextmsg = 0;
			IRIS_DeallocateStringValue(s_nextmsg);
		}
		// House keep
		IRIS_DeallocateStringValue(type);
	}

	if(unzip) my_free(objects);
	sprintf(temp, "%d", count);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()UPLOAD_MSG
//
// DESCRIPTION:	Requests an upload of a resource to the iRIS host
//
// PARAMETERS:	None
//
// RETURNS:		None
//-------------------------------------------------------------------------------------------
//

void __upload_msg(const char *message)
{
	IRIS_AppendToUpload(message);
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()UPLOAD_OBJ
//
// DESCRIPTION:	Adds an object to the upload message
//
// PARAMETERS:	None
//
// RETURNS:		None
//-------------------------------------------------------------------------------------------
//

void __upload_obj(const char *objectName)
{
	uint objectLength;
	char * objectData;

	// Make sure the object name is available
	if (objectName)
	{
		// Get the object data
		if ((objectData = IRIS_GetObjectData(objectName, &objectLength)) != NULL)
		{
			// Add it to the upload
			IRIS_AppendToUpload((const char *)objectData);

			// Lose the object Data. Not needed any longer
			UtilStrDup(&objectData, NULL);
		}
	}

}


//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()REMOTE
//
// DESCRIPTION:	Initiate a remote session to upload and download
//
// PARAMETERS:	None
//
// RETURNS:		None
//-------------------------------------------------------------------------------------------
//
bool __remote()
{
	// Use the system utility to do so.
	return(remoteTms());

}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()BATTERY_STATUS
//
// DESCRIPTION:	Returns the current battery status
//
// PARAMETERS:	None
//
// RETURNS:		None
//-------------------------------------------------------------------------------------------
//
void __battery_status(void)
{
#ifndef __VX670
#else
	int status = get_battery_sts();

#endif
}

//
//-------------------------------------------------------------------------------------------
// FUNCTION   : ()DOCK_STATUS
//
// DESCRIPTION:	Returns the current battery status
//
// PARAMETERS:	None
//
// RETURNS:		None
//-------------------------------------------------------------------------------------------
//
void __dock_status(void)
{
#ifndef __VX670
#else
	int dock = get_dock_sts();
#endif
}

char* get_csv_str( char * line, char sep, int idx , char *result)
{
	char *p = line;
	char *q;
	char tmps[256]="";
	int i ;

	if(line==NULL)  {
		strcpy(result,tmps);
		return (result);
	}

	for(i = 0 ; i < idx && *p ; i ++ ) {
	  p = strchr(p,sep) + 1;
	}

	if(p) {
	  for(q=p; *q != 0 && *q != sep; q++) ;
	  if(q) {
			memcpy(tmps,p,q-p);
	  		tmps[q-p] = 0;
	  }
	}

	strcpy(result,tmps);
	return(result);
}

bool __luhn(const char *pan)
{
	int luhn=0, i, j;

	if (pan)
	{
		for (luhn = 0, i = strlen(pan)-1, j = 0; i >= 0; i--, j++)
		{
			unsigned char digit = pan[i]-'0';
			luhn += j%2? (digit>=5? (digit%5)*2+1:digit*2):digit;
		}
	}

	if (luhn%10 || !pan)
		return false;
	else
		return true;
}
