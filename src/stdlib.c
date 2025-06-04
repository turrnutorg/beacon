/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * stdlib.c
 */

#include "stdlib.h"
#include "string.h"
#include "command.h"   // for reset()
#include "console.h"  // for print()
#include <stdint.h>
#include <stdarg.h>

// ─── simple spinlock ────────────────────────────────────────────────────────
static volatile int heap_lock = 0;
static void lock_heap(void) {
    while (__sync_lock_test_and_set(&heap_lock, 1)) { /* busy */ }
}
static void unlock_heap(void) {
    __sync_lock_release(&heap_lock);
}

// ─── memory allocator ────────────────────────────────────────────────────────
#define HEAP_SIZE 0x100000  // 1mb heap
#define ALIGNMENT 8

typedef struct block_header {
    size_t size;
    int free;
    struct block_header* next;
    struct block_header* prev;
} block_header;

static uint8_t heap[HEAP_SIZE];
static block_header* head = NULL;

static size_t align(size_t size) {
    return (size + (ALIGNMENT - 1)) & ~(ALIGNMENT - 1);
}

static void memory_init(void) {
    head = (block_header*)heap;
    head->size = HEAP_SIZE - sizeof(block_header);
    head->free = 1;
    head->next = NULL;
    head->prev = NULL;
}

static void split_block(block_header* block, size_t size) {
    if (block->size >= size + sizeof(block_header) + ALIGNMENT) {
        block_header* new_block = (block_header*)((uint8_t*)block + sizeof(block_header) + size);
        new_block->size = block->size - size - sizeof(block_header);
        new_block->free = 1;
        new_block->next = block->next;
        new_block->prev = block;
        if (new_block->next) new_block->next->prev = new_block;
        block->size = size;
        block->next = new_block;
    }
}

static block_header* find_free_block(size_t size) {
    block_header* curr = head;
    while (curr) {
        if (curr->free && curr->size >= size) return curr;
        curr = curr->next;
    }
    return NULL;
}

static void coalesce_forward(block_header* block) {
    while (block->next && block->next->free) {
        block_header* nxt = block->next;
        block->size += sizeof(block_header) + nxt->size;
        block->next = nxt->next;
        if (block->next) block->next->prev = block;
    }
}

static block_header* coalesce_backward(block_header* block) {
    if (block->prev && block->prev->free) {
        block_header* prev = block->prev;
        prev->size += sizeof(block_header) + block->size;
        prev->next = block->next;
        if (prev->next) prev->next->prev = prev;
        block = prev;
    }
    return block;
}

void* malloc(size_t size) {
    if (size == 0) return NULL;
    size = align(size);

    lock_heap();
    if (!head) memory_init();

    block_header* block = find_free_block(size);
    if (!block) { unlock_heap(); return NULL; }

    split_block(block, size);
    block->free = 0;
    void* user_ptr = (uint8_t*)block + sizeof(block_header);
    unlock_heap();
    return user_ptr;
}

void free(void* ptr) {
    if (!ptr) return;
    lock_heap();
    block_header* block = (block_header*)((uint8_t*)ptr - sizeof(block_header));
    block->free = 1;
    coalesce_forward(block);
    block = coalesce_backward(block);
    unlock_heap();
}

void* calloc(size_t num, size_t size) {
    if (num == 0 || size == 0 || num > (SIZE_MAX / size)) return NULL;
    size_t total = num * size;
    void* ptr = malloc(total);
    if (!ptr) return NULL;
    memset(ptr, 0, total);
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (size == 0) { free(ptr); return NULL; }
    size = align(size);

    block_header* block = (block_header*)((uint8_t*)ptr - sizeof(block_header));
    lock_heap();

    if (block->size >= size) {
        if (block->size >= size + sizeof(block_header) + ALIGNMENT) {
            split_block(block, size);
        }
        unlock_heap();
        return ptr;
    }

    if (block->next && block->next->free) {
        size_t combined = block->size + sizeof(block_header) + block->next->size;
        if (combined >= size) {
            block_header* nxt = block->next;
            block->size = combined;
            block->next = nxt->next;
            if (block->next) block->next->prev = block;
            if (block->size >= size + sizeof(block_header) + ALIGNMENT) {
                split_block(block, size);
            }
            block->free = 0;
            unlock_heap();
            return ptr;
        }
    }

    unlock_heap();
    void* new_ptr = malloc(size);
    if (!new_ptr) return NULL;
    size_t old_size = block->size;
    memcpy(new_ptr, ptr, old_size);
    free(ptr);
    return new_ptr;
}

// ─── exit / abort ────────────────────────────────────────────────────────────
void exit(int status) {
    (void)status;
    reset();
    while (1);
}

void abort(void) {
    while (1);
}

// ─── conversions ─────────────────────────────────────────────────────────────
int atoi(const char* str) {
    int res = 0, sign = 1;
    size_t i = 0;
    while (str[i] == ' ' || str[i] == '\t') i++;
    if (str[i] == '-') { sign = -1; i++; }
    else if (str[i] == '+') i++;
    for (; str[i] >= '0' && str[i] <= '9'; ++i) {
        res = res * 10 + (str[i] - '0');
    }
    return sign * res;
}

long strtol(const char* str, char** endptr, int base) {
    long result = 0;
    int sign = 1;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '-') { sign = -1; str++; }
    else if (*str == '+') str++;
    if ((base == 0 || base == 16) && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        base = 16; str += 2;
    } else if (base == 0) base = 10;
    while (*str) {
        int digit;
        if (*str >= '0' && *str <= '9') digit = *str - '0';
        else if (*str >= 'A' && *str <= 'F') digit = *str - 'A' + 10;
        else if (*str >= 'a' && *str <= 'f') digit = *str - 'a' + 10;
        else break;
        if (digit >= base) break;
        result = result * base + digit;
        str++;
    }
    if (endptr) *endptr = (char*)str;
    return sign * result;
}

long atol(const char* s) {
    return strtol(s, NULL, 10);
}

long long strtoll(const char* str, char** endptr, int base) {
    long long result = 0;
    int sign = 1;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '-') { sign = -1; str++; }
    else if (*str == '+') str++;
    if ((base == 0 || base == 16) && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        base = 16; str += 2;
    } else if (base == 0) base = 10;
    while (*str) {
        int digit;
        if (*str >= '0' && *str <= '9') digit = *str - '0';
        else if (*str >= 'A' && *str <= 'F') digit = *str - 'A' + 10;
        else if (*str >= 'a' && *str <= 'f') digit = *str - 'a' + 10;
        else break;
        if (digit >= base) break;
        result = result * base + digit;
        str++;
    }
    if (endptr) *endptr = (char*)str;
    return sign * result;
}

long long atoll(const char* s) {
    return strtoll(s, NULL, 10);
}

unsigned long strtoul(const char* str, char** endptr, int base) {
    unsigned long result = 0;
    int sign = 1;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '-') { sign = -1; str++; }
    else if (*str == '+') str++;
    if ((base == 0 || base == 16) && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        base = 16; str += 2;
    } else if (base == 0) base = 10;
    while (*str) {
        int digit;
        if (*str >= '0' && *str <= '9') digit = *str - '0';
        else if (*str >= 'A' && *str <= 'F') digit = *str - 'A' + 10;
        else if (*str >= 'a' && *str <= 'f') digit = *str - 'a' + 10;
        else break;
        if (digit >= base) break;
        result = result * base + digit;
        str++;
    }
    if (endptr) *endptr = (char*)str;
    return sign * result;
}

unsigned long long strtoull(const char* str, char** endptr, int base) {
    unsigned long long result = 0;
    int sign = 1;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '-') { sign = -1; str++; }
    else if (*str == '+') str++;
    if ((base == 0 || base == 16) && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        base = 16; str += 2;
    } else if (base == 0) base = 10;
    while (*str) {
        int digit;
        if (*str >= '0' && *str <= '9') digit = *str - '0';
        else if (*str >= 'A' && *str <= 'F') digit = *str - 'A' + 10;
        else if (*str >= 'a' && *str <= 'f') digit = *str - 'a' + 10;
        else break;
        if (digit >= base) break;
        result = result * base + digit;
        str++;
    }
    if (endptr) *endptr = (char*)str;
    return sign * result;
}

double atof(const char* str) {
    double result = 0.0, frac = 0.0;
    int sign = 1, exponent = 0, exp_sign = 1;
    while (*str == ' ' || *str == '\t') str++;
    if (*str == '-') { sign = -1; str++; }
    else if (*str == '+') str++;
    while (*str >= '0' && *str <= '9') {
        result = result * 10.0 + (*str++ - '0');
    }
    if (*str == '.') {
        str++;
        double divisor = 10.0;
        while (*str >= '0' && *str <= '9') {
            frac += (*str++ - '0') / divisor;
            divisor *= 10.0;
        }
    }
    result += frac;
    if (*str == 'e' || *str == 'E') {
        str++;
        if (*str == '-') { exp_sign = -1; str++; }
        else if (*str == '+') str++;
        while (*str >= '0' && *str <= '9') {
            exponent = exponent * 10 + (*str++ - '0');
        }
        double power = 1.0;
        for (int i = 0; i < exponent; i++) power *= 10.0;
        if (exp_sign < 0) result /= power;
        else result *= power;
    }
    return sign * result;
}

// ─── math helpers ────────────────────────────────────────────────────────────
int abs(int x) { return x < 0 ? -x : x; }
long labs(long x) { return x < 0 ? -x : x; }
long long llabs(long long x) { return x < 0 ? -x : x; }

// ─── div results ─────────────────────────────────────────────────────────────
div_t div(int n, int d) {
    div_t r = { n / d, n % d };
    return r;
}
ldiv_t ldiv(long n, long d) {
    ldiv_t r = { n / d, n % d };
    return r;
}

// ─── random ───────────────────────────────────────────────────────────────────
static unsigned int rng_seed = 123456789;

void srand(unsigned int seed) {
    rng_seed = seed;
}

int rand(void) {
    rng_seed = rng_seed * 1103515245 + 12345;
    return (int)((rng_seed >> 16) & 0x7FFF);
}

int rand_r(unsigned int* seed) {
    *seed = *seed * 1103515245 + 12345;
    return (int)((*seed >> 16) & 0x7FFF);
}

// ─── sorting / searching ───────────────────────────────────────────────────────
void qsort(void* base, size_t count, size_t size,
           int (*cmp)(const void*, const void*)) {
    char* array = (char*)base;
    for (size_t i = 0; i < count - 1; i++) {
        for (size_t j = 0; j < count - i - 1; j++) {
            char* a = array + j * size;
            char* b = array + (j + 1) * size;
            if (cmp(a, b) > 0) {
                for (size_t k = 0; k < size; k++) {
                    char tmp = a[k];
                    a[k] = b[k];
                    b[k] = tmp;
                }
            }
        }
    }
}

void* bsearch(const void* key, const void* base,
              size_t count, size_t size,
              int (*cmp)(const void*, const void*)) {
    size_t low = 0, high = count;
    while (low < high) {
        size_t mid = (low + high) / 2;
        void* elem = (char*)base + mid * size;
        int res = cmp(key, elem);
        if (res < 0) high = mid;
        else if (res > 0) low = mid + 1;
        else return elem;
    }
    return NULL;
}

// ─── formatted print ─────────────────────────────────────────────────────────
int vsnprintf(char* buffer, unsigned int buf_size, const char* format, va_list args) {
    char* buf_ptr = buffer;
    unsigned int remaining = buf_size ? buf_size - 1 : 0;
    for (const char* f = format; *f && remaining; f++) {
        if (*f == '%') {
            f++;
            if (*f == 'd') {
                int val = va_arg(args, int);
                char temp[16];
                // reuse itoa_base from string.c if available; else simple:
                int is_neg = (val < 0);
                unsigned int uval = is_neg ? -val : val;
                int pos = 0;
                if (is_neg) temp[pos++] = '-';
                char rev[12]; int rpos = 0;
                do {
                    rev[rpos++] = '0' + (uval % 10);
                    uval /= 10;
                } while (uval);
                while (rpos) temp[pos++] = rev[--rpos];
                temp[pos] = '\0';
                for (char* t = temp; *t && remaining; t++, remaining--) *buf_ptr++ = *t;
            } else if (*f == 's') {
                char* str = va_arg(args, char*);
                for (; *str && remaining; str++, remaining--) *buf_ptr++ = *str;
            } else if (*f == 'c') {
                char c = (char)va_arg(args, int);
                if (remaining) { *buf_ptr++ = c; remaining--; }
            } else {
                if (remaining) { *buf_ptr++ = '%'; remaining--; }
                if (remaining) { *buf_ptr++ = *f; remaining--; }
            }
        } else {
            *buf_ptr++ = *f;
            remaining--;
        }
    }
    if (buf_size) *buf_ptr = '\0';
    return (int)(buf_ptr - buffer);
}

int snprintf(char* buffer, unsigned int buf_size, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int written = vsnprintf(buffer, buf_size, format, args);
    va_end(args);
    return written;
}

void printf(const char* format, ...) {
    char buf[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    print(buf);
}

// note: assumes a 'print(const char*)' from console.c or similar.

// ─── eof ─────────────────────────────────────────────────────────────────────
