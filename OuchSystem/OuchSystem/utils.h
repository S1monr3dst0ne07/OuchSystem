#ifndef HUTILS
#define HUTILS

#include "types.h"

#include <stdbool.h>
#include <stdlib.h>

void logg(char* msg);

long clockMsRT();

char consu(char* s, int* i);
bool charInString(char* str, char c);
char readStringCustomDelim(char* dst, char* src, int* index, char* delim);
bool isOnlyDigits(const char* s);
unsigned int getSmallPosivNumNotInList(unsigned int* list, unsigned int len);

#endif