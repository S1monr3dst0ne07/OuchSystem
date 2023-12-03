

//--- vm ---

//absolute hard minimum, under which the dynamic iterLimit
//can not fall
#define iterLimitMin 1


//number of total process iterations in one cycle of task switching
//therefore number of iters per proc is 
//iterLimit = procIterCycle / procCount
#define procIterCycle 65535

//so the more processes run, the less time each of them gets,
//to allow shorter processes the finish faster


//--- fork checker ---
//ouch has a anti-fork-overflow system, which protect the system
//crashing when a process start to fork uncontrollably, to prevent crashes

//fork-depth of a process to be flaged as dangerous
#define forkCheckerDepthThreshold 10

//number of process needing to run, for the fork checker
//to kick into action
#define forkCheckerProcThreshold 10000

