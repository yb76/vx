#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef WIN32
	#include <io.h>
#endif

#include <fcntl.h>
#include <string.h>

#include "getopt.h"

#ifdef WIN32
	#include <winsock2.h>
#endif

#ifdef USE_MYSQL
	#include <mysql.h>
	#define	mysql_error(mysql, res)	mysql_error(mysql)
#else
	#define	MYSQL_RES				PGresult
	#define	MYSQL					PGconn
	#define	mysql_error(mysql, res)	PQresultErrorMessage(res)
	#define	mysql_close(mysql)		PQfinish(mysql)
	#include <libpq-fe.h>
#endif

#include "sha1.h"

char * host = "localhost";
char * user = "root";
char * password = "password.01";
char * unix_socket = NULL;

typedef enum
{
	STATE_NEW_LINE,
	STATE_COMMENT_1,
	STATE_COMMENT_2,
	STATE_SKIP_BLANK,
	STATE_ECHO,
} E_STATE;

void compactjson(char * sourceFile, char * outFileName)
{
	FILE * rdFile, * wrFile;
	unsigned char data;
	int state = STATE_NEW_LINE;

	if ((rdFile = fopen(sourceFile, "rb")) == NULL)
	{
		printf( "Failed to open source file: %s\n", sourceFile);
		exit(-3);
	}

	wrFile = fopen(outFileName, "w+b");

	while(fread(&data, 1, 1, rdFile) == 1)
	{
		switch(data)
		{
			case '\n':
				state = STATE_NEW_LINE;
				break;
			case '/':
				if (state == STATE_NEW_LINE)
					state = STATE_COMMENT_1;
				else if (state == STATE_COMMENT_1)
					state = STATE_COMMENT_2;
				else
					state = STATE_ECHO;
				break;
			case '\\':
				{
					char temp = '\\';
					fwrite(&temp, 1, 1, wrFile);
				}
				state = STATE_ECHO;
				break;
//#ifndef WIN32
			case '\'':
				{
					char temp = '\\';
					fwrite(&temp, 1, 1, wrFile);
				}
				state = STATE_ECHO;
				break;
//#endif
			case '\r':
			case '\t':
				if (state != STATE_COMMENT_2)
					state = STATE_SKIP_BLANK;
				break;
			default:
				if (state == STATE_COMMENT_1)
				{
					char data = '/';
					fwrite(&data, 1, 1, wrFile);
				}

				if (state != STATE_COMMENT_2)
					state = STATE_ECHO;
				break;
		}

		// Write the data if it is in ECHO mode
		if (state  == STATE_ECHO)
			fwrite(&data, 1, 1, wrFile);
	}

	// House keep
	fclose(rdFile);
	fclose(wrFile);
}

static int getObjectField(char * data, int count, char * field, char ** srcPtr, const char * tag)
{
	static char * ptr;
	int i = 0;
	int j = 0;

	if (srcPtr)
	{
		if ((*srcPtr = strstr(data, tag)) == NULL)
			return 0;
		else
			return (strlen(*srcPtr));
	}

	// Extract the TYPE
	if ((ptr = strstr(data, tag)) != NULL)
	{
		for (--count; ptr[i+strlen(tag)] != ',' && ptr[i+strlen(tag)] != ']' && ptr[i+strlen(tag)] != '}' ; i++)
		{
			for(; count; count--, i++)
			{
				for(; ptr[i+strlen(tag)] != ','; i++);
			}
			field[j++] = ptr[i+strlen(tag)];
		}
	}

	field[j] = '\0';

	return strlen(field);
}

int getNextObject(FILE * fp, char ** pType, char ** pName, char ** pVersion, char ** pJson)
{
	const char signSearch[] = "SIGN:0000000000000000000000000000000000000000";
	sha1_context ctx;
	unsigned char digest[20];
	unsigned int signLocation = 0;	// This is an invalid initial value sinze an object must at least start with '{'.
	FILE * wrfp;
	char escape = 0;

	char fileName[100];
	static char type[100];
	static char name[100];
	static char version[100];
	static char json[9999];
	char * trigger;
	char data;
	unsigned int length = 0;
	int marker = 0;

	// Initialisation
	json[0] = '\0';
	*pType = type;
	*pName = name;
	*pVersion = version;
	*pJson = json;

	// Start SHA-1 calculation
	sha1_starts(&ctx);

	while (fread(&data, 1, 1, fp) == 1)
	{
		// Update the sha-1 calculation
		if (data == '\\' && escape == 0)
			escape = data;
		else
		{
			if (escape && data != '\'')
				sha1_update(&ctx, &escape, 1);
			sha1_update(&ctx, &data, 1);
			escape = 0;
		}

		// If the marker is detected, start storing the JSON object
		if (marker || data == '{')
			json[length++] = data;

		// Detect the signature marker
		if (signLocation == 0 && length > strlen(signSearch))
		{
			if (memcmp(&json[length - strlen(signSearch)], signSearch, strlen(signSearch)) == 0)
				signLocation = length - strlen(signSearch) + 5;	// point just after "SIGN:"
		}

		// If we detect the start of an object, increment the object marker
		if (data == '{')
			marker++;

		// If we detect the end of an object, decrement the marker
		else if (data == '}')
		{
			marker--;
			if (marker < 0)
			{
				printf("Incorrectly formated JSON object. Found end of object without finding beginning of object\n");
				return -1;
			}
			if (marker == 0)
				break;
		}
	}

	// If the object exist...
	if (length)
	{
		int i;
		char keep = json[signLocation + 40];

		// Get the sha-1 digest
		sha1_finish(&ctx, digest);

		// Update the SHA-1 signature...
		for (i = 0; signLocation && i < sizeof(digest); i++)
			sprintf(&json[signLocation + i*2], "%02.2X", digest[i]);
		json[signLocation + 40] = keep;

		// Termine the "json" string
		json[length] = '\0';

		// Check if the object is a trigger object
		if (getObjectField(json, 1, NULL, &trigger, "TRIGGER:") > 0)
		{
			getObjectField(trigger, 1, type, NULL, "[");
			getObjectField(trigger, 2, name, NULL, "[");
			getObjectField(trigger, 3, version, NULL, "[");

			return 1;
		}
		else
		{
			// Extract the type field
			getObjectField(json, 1, type, NULL, "TYPE:");

			// Extract the name field
			getObjectField(json, 1, name, NULL, "NAME:");

			// Extract the version field
			getObjectField(json, 1, version, NULL, "VERSION:");

			// Create a file to send to a secure location in order to encrypt the signature but only if it wasn't already signed
			if (strcmp(&name[strlen(name)-2], ".s") && strcmp(&name[strlen(name)-5], ".json") )
			{
				unsigned int i;
				int skip = 0;

				sprintf(fileName, "%s.json", name);
				wrfp = fopen(fileName, "w+b");
				for (i = 0; i < strlen(json); i++)
				{
					if (json[i] == '\\' && skip == 0)
						skip = 1;
					else
					{
						skip = 0;
						fwrite(&json[i], 1, 1, wrfp);
					}
				}
				fclose(wrfp);
			}

			return 0;
		}

	}

	return -1;
}

static updateObject(MYSQL * mysql, char * query, char * json, char * type, char * name, char * version)
{
	MYSQL_RES * res;

	// Check if the object exists first by preparing a SELECT query
	sprintf(query, "SELECT COUNT(*) FROM object WHERE type=\'%s\' AND name=\'%s\' AND version=\'%s\'", type, name, version);
	printf("SELECT COUNT(*) FROM object WHERE type=\'%s\' AND name=\'%s\' AND version=\'%s\'\n", type, name, version);

	#ifdef USE_MYSQL
		if (mysql_real_query(mysql, query, strlen(query)) == 0) // success
		{
			MYSQL_ROW row;

			query[0] = '\0';

			if (res = mysql_store_result(mysql))
			{
				if (row = mysql_fetch_row(res))
				{
					if (atoi(row[0]) != 0)
					{
						// Prepare the UPDATE query
						sprintf(query, "UPDATE object SET version=\'%s\', json=\'%s\' WHERE type=\'%s\' AND name=\'%s\' AND version=\'%s\'", version, json, type, name, version);
					}
				}
				mysql_free_result(res);
			}
		}
	#else
		if (res = PQexec(mysql, query))
		{
			query[0] = '\0';

			if (PQresultStatus(res) == PGRES_TUPLES_OK)
			{
				if (PQntuples(res) > 0)
				{
					if (atoi(PQgetvalue(res, 0, 0)) != 0)
					{
						sprintf(query, "UPDATE object SET version=\'%s\', json=\'%s\' WHERE type=\'%s\' AND name=\'%s\' AND version=\'%s\'", version, json, type, name, version);
						printf("UPDATE object SET version=\'%s\', json=\'%s\' WHERE type=\'%s\' AND name=\'%s\' AND version=\'%s\'\n", version, "json", type, name, version);
					}
				}
			}

			PQclear(res);
		}
	#endif

	// Prepare the INSERT query
	if (query[0] == '\0')
	{
		sprintf(query, "INSERT INTO object values(\'%s\',\'%s\',\'%s\',\'%s\',default)", type, version, name, json);
		printf("INSERT INTO object values(\'%s\',\'%s\',\'%s\',\'%s\',default)\n", type, version, name, "json");
	}

	//  Add the object
	#ifdef USE_MYSQL
		if (mysql_real_query(mysql, query, strlen(query)) == 0) // success
	#else
		if (PQresultStatus((res = PQexec(mysql, query))) == PGRES_COMMAND_OK)
	#endif
			printf( "Object ==> Type:%s, Name:%s, Version:%s %s\n", type, name, version, query[0] == 'I'?"**ADDED**":"**UPDATED**");
		else
			printf( "Failed to insert/update object.  Error: %s\n", mysql_error(mysql, res));

	#ifndef USE_MYSQL
		if (res) PQclear(res);
	#endif
}

static int existTrigger(MYSQL * mysql, char * query, char * type, char * name, char * version, char * event, char * value)
{
	MYSQL_RES * res;
	int id = 0;

	// Check if the object exists first by preparing a SELECT query
	sprintf(query, "SELECT ID FROM triggerr WHERE type=\'%s\' AND name=\'%s\' AND version=\'%s\' AND event=\'%s\' AND value=\'%s\'", type, name, version, event, value);

	#ifdef USE_MYSQL
		if (mysql_real_query(mysql, query, strlen(query)) == 0) // success
		{
			MYSQL_ROW row;

			query[0] = '\0';

			if (res = mysql_store_result(mysql))
			{
				if (row = mysql_fetch_row(res))
				{
					id = atoi(row[0]);
				}
				mysql_free_result(res);
			}
		}
	#else
		if (res = PQexec(mysql, query))
		{
			if (PQresultStatus(res) == PGRES_TUPLES_OK)
			{
				if (PQntuples(res) > 0)
					id = atoi(PQgetvalue(res, 0, 0));
			}

			PQclear(res);
		}
	#endif

	return id;
}

static int updateTrigger(MYSQL * mysql, char * query, char * type, char * name, char * version, char * event, char * value)
{
	#ifndef USE_MYSQL
		MYSQL_RES * res;
	#endif
	int id = existTrigger(mysql, query, type, name, version, event, value);

	if (id == 0)
	{
		// Check if the object exists first by preparing a SELECT query
		sprintf(query, "INSERT INTO triggerr values(default, \'%s\',\'%s\',\'%s\',\'%s\',\'%s\')", type, name, version, event, value);

		//  Add the object
		#ifdef USE_MYSQL
			if (mysql_real_query(mysql, query, strlen(query)) == 0) // success
		#else
			if (PQresultStatus((res = PQexec(mysql, query))) == PGRES_COMMAND_OK)
		#endif
				printf( "Trigger ==> Type:%s, Name:%s, Version:%s Event:%s Value:%s **ADDED**\n", type, name, version, event, value);
			else
				printf( "Failed to insert trigger object.  Error: %s\n", mysql_error(mysql, res));

		#ifndef USE_MYSQL
			if (res) PQclear(res);
		#endif
	}
	else return id;

	return existTrigger(mysql, query, type, name, version, event, value);
}

static int existDownload(MYSQL * mysql, char * query, char * type, char * name, char * version, int trigger_id)
{
	MYSQL_RES * res;
	int id = 0;

	// Check if the object exists first by preparing a SELECT query
	sprintf(query, "SELECT ID FROM download WHERE type=\'%s\' AND name=\'%s\' AND version=\'%s\' AND trigger_id=%d", type, name, version, trigger_id);

#ifdef USE_MYSQL
	if (mysql_real_query(mysql, query, strlen(query)) == 0) // success
	{
		MYSQL_ROW row;

		query[0] = '\0';

		if (res = mysql_store_result(mysql))
		{
			if (row = mysql_fetch_row(res))
			{
				id = atoi(row[0]);
			}
			mysql_free_result(res);
		}
	}
#else
	if (res = PQexec(mysql, query))
	{
		query[0] = '\0';

		if (PQresultStatus(res) == PGRES_TUPLES_OK)
		{
			if (PQntuples(res) > 0)
				id = atoi(PQgetvalue(res, 0, 0));
		}

		PQclear(res);
	}
#endif

	return id;
}

static void updateDownload(MYSQL * mysql, char * query, char * type, char * name, char * version, int id)
{
	#ifndef USE_MYSQL
		MYSQL_RES * res;
	#endif

	// Check if the object exists first by preparing a SELECT query
	if (existDownload(mysql, query, type, name, version, id))
		return;

	// Prepare the insertion SQL command
	sprintf(query, "INSERT INTO download values(default, \'%s\',\'%s\',\'%s\',%d)", type, name, version, id);

	//  Add the object
	#ifdef USE_MYSQL
		if (mysql_real_query(mysql, query, strlen(query)) == 0) // success
	#else
		if (PQresultStatus((res = PQexec(mysql, query))) == PGRES_COMMAND_OK)
	#endif
			printf( "Download ==> Type:%s, Name:%s, Version:%s Trigger-ID:%d **ADDED**\n", type, name, version, id);
		else
			printf( "Failed to insert download object.  Error: %s\n", mysql_error(mysql, res));

	#ifndef USE_MYSQL
		if (res) PQclear(res);
	#endif
}

void import(char * schema, char * sourceFile)
{
	MYSQL * mysql;
	FILE * rdFile;
	char * type;
	char * name;
	char * version;
	char * json;
	int objectType;

#ifdef USE_MYSQL
	// mysql initialisation
	if ((mysql = mysql_init(NULL)) == NULL)
	{
		printf("MySql Initialisation error. Exiting...\n");
		exit(-1);
	}

	// mysql database connection
	if (!mysql_real_connect(mysql, host, user, password, schema, 0, unix_socket, 0))
	{
		fprintf(stderr, "Failed to connect to the 'i-ris' database.  Error: %s\n", mysql_error(mysql, res));
		exit(-2);
	}
#else
	char connectString[300];
	sprintf(connectString, "host=localhost port=5432 dbname=%s user=postgres connect_timeout=5", schema);
	mysql = PQconnectdb(connectString);
	PQsetClientEncoding(mysql, "SQL_ASCII");

	if (PQstatus(mysql) != CONNECTION_OK)
	{
		printf("\ni-ris connection status failed\n");
		exit(-1);
	}
//	else
//		printf("\ni-ris connection result = %X\n", mysql);
#endif


	// Open the file
	rdFile = fopen(sourceFile, "rb");

	// LOOP: get first/next object with type and name
	while((objectType = getNextObject(rdFile, &type, &name, &version, &json)) >= 0)
	{
		char query[11000];

		// If this is a trigger object, create a trigger
		if (objectType == 1)
		{
			int id;
			char event[100];
			char value[100];
			char * download;

			// Add the trigger if it is not already there...
			getObjectField(json, 1, event, NULL, "EVENT:");
			getObjectField(json, 1, value, NULL, "VALUE:");
			id = updateTrigger(mysql, query, type, name, version, event, value);

			// Add the downloads if they are not already there...
			for (download = json; getObjectField(download, 1, NULL, &download, "DOWNLOAD:"); download++)
			{
				char type[100];
				char name[100];
				char version[100];

				getObjectField(download, 1, type, NULL, "[");
				getObjectField(download, 2, name, NULL, "[");
				getObjectField(download, 3, version, NULL, "[");
				updateDownload(mysql, query, type, name, version, id);
			}
		}
		else
			updateObject(mysql, query, json, type, name, version);
	}

	// House keeping and terminate
	fclose(rdFile);
	mysql_close(mysql);
}

importJSON(char * schema, char * fileName)
{
	char outFileName[100];

	// Compact the file by removing tabs and line feeds (BUT NOT SPACES)
	sprintf(outFileName, "%s.json", fileName);
	compactjson(fileName, outFileName);

	// Import the json file into the database
	import(schema, outFileName);

	// Remove the output file. No need for it now
	remove(outFileName);
}

int main(int argc, char ** argv)
{
#ifdef WIN32
	HANDLE fh;
#endif
	int arg;

	while((arg = getopt(argc, argv, "o:u:U:k:h?")) != -1)
	{
		switch(arg)
		{
			case 'o':
				host = optarg;
				break;
			case 'u':
				user = optarg;
				break;
			case 'U':
				password = optarg;
				break;
			case 'k':
				unix_socket = optarg;
				break;
			case 'h':
			case '?':
			default:
				printf(	"Usage: %s [-o=host] [-h=help] [-?=help] [-u user=root] [-U password=password.01] [-k unix_socket=NULL]\n", argv[0]);
				exit(-1);
		}
	}

	// If an error detected, quit
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s schema [file]\n", argv[0]);
		exit(-1);
	}

	// A file name must be supplied
#ifdef WIN32
	else if (optind == argc)
	{
		WIN32_FIND_DATA * fd;
		fd = malloc(sizeof(WIN32_FIND_DATA));
		if ((fh = FindFirstFile("*.json.s", fd)) != INVALID_HANDLE_VALUE)
		{
			do
			{
				char temp[300];
				printf("\nImporting file: %s\n", fd->cFileName);
				importJSON(argv[optind], fd->cFileName);
				sprintf(temp, "copy %s p:\\auris\\verifone\\%.*s", fd->cFileName, strlen(fd->cFileName)-7, fd->cFileName);
				printf("%s\n", temp);
				system(temp);
			} while(FindNextFile(fh, fd));
		}
	}
	else importJSON(argv[optind], argv[2]);
#else
	else
	{
		int i;

		for (i = optind+1; i < argc; i++)
		{
			printf("Importing %s into %s...\n", argv[i], argv[optind]);
			importJSON(argv[optind], argv[i]);
		}
	}
#endif
}
