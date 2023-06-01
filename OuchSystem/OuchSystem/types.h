#ifndef HTYPES
#define HTYPES

typedef unsigned short int S1Int;

#define COMMA , //meta comma
#define msgMallocGuard "Unable to malloc, fuck\n"
#define fguard(x, y, z) if (!x) { flog(y); return z; }

#endif