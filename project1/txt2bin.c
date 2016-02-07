/*
  txt2bin.c

  MY NAME: 			Daniel Wright
  MY PITT EMAIL:	djw67@pitt.edu

  As you develop and test this file:

  use this command to compile: (you can name the executable whatever you like)
		gcc -W -Wall -Wextra -O2  txt2bin.c  -o txt2bin.exe

  use this command to execute:  (you will of course test on both input files)
		txt2bin.exe sine-1.bin
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main( int argc, char *argv[] ) {

	FILE* txtFile, *binFile;
	short int shortVal;

	if (argc < 3) {

		printf("usage: %s  <infilename> <outfilename> (i.e. ./txt2bin.exe sine-1.txt sine-1.bin)\n", argv[0] );
		return EXIT_FAILURE;

	}

	txtFile = fopen( argv[1], "rt" );

	if (txtFile == NULL ) {

		printf("Can't open %s for input. Program halting\n", argv[1] );
		return EXIT_FAILURE;
	
	}

	binFile = fopen( argv[2], "wb" );
	
	if (binFile == NULL ) {

		printf("Can't open %s for output. Program halting\n", argv[2] );
		return EXIT_FAILURE;
	
	}

	// Read in text file & write into binary file

	while ( feof(txtFile) == 0 ) {

		if ( fscanf(txtFile, "%hi", &shortVal) == 1 ) { // Read in value from .txt file
			// Only write if a value was actually read into 'shortVal'. fscanf will return the number of entries read from the buffers (in this case, address of 'shortVal').
			// Therefore, the max number of entries able to be read in is 1.
			fwrite(&shortVal, sizeof(shortVal), 1, binFile); // Write value into binary file
		}	
		

	}

	// Close binary and text file

	fclose( txtFile );
	fclose( binFile );

	// Reopen binary file for just reading

	binFile = fopen( argv[2], "rb" );

	while ( feof(binFile) == 0 ) {

		if ( fread(&shortVal, sizeof(shortVal), 1, binFile) == 1 ) { // Read in value from binary file & only print if value was successfully read
			printf("%hi\n", shortVal);
		}

	}

	fclose( binFile );

	return EXIT_SUCCESS;  // 0 value return means no errors

}