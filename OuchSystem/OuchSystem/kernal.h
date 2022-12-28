#ifndef HKERNAL
#define HKERNAL

#include <stdbool.h>

struct system
{
	struct fileNode* root;
	struct procPool* pool;

};

bool launchProcessFromPath(char* pathStr, struct system* ouch);
void ouch(char* imagePath);

#endif