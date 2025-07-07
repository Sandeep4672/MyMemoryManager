#include "dll.h"
#include <stdlib.h>
#include<stdio.h>
void dll_init_node(Node* node){
    if(!node)return;
    node->next=NULL;
    node->prev=NULL;
}

void add_after(Node* curr,Node* newNode){
     if(!curr->next){
        curr->next = newNode;
        newNode->prev = curr;
        return;
    }

    newNode->next=curr->next;
    newNode->next->prev=newNode;
    newNode->prev=curr;
    curr->next=newNode;
}

void add_before(Node* curr, Node* newNode) {
    if (!curr || !newNode) return;

    newNode->next = curr;
    newNode->prev = curr->prev;

    if (curr->prev)
        curr->prev->next = newNode;
    curr->prev = newNode;
}


void remove_node(Node* curr) {
    if (!curr) return;

    if (curr->prev)
        curr->prev->next = curr->next;
    if (curr->next)
        curr->next->prev = curr->prev;

    curr->prev = NULL;
    curr->next = NULL;
}


void add_last(Node* head, Node* newNode){
    Node* temp = head->next;
    Node* last = NULL;

    ITERATE_LIST_BEGIN(temp){
        last = temp;
    } ITERATE_LIST_END()

    if (!last){  
        head->next = newNode;
        newNode->prev = head;
    }
    else {
        last->next = newNode;
        newNode->prev = last;
    }
}

unsigned int dll_size(const Node* head) {
    unsigned int count = 0;
    Node *temp = head->next;
    ITERATE_LIST_BEGIN(temp) {
        count++;
    } ITERATE_LIST_END();

    return count;
}

void delete_dll(Node* head){
    Node* curr = head->next;
    Node* next = NULL;

    while (curr != NULL) {
        next = curr->next;
        curr->prev = NULL;
        curr->next = NULL;
        curr = next;
    }

    head->next = NULL;
}


void priority_insert(Node* head, Node* newNode, int (*comp)(void*, void*)) {
    dll_init_node(newNode); 

    Node* curr = head->next;

    if (!curr) {
        add_after(head, newNode);
        return;
    }

    while (curr) {
        if (comp(newNode, curr) == -1) {
            add_before(curr, newNode);
            return;
        }
        curr = curr->next;
    }

    Node* last = head;
    while (last->next) {
        last = last->next;
    }
    add_after(last, newNode);
}