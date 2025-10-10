#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "common.h"
#include "NFA.h"
#include "DFA.h"
#include "postfix.h"

#define ANSI_RED "\e[0;31m"
#define ANSI_WHITE "\e[0;37m"

void printError(char *message)
{
	printf("%s%s%s", ANSI_RED, message, ANSI_WHITE);
}

int main(int argc, char *argv[])
{
	bool verbose = false;
	int flagIndex = -1;

	if (strcmp(argv[1], "-v") == 0) // Verbose flag
	{
		verbose = true;
	}
	else if (argc > 2)
	{
		printError("Unknown flag/Too many arguments\n");
		return 0;
	}

	if (argc < 2 || (verbose && argc < 3))
	{
		printError("No regex provided\n");
		return 0;
	}

	if ((fseek(stdin, 0, SEEK_END), ftell(stdin)) > 0)
		while (1)
		{
			char input = fgetc(stdin);
			if (input <= 0)
				break;
			// Then process input from stdin
		}

	int regIndex = argc - 1;
	int count = 1;
	int i = 0;
	char c = argv[regIndex][0];
	while (c != '\0')
	{
		count++;
		c = argv[regIndex][++i];
	}

	struct regexChar **res = convertToPostfix(argv[regIndex], count);
	if (error)
	{
		printError(error);
		printf("\n");
		return 0;
	}
	// printf("%s", res);
	int index = 0;
	struct regexChar *ptr = res[0];
	if (verbose)
	{
		printf("Postfix representation: ");
		while (ptr && !ptr->isTerminal)
		{
			if (ptr->isOperator)
				printf("%c", operatorEnumToChar(ptr->operatorEnum));
			else
				printf("%c", ptr->character);
			ptr = res[++index];
		}

		printf("\n\n");
	}
	else
	{
		while (ptr && !ptr->isTerminal)
			ptr = res[++index];
	}

	struct NFAState *state0 = buildNFA(res, index + 1);

	if (error)
	{
		printError(error);
		printf("\n");
		return 0;
	}

	if (verbose)
	{
		printf("NFA:\n");
		printNFA(state0);
		printf("\n");
	}

	// Convert to DFA
	struct DFAState *startState = buildDFA(state0, nfaStateList, numNFAStates);

	if (verbose)
	{
		printf("DFA:\n");
		printDFA(startState);
	}

	// Delete the NFA now that it has been used
	deleteNFA();
	deleteDFA();

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
