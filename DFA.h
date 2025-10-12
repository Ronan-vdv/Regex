#ifndef DFA_H
#define DFA_H

#include "NFA.h"
#include "common.h"
#include <math.h>

struct DFATransition
{
    char character;
    // Destination state after transition
    struct DFAState *dest;
};

struct NFAStateLL // Linked list of NFAStates
{
    struct NFAState *nfaState;
    struct NFAStateLL *next;
};

struct CharStateMapLL // Used during DFA construction. Helps to keep track of what transitions are for what characters
{
    char character;
    struct NFAStateLL *stateList;
    struct CharStateMapLL *next;
};

// State used by a DFA
struct DFAState
{
    int id; // Global id, mostly for debugging (at least right now)
    bool isFinal;
    bool isVisited; // Useful for traversal
    // bool marked;    // For DFA construction purposes

    struct NFAState **nfaStatesEq; // Used during construction, to check what set of NFAStates this DFAState is equivalent to
    int numNFAStates;

    struct DFATransition **transitions;
    int numTransitions;
    int maxNumTransitions; // Max possible. When numTransitions reaches this limit the array needs to be resized
};

// If you have a list of CharStateMap then the idea is that you store transitions like this:
/*
    [a]-[b]-[c]-[d]->...
     |   |   |   |
     S0  S1  S3  S4
     |   S2  |   |
     |   |   |   |
*/
// This way you can track the set of states that a transition from one set of states leads to

struct DFAState *buildDFA(struct NFAState *s0, struct NFAState **nfaStList, int nfaLength);
void printDFA(struct DFAState *s);
void printfDFAEqStates();
void deleteDFA();

#endif