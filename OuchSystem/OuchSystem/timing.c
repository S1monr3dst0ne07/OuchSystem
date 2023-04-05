
#include "process.h"
#include "timing.h"
#include "utils.h"

struct processNap* allocProcNap()
{
	return (struct processNap*)malloc(sizeof(struct processNap));
}

//just for convention sake
void freeProcNap(struct processNap* procNap)
{
	free(procNap);
}

//create new processNap with sleep time in ms
struct processNap* createProcNap(int durMs)
{
	struct processNap* procNap = allocProcNap();
	procNap->startTime = 0;
	procNap->durMs     = durMs;

	return procNap;

};

//kinda redundant, ik, but it may be usefull in the future
struct processNap* cloneProcNap(struct processNap* src)
{
	if (!src) return NULL;

	struct processNap* dst = allocProcNap();
	dst = src;
	return dst;
}

//get time the process has been napping for, in msecs
int getNapDelta(struct processNap* procNap)
{
	long now   = clockMsRT();
	int delta = now - procNap->startTime;

	return delta;
}

//updates nap of given process
int updateProcNap(struct process* proc)
{
	struct processNap* procNap = proc->procNap;
	int msecDelta = getNapDelta(procNap);
	int msecRem = procNap->durMs - msecDelta;

	//if process has napped for the specified duration, remove the nap struct
	if (msecDelta >= procNap->durMs)
	{
		freeProcNap(procNap);
		proc->procNap = NULL;
		return 0;
	}
	
	//return the remaining naptime
	return msecRem;
}

//put process to nap for specified amount of msecs
void procNap(int durMs, struct process* proc)
{
	struct processNap* procNap = createProcNap(durMs);
	procNap->startTime = clockMsRT(); //set start time, so the task switcher knows when the nap started
	proc->procNap = procNap;
}
