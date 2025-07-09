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

    //student_t *stud1 = (student_t *) xcalloc("student_t", 1);
    //student_t *stud2 = (student_t *) xcalloc("student_t", 1);

    print_block_usage();

    xfree(emp1);
    print_memory_usage("emp_t");
    xfree(emp2);
    print_memory_usage("emp_t");
    xfree(emp3);
    //print_block_usage();
    print_memory_usage("emp_t");
    return 0;
}