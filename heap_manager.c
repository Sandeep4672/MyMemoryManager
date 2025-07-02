#include <stdio.h>
#include <memory.h>
#include <unistd.h>     
#include <sys/mman.h>   
#include "heap_manager.h"
#include<stdlib.h>
#include <assert.h>


static size_t SYSTEM_PAGE_SIZE = 0;
static sys_page *first_sys_page = NULL;

void initialization(){
    SYSTEM_PAGE_SIZE = getpagesize();
}

static void * get_new_page(int units){

    char *newPage = mmap(0,units * SYSTEM_PAGE_SIZE,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_ANON|MAP_PRIVATE,0, 0);

    if(newPage == MAP_FAILED){
        printf("Error : VM Page allocation Failed\n");
        return NULL;
    }
    memset(newPage, 0, units * SYSTEM_PAGE_SIZE);
    return (void *)newPage;
}

static void return_a_page(void *page, int units){

    if(munmap(page, units * SYSTEM_PAGE_SIZE)){
        printf("Error : Could not munmap page to kernel");
    }
}

struct_info * lookup_struct_info(char *struct_name){
    sys_page* curr_sys_page=first_sys_page;
    struct_info* curr_struct_info=NULL;
    while(curr_sys_page){
        ITERATE_STRUCT_INFO_BEGIN(curr_sys_page,curr_struct_info){
            if(strncmp(curr_struct_info->struct_name,struct_name,MM_MAX_STRUCT_NAME)==0)
            return curr_struct_info;
        }ITERATE_STRUCT_INFO_END
        curr_sys_page=curr_sys_page->next;
    }
    return NULL;
}

void register_struct_info(char *struct_name, unsigned int struct_size) {
    struct_info *curr_struct_info = NULL;
    sys_page *new_sys_page = NULL;

    if (struct_size > SYSTEM_PAGE_SIZE) {
        printf("Error: %s() Structure %s size exceeds system page size\n",
               __FUNCTION__, struct_name);
        return;
    }

    if (lookup_struct_info(struct_name)) {
        printf("Error: Structure %s is already registered\n", struct_name);
        return;
    }

    if (!first_sys_page) {
        first_sys_page = (sys_page *)get_new_page(1);
        first_sys_page->next = NULL;

        curr_struct_info = &first_sys_page->struct_info_array[0];
        strncpy(curr_struct_info->struct_name, struct_name, MM_MAX_STRUCT_NAME);
        curr_struct_info->struct_size = struct_size;
        curr_struct_info->first_page = NULL;
        init(&curr_struct_info->priority_free_block_head);
        return;
    }

    unsigned count = 0;
    ITERATE_STRUCT_INFO_BEGIN(first_sys_page, curr_struct_info) {
        count++;
    } ITERATE_STRUCT_INFO_END;

    if (count == MAX_STRUCT_INFO_PER_SYSTEM_PAGE) {
        new_sys_page = (sys_page *)get_new_page(1);
        new_sys_page->next = first_sys_page;
        first_sys_page = new_sys_page;

        curr_struct_info = &new_sys_page->struct_info_array[0];
    }

    strncpy(curr_struct_info->struct_name, struct_name, MM_MAX_STRUCT_NAME);
    curr_struct_info->struct_size = struct_size;
    curr_struct_info->first_page = NULL;
    init(&curr_struct_info->priority_free_block_head);
}

void print_registered_structs() {
    struct_info *curr_struct_info = NULL;
    sys_page *curr_sys_page = first_sys_page;
    unsigned count = 0;

    printf("\nRegistered Structures:\n");
    printf("%-4s %-32s %-10s\n", "No.", "Struct Name", "Size (bytes)");
    printf("--------------------------------------------------------\n");

    while (curr_sys_page) {
        ITERATE_STRUCT_INFO_BEGIN(curr_sys_page, curr_struct_info) {
            printf("%-4u %-32s %-10u\n",
                   ++count,
                   curr_struct_info->struct_name,
                   curr_struct_info->struct_size);
        } ITERATE_STRUCT_INFO_END;
        curr_sys_page = curr_sys_page->next;
    }

    if (count == 0) {
        printf("No structures registered yet.\n");
    }

    printf("--------------------------------------------------------\n");
}


void bind_allocated_and_free_blocks(block_meta *allocated_block, block_meta *free_block) {
    free_block->prev_block = allocated_block;
    free_block->next_block = allocated_block->next_block;
    allocated_block->next_block = free_block;

    if (free_block->next_block) {
        free_block->next_block->prev_block = free_block;
    }
}


static void merge_free_blocks(block_meta *first, block_meta *second) {
    assert(first->is_free == true && second->is_free == true);

    first->block_size += sizeof(block_meta) + second->block_size;
    first->next_block = second->next_block;

    if (second->next_block) {
        second->next_block->prev_block = first;
    }
}

static inline unsigned int get_max_allocatable_bytes(int units) {
    return (unsigned int)(
        (SYSTEM_PAGE_SIZE * units) - offsetof(vm_page, page_memory)
    );
}


bool is_vm_page_empty(vm_page *page) {
    return  page->block_meta_data.next_block == NULL && page->block_meta_data.prev_block == NULL && page->block_meta_data.is_free == true;
}

void mark_vm_page_empty(vm_page *page) {
    page->block_meta_data.next_block = NULL;
    page->block_meta_data.prev_block = NULL;
    page->block_meta_data.is_free = true;
}

vm_page *allocate_vm_page(struct_info *info) {
    vm_page *page = get_new_page(1);

    if (!page) return NULL;

    mark_vm_page_empty(page);
    page->block_meta_data.block_size = get_max_allocatable_bytes(1);
    page->block_meta_data.offset = offsetof(vm_page, block_meta_data);
    init(&page->block_meta_data.priority_node);

    page->next = NULL;
    page->prev = NULL;

    page->struct_info = info;

    if (!info->first_page) {
        info->first_page = page;
        return page;
    }

    page->next = info->first_page;
    info->first_page->prev = page;
    info->first_page = page;

    return page;
}

void delete_vm_page(vm_page *page) {
    struct_info *info = page->struct_info;

    if (info->first_page == page) {
        info->first_page = page->next;
        if (page->next)
            page->next->prev = NULL;

        page->next = NULL;
        page->prev = NULL;
        return_a_page((void *)page, 1);
        return;
    }

    if (page->next)
        page->next->prev = page->prev;

    if (page->prev)
        page->prev->next = page->next;

    page->next = NULL;
    page->prev = NULL;
    return_a_page((void *)page, 1);
}

void print_vm_page_details(vm_page* page) {
    printf("\n\t[Page Details]\n");
    printf("\t  Address      : %p\n", (void*)page);
    printf("\t  Next         : %p\n", (void*)page->next);
    printf("\t  Prev         : %p\n", (void*)page->prev);
    printf("\t  Struct Name  : %s\n", page->struct_info->struct_name);

    block_meta* curr = NULL;
    unsigned int index = 0;

    ITERATE_META_BLOCKS_BEGIN(page, curr) {
        printf("\t\t%-14p | Block %-2u | %-9s | Size: %-5u | Offset: %-5u | Prev: %-14p | Next: %-14p\n",
               (void*)curr,
               index++,
               curr->is_free ? "FREE" : "ALLOCATED",
               curr->block_size,
               curr->offset,
               (void*)curr->prev_block,
               (void*)curr->next_block);
    } ITERATE_META_BLOCKS_END;
}


static int compare_free_blocks(void* block1, void* block2) {
    block_meta* b1 = (block_meta*)block1;
    block_meta* b2 = (block_meta*)block2;

    if (b1->block_size > b2->block_size)
        return -1;
    else if (b1->block_size < b2->block_size)
        return 1;

    return 0;
}

static void add_free_block_to_priority_list(struct_info* info, block_meta* free_block) {
    assert(free_block->is_free == true);

    priority_insert(
        &info->priority_free_block_head,
        &free_block->priority_node,
        compare_free_blocks,
        offsetof(block_meta, priority_node)
    );
}

// void *xcalloc(char *struct_name, int units) {
//     struct_info *info = lookup_struct_info(struct_name);

//     if (!info) {
//         printf("Error: Structure %s not registered with memory manager\n", struct_name);
//         return NULL;
//     }

//     if (units * info->struct_size > MAX_PAGE_ALLOCATABLE_MEMORY(1)) {
//         printf("Error: Requested memory exceeds system page size\n");
//         return NULL;
//     }

//     block_meta *free_block = allocate_free_block(info, units * info->struct_size);

//     if (free_block) {
//         memset((char *)(free_block + 1), 0, free_block->block_size);
//         return (void *)(free_block + 1);
//     }

//     return NULL;
// }
