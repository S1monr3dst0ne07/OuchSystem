#include "syscall.h"

#include "files.h"
#include "kernal.h"


struct streamPool* allocStreamPool()
{
    struct streamPool* stmPool = (struct streamPool*)malloc(sizeof(struct streamPool));
    stmPool->idIndex = 1;
    stmPool->head = NULL;

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

//remove stream and dealloc
void removeStreamList(struct streamList* stmLst, struct system* ouch)
{
    struct stream* stm = stmLst->stm;
    struct streamPool* river = ouch->river;

    //link previouse to next
    if (stmLst->prev)
        stmLst->prev->next = stmLst->next;
    else
        river->head = stmLst->next;


    //link next to previouse 
    if (stmLst->next)
        stmLst->next->prev = stmLst->prev;

    freeStream(stm);
    free(stmLst);
}

struct streamList* findStreamList(S1Int id, struct system* ouch)
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

struct stream* findStream(S1Int id, struct system* ouch)
{ return findStreamList(id, ouch)->stm; }


void runSyscall(enum S1Syscall callType, struct process* proc, struct system* ouch)
{
    printf("SID: %d\n", ouch->river->idIndex);

    sprintf(cTemp, "Syscall %x\n", callType);
    log(cTemp);

    S1Int id = 0;
    switch (callType)
    {
    case scNoop:
        break;

    case scCloseStm:;
        if (!processStackPull(proc, &id))
        {
            log("Syscall scCloseStm: Stack corrupted");
            break;
        }

        struct streamList* stmLst = findStreamList(id, ouch);
        if (!stmLst) break;

        //todo: check for writeback
        
        removeStreamList(stmLst, ouch);
        break;

    case scOpenFileObj:;
        S1Int pathPtr = 0;
        if (!processStackPull(proc, &pathPtr))
        {
            log("Syscall scOpenFileObj: Stack corrupted");
            break;
        }

        char* pathStr = readStringFromProcessMemory(proc, pathPtr);
        struct filePath* path = parseFilePath(pathStr);
        S1Int* content = (S1Int*)readFileContent(ouch, path);

        printf("ptr: %d\n", pathPtr);
        printf("path: %s\n", pathStr);
        printf("content: %s\n", content);

        if (content)
        {
            struct stream* stm = createStream(content);
            id = injectStream(stm, ouch);
        }
        else
            id = 0x0;

        processStackPush(proc, &id);

        freeFilePath(path);
        free(pathStr);
        break;
    }


}
