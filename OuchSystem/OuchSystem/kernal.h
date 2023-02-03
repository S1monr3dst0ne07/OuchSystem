#ifndef HKERNAL
#define HKERNAL

#include <stdbool.h>

extern char cTemp[];

struct system
{
	struct fileNode*   root;
	struct procPool*   pool;
	struct streamPool* river; //get it, cuz' multible streams make a river
};

bool launchProcessFromPath(char* pathStr, struct system* ouch);
void ouch(char* imagePath);

#endif