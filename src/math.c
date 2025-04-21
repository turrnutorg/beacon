// math.c
#include "math.h"
#include <stdint.h>

// ─── float implementations ──────────────────────────────────────────────────
#define PI_F 3.14159265358979323846f
#define INF_F (1.0f/0.0f)

// absolute value
float fabsf(float x) { return x < 0 ? -x : x; }

// sqrt via Newton‑Raphson
float sqrtf(float x) {
    if (x <= 0) return 0;
    float r = x;
    for (int i = 0; i < 8; i++)
        r = 0.5f * (r + x/r);
    return r;
}

// e^x via series (good for |x| < ~4)
float expf(float x) {
    float sum = 1.0f, term = 1.0f;
    for (int i = 1; i < 12; i++) {
        term *= x / i;
        sum  += term;
    }
    return sum;
}

// ln(x) via Newton on f(y)=e^y–x
float logf(float x) {
    if (x <= 0) return 0;
    float y = 0.0f;
    for (int i = 0; i < 12; i++)
        y -= (expf(y) - x) / expf(y);
    return y;
}

// fmod = a - b * floor(a/b)
float fmodf(float a, float b) {
    if (b == 0) return 0;
    float q = a / b;
    float fl = (q >= 0 ? (int)q : (int)q - 1);
    return a - b * fl;
}

// trig via our sinf/cosf
float sinf(float x) {
    // reduce to [-pi,pi]
    while (x < -PI_F) x += 2*PI_F;
    while (x >  PI_F) x -= 2*PI_F;
    // Taylor 5‑term
    float x2 = x*x, term = x, sum = term;
    term *= -x2 / (2*3); sum += term;
    term *= -x2 / (4*5); sum += term;
    term *= -x2 / (6*7); sum += term;
    term *= -x2 / (8*9); sum += term;
    return sum;
}
float cosf(float x) { return sinf(x + PI_F/2); }
float tanf(float x) { return sinf(x)/cosf(x); }

// inverse trig
float atanf(float x) {
    // rough approximation
    return x / (1.0f + 0.28f * x*x);
}
float asinf(float x) {
    if (x <= -1 || x >= 1) return x < 0 ? -PI_F/2 : PI_F/2;
    return atanf(x / sqrtf(1 - x*x));
}
float acosf(float x) {
    return PI_F/2 - asinf(x);
}

// hyperbolic
float sinhf(float x) { return (expf(x) - expf(-x)) * 0.5f; }
float coshf(float x) { return (expf(x) + expf(-x)) * 0.5f; }
float tanhf(float x) { float e = expf(x), i = expf(-x); return (e - i)/(e + i); }

// rounding
float floorf(float x) {
    int i = (int)x;
    return (x < 0 && x != i) ? (float)(i - 1) : (float)i;
}
float ceilf(float x) {
    int i = (int)x;
    return (x > 0 && x != i) ? (float)(i + 1) : (float)i;
}
float roundf(float x) { return floorf(x + 0.5f); }

// signbit, nan, inf
int signbitf(float x) { return x < 0.0f; }
int isnanf(float x)   { return x != x; }
int isinff(float x)   { return x == INF_F || x == -INF_F; }

// pow via exp/log
float powf(float base, float exp) {
    return expf(exp * logf(base));
}


// ─── fixed‑point wrappers (Q16.16 → float → Q16.16) ─────────────────────────
static inline fixed_t to_fixed(float v) { return (fixed_t)(v * FIXED_ONE); }
static inline float    to_float(fixed_t v) { return (float)v / FIXED_ONE; }

fixed_t fabs(fixed_t x)   { return x < 0 ? -x : x; }
fixed_t sqrt(fixed_t x)   { return to_fixed( sqrtf(to_float(x)) ); }
fixed_t exp(fixed_t x)    { return to_fixed( expf(to_float(x)) ); }
fixed_t log(fixed_t x)    { return to_fixed( logf(to_float(x)) ); }
fixed_t fmod(fixed_t a, fixed_t b) { return to_fixed( fmodf(to_float(a), to_float(b)) ); }
fixed_t sin(fixed_t x)   { return to_fixed( sinf(to_float(x)) ); }
fixed_t cos(fixed_t x)   { return to_fixed( cosf(to_float(x)) ); }
fixed_t tan(fixed_t x)   { return to_fixed( tanf(to_float(x)) ); }
fixed_t asin(fixed_t x)  { return to_fixed( asinf(to_float(x)) ); }
fixed_t acos(fixed_t x)  { return to_fixed( acosf(to_float(x)) ); }
fixed_t atan(fixed_t x)  { return to_fixed( atanf(to_float(x)) ); }
fixed_t sinh(fixed_t x)   { return to_fixed( sinhf(to_float(x)) ); }
fixed_t cosh(fixed_t x)   { return to_fixed( coshf(to_float(x)) ); }
fixed_t tanh(fixed_t x)   { return to_fixed( tanhf(to_float(x)) ); }
fixed_t floor(fixed_t x)  { return to_fixed( floorf(to_float(x)) ); }
fixed_t ceil(fixed_t x)   { return to_fixed( ceilf(to_float(x)) ); }
fixed_t round(fixed_t x)  { return to_fixed( roundf(to_float(x)) ); }
fixed_t pow(fixed_t b, fixed_t e) { return to_fixed( powf(to_float(b), to_float(e)) ); }

int signbit(fixed_t x)     { return x < 0; }
int isnan(fixed_t x)       { (void)x; return 0; /* fixed‐point never nan */ }
int isinf(fixed_t x)       { (void)x; return 0; /* nor inf */ }

