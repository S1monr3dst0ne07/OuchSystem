#ifndef HVM
#define HVM

#include "process.h"
#include "utils.h"

bool stackPush(S1Int* stack, int* stackPtr, const S1Int* value);
bool stackPull(S1Int* stack, int* stackPtr, S1Int* value);
enum returnCodes runProcess(struct process* proc);

#endif