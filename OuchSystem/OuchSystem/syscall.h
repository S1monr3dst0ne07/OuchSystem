#ifndef HSYSCALL
#define HSYSCALL

#include <stdlib.h>

#include "process.h"
#define streamOutputSize (1 << 18)

struct stream
{
    S1Int id;

    S1Int* readContent;
    int readSize;
    int readIndex;

    S1Int writeContent[streamOutputSize];
    int writeIndex;
};

struct streamList
{
    struct stream* stm;
    struct streamList* prev;
    struct streamList* next;

};

struct streamPool
{
    struct streamList* head;
    int idIndex;
};


enum S1Syscall
{
    scNoop      = 0x0000,
    scCloseStm  = 0x0001,
    scReadStm,
    scWriteStm,
    scStmInfo,
    scOpenFileObj = 0x0010,
    scCreateObj,
    scDelObj,
    scSleepMs = 0x0020,
    scUnixEpoch,
    scFLocTime,
    scBindPort = 0x0030,
    scAcctSock,
    scCloseSock,

};

struct streamPool* allocStreamPool();
void runSyscall(enum S1Syscall callType, struct process* proc, struct system* ouch);

#endif