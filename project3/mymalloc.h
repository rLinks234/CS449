/*

	Daniel Wright
	CS/COE 449
	Project 3: Malloc implementation (Next Fit)
	Email: djw67@pitt.edu

*/

// Macro that defines false
#define MYMALLOC_FALSE 0

// Macro that defines true
#define MYMALLOC_TRUE 1

// Macro that defines the end of a linked list
#define MYMALLOC_END_OF_LIST NULL

// Macro that defines error result of sbrk()
#define MYMALLOC_BRK_ERR -1

// Macro that defines an invalid allocation size
#define MYMALLOC_ALLOC_INVALID -1

// Byte alignment. Align by 16 bytes
#define MYMALLOC_ALIGNMENT 8

// Smallest allocation allowed 
#define MYMALLOC_MIN_ALLOC_SIZE 16

// Macro that defines print statement that denotes sbrk() returned bad number
#define MYMALLOC_SBRK_FAIL_LOG() printf("sbrk() returned -1\n");

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef struct malloc_list {

	int mAllocSize;
	int mIsUsed;
	int mIsHead;
	void* mAddress;
	struct malloc_list* mPreviousNode;
	struct malloc_list* mNextNode;

} malloc_list_t;

void* my_nextfit_malloc(int pSize);
void my_free(void* pPointer);

// Macro that defines size of a linked list node entry
#define MYMALLOC_NODE_SIZE sizeof(malloc_list_t)