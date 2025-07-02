#ifndef __HEAP__
#define __HEAP__

#include "my_dll/dll.h"
#include <stdbool.h>

#define offsetof(container, field)  \
    ((size_t)&(((container *)0)->field))


struct struct_info;


typedef struct block_meta {
    bool is_free;
    unsigned block_size;
    unsigned offset;
    Node priority_node; 
    struct block_meta *prev_block;
    struct block_meta *next_block;
} block_meta;

typedef struct vm_page {
    struct vm_page *next;
    struct vm_page *prev;
    struct struct_info *struct_info;  
    block_meta block_meta_data;    
    char page_memory[0];           
} vm_page;

#define MM_MAX_STRUCT_NAME 32

typedef struct struct_info {
    char struct_name[MM_MAX_STRUCT_NAME];
    unsigned struct_size;
    vm_page *first_page;
    Node priority_free_block_head;  
} struct_info;

typedef struct sys_page{
    struct sys_page *next;
    struct_info struct_info_array[0];
} sys_page;

#define MAX_STRUCT_INFO_PER_SYSTEM_PAGE   \
    ((SYSTEM_PAGE_SIZE - sizeof(sys_page *)) / sizeof(struct_info))

#define ITERATE_STRUCT_INFO_BEGIN(sys_page_ptr, curr)                             \
    for ((curr) = (sys_page_ptr)->struct_info_array;                              \
         (curr)->struct_size != 0;                                                \
         (curr)++) {

#define ITERATE_STRUCT_INFO_END }


#define ITERATE_VM_PAGES_BEGIN(info,curr){\
    for(curr=info->firstPage;curr;curr=curr->next){

#define ITERATE_VM_PAGES_END }}


#define ITERATE_META_BLOCKS_BEGIN(page,curr){\
    for(curr=&page->block_meta_data;curr;curr=curr->next_block){

#define ITERATE_META_BLOCKS_END }}



void bind_allocated_and_free_blocks(block_meta *allocated_block, block_meta *free_block);
void mark_vm_page_empty(vm_page *page) ;

#endif
