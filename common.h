#ifndef COMMON_H
#define COMMON_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

extern char *error;

// c == '*' || c == '|' || c == '?' || c == '+'
enum operator
{
    star, // *
    or
    ,          // |
    atmstone,  // ?
    atlstone,  // +
    concat,    // .
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

int operatorCharToEnum(char c);

// Mostly just for debugging
char operatorEnumToChar(int e);
// Get the operator precedence, lower value is higher precedence
// Takes an operator enum
int getOperatorPrecedence(int operator);

// This is just a helper function that checks if an operator is 'concatible',
// meaning that anything following it should be explicitly concatted
bool operatorIsConcatible(int operator);
struct regexChar *getTerminalCharR();
struct regexChar *getNonTerminalCharRCharacter(char c);
struct regexChar *getNonTerminalCharROperator(int operator);

#endif