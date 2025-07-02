#ifndef __GLUETHREAD__
#define __GLUETHREAD__

typedef struct Node{

    struct Node *next;
    struct Node *prev;
} Node;

void init(Node* node);
void add_after(Node *curr, Node *newNode);
void add_before(Node* curr,Node* newNode);
void remove_node(Node* curr);
void delete_dll(Node* head);
void add_last(Node* head,Node* newNode);
unsigned int dll_size(Node* head);
void priority_insert(Node* head,Node* newNode,int(*comp)(void*,void*),int offset);

#define LIST_EMPTY(node)         \
    (!node->next && !node->prev)

#define NODE_TO_CONTAINER_GIVEN_OFFSET(node, offset)  \
    (void *)((char *)(node) - offset)



#define ITERATE_LIST_BEGIN(node)         \
{                                        \
    for (; node != NULL; node = node->next) {

#define ITERATE_LIST_END()              \
    }                                   \
}



#endif