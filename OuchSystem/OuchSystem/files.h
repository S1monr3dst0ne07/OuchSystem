#ifndef HFILES
#define HFILES

#define imageTermi 0x01
#define imageNode  0x10

#define subNodeCount 128

char cTemp[2048];

struct fileNode;
struct rawImage;

struct fileNode* mountRootImage(char* path);
struct fileNode* parseNode(struct rawImage* image);
char* readTermed(struct rawImage* image);
void printImage(struct fileNode* ptr, int l);

#endif