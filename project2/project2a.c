/*

	Daniel Wright (djw67@pitt.edu)
	March 6 2016
	'mystrings' program, (part 1 of project 2)
	CS/COE 449, Spring 2016

*/

#include <stdlib.h>
#include <stdio.h>

#define ASCII_START 32
#define ASCII_END 126

#define STRING_MINIMUM_LENGTH 4

static char* findNextString(FILE* fPointer, unsigned int* advancedOut, unsigned int* stringOutLength);

int main(int argc, char* argv[] ) {

	if (argc < 2) {
		printf("usage: %s <input file name>\n", argv[0]);
	}

	FILE* input = fopen(argv[1], "rb");

	if (input == NULL) {

		printf("failed to open file %s\n", argv[1]);
		return EXIT_FAILURE;

	}

	unsigned int advancedBy = 0;
	unsigned int stringLength = 0;

	while (feof(input) == 0) {

		char* currString = findNextString(input, &advancedBy, &stringLength);

		if (currString != NULL) {
		
			printf("%s\n", currString);
			free(currString);

		}


	}

	return EXIT_SUCCESS;

}

static char* findNextString(FILE* fPointer, unsigned int* advancedOut, unsigned int* stringOutLength) {

	long int startMark = ftell(fPointer);

	long int strStartMark = -1;
	long int strEndMark = -1;

	char* outputString = NULL;
	unsigned int currStringLength = 0;


	while (feof(fPointer) == 0) {

		char currChar = fgetc(fPointer);

		if ( (currChar >= ASCII_START) && (currChar <= ASCII_END) ) {
			currStringLength++;
		} else if (currStringLength >= STRING_MINIMUM_LENGTH) {

			strEndMark = ftell(fPointer);
			strStartMark = (strEndMark - currStringLength - 1);

			break;

		} else {
			currStringLength = 0;
		}

	}

	if (strStartMark > -1) {

		if (fseek(fPointer, strStartMark, SEEK_SET) == 0) {

			outputString = malloc(currStringLength + 1);	// + 1 for NULL at end
			outputString[currStringLength] = '\0';

			unsigned int readCt = fread(outputString, 1, currStringLength, fPointer);

			if (readCt != currStringLength) {
				// error reading?
			}

			unsigned int advancedBy = (unsigned int)(strEndMark - startMark);

			*advancedOut = advancedBy;
			*stringOutLength = currStringLength; 

		}

		//fseek(fPointer, strEndMark + 1, SEEK_SET); // Set file pointer back for next string

	}

	return outputString;

}