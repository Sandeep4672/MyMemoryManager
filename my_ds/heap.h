#pragma once
#include "dll.h"

typedef struct Heap {
    Node head;  
    int (*cmp)(void *, void *); 
} Heap;

void heap_init(Heap *heap, int (*cmp)(void *, void *));
void heap_insert(Heap *heap, Node *newNode);
Node* heap_extract_max(Heap *heap);
int heap_is_empty(Heap *heap);
