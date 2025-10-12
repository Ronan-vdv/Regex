#include "DFA.h"

// This leaves a lot to be desired, however the conversion does work.
// I think maybe something like a transition table would have worked better instead of using structs in an OOP fashion

struct DFAState **dfaStateList = 0; // A list of all created states to have an easy handle on all of them
int numDFAStates = 0;
int dfaStateIDCounter = 0;
int dfaStatesSize = 0; // Track the size of the array containing all dfa states

struct DFAState *getNewDFAState()
{
    struct DFAState *s = malloc(sizeof(struct DFAState));
    s->id = dfaStateIDCounter++;
    s->isFinal = false;
    s->isVisited = false;
    s->transitions = 0;
    s->numTransitions = 0;
    s->nfaStatesEq = 0;
    s->numNFAStates = 0;
    s->maxNumTransitions = 0;

    if (numDFAStates == dfaStatesSize) // Check if we've run out of space in the states array. TODO: Probably not necessary, I don't think it can ever exceed due to nature of NFA construction
    {
        struct DFAState **newList = malloc(sizeof(struct DFAState) * dfaStatesSize * 2);
        for (int i = 0; i < dfaStatesSize; i++)
        {
            newList[i] = dfaStateList[i];
        }
        free(dfaStateList);
        dfaStateList = newList;
        dfaStatesSize *= 2;
    }
    dfaStateList[numDFAStates++] = s; // Add to global state list
    return s;
}

// Given a set of equivalent NFA states, either add a new DFA state or return an already existing one that is equivalent to the given states
struct DFAState *addNewDFAState(struct NFAStateLL *stateSet)
{
    for (int i = 0; i < numDFAStates; i++)
    {
        struct DFAState *curState = dfaStateList[i];

        struct NFAStateLL *statePtr = stateSet;
        bool eq = true;
        for (int p = 0; p < curState->numNFAStates; p++)
        {
            if (!statePtr || curState->nfaStatesEq[p]->id != statePtr->nfaState->id) // Difference in number of states or states not the same
            {

                eq = false;
                break;
            }

            statePtr = statePtr->next;
        }

        if (eq && statePtr && statePtr->next) // There are more NFA states in stateSet than in the current DFA state
            eq = false;

        if (eq) // Found an already existing equivalent state
        {
            // Delete the passed-in list since it's no longer needed
            struct NFAStateLL *curPtr = stateSet;
            while (curPtr)
            {
                struct NFAStateLL *next = curPtr->next;
                free(curPtr);
                curPtr = next;
            }

            return curState;
        }
    }

    // If we get here then there are no pre-existing states that match the one being added, so we can create a new one

    struct DFAState *newState = getNewDFAState();
    // First count number of states in the list and also set the Final variable for this new state
    struct NFAStateLL *statePtr = stateSet;
    int scount = 0;
    while (statePtr)
    {
        scount++;
        newState->isFinal = newState->isFinal || statePtr->nfaState->isFinal;
        statePtr = statePtr->next;
    }

    newState->numNFAStates = scount;
    newState->nfaStatesEq = malloc(sizeof(struct NFAState *) * scount);

    // Populate state array from list
    statePtr = stateSet;
    int i = 0;

    while (statePtr)
    {
        newState->nfaStatesEq[i++] = statePtr->nfaState;
        struct NFAStateLL *next = statePtr->next;
        free(statePtr);
        statePtr = next;
    }

    return newState;
}

// Recursively build a structure to get the closure of an nfastate
void getEClosure(struct NFAState *statePtr, struct NFAState ***closureList, int *closureIndex, int *closureArraySize)
{
    // First check if we've already checked this state
    for (int i = 0; i < *closureIndex; i++)
    {
        if ((*closureList)[i]->id == statePtr->id)
            return;
    }

    // Then check if the array is still big enough
    if (*closureIndex >= *closureArraySize)
    {
        struct NFAState **newList = malloc(sizeof(struct NFAState *) * (*closureArraySize) * 2);
        for (int i = 0; i < *closureArraySize; i++)
        {
            newList[i] = (*closureList)[i];
        }

        free(*closureList);
        *closureList = newList;
        *closureArraySize *= 2;
    }

    (*closureList)[(*closureIndex)++] = statePtr;

    if (statePtr->transition1 && statePtr->transition1->emptyTransition)
        getEClosure(statePtr->transition1->dest, closureList, closureIndex, closureArraySize);

    if (statePtr->transition2 && statePtr->transition2->emptyTransition)
        getEClosure(statePtr->transition2->dest, closureList, closureIndex, closureArraySize);
}

struct NFAStateLL *getNewNFAStateLL(struct NFAState *s)
{
    struct NFAStateLL *n = malloc(sizeof(struct NFAStateLL));
    n->next = 0;
    n->nfaState = s;
    return n;
}

struct CharStateMapLL *getNewCharStateMapLL(char c)
{
    struct CharStateMapLL *n = malloc(sizeof(struct CharStateMapLL));
    n->character = c;
    n->next = 0;
    n->stateList = 0;
    return n;
}

void addStateToList(struct NFAStateLL **headPtr, struct NFAState *s)
{
    if (!(*headPtr)) // No states yet
    {
        *headPtr = getNewNFAStateLL(s);
        return;
    }

    if (s->id == (*headPtr)->nfaState->id) // State already present
        return;

    if (s->id < (*headPtr)->nfaState->id) // Need to set this new state as the head
    {
        struct NFAStateLL *newS = getNewNFAStateLL(s);
        newS->next = *headPtr;
        *headPtr = newS;
        return;
    }

    struct NFAStateLL *ptr = *headPtr;
    while (ptr->next && s->id >= ptr->next->nfaState->id) // Find the right position for this state based on order of their ids
    {
        if (s->id == ptr->next->nfaState->id) // State already present in this set
            return;

        ptr = ptr->next;
    }

    // Insert in between two states
    struct NFAStateLL *newS = getNewNFAStateLL(s);
    newS->next = ptr->next;
    ptr->next = newS;
}

void addTransitionToMap(struct CharStateMapLL **headPtr, char c)
{

    struct CharStateMapLL *ptr = *headPtr;
    if (!ptr) // Check if this will be the first transition
    {
        *headPtr = getNewCharStateMapLL(c);
        return;
    }
    struct CharStateMapLL *prev;

    while (ptr)
    {
        if (ptr->character == c) // Transition already exists
            return;
        prev = ptr;
        ptr = ptr->next;
    }

    // Transition does not exist, add it
    prev->next = getNewCharStateMapLL(c);
}

// This basically bridges addTransitionToMap and addStateToList
// It finds the list by character, then adds the state to the correct list
void addStateToCharTransition(struct CharStateMapLL *head, struct NFAState *s, char c)
{
    struct CharStateMapLL *ptr = head;
    while (ptr)
    {
        if (ptr->character == c)
        {
            addStateToList(&(ptr->stateList), s);
            return;
        }
        ptr = ptr->next;
    }
}

void addTransitionToState(struct DFAState *s, char c, struct DFAState *dest)
{
    if (s->numTransitions == s->maxNumTransitions) // Check if array needs to be resized before adding to it
    {
        int newSize = s->maxNumTransitions + 10;
        struct DFATransition **newList = malloc(sizeof(struct DFATransition *) * newSize);
        s->maxNumTransitions = newSize;
        for (int i = 0; i < s->numTransitions; i++)
        {
            newList[i] = s->transitions[i];
        }
        if (s->transitions)
            free(s->transitions);
        s->transitions = newList;
    }

    struct DFATransition *newT = malloc(sizeof(struct DFATransition));
    newT->character = c;
    newT->dest = dest;
    s->transitions[s->numTransitions++] = newT;
}

struct DFAState *buildDFA(struct NFAState *s0, struct NFAState **nfaStList, int nfaLength)
{
    /*
        1. For start state, find e-closure. Result is start state of DFA. Add to list
        2. For each unmarked DFA state:
            2.1 Mark and find transitions. For every NFA state in the set, get the set of states for each transition + e-closure
            2.2 For every transition, add a new unmarked DFA state equivalent
            2.3 When adding a new DFA state, check if there is already an existing equivalent (all NFA states match)
        3. Final states of the DFA contain at least one final state of the NFA
    */

    numDFAStates = 0;
    dfaStateList = malloc(sizeof(struct DFAState) * nfaLength);
    dfaStatesSize = nfaLength;

    // Get start state before the others
    struct DFAState *start = getNewDFAState();

    int closureArraySize = 10;
    int closureCount = 0;
    struct NFAState **closureList = malloc(sizeof(struct NFAState *) * closureArraySize);
    getEClosure(s0, &closureList, &closureCount, &closureArraySize);

    // Initialise nfastate equivalence list by copying from the closure we got
    // Also sort the list
    start->nfaStatesEq = malloc(sizeof(struct NFAState *) * closureCount);
    start->numNFAStates = closureCount;
    for (int i = 0; i < closureCount; i++)
    {
        int currentSmallest = 999999;
        int currentSmallestIndex;
        for (int k = 0; k < closureCount; k++)
        {
            if (closureList[k] && closureList[k]->id < currentSmallest)
            {
                currentSmallest = closureList[k]->id;
                currentSmallestIndex = k;
            }
        }

        start->nfaStatesEq[i] = closureList[currentSmallestIndex];
        start->isFinal = start->isFinal || start->nfaStatesEq[i]->isFinal;
        closureList[currentSmallestIndex] = 0;
    }
    free(closureList);

    for (int stateCount = 0; stateCount < numDFAStates; stateCount++) // For each unprocessed DFA state....
    {
        // Get transitions and e-closures for all states in the NFAState array, while new states are being added

        struct DFAState *currentState = dfaStateList[stateCount];
        struct CharStateMapLL *charStateMapList = 0; // A list of mappings from a character to a set of states, representing transitions

        for (int i = 0; i < currentState->numNFAStates; i++) // For every NFA state in this set of states....
        {
            // For every character transition get the destination and the epsilon-closure
            struct NFAState *currentNFAState = currentState->nfaStatesEq[i];
            if (currentNFAState->transition1 && !currentNFAState->transition1->emptyTransition)
            {
                int closureArraySize = 10;
                int closureCount = 0;
                struct NFAState **closure = malloc(sizeof(struct NFAState *) * closureArraySize);
                getEClosure(currentNFAState->transition1->dest, &closure, &closureCount, &closureArraySize);

                addTransitionToMap(&charStateMapList, currentNFAState->transition1->character->character);
                for (int s = 0; s < closureCount; s++)
                    addStateToCharTransition(charStateMapList, closure[s], currentNFAState->transition1->character->character);

                free(closure);
            }

            if (currentNFAState->transition2 && !currentNFAState->transition2->emptyTransition)
            {
                int closureArraySize = 10;
                int closureCount = 0;
                struct NFAState **closure = malloc(sizeof(struct NFAState *) * closureArraySize);
                getEClosure(currentNFAState->transition2->dest, &closure, &closureCount, &closureArraySize);

                addTransitionToMap(&charStateMapList, currentNFAState->transition2->character->character);
                for (int s = 0; s < closureCount; s++)
                    addStateToCharTransition(charStateMapList, closure[s], currentNFAState->transition2->character->character);

                free(closure);
            }
        }

        // Now iterate over all the sets of states (mapped to characters as transitions) and add new equivalent DFA states for them
        struct CharStateMapLL *ptr = charStateMapList;
        while (ptr)
        {
            // Get an equivalent DFA state and add a transition to it
            struct DFAState *newState = addNewDFAState(ptr->stateList);
            addTransitionToState(currentState, ptr->character, newState);
            struct CharStateMapLL *next = ptr->next;
            free(ptr);
            ptr = next;
        }
    }

    return start;
}

void deleteDFA()
{
    for (int i = 0; i < numDFAStates; i++)
    {
        for (int k = 0; k < dfaStateList[i]->numTransitions; k++)
            free(dfaStateList[i]->transitions[k]);
        free(dfaStateList[i]->transitions);
        free(dfaStateList[i]->nfaStatesEq); // Free the NFAStates array but not the NFA states themselves, since they are still part of an existing NFA
        free(dfaStateList[i]);
    }

    free(dfaStateList);
}

void printDFAHelper(struct DFAState *s)
{
    if (s->isFinal)
        printf("(|%i|)", s->id);
    else
        printf("( %i )", s->id);
}

void printDFA(struct DFAState *s)
{
    if (s->isVisited)
        return;

    for (int i = 0; i < s->numTransitions; i++)
    {
        printDFAHelper(s);
        printf("-%c->", s->transitions[i]->character);
        printDFAHelper(s->transitions[i]->dest);
        printf("\n");
    }

    s->isVisited = true;

    for (int i = 0; i < s->numTransitions; i++)
    {
        printDFA(s->transitions[i]->dest);
    }
}

void printfDFAEqStates()
{
    for (int i = 0; i < numDFAStates; i++)
    {
        char surround = ' ';
        if (dfaStateList[i]->isFinal)
            surround = '|';
        printf("(%c%i%c) => {", surround, dfaStateList[i]->id, surround);
        for (int k = 0; k < dfaStateList[i]->numNFAStates; k++)
        {
            printf("%i", dfaStateList[i]->nfaStatesEq[k]->id);
            if (k < dfaStateList[i]->numNFAStates - 1)
                printf(",");
        }
        printf("}\n");
    }
}