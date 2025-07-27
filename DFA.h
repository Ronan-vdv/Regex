#ifndef DFA_H
#define DFA_H

#include "NFA.h"
#include "common.h"
#include <math.h>

// Used while constructing DFA
struct NFAClosureList
{
    int stateId;
    // A list of ids of NFA states that are the e-closure of a state
    int *ids;
    int listSize;
};

struct DFATransition
{
    struct regexChar *character;
    // Destination state after transition
    struct DFAState *dest;
};

// State used by a DFA
struct DFAState
{
    int id; // Global id, mostly for debugging (at least right now)
    bool isFinal;
    bool isVisited; // Useful for traversal

    struct DFATransition **transitions;
    int numTransitions;
};

struct DFAState *buildDFA(struct NFAState *s0, struct NFAState **nfaStList, int nfaLength);
void deleteDFA();

#endif