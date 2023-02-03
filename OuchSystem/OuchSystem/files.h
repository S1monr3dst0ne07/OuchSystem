#ifndef HFILES
#define HFILES

#define imageTermi 0x01
#define imageNode  0x10

#define subNodeCount 128
#define pathLenLimit 128
#define defBufferSize 128

#include "kernal.h"

//i hate microsoft
#define _CRT_NONSTDC_NO_DEPRECATE

char cTemp[2048];

struct fileNode
{
	char* name;
	int type; //(1 -> dir, 2 -> file)
	int prior;

	//only used if type == 1
	struct fileNode* subNodes[subNodeCount];
	int count; //number of subnodes

	//only used if type == 2
	char* content;


};

struct rawImage;

struct fileNode* mountRootImage(char* path);
void freeFileSystem(struct fileNode* root);
void unmountRootImage(char* path, struct fileNode* root);

struct fileNode* getSubNodeByName(struct fileNode* top, char* name);
struct fileNode* getNodeByPath(struct fileNode* root, struct filePath* path);

struct filePath
{
	int len;
	char* dirPath[pathLenLimit];
};
struct filePath* parseFilePath(char* path);
void freeFilePath(struct filePath* path);

char* readFileContent(struct system* ouch, struct filePath* path);
bool writeFileContent(struct system* ouch, struct filePath* path, char* content);

void printImage(struct fileNode* ptr, int l);

#endif