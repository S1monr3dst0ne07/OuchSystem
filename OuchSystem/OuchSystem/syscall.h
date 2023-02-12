#ifndef HSYSCALL
#define HSYSCALL

#include "utils.h"
#include "process.h"

#include <stdlib.h>

#define streamOutputSize (1 << 16)
#define riverListSize 65535
#define networkBufferSize 1024

#define i2id(x) x + 1
#define id2i(x) x - 1

enum streamType
{
    stmInvailed = 0,
    stmTypFile = 1,
    stmTypDir,
    stmTypSocket,
};

struct stream
{
    //S1Int id;

    char* readContent;
    int readSize;
    int readIndex;

    char writeContent[streamOutputSize];
    int writeIndex;

    enum streamType type;

    //general metadata
    // stmTypFile   -> file path
    // stmTypSocket -> socket fd 
    void* meta;
};


//contains stream, id given by the position in container
//id and array index are synonymus (index 0 -> id 1)
struct streamPool
{
    struct stream* container[riverListSize];
    int count;
};

struct streamPool* allocStreamPool();
void freeStreamPool(struct system* ouch);
void runSyscall(enum S1Syscall callType, struct process* proc, struct system* ouch);
void updateStreams(struct system* ouch);

#endif