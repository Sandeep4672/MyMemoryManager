#ifndef __ULIB_H__
#define __ULIB_H__

void initialization();
void register_struct_info(char*,unsigned int);
void print_registered_structs();
void* xcalloc(char*,int);
void xfree(void*);
void print_block_usage();
void print_memory_usage(const char* );
#define INIT()\
    initialization()

#define MM_REG_STRUCT(struct_name)  \
    (register_struct_info(#struct_name, sizeof(struct_name)))

#endif
