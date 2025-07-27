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
		c = argv[1][++i];
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

	printNFA(state0);
	printf("\n");

	// Convert to DFA
	struct DFAState *startState = buildDFA(state0, nfaStateList, numNFAStates);

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
