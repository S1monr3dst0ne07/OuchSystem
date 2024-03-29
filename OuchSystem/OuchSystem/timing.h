#ifndef HTIMING
#define HTIMING

//forward
struct process;

#include "types.h"

#include <time.h>
#include <string.h>

//keeps track of duration process is napping for
struct processNap
{
	clock_t startTime;
	int durMs;
};

void freeProcNap(struct processNap* procNap);
int updateProcNap(struct process* proc);
void procNap(int durMs, struct process* proc);
struct processNap* cloneProcNap(struct processNap* src);

#endif