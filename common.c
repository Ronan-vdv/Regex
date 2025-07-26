#include "common.h"

char *error = 0;

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

// This is just a helper function that checks if an operator is 'concatible',
// meaning that anything following it should be explicitly concatted
bool operatorIsConcatible(int operator)
{
    return (operator == star || operator == rightparen || operator == atlstone || operator == atmstone);
}