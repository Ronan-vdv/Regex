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
	star,
	or,
	atmstone,
	atlstone,
	concat,
	leftparen,
	rightparen
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
	bool isFinal;
	// Only two transitions max for the NFAs we want to build
	struct NFATransition *transition1;
	struct NFATransition *transition2;
};

void printNFA(struct NFAState *);
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

	printNFA(state0);

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

// Build an NFA from a postfix-form input string
struct NFAState *buildNFA(struct regexChar **input, int length)
{
	struct NFAState *stack[length];
	int inputPos = 0, stackPos = 0;

	// Iterate over input and build NFA on the stack
	while (inputPos < length)
	{
		struct regexChar *character = input[inputPos++];

		if (character->isTerminal)
			break;

		if (character->isOperator)
		{
			if (stackPos <= 0) // At bottom of the stack
			{
				error = "Invalid use of operator " + operatorEnumToChar(character->operatorEnum);
				return 0;
			}
			// Add states relevant to each operator
			switch (character->operatorEnum)
			{
			case concat: // Pop two from stack, concat them and add back
			{
				if (stackPos < 2) // Less than two available
				{
					error = "Invalid use of operator " + operatorEnumToChar(character->operatorEnum);
					return 0;
				}
				struct NFAState *s2 = stack[--stackPos];
				struct NFAState *s1 = stack[stackPos - 1];

				// Just keep it on the stack and 'append' the second state as a transition
				s1->transition1->dest = s2;
				break;
			}
			case or: // Pop two states from stack, 'or' them
			{
				if (stackPos < 2) // Less than two available
				{
					error = "Invalid use of operator " + operatorEnumToChar(character->operatorEnum);
					return 0;
				}

				// Pop 2
				struct NFAState *s2 = stack[--stackPos];
				struct NFAState *s1 = stack[--stackPos];

				// Create new state and two empty transitions to the existing states
				struct NFATransition *trans1 = malloc(sizeof(struct NFATransition));
				trans1->emptyTransition = true;
				trans1->dest = s1;

				struct NFATransition *trans2 = malloc(sizeof(struct NFATransition));
				trans2->emptyTransition = true;
				trans2->dest = s2;

				struct NFAState *newState = malloc(sizeof(struct NFAState));
				newState->isFinal = false;
				newState->transition1 = trans1;
				newState->transition2 = trans2;

				// Push new state
				stack[stackPos++] = newState;
				break;
			}
			case star:
				/* code */
				break;

			default:
				break;
			}
		}
		else // Push char transition to stack
		{
			// Create transition
			struct NFATransition *trans = malloc(sizeof(struct NFATransition));
			trans->character = getNonTerminalCharRCharacter(character->character);
			trans->dest = 0;
			trans->emptyTransition = false;

			// Create state
			struct NFAState *state = malloc(sizeof(struct NFAState));
			state->transition1 = trans;
			state->transition2 = 0;
			state->isFinal = false;
			stack[stackPos++] = state;
		}
	}

	return stack[0];
}

int stateCounter = 0;
void printNFAHelper(struct NFAState *s0)
{
	int me = stateCounter++;

	if (s0)
	{
		if (s0->transition1)
		{

			char transition = '-';
			if (!s0->transition1->emptyTransition)
				transition = s0->transition1->character->character;
			printf("(%i)-%c->", me, transition);

			printNFAHelper(s0->transition1->dest);
		}
		if (s0->transition2)
		{
			printf("\n");

			char transition = '-';
			if (!s0->transition2->emptyTransition)
				transition = s0->transition2->character->character;
			printf("(%i)-%c->", me, transition);

			printNFAHelper(s0->transition2->dest);
		}
	}
}

// Print an NFA, given its first state
// Mostly for debugging
// If we want to get really mf'ing wild, make this pretty-print a graph of states :)
void printNFA(struct NFAState *s0)
{
	stateCounter = 0;
	printNFAHelper(s0);
}