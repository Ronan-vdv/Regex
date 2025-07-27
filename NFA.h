#ifndef NFA_H
#define NFA_H

#include <stdbool.h>
#include "common.h"

struct NFAState *buildNFA(struct regexChar **input, int length);

extern struct NFAState **nfaStateList;
extern int numNFAStates;

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

// Add two statelists together
struct NFAStatePtrList concatLists(struct NFAStatePtrList l1, struct NFAStatePtrList l2);
// Make the out pointers of the first nfa part point to the state passed int
struct NFAPart mergeNFAParts(struct NFAPart p1, struct NFAState *p2);

// Convenience method to convert a single pointer to NFA state into a pointer list, of length 1
// Seems to cause a segfault?
struct NFAStatePtrList stateToNFAPtrList(struct NFAState *s);
//  Build an NFA from a postfix-form input string
struct NFAState *buildNFA(struct regexChar **input, int length);

struct NFAState *getNFAStateFromId(int id);

// Print an NFA, given its first state
// Mostly for debugging
void printNFA(struct NFAState *s0);

void printNFAHelper(struct NFAState *s0);
// Deletes all states found in the global NFA state list
void deleteNFA();

#endif