#include "NFA.h"

#include <stdbool.h>

struct NFAState **nfaStateList = 0; // A list of all created states to have an easy handle on all of them
int numNFAStates = 0;               // Needs to be incremented every time a new state is created
int nfaStateIDCounter = 0;          // ID counter to lable each state uniquely

struct NFAState *getNewNFAState()
{
    struct NFAState *s = malloc(sizeof(struct NFAState));
    s->id = nfaStateIDCounter++;
    s->isFinal = false;
    s->isVisited = false;
    s->transition1 = 0;
    s->transition2 = 0;

    nfaStateList[numNFAStates++] = s; // Add to global state list
    return s;
}

struct NFATransition *getNewNFATransition(char c)
{
    struct NFATransition *trans = malloc(sizeof(struct NFATransition));
    trans->character = getNonTerminalCharRCharacter(c);
    trans->dest = 0;
    trans->emptyTransition = false;
}

// Add two statelists together
struct NFAStatePtrList concatLists(struct NFAStatePtrList l1, struct NFAStatePtrList l2)
{
    struct NFAStatePtrList newList;
    newList.length = l1.length + l2.length;
    newList.list = malloc(sizeof(struct NFAState **) * (l1.length + l2.length)); // Create new list of combined size

    for (int i = 0; i < l1.length; i++)
        newList.list[i] = l1.list[i];

    for (int i = 0; i < l2.length; i++)
        newList.list[l1.length + i] = l2.list[i];

    return newList;
}

// Make the out pointers of the first nfa part point to the state passed int
struct NFAPart mergeNFAParts(struct NFAPart p1, struct NFAState *p2)
{
    // Update pointers of the states housed in p1
    // I learned this pointer trick from an article about NFAs
    for (int i = 0; i < p1.outList.length; i++)
    {
        *(p1.outList.list[i]) = p2;
    }

    struct NFAPart newPart;
    newPart.in = p1.in;
    return newPart;
}

// Convenience method to convert a single pointer to NFA state into a pointer list, of length 1
// Seems to cause a segfault?
struct NFAStatePtrList stateToNFAPtrList(struct NFAState *s)
{
    struct NFAStatePtrList newList;
    newList.length = 1;
    newList.list = malloc(sizeof(struct NFAState **));
    newList.list[0] = &s;

    return newList;
}

//  Build an NFA from a postfix-form input string
struct NFAState *buildNFA(struct regexChar **input, int length)
{
    nfaStateList = malloc(sizeof(struct NFAState *) * length); // Should be enough to house all states
    struct NFAPart stack[length];                              // NFAParts are not dynamically allocated as they are only used here
    int inputPos = 0, stackPos = 0;

    // Iterate over input and build NFA on the stack
    while (inputPos < length)
    {
        struct regexChar *character = input[inputPos++];

        if (character->isTerminal)
            break;

        if (character->isOperator)
        {
            // Add states relevant to each operator
            switch (character->operatorEnum)
            {
            case concat: // Pop two from stack, concat states them and add new one back
            {
                if (stackPos < 2) // Less than two available
                {
                    error = "Invalid use of operator (concatenation)";
                    return 0;
                }
                struct NFAPart p2 = stack[--stackPos];
                struct NFAPart p1 = stack[--stackPos];

                struct NFAPart newPart = mergeNFAParts(p1, p2.in);
                newPart.outList = p2.outList;
                stack[stackPos++] = newPart;
                free(p1.outList.list);

                break;
            }
            case or: // Pop two states from stack, 'or' them
            {
                if (stackPos < 2) // Less than two available
                {
                    error = "Invalid use of operator |"; // + operatorEnumToChar(character->operatorEnum);
                    return 0;
                }

                // Pop 2
                struct NFAPart p2 = stack[--stackPos];
                struct NFAPart p1 = stack[--stackPos];

                // Create new state and two empty transitions to the existing states
                struct NFATransition *trans1 = getNewNFATransition(0);
                trans1->emptyTransition = true;
                trans1->dest = p1.in;

                struct NFATransition *trans2 = getNewNFATransition(0);
                trans2->emptyTransition = true;
                trans2->dest = p2.in;

                struct NFAState *newState = getNewNFAState();
                newState->transition1 = trans1;
                newState->transition2 = trans2;

                struct NFAPart newPart;
                newPart.in = newState;
                newPart.outList = concatLists(p1.outList, p2.outList);

                free(p1.outList.list);
                free(p2.outList.list);

                // Push new state
                stack[stackPos++] = newPart;
                break;
            }
            case atmstone:
            {
                if (stackPos < 1)
                {
                    error = "Invalid use of operator ?"; // + operatorEnumToChar(character->operatorEnum);
                    return 0;
                }
                struct NFAPart p1 = stack[--stackPos];

                struct NFAState *newState = getNewNFAState();

                newState->transition1 = getNewNFATransition(0);
                newState->transition2 = getNewNFATransition(0);
                newState->transition1->emptyTransition = true;
                newState->transition2->emptyTransition = true;

                newState->transition1->dest = p1.in; // Bind the first transition to the top fragment's input state

                // We need to create a new list just to be able to easily concat the state destination pointer with the existing p1 list
                struct NFAStatePtrList newList;
                newList.length = 1;
                newList.list = malloc(sizeof(struct NFAState **));
                newList.list[0] = &(newState->transition2->dest); // Bind up the second transition of the new state as the 'output' of this list

                struct NFAPart newP;
                newP.in = newState;
                newP.outList = concatLists(p1.outList, newList);

                free(newList.list);
                free(p1.outList.list);

                stack[stackPos++] = newP;

                break;
            }
            case atlstone:
            {
                if (stackPos < 1)
                {
                    error = "Invalid use of operator +"; // + operatorEnumToChar(character->operatorEnum);
                    return 0;
                }
                struct NFAPart p1 = stack[--stackPos];

                struct NFAState *state = getNewNFAState();
                state->transition1 = getNewNFATransition(0);
                state->transition1->emptyTransition = true;
                state->transition1->dest = p1.in; // First transition goes to the existing nfa part's 'in' state

                state->transition2 = getNewNFATransition(0);
                state->transition2->emptyTransition = true;

                struct NFAStatePtrList newList;
                newList.length = 1;
                newList.list = malloc(sizeof(struct NFAState **));
                newList.list[0] = &state->transition2->dest;

                struct NFAPart part = mergeNFAParts(p1, state);
                part.outList = newList;

                free(p1.outList.list);

                stack[stackPos++] = part;

                break;
            }
            case star:
            {
                if (stackPos < 1) // Less than one item left or the previous item is an operator
                {
                    error = "Invalid use of operator *";
                    return 0;
                }
                struct NFAPart p1 = stack[--stackPos];

                struct NFAState *state = getNewNFAState();
                state->transition1 = getNewNFATransition(0);
                state->transition1->emptyTransition = true;
                state->transition1->dest = p1.in;

                state->transition2 = getNewNFATransition(0);
                state->transition2->emptyTransition = true;

                struct NFAPart newPart = mergeNFAParts(p1, state);
                newPart.in = state;

                struct NFAStatePtrList newList;
                newList.length = 1;
                newList.list = malloc(sizeof(struct NFAState **));
                newList.list[0] = &state->transition2->dest;

                newPart.outList = newList;
                free(p1.outList.list);
                stack[stackPos++] = newPart;
                break;
            }
            default:
                break;
            }
        }
        else // Push char transition to stack
        {
            // Create transition
            struct NFATransition *trans = getNewNFATransition(character->character);

            // Create state
            struct NFAState *state = getNewNFAState();

            state->transition1 = trans;

            struct NFAPart part;
            part.in = state;
            // part.outList.list = stateToNFAPtrList(state->transition1->dest).list;
            part.outList.list = malloc(sizeof(struct NFAState **)); // Create list of size 1
            part.outList.list[0] = &state->transition1->dest;
            part.outList.length = 1;

            stack[stackPos++] = part;
        }
    }

    // Check if there is only one item on the stack
    if (stackPos != 1)
    {
        error = "Something went wrong while constructing NFA";
        return 0;
    }

    // Add in a final state
    struct NFAState *state = getNewNFAState();
    state->isFinal = true;
    struct NFAPart part = stack[--stackPos];
    struct NFAState *startState = mergeNFAParts(part, state).in;

    free(part.outList.list);

    return startState;
}

struct NFAState *getNFAStateFromId(int id)
{
    for (int i = 0; i < numNFAStates; i++)
        if (nfaStateList[i]->id == id)
            return nfaStateList[i];

    return 0;
}

// Print an NFA, given its first state

void printNFAHelper(struct NFAState *s)
{
    if (s->isFinal)
        printf("(|%i|)", s->id);
    else
        printf("( %i )", s->id);
}

void printNFA(struct NFAState *s)
{
    if (!s || s->isVisited)
        return;

    if (s->transition1)
    {
        printNFAHelper(s);
        char charact = '-';
        if (!s->transition1->emptyTransition)
            charact = s->transition1->character->character;
        printf("-%c->", charact);
        printNFAHelper(s->transition1->dest);
        printf("\n");
    }

    if (s->transition2)
    {
        printNFAHelper(s);
        char charact = '-';
        if (!s->transition2->emptyTransition)
            charact = s->transition2->character->character;
        printf("-%c->", charact);
        printNFAHelper(s->transition2->dest);
        printf("\n");
    }

    s->isVisited = true;

    if (s->transition1)
        printNFA(s->transition1->dest);
    if (s->transition2)
        printNFA(s->transition2->dest);
}
// Deletes all states found in the global NFA state list
void deleteNFA()
{
    for (int i = 0; i < numNFAStates; i++)
    {
        if (nfaStateList[i]->transition1)
        {
            free(nfaStateList[i]->transition1->character);
            free(nfaStateList[i]->transition1);
        }

        if (nfaStateList[i]->transition2)
        {
            free(nfaStateList[i]->transition2->character);
            free(nfaStateList[i]->transition2);
        }

        free(nfaStateList[i]);
    }

    free(nfaStateList);
    nfaStateList = 0;
    numNFAStates = 0;
}