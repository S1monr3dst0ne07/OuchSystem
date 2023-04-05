#ifndef HTIMING
#define HTIMING

//forward
struct process;

#include <time.h>

//keeps track of duration process is napping for
struct processNap
{
	clock_t startTime;
	int durMs;
};

void updateProcNap(struct process* proc);
void procNap(int durMs, struct process* proc);
struct processNap* cloneProcNap(struct processNap* src);

#endif