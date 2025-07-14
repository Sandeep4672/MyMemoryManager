# Custom Heap Memory Manager in C

This project implements a custom heap memory manager in C, providing dynamic memory allocation (`xcalloc`) and deallocation (`xfree`) functions tailored for user-defined data structures. It simulates virtual memory page management and mimics core functionality of `malloc`/`calloc` with block metadata tracking and worst-fit allocation.

---

## Features

* **Structure-based memory allocation** using `xcalloc(struct_name, units)`
* **Worst-fit memory allocation** strategy using a max-heap of free blocks
* **Custom VM page allocation** and management using `mmap`
* **Block metadata management** with splitting and coalescing
* **Efficient memory reuse** using doubly linked lists (DLL)
* **Memory fragmentation reduction** and visualization tools
* **Priority queue (max-heap)** for quick access to largest free blocks
* **Macros and iterators** for managing VM pages, structs, and blocks

---

## Project Structure

```
.
├── heap_manager.h         # Main interface and implementation
├── ulib.c / ulib.h        # Public API and utility macros
├── my_ds/
│   ├── dll.h / dll.c      # Doubly linked list implementation
│   ├── heap.h / heap.c    # Heap (priority queue) implementation
```

---

## Core Functions

### Allocation and Free

* `xcalloc(struct_name, units)` – Allocate memory for `units` instances of a registered structure.
* `xfree(ptr)` – Free the memory block associated with the given pointer.

### Structure Management

* `MM_REG_STRUCT(struct_type)` – Macro to register structure with memory manager.
* `print_registered_structs()` – Print all registered structures.

### Debugging Tools

* `print_meta_blocks_for_struct_info(info)` – View internal memory blocks for a structure.
* `print_block_usage()` – View usage stats (free/occupied blocks).
* `print_priority_queue_of_struct_info(info)` – View free block heap.
* `print_memory_usage()` – View total memory stats.

---

## How It Works

1. **Initialization**

   * The page size is retrieved using `getpagesize()`.
   * Structures are registered via `MM_REG_STRUCT()` macro.

2. **Memory Request (xcalloc)**

   * Uses worst-fit from max-heap to find best block.
   * If insufficient, new page is allocated.
   * Block may be split if excess space exists.

3. **Memory Release (xfree)**

   * Block is marked free.
   * Adjacent free blocks are merged.
   * Page is unmapped if fully free.

4. **Block Management**

   * Blocks are doubly linked for quick merges.
   * Free blocks are tracked using a heap structure.

---

## Sample Usage

```c
#include "ulib.h"

typedef struct employee_ {
    char name[32];
    int emp_id;
} employee_t;

int main() {
    INIT();
    MM_REG_STRUCT(employee_t);

    employee_t *emp1 = xcalloc("employee_t", 1);
    // Use emp1
    xfree(emp1);

    print_block_usage();
    return 0;
}
```

---

## Build Instructions

Make sure to link all required files:

```bash
gcc -o memman main.c ulib.c heap_manager.c my_ds/heap.c my_ds/dll.c -Wall
```

---

## Notes

* This is an educational tool, not a production-ready allocator.
* Demonstrates custom memory control, page mapping, and data structure management.
* Tested on Linux systems using `mmap()`.

---

## Author

Created by P Sandeep as part of a custom memory manager project for educational purposes.

---

## License

This project is licensed under the MIT License.
