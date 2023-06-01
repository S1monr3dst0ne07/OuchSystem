#ifndef HVM
#define HVM

#include "utils.h"
#include "process.h"
#include "types.h"

#include <stdlib.h>

//file mapping to process memory
//used for shared memory, etc ...
struct fileMap
{
	struct filePath* filePath; //file to map into memory
	S1Int size;                //size of region to map
	S1Int addr;                //address to load file to, in process memory
	S1Int offset;              //offset into the file to start loading from

	struct fileMap* next;
};


bool stackPush(S1Int* stack, int* stackPtr, const S1Int* value);
bool stackPull(S1Int* stack, int* stackPtr, S1Int* value);

bool loadFileMap(struct process* proc, struct fileMap* fmap, struct system* ouch);
bool saveFileMap(struct process* proc, struct fileMap* fmap, struct system* ouch);

void injectFileMap(struct process* proc, struct fileMap* fmap);
struct fileMap* createFileMap(char* filePath, S1Int size, S1Int addr, S1Int offset);

void freeFileMaps(struct fileMap* fmaps);

enum returnCodes runProcess(struct process* proc, int iterLimit, struct system* ouch);

#endif