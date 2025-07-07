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
    if (!struct_name) return NULL;
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

static int block_meta_comparator(void* node1, void* node2) {
    block_meta* b1 = NODE_TO_STRUCT(node1, block_meta, priority_node);
    block_meta* b2 = NODE_TO_STRUCT(node2, block_meta, priority_node);

    if (b1->block_size > b2->block_size)
        return -1; // max-heap: bigger block = higher priority
    else if (b1->block_size < b2->block_size)
        return 1;

    return 0;
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
    } else {
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
    }

    strncpy(curr_struct_info->struct_name, struct_name, MM_MAX_STRUCT_NAME - 1);
    curr_struct_info->struct_name[MM_MAX_STRUCT_NAME - 1] = '\0';  // Ensure null-termination
    curr_struct_info->struct_size = struct_size;
    curr_struct_info->first_page = NULL;

    heap_init(&curr_struct_info->free_blocks_heap, block_meta_comparator);
}


void print_registered_structs() {
    struct_info *curr_struct_info = NULL;
    sys_page *curr_sys_page = first_sys_page;
    unsigned count = 0;

    if (!first_sys_page) {
        printf("\nNo structures registered yet.\n");
        return;
    }

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
    // Link free_block after allocated_block
    free_block->prev_block = allocated_block;
    free_block->next_block = allocated_block->next_block;

    // Reconnect next block (if it exists) to point back to free_block
    if (free_block->next_block) {
        free_block->next_block->prev_block = free_block;
    }

    allocated_block->next_block = free_block;

    // Mark it free and clear any dangling priority pointers
    free_block->is_free = true;
    dll_init_node(&allocated_block->priority_node);
}




static void merge_free_blocks(block_meta *first, block_meta *second) {
    assert(first->is_free == true && second->is_free == true);

    // Remove second's node from heap if it’s there (optional, depending on context)
    dll_init_node(&second->priority_node);  // Always clean the node

    // Merge block sizes (include second’s metadata overhead)
    first->block_size += sizeof(block_meta) + second->block_size;

    // Update linkages
    first->next_block = second->next_block;
    if (second->next_block) {
        second->next_block->prev_block = first;
    }
}


static inline unsigned int get_max_allocatable_bytes(int units) {
    if (units <= 0) return 0;

    // Returns max usable bytes in `units` system pages, subtracting metadata size
    return (unsigned int)(
        (SYSTEM_PAGE_SIZE * units) - offsetof(vm_page, page_memory)
    );
}



bool is_vm_page_empty(vm_page *page) {
    block_meta *block = &page->block_meta_data;
    return block->is_free &&
           block->next_block == NULL &&
           block->prev_block == NULL &&
           block->block_size == get_max_allocatable_bytes(1);
}


void mark_vm_page_empty(vm_page *page) {
    block_meta *block = &page->block_meta_data;

    block->next_block = NULL;
    block->prev_block = NULL;
    block->is_free = true;
    block->block_size = get_max_allocatable_bytes(1);

    dll_init_node(&block->priority_node);
}


vm_page *allocate_vm_page(struct_info *info) {
    vm_page *page = get_new_page(1);
    if (!page) return NULL;

    mark_vm_page_empty(page);  // sets is_free, block_size, links, and cleans node

    page->block_meta_data.offset = offsetof(vm_page, block_meta_data);

    page->next = NULL;
    page->prev = NULL;
    page->struct_info = info;

    if (!info->first_page) {
        printf("This is my first page \n");
        info->first_page = page;
    } else {
        page->next = info->first_page;
        info->first_page->prev = page;
        info->first_page = page;
    }

    return page;
}

void delete_vm_page(vm_page *page) {
    if (!page || !page->struct_info) return;

    struct_info *info = page->struct_info;

    // Unlink from struct_info's vm_page list
    if (info->first_page == page) {
        info->first_page = page->next;
        if (page->next)
            page->next->prev = NULL;
    } else {
        if (page->prev)
            page->prev->next = page->next;
        if (page->next)
            page->next->prev = page->prev;
    }

    // Clear link fields
    page->next = NULL;
    page->prev = NULL;
    page->struct_info = NULL;

    // Optional: clean block node (defensive)
    dll_init_node(&page->block_meta_data.priority_node);

    return_a_page((void *)page, 1);
}


void print_vm_page_details(vm_page* page) {
    if (!page || !page->struct_info) {
        printf("Invalid page or uninitialized struct_info\n");
        return;
    }

    printf("\n\t[Page Details]\n");
    printf("\t  Address      : %p\n", (void*)page);
    printf("\t  Next         : %p\n", (void*)page->next);
    printf("\t  Prev         : %p\n", (void*)page->prev);
    printf("\t  Struct Name  : %s\n", page->struct_info->struct_name);
    printf("\t  Struct Size  : %u bytes\n", page->struct_info->struct_size);

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

    printf("\t  Total Blocks : %u\n", index);
}

static void add_free_block_to_priority_list(struct_info* info, block_meta* free_block) {
    assert(free_block->is_free == true);
    printf("%u %p Priority List \n",free_block->block_size,&free_block->priority_node);
    heap_insert(&info->free_blocks_heap,&free_block->priority_node);
}

static vm_page *
struct_info_add_new_page(struct_info *info){

    vm_page *vm_page = allocate_vm_page(info);

    if(!vm_page)
        return NULL;

    /* The new page is like one free block, add it to the
     * free block list*/
    add_free_block_to_priority_list(
            info, &vm_page->block_meta_data);

    return vm_page;
}


block_meta *
get_biggest_free_block_meta(
        struct_info *info){
    Node *biggest_free_block =
        (info->free_blocks_heap).head.next;
    if(!biggest_free_block)
    printf("It is NULL \n");
    else
    printf("%p \n",biggest_free_block);
    if(biggest_free_block)
        return (block_meta*)NODE_TO_STRUCT(biggest_free_block,block_meta,priority_node);

    return NULL;
}

void print_priority_queue_of_struct_info(struct_info*);

static bool
split_free_data_block_for_allocation(
    struct_info* info,
    block_meta *curr_block_meta, 
    unsigned int size)
{
    block_meta *next_block_meta = NULL;

    assert(curr_block_meta->is_free == true);

    if (curr_block_meta->block_size < size) {
        return false;
    }

    unsigned remaining_size = curr_block_meta->block_size - size;

    curr_block_meta->is_free = false;
    curr_block_meta->block_size = size;
    Node* node=heap_extract_max(&info->free_blocks_heap);
    assert(node==&curr_block_meta->priority_node);
    printf("After extraction \n");
    print_priority_queue_of_struct_info(info);
    // curr_block_meta->offset stays the same

    if (remaining_size == 0) {
        // Case 1: Perfect fit, no split
        return true;
    }

    if (remaining_size < sizeof(block_meta)) {
        // Case 3: Hard internal fragmentation
        // Remaining space too small to form new block
        return true;
    }

    // Create next free block (for both soft and full split cases)

    next_block_meta = (block_meta *)((char *)curr_block_meta + sizeof(block_meta) + size);
    next_block_meta->is_free = true;
    next_block_meta->block_size = remaining_size - sizeof(block_meta);
    printf("%d is the size of new meta block\n with address %p \n",next_block_meta->block_size,next_block_meta);
    next_block_meta->offset = curr_block_meta->offset + sizeof(block_meta) + size;

    dll_init_node(&next_block_meta->priority_node);
    add_free_block_to_priority_list(info, next_block_meta);
    printf("After adding to priority list check: \n");
    assert(&info->free_blocks_heap.head==next_block_meta->priority_node.prev);

    print_priority_queue_of_struct_info(info);
    bind_allocated_and_free_blocks(curr_block_meta, next_block_meta);
    printf("After binding check \n");
    assert(&info->free_blocks_heap.head==next_block_meta->priority_node.prev);
    print_priority_queue_of_struct_info(info);
    return true;
}


static block_meta *
allocate_free_data_block(
        struct_info *info,
        unsigned int req_size){
    
    bool status = false;
    vm_page *page = NULL;
    block_meta *curr_block_meta = NULL;

    block_meta *biggest_block_meta =
        get_biggest_free_block_meta(info);
    if(!biggest_block_meta ||
            biggest_block_meta->block_size < req_size){

        page = struct_info_add_new_page(info);
        print_priority_queue_of_struct_info(info);
        printf("%u \n",page->block_meta_data.block_size);
        /*Allocate the free block from this page now*/
        status = split_free_data_block_for_allocation(info,
                &page->block_meta_data, req_size);

        if(status)
            return &page->block_meta_data;

        return NULL;
    }
    /*The biggest block meta data can satisfy the request*/
    if(biggest_block_meta){
        printf("The biggest meta block is: %p \n",biggest_block_meta);
        status = split_free_data_block_for_allocation(info,
                biggest_block_meta, req_size);
    }

    if(status)
        return biggest_block_meta;

    return NULL;
}

void print_meta_blocks_for_struct_info(struct_info *info) {
    if (!info) {
        printf("Invalid struct_info pointer\n");
        return;
    }

    vm_page *page = info->first_page;
    int page_no = 0;

    if (!page) {
        printf("No VM pages allocated for struct: %s\n", info->struct_name);
        return;
    }

    printf("\n[Meta Blocks for struct: %s]\n", info->struct_name);
    printf("Struct Size: %u bytes\n", info->struct_size);
    printf("-------------------------------------------------------------\n");

    while (page) {
        printf("\nPage %d - Address: %p\n", page_no++, (void *)page);

        block_meta *curr = NULL;
        int block_index = 0;

        ITERATE_META_BLOCKS_BEGIN(page, curr) {
            printf("  Block %-2d | %-14p | %-9s | Size: %-5u | Offset: %-5u | Prev: %-14p | Next: %-14p\n",
                   block_index++,
                   (void *)curr,
                   curr->is_free ? "FREE" : "ALLOCATED",
                   curr->block_size,
                   curr->offset,
                   (void *)curr->prev_block,
                   (void *)curr->next_block);
        } ITERATE_META_BLOCKS_END;

        if (block_index == 0) {
            printf("  No meta blocks on this page\n");
        }

        page = page->next;
    }

    printf("-------------------------------------------------------------\n");
}
void print_priority_queue_of_struct_info(struct_info *info) {
    if (!info) {
        printf("Invalid struct_info pointer\n");
        return;
    }

    Heap *heap = &info->free_blocks_heap;
    Node *head = &heap->head;

    printf("\n[Free Block Priority Queue for struct: %s]\n", info->struct_name);
    printf("---------------------------------------------------------------\n");
    printf("%-4s | %-14s | %-9s | %-6s | %-6s\n", "No.", "Block Addr", "Status", "Size", "Offset");
    printf("---------------------------------------------------------------\n");

    Node *curr = head->next;
    int index = 0;

    while (curr) {
        block_meta *block = NODE_TO_STRUCT(curr, block_meta, priority_node);

        printf("%-4d | %-14p | %-9s | %-6u | %-6u\n",
               index++,
               (void *)block,
               block->is_free ? "FREE" : "ALLOCATED",
               block->block_size,
               block->offset);

        curr = curr->next;
    }

    if (index == 0) {
        printf("No free blocks in the priority queue.\n");
    }

    printf("---------------------------------------------------------------\n");
}


void *xcalloc(char *struct_name, int units) {
    struct_info *info = lookup_struct_info(struct_name);
    //print_meta_blocks_for_struct_info(info);
    //print_priority_queue_of_struct_info(info);

    printf("%s \n",info->struct_name);
    if (!info) {
        printf("Error: Structure %s not registered with memory manager\n", struct_name);
        return NULL;
    }
    if (units * info->struct_size > get_max_allocatable_bytes(1)) {
        printf("Error: Requested memory exceeds system page size\n");
        return NULL;
    }

    block_meta *free_block = allocate_free_data_block(info, units * info->struct_size);
    //print_priority_queue_of_struct_info(info);

    if (free_block) {
        memset((char *)(free_block + 1), 0, free_block->block_size);
        return (void *)(free_block + 1);
    }

    return NULL;
}


void print_block_usage() {
    sys_page *sys_pg = first_sys_page;
    struct_info *info = NULL;
    vm_page *page = NULL;
    block_meta *block = NULL;

    unsigned total_block_count = 0;
    unsigned free_block_count = 0;
    unsigned occupied_block_count = 0;
    unsigned application_memory_usage = 0;

    printf("\n[Memory Usage Report]\n");
    printf("--------------------------------------------------------------\n");
    printf("%-32s | TBC | FBC | OBC | AppMemUsage\n", "Struct Name");
    printf("--------------------------------------------------------------\n");

    while (sys_pg) {
        ITERATE_STRUCT_INFO_BEGIN(sys_pg, info) {
            total_block_count = free_block_count = occupied_block_count = application_memory_usage = 0;

            page = info->first_page;

            while (page) {
                ITERATE_META_BLOCKS_BEGIN(page, block) {
                    total_block_count++;

                    if (block->is_free) {
                        free_block_count++;

                        // Sanity: should exist in heap
                        assert(block->priority_node.next != NULL || block->priority_node.prev != NULL);
                    } else {
                        occupied_block_count++;
                        application_memory_usage += block->block_size + sizeof(block_meta);
                        // Sanity: should not be in heap
                        assert(block->priority_node.next == NULL && block->priority_node.prev == NULL);
                    }

                } ITERATE_META_BLOCKS_END;
                page = page->next;
            }

            printf("%-32s | %-3u | %-3u | %-3u | %-10u\n",
                   info->struct_name,
                   total_block_count,
                   free_block_count,
                   occupied_block_count,
                   application_memory_usage);

        } ITERATE_STRUCT_INFO_END;
        sys_pg = sys_pg->next;
    }

    printf("--------------------------------------------------------------\n");
}
