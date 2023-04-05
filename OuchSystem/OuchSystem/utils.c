
#include "utils.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>

void logg(char* msg)
{
	printf("%s", msg);

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
    //get bad inital, for special cases
    unsigned int res = 0;
    for (int i = 0; i < len; i++)
        if (res <= list[i]) res = list[i] + 1;

    //record what value are free from the list
    bool* freeRecord = (bool*)malloc(sizeof(bool) * len);
    memset(freeRecord, true, len);

    //populate the record
    for (int i = 0; i < len; i++)
        freeRecord[list[i]] = false;

    //find first value in freeRecord that is free
    for (int i = 0; i < len; i++)
        if (freeRecord[i])
        {
            res = (unsigned int)i;
            break;
        }

    free(freeRecord);
    return res;
}
