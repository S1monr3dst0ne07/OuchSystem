#ifndef HTYPES
#define HTYPES

typedef unsigned short int S1Int;

#define COMMA , //meta comma
#define msgMallocGuard "Unable to malloc, fuck\n"

#define  guard(x, y)    if (!(x)) return y;
#define fguard(x, y, z) if (!(x)) { flog(y); return z; }


#define Bit16IntLimit (1 << 16)

#define max(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b;       \
})

#define min(a,b)             \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})



#endif