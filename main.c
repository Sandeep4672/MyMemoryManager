#include "ulib.h"
#include <stdio.h>

typedef struct emp_ {

    char name[32];
    unsigned int emp_id;
} emp_t;

typedef struct student_ {

    char name[32];
   unsigned int rollno;
    unsigned int marks_phys;
    unsigned int marks_chem;
    unsigned int marks_maths;
    struct student_ *next;
} student_t;

int main(int argc, char **argv) {
    int wait;
    initialization();
    MM_REG_STRUCT(emp_t);
    MM_REG_STRUCT(student_t);
    print_registered_structs();

    emp_t *emp1 = (emp_t *) xcalloc("emp_t", 1);
    emp_t *emp2 = (emp_t *) xcalloc("emp_t", 1);
    emp_t *emp3 = (emp_t *) xcalloc("emp_t", 1);

    student_t *stud1 = (student_t *) xcalloc("student_t", 1);
    student_t *stud2 = (student_t *) xcalloc("student_t", 1);

    // do something with the data...
    // printf(" \nSCENARIO 1 : *********** \n");
    // mm_print_memory_usage(0);
    // mm_print_block_usage();


    // scanf("%d", &wait); 

    // XFREE(emp1);
    // XFREE(emp3);
    // XFREE(stud2);
    // printf(" \nSCENARIO 2 : *********** \n");
    // mm_print_memory_usage(0);
    // mm_print_block_usage();


    // scanf("%d", &wait); 
    
    // XFREE(emp2);
    // XFREE(stud1);
    // printf(" \nSCENARIO 3 : *********** \n");
    // mm_print_memory_usage(0);
    // mm_print_block_usage();
    print_block_usage();
    return 0;
}