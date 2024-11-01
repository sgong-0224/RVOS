#include "os.h"
/*
 * Following global vars are defined in mem.S
 */
extern ptr_t TEXT_START;
extern ptr_t TEXT_END;
extern ptr_t DATA_START;
extern ptr_t DATA_END;
extern ptr_t RODATA_START;
extern ptr_t RODATA_END;
extern ptr_t BSS_START;
extern ptr_t BSS_END;
extern ptr_t HEAP_START;
extern ptr_t HEAP_SIZE;

// 内存块结构体
typedef struct mem_block {
    uint32_t alloca_size;   // alloca[31]|size[30:0]
    struct mem_block* next; // 指向下一个内存块
} mem_block;

static inline uint32_t get_block_size(const mem_block* block) {
    return block->alloca_size & 0x7FFFFFFF;
}

static inline void set_block_size(mem_block* block, uint32_t size) {
    block->alloca_size = (block->alloca_size & 0x80000000) | (size & 0x7FFFFFFF);
}

static inline uint32_t is_allocated(const mem_block* block) {
    return (block->alloca_size & 0x80000000) != 0;
}

static inline void mark_allocated(mem_block* block) {
    block->alloca_size |= 0x80000000;
}

static inline void mark_free(mem_block* block) {
    block->alloca_size &= ~0x80000000;
}

static mem_block* heap_start = NULL;
static mem_block* free_list = NULL;

#define PAGE_ORDER 12
static inline ptr_t _align_page(ptr_t address)
{
	ptr_t order = (1 << PAGE_ORDER) - 1;
	return (address + order) & (~order);
}

void heap_init() {
    if (heap_start == NULL) {
        heap_start = (mem_block*)_align_page(HEAP_START);
        set_block_size(heap_start, HEAP_SIZE - sizeof(mem_block));
        printf("HEAP_START = %p, HEAP_SIZE = 0x%x,\n", heap_start, get_block_size(heap_start));
        mark_free(heap_start);
        heap_start->next = NULL;
        free_list = heap_start;
    }
}

void* malloc(uint32_t size) {
    heap_init();
    mem_block* current_block = free_list;
    while (current_block != NULL) {
        if (!is_allocated(current_block) && get_block_size(current_block) >= size + sizeof(mem_block)) {
            if (get_block_size(current_block) > size + sizeof(mem_block) * 2) { 
                mem_block* new_block = (mem_block*)((uint8_t*)current_block + sizeof(mem_block) + size);
                // 从尾部划分新的空闲块
                set_block_size(new_block, get_block_size(current_block)-(size + sizeof(mem_block)));
                mark_free(new_block);
                new_block->next = current_block->next;
                set_block_size(current_block, size + sizeof(mem_block));
                current_block->next = new_block;
            }
            mark_allocated(current_block);
            return (void*)(current_block + 1);
        }
        current_block = current_block->next;
    }
    return NULL;
}

void free(void* ptr) {
    if (ptr == NULL) 
        return;
    mem_block* block = (mem_block*)ptr-1; // 获取块头部
    mark_free(block);
    // 简单合并碎片
    mem_block* prev = NULL;
    mem_block* next = block->next;    
    if (prev && !is_allocated(prev)) {
        set_block_size(prev, get_block_size(prev)+get_block_size(block));
        prev->next = next;
        block = prev;
    }
    if (next && !is_allocated(next)) {
        set_block_size(block, get_block_size(next)+get_block_size(block));
        block->next = next->next;
    }
}

void malloc_test()
{
	void *p1 = malloc(22);
	printf("malloc p1 = %p\n", p1);
	void *p2 = malloc(71);
	printf("malloc p2 = %p\n", p2);
	void *p3 = malloc(114);
	printf("malloc p3 = %p\n", p3);
    void *p4 = malloc(14);
	printf("malloc p4 = %p\n", p4);
    free(p4);
    printf("%p freed.\n", p4);
    void *p6 = malloc(39);
    printf("malloc p6 = %p\n", p6);
    free(p2);
    printf("%p freed.\n", p2);
    void *p7 = malloc(39);
    printf("malloc p7 = %p\n", p7);
}