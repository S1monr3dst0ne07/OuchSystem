#ifndef HUTILS
#define HUTILS

#include "types.h"

#include <stdbool.h>
#include <stdlib.h>

void flog(char* format, ...);

long clockMsRT();

char consu(char* s, int* i);
bool charInString(char* str, char c);
char readStringCustomDelim(char* dst, char* src, int* index, char* delim);
bool isOnlyDigits(const char* s);
unsigned int getSmallPosivNumNotInList(unsigned int* list, unsigned int len);
char* renderObjectByFuncWithSeps(void* object, char* get(void*, int), int len, char sep);


#ifdef _WIN32
#define CLOCK_REALTIME 0
#endif



#endif