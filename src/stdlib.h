/**
 * Copyright (c) Turrnut Open Source Organization
 * Under the GPL v3 License
 * See COPYING for information on how you can use this file
 * 
 * stdlib.h
 */

#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

// ─── memory allocation ──────────────────────────────────────────────────────
void* malloc(size_t size);
void* calloc(size_t num, size_t size);
void* realloc(void* ptr, size_t size);
void  free(void* ptr);

// ─── exit / abort ───────────────────────────────────────────────────────────
void exit(int status);
void abort(void);

// ─── conversions ────────────────────────────────────────────────────────────
int     atoi(const char* str);
long    atol(const char* str);
long long atoll(const char* str);
double  atof(const char* str);

long    strtol(const char* str, char** endptr, int base);
long long strtoll(const char* str, char** endptr, int base);
unsigned long strtoul(const char* str, char** endptr, int base);
unsigned long long strtoull(const char* str, char** endptr, int base);

// ─── division results ───────────────────────────────────────────────────────
typedef struct { int quot, rem; } div_t;
typedef struct { long quot, rem; } ldiv_t;

div_t  div(int numer, int denom);
ldiv_t ldiv(long numer, long denom);

// ─── random ─────────────────────────────────────────────────────────────────
void srand(unsigned int seed);
int  rand(void);
int  rand_r(unsigned int* seed);

// ─── sorting / searching ────────────────────────────────────────────────────
void qsort(void* base, size_t count, size_t size,
           int (*cmp)(const void*, const void*));
void* bsearch(const void* key, const void* base,
              size_t count, size_t size,
              int (*cmp)(const void*, const void*));

// ─── formatted print ────────────────────────────────────────────────────────
int vsnprintf(char* buffer, unsigned int buf_size, const char* format, va_list args);
int snprintf(char* buffer, unsigned int buf_size, const char* format, ...);
void printf(const char* format, ...);

// ─── handy helpers ──────────────────────────────────────────────────────────
#define ENOMEM -12  // hack error code for oom

#endif
