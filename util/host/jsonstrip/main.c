#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>

#include <winsock2.h>

typedef enum
{
	STATE_NEW_LINE,
	STATE_COMMENT_1,
	STATE_COMMENT_2,
	STATE_SKIP_BLANK,
	STATE_ECHO,
} E_STATE;

int main(int argc, char ** argv)
{
	HANDLE fh;
	WIN32_FIND_DATA * fd;

	// If an error detected, quit
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s directory\n", argv[0]);
		exit(-1);
	}

	// A file name must be supplied
	fd = malloc(sizeof(WIN32_FIND_DATA));
	if ((fh = FindFirstFile("*.json.s", fd)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			char temp[300];
			printf("\nImporting file: %s\n", fd->cFileName);
			sprintf(temp, "copy %s %s\\%.*s", fd->cFileName, argv[1], strlen(fd->cFileName)-7, fd->cFileName);
			printf("%s\n", temp);
			system(temp);
		} while(FindNextFile(fh, fd));
	}
}
