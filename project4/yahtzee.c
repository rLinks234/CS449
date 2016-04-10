#include <stdio.h>
#include <string.h>

// Macros

#define INVALID_VAL -1
#define ROLL_COUNT 5
#define UPPER_SECTION_COUNT 6
#define LOWER_SECTION_COUNT 7

#define LOWER_SELECTION 1
#define UPPER_SELECTION 2

// Globals

typedef struct {

	int needReRoll;
	char dieValue;

} DieEntry;

int mUpperSection[6];
int mLowerSection[7];
int mUpperBonus;
DieEntry mDiceEntries[ROLL_COUNT];

// Forward Declarations
static void game_instance();
static void init_dice_arrays();
static void do_dice_roll();
static void print_dice();
static char get_random();
static void additional_rolls();
static void reset_dice(int);
static void score_upper(int);
static void score_lower(int);
static void print_total_score();
static void print_scoreboard();
static int comp (const void *, const void *);
static int sl_kind(int, char*);
static int sl_straight(int, char*);
static void unique (char*);
static int sl_fullhouse(char*);
static char *int_to_string(int);
static void reverse(char*, int);


int main() {

	int i;

	for (i = 0; i < 6; i++) {
		mLowerSection[i] = mUpperSection[i] = INVALID_VAL;
	}

	mLowerSection[6] = INVALID_VAL;

	game_instance();

	return 0;

}

static void game_instance() {

	int total_score = 0;
	int current_turn = 1;
	int i, j;
	int selection;
	char user_input[10];

	// Game Loop:
	while(current_turn <= 13) {

		printf("\nYour first roll:\n");

		do_dice_roll();
		print_dice();
		
		additional_rolls(); // Rerolls 2 more times if User chooses to do so
		
		// * * *  Dice are now finalized  * * * //

		// Let the user decide how to score:
		printf("Place dice into section:\n");
		printf("%d) Upper Section\n%d) Lower Section\n", UPPER_SELECTION, LOWER_SELECTION);
		printf("Selection? ");

		fgets(user_input, sizeof(int), stdin); // get user input
		selection = atoi(user_input);

		printf("\nPlace dice into: \n");

		if(selection == 1) {// Upper section

			printf("1) Ones\n2) Twos\n3) Threes\n4) Fours\n5) Fives\n6) Sixes\n");
			printf("Selection? ");
			fgets(user_input, sizeof(int), stdin); // get user input
			selection = atoi(user_input);
			score_upper(selection); // Handle Scoreboard

		} else if (selection == 2) { // Lower Section

			printf("1) Three of a kind\n2) Four of a kind\n3) Small straight\n4) Large straight\n5) Full house\n6) Yahtzee\n7) Chance\n");
			printf("Selection? ");
			fgets(user_input, sizeof(int), stdin); // get user input
			selection = atoi(user_input);
			score_lower(selection);

		}

		print_total_score(); // Print score
		print_scoreboard(); // Display scoreboard (upper and lower sections)

		// Loop 13 times
		current_turn++;
		// Reset all dice (Need rerolled!)
		reset_dice(1);

	}

}

/*
 *  Prints the dice values
 */
static void print_dice() {

	int i;
	
	for(i = 0; i < ROLL_COUNT; i++) {
		printf(" %d ", mDiceEntries[i].dieValue);
	}

	printf("\n\n");

}

/**
 * 
 */
static void do_dice_roll() {
	
	int i;
	
	for(i = 0; i < ROLL_COUNT; i++) {
		if (mDiceEntries[i].needReRoll != 0) {
			mDiceEntries[i].dieValue = get_random();
		}	
	}

	reset_dice(0); // none need rerolled

}


/*
 *  Prints scoreboard 
 */
static void print_scoreboard() {
	printf("\nOnes: %s \t\t\tFours: %s\nTwos: %s\t\t\t\tFives: %s\nThrees: %s\t\t\tSixes: %s\n", (upper_section[0] == -1)?" ":int_to_string(upper_section[0]), (upper_section[3] == -1)?" ":int_to_string(upper_section[3]), (upper_section[1] == -1)?" ":int_to_string(upper_section[1]), (upper_section[4] == -1)?" ":int_to_string(upper_section[4]), (upper_section[2] == -1)?" ":int_to_string(upper_section[2]), (upper_section[5] == -1)?" ":int_to_string(upper_section[5]));
	printf("Upper Section Bonus: %d\n", upper_bonus);
 	printf("Three of a kind: %s\t\tFour of a kind: %s\nSmall straight: %s\t\tLarge straight: %s\nFull house: %s\t\t\tYahtzee: %s\nChance: %s\n", (lower_section[0] == -1)?" ":int_to_string(lower_section[0]), (lower_section[1] == -1)?" ":int_to_string(lower_section[1]), (lower_section[2] == -1)?" ":int_to_string(lower_section[2]), (lower_section[3] == -1)?" ":int_to_string(lower_section[3]), (lower_section[4] == -1)?" ":int_to_string(lower_section[4]), (lower_section[5] == -1)?" ":int_to_string(lower_section[5]), (lower_section[6] == -1)?" ":int_to_string(lower_section[6]));
}

/*
 *  Prints total score
 */
static void print_total_score() {
	int i;
	int total = 0;

	// Sum Upper section scores
	for(i = 0; i < UPPER_SECTION_COUNT; i++) { // 6 is max in upper section
		if(upper_section[i] > 0) {  // Only add positive values (ignore initialization values)
			total += upper_section[i];
		}
	}

	// Bonus for upper section
	if(total >= 63) {
		upper_bonus = 35;
		total += upper_bonus;
	}

	// Sum Lower section scores
	for(i = 0; i < LOWER_SECTION_COUNT; i++) { // 7 is max in lower section
		if (lower_section[i] > 0) {   // Only add positive values (ignore initialization values)
			total += lower_section[i];
		}
	}

	printf("\nYour score so far is: %d\n", total);
}

/*
 *  returns BOOLEAN -- 1 if fullhouse exists, 0 if not
 */
static int sl_fullhouse(char* values) {

	int i, j;
	int freq_a = 1;
	int freq_b = 0;

	char a = values[0];
	char b = 0;

	for(i = 1; i < ROLL_COUNT; i++) {
		
		if (values[i] == a) {
			freq_a += 1;
		} else {

			if (b == 0) { // we found a new number

				b = values[i];
				freq_b = 1;
			
			} else if (values[i] == b) {
				freq_b += 1;
			}

		}

	}

	if ((freq_a == 2 && freq_b == 3) || (freq_a == 3 && freq_b == 2)) {
		return 1; // TRUE
	}

	return 0; // FALSE
}

/*
 *  (s)core (l)ower / 3,4,5 of a kind 
 *	Returns BOOLEAN -- 1 if _ of a kind exists, 0 if it does not
 */
static int sl_kind(int kind, char* values) {

	int run, i, j;

	for (j = 1; j <= UPPER_SECTION_COUNT; j++) { // cycle all possible dice values

		run = 0;

		for (i = 0; i < ROLL_COUNT; i++) { // cycle all dice
			if (values[i] == j) {
				run++;
			}
		}

		if (run >= kind) {
			return 1; // TRUE
		}
	
	}

	return 0; // FALSE

}

/*
 * Returns BOOLEAN if small/large straight exists
 */
static int sl_straight(int target_length, char* values) {
	
	int i;
	int run = 1;

	unique(values); // Delete duplicates in values[]

	for (i = 0; i < target_length-1; i++) {
		
		if( values[i + 1] == (values[i] + 1) ) { // check if next # in sequence is one above the current i # (2,3 yeilds "if 3 == 3") || ie: check for a straight!
			run++;
		} else { // reset run
			return 0; // FALSE -- CANNOT BE A STRAIGHT
		}

	}

	if (run == target_length) {
		return 1; // TRUE! Its a straight of (length)
	}

	return 0;// FALSE -- safety check

} 


/*
 *  Calculates score for LOWER section
 */
static void score_lower(int selection) {

	int index = selection - 1;
	int total = 0;
	char values[5];
	int i;

	// Sort the dice:
	for(i = 0; i < ROLL_COUNT; i++) {
		values[i] = die_value[i];
	}

	qsort (values, ROLL_COUNT, 1, comp); // sort array

	// * * *  Dice are now sorted in values[]  * * *

	// Determine score:
	switch(selection) {
		
		// Three of a kind: if good, +total of all dice
		case 1: {
			
			if(sl_kind(3, values)) { // if good
				for(i = 0; i < ROLL_COUNT; i++) {
					total+=die_value[i];
				}
			}
		
			break;

		}

		// Four of a kind: if good, +total of all dice
		case 2: {
			
			if(sl_kind(4, values)) {
				for(i = 0; i < ROLL_COUNT; i++) {
					total+= die_value[i];
				}
			}

			break;

		}

		// Small Straight: if good, +30
		case 3: {
			
			if(sl_straight(4, values)) { // if four in a row
				total = 30;
			}
			
			break;

		}

		// Large Straight: if good, +40
		case 4: {
		
			if(sl_straight(5, values)) { // if five in a row
				total = 40;
			}
		
			break;

		}

		// Full House: if good, +25
		case 5: {

			if(sl_fullhouse(values)) {
				total = 25;
			}

			break;

		}

		// Yahtzee!: if good, +50
		case 6: {

			if(sl_kind(5, values)) {
				for(i = 0; i < ROLL_COUNT; i++) {
					total+= die_value[i];
				}
			}

			break;

		}

		// Chance: +Total of all dice
		case 7: {

			for(i = 0; i < ROLL_COUNT; i++) {
				total+= die_value[i];		
			}
			
			break;

		}

		// Do nothing
	}

	lower_section[index] = total; // Store score

}

/*
 *  Calculates and stores score for the UPPER section
 */ 
static void score_upper(int pSelection) {

	int index = pSelection - 1;
	int quantity = 0;
	int i;
	
	// Get quantity:
	for(i = 0; i < ROLL_COUNT; i++) {
		if (die_value[i] == pSelection) {
			quantity++;
		}
	}

	// Calculate and store score
	upper_section[index] = pSelection * quantity; // Example: User has three 1's:  upper_section[0] = 1 * 3;

}

/*
 *  Resets ALL dice for rerolling if b = 1, and not rerolling if b = 0
 */
static void reset_dice(int pBool) {

	int i;

	for(i = 0; i < ROLL_COUNT; i++) {
		mDiceEntries[i].needReRoll = pBool;
	}

}

/*
 *  Handles the logic for roll 2 and 3 if the user chooses to reroll any dice
 */
static void additional_rolls() {

	int i;
	int j;

	char selection;
	char* token;
	int reroll_input[6];
	char user_input[10];

 	for(j = 0; j < 2; j++) {
 			
 		for(i = 0; i < 6; i++) { // reset 
 			reroll_input[i] = 0;
 		}

 		printf("Which dice to reroll?\n");
 		// Get input
 		fgets(user_input, 10, stdin);

 		// Parse input to reroll array
 		i = 0;
 		token = strtok(user_input, " "); // get first token
 		
 		while (token != NULL) {
 		
 			reroll_input[i++] = atoi(token); // string to int convert
 			token = strtok(NULL, " "); // get next token
 		
 		}

 		// Check if user entered 0 as the first input
 		if(reroll_input[0] == 0) { // user does not want to reroll any dice
 			break; // BREAK FOR Loop
 		}

		// If input == 0, move on, else roll again//
 		i = 0;

 		do {
 		
 			selection = reroll_input[i++];
 		
 			if (selection != 0) {
 				reroll_bool[selection-1] = 1; // True, must reroll!
			}

		} while (selection != 0);

		// Reroll dice:
		printf("Your %s roll:\n", (j==1) ? "final" : "second");
		do_dice_roll(); // Get dice roll
		print_dice(); // Print dice roll

			
	} // End Rerollings

}

/**
 *
 * Get a random byte from the dice driver
 * 
 * @return a byte within the range [0, 5]
 */
static char get_random() {

	FILE* f;
	
	// Only need to read a single byte
	char result;

	int readCount;

	f = fopen("/dev/dice", "r");
	
	do {
		readCount = fread(&result, sizeof(char), 1, f);
	} while (readCount != 1);

	fclose(f);

	return result;

}

/*
 *  Comporartor function for qsort
 */ 
static int comp(const void* pValueA, const void* pValueB) {

	const char pA = *(char*)pValueA; 
	const char pB = *(char*)pValueB;
	if (pA == 0) {
		return 1;
	}
	
	if (pB == 0) {
		return -1;
	}

   return pA - pB; 

}

/*
 *  Unique function for removing duplicates
 */
static void unique(char* pValues) {
	
	char one = pValues[0];
	char two = pValues[1];
	char three = pValues[2];
	char four = pValues[3];
	char five = pValues[4];

	if (four == five) {
		five = 0;
	}
	
	if (three == four) {
		four = 0;
	}
	
	if (two == three) {
		three = 0;
	}

	if (one == two) {
		two = 0;
	}

	pValues[0] = one;
	pValues[1] = two;
	pValues[2] = three;
	pValues[3] = four;
	pValues[4] = five;

	qsort (pValues, ROLL_COUNT, 1, comp); // sort array

}

/*
 * Implementation of Integer to String
 */
static char* int_to_string(int pNumber) {
	
	char* out = malloc(10);
	// if negative, need 1 char for the sign
	int sign = pNumber < 0 ? 1 : 0;
	int i = 0;
	
	if (pNumber == 0) {
	    out[i++] = '0';
	} else if (pNumber < 0) {
	
	    out[i++] = '-';
	    pNumber = -pNumber;
	
	}
	
	while (pNumber > 0) {
	
	    out[i++] = '0' + pNumber % 10;
	    pNumber /= 10;
	
	}
	
	out[i] = '\0';
	reverse(out + sign, i - sign);

	return out;

}

// Reverse function for i to a above ^
static void reverse(char* pString, int pLength) {

	int i = 0;
	int j = pLength - 1;

	char tmp;

	while (i < j) {

		tmp = pString[i];
		pString[i] = pString[j];
		pString[j] = tmp;
		i++; j--;

	}

}