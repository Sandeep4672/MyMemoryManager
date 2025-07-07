#pragma once

typedef struct Node{

    struct Node *next;
    struct Node *prev;
} Node;


#define offsetof(container, field)  \
    ((size_t)&(((container *)0)->field))

void dll_init_node(Node* node);
void add_after(Node *curr, Node *newNode);
void add_before(Node* curr,Node* newNode);
void remove_node(Node* curr);
void delete_dll(Node* head);
void add_last(Node* head,Node* newNode);
unsigned int dll_size(const Node* head);
void priority_insert(Node* head,Node* newNode,int(*comp)(void*,void*));

#define LIST_EMPTY(node)         \
    (!node->next && !node->prev)

#define NODE_TO_STRUCT(node, type, field) \
    ((type *)((char *)(node) - offsetof(type, field)))



#define ITERATE_LIST_BEGIN(node)         \
{                                        \
    for (; node != NULL; node = node->next) {

#define ITERATE_LIST_END()              \
    }                                   \
}

