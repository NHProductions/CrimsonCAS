#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include "Tundora.h"
typedef int (*CompareFunction)(void*,void*);
string VoidString(int capacity) {
    string s;
    s.length = 0;
    s.capacity = (capacity > 0 ? capacity : 16);
    s.Content = malloc(s.capacity + 1); // +1 for NUL
    if (!s.Content) { s.capacity = 0; s.length = 0; return s; }
    s.Content[0] = '\0';
    return s;
}

string NewStr(const char* Content) {
    if (!Content) return VoidString(0);
    int length = strlen(Content);            // simpler and safe
    string s = VoidString(length);
    memcpy(s.Content, Content, length);
    s.length = length;
    s.Content[length] = '\0';
    return s;
}

static void ensure_capacity(string* s, int needed) {
    if (s->capacity >= needed) return;
    int newcap = s->capacity ? s->capacity : 16;
    while (newcap < needed) newcap *= 2;
    char *p = realloc(s->Content, newcap + 1);
    if (!p) return; // leave as-is
    s->Content = p;
    s->capacity = newcap;
}

void AddCharacter(string* a, char b) {
    if (!a) return;
    ensure_capacity(a, a->length + 1);
    a->Content[a->length++] = b;
    a->Content[a->length] = '\0';
}

void ConcatStrings(string* a, string b) {
    if (!a) return;
    ensure_capacity(a, a->length + b.length);
    memcpy(a->Content + a->length, b.Content, b.length);
    a->length += b.length;
    a->Content[a->length] = '\0';
}

void DestroyString(string* s) {
    if (!s) return;
    free(s->Content);
    s->Content = NULL;
    s->length = 0;
    s->capacity = 0;
}
List NewList() {
    List Toreturn;
    Toreturn.First = NULL;
    Toreturn.length = 0;
    return Toreturn;
};
struct List_Node* List_GetElement(List* L, int Index) {
    if (Index == -1) {
        // Gets last element in the list
        struct List_Node* Current_Data = L->First;
        while (Current_Data->Next != NULL) {
            Current_Data = Current_Data->Next;
        }
        return Current_Data;
    }
    else {
        // gets element at index
        struct List_Node* Current_Data = L->First;
        if (Index >= L->length) {
            printf("List Access Error!");    
            return NULL;
        }
        for (int i = 0; i < Index; i++) {
            Current_Data = Current_Data->Next;
        }
        return Current_Data;


    }
}
void List_AppendElement(List* L, void* Value) {
    // Appends element to end of array.
    struct List_Node* ToAdd = malloc(sizeof(struct List_Node));
    ToAdd->data = Value;
    ToAdd->Next = NULL;
    if (L->length == 0) {
        L->First = ToAdd;
    }
    else {
        List_GetElement(L,-1)->Next = ToAdd;
    }
    L->length++;


};
void List_InsertElement(List* L, int Index, void *Value) {
    // so here, gotta change the element before index's -> next value to the new value
    
    struct List_Node* ToAdd = malloc(sizeof(struct List_Node));
    ToAdd->data = Value;
    if (Index != 0) {
    // Index-1->next = ToAdd;
    // ToAdd->next = Index
    struct List_Node* Before = List_GetElement(L,Index-1);
    struct List_Node* After = List_GetElement(L,Index);

    Before->Next = ToAdd;
    ToAdd->Next = After;
    }
    else {
        ToAdd->Next = L->First;
        L->First = ToAdd;
    }
    L->length++;
}
void List_DeleteElement(List* L, int Index) {
    if (Index < 0 || Index >= L->length) return;

    struct List_Node* ToDelete;
    if (Index == 0) {
        ToDelete = L->First;
        L->First = ToDelete->Next;
    } else {
        struct List_Node* Before = List_GetElement(L, Index - 1);
        ToDelete = Before->Next;
        Before->Next = ToDelete->Next;
    }
    if (ToDelete->data) free(ToDelete->data);
    free(ToDelete);
    L->length--;
}
int CMP_int(void* AA, void* BA) {
    // If A>B; 1 | If A==B; 0 | If A<B; -1
    int A = *((int*)AA);
    int B = *((int*)BA);
    if (A==B) {
        return 0;
    }
    return ((A-B)/abs(A-B));

}
List List_FindInstances(List* L, void* Value,CompareFunction cmp) {
    // Finds instances of an element in a list, and appends them to another list.
    List Instances = NewList();
    for (int i = 0; i < L->length; i++) {
        struct List_Node* LN = List_GetElement(L,i);
        if (cmp(LN->data, Value) == 0) {
            int* ToBeAdded = malloc(sizeof(int));
            *ToBeAdded = i;
            void* PtrToAdded = ToBeAdded;
            List_AppendElement(&Instances, PtrToAdded);
        }
    }
    return Instances;
}
void List_Print(List* L, int Case) {
    for (int i = 0; i < L->length;i++) {
        switch (Case) {
            case INT: {
                printf("%d\n", *((int*)List_GetElement(L,i)->data));
            }
        }
    }
}
void* List_VdP(int i) {
    int* k = malloc(sizeof(int));
    *k = i;
    return k;
}
void List_Free_List(List* L) {
    for (int i = L->length-1; i >= 0; i--) {
        struct List_Node* LN = List_GetElement(L,i);
        free(LN->data);
        free(LN);
    }
    L->First = NULL;
    L->length = 0;
}
void List_ForEach(List* L, void (*func)(void*)) {
    // takes in a list and does a function on each element.
    for (int i = 0; i < L->length; i++) {
        func(List_GetElement(L,i)->data);
    }

}
Line NewLine(int Type) {
    Line Ln;
    Ln.Entries = NewList();
    Ln.Type = Type;
    return Ln;
}
void Line_Push(Line* Ln, void* v) {
    if (Ln->Type == STACK) {
        List_InsertElement(&Ln->Entries,0,v);
    }
    else if (Ln->Type == QUEUE) {
        List_AppendElement(&Ln->Entries,v);
    }
    
}
void* Line_Pop(Line* Ln) {
    if (Ln->Entries.length == 0) {
        return NULL;
    }
    void* ToReturn;
    if (Ln->Type == STACK) {
        ToReturn = List_GetElement(&Ln->Entries,Ln->Entries.length-1)->data;
        List_DeleteElement(&Ln->Entries,Ln->Entries.length-1);
    }
    else if (Ln->Type == QUEUE)
    {
        ToReturn = List_GetElement(&Ln->Entries,0)->data;
        List_DeleteElement(&Ln->Entries,0);
    }
    return ToReturn;
}
void* Line_Peek(Line* Ln) {
    if (Ln->Type == STACK) {
        return Ln->Entries.First->data;
    }
    else if (Ln->Type == QUEUE)
    {
        return List_GetElement(&Ln->Entries, -1)->data;
    }
    return NULL;
}
Array NewArray(int len, int elementSize) {
    // so this allocates memory to a new array of size len
    Array A;
    A.Length = len;
    A.ElementSize = elementSize;
    A.Data = malloc(elementSize*len); // allocate memory;
    return A;
}
void Array_Set(Array* A, int index, void* value) {
    if (index < 0 || index >= A->Length) {
        printf("Out of Bounds Error\n");
        return;
    }
    void* dest = (char*)A->Data + index * A->ElementSize;  
    memcpy(dest, value, A->ElementSize);
}
void* Array_Get(Array* A, int index) {
    if (!A || A->Length == 0 || index > A->Length) {
        printf("Out of bounds error");
        return NULL;
    }
    return (char*)A->Data + index * A->ElementSize;
}
void Array_Free(Array* A) {
    if (!A) return;
    free(A->Data);
    A->Data = NULL;
    A->Length = 0;
    A->ElementSize = 0;
}

Hashmap NewMap(int Bucket_Amt, int (*hash)(void* v), int (*comp)(void* a, void* b)) {
    Hashmap HM;
    HM.Buckets = NewArray(Bucket_Amt, sizeof(List));
    for (int i = 0; i < HM.Buckets.Length; i++) {
        List* L = malloc(sizeof(List));
        *L = NewList();
        Array_Set(&HM.Buckets,i,L);
    }
    HM.hash = hash;
    HM.comp = comp;
    return HM; 
}
List* HM_GetKeys(Hashmap* HM) {
    List* ToReturn = malloc(sizeof(List));
    *ToReturn = NewList();
    for (int i = 0; i < HM->Buckets.Length; i++) {
        List* BucketEntries = (List*)Array_Get(&HM->Buckets, i);
        for (int j = 0; j < BucketEntries->length; j++) {
            List_AppendElement(ToReturn, ((Hashmap_Entry*)List_GetElement(BucketEntries, j)->data)->Key);
        }
    }
    return ToReturn;
}
void* Map_GetKey(Hashmap* HM, void* key) {
    int index = HM->hash(key) % HM->Buckets.Length;
    void* BucketFound = Array_Get(&HM->Buckets,index);
    List *N = (List*)BucketFound;
    for (int i = 0; i < N->length; i++) {
        Hashmap_Entry* entry = (Hashmap_Entry*)List_GetElement(N, i)->data;
        if (HM->comp(entry->Key, key) == 0) {
             return entry->Value;
            }
        }
    
    return NULL;

}
void Map_SetKey(Hashmap* HM, void* key, void* value) {
    
    int index = HM->hash(key) % HM->Buckets.Length;
    void* BucketFound = Array_Get(&HM->Buckets,index);
    List *N = (List*)BucketFound;
    Hashmap_Entry ToSet = {key, value};
    bool Found = false;
    for (int i = 0; i < N->length; i++) {
        if ( ((Hashmap_Entry*)List_GetElement(N,i)->data)->Key == key) {
            ((Hashmap_Entry*)(List_GetElement(N,i)->data))->Value = value;
            Found = true;       
        }
    }
    if (!Found) {
        Hashmap_Entry* HE = malloc(sizeof(Hashmap_Entry));
        HE->Key = key;
        HE->Value = value;
        List_AppendElement(N,HE);
    }
    
}
void DeleteKey(Hashmap* HM, void* key) {
    int index = HM->hash(key) % HM->Buckets.Length;
    void* BucketFound = Array_Get(&HM->Buckets,index);
    List *N = (List*)BucketFound;

    List M = List_FindInstances(N,key,HM->comp);
    if (M.length == 0) {
        return;
    }
    else {
        int IndexToDelete = *((int*)M.First->data);
        List_DeleteElement(N, IndexToDelete);
    }

}
int BasicHash(void* s1) {
    string* s = (string*)s1;
    int base = 0;
    for (int i = 0; i < s->length; i++) {
        base += (s->Content[i]%10)*pow(10,i);
    }
    return base;
}
void Map_Free(Hashmap *HM) {
    if (!HM) return;

    // Iterate through each bucket in the array
    for (int i = 0; i < HM->Buckets.Length; ++i) {
        List* bucket = (List*)Array_Get(&HM->Buckets, i);
        if (!bucket) continue;

        // Walk the nodes and free each entry/key/value
        struct List_Node* cur = bucket->First;
        while (cur) {
            Hashmap_Entry* e = (Hashmap_Entry*)cur->data;
            if (e) {
                free(e->Key);     // keys from strdup
                free(e->Value);   // values from malloc
                free(e);          // entry itself
            }
            cur = cur->Next;
        }

        // Now free the list nodes and clear the list
        List_Free_List(bucket);  // frees nodes, resets .First and .length
    }

    // Finally, free the array of buckets itself
    Array_Free(&HM->Buckets);
    HM->Buckets.Data = NULL;
    HM->Buckets.Length = 0;
    HM->Buckets.ElementSize = 0;
}