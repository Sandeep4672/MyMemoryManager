#include <stdio.h>
#include <string.h>
#include "ulib.h"  

typedef struct student {
    char name[32];
    int age;
    float gpa;
} student_t;

int main() {
    INIT();
    MM_REG_STRUCT(student_t);
    print_registered_structs();
    
    return 0;
}