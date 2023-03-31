
#include "process.h"
#include "timing.h"


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


//get time the process has been napping for, in msecs
int getNapDelta(struct processNap* procNap)
{
	clock_t now   = clock();
	clock_t delta = now - procNap->startTime;
	int      msec = delta * 1000 / CLOCKS_PER_SEC;

	return msec;
}

//updates nap of given process
void updateProcNap(struct process* proc)
{
	struct processNap* procNap = proc->procNap;
	int msecDelta = getNapDelta(procNap);

	//if process has napped for the specified duration, remove the nap struct
	if (msecDelta >= procNap->durMs)
	{
		freeProcNap(procNap);
		proc->procNap = NULL;
	}

}

//put process to nap for specified amount of msecs
void procNap(int durMs, struct process* proc)
{
	struct processNap* procNap = createProcNap(durMs);
	procNap->startTime = clock(); //set start time, so the task switcher knows when the nap started
	proc->procNap = procNap;
}
