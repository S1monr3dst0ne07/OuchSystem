#ifndef HSYSCALL
#define HSYSCALL

#include <stdlib.h>

#include "process.h"
#define streamOutputSize (1 << 17)
#define riverListSize 65535

#define i2id(x) x + 1
#define id2i(x) x - 1

struct stream
{
    S1Int id;

    S1Int* readContent;
    int readSize;
    int readIndex;

    S1Int writeContent[streamOutputSize];
    int writeIndex;
};


//contains stream, id given by the position in container
//id and array index are synonymus (index 0 -> id 1)
struct streamPool
{
    struct stream* container[riverListSize];
    int count;
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