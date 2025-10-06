
#ifndef CRIMSON_H
#define CRIMSON_H


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "Tundora.h"

typedef enum {
    IDENTIFIER=0,
    NUMBER=1,
    OPERATOR=2,
    LEFT_PAREN=3,
    RIGHT_PAREN=4,
    EQUALS=5,
    NONE=7
} Node_Type;
typedef enum  {
    OP_ADD=0,
    OP_MINUS=1,
    OP_MULT=2,
    OP_DIV=3,
    OP_EXP=4,
    OP_LOG=5,
    OP_EQ=6,
    OP_NULL=7,
    OP_ABS=8
} OperatorType;
typedef enum  {
    ARITHMETIC=0, // +
    LINEAR=1, // /
    QUADRATIC=2, // Q
    ABS=3, // |x|
    SYSTEMS=4, // X
} Equation_Type;
typedef struct ASTNode {

    Node_Type Type;
    union {
        char identifier;
        double number;
        OperatorType operator; // it says "expected an operator" here
    } value;
    struct ASTNode* Left;
    struct ASTNode* Right;
    bool pool_allocated;
} ASTNode;
string EvaluateEquation(string S, Equation_Type T);
void FreeASTNode(ASTNode* a);
void pool_reset();
ASTNode* Number_Node(double num);
ASTNode* MakeVariableNode(const char* symbol, double coeff);
ASTNode* NewOperatorNode(OperatorType op);
void PrintAST(ASTNode* A, int depth);
ASTNode* PopulatedOperatorNode(OperatorType OT, ASTNode* Left, ASTNode* Right);
Node_Type GetCharacterToken(char t);
ASTNode* Parse_EQ(List* Tokens);
void PrintLst(void* V);
ASTNode* Parse_ASA(List* Tokens, int StartIndex, int* NextIndex);
ASTNode* Parse_MD(List* Tokens, int StartIndex, int* NextIndex);
ASTNode* Parse_EMISC(List* Tokens, int StartIndex, int* NextIndex);
ASTNode* Parse_ATOMS(List* Tokens, int StartIndex, int* NextIndex);
ASTNode* Flatten(ASTNode* S);
#endif