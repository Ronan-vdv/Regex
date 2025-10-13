#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "common.h"
#include "NFA.h"
#include "DFA.h"
#include "postfix.h"

#define ANSI_RED "\e[0;31m"
#define ANSI_CYAN "\e[0;36m"
#define ANSI_RESET "\e[m"

void printError(char *message)
{
	printf("%s%s%s", ANSI_RED, message, ANSI_RESET);
}

void printUsage()
{
	printf("Usage: rex [-v] PATTERN\n");
}

int main(int argc, char *argv[])
{
	bool verbose = false;
	int flagIndex = -1;

	if (argc < 2)
	{
		printUsage();
		return 0;
	}

	if (!strcmp(argv[1], "-v")) // Verbose flag
	{
		verbose = true;
		if (argc != 3)
		{
			printUsage();
			return 0;
		}
	}
	else if (argc != 2)
	{
		printUsage();
		return 0;
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
		printf("Postfix representation of regex:\n");
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
		printf("DFA state equivalence to NFA:\n");
		printfDFAEqStates();
		printf("\nDFA:\n");
		printDFA(startState);
	}

	int nmBufferSize = 50;
	int nmBufferReadIndex = 0;
	int nmBufferWriteIndex = 0;
	char *nmBuffer = malloc(sizeof(char) * nmBufferSize); // NonMatchBuffer to hold all the rest of the string that does not match (for some nice output later)

	int tokenBufferSize = 50;
	int tokenBufferReadIndex = 0;
	int tokenBufferWriteIndex = 0;
	char *tokenBuffer = malloc(sizeof(char) * tokenBufferSize); // To hold existing token (string match) as it gets matched

	struct DFAState *currentState = startState;

	char line[4096];
	while (fgets(line, sizeof(line), stdin))
	{
		size_t len = strlen(line);
		if (len && line[len - 1] == '\n')
			line[len - 1] = '\0';

		int matchStart = 0;
		int matchEnd = 0;
		bool startedMatch = false; // Started a potential match
		bool foundMatch = false;   // Found a match (went to a final state)
		bool lineMatch = false;

		for (int l = 0; l < len;)
		{
			char input = line[l];
			bool canTransition = false;

			// Find a transition that will work for this input
			for (int i = 0; i < currentState->numTransitions; i++)
			{
				if (currentState->transitions[i]->character == input) // Perform transition
				{
					if (!startedMatch)
					{
						startedMatch = true;
						matchStart = l;
					}

					canTransition = true;
					currentState = currentState->transitions[i]->dest;

					if (currentState->isFinal) // Print everything in this line
					{
						lineMatch = true; // There has been at least one match in this line
						// Print first part of line (after last match)

						for (int ch = matchEnd; ch < matchStart; ch++)
							printf("%c", line[ch]);

						matchEnd = l + 1;

						// Print match
						printf("%s", ANSI_CYAN);
						for (int ch = matchStart; ch < l + 1; ch++)
							printf("%c", line[ch]);
						printf("%s", ANSI_RESET);

						// Reset how we measure the next match
						matchStart = matchEnd;
						foundMatch = true;
					}
					break;
				}
			}

			l++;
			if (!canTransition) // Whatever has been processed since the last match checkpoint is no longer valid
			{
				currentState = startState;
				startedMatch = false;
				if (foundMatch) // Reset to last matchend
					l = matchEnd;

				foundMatch = false;
			}
		}

		if (lineMatch) // Only print the rest of the chars in this line if there was at least one match
		{
			for (int i = matchEnd; i < len; i++)
				printf("%c", line[i]);
			printf("\n");
		}
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
