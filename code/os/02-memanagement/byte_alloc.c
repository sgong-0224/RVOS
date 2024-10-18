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

/*
 * _alloc_start points to the actual start address of heap pool
 * _alloc_end points to the actual end address of heap pool
 * _num_bytes holds the actual max number of pages we can allocate.
 */
static ptr_t _alloc_start = 0;
static ptr_t _alloc_end = 0;
static uint32_t _num_bytes = 0;

struct Byte{
    uint8_t flags;
};

#define BYTE_TAKEN (uint8_t)(1<<0)
#define LAST_BYTE  (uint8_t)(1<<1)

static inline void _clear(struct Byte* byte)
{
    byte->flags=0;
}

static inline int _is_free(struct Byte* byte)
{
    return (byte->flags & BYTE_TAKEN) ? 0 : 1;
}

static inline void _set_flag(struct Byte* byte, uint8_t flags)
{
    byte->flags|=flags;
}

static inline int _is_last(struct Byte* byte)
{
    return (byte->flags & LAST_BYTE) ? 1 : 0;
}

void heap_init()
{
    uint32_t metadata_bytes = LENGTH_RAM>>1;     // 1B metadata per 1B
    _num_bytes = HEAP_SIZE-metadata_bytes;
    printf("HEAP_START = %p, HEAP_SIZE = 0x%lx,\n"
	       "num of reserved bytes = %d, num of bytes to be allocated for heap = %d\n",
	       HEAP_START, HEAP_SIZE,
	       metadata_bytes, _num_bytes);

    // struct Byte* byte = (struct Byte*)HEAP_START;
	
	_alloc_start = HEAP_START + metadata_bytes;
	_alloc_end = _alloc_start + _num_bytes;

	printf("TEXT:   %p -> %p\n", TEXT_START, TEXT_END);
	printf("RODATA: %p -> %p\n", RODATA_START, RODATA_END);
	printf("DATA:   %p -> %p\n", DATA_START, DATA_END);
	printf("BSS:    %p -> %p\n", BSS_START, BSS_END);
	printf("HEAP:   %p -> %p\n", _alloc_start, _alloc_end);
}

// TODO: alloc/free from page
extern void* page_alloc(int npage);
extern void page_free(void* p);

void* malloc(uint32_t bytes)
{
    int found = 0;
    struct Byte* byte_i = (struct Byte*)HEAP_START;
    int i=0;
    while( i<_num_bytes-bytes ){
        struct Byte* byte_j = byte_i;
        if(_is_free(byte_i)){
            found = 1;
            for(int j=i;j<i+bytes;++j){
                if(!_is_free(byte_j)){
                    found = 0;
                    break;
                }
                byte_j+=1;
            }
        }
        if(found){
            for (int k=0; k<bytes;++k)
                _set_flag(byte_i+k, BYTE_TAKEN);
            _set_flag(byte_i+bytes-1, LAST_BYTE);
            return (void *)(_alloc_start+(byte_i-HEAP_START));
		}
        byte_i = byte_j + 1;
    }
    return NULL;
}

void free(void* mem)
{
    if( mem==NULL || (ptr_t)mem >= _alloc_end )
        return;
    struct Byte* byte = (struct Byte*) HEAP_START;
    byte += ((ptr_t)mem-_alloc_start);
    while(!_is_free(byte)){
        if(_is_last(byte)){
            _clear(byte);
            return;
        }
        _clear(byte);
        ++byte;
    }
}

void malloc_test()
{
	void *p1 = malloc(2);
	printf("malloc p1 = %p\n", p1);
	//page_free(p);

	void *p2 = malloc(7);
	printf("malloc p2 = %p\n", p2);

	void *p3 = malloc(114);
	printf("malloc p3 = %p\n", p3);

    free(p2);
    void *p4 = malloc(14);
	printf("malloc p4 = %p\n", p4);
    free(p4);
    free(p3);
    void *p5 = malloc(3);
    printf("malloc p5 = %p\n", p5);
    free(p5);
    free(p1);
}