#include "postfix.h"

// Regex in postfix notation is easier to convert to states
// Implemtation of the shuntingyard algorithm

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
        stringPos++;
        c = string[stringPos];
    }
    input[inputPos] = getTerminalCharR(); // Add termination to list

    int stackPos = 0;                            // Use indices as pointers on the stack/queue
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
                        error = "Mismatched parentheses";
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
            error = "Mismatched parentheses";
            return 0;
        }
        output[outputIndex++] = operatorStack[--stackPos];
    }
    output[outputIndex] = ptr; // Add terminating item, reuse the one from the input array

    return output;
}