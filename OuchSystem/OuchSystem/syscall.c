#include "syscall.h"

#include "files.h"
#include "kernal.h"


struct streamPool* allocStreamPool()
{
    struct streamPool* stmPool = (struct streamPool*)malloc(sizeof(struct streamPool));
    stmPool->idIndex = 1;
    return stmPool;
}

void freeStreamPool(struct system* ouch)
{
    printf("todo: free stream pool");
}

void freeStream(struct stream* stm)
{
    printf("todo: free stream");
    //todo: dealloc memory
}

//allocates new stream, content must be zero terminated
struct stream* createStream(S1Int* content)
{
    struct stream* stm = (struct stream*)malloc(sizeof(struct stream));
    stm->readContent = content;
    stm->readSize = strlen(content);

    stm->readIndex  = 0;
    stm->writeIndex = 0;

    memset(stm->writeContent, 0x0, streamOutputSize);

    return stm;
}

S1Int injectStream(struct stream* stm, struct system* ouch)
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

    //id
    stm->id = (S1Int)(pool->idIndex++);
    return stm->id;
}

struct stream* findStream(S1Int id, struct system* ouch)
{
    struct streamList* stmLst = ouch->river->head;
    while (stmLst)
    {
        struct stream* stm = stmLst->stm;
        if (stm->id == id) return stm;
        stmLst = stmLst->next;
    }

    return NULL;
}


void runSyscall(enum S1Syscall callType, struct process* proc, struct system* ouch)
{
    sprintf(cTemp, "Syscall %x\n", callType);
    log(cTemp);

    switch (callType)
    {
    case scNoop:
        break;

    case scOpenFileObj:;
        S1Int* pathPtr = 0;
        processStackPull(proc, &pathPtr);
        char* pathStr = readStringFromProcessMemory(proc, pathPtr);
        struct filePath* path = parseFilePath(pathStr);
        S1Int* content = (S1Int*)readFileContent(ouch, path);

        S1Int id = 0x0;
        if (content)
        {
            struct stream* stm = createStream(content);
            id = injectStream(stm, ouch);
        }

        processStackPush(proc, &id);

        freeFilePath(path);
        free(pathStr);
        break;
    }


}
