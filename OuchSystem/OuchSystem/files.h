#ifndef HFILES
#define HFILES

#define imageTermi 0x01
#define imageNode  0x10

#define subNodeCount 128

char cTemp[2048];

struct fileNode;

struct fileNode* mountRootImage(char* path);
struct fileNode* parseNode(char* ptr, int* i);
char* readTermed(char* ptr, int* i);
//void printImage(struct fileNode* ptr, int l);

#endif