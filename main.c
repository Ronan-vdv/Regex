#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define ANSI_RED "\e[0;31m"
#define ANSI_WHITE "\e[0;37m"

void printError(char *message)
{
	printf("%s%s%s", ANSI_RED, message, ANSI_WHITE);
}

void readRegex(char *regex);
char operatorEnumToChar(int e);
struct regexChar **convertToPostfix(char *, int);

// c == '*' || c == '|' || c == '?' || c == '+'
enum operator
{
	star,	   // *
	or,		   // |
	atmstone,  // ?
	atlstone,  // +
	concat,	   // .
	leftparen, // (
	rightparen // )
};

// A helper struct to represent a character in a regex, with more info
struct regexChar
{
	enum operator operatorEnum;
	char character;
	bool isOperator;
	bool isTerminal;
};

struct NFAState *buildNFA(struct regexChar **input, int length);

// A transition in an NFA
struct NFATransition
{
	struct regexChar *character;
	bool emptyTransition; // If true, then there is no char to transition on
	// Destination states after transition
	struct NFAState *dest;
};

// State used by an NFA
struct NFAState
{
	int id; // Global id, mostly for debugging (at least right now)
	bool isFinal;
	bool isVisited; // Useful for traversal
	// Only two transitions max for the NFAs we want to build
	struct NFATransition *transition1;
	struct NFATransition *transition2;
};

// To house a list of NFAStates
struct NFAStatePtrList
{
	int length;
	struct NFAState ***list; // An array of pointers to state pointers
};

// A 'chunk' of an NFA, representing a set of states with a single state as input and pointers to output
struct NFAPart
{
	struct NFAState *in;
	struct NFAStatePtrList outList;
};

void printNFA(struct NFAState *);
void deleteNFA();
char *error = 0;

int main(int argc, char *argv[])
{

	if (argc < 2)
	{
		printError("No regex provided\n");
		return 0;
	}

	int count = 1;
	int i = 0;
	char c = argv[1][0];
	while (c != '\0')
	{
		count++;
		c = argv[0][i++];
	}

	struct regexChar **res = convertToPostfix(argv[1], count);
	if (error)
	{
		printf("%s", error);
		return 0;
	}
	// printf("%s", res);
	struct regexChar *ptr = res[0];
	int index = 0;
	printf("Postfix representation: ");
	while (ptr && !ptr->isTerminal)
	{
		if (ptr->isOperator)
			printf("%c", operatorEnumToChar(ptr->operatorEnum));
		else
			printf("%c", ptr->character);
		ptr = res[++index];
	}

	printf("\n");

	struct NFAState *state0 = buildNFA(res, index + 1);

	if (error)
		printf("%s", error);

	// printf("%c", state0->transition2->dest);
	printNFA(state0);

	// Convert to DFA HERE

	// Delete the NFA now that it has been used
	deleteNFA(state0);

	// Delete the postfix character data
	ptr = res[0];
	index = 0;
	while (ptr && !ptr->isTerminal)
	{
		free(ptr);
		ptr = res[++index];
	}
	if (ptr)
		free(ptr);

	free(res);

	return 0;
}

// Regex in postfix notation is easier to convert to states
// Implemtation of the shuntingyard algorithm

// Get the operator precedence, lower value is higher precedence
// Takes an operator enum
int getOperatorPrecedence(int operator)
{
	// Parentheses not accounted for
	switch (operator)
	{
	case star:
		return 0;
	case atmstone:
		return 0;
	case atlstone:
		return 0;
	case concat:
		return 1;
	case or:
		return 2;
	default:
		return -1;
	}
}

struct regexChar *getTerminalCharR()
{
	struct regexChar *c1 = malloc(sizeof(struct regexChar));
	c1->isTerminal = true;
	return c1;
}

struct regexChar *getNonTerminalCharRCharacter(char c)
{
	struct regexChar *c1 = malloc(sizeof(struct regexChar));
	c1->character = c;
	c1->isTerminal = false;
	c1->isOperator = false;
	return c1;
}

struct regexChar *getNonTerminalCharROperator(int operator)
{
	struct regexChar *c1 = malloc(sizeof(struct regexChar));
	c1->operatorEnum = operator;
	c1->isTerminal = false;
	c1->isOperator = true;
	return c1;
}

struct NFAState **nfaStateList = 0; // A list of all created states to have an easy handle on all of them
int numNFAStates = 0;				// Needs to be incremented every time a new state is created
int nfaStateIDCounter = 0;			// ID counter to lable each state uniquely
struct NFAState *getNewNFAState()
{
	struct NFAState *s = malloc(sizeof(struct NFAState));
	s->id = nfaStateIDCounter++;
	s->isFinal = false;
	s->isVisited = false;
	s->transition1 = 0;
	s->transition2 = 0;

	nfaStateList[numNFAStates++] = s; // Add to global state list
	return s;
}

struct NFATransition *getNewNFATransition(char c)
{
	struct NFATransition *trans = malloc(sizeof(struct NFATransition));
	trans->character = getNonTerminalCharRCharacter(c);
	trans->dest = 0;
	trans->emptyTransition = false;
}

int operatorCharToEnum(char c)
{
	switch (c)
	{
	case '*':
		return star;
	case '|':
		return or;
	case '?':
		return atmstone;
	case '+':
		return atlstone;
	case '(':
		return leftparen;
	case ')':
		return rightparen;
	default:
		return 0;
	}
}

// Mostly just for debugging
char operatorEnumToChar(int e)
{
	switch (e)
	{
	case star:
		return '*';
	case or:
		return '|';
	case atmstone:
		return '?';
	case atlstone:
		return '+';
	case leftparen:
		return '(';
	case rightparen:
		return ')';
	case concat:
		return '.';
	default:
		return 0;
	}
}

// This is just a helper function that checks if an operator is 'concatible',
// meaning that anything following it should be explicitly concatted
bool operatorIsConcatible(int operator)
{
	return (operator== star || operator== rightparen || operator== atlstone || operator== atmstone);
}

// TODO: Fix memory leak when using brackets
struct regexChar **convertToPostfix(char *string, int length)
{
	char c = string[0];
	int stringPos = 0;

	struct regexChar *input[2 * length]; // The input string converted to charreplacements
	int inputPos = 0;

	// Convert char array to charReplacement array and insert concat operators
	while (c != '\0')
	{
		if (c == '*' || c == '|' || c == '?' || c == '+' || c == '(' || c == ')') // Operator, not character
		{
			if (c == '(' && inputPos > 0)
				if (!input[inputPos - 1]->isOperator || operatorIsConcatible(input[inputPos - 1]->operatorEnum)) // Add concatenation operator in between
				{
					input[inputPos++] = getNonTerminalCharROperator(concat);
				}
			input[inputPos] = getNonTerminalCharROperator(operatorCharToEnum(c));
		}
		else // Normal character
		{
			if (inputPos > 0)
				if (!input[inputPos - 1]->isOperator || operatorIsConcatible(input[inputPos - 1]->operatorEnum)) // Add concatenation operator in between
				{
					input[inputPos++] = getNonTerminalCharROperator(concat);
				}
			input[inputPos] = getNonTerminalCharRCharacter(c);
		}

		inputPos++;
		c = string[++stringPos];
	}
	input[inputPos] = getTerminalCharR(); // Add termination to list

	int stackPos = 0;							 // Use indices as pointers on the stack/queue
	struct regexChar *operatorStack[2 * length]; // Create a stack to store the operators while moving through the algorithm
	struct regexChar **output = malloc(2 * length * sizeof(struct regexChar *));
	int outputIndex = 0;

	struct regexChar *ptr = input[0];
	inputPos = 0;
	while (!ptr->isTerminal)
	{
		if (ptr->isOperator)
		{
			if (ptr->operatorEnum == leftparen)
			{
				struct regexChar *c1 = getNonTerminalCharROperator(leftparen);
				operatorStack[stackPos++] = c1;
			}
			else if (ptr->operatorEnum == rightparen)
			{
				// If right parenthesis, try looking for corresponding left paren
				while (1)
				{
					if (stackPos <= 0) // Stack is empty without finding matching left parenthesis
					{
						error = "Mismatched parentheses\n";
						return 0;
					}
					if (operatorStack[stackPos - 1]->operatorEnum == leftparen)
					{
						// Remove left parenthesis and dispose of it, then move on
						free(operatorStack[--stackPos]);

						// Also free right parenthesis memory, since it will no longer be used
						free(ptr);
						break;
					}

					// Pop operator from top of stack into output
					output[outputIndex++] = operatorStack[--stackPos];
				}
			}
			else // A different operator
			{
				while (1)
				{
					if (stackPos <= 0)
						break;
					struct regexChar *popped = operatorStack[stackPos - 1];

					if (popped->operatorEnum == leftparen)
						break;

					int p1 = getOperatorPrecedence(ptr->operatorEnum);
					int p2 = getOperatorPrecedence(popped->operatorEnum);

					if (p2 > p1 && (p1 != p2))
						break;

					// If passed all these checks, then pop top from stack, add to output
					stackPos--;
					output[outputIndex++] = popped;
				}
				// Push current operator to operator stack
				operatorStack[stackPos++] = ptr;
			}
		}
		else // Just a character, add to output
		{
			output[outputIndex++] = ptr;
		}
		ptr = input[++inputPos];
	}

	// Pop remaining operators and place on output
	while (stackPos > 0)
	{
		if (operatorStack[stackPos - 1]->operatorEnum == leftparen) // Rogue left parenthesis
		{
			error = "Mismatched parentheses\n";
			return 0;
		}
		output[outputIndex++] = operatorStack[--stackPos];
	}
	output[outputIndex] = ptr; // Add terminating item, reuse the one from the input array

	return output;
}

// Add two statelists together
struct NFAStatePtrList concatLists(struct NFAStatePtrList l1, struct NFAStatePtrList l2)
{
	struct NFAStatePtrList newList;
	newList.length = l1.length + l2.length;
	newList.list = malloc(sizeof(struct NFAState **) * (l1.length + l2.length)); // Create new list of combined size

	for (int i = 0; i < l1.length; i++)
		newList.list[i] = l1.list[i];

	for (int i = 0; i < l2.length; i++)
		newList.list[l1.length + i] = l2.list[i];

	return newList;
}

// Make the out pointers of the first nfa part point to the state passed int
struct NFAPart mergeNFAParts(struct NFAPart p1, struct NFAState *p2)
{
	// Update pointers of the states housed in p1
	// I learned this pointer trick from an article about NFAs
	for (int i = 0; i < p1.outList.length; i++)
	{
		*(p1.outList.list[i]) = p2;
	}

	struct NFAPart newPart;
	newPart.in = p1.in;
	return newPart;
}

// Convenience method to convert a single pointer to NFA state into a pointer list, of length 1
// Seems to cause a segfault?
struct NFAStatePtrList stateToNFAPtrList(struct NFAState *s)
{
	struct NFAStatePtrList newList;
	newList.length = 1;
	newList.list = malloc(sizeof(struct NFAState **));
	newList.list[0] = &s;

	return newList;
}

//  Build an NFA from a postfix-form input string
struct NFAState *buildNFA(struct regexChar **input, int length)
{
	nfaStateList = malloc(sizeof(struct NFAState *) * length); // Should be enough to house all states
	struct NFAPart stack[length];							   // NFAParts are not dynamically allocated as they are only used here
	int inputPos = 0, stackPos = 0;

	// Iterate over input and build NFA on the stack
	while (inputPos < length)
	{
		struct regexChar *character = input[inputPos++];

		if (character->isTerminal)
			break;

		if (character->isOperator)
		{
			// Add states relevant to each operator
			switch (character->operatorEnum)
			{
			case concat: // Pop two from stack, concat states them and add new one back
			{
				if (stackPos < 2) // Less than two available. This should never happen
				{
					error = "(THIS SHOULD NOT BE POSSIBLE) Invalid use of operator ."; // + operatorEnumToChar(character->operatorEnum);
					return 0;
				}
				struct NFAPart p2 = stack[--stackPos];
				struct NFAPart p1 = stack[--stackPos];

				struct NFAPart newPart = mergeNFAParts(p1, p2.in);
				newPart.outList = p2.outList;
				stack[stackPos++] = newPart;
				free(p1.outList.list);

				break;
			}
			case or: // Pop two states from stack, 'or' them
			{
				if (stackPos < 2) // Less than two available
				{
					error = "Invalid use of operator |"; // + operatorEnumToChar(character->operatorEnum);
					return 0;
				}

				// Pop 2
				struct NFAPart p2 = stack[--stackPos];
				struct NFAPart p1 = stack[--stackPos];

				// Create new state and two empty transitions to the existing states
				struct NFATransition *trans1 = getNewNFATransition(0);
				trans1->emptyTransition = true;
				trans1->dest = p1.in;

				struct NFATransition *trans2 = getNewNFATransition(0);
				trans2->emptyTransition = true;
				trans2->dest = p2.in;

				struct NFAState *newState = getNewNFAState();
				newState->transition1 = trans1;
				newState->transition2 = trans2;

				struct NFAPart newPart;
				newPart.in = newState;
				newPart.outList = concatLists(p1.outList, p2.outList);

				free(p1.outList.list);
				free(p2.outList.list);

				// Push new state
				stack[stackPos++] = newPart;
				break;
			}
			case atmstone:
			{
				if (stackPos < 1)
				{
					error = "Invalid use of operator ?"; // + operatorEnumToChar(character->operatorEnum);
					return 0;
				}
				struct NFAPart p1 = stack[--stackPos];

				struct NFAState *newState = getNewNFAState();

				newState->transition1 = getNewNFATransition(0);
				newState->transition2 = getNewNFATransition(0);
				newState->transition1->emptyTransition = true;
				newState->transition2->emptyTransition = true;

				newState->transition1->dest = p1.in; // Bind the first transition to the top fragment's input state

				// We need to create a new list just to be able to easily concat the state destination pointer with the existing p1 list
				struct NFAStatePtrList newList;
				newList.length = 1;
				newList.list = malloc(sizeof(struct NFAState **));
				newList.list[0] = &(newState->transition2->dest); // Bind up the second transition of the new state as the 'output' of this list

				struct NFAPart newP;
				newP.in = newState;
				newP.outList = concatLists(p1.outList, newList);

				free(newList.list);
				free(p1.outList.list);

				stack[stackPos++] = newP;

				break;
			}
			case atlstone:
			{
				if (stackPos < 1)
				{
					error = "Invalid use of operator +"; // + operatorEnumToChar(character->operatorEnum);
					return 0;
				}
				struct NFAPart p1 = stack[--stackPos];

				struct NFAState *state = getNewNFAState();
				state->transition1 = getNewNFATransition(0);
				state->transition1->emptyTransition = true;
				state->transition1->dest = p1.in; // First transition goes to the existing nfa part's 'in' state

				state->transition2 = getNewNFATransition(0);
				state->transition2->emptyTransition = true;

				struct NFAStatePtrList newList;
				newList.length = 1;
				newList.list = malloc(sizeof(struct NFAState **));
				newList.list[0] = &state->transition2->dest;

				struct NFAPart part = mergeNFAParts(p1, state);
				part.outList = newList;

				free(p1.outList.list);

				stack[stackPos++] = part;

				break;
			}
			case star:
			{
				struct NFAPart p1 = stack[--stackPos];

				struct NFAState *state = getNewNFAState();
				state->transition1 = getNewNFATransition(0);
				state->transition1->emptyTransition = true;
				state->transition1->dest = p1.in;

				state->transition2 = getNewNFATransition(0);
				state->transition2->emptyTransition = true;

				struct NFAPart newPart = mergeNFAParts(p1, state);
				newPart.in = state;

				struct NFAStatePtrList newList;
				newList.length = 1;
				newList.list = malloc(sizeof(struct NFAState **));
				newList.list[0] = &state->transition2->dest;

				newPart.outList = newList;

				stack[stackPos++] = newPart;
				break;
			}
			default:
				break;
			}
		}
		else // Push char transition to stack
		{
			// Create transition
			struct NFATransition *trans = getNewNFATransition(character->character);

			// Create state
			struct NFAState *state = getNewNFAState();

			state->transition1 = trans;

			struct NFAPart part;
			part.in = state;
			// part.outList.list = stateToNFAPtrList(state->transition1->dest).list;
			part.outList.list = malloc(sizeof(struct NFAState **)); // Create list of size 1
			part.outList.list[0] = &state->transition1->dest;
			part.outList.length = 1;

			stack[stackPos++] = part;
		}
	}

	// Check if there is only one item on the stack
	if (stackPos != 1)
	{
		error = "Something went wrong while constructing NFA";
		return 0;
	}

	// Add in a final state
	struct NFAState *state = getNewNFAState();
	state->isFinal = true;
	struct NFAPart part = stack[--stackPos];
	struct NFAState *startState = mergeNFAParts(part, state).in;

	free(part.outList.list);

	return startState;
}

void printNFAHelper(struct NFAState *s0)
{
	if (!s0)
		return;

	char finalIndicator = ' ';
	if (s0->isFinal)
		finalIndicator = '|';

	if (s0->isVisited)
	{
		printf("(%c%i%c)", finalIndicator, s0->id, finalIndicator);
		return;
	}

	s0->isVisited = true;

	if (s0->transition1)
	{
		char transition = '-';

		if (!s0->transition1->emptyTransition)
			transition = s0->transition1->character->character;
		printf("(%c%i%c)-%c->", finalIndicator, s0->id, finalIndicator, transition);

		printNFAHelper(s0->transition1->dest);
	}

	if (s0->transition2)
	{
		printf("\n");

		char transition = '-';
		if (!s0->transition2->emptyTransition)
			transition = s0->transition2->character->character;
		printf("(%c%i%c)-%c->", finalIndicator, s0->id, finalIndicator, transition);

		printNFAHelper(s0->transition2->dest);
	}

	if (!s0->transition1 && !s0->transition2)
	{
		printf("(%c%i%c)", finalIndicator, s0->id, finalIndicator);
	}
}

// Print an NFA, given its first state
// Mostly for debugging
// If we want to get really mf'ing wild, make this pretty-print a graph of states :)
void printNFA(struct NFAState *s0)
{
	for (int i = 0; i < numNFAStates; i++)
		nfaStateList[i]->isVisited = false;
	printNFAHelper(s0);
}

// Deletes all states found in the global NFA state list
void deleteNFA()
{
	for (int i = 0; i < numNFAStates; i++)
	{
		if (nfaStateList[i]->transition1)
		{
			free(nfaStateList[i]->transition1->character);
			free(nfaStateList[i]->transition1);
		}

		if (nfaStateList[i]->transition2)
		{
			free(nfaStateList[i]->transition2->character);
			free(nfaStateList[i]->transition2);
		}

		free(nfaStateList[i]);
	}

	free(nfaStateList);
	nfaStateList = 0;
	numNFAStates = 0;
}