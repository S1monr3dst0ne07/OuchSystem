
#include "syscall.h"

#include "files.h"
#include "kernal.h"


struct streamPool* allocStreamPool()
{
    struct streamPool* stmPool = (struct streamPool*)malloc(sizeof(struct streamPool));
    stmPool->count = 0;
    memset(stmPool->container, 0x0, riverListSize * sizeof(struct stream*));

    return stmPool;
}

void freeStreamPool(struct system* ouch)
{
    printf("todo: free stream pool");
}

void freeStream(struct stream* stm)
{
    free(stm->readContent);
    free(stm);
}

bool isVaildStream(S1Int id, struct system* ouch)
{
    //check for empty stream
    if (!id) return false;

    //check for stream
    return (bool)ouch->river->container[id2i(id)];
}

//allocates new stream, content must be zero terminated
struct stream* createStream(S1Int* content)
{
    struct stream* stm = (struct stream*)malloc(sizeof(struct stream));
    stm->readContent = content;
    stm->readSize = strlen(content);

    stm->readIndex  = 0;
    stm->writeIndex = 0;

    memset(stm->writeContent, 0x0, streamOutputSize * sizeof(S1Int));

    return stm;
}

S1Int injectStream(struct stream* stm, struct system* ouch)
{
    struct streamPool* river = ouch->river;

    //search for free spot
    for (S1Int i = 0; i < riverListSize; i++)
        if (!river->container[i])
        {
            river->container[i] = stm;
            return (stm->id = i2id(i));
        }

    //if no spot was found, deallocate the stream
    freeStream(stm);
    return 0x0;
}


struct stream* getStream(S1Int id, struct system* ouch)
{
    if (!isVaildStream(id, ouch)) return NULL;
    return ouch->river->container[id2i(id)];
}


void logStackCorrSc(enum S1Syscall sc)
{
    sprintf(cTemp, "Syscall 0x%x: Stack corrupted\n", sc);
    log(cTemp);
}

void runSyscall(enum S1Syscall callType, struct process* proc, struct system* ouch)
{
    struct streamPool* river = ouch->river;


    sprintf(cTemp, "Syscall 0x%x\n", callType);
    log(cTemp);

    S1Int id = 0;
    switch (callType)
    {
    case scNoop:
        break;

    case scCloseStm:;
        if (!processStackPull(proc, &id))
        { logStackCorrSc(callType); break; }

        struct stream* stm = getStream(id, ouch);
        bool success = (bool)stm;
        if (success)
        {
            //todo: check for writeback

            //free and reset container
            free(stm);
            river->container[id2i(id)] = NULL;
        }
        
        if (!processStackPush(proc, &success))
        { logStackCorrSc(callType); break; }

        break;

    case scOpenFileObj:;
        S1Int pathPtr = 0;
        if (!processStackPull(proc, &pathPtr))
        { logStackCorrSc(callType); break; }

        char* pathStr = readStringFromProcessMemory(proc, pathPtr);
        struct filePath* path = parseFilePath(pathStr);
        S1Int* content = (S1Int*)readFileContent(ouch, path);

        if (content)
        {
            struct stream* stm = createStream(content);
            id = injectStream(stm, ouch);
        }
        else
            id = 0x0;

        if (!processStackPush(proc, &id))
        { logStackCorrSc(callType); break; }

        freeFilePath(path);
        free(pathStr);
        break;
    }


}
