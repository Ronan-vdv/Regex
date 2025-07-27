#include "DFA.h"

// Need to traverse the NFA and convert NFA states to DFA states.
// We do this by looking at the e-closures of the states, converting a set of those states to a single DFA state
// So for each state and each transition in the NFA, look at all the next states that can be reached with that. This includes empty transitions
// 1. For each state, start by checking e-closure to create equivalent set of states.
// 2. For each transition of each NFA state in that set, create a set of reachable states for each transition
// 3. Either create a new equivalent state

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

// Recursively build a structure to get the closure of an nfastate
void getEClosure(struct NFAState *statePtr, int **closureList, int *closureIndex, int *closureArraySize)
{
    // First check if we've already checked this state
    for (int i = 0; i < *closureIndex; i++)
    {
        if ((*closureList)[i] == statePtr->id)
            return;
    }

    // Then check if the array is still big enough
    if (*closureIndex >= *closureArraySize)
    {
        int *newList = malloc(sizeof(int) * (*closureArraySize) * 2);
        for (int i = 0; i < *closureArraySize; i++)
        {
            newList[i] = (*closureList)[i];
        }

        free(*closureList);
        *closureList = newList;
        *closureArraySize *= 2;
    }

    (*closureList)[(*closureIndex)++] = statePtr->id;

    if (statePtr->transition1 && statePtr->transition1->emptyTransition)
        getEClosure(statePtr->transition1->dest, closureList, closureIndex, closureArraySize);

    if (statePtr->transition2 && statePtr->transition2->emptyTransition)
        getEClosure(statePtr->transition2->dest, closureList, closureIndex, closureArraySize);
}

// Get the index of a certain closure in an array
int getIndexOfClosure(struct NFAClosureList *targetClosure, struct NFAClosureList *closureLists, int closureArraySize)
{
    for (int i = 0; i < closureArraySize; i++)
    {
        // Now compare closures to see if they're equivalent
        bool isRightOne = (closureLists)[i].listSize == targetClosure->listSize;
        for (int k = 0; k < targetClosure->listSize && isRightOne; k++)
            isRightOne = targetClosure->ids[k] == (closureLists[i]).ids[k];

        if (isRightOne)
            return i;
    }

    return -1;
}

struct DFAState *buildDFA(struct NFAState *s0, struct NFAState **nfaStList, int nfaLength)
{
    numDFAStates = 0;
    dfaStateList = malloc(sizeof(struct DFAState) * nfaLength);
    dfaStatesSize = nfaLength;

    struct DFAState *start = 0;

    struct NFAClosureList *closures = malloc(sizeof(struct NFAClosureList) * nfaLength);

    // First, get epsilon closures of states
    for (int i = 0; i < nfaLength; i++)
    {
        struct NFAState *ptr = nfaStList[i];
        int closureArraySize = 10;
        int closureCount = 0;
        int *closureList = malloc(sizeof(int) * closureArraySize);

        getEClosure(ptr, &closureList, &closureCount, &closureArraySize);

        closures[i].stateId = ptr->id;
        closures[i].listSize = closureCount;
        closures[i].ids = closureList;

        struct DFAState *newState = getNewDFAState();
    }

    // For each e-closure and created equivalent DFA state,
    // Start building up transitions
    for (int i = 0; i < nfaLength; i++)
    {
        int closureCount = closures[i].listSize;
        int *closureList = closures[i].ids;

        // For every state listed in the closure, get transitions to existing NFA states
        // The new destination in the DFA will be the e-closure of that state's equivalent
        for (int k = 0; k < closureCount; k++)
        {
            struct NFAState *nfas = getNFAStateFromId(closureList[k]);

            if (nfas->transition1 && !nfas->transition1->emptyTransition)
            {
                int closureArraySize2 = 10;
                int closureCount2 = 0;
                int *closureList2 = malloc(sizeof(int) * closureArraySize2);
                getEClosure(nfas->transition1->dest, &closureList2, &closureCount2, &closureArraySize2);

                struct NFAClosureList cl2;
                cl2.ids = closureList2;
                cl2.stateId = nfas->id;
                cl2.listSize = closureCount2;

                int ind = getIndexOfClosure(&cl2, closures, nfaLength);
                printf("Adding transition from %i to %i on %c\n", i, ind, nfas->transition1->character->character);
                free(closureList2);
            }
        }
    }

    for (int i = 0; i < nfaLength; i++)
    {
        for (int k = 0; k < closures[i].listSize; k++)
            printf("%i, ", closures[i].ids[k]);
        printf("\n");
        free(closures[i].ids);
    }

    free(closures);

    return 0;
}

void deleteDFA()
{
    // TODO
    //  for (int i = 0; i < numDFAStates; i++)
    //  {
    //      free(dfaStateList[i]);
    //  }

    // free(dfaStateList);
}