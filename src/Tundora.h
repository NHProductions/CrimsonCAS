#ifndef TUNDORA_H

#define TUNDORA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

// ---- Compare function typedef ----
typedef int (*CompareFunction)(void*, void*);

// ---- String ----
typedef struct {
    char* Content;
    int length;
    int capacity;
} string;

string VoidString(int len);
string NewStr(const char* Content);
void AddCharacter(string* a, char b);
void ConcatStrings(string* a, string b);
void DestroyString(string* s);

// ---- Linked List ----
struct List_Node {
    void* data;
    struct List_Node* Next;
};

typedef struct {
    struct List_Node* First;
    int length;
} List;

List NewList();
struct List_Node* List_GetElement(List* L, int Index);
void List_AppendElement(List* L, void* Value);
void List_InsertElement(List* L, int Index, void* Value);
void List_DeleteElement(List* L, int Index);
int CMP_int(void* AA, void* BA);
List List_FindInstances(List* L, void* Value, CompareFunction cmp);

enum PRINT_LIST_SPECIFIERS {
    INT = 1
};

void List_Print(List* L, int Case);
void* List_VdP(int i);
void List_Free_List(List* L);
void List_ForEach(List* L, void (*func)(void*));

// ---- Line (Queue/Stack) ----
enum LINE_TYPES {
    QUEUE = 0,
    STACK = 1
};

typedef struct {
    int Type;
    List Entries;
} Line;

Line NewLine(int Type);
void Line_Push(Line* Ln, void* v);
void* Line_Pop(Line* Ln);
void* Line_Peek(Line* Ln);

// ---- Array ----
typedef struct {
    void* Data;
    int Length;
    int ElementSize;
} Array;

Array NewArray(int len, int elementSize);
void Array_Set(Array* A, int index, void* value);
void* Array_Get(Array* A, int index);
void Array_Free(Array* A);

// ---- Hashmap ----
typedef struct {
    void* Key;
    void* Value;
} Hashmap_Entry;

typedef struct {
    Array Buckets;
    int (*hash)(void* v);
    int (*comp)(void* a, void* b);
} Hashmap;

Hashmap NewMap(int Bucket_Amt, int (*hash)(void* v), int (*comp)(void* a, void* b));
void* Map_GetKey(Hashmap* HM, void* key);
void Map_SetKey(Hashmap* HM, void* key, void* value);
void DeleteKey(Hashmap* HM, void* key);
int BasicHash(void* s1);
List* HM_GetKeys(Hashmap* HM);
void Map_Free(Hashmap* HM);

#endif // DATALIB_H
