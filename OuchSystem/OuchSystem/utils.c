
#include "utils.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

void flog(char* format, ...)
{
    char tmp[2048] = { 0 };

    va_list vargs;
    va_start(vargs, format);
    vsprintf(tmp, format, vargs);
    va_end(vargs);

	printf("%s", tmp);

}

long clockMsRT()
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);

    return (spec.tv_sec * 1000) + (spec.tv_nsec / 1000000);
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
//returns delim that was hit
char readStringCustomDelim(char* dst, char* src, int* index, char* delim)
{
    int i = 0;
    char c;
    while ((c = src[*index]) != 0 && !charInString(delim, c)) dst[i++] = consu(src, index);
	dst[i] = 0; //terminator

    return c;
}



bool isOnlyDigits(const char* s)
{
    while (*s) {
        if (isdigit(*s++) == 0) return false;
    }

    return true;
}

unsigned int getSmallPosivNumNotInList(unsigned int* list, unsigned int len)
{
    //base case
    if (!len) return 0;

    //get biggest number in list
    unsigned int limit = 0;
    for (int i = 0; i < len; i++)
        if (limit <= list[i]) limit = list[i];

    //record what value are free from the list
    bool* freeRecord = (bool*)malloc(limit + 1);
    fguard(freeRecord, msgMallocGuard, -1);
    memset(freeRecord, true, limit + 1);

    //populate the record
    for (int i = 0; i < len; i++)
        freeRecord[list[i]] = false;

    //find first value in freeRecord that is free
    unsigned int res = limit + 1;
    for (int i = 0; i < limit; i++)
        if (freeRecord[i])
        {
            res = (unsigned int)i;
            break;
        }

    free(freeRecord);
    return res;
}
