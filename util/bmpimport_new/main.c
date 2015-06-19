#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fcntl.h>
#include <string.h>

#include "getopt.h"

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


importBMP( char * version, char * fileName)
{
	unsigned int i;
	unsigned char * image;
	unsigned int size;
	char * object;
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

	strcat(object, "}");
	free(image);

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

	while((arg = getopt(argc, argv, "h?")) != -1)
	{
		switch(arg)
		{
			case 'h':
			case '?':
			default:
				printf(	"Usage: %s [-h=help] [-?=help] \n", argv[0]);
				exit(-1);
		}
	}

	// If an error detected, quit
	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s bmp_file\n", argv[0]);
		exit(-1);
	}
	fprintf(stderr, "File: %s \n", argv[1]);

	// A file name must be supplied
	importBMP( "1.00", argv[1]);
}
