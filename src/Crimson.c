

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <debug.h>
#include "Tundora.h"
#include "Crimson.h"
#define MAX_SIZE 64
#define ASTPOOL_SIZE 512
#define PRECISION 4
double R(double d) {
    return round(pow(10,PRECISION)*d)/pow(10,PRECISION);
}
static ASTNode ast_pool[ASTPOOL_SIZE];
int ast_pool_top = 0;

void pool_reset(void) {
    ast_pool_top = 0;
}
ASTNode* pool_alloc_node(void) {
    if (ast_pool_top < ASTPOOL_SIZE) {
        ASTNode* n = &ast_pool[ast_pool_top++];
        n->pool_allocated = true;
        return n;
    }
    ASTNode* n = malloc(sizeof(ASTNode));
    n->pool_allocated = false; 
    return n;
}
void FreeASTNode(ASTNode* A) {
    return;
    if (!A) return;
    FreeASTNode(A->Left);
    FreeASTNode(A->Right);
    if (A->Type == IDENTIFIER && A->value.identifier)
        free(A->value.identifier);

    if (!A->pool_allocated) {
        free(A);
    }

    A->Left = A->Right = NULL;
}
ASTNode* DistributeOverTree(ASTNode* node, double Factor) {
    if (node == NULL) return NULL;

    if (node->Type == NUMBER) {
        ASTNode* negNum = pool_alloc_node();
        *negNum = *node;
        negNum->value.number = Factor*(node->value.number);
        negNum->Left = NULL;
        negNum->Right = NULL;
        return negNum;
    }

    if (node->Type == IDENTIFIER) {
        ASTNode* mult = pool_alloc_node();
        mult->Type = OPERATOR;
        mult->value.operator = OP_MULT;

        ASTNode* negOne = pool_alloc_node();
        negOne->Type = NUMBER;
        negOne->value.number = Factor;
        negOne->Left = negOne->Right = NULL;

        mult->Left = negOne;
        mult->Right = node;
        return mult;
    }

    if (node->Type == OPERATOR) {
        switch (node->value.operator) {
            case OP_ADD: {
                ASTNode* newNode = pool_alloc_node();
                newNode->Type = OPERATOR;
                newNode->value.operator = OP_ADD;
                newNode->Left = DistributeOverTree(node->Left,Factor);
                newNode->Right = DistributeOverTree(node->Right,Factor);
                return newNode;
            }
            case OP_MULT: {
                ASTNode* newNode = pool_alloc_node();
                *newNode = *node;
                newNode->Left = DistributeOverTree(node->Left,Factor);
                newNode->Right = node->Right;
                return newNode;
            }
            default: {
                ASTNode* mult = pool_alloc_node();
                mult->Type = OPERATOR;
                mult->value.operator = OP_MULT;

                ASTNode* negOne = pool_alloc_node();
                negOne->Type = NUMBER;
                negOne->value.number = Factor;
                negOne->Left = negOne->Right = NULL;

                mult->Left = negOne;
                mult->Right = node;
                return mult;
            }
        }
    }

    return NULL; // should never happen
}
ASTNode* AutoDist(ASTNode* node) {
    if (!node || node->Type != OPERATOR) return node;

    node->Left = AutoDist(node->Left);
    node->Right = AutoDist(node->Right);

    if (node->value.operator == OP_MULT) {
        if (node->Left && node->Left->Type == OPERATOR && node->Left->value.operator == OP_ADD) {
            // (a + b) * c  →  (a*c + b*c)
            ASTNode* a = node->Left->Left;
            ASTNode* b = node->Left->Right;
            ASTNode* c = node->Right;

            ASTNode* ac = NewOperatorNode(OP_MULT);
            ac->Left = a;
            ac->Right = c;

            ASTNode* bc = NewOperatorNode(OP_MULT);
            bc->Left = b;
            bc->Right = c;

            ASTNode* sum = NewOperatorNode(OP_ADD);
            sum->Left = ac;
            sum->Right = bc;

            return AutoDist(sum);
        }
        else if (node->Right && node->Right->Type == OPERATOR && node->Right->value.operator == OP_ADD) {
            // a * (b + c) → (a*b + a*c)
            ASTNode* a = node->Left;
            ASTNode* b = node->Right->Left;
            ASTNode* c = node->Right->Right;

            ASTNode* ab = NewOperatorNode(OP_MULT);
            ab->Left = a;
            ab->Right = b;

            ASTNode* ac = NewOperatorNode(OP_MULT);
            ac->Left = a;
            ac->Right = c;

            ASTNode* sum = NewOperatorNode(OP_ADD);
            sum->Left = ab;
            sum->Right = ac;

            return AutoDist(sum);
        }
    }
    return node;
}
List SplitByChar(string s, char c) {
    List NS = NewList();
    bool HasPassed = false;
    string first = NewStr("");
    string second = NewStr("");
    for (int i = 0; i < s.length; i++) {
        if (HasPassed && s.Content[i] != ' ') {
            ConcatStrings(&second, (string){&s.Content[i],1});
        }
        else {
            if (s.Content[i] == c) {
                HasPassed = true;
            }
            else if (s.Content[i] != ' ') {
                ConcatStrings(&first, (string){&s.Content[i],1});
            }
        }
    }
    List_AppendElement(&NS,&first);
    List_AppendElement(&NS,&second);
    return NS;
}
bool Inclusive_Range(int x, int min, int max) {
    return (min <= x) && (x <= max);
}
char NUM[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
#define OPLENGTH 7
char OP[OPLENGTH] = {'+', '-', '*', '/', '^','=', '|'};
char LP = '(';
char RP = ')';
bool ArrayContains(char* t, int length, char l) {
    for (int i = 0; i < length; i++) {
        char R = *(t+i);
        if (R==l) {
            return true;
        }
    }
    return false;
}
Node_Type GetCharacterToken(char t) {
    
    if (Inclusive_Range(t,48,57)) {
        return NUMBER;
    }
    else if (ArrayContains(&OP[0],OPLENGTH,t)) {
        return OPERATOR;
    }
    else if (t == LP) {
        return LEFT_PAREN;
    }
    else if (t == RP) {
        return RIGHT_PAREN;
    }
    else if (t != '.' && t != '\0') {
        return IDENTIFIER;
    }
    return NONE;
}
int strcmp2(string A, string B) {
    if (A.length != B.length) {
        return 0;
    }
    for (int i = 0; i < A.length; i++) {
        if (A.Content[i] != B.Content[i]) {
            return 0;
        }
    }
    return 1;
}
OperatorType OperatorFromString(string S) {
    if (strcmp2(S, (string){"+", 2} ) ) {
        return OP_ADD;
    }
    else if (strcmp2(S, (string){"-", 2} ) ) {
        return OP_MINUS;
    }
    else if (strcmp2(S, (string){"*", 2} ) ) {
        return OP_MULT;
    }
    else if (strcmp2(S, (string){"/", 2} ) ) {
        return OP_DIV;
    }
    else if (strcmp2(S, (string){"^", 2} ) ) {
        return OP_EXP;
    }
    else if (strcmp2(S, (string){"log", 4} ) ) {
        return OP_LOG;
    }
    else if (strcmp2(S, (string){"=",2})) {
        return OP_EQ;
    }
    else if (strcmp2(S, (string){"|",2}) || strcmp2(S, (string){"abs", 4})) {
        return OP_ABS;
    }
    return OP_NULL;
}
string DoubleToString(double value) {
    
    int len = snprintf(NULL, 0, "%g", value);  
    if (len < 0) {
        string empty = {NULL, 0};
        return empty;
    }

    string result;
    result.length = len;
    result.Content = malloc(len + 1); 

    if (result.Content == NULL) {
        string empty = {NULL, 0};
        return empty;
    }

    snprintf(result.Content, len + 1, "%g", value);

    return result;
}
int DigitCharToInt(char a) {
    if (Inclusive_Range(a, 48, 57)) {
        return a-48;
    }
    return 0;
}
double StringToDouble(string s) {
    double result = 0.0;
    double fraction = 0.0;
    double divisor = 1.0;
    bool pastDecimal = false;

    for (int i = 0; i < s.length; i++) {
        char c = s.Content[i];

        if (c == '.') {
            if (pastDecimal) {
                return result;
            }
            pastDecimal = true;
            continue;
        }

        if (c < '0' || c > '9') {
            return result;
        }

        int digit = c - '0';

        if (!pastDecimal) {
            result = result * 10.0 + digit;
        } else {
            divisor *= 10.0;
            fraction += digit / divisor;
        }
    }

    return result + fraction;
}
char Make_Lower(char s) {
    if (Inclusive_Range(s, 'A', 'Z')) {
        return (s ^ 0b00100000);
    }
    return s;
}

enum Layers {
    EQ=0,
    AS=1,
    MD=2,
    EMISC=3,
    ATOMS=4
};
struct SplitResult {
    List* First;
    List* Second;
};
struct SplitResult Split_List(List* T, int Index) {
    List* A = malloc(sizeof(List));
    *A = NewList();
    List* B = malloc(sizeof(List));
    *B = NewList();
    for (int i = 0; i < T->length; i++) {
        if (i < Index) {
            List_AppendElement(A, List_GetElement(T, i)->data);
        }
        else if (i > Index)
        {
            List_AppendElement(B, List_GetElement(T,i)->data);
        }
        
    }
    struct SplitResult *Result = malloc(sizeof(struct SplitResult));
    *Result = (struct SplitResult){A, B};
    return *Result;
}
int FindRightParen(List Tokens, int StartIndex) {
    int Depth = -1;
    for (int i = StartIndex; i < Tokens.length;i++) {
        ASTNode* Token = (ASTNode*)List_GetElement(&Tokens, i)->data;
        if (Token->Type == RIGHT_PAREN && Depth == 0) {
            return i;
        }
        else if (Token->Type == RIGHT_PAREN)
        {
            Depth--;
        }  
        if (Token->Type == LEFT_PAREN) {
            Depth++;
        }
        
    }
   dbg_printf("ERROR");
    return 0;
}
List MakeSublist(List Tokens, int Start, int End) {
    if (Start > Tokens.length || End > Tokens.length) {
       dbg_printf("Error");
    }
    List* ToReturn = malloc(sizeof(List));
    *ToReturn = NewList();
    for (int i = Start; i <= End; i++) {
        List_AppendElement(ToReturn,List_GetElement(&Tokens, i)->data);
    }
    return *ToReturn;
}
ASTNode* Copy_Token(ASTNode* A) {
    
    ASTNode* newNode = pool_alloc_node();
    *newNode = *A; 
    newNode->Left = A->Left;
    newNode->Right = A->Right;
    return newNode;
}
ASTNode* Parse_ATOMS(List* Tokens, int StartIndex, int* NextIndex) {
    int idx = StartIndex;

    if (idx >= Tokens->length) {
        *NextIndex = idx;
        return NULL;
    }

    ASTNode* Token = (ASTNode*)List_GetElement(Tokens, idx)->data;

    
    if (Token->Type == NUMBER || Token->Type == IDENTIFIER) {
        ASTNode* Node = pool_alloc_node();
        *Node = *Token; 
        Node->Left = NULL;
        Node->Right = NULL;
        *NextIndex = idx + 1;
        return Node;
    }

    
    if (Token->Type == LEFT_PAREN) {
        int EndIndex = FindRightParen(*Tokens, idx);
        if (EndIndex <= idx) {
           dbg_printf("ERROR: unmatched parenthesis\n");
            *NextIndex = idx;
            return NULL;
        }

        
        List Sublist = MakeSublist(*Tokens, idx + 1, EndIndex - 1);

        int SubNext = 0;
        ASTNode* Node = Parse_ASA(&Sublist, 0, &SubNext);

        *NextIndex = EndIndex + 1; 
        return Node;
    }

    
    *NextIndex = idx;
    return NULL;
}

ASTNode* Parse_EMISC(List* Tokens, int StartIndex, int* NextIndex) {
    int idx = StartIndex;
    ASTNode* Node = pool_alloc_node();
    if (idx >= Tokens->length) {
       dbg_printf("idx is bad");
        return NULL;
    }
    ASTNode* Token = (ASTNode*)List_GetElement(Tokens, idx)->data;
    if (Token->Type == OPERATOR && (Token->value.operator == OP_MINUS || Token->value.operator == OP_ADD || Token->value.operator == OP_ABS)) {
        ASTNode* UnaryOp = pool_alloc_node();
        UnaryOp->Type = OPERATOR;
        UnaryOp->value.operator = Token->value.operator;
        UnaryOp->Left = NULL;
        idx++;
        UnaryOp->Right = Parse_ATOMS(Tokens, idx, &idx);

        *NextIndex = idx;
        return UnaryOp;
    }

    Node = Parse_ATOMS(Tokens, idx, &idx);

    while (idx < Tokens->length) {
        ASTNode* NextToken = (ASTNode*)List_GetElement(Tokens, idx)->data;
        if (NextToken->Type != OPERATOR || (NextToken->value.operator != OP_LOG && NextToken->value.operator != OP_EXP)) {
            break;
        }

        ASTNode* ExpNode = pool_alloc_node();
        ExpNode->Type = OPERATOR;
        ExpNode->value.operator = NextToken->value.operator;
        ExpNode->Left = Node;

        idx++; 
        ExpNode->Right = Parse_ATOMS(Tokens, idx, &idx);

        Node = ExpNode; 
    }

    *NextIndex = idx;
    return Node;
}
ASTNode* Parse_MD(List* Tokens, int StartIndex, int* NextIndex) {
    int idx = StartIndex;
    ASTNode* Left = Parse_EMISC(Tokens, idx, &idx);

    while (idx < Tokens->length) {
        ASTNode* Token = (ASTNode*)List_GetElement(Tokens, idx)->data;
        if (Token->Type != OPERATOR || (Token->value.operator != OP_MULT && Token->value.operator != OP_DIV)) {
            break;
        }

        ASTNode* OpNode = pool_alloc_node();
        OpNode->Type = OPERATOR;
        OpNode->value.operator = Token->value.operator;
        OpNode->Left = Left;

        idx++; 
        OpNode->Right = Parse_EMISC(Tokens, idx, &idx);

        Left = OpNode; 
    }

    *NextIndex = idx;
    return Left;
}
ASTNode* Parse_ASA(List* Tokens, int StartIndex, int* NextIndex) {
    int idx = StartIndex;
    ASTNode* Left = Parse_MD(Tokens, idx, &idx);
    while (idx < Tokens->length) {
        ASTNode* Token = (ASTNode*)List_GetElement(Tokens, idx)->data;
        if (Token->Type != OPERATOR || (Token->value.operator != OP_ADD && Token->value.operator != OP_MINUS)) {
            break;
        }

        ASTNode* OpNode = pool_alloc_node();
        OpNode->Type = OPERATOR;
        OpNode->value.operator = Token->value.operator;
        OpNode->Left = Left;

        idx++; 
        OpNode->Right = Parse_MD(Tokens, idx, &idx);

        Left = OpNode; 
    }

    *NextIndex = idx;
    return Left;
}
ASTNode* Parse_EQ(List* Tokens) {
    for (int i = 0; i < Tokens->length; i++) {
        ASTNode* Current = (ASTNode*)List_GetElement(Tokens, i)->data;
        if (Current->Type == OPERATOR) {
            if (Current->value.operator == OP_EQ) {
                ASTNode* Root = pool_alloc_node();
                Root->Type = OPERATOR;
                Root->value.operator = OP_EQ;
                struct SplitResult SR = Split_List(Tokens, i);
                int NextIndexL = 0;
                int NextIndexR = 0;
                Root->Left = Parse_ASA(SR.First,0,&NextIndexL);
                Root->Right = Parse_ASA(SR.Second,0,&NextIndexR);

                return Root;
            }
        }
    
    }
    int L = 0;
    return Parse_ASA(Tokens,0,&L);
}
double d_abs(double b) {
    if (b >= 0) {
        return b;
    }
    return b*-1;
}
double DoOperation(double left, double right,OperatorType OT) {
    switch (OT) {
            case OP_ADD:
                return left+right;
            case OP_MINUS:
                return left-right;
            case OP_MULT:
                return left*right;
            case OP_DIV:
                return left/right;
            case OP_EXP:
                return pow(left, right);
            case OP_LOG:
                return log(right)/log(left);
            case OP_ABS:
                return d_abs(right);
        }
    return 0;
}
double EvaluateArithmetic(ASTNode* AN) {
    double left = 0;
    double right = 0;
    if (!AN) {
       dbg_printf("NULL");
        return 0;
    }
    if (AN->Type == NUMBER) {
        return AN->value.number;
    }
    else if (AN->Type == OPERATOR)
    {
        left = AN->Left ? EvaluateArithmetic(AN->Left) : 0;
        right = AN->Right ? EvaluateArithmetic(AN->Right) : 0;
        return DoOperation(left,right,AN->value.operator);
    }
    return 0;
}
typedef struct {
    char identifier;
    double value;
} Variable;
double EvaluateSubstitution(ASTNode* AN, Variable* vars, int Amt, bool Any) {
    double left;
    double right;
    switch (AN->Type) {
        case NUMBER:
            return AN->value.number;
        case IDENTIFIER:
        if (!Any) {
            for (int i = 0; i < Amt; i++) {
                if (vars[i].identifier == AN->value.identifier) {
                    return vars[i].value;
                }
            }
        }
        else {
            return vars[0].value;
        }
           dbg_printf("Variable not found error");
        case OPERATOR: {
            if (AN->Left) {
                left = EvaluateSubstitution(AN->Left, vars, Amt, Any);
            }
            if (AN->Right) {
                right = EvaluateSubstitution(AN->Right, vars, Amt, Any);
            }
            return DoOperation(left, right, AN->value.operator);
        }
    }
    return 0;


}
ASTNode* TreeSubstitution(ASTNode* AN, Variable* vars, int Amt) {
    if (!AN) return NULL;

    switch (AN->Type) {
        case NUMBER:
            return AN;

        case IDENTIFIER:
            for (int i = 0; i < Amt; i++) {
                if (vars[i].identifier == AN->value.identifier) {
                    return Number_Node(vars[i].value);
                }
            }
            return AN;

        case OPERATOR: {
            ASTNode* left = TreeSubstitution(AN->Left, vars, Amt);
            ASTNode* right = TreeSubstitution(AN->Right, vars, Amt);

            if (left && right && left->Type == NUMBER && right->Type == NUMBER) {
                return Number_Node(DoOperation(left->value.number, right->value.number, AN->value.operator));
            }

            ASTNode* newNode = NewOperatorNode(AN->value.operator);
            newNode->Left = left;
            newNode->Right = right;
            return newNode;
        }
    }

    return AN;
}
OperatorType GetOppositeOperation(OperatorType OT) {
    switch (OT) {
        case OP_ADD:
            return OP_MINUS;
        case OP_MINUS:
            return OP_ADD;
        case OP_MULT:
            return OP_DIV;
        case OP_DIV:
            return OP_MULT;
    }
    return 7;
}
typedef enum  {
    LEFT=0,
    RIGHT=1,
} Side;
typedef struct {
    ASTNode* Node;
    Side S;
} SidedASTNode;
ASTNode* GetSide(ASTNode* A, Side S) {
    switch (S) {
        case LEFT:
            return A->Left;
        case RIGHT:
            return A->Right;
    }
   dbg_printf("ERROR");
    return NULL;
}
bool IsIdentity(ASTNode* S, double Identity) {
    if (S == NULL) return false;
    if (S->Type == NUMBER) {
        if (S->value.number == Identity) {
            return true;
        }
    }
    return false;
}
ASTNode* Number_Node(double num) {
    ASTNode* ToReturn = pool_alloc_node();
    ToReturn->Type = NUMBER;
    ToReturn->value.number = num;
    ToReturn->Left = NULL;
    ToReturn->Right = NULL;
    return ToReturn;
}
void Collect(ASTNode* Node, OperatorType op, List* Operands) {
    if (Node == NULL) return;

    // Treat absolute value as atomic — don't flatten inside it
    if (Node->Type == OPERATOR && Node->value.operator == OP_ABS) {
        List_AppendElement(Operands, Node);
        return;
    }

    // If node matches the operator, recurse into both sides (flatten)
    if (Node->Type == OPERATOR && Node->value.operator == op) {
        if (Node->Left)  Collect(Node->Left,  op, Operands);
        if (Node->Right) Collect(Node->Right, op, Operands);
        return;
    }

    // If it’s a unary operator (only right child), flatten just that
    if (Node->Type == OPERATOR && Node->Left == NULL) {
        Collect(Node->Right, op, Operands);
        return;
    }

    // Otherwise, this node is a leaf or a different operator — keep it as is
    List_AppendElement(Operands, Node);
}
ASTNode* NewOperatorNode(OperatorType op) {
    ASTNode* N = pool_alloc_node();
    N->Type = OPERATOR;
    N->value.operator = op;
    N->Left = NULL;
    N->Right = NULL;
    return N;
}

ASTNode* RebuildTree(OperatorType op, List Operands) {
    if (Operands.length == 0) return NULL;
    if (Operands.length == 1) {
        return (ASTNode*)List_GetElement(&Operands, 0)->data;  // Get the actual ASTNode*
    }

    int mid = Operands.length / 2;

    // Split the list safely
    List LeftList = NewList();
    List RightList = NewList();

    for (int i = 0; i < Operands.length; i++) {
        ASTNode* elem = (ASTNode*)List_GetElement(&Operands, i)->data;
        if (i < mid) List_AppendElement(&LeftList, elem);
        else List_AppendElement(&RightList, elem);
    }

    ASTNode* Left = RebuildTree(op, LeftList);
    ASTNode* Right = RebuildTree(op, RightList);

    ASTNode* Node = NewOperatorNode(op);
    Node->Left = Left;
    Node->Right = Right;
    
    return Node;
}
ASTNode* Flatten(ASTNode* S) {
    if (S == NULL) {
        return NULL;
    }
    if (S->Type != OPERATOR) {
        return S;
    }
    if (S->value.operator == OP_ABS) {
        ASTNode* node = NewOperatorNode(OP_ABS);
        node->Left = NULL;
        node->Right = Flatten(S->Right);
        return node;
    }

    if ( (S->value.operator == OP_ADD || S->value.operator == OP_MULT) && S->Type == OPERATOR ) {
        OperatorType op = S->value.operator;
        
        List Operands = NewList();

        Collect(S->Left, op, &Operands);
        Collect(S->Right, op, &Operands);

        return RebuildTree(op, Operands);
    
    }
    else {
        ASTNode* ToReturn = NewOperatorNode(S->value.operator);
        ToReturn->Left = Flatten(S->Left);
        ToReturn->Right = Flatten(S->Right);
        return ToReturn;
    }
}
typedef struct {
    char Identifier;
    double Power;
} Var;
// string hash (djb2)
int HashFunction(void* vp) {
    const char* s = (const char*)vp;
    unsigned long h = 5381;
    while (*s) {
        h = ((h << 5) + h) + (unsigned char)(*s);
        s++;
    }
    return (int)(h & 0x7FFFFFFF);
}

char* GetVariableKey(ASTNode* S) {
    
    
    
    if (S == NULL) return NULL;
    if (S->Type == NUMBER) return NULL;
    if (S->Type == IDENTIFIER) {
        char* k = malloc(2);
        k[0] = S->value.identifier;
        k[1] = '\0';
        return k;
    }
    else if (S->Type == OPERATOR)
    {   
        OperatorType O = S->value.operator;
        if (O == OP_MULT) {
            char* L = GetVariableKey(S->Left);
            char* R = GetVariableKey(S->Right);

            if (L == NULL && R == NULL) {
                return NULL;
            }
            if (L == NULL) return R ? strdup(R) : NULL;
            if (R == NULL) return L ? strdup(L) : NULL;
            size_t lenL = strlen(L);
            size_t lenR = strlen(R);
            char* out = malloc(lenL + lenR + 1);
            memcpy(out, L, lenL);
            memcpy(out + lenL, R, lenR);
            out[lenL + lenR] = '\0';
            free(L);
            free(R);
            return out;
        }
        if (O == OP_EXP) {
            if (S->Left && S->Left->Type == IDENTIFIER && S->Right && S->Right->Type == NUMBER) {

                char buf[32];
                snprintf(buf, sizeof(buf), "%c^%.4f", S->Left->value.identifier, S->Right->value.number);
                char* out = malloc(strlen(buf)+1);
                strcpy(out, buf);
                return out;
            } 
            return NULL;
        }
    }
    return NULL;
}
double GetCoeff(ASTNode* S) {
    if (!S) return 0;

    if (S->Type == NUMBER) {
        return S->value.number;
    }
    if (S->Type == IDENTIFIER) {
        return 1;
    }
    if (S->value.operator != OP_MULT) {
        return 1;
    }
    if (S->Left->Type == NUMBER && S->Right->Type != NUMBER) {
        return S->Left->value.number;
    }
    if (S->Right->Type == NUMBER && S->Left->Type != NUMBER) {
        return S->Right->value.number;
    }
    return 0;
    /*if (S == NULL) {
        return 0;
    }
    if (S->Type == NUMBER) {
        return S->value.number;
    }
    if (S->Type == IDENTIFIER) {
        return 1;
    }
    if (S->Type == OPERATOR) {
        if (S->value.operator == OP_MULT || S->value.operator == OP_DIV) {
            double a = GetCoeff(S->Left);
            double b = GetCoeff(S->Right);
            if (S->value.operator == OP_DIV && b == 0) {
                return NAN;
            }
            if (S->value.operator == OP_MULT || ( S->Left && S->Left->Type == NUMBER && S->Right && S->Right->Type == NUMBER)) {
            return DoOperation(a,b,S->value.operator);
            }
            return 1;
        }
    }
    return 1;*/
}


ASTNode* MakeVariableNode(const char* symbol, double coeff) {
    ASTNode* varPart = malloc(sizeof(ASTNode));
    if (strcmp("", symbol) == 0) {
        return Number_Node(coeff);
    }
    // Case 1: single identifier, e.g. "x"
    if (strlen(symbol) == 1) {
        varPart = malloc(sizeof(ASTNode));
        varPart->Type = IDENTIFIER;
        varPart->value.identifier = symbol[0];
        varPart->Left = NULL;
        varPart->Right = NULL;
    }
    // Case 2: exponent, e.g. "x^2"
    else if (strchr(symbol, '^')) {
        char base = symbol[0];
        double exp = atof(symbol + 2);  // assumes form "x^<num>"

        ASTNode* baseNode = malloc(sizeof(ASTNode));
        baseNode->Type = IDENTIFIER;
        baseNode->value.identifier = base;
        baseNode->Left = NULL;
        baseNode->Right = NULL;

        ASTNode* expNode = Number_Node(exp);

        varPart = NewOperatorNode(OP_EXP);
        varPart->Left = baseNode;
        varPart->Right = expNode;
    }
    // Case 3: product like "xy"
    else {
        size_t len = strlen(symbol);
        if (len > 1) {
            // recursively multiply all characters

            varPart = NULL;
            for (size_t i = 0; i < len; i++) {
                ASTNode* id = malloc(sizeof(ASTNode));
                id->Type = IDENTIFIER;
                id->value.identifier = symbol[i];
                id->Left = NULL;
                id->Right = NULL;

                if (varPart == NULL) {
                    varPart = id;
                } else {
                    ASTNode* mult = NewOperatorNode(OP_MULT);
                    mult->Left = varPart;
                    mult->Right = id;
                    varPart = mult;
                }
            }
        }
    }

    // Now combine with coefficient if needed
    if (coeff == 1) {
        return varPart; // just variable
    } else {
        ASTNode* coeffNode = Number_Node(coeff);
        ASTNode* mult = NewOperatorNode(OP_MULT);
        mult->Left = coeffNode;
        mult->Right = varPart;
        return mult;
    }
}
ASTNode* CombineLikeTerms(ASTNode* S, Hashmap* HM) {
    if (S == NULL) return NULL;

    // Handle absolute value as atomic
    if (S->Type == OPERATOR && S->value.operator == OP_ABS) {
        ASTNode* node = NewOperatorNode(OP_ABS);
        node->Left = NULL;
        node->Right = CombineLikeTerms(S->Right, HM);
        return node;
    }

    // Recurse for non-additive operators
    if (S->Type != OPERATOR || S->value.operator != OP_ADD) {
        S->Left = CombineLikeTerms(S->Left, HM);
        S->Right = CombineLikeTerms(S->Right, HM);
        return S;
    }

    // --- Start combine for addition ---
    List operands = NewList();
    Collect(S, OP_ADD, &operands);

    List absTerms = NewList(); // store |...| terms separately
    List Keys = NewList();
    for (int i = 0; i < operands.length; i++) {
        ASTNode* term = (ASTNode*)List_GetElement(&operands, i)->data;
        // skip abs from hashmap math, but preserve it
        if (term->Type == OPERATOR && term->value.operator == OP_ABS) {
            List_AppendElement(&absTerms, term);
            continue;
        }

        char* key = GetVariableKey(term);
        double coeff = GetCoeff(term);
        dbg_printf("\n%.4f%s\n", coeff, key);
        if (key == NULL) key = strdup(""); // constant

        double* cur = (double*)Map_GetKey(HM, key);
        if (!cur) {
            double* v = malloc(sizeof(double));
            *v = coeff;
            List_AppendElement(&Keys, strdup(key));
            Map_SetKey(HM, strdup(key), v);
        } else {
            *cur += coeff;
        }
        free(key);
    }

    // --- Rebuild result ---
    List NewOperands = NewList();
    dbg_printf("----------------");
    for (int i = 0; i < Keys.length; i++) {
        char* Key = (char*)List_GetElement(&Keys, i)->data;
        double* pval = (double*)Map_GetKey(HM, Key);
        dbg_printf("Key %s : %.4f", Key, *pval);
        if (!pval) continue;

        if (*pval == 0) continue;

        ASTNode* t = MakeVariableNode(Key, *pval);
        List_AppendElement(&NewOperands, t);
    }

    for (int i = 0; i < absTerms.length; i++) {
        ASTNode* a = (ASTNode*)List_GetElement(&absTerms, i)->data;
        List_AppendElement(&NewOperands, a);
    }

    if (NewOperands.length == 0) {
        return Number_Node(0);
    }
    if (NewOperands.length == 1) {
        return (ASTNode*)List_GetElement(&NewOperands, 0)->data;
    }
    ASTNode* Rebuilt = RebuildTree(OP_ADD, NewOperands);
    return Rebuilt;;
}

int CMP_Doub(void* A, void* B) {
    double AA = *(double*)A;
    double BB = *(double*)B;
    if (AA == BB) {
        return 0;
    }
    return 1;
}
int strcmp3(void* A, void* B) {
    char* C = (char*)A;
    char* D = (char*)B;

    return strcmp(C, D);
}

ASTNode* SimplifyExpression(ASTNode* S) {
    if (S==NULL) {
        return NULL;
    }
    if (S->Type == NUMBER || S->Type == IDENTIFIER) {
        return S;
    }

    if (S->Type == OPERATOR) {
        if (S->value.operator == OP_ABS) {
            ASTNode* O = NewOperatorNode(OP_ABS);
            O->Left = NULL;
            O->Right = SimplifyExpression(S->Right);
            return O;
        }
        ASTNode* Left = SimplifyExpression(S->Left);
        ASTNode* Right = SimplifyExpression(S->Right);
        if (Left && Right) {

        if (Left->Type == NUMBER && Right->Type == NUMBER) {
            ASTNode* New = pool_alloc_node();
            New->Type = NUMBER;
            New->value.number = DoOperation(Left->value.number, Right->value.number, S->value.operator);
            return New;
        }}
        
        switch (S->value.operator) {
            // Algebraic identities:
            /*
                n+0 & 0+n = n
                n-0 & 
            */
            case OP_ADD:
                if (IsIdentity(Left, 0)) {
                    return Right;
                }
                else if (IsIdentity(Right, 0))
                {
                    return Left;
                }
                break;
            case OP_MINUS:
                if (IsIdentity(Left, 0)) {
                    return Right;
                }
                break;
            case OP_MULT:
                if (IsIdentity(Left, 0) || IsIdentity(Right, 0)) {
                    return Number_Node(0);
                }
                if (IsIdentity(Left, 1)) {
                    return Right;
                }
                else if (IsIdentity(Right, 1))
                {
                    return Left;
                }
                break;
            case OP_DIV:
                if (IsIdentity(Left, 0)) {
                    return Number_Node(0);
                }
                else if (IsIdentity(Right, 1))
                {
                    return Left;
                }
                break;
            case OP_EXP:
                if (IsIdentity(Right, 0)) {
                    return Number_Node(1);
                }
                if (IsIdentity(Right, 1)) {
                    return Left;
                }
                if (IsIdentity(Left, 0)) {
                    return Number_Node(0);
                }
                if (IsIdentity(Left, 1)) {
                    return Number_Node(1);
                }
                break;

        }
        ASTNode* node = Flatten(S);
        Hashmap Variables = NewMap(16, &HashFunction, &strcmp3);
        node = CombineLikeTerms(node,&Variables);
        return node;
        
    }
    
    return S;

};
char Ops[10] = {'+', '-', '*', '/', '^', 'L', '=', '0', '|'};
void PrintAST(ASTNode* node, int depth) {
    if (depth == 0) {
       dbg_printf("\n");
    }
    if (node == NULL) return;
    // Indentation
    for (int i = 0; i < depth; i++) {
       dbg_printf("  "); // two spaces per level
    }
    // Print node info
    if (node->Type == NUMBER) {
       dbg_printf("NUMBER: %.4f\n", node->value.number);
    }
    else if (node->Type == IDENTIFIER) {
       dbg_printf("IDENTIFIER: %c\n", node->value.identifier);
    }
    else if (node->Type == OPERATOR) {
       dbg_printf("OPERATOR: %c\n", Ops[node->value.operator]);
    }
    else {
       dbg_printf("UNKNOWN NODE: %d", node->Type);
    }

    // Recurse into children
    if (node->Left != NULL)  PrintAST(node->Left, depth + 1);
    if (node->Right != NULL) PrintAST(node->Right, depth + 1);
}

void PrintLst(void* V) {
    ASTNode* ToPrint = (ASTNode*)V;
    
    if (ToPrint->Type == IDENTIFIER) {
       dbg_printf("(Identifier %c)", ToPrint->value.identifier);
    }
    if (ToPrint->Type == NUMBER) {
       dbg_printf("(Number %.4f)", ToPrint->value.number);
    }
    if (ToPrint->Type == OPERATOR) {
       dbg_printf("(Operator %c)", Ops[ToPrint->value.operator]);
    }
    if (ToPrint->Type == LEFT_PAREN) {
       dbg_printf("(LEFT_PAREN)");
    }
    if (ToPrint->Type == RIGHT_PAREN) {
       dbg_printf("(RIGHT PAREN)");
    }
}
bool IsStartOfStr(string Full, string Checking, int index) {
    if (index < Full.length-3) {
        if (Full.Content[index] == Checking.Content[0] && Full.Content[index+1] == Checking.Content[1] && Full.Content[index+2] == Checking.Content[2]) {
            return true;
        }
    }
    return false;
}
struct NumSubstringResult {
    double number;
    int AmtToSkip;
};

struct NumSubstringResult GetNumberFromSubstring(string s, int Start) {
    struct NumSubstringResult NS = {0, 0};
    char Buffer[32];
    int bufIndex = 0;

    int i = Start;
    bool hasDigit = false;

    while (i < s.length && bufIndex < (int)sizeof(Buffer) - 1) {
        char ch = s.Content[i];
        if (GetCharacterToken(ch) == NUMBER || ch == '.') {
            Buffer[bufIndex++] = ch;
            NS.AmtToSkip++;
            if (GetCharacterToken(ch) == NUMBER)
                hasDigit = true;
            i++;
        } else {
            break;
        }
    }

    Buffer[bufIndex] = '\0';   // null terminate
    
    if (hasDigit) {
        string s1 = NewStr(Buffer);
        NS.number = StringToDouble(s1);
        DestroyString(&s1);
    } else {
        NS.number = 0.0;
    }
    
    return NS;
}
char tolower2(char s) {
    if (s >= 'A' && s < 'a') {
        return s^32;
    }
    return s;
}
OperatorType StringOperatorHandler(string s, int i) {
    // not enough space for a 3-char function
    if (i > s.length - 3) {
        // maybe still a single-char operator
        char tmp[2] = { s.Content[i], '\0' };
        return OperatorFromString((string){ tmp, 2 });
    }

    // check 3-letter operators first
    char buf[4] = { 
        (char)tolower2(s.Content[i]), 
        (char)tolower2(s.Content[i+1]), 
        (char)tolower2(s.Content[i+2]), 
        '\0' 
    };

    if (strcmp(buf, "log") == 0) return OP_LOG;
    if (strcmp(buf, "abs") == 0) return OP_ABS;

    // fallback: single-char operator
    char tmp[2] = { s.Content[i], '\0' };
    return OperatorFromString((string){ tmp, 2 });
}
List GetTokens(string s) {
    List ToReturn = NewList();
    bool X_EXP_NEG1 = false; // if true, when this encounters a right parenthesis, it'll add "^-1".
    Node_Type Previous_Token = NONE;
    for (int i = 0; i < s.length; i++) {
            
            // Adds first element to list;
            Node_Type T = GetCharacterToken(s.Content[i]);
            switch (T) {
                case NUMBER:
                    Previous_Token = NUMBER;
                    struct NumSubstringResult NS = GetNumberFromSubstring(s, i);
                    ASTNode* ToAppend = Number_Node(NS.number);
                    List_AppendElement(&ToReturn, ToAppend);
                    if (NS.AmtToSkip == 0) {
                        break;
                    }
                    i += NS.AmtToSkip-1;
                    if (i <= s.length-1) {
                        if (GetCharacterToken(s.Content[i+1]) == IDENTIFIER || s.Content[i+1] == '(' || s.Content[i+1] == 'a' ) {
                            if (StringOperatorHandler(s,i+1) == OP_NULL || s.Content[i+1] == '(' || s.Content[i+1] == 'a' ) {
                                ASTNode* MultToAppend = NewOperatorNode(OP_MULT);
                                List_AppendElement(&ToReturn, MultToAppend);
                            }
                        }
                    }
                    break;
                case IDENTIFIER:
                    Previous_Token = IDENTIFIER;
                    OperatorType OPH = StringOperatorHandler(s,i);
                    if (OPH == OP_NULL) {
                    ASTNode* ToAppend2 = pool_alloc_node();
                    ToAppend2->Type = IDENTIFIER;
                    ToAppend2->value.identifier = s.Content[i];
                    List_AppendElement(&ToReturn, ToAppend2);
                    break;
                    }
                    else {
                        ASTNode* ToAppend4 = NewOperatorNode(OPH);
                        List_AppendElement(&ToReturn, ToAppend4);
                        i += 2;
                        break;
                    }
                case LEFT_PAREN:
                    Previous_Token = LEFT_PAREN;
                    ASTNode* LeftParenAppend = pool_alloc_node();
                    LeftParenAppend->Type = LEFT_PAREN;
                    List_AppendElement(&ToReturn, LeftParenAppend);
                    break;
                case RIGHT_PAREN:
                    Previous_Token = RIGHT_PAREN;
                    ASTNode* RightParen = pool_alloc_node();
                    RightParen->Type = RIGHT_PAREN;
                    List_AppendElement(&ToReturn, RightParen);
                    if (X_EXP_NEG1) {
                        ASTNode* EXP2 = NewOperatorNode(OP_EXP);
                        ASTNode* NEG1_2 = Number_Node(-1);
                        List_AppendElement(&ToReturn, EXP2);
                        List_AppendElement(&ToReturn, NEG1_2);
                        // TODO: make this work with nested problems like 1/(2+(x+2) ).
                    }
                    break;
                case OPERATOR:
                    Previous_Token = OPERATOR;
                    if (s.Content[i] != '-' && s.Content[i] != '/') {
                        ASTNode* ToAppend3 = NewOperatorNode(StringOperatorHandler(s,i));
                        List_AppendElement(&ToReturn, ToAppend3);
                        break;
                    }
                    else if (s.Content[i] == '-')
                    {
                        
                    if (GetCharacterToken(s.Content[i+1]) == NUMBER) {
                        struct NumSubstringResult NS2 = GetNumberFromSubstring(s, i+1);
                        ASTNode* NumberToAppend = Number_Node(-1*NS2.number);
                        ASTNode* Plus = NewOperatorNode(OP_ADD);
                        if (s.Content[i-1] != '^' && s.Content[i-1] != '*') {
                            if (i-1 >= 0) {
                            if (GetCharacterToken(s.Content[i-1]) != OPERATOR) {
                            
                            List_AppendElement(&ToReturn, Plus);
                            }
                        }
                        }
                        List_AppendElement(&ToReturn, NumberToAppend);
                        i += NS2.AmtToSkip;
                    }
                    else if (GetCharacterToken(s.Content[i+1]) == IDENTIFIER) {
                        ASTNode* Mult = NewOperatorNode(OP_MULT);
                        ASTNode* Identifier = pool_alloc_node();
                        ASTNode* NegativeOne = Number_Node(-1);
                        ASTNode* Plus = NewOperatorNode(OP_ADD);
                        Identifier->Type = IDENTIFIER;
                        Identifier->value.identifier = s.Content[i+1];
                        if (i-1 >= 0) {
                            if (GetCharacterToken(s.Content[i-1]) != OPERATOR && s.Content[i-1] != ')' && s.Content[i-1] != '(') {
                            List_AppendElement(&ToReturn, Plus);
                            }
                        }
                        List_AppendElement(&ToReturn, NegativeOne);
                        List_AppendElement(&ToReturn, Mult);
                        List_AppendElement(&ToReturn, Identifier);
                        i+=1;
                    }
                    if (i <= s.length-1) {
                        if (GetCharacterToken(s.Content[i+1]) == IDENTIFIER || s.Content[i+1] == '(' ) {
                            if (StringOperatorHandler(s,i+1) == OP_NULL || s.Content[i+1] == '(' ) {
                                ASTNode* MultToAppend = NewOperatorNode(OP_MULT);
                                List_AppendElement(&ToReturn, MultToAppend);
                            }
                        }
                    }
                    break;
                    }
                    else if (s.Content[i] == '/')
                    {
                        ASTNode* M = NewOperatorNode(OP_MULT);
                        List_AppendElement(&ToReturn, M);
                        ASTNode* Exp = NewOperatorNode(OP_EXP);
                        ASTNode* Neg1 = Number_Node(-1);
                        if (s.Content[i+1] == '(') {
                            X_EXP_NEG1 = true;
                        }
                        if (GetCharacterToken(s.Content[i+1]) == NUMBER) {
                            struct NumSubstringResult NS2 = GetNumberFromSubstring(s, i+1);
                            ASTNode* DivNumberNode = Number_Node(NS2.number);
                            List_AppendElement(&ToReturn, DivNumberNode);
                            List_AppendElement(&ToReturn, Exp);
                            if (GetCharacterToken(s.Content[i+NS2.AmtToSkip+1]) == IDENTIFIER) {
                                ASTNode* ToAlsoAppend = pool_alloc_node();
                                ToAlsoAppend->Type = IDENTIFIER;
                                ToAlsoAppend->value.identifier = s.Content[i+NS2.AmtToSkip+1];
                                i += 1;
                                List_AppendElement(&ToReturn, Neg1);
                            }
                            List_AppendElement(&ToReturn, Neg1);
                            i += NS2.AmtToSkip;
                        }
                        else if (GetCharacterToken(s.Content[i+1]) == IDENTIFIER)
                        {
                            ASTNode* NewID = pool_alloc_node();
                            NewID->Type = IDENTIFIER;
                            NewID->value.identifier = s.Content[i+1];
                            List_AppendElement(&ToReturn, NewID);
                            List_AppendElement(&ToReturn, Exp);
                            List_AppendElement(&ToReturn, Neg1);
                        }
                        
                    }
                    
                    
            }
        }
    
    ASTNode* Last_Node = (ASTNode*)List_GetElement(&ToReturn, -1);
    while ( ((ASTNode*)List_GetElement(&ToReturn, -1)->data)->Type == OPERATOR) {
            List_DeleteElement(&ToReturn, ToReturn.length-1);
    }
    return ToReturn;
    }

typedef struct {
    double M;
    double B;
} LinearForm;

double CollectNumberTermsOfOperation(OperatorType OT, ASTNode* A) {
    
    if (!A) {
        return 0;
    }
    double sum = 0;
    if (A->Type == OPERATOR) {
        if (A->value.operator == OT) {
            if (A->Left) {
            if (A->Left->Type == NUMBER) {
                sum += A->Left->value.number;
            }}
            if (A->Right) {
            if (A->Right->Type == NUMBER) {
                sum += A->Right->value.number;
            }
            
            }
        }
    }

    sum += CollectNumberTermsOfOperation(OT,A->Left);
    sum += CollectNumberTermsOfOperation(OT,A->Right);

    return sum;

}

bool IsAlreadySol(ASTNode* A) {
    if (A->Type == OPERATOR) {
        if (A->value.operator == OP_EQ) {
        if (A->Left->Type == IDENTIFIER && A->Right->Type == NUMBER) {
            return true;
        }
        else if (A->Left->Type == NUMBER && A->Right->Type == IDENTIFIER) {
            return true;
        }
        }
    }
    return false;
}

ASTNode* Solve_Equation(ASTNode* A) {
    if (IsAlreadySol(A)) {
        dbg_printf("ALREAYD SOLVED");
        return A;
    }
    dbg_printf("CURRENT ");
    PrintAST(A, 0);
    
    // combines like terms on both sides;
    ASTNode* Left = SimplifyExpression(A->Left);
    ASTNode* Right = SimplifyExpression(A->Right);
    // so now it'd be something like ax+b=bx+c, unless further simplification is needed.
    // move Right to left by minussing it
    
    ASTNode* Plus = NewOperatorNode(OP_ADD);

    Plus->Left = Left;
    Plus->Right = DistributeOverTree(Right, -1);
    A->Left = Plus;
    A->Right = Number_Node(0);
    // so now equation is in the form of ax+b=0, so now we just get a & b, and do -b/a and that's the solution.
    // so since it's in the form of ax+b=0, we can do A.Left.Left.Left and get a
    A->Left = SimplifyExpression(A->Left);
    
    dbg_printf("ZEROed ");
    PrintAST(A, 0);
    double M = CollectNumberTermsOfOperation(OP_MULT, A->Left);
    if (A->Left->Type == OPERATOR) {
        if (A->Left->value.operator == OP_ADD) {
            if (A->Left) {
            if (A->Left->Left->Type == IDENTIFIER) {
                M = 1;
            }}
        }
    }
    double B = CollectNumberTermsOfOperation(OP_ADD, A->Left);
    LinearForm L = {M,B};
    dbg_printf("\n%.4fx + %.4f = 0\n", L.M, L.B);
    dbg_printf("x = %.4f", (-1*L.B)/(L.M));
    ASTNode* ID = pool_alloc_node();
    ID->Type = IDENTIFIER;
    ID->value.identifier = 'x';
    ASTNode* ToReturn = PopulatedOperatorNode(OP_EQ, ID, Number_Node((-1*L.B)/(L.M)) );
    dbg_printf("ANS: ");
    PrintAST(ToReturn, 0);
    return ToReturn;


    
}
void PrintTreeAsEquation(ASTNode* A) {


    

    switch (A->Type) {
        case NUMBER:
           dbg_printf("%.4f", A->value.number);
        case OPERATOR:
           dbg_printf("%c", Ops[A->value.operator]);
        case IDENTIFIER:
           dbg_printf("%c", A->value.identifier);
    }

    if (A->Left) PrintTreeAsEquation(A->Left);
    if (A->Right) PrintTreeAsEquation(A->Right);
    

}
bool CheckIfExit(char* E) {
    char e = *(E);
    char x = *(E+1);
    char i = *(E+2);
    char t = *(E+3);

    if (e == 'e' && x == 'x' && i == 'i' && t == 't') {
        return true;
    }
    return false;
}
ASTNode* PopulatedOperatorNode(OperatorType OT, ASTNode* Left, ASTNode* Right) {
    ASTNode* O = NewOperatorNode(OT);
    O->Left = Left;
    O->Right = Right;
    return O;
}
ASTNode* CloneNode(ASTNode* A) {
    if (A == NULL) return NULL;
    ASTNode* ToReturn = pool_alloc_node();
    ToReturn->Type = A->Type;
    ToReturn->value = A->value;
    if (!ToReturn) {
       dbg_printf("Allocation Failure");
    }
    ToReturn->Left = CloneNode(A->Left);
    ToReturn->Right = CloneNode(A->Right);
    return ToReturn;
}
ASTNode* AttemptToDo(ASTNode* A) {
    if (A->Left) {
        if (A->Right) {
            if (A->Left->Type == NUMBER && A->Right->Type == NUMBER) {
                DoOperation(A->Left->value.number, A->Right->value.number, A->value.operator);
            }
        }
    }
    return A;
}
ASTNode* Distribute(ASTNode* node) {
    if (!node) return NULL;
    if (node->Type == OPERATOR && node->value.operator == OP_MULT) {
        ASTNode* L = node->Left;
        ASTNode* R = node->Right;

        // (A + B) * C → (A*C) + (B*C)
        if (!L) {

        }
        else if (L->Type == OPERATOR && L->value.operator == OP_ADD) {
            ASTNode* newLeft  = Distribute(PopulatedOperatorNode(OP_MULT, L->Left, CloneNode(R)));
            ASTNode* newRight = Distribute(PopulatedOperatorNode(OP_MULT, L->Right, CloneNode(R)));
            if (L->Left->Type == NUMBER && R->Type == NUMBER) {
                newLeft = Number_Node(L->Left->value.number * R->value.number);
            }
            if (L->Right->Type == NUMBER && R->Type == NUMBER) {
                newLeft = Number_Node(L->Right->value.number * R->value.number);
            }
            ASTNode* Attempt = PopulatedOperatorNode(OP_ADD, newLeft, newRight);
            return AttemptToDo(Attempt);
        }
        if (!R) {
            
        }
        else if (R->Type == OPERATOR && R->value.operator == OP_ADD) {
            ASTNode* newLeft  = Distribute(PopulatedOperatorNode(OP_MULT, CloneNode(L), R->Left));
            ASTNode* newRight = Distribute(PopulatedOperatorNode(OP_MULT, CloneNode(L), R->Right));
            if (L->Left->Type == NUMBER && L->Type == NUMBER) {
                newLeft = Number_Node(R->Left->value.number * L->value.number);
            }
            if (L->Right->Type == NUMBER && L->Type == NUMBER) {
                newLeft = Number_Node(R->Right->value.number * L->value.number);
            }
            ASTNode* Attempt = PopulatedOperatorNode(OP_ADD, newLeft, newRight);
            return AttemptToDo(Attempt);
        }
    }

    // Otherwise recurse
    node->Left  = Distribute(node->Left);
    node->Right = Distribute(node->Right);
    return node;
}
struct QuadraticVariable {
    char Identifier;
    double Power;
    double Coeff;
    bool Valid;
    bool Number;
    // ax^b
};
struct QuadraticVariable CollectExponentVar(ASTNode* A) {
    if (A == NULL) {
        return (struct QuadraticVariable){'_', 0, 0, false};
    }
    if (A->Type == NUMBER) {
        // 3
        return (struct QuadraticVariable){'a', 0, A->value.number, true,true};
    }
    if (A->Type == IDENTIFIER) {
        // x
        return (struct QuadraticVariable){A->value.identifier, 1, 1, true};
    }
    if (A->value.operator == OP_EXP) {
        if (A->Left && A->Right ) { // here's where read access violation happened
            if (A->Right->Type == NUMBER && A->Left->Type == IDENTIFIER) {
            return (struct QuadraticVariable){A->Left->value.identifier, A->Right->value.number, 1, true};
            }
        }
    }
    if (A->value.operator == OP_ADD) {
    struct QuadraticVariable L = CollectExponentVar(A->Left);
    struct QuadraticVariable R = CollectExponentVar(A->Right);

    if (L.Valid && !R.Valid) return L;
    if (!L.Valid && R.Valid) return R;
    if (L.Valid && R.Valid && L.Power == R.Power && L.Identifier ==  R.Identifier) {
        return (struct QuadraticVariable){L.Identifier, L.Power, L.Coeff + R.Coeff, true, L.Number && R.Number};
    }
    }
    if (A->value.operator == OP_MULT) {
        struct QuadraticVariable L = CollectExponentVar(A->Left);
        struct QuadraticVariable R = CollectExponentVar(A->Right);

        if (L.Valid && R.Valid && L.Identifier == R.Identifier) {
            return (struct QuadraticVariable){L.Identifier, L.Power + R.Power, L.Coeff * R.Coeff, true};
        }

        if (L.Valid && R.Valid && (L.Power == 0)) {
            return (struct QuadraticVariable){R.Identifier, R.Power, L.Coeff * R.Coeff, true,false};
        }
        if (L.Valid && R.Valid && (R.Power == 0) ) {
            return (struct QuadraticVariable){L.Identifier, L.Power, L.Coeff * R.Coeff, true,false};
        }

        if (A->Left->Type == NUMBER) {
            struct QuadraticVariable RV = R;
            if (RV.Valid)
                return (struct QuadraticVariable){RV.Identifier, RV.Power, RV.Coeff * A->Left->value.number, true,false};
        }

        if (A->Right->Type == NUMBER || R.Number ) {
            struct QuadraticVariable LV = L;
            if (LV.Valid)
                return (struct QuadraticVariable){LV.Identifier, LV.Power, LV.Coeff * A->Right->value.number, true,false};
        }

        if (A->Left->Type == IDENTIFIER && A->Right->Type == IDENTIFIER &&
            A->Left->value.identifier == A->Right->value.identifier) {
            return (struct QuadraticVariable){A->Left->value.identifier, 2, 1, true,false};
        }
    }
    
    return (struct QuadraticVariable){'_', 0, 0, false};
}

double SumCoeffsForPower(ASTNode* A, int pow) {
    if (!A) return 0.0;
    if (A->Type == NUMBER && pow == 0) return A->value.number;

    if (A->Type == OPERATOR && A->value.operator == OP_ADD) {
        return SumCoeffsForPower(A->Left, pow) + SumCoeffsForPower(A->Right, pow);
    }

    struct QuadraticVariable V = CollectExponentVar(A);
    if (V.Valid && V.Power == pow)
        return V.Coeff;

    return 0.0;
}
ASTNode* NewQuadraticNode(char Id, double Power, double Coeff) {
    ASTNode* IDNode = pool_alloc_node();
    IDNode->Type = IDENTIFIER;
    IDNode->value.identifier = Id;
    return PopulatedOperatorNode(OP_MULT, Number_Node(Coeff), PopulatedOperatorNode(OP_EXP, IDNode, Number_Node(Power) ));
}
ASTNode* ExponentLaws(ASTNode* A) {
    if (!A) {
        return NULL;
    }
    if (A->Type == OPERATOR && A->value.operator == OP_ABS) {
        return 0;
    }
    if (A->Type == OPERATOR) {
        if (A->value.operator == OP_MULT) {
            if (A->Left == NULL || A->Right == NULL) {
                return NULL;
            }
            struct QuadraticVariable Left = CollectExponentVar(A->Left);
            struct QuadraticVariable Right = CollectExponentVar(A->Right);
            if (Left.Valid && Right.Valid) {
                return NewQuadraticNode(Left.Identifier, Left.Power+Right.Power, Left.Coeff*Right.Coeff);
            }
            else if (Left.Valid && !Right.Valid && A->Right->Type == NUMBER) {
               return NewQuadraticNode(Left.Identifier, Left.Power, Left.Coeff * A->Right->value.number);
            }
            else if (!Left.Valid && Right.Valid && A->Left->Type == NUMBER) {
                return NewQuadraticNode(Right.Identifier, Right.Power, Right.Coeff * A->Left->value.number);
            }
        }
    }
    
    A->Left = ExponentLaws(A->Left);
    A->Right = ExponentLaws(A->Right);
    return A;

}
struct QuadraticVariable GetTerm(ASTNode* A, int pow) {

    if (A == NULL) {
        return (struct QuadraticVariable){'_', 0, 0, false};
    }
    struct QuadraticVariable V = CollectExponentVar(A);
    if (V.Power == pow) {
        return V;
    }
    else {
        if (A->Left != NULL) {
            struct QuadraticVariable V1 = GetTerm(A->Left, pow);
            if (V1.Valid && V1.Power == pow) {
                if (pow == 0) {
                    PrintAST(A,0);
                }
                return V1;
            }
        }
        if (A->Right != NULL) {
            struct QuadraticVariable V1 = GetTerm(A->Right, pow);
            if (V1.Valid && V1.Power == pow) { 
                if (pow == 0) {
                    PrintAST(A,0);
                }
                return V1;
            }
        }
    }
    return (struct QuadraticVariable){'a', 0, 0, false};
}
typedef struct {
    double r;
    double i;
    bool Real;
} Imaginary_Number;
typedef struct {
    Imaginary_Number A;
    Imaginary_Number B;
    int Amt;
} Solution_Pair;
typedef struct {
    Solution_Pair X_Intercepts;
    double Y_Intercept;
    double Vertex;
} Quadratic_Solution;
Quadratic_Solution Safe_QuadraticFormula(double A, double B, double C) {
    
    // -b +- sqrt(b^2 - 4ac) / 2a
    bool DoImag = false;
    double Safe_Discriminant = (B*B) - (4*A*C);
    if (Safe_Discriminant < 0) {
        DoImag = true;
        Safe_Discriminant *= -1;
    }
    Solution_Pair P = {{0,0,DoImag},{0,0,DoImag},2};
    P.A = (Imaginary_Number){ (((-1*B)+sqrt(Safe_Discriminant))/(2*A)),DoImag,DoImag};
    P.B = (Imaginary_Number){ (((-1*B)-sqrt(Safe_Discriminant))/(2*A)),DoImag,DoImag};

    if (P.A.r == P.B.r) {
        P.Amt = 1;
    }
    return (Quadratic_Solution){P, (C), (-1*B)/(2*A)};
}
char* ImagToString(Imaginary_Number N) {
    if (N.i == 0) {
        string s = DoubleToString(N.r);
        char* out = strdup(s.Content);
        DestroyString(&s);
        return out;
    } else {
        string a = DoubleToString(N.r);
        string b = DoubleToString(N.i);
        size_t needed = strlen(a.Content) + 1 + strlen(b.Content) + 2 + 1; // " + " and "i" and NUL
        char* out = malloc(needed);
        if (!out) { DestroyString(&a); DestroyString(&b); return NULL; }
        snprintf(out, needed, "%s+%si", a.Content, b.Content);
        DestroyString(&a);
        DestroyString(&b);
        return out;
    }
}
typedef struct {
    double x;
    double y;
} Point;
string QuadraticEquation(ASTNode* A) {
    // combines like terms on both sides;
    // so now it'd be something like ax^2 + bx + c, unless further simplification is needed.
    // move Right to left by minussing it

    ASTNode* Plus = NewOperatorNode(OP_ADD);

    Plus->Left = A->Left;
    Plus->Right = DistributeOverTree(A->Right, -1);
    A->Left = Plus;
    A->Right = Number_Node(0);
    PrintAST(A,0);
    dbg_printf("Before^");
    A->Left = SimplifyExpression(A->Left);
    // eq is now in the form of ax^2 + bx + c = 0
    PrintAST(A,0);
    dbg_printf("^ - A");
    double a = GetTerm(A->Left,2).Coeff;
    double b = SumCoeffsForPower(A->Left, 1);
    double c = SumCoeffsForPower(A->Left, 0);
   dbg_printf("A: %.4f", a);
    Quadratic_Solution Solution = Safe_QuadraticFormula(a,b,c);
    Solution_Pair X_Intercepts = Solution.X_Intercepts;
    Solution_Pair S = X_Intercepts;
    Variable Solving_Vertex[1] = {
        (Variable){'x', Solution.Vertex}
    };
   dbg_printf("\n--- SOLUTION: ---\n");
   char ToReturn[MAX_SIZE*2];
   dbg_printf("\n%.4fx^2 + %.4fx + %.4f = 0\n", a, b, c);
   if (S.Amt == 1) {
      dbg_printf("x = %s Vtx At: (%.4f, %.4f) Y-Intercept: (%.4f, 0)", a, b, c, ImagToString(S.A), Solution.Vertex, EvaluateSubstitution(A->Left, Solving_Vertex, 1,true), Solution.Y_Intercept);
      snprintf(ToReturn, sizeof(ToReturn), "x = %.4f*%.0fi V: (%.4f, %.4f) Y-Intercept: (%.4f, 0)", R(S.A.r),R(S.A.i),  R(Solution.Vertex), R(EvaluateSubstitution(A->Left, Solving_Vertex, 1,true)), Solution.Y_Intercept );
    }
    else {
       dbg_printf("x = %.4f+%.4fi; x = %.4f+%.4fi\nVtx At: (%.4f, %.4f)\nY-Intercept: (%.4f,0)", S.A.r, S.A.i, S.B.r, S.B.i, Solution.Vertex, EvaluateSubstitution(A->Left, Solving_Vertex, 1,true), Solution.Y_Intercept  );
       snprintf(ToReturn, sizeof(ToReturn), "x = %.4f*%.0fi; x = %.4f+%.4fi V: (%.4f, %.4f) Y-Intercept: (%.4f, 0)", R(S.A.r), R(S.A.i), R(S.B.r), R(S.B.i), R(Solution.Vertex), R(EvaluateSubstitution(A->Left, Solving_Vertex, 1,true)), Solution.Y_Intercept );
    }
    dbg_printf("\n-----------------\n");
    return NewStr(ToReturn);


}

void Solve_AbsEQ(ASTNode* A) {
    // 2|5+2|-2
    /*
    I rlly only have to worry about multiplication & addition here, since division & subtraction don't exist due to my tokenizer, & exponentiation is always positive.
    So, here's the basic plan:
    2|5+x|-7=8x-4
    2|5+x|=8x-11
    |5+x|=0.5(8x-11)
    |5+x|=4x-5.5 (use Distribute() here.)
    
    -1(5+x)=4x-5.5
    5+x=4x-5.5
    then call my linear equation solver.
    */
    PrintAST(A,0);
    double Addition = CollectNumberTermsOfOperation(OP_ADD, A->Left);
    double CoEff = CollectNumberTermsOfOperation(OP_MULT, A->Left);
   dbg_printf("%.4f | %.4f", Addition, CoEff);
    if (Addition != 0) {
        A->Right = PopulatedOperatorNode(OP_ADD, A->Right, Number_Node(-1*Addition));
        A->Right = SimplifyExpression(A->Right); 
        A->Left = PopulatedOperatorNode(OP_ADD, A->Left, Number_Node(-1*Addition));
        A->Left = SimplifyExpression(A->Right);
    }
    PrintAST(A,0);
}
ASTNode* InvertNums(ASTNode* A) {
    if (!A) {
        return NULL;
    }
    if (A->Type == NUMBER) {
        return Number_Node(A->value.number * -1);
    }
    A->Left = InvertNums(A->Left);
    A->Right = InvertNums(A->Right);
    return A;
}
ASTNode* ReplaceAbs(ASTNode* A, double m) {

    if (m == -1) {
    if (!A) return NULL;
    if (A->Type == OPERATOR && A->value.operator == OP_ABS) {
        return DistributeOverTree(A->Right, -1);
    }
    if (A->Type == IDENTIFIER || A->Type == NUMBER) {
        return A;
    }
    if (A->Type == OPERATOR) {
        A->Left = ReplaceAbs(A->Left, m);
        A->Right = ReplaceAbs(A->Right, m);
        return A;
    }
    }
    else if (m == 1)
    {
        if (!A) return NULL;
        if (A->Type == OPERATOR && A->value.operator == OP_ABS) {
            return A->Right;
        }
        if (A->Type == IDENTIFIER || A->Type == NUMBER) {
            return A;
        }
        if (A->Type == OPERATOR) {
            A->Left = ReplaceAbs(A->Left, m);
            A->Right = ReplaceAbs(A->Right, m);
        }
    }
    
    return NULL;
}


List SplitByCharacter(string s, char c) {
    // takes in string, and returns a list of substrings split by character
    // ex: ABCDBCE, 'c' -> AB DB E
    List Toreturn = NewList();
    string* initial = malloc(sizeof(string));
    *initial = NewStr("");
    List_AppendElement(&Toreturn, initial);
    for (int i = 0; i < s.length; i++) {
        if (s.Content[i] == c) {
            string* new = malloc(sizeof(string));
            *new = NewStr("");
            List_AppendElement(&Toreturn, new);
            continue;
        }
        string* c = (string*)List_GetElement(&Toreturn, -1)->data;
        AddCharacter(c, s.Content[i]);
    }

    return Toreturn;

}
void PrintStrInLst(void* L) {
    if (L == NULL ) {
       dbg_printf("NULL");
        return;
    }
   dbg_printf("%s", ((string*)L)->Content);
}
List GetTokenList(string s) {

    List C = SplitByCharacter(s, ';');
    
    List_ForEach(&C, &PrintStrInLst);
    List ToReturn = NewList();

    for (int i = 0; i < C.length; i++) {
        List* ToAppend = malloc(sizeof(List));
        List ToAppend2 = GetTokens(*(string*)List_GetElement(&C, i)->data);
        *ToAppend = ToAppend2;
       dbg_printf("\nAppended List W/ %d Size; made from '%s':\n", ToAppend->length, (*(string*)List_GetElement(&C,i)->data).Content );
        List_AppendElement(&ToReturn, ToAppend);
    }
    return ToReturn;
}
double CollectCoeffForVariable(ASTNode* A, char ID) {
    if (!A) {
        return 0;
    }
    if (A->Type == IDENTIFIER) {
        if (A->value.identifier == ID) {
            return 1;
        }
        return 0;
    }
    if (A->Type == NUMBER && ID == ' ') {
        return A->value.number;
    }
    if (A->Type != OPERATOR) {
        return 0;
    }
    if (A->Type == OPERATOR && A->value.operator == OP_MULT) {
        if ((A->Left->Type == NUMBER || A->Right->Type == NUMBER) && (A->Left->Type == IDENTIFIER || A->Right->Type == IDENTIFIER)) {
            char CurrentID = A->Left->Type == IDENTIFIER ? A->Left->value.identifier : A->Right->value.identifier;
            if (CurrentID == ID) {
            double Coeff = A->Left->Type == NUMBER ? A->Left->value.number : A->Right->value.number;
            return Coeff;
            }
            
        }
    }
    else if (A->Type == OPERATOR && A->value.operator == OP_ADD) {
        if (A->Left->Type == IDENTIFIER || A->Right->Type == IDENTIFIER) {
            char CurrentID = A->Left->Type == IDENTIFIER ? A->Left->value.identifier : A->Right->value.identifier;
            if (CurrentID == ID) {
                return 1;
            }
            
        }
        else if (ID == ' ')
        {
            if (A->Left->Type == NUMBER || A->Right->Type == NUMBER) {
                double Coeff = A->Left->Type == NUMBER ? A->Left->value.number : A->Right->value.number;
                return Coeff;
            }
        }
        
    }
    double sum = 0;
    sum += CollectCoeffForVariable(A->Left, ID);
    sum += CollectCoeffForVariable(A->Right, ID);
    return sum;
}
typedef struct {

    double x;
    double y;
    double z;
    double ans;
    bool Do_Z;
} MtrxRow;
typedef struct {

    MtrxRow One;
    MtrxRow Two;
    MtrxRow Three;
    double Dims;
    bool Do_Z;
} Gaussian_Matrix;
MtrxRow* MakeRowFromEQ(ASTNode* A, bool HAS_Z) {
    if (!A) {
       dbg_printf("Row Creation failure");
        MtrxRow ToReturn = {0, 0, 0, 0, 0};
        return &ToReturn;
    }
    MtrxRow* ToReturn = malloc(sizeof(MtrxRow));
    ToReturn->x = CollectCoeffForVariable(A->Left, 'x') - CollectCoeffForVariable(A->Right, 'x');
    ToReturn->y = CollectCoeffForVariable(A->Left, 'y') - CollectCoeffForVariable(A->Right, 'y');
    if (HAS_Z) {
        ToReturn->z = CollectCoeffForVariable(A->Left, 'z') - CollectCoeffForVariable(A->Right, 'z');
    }
    ToReturn->ans = CollectCoeffForVariable(A->Right, ' ');
    ToReturn->Do_Z = HAS_Z;
    return ToReturn;
}
MtrxRow Multiply_Mtrx(MtrxRow* A, double B, bool Doing_Z) {
    if (B == 0) {
       dbg_printf("Not allowed to multiply a matrix by 0 during Gaussian Elimination.");
        return *A;
    }

    MtrxRow ToReturn;
    ToReturn.x = A->x*B;
    ToReturn.y = A->y*B;
    if (Doing_Z) {
        ToReturn.z = A->z*B;
    }
    ToReturn.ans = A->ans*B;
    return ToReturn;

}
MtrxRow Add_Mtrx(MtrxRow* A, MtrxRow* B, bool Doing_Z) {
    MtrxRow ToReturn;
    ToReturn.x = A->x+B->x;
    ToReturn.y = A->y+B->y;
    if (Doing_Z) {
        ToReturn.z = A->z + B->z;
    }
    ToReturn.ans = A->ans+B->ans;
    return ToReturn;
}
ASTNode* RowToEquation(MtrxRow M, bool Doing_Z) {
    ASTNode* X_ID = pool_alloc_node();
    ASTNode* Y_ID = pool_alloc_node();
    X_ID->Left = NULL;
    X_ID->Right = NULL;
    X_ID->Type = IDENTIFIER;
    X_ID->value.identifier = 'x';
    Y_ID->Left = NULL;
    Y_ID->Right = NULL;
    Y_ID->Type = IDENTIFIER;
    Y_ID->value.identifier = 'y';
    ASTNode* X_Coeff = PopulatedOperatorNode(OP_MULT, Number_Node(M.x), X_ID);
    ASTNode* Y_Coeff = PopulatedOperatorNode(OP_MULT, Number_Node(M.y), Y_ID);
    ASTNode* Ans = Number_Node(M.ans);
    if (Doing_Z) {
        ASTNode* Z_ID = pool_alloc_node();
        Z_ID->Left = NULL;
        Z_ID->Right = NULL;
        Z_ID->Type = IDENTIFIER;
        Z_ID->value.identifier = 'z';
        ASTNode* Z_Coeff = PopulatedOperatorNode(OP_MULT, Number_Node(M.z), Z_ID);
        
        return PopulatedOperatorNode(OP_EQ, PopulatedOperatorNode(OP_ADD, X_Coeff, PopulatedOperatorNode(OP_ADD, Y_Coeff, Z_Coeff)), Ans);

    }
    else {
        return PopulatedOperatorNode(OP_EQ, PopulatedOperatorNode(OP_ADD, X_Coeff, Y_Coeff), Ans);
    }
    return PopulatedOperatorNode(OP_EQ, PopulatedOperatorNode(OP_ADD, X_Coeff, Y_Coeff), Ans);
}
void PrntRow(MtrxRow M) {
   dbg_printf("\n%.4fx + %.4fy + %.4fz = %.4f\n", M.x, M.y, M.z, M.ans);
}
void PrntMtrx(Gaussian_Matrix MTX, bool DoingZ) {
    if (DoingZ) {
       dbg_printf("\n");
       dbg_printf("\n[%.4f %.4f %.4f|%.4f],", MTX.One.x, MTX.One.y, MTX.One.z, MTX.One.ans);
       dbg_printf("\n[%.4f %.4f %.4f|%.4f],", MTX.Two.x, MTX.Two.y, MTX.Two.z, MTX.Two.ans);
       dbg_printf("\n[%.4f %.4f %.4f|%.4f],\n", MTX.Three.x, MTX.Three.y, MTX.Three.z, MTX.Three.ans);
    }
    else {
        dbg_printf("\n");
        dbg_printf("\n[%.4f %.4f|%.4f],", MTX.One.x, MTX.One.y, MTX.One.ans);
        dbg_printf("\n[%.4f %.4f|%.4f]", MTX.Two.x, MTX.Two.y, MTX.Two.ans);
    }
}
string Gaussian_Elimination(ASTNode* A, ASTNode* B, ASTNode* C,bool DoingZ) {
    
    Gaussian_Matrix* MX = malloc(sizeof(Gaussian_Matrix));
    MX->Do_Z = DoingZ;
    MtrxRow* I = MakeRowFromEQ(A,DoingZ);
    MtrxRow* II = MakeRowFromEQ(B,DoingZ);
    MX->One = *I;
    MX->Two = *II;
    printf("STARTING");
    PrntRow(MX->One);
    PrntRow(MX->Two);
    if (DoingZ) {
        MtrxRow* III = MakeRowFromEQ(C,true);
        MX->Three = *III;
        PrntRow(MX->Three);
    }
    char Return[MAX_SIZE];
    /*

    R1:[A,B,C|D]
    R2:[E,F,G|H]
    R3:[I,J,K|L]
    R1 has all 3, which is normal.
    R2 needs to have E removed; so:
    R2 -> (-A*R2)+(E*R1)
    R2 now is [0,-AF,-AG|-AH].
    R3 -> (-A*R3)+(I*R1)
    R3 now is [0,-AJ,-AK|-AL].
    R3 -> (-F*R3)+(J*R2)
    R3 now is [0, 0, -AFK|-AFL]
    
    Then, Z=-AFL/-AFK
    Then substitute in R2 & R1, and we got the solution.
    
    */
    double factor_2 = MX->Two.x / MX->One.x;
    MtrxRow New1 = Multiply_Mtrx(&MX->One, -factor_2,DoingZ); // I only have adding matrices, so doing -factor will work.
    MX->Two = Add_Mtrx(&New1, &MX->Two,DoingZ);
    if (DoingZ) {
        
        double factor_3A = MX->Three.x/MX->One.x;
        MtrxRow New3A = Multiply_Mtrx(&MX->One, -factor_3A, DoingZ);
        MX->Three = Add_Mtrx(&New3A, &MX->Three, DoingZ);

        double factor_3B = MX->Three.y/MX->Two.y;
        MtrxRow New3B = Multiply_Mtrx(&MX->Two,-factor_3B,DoingZ);
        MX->Three = Add_Mtrx(&New3B,&MX->Three,DoingZ);
        PrntRow(MX->Three);
        double Z = MX->Three.ans/MX->Three.z;
        // ay+bz = ans
        // y = (ans-bz)/a
        double Y = ((MX->Two.ans)-(MX->Two.z*Z))/MX->Two.y;
        // ax+by+cz=ans
        // ax=ans-by-cz
        // x = (ans-by-cz)/a
        double X = ((MX->One.ans)-(MX->One.y*Y)-(MX->One.z*Z))/MX->One.x;
       dbg_printf("(%.4f, %.4f, %.4f)", X, Y, Z);
        snprintf(Return, sizeof(Return), "(%.4f, %.4f, %.4f)", R(X), R(Y), R(Z) );
        return NewStr(Return);
        
    }
    else {
        // so now it's a 2D matrix, the bottom row is the y value
        double Y = round(pow(10,5)*MX->Two.ans/MX->Two.y)/pow(10,5);
       dbg_printf("\n Y = %.4f \n", Y);
        Variable Vars[2] = {
            (Variable){'y', Y},
            (Variable){'Y', Y},
        };
        // ax+by=ans
        // ax=ans-by
        double X = round(pow(10,5)*(MX->One.ans-(Y*MX->One.y))/MX->One.x)/pow(10,5);
        snprintf(Return, sizeof(Return), "(%.4f, %.4f)", R(X), R(Y) );
        return NewStr(Return);
    }

}
ASTNode* Simplification_Pipeline(ASTNode* A) {
    A->Left = Flatten(A->Left);
    A->Right = Flatten(A->Right);
    A->Left = Distribute(A->Left);
    A->Right = Distribute(A->Right);
    A->Left = SimplifyExpression(A->Left);
    A->Right = SimplifyExpression(A->Right);
    return A;
}  
typedef struct {
    double pos;
    double neg;
} Sol_P;
string EvaluateEquation(string S, Equation_Type T) {
    
   dbg_printf("S:");
   ASTNode* V = MakeVariableNode("x^2",2);
   ASTNode* TT = PopulatedOperatorNode(OP_MULT, Number_Node(2), V);
   dbg_printf("%.4f", GetCoeff(TT));
   char AnsBuf[MAX_SIZE];
    switch (T) {
        case ARITHMETIC: {
            List Tokens = GetTokens(S);
            ASTNode* Tree = Parse_EQ(&Tokens);
    
            PrintAST(Tree,0);
            List_ForEach(&Tokens, &PrintLst);
            Tree->Left = Flatten(Tree->Left);
            Tree->Left = SimplifyExpression(Tree->Left);
            Tree->Right = Flatten(Tree->Right);
            Tree->Right = SimplifyExpression(Tree->Right);
            Tree = Flatten(Tree);
            Tree = SimplifyExpression(Tree);
            snprintf(AnsBuf, sizeof(AnsBuf), "%s=%.4f", S.Content, R(EvaluateArithmetic(Tree)) );
            FreeASTNode(Tree);
            return NewStr(AnsBuf);
            break;}
        case LINEAR: {
            List Tokens = GetTokens(S);
            ASTNode* Tree = Parse_EQ(&Tokens);
            List_ForEach(&Tokens, &PrintLst);
            Tree->Left = Flatten(Tree->Left);
            Tree->Left = Distribute(Tree->Left);
            Tree->Left = SimplifyExpression(Tree->Left);
            Tree->Right = Flatten(Tree->Right);
            Tree->Right = Distribute(Tree->Right);
            Tree->Right = SimplifyExpression(Tree->Right);
            ASTNode* Solved_Equation = Solve_Equation(Tree);
            snprintf(AnsBuf, sizeof(AnsBuf), "x=%.4f", R(Solved_Equation->Right->value.number) );
            FreeASTNode(Tree);
            return NewStr(AnsBuf);
            break;}
        case QUADRATIC: {
            List Tokens = GetTokens(S);
            ASTNode* Tree = Parse_EQ(&Tokens);
    
            PrintAST(Tree,0);
            List_ForEach(&Tokens, &PrintLst);
            Tree->Left = Flatten(Tree->Left);
            Tree->Right = Flatten(Tree->Right);
            Tree->Left = Distribute(Tree->Left);
            Tree->Right = Distribute(Tree->Right);
            PrintAST(Tree,0);
            Tree->Left = ExponentLaws(Tree->Left);
            Tree->Right = ExponentLaws(Tree->Right);
            PrintAST(Tree,0);
            string s = QuadraticEquation(Tree);
            FreeASTNode(Tree);
            return s;
            break;}
        case ABS: {
            List Tokens = GetTokens(S);
            ASTNode* Tree = Parse_EQ(&Tokens);
            Sol_P P = {0,0};
            PrintAST(Tree,0);
            List_ForEach(&Tokens, &PrintLst);
            ASTNode* Neg = CloneNode(Tree);
            Neg->Left = ReplaceAbs(Neg->Left, -1);
            Neg->Right = ReplaceAbs(Neg->Right, -1);
            ASTNode* A = Solve_Equation(Neg);
            P.neg = A->Right->value.number;
            FreeASTNode(A);

            ASTNode* Pos = CloneNode(Tree);
            Pos->Left = ReplaceAbs(Pos->Left, 1);
            Pos->Right = ReplaceAbs(Pos->Right, 1);
            
            PrintAST(Pos,0);
            Pos->Right = Distribute(Pos->Right);
            Pos->Left = Distribute(Pos->Left);
            Pos->Left = Flatten(Pos->Left);
            Pos->Left = Distribute(Pos->Left);
            Pos->Left = SimplifyExpression(Pos->Left);
            Pos->Right = Flatten(Pos->Right);
            Pos->Right = Distribute(Pos->Right);
            Pos->Right = SimplifyExpression(Pos->Right);
            ASTNode* B = Solve_Equation(Pos);
            P.pos = B->Right->value.number;
            snprintf(AnsBuf, sizeof(AnsBuf), "x=%.4f,x=%.4f", R(P.neg), R(P.pos) );
            return NewStr(AnsBuf);
            break;}
        case SYSTEMS: {
            // ex: 5x+2y=5;9x+25y=15
            List Equations = GetTokenList(S);
           dbg_printf("EQLen: %d", Equations.length);
            List TokensA = *(List*)Equations.First->data;
           dbg_printf("\n");
            List_ForEach(&TokensA,&PrintLst);
            List TokensB = *(List*)Equations.First->Next->data;
           dbg_printf("\n");
            List_ForEach(&TokensB,&PrintLst);
            bool Doing_Z = true;
            if (Equations.length == 3) {

                List TokensC = *(List*)Equations.First->Next->Next->data;
               dbg_printf("\nA");
                List_ForEach(&TokensC,&PrintLst);
               dbg_printf("---");
                Doing_Z = true;
                ASTNode* Tree_A = Parse_EQ(&TokensA);
                ASTNode* Tree_B = Parse_EQ(&TokensB);
                ASTNode* Tree_C = Parse_EQ(&TokensC);
                Tree_A = Simplification_Pipeline(Tree_A);
                Tree_B = Simplification_Pipeline(Tree_B);
                Tree_C = Simplification_Pipeline(Tree_C);
                string l = Gaussian_Elimination(Tree_A, Tree_B, Tree_C, true);
                FreeASTNode(Tree_A);
                FreeASTNode(Tree_B);
                FreeASTNode(Tree_C);
                return l;
            }
            else {
                ASTNode* Tree_A = Parse_EQ(&TokensA);
                ASTNode* Tree_B = Parse_EQ(&TokensB);
                Tree_A = Simplification_Pipeline(Tree_A);
                Tree_B = Simplification_Pipeline(Tree_B);
                string l2 = Gaussian_Elimination(Tree_A, Tree_B, NULL, false);
                FreeASTNode(Tree_A);
                FreeASTNode(Tree_B);
                return l2;
            }
            
            break;}
    }
}
