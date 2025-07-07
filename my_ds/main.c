#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dll.h"
#include <stddef.h>

typedef struct student_ {
    char name[32];
    int marks;
    Node list_node; 
} student_t;

int student_comparator(void* a, void* b) {
    student_t* s1 = (student_t*)a;
    student_t* s2 = (student_t*)b;

    // Higher marks = higher priority
    if (s1->marks > s2->marks) return -1;
    if (s1->marks < s2->marks) return 1;
    return 0;
}

// Helper to print the student list
void print_students(Node* head) {
    Node* curr = head->next;
    printf("Student list [sorted by marks]:\n");

    ITERATE_LIST_BEGIN(curr) {
        student_t* student = (student_t*)NODE_TO_CONTAINER_GIVEN_OFFSET(curr, offsetof(student_t, list_node));
        printf("Name: %-10s | Marks: %d\n", student->name, student->marks);
    } ITERATE_LIST_END();
}

int main() {
    // Sentinel head
    Node head;
    init(&head);

    // Create students
    student_t* s1 = malloc(sizeof(student_t));
    strcpy(s1->name, "Alice");
    s1->marks = 85;

    student_t* s2 = malloc(sizeof(student_t));
    strcpy(s2->name, "Bob");
    s2->marks = 92;

    student_t* s3 = malloc(sizeof(student_t));
    strcpy(s3->name, "Charlie");
    s3->marks = 78;

    student_t* s4 = malloc(sizeof(student_t));
    strcpy(s4->name, "Diana");
    s4->marks = 92;
    printf("%lu \n",offsetof(student_t, list_node));
    // Insert using priority
    priority_insert(&head, &s1->list_node, student_comparator, offsetof(student_t, list_node));
    priority_insert(&head, &s2->list_node, student_comparator, offsetof(student_t, list_node));
    priority_insert(&head, &s3->list_node, student_comparator, offsetof(student_t, list_node));
    priority_insert(&head, &s4->list_node, student_comparator, offsetof(student_t, list_node));

    print_students(&head);

    delete_dll(&head);


    free(s1);
    free(s2);
    free(s3);
    free(s4);

    return 0;
}