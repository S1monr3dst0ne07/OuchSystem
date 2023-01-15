#include "syscall.h"

#include "files.h"
#include "kernal.h"


struct streamPool* allocStreamPool()
{
    return (struct streamPool*)malloc(sizeof(struct streamPool));
}

void freeStreamPool(system)
{
    printf("todo: free stream pool");
}

//allocates new stream, content must be zero terminated
struct stream* createStream(S1Int* content)
{
    struct stream* stm = (struct stream*)malloc(sizeof(struct stream));
    stm->readContent = content;
    stm->readSize = strlen(content);

    stm->readIndex  = 0;
    stm->writeIndex = 0;


    return stm;
}

void injectStream(struct stream* stm, struct system* ouch)
{
    struct streamPool* pool = ouch->river;
    struct streamList* last = pool->head;

    //new streamList
    struct streamList* newStreamList = (struct streamList*)malloc(sizeof(struct streamList));

    //retarget
    pool->head = newStreamList;
    if (last) last->prev = newStreamList;

    newStreamList->next = last;
    newStreamList->prev = NULL;

    //inject stream
    newStreamList->stm = stm;
}


void runSyscall(enum S1Syscall callType, struct process* proc, struct system* ouch)
{
    sprintf(cTemp, "Syscall %x\n", callType);
    log(cTemp);

    switch (callType)
    {
    case scNoop:
        break;

    }

}
