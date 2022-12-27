#ifndef HKERNAL
#define HKERNAL


struct system
{
	struct fileNode* root;
	struct procPool* pool;

};

void ouch(char* imagePath);

#endif