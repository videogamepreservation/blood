#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>

#include <memcheck.h>


#define LENGTH(x) (sizeof(x) / sizeof(x[0]))

const char signature[] = { 0x7E, 0x81, 0xA5, 0x81, 0xBD, 0x99, 0x81, 0x7E };

const char patch[] =
{
	0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF,
	0xFF, 0xC3, 0xA5, 0x99, 0x99, 0xA5, 0xC3, 0xFF,
	0x3C, 0x42, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3C,
	0x3C, 0x42, 0x99, 0xBD, 0xBD, 0x99, 0x42, 0x3C,
};


void main( void )
{
	int nLength;
	int nHandle;
	char *data;
	char *p;

	printf("Build font patcher by Peter M. Freese   (c) 1994 Q Studios\n");

	nHandle = open("TABLES.DAT", O_BINARY | O_RDWR);
	if (nHandle == -1)
	{
		printf("\aCannot open TABLES.DAT.\n");
		exit(1);
	}

	nLength = filelength(nHandle);
	data = malloc(nLength);
	if ( !data )
	{
		close(nHandle);
		printf("\aCannot allocate memory\n");
		exit(1);
	}

	if ( read(nHandle, data, nLength) != nLength)
	{
		close(nHandle);
		printf("\aCannot read TABLES.DAT\n");
		exit(1);
	}

	for (p = data; p < data + nLength - LENGTH(signature) - LENGTH(patch); p++)
	{
		if (memcmp(p, signature, LENGTH(signature)) == 0)
			break;
	}

	if (p >= data + nLength - LENGTH(signature) - LENGTH(patch))
	{
		close(nHandle);
		printf("\aCannot find font in TABLES.DAT\n");
		exit(1);
	}

	p += LENGTH(signature);
	if ( memcmp(p, patch, LENGTH(patch)) == 0 )
	{
		close(nHandle);
		printf("\aTABLES.DAT is already patched!\n");
		exit(0);
	}

	memcpy(p, patch, LENGTH(patch));
	lseek(nHandle, 0, SEEK_SET);
	if ( write(nHandle, data, nLength) != nLength)
	{
		close(nHandle);
		printf("\aCannot write to TABLES.DAT\n");
		exit(1);
	}

	close(nHandle);
	printf("TABLES.DAT has been successfully updated with the new font.\n");
}



