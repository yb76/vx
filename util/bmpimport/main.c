#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

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

unsigned char * bmpToPbm(char * name, unsigned int * size)
{
	FILE * fp;
	char data[2];
	unsigned char * image;
	int width, height;
	int offset;
	int bmp_width;
	int i;

	// Make sure the file is accessible
	if ((fp = fopen(name, "rb")) == NULL)
	{
		fprintf(stderr, "Failed to open source file: %s\n", name);
		return NULL;
	}

	// Read the magic signature of a BMP file
	if (fread(data, 1, 2, fp) != 2)
	{
		fprintf(stderr, "Failed to read source file: %s\n", name);
		return NULL;
	}

	// It must start with a "BM"
	if (data[0] != 'B' && data[1] != 'M')
	{
		fprintf(stderr, "Invalid BMP signature for file: %s\n", name);
		return NULL;
	}

	// Read the number of colours in the BMP file image
	fseek(fp, 28, SEEK_SET);
	if (fread(data, 1, 2, fp) != 2)
	{
		fprintf(stderr, "Failed to read BMP header (colours) for file: %s\n", name);
		return NULL;
	}

	// It must be a monochrome BMP
	if ( *((short *) data) != 1)
	{
		fprintf(stderr, "Not a MONOCHROME bmp file: %s\n", name);
		return NULL;
	}

	// Read the width and height of the image
	fseek(fp, 18, SEEK_SET);
	if (fread(&width, sizeof(width), 1, fp) != 1)
	{
		fprintf(stderr, "Invalid BMP header (width) for file: %s\n", name);
		return NULL;
	}
	if ((bmp_width = width/32*32) != width) bmp_width += 32;
	if (fread(&height, sizeof(height), 1, fp) != 1)
	{
		fprintf(stderr, "Failed to read BMP header (height) for file: %s\n", name);
		return NULL;
	}

	// Read the offset to the image
	fseek(fp, 10, SEEK_SET);
	if (fread(&offset, sizeof(offset), 1, fp) != 1)
	{
		fprintf(stderr, "Failed to read BMP header (image offset) for file: %s\n", name);
		return NULL;
	}

	// Allocate enough buffer to store the resulting image
	if ((width/8*8) != width)
		width = width / 8 * 8 + 8;
	*size = width*height/8 + 4;
	if ((image = malloc(*size)) == NULL)
	{
		fprintf(stderr, "Not enough memory to convert file: %s\n", name);
		return NULL;
	}

	// Store the width and height of the image as required by the device
	image[0] = (char) (width/256);
	image[1] = (char) (width%256);
	image[2] = (char) (height/256);
	image[3] = (char) (height%256);

	// Position to the beginning of the image
	fseek(fp, offset, SEEK_SET);

	// Read the last row of the image
	for (i = height; i > 0; i--)
	{
		fread(&image[(i-1)*width/8+4], 1, width/8, fp);
		fseek(fp, (bmp_width-width)/8, SEEK_CUR);
	}

	// Reverse the bits as they are a mirror image
	for (i = 0; i < width*height/8; i++)
		image[i+4] = ~image[i+4];

	fclose(fp);

	return image;
}

void import(char * schema, char * json, char * version, char * fileName)
{
	MYSQL * mysql;
	MYSQL_RES * res;
	char query[40000];


#ifdef USE_MYSQL
	if ((mysql = mysql_init(NULL)) == NULL)
	{
		printf("MySql Initialisation error. Exiting...\n");
		exit(-1);
	}

	if (!mysql_real_connect(mysql, host, user, password, schema, 0, unix_socket, 0))
	{
		printf( "Failed to connect to the '%s' database.  Error: %s\n", schema, mysql_error(mysql, res));
		exit(-2);
	}
#else
	char connectString[300];
	sprintf(connectString, "host=localhost port=5432 dbname=%s user=postgres connect_timeout=5", schema);
	mysql = PQconnectdb(connectString);

	if (PQstatus(mysql) != CONNECTION_OK)
	{
		printf("\ni-ris connection status failed\n");
		exit(-1);
	}
//	else
//		printf("\ni-ris connection result = %X\n", mysql);
#endif

	// Check if the object exists first by preparing a SELECT query
	sprintf(query, "SELECT COUNT(*) FROM object WHERE type=\'IMAGE\' AND name=\'%s\'", fileName);
	printf("SELECT COUNT(*) FROM object WHERE type=\'IMAGE\' AND name=\'%s\'\n", fileName);

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
						sprintf(query, "UPDATE object SET version=\'%s\', json=\'%s\' WHERE type=\'IMAGE\' AND name=\'%s\'", version, json, fileName);
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
						sprintf(query, "UPDATE object SET version=\'%s\', json=\'%s\' WHERE type=\'IMAGE\' AND name=\'%s\'", version, json, fileName);
						printf("UPDATE object SET version=\'%s\', json=\'%s\' WHERE type=\'IMAGE\' AND name=\'%s\'\n", version, json, fileName);
					}
				}
			}

			PQclear(res);
		}
	#endif

	// Prepare the INSERT query
	if (query[0] == '\0')
	{
		sprintf(query, "INSERT INTO object values(\'IMAGE\',\'%s\',\'%s\',\'%s\',default)", version, fileName, json);
		printf("INSERT INTO object values(\'IMAGE\',\'%s\',\'%s\',\'%s\',default)\n", version, fileName, json);
	}

	//  Add the object
	#ifdef USE_MYSQL
		if (mysql_real_query(mysql, query, strlen(query)) == 0) // success
	#else
		if (PQresultStatus((res = PQexec(mysql, query))) == PGRES_COMMAND_OK)
	#endif
		printf( "Object ==> Type:IMAGE, Name:%s, Version:%s %s\n", fileName, version, query[0] == 'I'?"**ADDED**":"**UPDATED**");
	else
		printf( "Failed to insert/update object.  Error: %s\n", mysql_error(mysql, res));

	#ifndef USE_MYSQL
		if (res) PQclear(res);
	#endif

	// House keeping and terminate
	mysql_close(mysql);
}


importBMP(char * schema, char * version, char * fileName)
{
	unsigned int i;
	unsigned char * image;
	unsigned int size;
	char * object;
	sha1_context ctx;
	unsigned char digest[20];
	unsigned int signLocation;
	char jsonFile[100];
	FILE * fp;

	// Create a json object
	if ((image = bmpToPbm(fileName, &size)) == NULL)
		exit(-3);

	// Create enough memory for the object
	if ((object = malloc(size*2 + 300)) == NULL)
	{
		fprintf(stderr, "Not enough memory to create object: %s\n", fileName);
		exit(-4);
	}

	// Create the start of the object
	sprintf(object, "{TYPE:IMAGE,NAME:%s,VERSION:%s,IMAGE:", fileName, version);

	// Populate the image
	for (i = 0; i < size; i++)
		sprintf(&object[strlen(object)], "%02X", image[i]);

	// Add the ZERO signature and close the object
	strcat(object, ",SIGN:0000000000000000000000000000000000000000}");
	free(image);

	// Start SHA-1 calculation
	sha1_starts(&ctx);

	// Update the sha-1 calculation
	sha1_update(&ctx, object, strlen(object));

	// Get the sha-1 digest
	sha1_finish(&ctx, digest);

	// Update the SHA-1 signature...
	signLocation = strlen(object) - 41;
	for (i = 0; i < sizeof(digest); i++)
		sprintf(&object[signLocation + i*2], "%02.2X", digest[i]);
	object[signLocation + 40] = '}';

	// Import the json object into the database
	import(schema, object, version, fileName);

	// Create a file to send to a secure location in order to encrypt the signature
	sprintf(jsonFile, "%s.json", fileName);
	fp = fopen(jsonFile, "w+b");
	for (i = 0; i < strlen(object); i++)
		fwrite(&object[i], 1, 1, fp);
	fclose(fp);

	// clean up
	free(object);
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
				printf(	"Usage: %s [-h=help] [-?=help] [-o host=host] [-u user=root] [-U password=password.01] [-k unix_socket=NULL]\n", argv[0]);
				exit(-1);
		}
	}

	// If an error detected, quit
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s schema [bmp_file]\n", argv[0]);
		exit(-1);
	}

	// A file name must be supplied
#ifdef WIN32
	else if (optind == argc)
	{
		WIN32_FIND_DATA * fd;
		fd = malloc(sizeof(WIN32_FIND_DATA));
		if ((fh = FindFirstFile("*.bmp", fd)) != INVALID_HANDLE_VALUE)
		{
			do
			{
				printf("\nImporting BMP file: %s\n", fd->cFileName);
				importBMP(argv[optind], "1.00", fd->cFileName);
			} while(FindNextFile(fh, fd));
		}
		free(fd);
	}
	else importBMP(argv[optind], "1.00", argv[2]);
#else
	else
	{
		int i;

		for (i = optind+1; i < argc; i++)
		{
			printf("Importing BMP file %s into %s...\n", argv[i], argv[optind]);
			importBMP(argv[optind], "1.00", argv[i]);
		}
	}
#endif
}
