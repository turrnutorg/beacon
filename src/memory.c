#include "memory.h"
#include <stdint.h>

typedef struct block_header {
    size_t size;
    int free;
    struct block_header* next;
} block_header_t;

static block_header_t* head = 0;
static void* heap_base = 0;
static size_t heap_total = 0;

#define ALIGN4(x) (((x) + 3) & ~3)
#define HEADER_SIZE sizeof(block_header_t)

void memory_init(void* heap_start, size_t heap_size) {
    heap_base = heap_start;
    heap_total = heap_size;
    head = (block_header_t*)heap_start;
    head->size = heap_size - HEADER_SIZE;
    head->free = 1;
    head->next = 0;
}

void* malloc(size_t size) {
    size = ALIGN4(size);
    block_header_t* curr = head;

    while (curr) {
        if (curr->free && curr->size >= size) {
            if (curr->size >= size + HEADER_SIZE + 4) {
                block_header_t* split = (void*)((uint8_t*)curr + HEADER_SIZE + size);
                split->size = curr->size - size - HEADER_SIZE;
                split->free = 1;
                split->next = curr->next;

                curr->size = size;
                curr->next = split;
            }
            curr->free = 0;
            return (void*)((uint8_t*)curr + HEADER_SIZE);
        }
        curr = curr->next;
    }
    return 0; // outta memory
}

void* calloc(size_t nmemb, size_t size) {
    void* ptr = malloc(nmemb * size);
    if (!ptr) return 0;
    uint8_t* p = ptr;
    for (size_t i = 0; i < nmemb * size; i++) p[i] = 0;
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);

    block_header_t* header = (block_header_t*)((uint8_t*)ptr - HEADER_SIZE);
    if (header->size >= size) return ptr;

    void* new_ptr = malloc(size);
    if (!new_ptr) return 0;

    uint8_t* src = ptr;
    uint8_t* dst = new_ptr;
    for (size_t i = 0; i < header->size; i++) dst[i] = src[i];

    free(ptr);
    return new_ptr;
}

void free(void* ptr) {
    if (!ptr) return;

    block_header_t* header = (block_header_t*)((uint8_t*)ptr - HEADER_SIZE);
    header->free = 1;

    // coalescing
    block_header_t* curr = head;
    while (curr && curr->next) {
        if (curr->free && curr->next->free) {
            curr->size += HEADER_SIZE + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}
