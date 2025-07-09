#include "heap.h"
#include<stdio.h>
#include<stdlib.h>
void heap_init(Heap *heap, int (*cmp)(void *, void *)) {
    dll_init_node(&heap->head);
    heap->cmp = cmp;
}

void heap_insert(Heap *heap, Node *newNode) {
    priority_insert(&heap->head, newNode, heap->cmp);
    
}

Node* heap_extract_max(Heap *heap) {
    Node *max_node = heap->head.next;
    if (!max_node) return NULL;
    remove_node(max_node);

    return max_node;
}

int heap_is_empty(Heap *heap) {
    Node* node=&(heap->head);
    return LIST_EMPTY(node);
}
