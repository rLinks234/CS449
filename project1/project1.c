/*
  project1.c

  MY NAME: 			Daniel Wright
  MY PITT EMAIL:	djw67@pitt.edu

	gcc -W -Wall -Wextra -O2 project1.c -o project1

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct wave Wave;
typedef struct waveList WaveList;

typedef struct wave {

	int endingSampleIndex;
	short int endingSampleValue;
	int sampleCount;
	short int maxSampleValue;
	short int minSampleValue;

} Wave;

typedef struct waveList {

	WaveList* nextNode;
	WaveList* previousNode;	// Only include to iterate from back to front when deleting list
	Wave thzElement;

} WaveList;

static WaveList* createList();
static void addWave(WaveList* list, int endingSampleIndex, short int endingSampleValue, int sampleCount, short int maxSampleValue, short int minSampleValue);
static void printList(WaveList* list);
static void deleteList(WaveList* list);

int main( int argc, char* argv[] ) {

	if (argc != 2) {

		printf("usage: %s  <infilename>\n", argv[0] );
		return EXIT_FAILURE;

	}

	int sampleCount = 0;
	int sampleCountInWave = 0;
	short int sampleMax = 0;
	short int sampleMin = 0;

	short int currentSampleValue = 0;
	short int nextSampleValue = 0;

	FILE* binFile = fopen(argv[1], "rb");

	WaveList* list = createList();

	int firstFullWave = 0; // Flag that will be set to true once we have reached the point in the file
	// where we have seen the beginning of the first full wave. 
	// This will be when we are in the negative values and encounter a number greater than or equal to 0.

	while( feof(binFile) == 0 ) {

		if ( fread(&nextSampleValue, sizeof(nextSampleValue), 1, binFile) == 1 ) {
			
			sampleCountInWave++;
			sampleCount++;

		}

		if (sampleMax < currentSampleValue) {
			sampleMax = currentSampleValue;
		} else if (sampleMin > currentSampleValue) {
			sampleMin = currentSampleValue;
		}

		if ( (sampleCountInWave > 1) && (currentSampleValue < 0) && (nextSampleValue >= 0) ) { // End of the wave

			if (nextSampleValue == 0) {

				if (firstFullWave == 0) {
					firstFullWave = 1;
				} else {
					addWave(list, sampleCount, nextSampleValue, sampleCountInWave + 1, sampleMax, sampleMin);
				}

				sampleCountInWave = 0; 	// reset sample counter
			
			} else {

				if (firstFullWave == 0) {
					firstFullWave = 1;
				} else {
					addWave(list, sampleCount - 1, currentSampleValue, sampleCountInWave, sampleMax, sampleMin);
				}

				sampleCountInWave = 0; 	// reset sample counter
			
			}
	
			sampleMax = 0;			// reset max
			sampleMin = 0; 			// reset min

		}

		currentSampleValue = nextSampleValue;

	}

	printList(list);
	deleteList(list);

	return EXIT_SUCCESS;  // 0 value return means no errors

}

// Creates a new WaveList to be used
static WaveList* createList() {

	WaveList* list = (WaveList*)malloc(sizeof(WaveList));
	list->nextNode = NULL;
	list->previousNode = NULL;

	return list;

}

// Adds a wave to the list
static void addWave(WaveList* list, int _endingSampleIndex, short int _endingSampleValue, int _sampleCount, short int _maxSampleValue, short int _minSampleValue) {

	while(list->nextNode != NULL) {
		list = list->nextNode;	// Moving to next node
	}

	Wave newWave;

	newWave.endingSampleIndex	= _endingSampleIndex;
	newWave.endingSampleValue	= _endingSampleValue;
	newWave.sampleCount			= _sampleCount;
	newWave.maxSampleValue		= _maxSampleValue;
	newWave.minSampleValue		= _minSampleValue;

	list->thzElement = newWave;

	list->nextNode = (WaveList*)malloc(sizeof(WaveList));
	list->nextNode->previousNode = list;

}

static void printList(WaveList* list) {

	while(list->nextNode != NULL) {

		printf("%i\t%hi\t%i\t%hi\t%hi\n", list->thzElement.endingSampleIndex, list->thzElement.endingSampleValue, list->thzElement.sampleCount, list->thzElement.maxSampleValue, list->thzElement.minSampleValue);
		list = list->nextNode;	// Moving to next node
	
	}

}

static void deleteList(WaveList* list) {

	// Move to last node
	while(list->nextNode != NULL) {
		list = list->nextNode;	// Moving to next node	
	}

	while(list->previousNode != NULL) {

		WaveList* temp = list;
		list = list->previousNode;

		free(temp);

	}

	free(list);

}