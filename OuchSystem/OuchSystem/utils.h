#ifndef HUTILS
#define HUTILS

#include <stdbool.h>


void log(char* msg);

char consu(char* s, int* i);
bool charInString(char* str, char c);
void readStringCustomDelim(char* dst, char* src, int* index, char* delim);
bool isOnlyDigits(const char* s);


#endif