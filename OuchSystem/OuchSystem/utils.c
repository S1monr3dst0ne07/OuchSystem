#include "utils.h"
#include <stdio.h>

void log(char* msg)
{
	printf(msg);

}


char consu(char* s, int* i)
{
	return s[(*i)++];
}


bool charInString(char* str, char c)
{
	for (int i = 0; str[i] != 0; i++) if (str[i] == c) return true;
	return false;
}

//no, no, my function names aren't too long
//index perfers to index into the source
void readStringCustomDelim(char* dst, char* src, int* index, char* delim)
{
    int i = 0;
    while (!charInString(delim, src[*index])) dst[i++] = consu(src, index);
	dst[i] = 0; //terminator
}



bool isOnlyDigits(const char* s)
{
    while (*s) {
        if (isdigit(*s++) == 0) return false;
    }

    return true;
}