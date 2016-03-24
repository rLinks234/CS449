/*

	Daniel Wright
	CS/COE 449
	Project 3: Malloc implementation (Next Fit)
	Email: djw67@pitt.edu

	GCC flags used: '-O2 -m32'

*/

#include "mymalloc.h"

// ----------------------------------------------------------------
// [static] Global Variables

extern void *_end;

// The malloc list (pointed to node of last allocated)
static malloc_list_t* mGlobalList = MYMALLOC_END_OF_LIST;

// First node in 'mGlobalList' (also the 'head' node)
static malloc_list_t* mBeginningNode = MYMALLOC_END_OF_LIST;

// Last node in 'mGlobalList'
static malloc_list_t* mEndingNode = MYMALLOC_END_OF_LIST;

// Used in my_nextfit_malloc(int) to determine how many times the function my_nextfit_malloc(int) was called (for list bookkeeping)
static int mMallocCallCounter = 0;	

// ----------------------------------------------------------------
// Static Function Declarations / Inlines

// Returns the adjusted allocation size (accounting for alignment)
static int _getAlignedAlloc(int pInput);

// Returns the address of the current program break
static void* _getProgramBreak();

// Allocates a node at the end of the list, also allocating 'pAllocSize' bytes for the data
// Returns 'MYMALLOC_END_OF_LIST' if sbrk() failed, or if 'pAllocSize' is negative
// If sucessful, this function also assigns 'mEndingNode' to the returned struct pointer
// This moves the program break 'up' (larger address in memory, away from code)
static malloc_list_t* _advanceSbrkForNode(int pAllocSize);

// Returns the address suitable for using an allocation of size 'pSize' within the linked list
// If no block size is as large as or greater than 'pSize' then this returns NULL
static void* _findCompatibleBlockForSize(int pSize);

// Adds an allocation entry of size 'pSize' to the end of 'mGlobalList'
// Returns MYMALLOC_TRUE on success, MYMALLOC_FALSE on failure
static int _addToList(int pSize);

// Returns if the given pointer 'pPointer' is within the linked list
static int _listContains(void* pPointer);

// Returns allocation size for the given input pointer. If pointer is invalid/not in list, 'MYMALLOC_ALLOC_INVALID' is returned.
static int _getAllocatedSize(void* pPointer);

// Called on a call to my_free(void* pPointer) where 'pPointer' is valid
static void _onFree();

// Effectively removes the last node from the list (by ;giving' memory back to operating system)
// This moves the program break 'down' (smaller address in memory, closer to code)
static void _pruneEndingNode();

// Coalesce adjacent nodes and shift pointers as needed
static void _coalesce();

// ----------------------------------------------------------------
// Extern Function Implementations

void* my_nextfit_malloc(int pSize) {

	void* tAddress = NULL;
	int allocSize = pSize;

	// Invalid for <= 0
	if (allocSize <= 0) {
		return tAddress;
	}

	// Adjust for alignment
	allocSize = _getAlignedAlloc(allocSize);

	// Find if a block size already lists in the linked list
	tAddress = _findCompatibleBlockForSize(allocSize);

	// Use existing block, if found
	if (tAddress != NULL) {

		mMallocCallCounter++;
		return tAddress;

	}

	// Existing block not found, add new node & move sbrk
	if (_addToList(allocSize) == MYMALLOC_FALSE) {
		// Failed to add new node
		return tAddress;
	}

	tAddress = mGlobalList->mAddress;

	mMallocCallCounter++;

	return tAddress;

}

void my_free(void* pPointer) {

	// Pointer is invalid, or doesn't exist in the list. Don't do anything & return
	if (_listContains(pPointer) != MYMALLOC_TRUE) {
		return;
	}

	while (pPointer != mGlobalList->mAddress && mGlobalList->mIsHead != MYMALLOC_TRUE) {
		mGlobalList = mGlobalList->mNextNode;
	}

	if (mGlobalList->mIsUsed == MYMALLOC_FALSE) {
		return;
	}

	mGlobalList->mIsUsed = MYMALLOC_FALSE;

	_onFree();

}

// ----------------------------------------------------------------
// Static Function Implementations

static int _getAlignedAlloc(int pInput) {

	if (pInput < MYMALLOC_ALIGNMENT) {
		return MYMALLOC_MIN_ALLOC_SIZE;
	} else {
		return MYMALLOC_ALIGNMENT * ((pInput + (MYMALLOC_ALIGNMENT) + (MYMALLOC_ALIGNMENT - 1)) / MYMALLOC_ALIGNMENT);
	}

}

static void* _getProgramBreak() {
	return (void*)sbrk(0);
}

static malloc_list_t* _advanceSbrkForNode(int pAllocSize) {

	if (pAllocSize < 0) {
		return MYMALLOC_END_OF_LIST;
	}

	// First allocate for bookkeeping struct
	// THEN allocate for data.

	int tCurrentBrk = sbrk(MYMALLOC_NODE_SIZE + pAllocSize); 	// New brk

	if (tCurrentBrk == MYMALLOC_BRK_ERR) {

		MYMALLOC_SBRK_FAIL_LOG();
		return MYMALLOC_END_OF_LIST;

	}

	malloc_list_t* ret = (malloc_list_t*)tCurrentBrk;

	ret->mAllocSize = pAllocSize;
	ret->mAddress = (void*) (tCurrentBrk + MYMALLOC_NODE_SIZE);
	ret->mIsUsed = MYMALLOC_TRUE;
	ret->mIsHead = MYMALLOC_FALSE;

	if (mMallocCallCounter == 1) {				// Only 2 nodes in list

		ret->mPreviousNode = mBeginningNode;
		ret->mNextNode = mBeginningNode;

		mEndingNode = ret;
		mGlobalList = ret;

		mBeginningNode->mNextNode = ret;
		mBeginningNode->mPreviousNode = ret;

	} else if (mMallocCallCounter == 2) {		// Only 3 nodes in list

		ret->mPreviousNode = mBeginningNode->mNextNode;
		ret->mNextNode = mBeginningNode;

		mBeginningNode->mPreviousNode = ret;
		mEndingNode->mNextNode = ret;

		mEndingNode = ret;
		mGlobalList = ret;

	} else if (mMallocCallCounter > 2) {		// > 3 nodes in list

		ret->mPreviousNode = mEndingNode;
		ret->mNextNode = mBeginningNode;
		mBeginningNode->mPreviousNode = ret;
		mEndingNode->mNextNode = ret;

		mEndingNode = ret;
		mGlobalList = ret;
	
	} else {									// Only 1 node in list

		mBeginningNode = ret;
		mEndingNode = ret;
		mGlobalList = ret;

		mGlobalList->mIsHead = MYMALLOC_TRUE;

	}	

	return ret;

}

static void* _findCompatibleBlockForSize(int pAllocSize) {

	if (mGlobalList == MYMALLOC_END_OF_LIST) {
		return NULL;
	}

	malloc_list_t* startNode = mGlobalList;
	malloc_list_t* endNode = mGlobalList->mPreviousNode;

	while (mGlobalList != endNode) {

		//printf("mallocctr = %d\n", mMallocCallCounter);

		if (mGlobalList == MYMALLOC_END_OF_LIST) {

			mGlobalList = startNode;
			return NULL;

		}

		if ( ( mGlobalList->mAllocSize >= pAllocSize ) && ( !mGlobalList->mIsUsed == MYMALLOC_TRUE ) ) {

			mGlobalList->mIsUsed = MYMALLOC_TRUE;
			return mGlobalList->mAddress;

		}

		mGlobalList = mGlobalList->mNextNode;

	}

	mGlobalList = startNode;
	return NULL;

}

static int _addToList(int pAllocSize) {

	malloc_list_t* oldEndNode = mEndingNode;
	malloc_list_t* newNode = _advanceSbrkForNode(pAllocSize);

	// sbrk() returned bad result. Return
	if (newNode == (void*)MYMALLOC_BRK_ERR) {

		MYMALLOC_SBRK_FAIL_LOG();
		return MYMALLOC_FALSE;

	}
	
	return MYMALLOC_TRUE;

}

static int _listContains(void* pPointer) {

	if (pPointer == NULL) {
		return MYMALLOC_FALSE;
	}

	malloc_list_t* startingNode = mBeginningNode;
	malloc_list_t* iteratedNode = startingNode;

	while (iteratedNode->mAddress != pPointer) {

		iteratedNode = iteratedNode->mNextNode;

		if (iteratedNode == MYMALLOC_END_OF_LIST) {
			return MYMALLOC_FALSE;
		}

		if (iteratedNode == startingNode) {
			return MYMALLOC_FALSE;
		}

		if (iteratedNode->mAddress == pPointer) {
			return MYMALLOC_TRUE;
		}

	}

	return MYMALLOC_TRUE;

}

static int _getAllocatedSize(void* pPointer) {

	malloc_list_t* startingNode = mGlobalList;
	malloc_list_t* iteratedNode = startingNode;

	while (iteratedNode->mAddress != pPointer) {

		iteratedNode = iteratedNode->mNextNode;

		if (iteratedNode == MYMALLOC_END_OF_LIST) {
			return MYMALLOC_ALLOC_INVALID;
		}

		if (iteratedNode == startingNode) {
			return MYMALLOC_ALLOC_INVALID;
		}

		if (iteratedNode->mAddress == pPointer) {
			return iteratedNode->mAllocSize;
		}

	}

	return iteratedNode->mAllocSize;

}

static void _onFree() {

	malloc_list_t* nextNode = mGlobalList->mNextNode;
	malloc_list_t* previousNode = mGlobalList->mPreviousNode;

	if (mGlobalList == mEndingNode) {

		_pruneEndingNode();

	} else if (mGlobalList == mBeginningNode) {

		if (nextNode->mIsUsed == MYMALLOC_FALSE) {
			_coalesce();
		}

	} else {

		if ( (nextNode->mIsUsed == MYMALLOC_FALSE) || (previousNode->mIsUsed == MYMALLOC_FALSE) ) {
			_coalesce();
		}

	}

}

static void _pruneEndingNode() {

	int tAllocAmount = -(mEndingNode->mAllocSize + MYMALLOC_NODE_SIZE);
	malloc_list_t* oldEndingNode = mEndingNode;

	// Move ending node to node before current ending node
	mEndingNode = mEndingNode->mPreviousNode;

	// set cyclical pointers for new ending node
	mEndingNode->mNextNode = mBeginningNode;
	mBeginningNode->mPreviousNode = mEndingNode;

	mGlobalList = mEndingNode;

	sbrk(tAllocAmount);

}

static void _coalesce() {

	malloc_list_t* nextNode = mGlobalList->mNextNode;
	malloc_list_t* previousNode = mGlobalList->mPreviousNode;
	
	malloc_list_t* newNextNode = MYMALLOC_END_OF_LIST;
	malloc_list_t* newPreviousNode = MYMALLOC_END_OF_LIST;

	int mergeIntoNext = nextNode->mIsUsed == MYMALLOC_FALSE;
	int mergeIntoPrevious = ( previousNode->mIsUsed == MYMALLOC_FALSE ) && (mGlobalList->mIsHead);

	int currentAllocSize = mGlobalList->mAllocSize;
	int nextAllocSize = nextNode->mAllocSize;
	int previousAllocSize = previousNode->mAllocSize;

	void* newDataAddress = NULL;

	// New allocation size for coalesced node(s)
	int pureResultAllocSize = currentAllocSize;

	if (mergeIntoNext && !mergeIntoPrevious) {			// Merge into only next

		// mGlobalList pointer stays the same
		// Update alloc size for mGlobalList

		pureResultAllocSize += nextAllocSize + MYMALLOC_NODE_SIZE;

		newNextNode = nextNode->mNextNode;
		mGlobalList->mNextNode = newNextNode;

	} else if (!mergeIntoNext && mergeIntoPrevious) { 	// Merge into only previous

		// mGlobalList pointer moved to mGlobalList->mPreviousNode
		// Update alloc size for mGlobalList

		pureResultAllocSize += previousAllocSize + MYMALLOC_NODE_SIZE;

		newPreviousNode = previousNode->mPreviousNode;
		mGlobalList = previousNode;
		mGlobalList->mPreviousNode = newPreviousNode;

	} else {											// Merge into both

		// mGlobalList pointer moved to mGlobalList->mPreviousNode
		// Update alloc size for mGlobalList

		pureResultAllocSize += nextAllocSize + previousAllocSize + ( 2 * MYMALLOC_NODE_SIZE );

		newPreviousNode = previousNode->mPreviousNode;
		newNextNode = nextNode->mNextNode;
		mGlobalList = previousNode;
		mGlobalList->mPreviousNode = newPreviousNode;
		mGlobalList->mNextNode = newNextNode;

	}

	mGlobalList->mAllocSize = pureResultAllocSize;

}