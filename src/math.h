// math.h
#ifndef MATH_H
#define MATH_H

#include <stdint.h>
#include <stddef.h>

// ─── fixed‑point type Q16.16 ──────────────────────────────────────────────────
typedef int32_t fixed_t;
#define FIXED_ONE      (1<<16)
#define FIXED_HALF     (1<<15)
#define FIXED_FROM_INT(i) ((fixed_t)((i)<<16))
#define FIXED_TO_INT(f)   ((int)((f)>>16))

// ─── fixed‑point math ───────────────────────────────────────────────────────
fixed_t fabs(fixed_t x);
fixed_t sqrt(fixed_t x);
fixed_t pow(fixed_t base, fixed_t exp);
fixed_t exp(fixed_t x);
fixed_t log(fixed_t x);
fixed_t fmod(fixed_t a, fixed_t b);
fixed_t sin(fixed_t x);
fixed_t cos(fixed_t x);
fixed_t tan(fixed_t x);
fixed_t asin(fixed_t x);
fixed_t acos(fixed_t x);
fixed_t atan(fixed_t x);
fixed_t sinh(fixed_t x);
fixed_t cosh(fixed_t x);
fixed_t tanh(fixed_t x);
fixed_t floor(fixed_t x);
fixed_t ceil(fixed_t x);
fixed_t round(fixed_t x);
int     signbit(fixed_t x);
int     isnan(fixed_t x);
int     isinf(fixed_t x);

// ─── single‑precision float math ────────────────────────────────────────────
float   fabsf(float x);
float   sqrtf(float x);
float   powf(float base, float exp);
float   expf(float x);
float   logf(float x);
float   fmodf(float a, float b);
float   sinf(float x);
float   cosf(float x);
float   tanf(float x);
float   asinf(float x);
float   acosf(float x);
float   atanf(float x);
float   sinhf(float x);
float   coshf(float x);
float   tanhf(float x);
float   floorf(float x);
float   ceilf(float x);
float   roundf(float x);
int     signbitf(float x);
int     isnanf(float x);
int     isinff(float x);

#endif // MATH_H
