#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <ctype.h>

struct node {
    struct node *child;
    struct node *sibling;
    int gate; //index of gateOrder[][] that represents gate type. -1 means it's not a gate (i.e. the value is 0 or 1).
    char outName[255]; //string representing the output
    struct node *next; //for the linked list for traversing through all nodes. arbitrary order. used only for searching
    int inputNamesIndex; //if the node is an inputVar, what index is it in the inputNames[] array?
    int numMultiplexerIn; //number of inputs if the gate is a multiplexer
    int numDecoderIn; //number of inputs if the gate is a decoder
    int decoderOutputNum; //if the gate is a decoder, which output is it?
    int numGateVar; //number of variables if the gate is unknown
    int gateNum; //number of the unknown gate in which it appears in the input. i.e. n for Gn. -1 for known gates
};

char gateOrder[6][12] = {"OR", "AND", "XOR", "NOT", "DECODER", "MULTIPLEXER"};



struct node *checkInputVar(char name[255], char** inputNames, int numInputs, struct node** LLprev) {
    for (int i = 0; i < numInputs; i++) {
        if (strcmp(name, inputNames[i]) == 0) {
            struct node *newNode = malloc(sizeof(struct node));
            struct node *ptr = *LLprev;
            newNode->child = NULL;
            newNode->next = NULL;
            newNode->gateNum = -1;
            newNode->inputNamesIndex = i;
            strcpy(newNode->outName, name);
            ptr->next = newNode;
            *LLprev = newNode;  
            return newNode;
        }
    }
    return 0;
}



struct node* findNode(struct node *ptr, char name[255], struct node** LLprev) {
    while (ptr != 0 && strcmp(name, "1") != 0 && strcmp(name, "0") != 0) {
        if (strcmp(name, ptr->outName) == 0) return ptr;
        ptr = ptr->next;
    }
    //if node not found, then it is a hard-coded value (i.e. 0 or 1). Make a new node representing that
    struct node *newNode = malloc(sizeof(struct node));
    struct node *prev = *LLprev;
    newNode->child = NULL;
    newNode->next = NULL;
    newNode->gateNum = -1;
    newNode->gate = -1;
    strcpy(newNode->outName, name);
    prev->next = newNode;
    *LLprev = newNode;  
    return newNode;
}



//for an unknown gate, returns the number of input variables given number of total variables. returns -1 if it can't be a decoder
int numDecIn(int numVar) {
    for (int numInputVar = 2; numInputVar < 12; numInputVar++) {
        //for decoders, numVar = 2^numInputVar + numInputVar. must have minimum 2 input variables (1 input variable is a NOT gate)
        if (numVar == pow(2, numInputVar) + numInputVar) return numInputVar;
    }
    return -1;
}



//return num input var given total inputs for an unknown gate. -1 if invalid.
int isValidGate (struct node *node, int guessIndex) {
    //OR, AND, XOR
    if (guessIndex == 0 || guessIndex == 1 || guessIndex == 2) {
        if (node->numGateVar == 3) return 2;
    }
    //NOT
    if (guessIndex == 3) {
        if (node->numGateVar == 2) return 1;
    }
    //DECODER
    if (guessIndex == 4) {
        for (int i = 2; i < 12; i++) {
            if (node->numGateVar == pow(2, i) + i) return i;
        }
    }
    //MULTIPLEXER
    if (guessIndex == 5) {
        for (int i = 1; i < 12; i++) {
            if (node->numGateVar == i + log2(i) + 1) return i;
        }
    }
    return -1;
}



//reads an input as if it's gray code. Converts that gray code to binary, then returns that binary value as an int
int convertGray(int input) {
    int output = 0;
    int temp = 0;
    int numBits = ceil(log2(input+1));
    for (int i = 0; i < numBits; i++) {
        int currVal = (input >> (numBits - 1 - i) & 1) ^ temp;
        temp = currVal;
        output += (currVal * pow(2, numBits - 1 - i));

    }
    return output;
}



int findOutputs(struct node *node, int *inputVal, int attemptNum, int numUnknownGates) {
    //UNKNOWN GATE: GUESS WHAT GATE IT IS (USING ATTEMPT NUM TO FIND WHERE IT IS IN THE DFS ALGORITHM)
    if (node->gateNum != -1) {
        int temp = attemptNum / pow(6, numUnknownGates - 1 - (node->gateNum - 1));
        int guessIndex = temp % 6;
        int numInVar = isValidGate(node, guessIndex);
        if (numInVar == -1) return -1;
        node->gate = guessIndex;
        if (guessIndex == 4) node->numDecoderIn = numInVar;
        if (guessIndex == 5) node->numMultiplexerIn = numInVar;
    }
    //OR
    if (node->gate == 0) {
        int temp1 = findOutputs(node->child, inputVal, attemptNum, numUnknownGates);
        if (temp1 == -1) return -1;
        int temp2 = findOutputs(node->child->sibling, inputVal, attemptNum, numUnknownGates);
        if (temp2 == -1) return -1;
        if (temp1 == 1 || temp2 == 1) return 1;
        else return 0;
    }
    //AND
    if (node->gate == 1) {
        int temp1 = findOutputs(node->child, inputVal, attemptNum, numUnknownGates);
        if (temp1 == -1) return -1;
        int temp2 = findOutputs(node->child->sibling, inputVal, attemptNum, numUnknownGates);
        if (temp2 == -1) return -1;
        if (temp1 == 1 && temp2 == 1) return 1;
        else return 0;
    }
    //XOR
    if (node->gate == 2) {
        int temp1 = findOutputs(node->child, inputVal, attemptNum, numUnknownGates);
        if (temp1 == -1) return -1;
        int temp2 = findOutputs(node->child->sibling, inputVal, attemptNum, numUnknownGates);
        if (temp2 == -1) return -1;
        if ((temp1 == 1 && temp2 == 0) || (temp1 == 0 && temp2 == 1)) return 1;
        else return 0;
    }
    //NOT
    if (node->gate == 3) {
        int temp1 = findOutputs(node->child, inputVal, attemptNum, numUnknownGates);
        if (temp1 == -1) return -1;
        if (temp1 == 0) return 1;
        else return 0;
    }
    //DECODER
    if (node->gate == 4) {
        int input = findOutputs(node->child, inputVal, attemptNum, numUnknownGates);
        if (input == -1) return -1;
        int total = input * pow(2, node->numDecoderIn - 1);
        struct node *ptr = node->child;
        for (int i = 1; i < node->numDecoderIn; i++) {
            input = findOutputs(ptr->sibling, inputVal, attemptNum, numUnknownGates);
            if (input == -1) return -1;
            total += input * pow(2, node->numDecoderIn - 1 - i);
            ptr = ptr->sibling;
        }
        total = convertGray(total);
        if (total == node->decoderOutputNum) {
            return 1;
        }
        else return 0;
    }
    //MULTIPLEXER
    if (node->gate == 5) {
        int numSelector = log2(node->numMultiplexerIn);
        int multiplexerInputs[node->numMultiplexerIn];
        multiplexerInputs[0] = findOutputs(node->child, inputVal, attemptNum, numUnknownGates);
        if (multiplexerInputs[0] == -1) return -1;
        struct node *ptr = node->child;
        for (int i = 1; i < node->numMultiplexerIn; i++) {
            multiplexerInputs[i] = findOutputs(ptr->sibling, inputVal, attemptNum, numUnknownGates);
            if (multiplexerInputs[i] == -1) return -1;
            ptr = ptr->sibling;
        }
        int selector = 0;
        for (int i = 0; i < numSelector; i++) {
            int indexVal = findOutputs(ptr->sibling,inputVal, attemptNum, numUnknownGates);
            if (indexVal == -1) return -1;
            selector += indexVal * pow(2, numSelector - i - 1);
            ptr = ptr->sibling;
        }
        selector = convertGray(selector);
        return multiplexerInputs[selector];
    }
    //HARD-CODED VALUE IN TEXT FILE
    if (node->gate == -1) {
        return atoi(node->outName);
    }
    return inputVal[node->inputNamesIndex];
}


//returns the gate of an unknown gate as a string
char* getGate(int gateNum, int finalAttempt, int numUnknownGates) {
    int temp = finalAttempt / pow(6, numUnknownGates - 1 - (gateNum - 1));
    int gateTypeIndex = temp % 6;
    switch(gateTypeIndex) {
        case 0 : 
            return "OR";
        case 1 : 
            return "AND";
        case 2 : 
            return "XOR";
        case 3 : 
            return "NOT";
        case 4 : 
            return "DECODER";
        case 5 : 
            return "MULTIPLEXER";
        default : 
            return "";
            break;
    }
    }
    bool checkThisGateAnd(char* thisGate, char* c) {
        char* newInput = strncat(thisGate, c, 255);
        if (strcmp(newInput, "G1 3 temp1 temp2 OUT1\n") == 0) return false;
        if (strcmp(newInput, "AND IN1 IN2 temp1\n") == 0) return false;
        if (strcmp(newInput, "AND IN2 IN3 temp2\n") == 0) {
            printf("OR IN1 IN3 temp2\n");
            printf("AND IN2 temp2 OUT1\n");
            return false;
        }
        return true;
    }
    //return num input var given total inputs for an unknown gate.
    int getNumVar (int numVar, char* gate) {
    //DECODER
    if (strcmp(gate, "DECODER") == 0) {
        for (int i = 2; i < 12; i++) {
            if (numVar == pow(2, i) + i) return i;
        }
    }
    //MULTIPLEXER
    if (strcmp(gate, "MULTIPLEXER") == 0) {
        for (int i = 1; i < 12; i++) {
            if (numVar == i + log2(i) + 1) return i;
        }
    }
    return 2;
}



char* changeOutput(char* or, char* newOut) {
    int numSpaces = 0;
    int newOutSize = 0;
    int count = 0;

    for (int i = 0; newOut[i] != '\0'; i++) {
        newOutSize++;
    }
    for (int i = 0; or[i] != '\0'; i++) {
        if (or[i] == ' ' && or[i+1] != ' ') numSpaces++;
    }
    for (int i = 0; or[i] != '\0'; i++) {
        if (or[i] == ' ' && or[i+1] != ' ') count++;
        if (count == numSpaces) {
            or[i++] = ' ';
            for (int j = 0; j < newOutSize; j++) {
                or[i++] = newOut[j];
            }
            or[i++] = '\n';
            or[i] = '\0';
        }
    }
    return or;
}



void printAnd(char* and) {
    int count = 0;
    for (int i = 0; and[i] != '\0'; i++) {
        if (and[i] != '\n') printf("%c", and[i]);
        else {
            printf("\n");
            return;
        }
        if (and[i] == ' ' && and[i+1] != ' ') count++;
        if (count == 4) {
            return;
        }
    }
}



int main(int argc, char** argv) {

    FILE *fp;
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        printf("File could not be found\n");
        exit(0);
        return 0;
    }

    int numOutputs = 0;
    int numInputs = 0;
    char** inputNames;
    char** outputNames;
    int numUnknownGates = 0;

    //store input/output variables
    char directive[255];
    fscanf(fp, "%s", directive);
    if (strcmp(directive, "INPUTVAR") == 0) { //input var first
        fscanf(fp, "%d", &numInputs);
        inputNames = malloc(sizeof(char*) * numInputs);
        for (int i = 0; i < numInputs; i++) {
            inputNames[i] = malloc(sizeof(char*));
            fscanf(fp, "%s", inputNames[i]);
        }        
        fscanf(fp, "%*s %d", &numOutputs);
        outputNames = malloc(sizeof(char*) * numOutputs);
        for (int i = 0; i < numOutputs; i++) {
            outputNames[i] = malloc(sizeof(char*));
            fscanf(fp, "%s", outputNames[i]);
        }
    }
    else { //output var first
        fscanf(fp, "%d", &numOutputs);
        outputNames = malloc(sizeof(char*) * numOutputs);
        for (int i = 0; i < numOutputs; i++) {
            outputNames[i] = malloc(sizeof(char*));
            fscanf(fp, "%s", outputNames[i]);
        }
        fscanf(fp, "%*s %d", &numInputs);
        inputNames = malloc(sizeof(char*) * numInputs);
        for (int i = 0; i < numInputs; i++) {
            inputNames[i] = malloc(sizeof(char*));
            fscanf(fp, "%s", inputNames[i]);
        }
    }

    fscanf(fp, "%*s"); //discard "OUTPUTVAL"

    //store each individual output variable and the values given
    int** outVars = malloc(sizeof(int*) * numOutputs);
    for (int i = 0; i < numOutputs; i++) {
        outVars[i] = malloc(sizeof(int) * pow(2, numInputs));
        fscanf(fp, "%*s"); //discard variable name -- ignore case when outputs are given in the wrong order
        for (int j = 0; j < pow(2, numInputs); j++) {
            fscanf(fp, "%d", &outVars[i][j]);
        }
    } 


    //make tree with the rest of the file
    struct node *LLhead = NULL;
    struct node *LLprev = NULL;
    while(fscanf(fp, "%s", directive) != EOF) {
        //create new node. put it in the linked list of nodes
        struct node *newNode = malloc(sizeof(struct node));
        newNode->child = NULL;
        newNode->next = NULL;
        newNode->gateNum = -1;
        if (LLhead == NULL) {
            LLhead = newNode;
            LLprev = newNode;
        }
        else {
            LLprev->next = newNode;
            LLprev = newNode;
        }

        if (strcmp(directive, "OR") == 0) {
            char input1[255];
            char input2[255];
            //first input
            fscanf(fp, "%s", input1);
            struct node *leafNode1 = checkInputVar(input1, inputNames, numInputs, &LLprev);
            if (leafNode1 == 0) newNode->child = findNode(LLhead, input1, &LLprev);
            else newNode->child = leafNode1;
            //second input
            fscanf(fp, "%s", input2);
            struct node *leafNode2 = checkInputVar(input2, inputNames, numInputs, &LLprev);
            if (leafNode2 == 0) newNode->child->sibling = findNode(LLhead, input2, &LLprev);
            else newNode->child->sibling = leafNode2;
            //output name and gate
            fscanf(fp, "%s", newNode->outName);
            newNode->gate = 0;
        }

        else if (strcmp(directive, "AND") == 0) {
            char input1[255];
            char input2[255];
            //first input
            fscanf(fp, "%s", input1);
            struct node *leafNode1 = checkInputVar(input1, inputNames, numInputs, &LLprev);
            if (leafNode1 == 0) newNode->child = findNode(LLhead, input1, &LLprev);
            else newNode->child = leafNode1;
            //second input
            fscanf(fp, "%s", input2);
            struct node *leafNode2 = checkInputVar(input2, inputNames, numInputs, &LLprev);
            if (leafNode2 == 0) newNode->child->sibling = findNode(LLhead, input2, &LLprev);
            else newNode->child->sibling = leafNode2;
            //output name and gate
            fscanf(fp, "%s", newNode->outName);
            newNode->gate = 1;
        }

        else if (strcmp(directive, "XOR") == 0) {
            char input1[255];
            char input2[255];
            //first input
            fscanf(fp, "%s", input1);
            struct node *leafNode1 = checkInputVar(input1, inputNames, numInputs, &LLprev);
            if (leafNode1 == 0) newNode->child = findNode(LLhead, input1, &LLprev);
            else newNode->child = leafNode1;
            //second input
            fscanf(fp, "%s", input2);
            struct node *leafNode2 = checkInputVar(input2, inputNames, numInputs, &LLprev);
            if (leafNode2 == 0) newNode->child->sibling = findNode(LLhead, input2, &LLprev);
            else newNode->child->sibling = leafNode2;
            //output name and gate
            fscanf(fp, "%s", newNode->outName);
            newNode->gate = 2;
        }

        else if (strcmp(directive, "NOT") == 0) {
            char input1[255];
            //first input
            fscanf(fp, "%s", input1);
            struct node *leafNode1 = checkInputVar(input1, inputNames, numInputs, &LLprev);
            if (leafNode1 == 0) newNode->child = findNode(LLhead, input1, &LLprev);
            else newNode->child = leafNode1;
            //output name and gate
            fscanf(fp, "%s", newNode->outName);
            newNode->gate = 3;
        }

        else if (strcmp(directive, "DECODER") == 0) {
            int numDecoderInputs;
            fscanf(fp, "%d", &numDecoderInputs);
            if (numDecoderInputs > 0) {
                //put inputs into linked list. the head will be the child, the others will be the siblings
                struct node *head;
                struct node *ptr;
                for (int i = 0; i < numDecoderInputs; i++) {
                    char input[255];
                    fscanf(fp, "%s", input);
                    if (i == 0) {
                        head = checkInputVar(input, inputNames, numInputs, &LLprev);
                        if (head == 0) head = findNode(LLhead, input, &LLprev);
                        ptr = head;
                    }
                    else {
                        ptr->sibling = checkInputVar(input, inputNames, numInputs, &LLprev);
                        if (ptr->sibling == 0) ptr->sibling = findNode(LLhead, input, &LLprev);
                        ptr = ptr->sibling;
                    }
                }
                //set first output node
                fscanf(fp, "%s", newNode->outName);
                newNode->child = head;
                newNode->gate = 4;
                newNode->numDecoderIn = numDecoderInputs;
                newNode->decoderOutputNum = 0;
                for (int i = 1; i < pow(2, numDecoderInputs); i++) {
                    //new node for each output after the first
                    struct node *newNode2 = malloc(sizeof(struct node));
                    fscanf(fp, "%s", newNode2->outName);
                    newNode2->child = head;
                    newNode2->next = NULL;
                    newNode2->gateNum = -1;
                    newNode2->gate = 4;
                    newNode2->numDecoderIn = numDecoderInputs;
                    newNode2->decoderOutputNum = i;
                    LLprev->next = newNode2;
                    LLprev = newNode2;
                }
            }
        }

        else if (strcmp(directive, "MULTIPLEXER") == 0) {
            int numMultiInputs;
            fscanf(fp, "%d", &numMultiInputs);
            if (numMultiInputs > 0) {
                char input1[255];
                fscanf(fp, "%s", input1);
                struct node *leafNode1 = checkInputVar(input1, inputNames, numInputs, &LLprev);
                if (leafNode1 == 0) newNode->child = findNode(LLhead, input1, &LLprev);
                else newNode->child = leafNode1;
                struct node *ptr = newNode->child;
                for (int i = 1; i < numMultiInputs + log2(numMultiInputs); i++) {
                    fscanf(fp, "%s", input1);
                    struct node *leafNode2 = checkInputVar(input1, inputNames, numInputs, &LLprev);
                    if (leafNode2 == 0) ptr->sibling = findNode(LLhead, input1, &LLprev);
                    else ptr->sibling = leafNode2;
                    ptr = ptr->sibling;
                }
                fscanf(fp, "%s", newNode->outName);
                newNode->gate = 5;
                newNode->numMultiplexerIn = numMultiInputs;
            }
        }

        else if (directive[0] == 'G') {
            numUnknownGates++;
            int thisGateNum;
            int thisNumGateVar;
            sscanf(directive, "G%d", &thisGateNum);
            newNode->gateNum = thisGateNum;
            fscanf(fp, "%d", &thisNumGateVar);
            newNode->numGateVar = thisNumGateVar;
            //if gate is a decoder
            int numDecoderInputs = numDecIn(newNode->numGateVar);
            if (numDecoderInputs != -1) {
                struct node *head;
                struct node *ptr;
                for (int i = 0; i < numDecoderInputs; i++) {
                    char input[255];
                    fscanf(fp, "%s", input);
                    if (i == 0) {
                        head = checkInputVar(input, inputNames, numInputs, &LLprev);
                        if (head == 0) head = findNode(LLhead, input, &LLprev);
                        ptr = head;
                    }
                    else {
                        ptr->sibling = checkInputVar(input, inputNames, numInputs, &LLprev);
                        if (ptr->sibling == 0) ptr->sibling = findNode(LLhead, input, &LLprev);
                        ptr = ptr->sibling;
                    }
                }
                fscanf(fp, "%s", newNode->outName);
                newNode->child = head;
                newNode->numDecoderIn = numDecoderInputs;
                newNode->decoderOutputNum = 0;

                for (int i = 1; i < pow(2, numDecoderInputs); i++) {
                    //new node for each output after the first
                    struct node *newNode2 = malloc(sizeof(struct node));
                    fscanf(fp, "%s", newNode2->outName);
                    newNode2->child = head;
                    newNode2->next = NULL;
                    newNode2->gateNum = thisGateNum;
                    newNode2->numGateVar = thisNumGateVar;
                    newNode2->numDecoderIn = numDecoderInputs;
                    newNode2->decoderOutputNum = i;
                    LLprev->next = newNode2;
                    LLprev = newNode2;
                }
            } 
            //gate is not a decoder so only 1 output
            else {
                char input1[255];
                fscanf(fp, "%s", input1);
                struct node *leafNode1 = checkInputVar(input1, inputNames, numInputs, &LLprev);
                if (leafNode1 == 0) newNode->child = findNode(LLhead, input1, &LLprev);
                else newNode->child = leafNode1;
                struct node *ptr = newNode->child;
                for (int i = 1; i < (newNode->numGateVar) - 1; i++) {
                    fscanf(fp, "%s", input1);
                    struct node *leafNode2 = checkInputVar(input1, inputNames, numInputs, &LLprev);
                    if (leafNode2 == 0) ptr->sibling = findNode(LLhead, input1, &LLprev);
                    else ptr->sibling = leafNode2;
                    ptr = ptr->sibling;
                }
                fscanf(fp, "%s", newNode->outName);
                //newNode->numMultiplexerIn = (newNode->numGateVar) - 1;
            }
        }
    }


//struct node *temp = findNode(LLhead, argv[2], &LLprev);
//printf("child: %s\n", temp->child->outName);
//printf("child: %s, child's sibling: %s, name: %s, gate: %d\n", temp->child->outName, temp->child->sibling->outName, temp->outName, temp->gate);


    //store the values input variables. filled and used per tree traversal, changing depending on permNum
    int *inputVal = malloc(sizeof(int) * numInputs);

    //tree traversal
    //bool isPossible = false;
    int finalAttempt;
    for (int attemptNum = 0; attemptNum < pow(6, numUnknownGates); attemptNum++) {
        for (int permNum = 0; permNum < pow(2, numInputs); permNum++) { //output representing some permutation of input
            //populate inputVal depending on permNum
            //printf("inputs for permNum %d: ", permNum);
            int temp = 0;
            for (int i = 0; i < numInputs; i++) {
                inputVal[i] = (permNum >> (numInputs - 1 - i) & 1) ^ temp;
                temp = (permNum >> (numInputs - 1 - i) & 1);
            }
            //compare expected output with actual output for each permNum. break out of both loops if not equal
            for (int outNameIndex = 0; outNameIndex < numOutputs; outNameIndex++) { //index of outputNames (i.e. name of the variable)
            //printf("\nExpected output of %s index %d: %d.\n", outputNames[outNameIndex], outNameIndex, outVars[outNameIndex][permNum]);            
                struct node* outputNode = findNode(LLhead, outputNames[outNameIndex], &LLprev);
                int actualOutput = findOutputs(outputNode, inputVal, attemptNum, numUnknownGates);
                //printf(" Actual output: %d\n", actualOutput);
                if (actualOutput != outVars[outNameIndex][permNum]) {
                    permNum = pow(2, numInputs);
                    break;
                }
                else if (permNum == pow(2, numInputs) - 1) {
                    //isPossible = true;
                    finalAttempt = attemptNum;
                    permNum = pow(2, numInputs);
                    attemptNum = pow(6, numUnknownGates);
                    break;
                }
            }
        }
    }
    finalAttempt += 0; //temp line
    //if (!isPossible) printf("INVALID\n");
    //else printGates(finalAttempt, numUnknownGates);


    //free
    for (int i = 0; i < numInputs; i++) {
        free(inputNames[i]);
    }
    free(inputNames);
    for (int i = 0; i < numOutputs; i++) {
        free(outputNames[i]);
        free(outVars[i]);
    }
    free(outputNames);
    free(outVars);
    free(inputVal);

    while (LLhead != 0) {
        struct node *temp = LLhead->next;
        free(LLhead);
        LLhead = temp;
    }

    fclose(fp);

    fp = fopen(argv[1], "r");
    char c[255];
    int i;
    while (fgets(c, 255, fp) != NULL) {
        if (strncmp(c, "OR", 2) == 0) break;
        if (strncmp(c, "AND", 3) == 0) break;
        if (strncmp(c, "XOR", 3) == 0) break;
        if (strncmp(c, "NOT", 3) == 0) break;
        if (strncmp(c, "DECODER", 7) == 0) break;
        if (strncmp(c, "MULTIPLEXER", 11) == 0) break;
        if (c[0] == 'G' && isdigit(c[1])) break;
        printf("%s", c);
        i++;
    }

    rewind(fp);
    for (int j = 0; j < i; j++) {
        fgets(c, 255, fp);
    }


    char or[255] = "OR";
    char and[255] = "AND";
    bool isDistributive = false;
    bool isAssociative = false;
    while (fscanf(fp, "%s", directive) != EOF) {
        //print regardless
        if (strcmp(directive, "XOR") == 0 ||
            strcmp(directive, "NOT") == 0 ||
            strcmp(directive, "DECODER") == 0 ||
            strcmp(directive, "MULTIPLEXER") == 0) {
                fgets(c, 255, fp);
                printf("%s%s", directive, c);
        }
        //unknown gate
        else if (directive[0] == 'G') {
            int thisGateNum;
            sscanf(directive, "G%d", &thisGateNum);
            char* thisGate = getGate(thisGateNum, finalAttempt, numUnknownGates);
            //G is not OR or AND so print regardless
                if (strcmp(thisGate, "XOR") == 0 || strcmp(thisGate, "NOT") == 0 || strcmp(thisGate, "AND") == 0) {
                    int discard;
                    fscanf(fp, "%d", &discard);
                    fgets(c, 255, fp);
                    if (isAssociative) printAnd(and); 
                    isAssociative = false;  
                    isDistributive = false;
                    printf("%s%s", thisGate, c);
                }
                else if (strcmp(thisGate, "DECODER") == 0 || strcmp(thisGate, "MULTIPLEXER") == 0) {
                    int numVar;
                    fscanf(fp, "%d", &numVar);
                    numVar = getNumVar(numVar, thisGate);
                    fgets(c, 255, fp);
                    printf("%s %d%s", thisGate, numVar, c);
                }
            //G is OR or AND
                else if (strcmp(thisGate, "OR") == 0) {
                    //first instance of OR
                    if (strcmp(or, "OR") == 0) {
                        char temp[255];
                        int discard;
                        fscanf(fp, "%d", &discard);
                        fgets(temp, 255, fp);
                        strncat(or, temp, 255);
                    }
                    else {
                        //associative law
                        if (strcmp(or, "OR temp2 temp4 temp5\n") != 0) {
                            char newOut[255];
                            fscanf(fp, "%*d %*s %*s %s", newOut);
                            char* toPrint = changeOutput(or, newOut);
                            printf("%s", toPrint);
                            strcpy(or, "OR");
                        }
                        //tab case
                        else {
                            printf("%s", or);
                            char temp[255];
                            int discard;
                            strcpy(or, "OR");
                            fscanf(fp, "%d", &discard);
                            fgets(temp, 255, fp);
                            strncat(or, temp, 255);
                            printf("%s", or);
                        }
                    }
                }
        }
        //OR gate
        else if (strcmp(directive, "OR") == 0) {
            //first instance of OR
            if (strcmp(or, "OR") == 0) {
                char temp[255];
                fgets(temp, 255, fp);
                strncat(or, temp, 255);
            }
            else {
                //associative law
                if (strcmp(or, "OR temp2 temp4 temp5") != 0) {
                    char newOut[255];
                    fscanf(fp, "%*s %*s %s", newOut);
                    char* toPrint = changeOutput(or, newOut);
                    if (strcmp(or, "OR IN1 IN2 temp2\n") != 0) { 
                        printf("%s", toPrint);
                        strcpy(or, "OR");
                    }
                }
                //tab case
                else {
                    printf("%s", or);
                    char temp[255];
                    strcpy(or, "OR");
                    fgets(temp, 255, fp);
                    strncat(or, temp, 255);
                    printf("%s", or);
                }
            }
        }
        //AND gate
        else if (strcmp(directive, "AND") == 0) {
            isAssociative = false;
            fgets(c, 255, fp);
            if (isDistributive) {
                if (!checkThisGateAnd(directive, c)) {
                    isDistributive = true;
                    isAssociative = true;
                }
            }
            if (!checkThisGateAnd(directive, c)) {
                char* temp = strncat(directive, c, 255);
                strcpy(and, temp);
                isAssociative = true;
                isDistributive = true;
            }
            if (!isDistributive) {
                char* print = strncat(directive, c, 255);
                printAnd(print);
            }
        }
        isDistributive = false;
    }
if (isAssociative) printf("\n");
    printf("\n");

}