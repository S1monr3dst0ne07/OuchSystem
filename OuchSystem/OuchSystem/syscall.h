#ifndef HSYSCALL
#define HSYSCALL

enum S1Syscall
{
    scNoop = 0,
};


void runSyscall(enum S1Syscall callType, struct system* ouch);

#endif