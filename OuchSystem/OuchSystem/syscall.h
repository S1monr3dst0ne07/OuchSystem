#ifndef HSYSCALL
#define HSYSCALL

#include "process.h"
#define streamOutputSize 1 << 20

struct stream
{
    S1Int* readContent;
    int readSize;
    int readIndex;

    S1Int* writeContent[streamOutputSize];
    int writeIndex;
};

struct streamList
{
    struct stream stm;
    struct streamList* prev;
    struct streamList* next;

};

struct streamPool
{
    struct streamList* head;
    int streamCount;
    int idGen;
};


enum S1Syscall
{
    scNoop = 0,
};


void runSyscall(enum S1Syscall callType, struct process* proc, struct system* ouch);

#endif