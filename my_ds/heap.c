#include "heap.h"
#include<stdio.h>
#include<stdlib.h>
void heap_init(Heap *heap, int (*cmp)(void *, void *)) {
    dll_init_node(&heap->head);
    heap->cmp = cmp;
}

void heap_insert(Heap *heap, Node *newNode) {
    priority_insert(&heap->head, newNode, heap->cmp);
    if(heap->head.next==newNode)
    printf("Correct here \n");
    if(newNode->prev==&heap->head)
    printf("Also correct \n");
}

Node* heap_extract_max(Heap *heap) {
    Node *max_node = heap->head.next;
    if (!max_node) return NULL;
        printf("%p is the max node \n",max_node);
    if(max_node->prev==NULL)
    printf("Fuck you \n");
    remove_node(max_node);
    if(heap->head.next==max_node)
    printf("I fucking got the problem \n");
    if(max_node->prev==NULL)
    printf("Oh fuck the second problem \n");

    return max_node;
}

int heap_is_empty(Heap *heap) {
    Node* node=&(heap->head);
    return LIST_EMPTY(node);
}
