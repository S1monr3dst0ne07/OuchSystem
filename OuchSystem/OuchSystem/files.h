#ifndef HFILES
#define HFILES

#define imageTermi 0x01
#define imageNode  0x10

#define subNodeCount 128
#define pathLenLimit 128
#define defBufferSize 128
#define nodeNameLimit 256

#include "kernal.h"
#include "types.h"

//i hate microsoft
#define _CRT_NONSTDC_NO_DEPRECATE


enum fileNodeTypes
{
	fileNodeInvaild = 0x0,
	fileNodeDir,
	fileNodeFile,
};

struct fileNode
{
	char name[nodeNameLimit];
	enum fileNodeTypes type;

	int subCount;
	struct fileNode* subNodes[subNodeCount];

	int contLen;
	char* contPtr;

	//used to update len of super nodes
	struct fileNode* superNode;

};

struct file
{
	int contLen;
	char* contPtr;
};

#define emptyFile (struct file) { .contLen = 0, .contPtr = NULL }

struct fileNode* mountRootImage(char* path);
void freeFileSystem(struct fileNode* root);
void unmountRootImage(char* path, struct fileNode* root);

//struct fileNode* getSubNodeByName(struct fileNode* top, char* name);
//struct fileNode* getNodeByPath(struct fileNode* root, struct filePath* path);

struct filePath
{
	int len;
	char* dirPath[pathLenLimit];
};
struct filePath* parseFilePath(char* path);
void freeFilePath(struct filePath* path);
struct filePath* cloneFilePath(struct filePath* src);

struct file readFile(struct system* ouch, struct filePath* path);
char* readFileContent(struct system* ouch, struct filePath* path);

bool writeFile(struct system* ouch, struct filePath* path, struct file f);


struct file getFileContentPtr(struct system* ouch, struct filePath* path);

enum fileNodeTypes getNodeTypeByPath(struct system* ouch, struct filePath* path);
bool isFile(struct system* ouch, struct filePath* path);

//void printImage(struct fileNode* ptr, int l);

#endif