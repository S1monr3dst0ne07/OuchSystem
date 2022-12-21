#ifndef HFILES
#define HFILES

#define imageTermi 0x01
#define imageNode  0x10

struct fileNode;
struct nodeList;

struct fileNode* mountRootImage(char* path);
struct fileNode* parseNode(char* ptr);
char* readTermed(char* ptr);

#endif