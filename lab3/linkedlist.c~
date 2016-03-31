#include <stdio.h>

struct Node {
	int grade;
	struct Node* next;
}

static void addToList(int grade, struct Node* list);
static struct Node* allocateNewList();
static void freeMemory(struct Node* rootNode);
static void printGrades(struct Node* rootNode);

int main() {

	int grade;
	printf("Begin entering grades\n");
	char buffer[128];

	struct Node* rootNode = allocateNewList();

	while(1) {
	
		scanf("%d", buffer, &grade);
		
		if (grade > -1) {
			addToList(grade, rootNode);
		} else {
			break;
		}

	}

	printGrades(rootNode);
	freeMemory(rootNode);
	
	return 0;

}

static void addToList(int grade, struct Node* list) {

}

static struct Node* allocateNewList() {

}

static void freeMemory(struct Node* rootNode) {

}

static void printGrades(struct Node* rootNode) {

}
