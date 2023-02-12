

#include "files.h"
#include "kernal.h"
#include "syscall.h"

struct streamPool* allocStreamPool()
{
    struct streamPool* stmPool = (struct streamPool*)malloc(sizeof(struct streamPool));
    stmPool->count = 0;    
    memset(stmPool->container, 0x0, riverListSize * sizeof(struct stream*));

    return stmPool;
}

void freeStream(struct stream* stm)
{
    free(stm->readContent);
    free(stm->meta);
    free(stm);
}

void freeStreamPool(struct system* ouch)
{
    struct streamPool* river = ouch->river;

    if (river->count)
    {        
        struct stream* temp;
        for (int i = 0; i < riverListSize; i++)
            if (temp = river->container[i]) freeStream(temp);
    }

    free(river);
}

bool isVaildStream(S1Int id, struct system* ouch)
{
    //check for empty stream
    if (!id) return false;

    //check for stream
    return (bool)ouch->river->container[id2i(id)];
}

//allocates new stream, content must be zero terminated
struct stream* createStream(char* content)
{
    struct stream* stm = (struct stream*)malloc(sizeof(struct stream));
    stm->readContent = content;
    stm->readSize = strlen(content);

    stm->readIndex  = 0;
    stm->writeIndex = 0;

    stm->type = stmInvailed;

    memset(stm->writeContent, 0x0, streamOutputSize * sizeof(char));

    return stm;
}

S1Int injectStream(struct stream* stm, struct system* ouch)
{
    struct streamPool* river = ouch->river;

    //search for free spot
    for (S1Int i = 0; i < riverListSize; i++)
        if (!river->container[i])
        {
            river->count++;
            river->container[i] = stm;
            //return (stm->id = i2id(i)); S1Int id is currently not in use
            return i2id(i);
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

bool readStream(struct stream* stm, S1Int* data)
{
    //read only succeeds if there's remainder in readContent
    bool succ = (stm->readIndex < stm->readSize);
    *data = succ ? (S1Int)stm->readContent[stm->readIndex++] : 0;
    return succ;
}

bool writeStream(struct stream* stm, S1Int val)
{
    //write only succeeds if there's space in the writeContent
    bool succ = (stm->writeIndex < streamOutputSize);
    if (succ) stm->writeContent[stm->writeIndex++] = (char)val;
    return succ;
}

void updateStreams(struct system* ouch)
{
    struct streamPool* river = ouch->river;
    char buffer[networkBufferSize];


    //iterate river
    for (int i = 0; i < river->count; i++)
    {
        struct stream* stm = river->container[i];
        if (stm->type != stmTypSocket) continue;

        //read
        memset(buffer, 0x0, networkBufferSize);
        if (0 <= recv(stm->meta, buffer, networkBufferSize, MSG_DONTWAIT))
        {
            int bufferSize = strlen(buffer);
            int readDelta = stm->readSize - stm->readIndex;

            //alloc new content
            int readSize = bufferSize + readDelta;
            char* readContentNew = (char*)malloc(sizeof(char) * readSize);
            sprintf(readContentNew, "%s%s", stm->readContent+stm->readIndex, buffer);

            //inject
            free(stm->readContent);
            stm->readSize  = readSize;
            stm->readIndex = 0;
        }

        //write
        if (0 <= send(stm->meta, stm->writeContent, strlen(stm->writeContent), 0))
        {
            memset(stm->writeContent, 0x0, streamOutputSize);
            stm->writeIndex = 0;
        }



    }
}


void logStackCorrSc(enum S1Syscall sc)
{
    sprintf(cTemp, "Syscall 0x%x: Stack corrupted\n", sc);
    logg(cTemp);
}

bool syscallStackPull(struct process* proc, S1Int* val, enum S1Syscall callType)
{
    bool succ = processStackPull(proc, val);
    if (!succ) logStackCorrSc(callType);
    return succ;
}
bool syscallStackPush(struct process* proc, S1Int* val, enum S1Syscall callType)
{
    bool succ = processStackPush(proc, val);
    if (!succ) logStackCorrSc(callType);
    return succ;
}


void runSyscall(enum S1Syscall callType, struct process* proc, struct system* ouch)
{
    struct streamPool* river = ouch->river;
    //sprintf(cTemp, "Syscall 0x%x\n", callType);
    //logg(cTemp);

    S1Int id = 0;
    struct stream* stm;
    S1Int success;
    S1Int data;
    struct sockaddr_in netAddr;

    switch (callType)
    {
    case scNoop:
        break;

    case scCloseStm:;
        S1Int doWriteBack = 0;
        if (!syscallStackPull(proc, &doWriteBack, callType)) break;
        if (!syscallStackPull(proc, &id, callType)) break;

        stm = getStream(id, ouch);
        if (stm)
        {
            //writeback
            
            if (doWriteBack) switch (stm->type)
            {
            case 0:
                break;
            case 1:;
                struct filePath* path = parseFilePath(stm->meta);
                success = writeFileContent(ouch, path, stm->writeContent);
                freeFilePath(path);

                break;
            default:
                success = true;
                break;
            }

            //free and reset container
            river->count--;
            freeStream(stm);
            river->container[id2i(id)] = NULL;
        }
        else success = false;        

        if (!syscallStackPush(proc, &success, callType)) break;
        break;

    case scReadStm:;
        if (!syscallStackPull(proc, &id, callType)) break;

        stm = getStream(id, ouch);
        if (stm)
            success = readStream(stm, &data) ? 1 : 2;
        else
            success = 0;

        if (!syscallStackPush(proc, &data, callType)) break;
        if (!syscallStackPush(proc, &success, callType)) break;
        break;

    case scWriteStm:;
        if (!syscallStackPull(proc, &data, callType)) break;
        if (!syscallStackPull(proc, &id, callType)) break;

        stm = getStream(id, ouch);
        success = (bool)stm && writeStream(stm, data);

        if (!syscallStackPush(proc, &success, callType)) break;
        break;
    
    case scStmInfo:;
        if (!syscallStackPull(proc, &id, callType)) break;
        stm = getStream(id, ouch);

        S1Int type = (S1Int)(stm ? stm->type : stmInvailed);
        S1Int size = (S1Int)(stm ? stm->readSize : 0);

        if (!syscallStackPush(proc, &size, callType)) break;
        if (!syscallStackPush(proc, &type, callType)) break;
        break;


    case scOpenFileObj:;
        S1Int pathPtr = 0;
        if (!syscallStackPull(proc, &pathPtr, callType)) break;

        char* pathStr = readStringFromProcessMemory(proc, pathPtr);
        struct filePath* path = parseFilePath(pathStr);
        char* content = readFileContent(ouch, path);

        if (content)
        {
            struct stream* stm = createStream(content);
            stm->type = stmTypFile;
            stm->meta = pathStr;
            id = injectStream(stm, ouch);
        }
        else
            id = 0x0;

        if (!syscallStackPush(proc, &id, callType)) break;

        freeFilePath(path);
        break;
    
    case scBindPort:;
        S1Int port = 0;
        S1Int queueSize = 0;
        success = true;

        if (!syscallStackPull(proc, &queueSize, callType)) break;
        if (!syscallStackPull(proc, &port, callType)) break;

        //set sockaddr_in
        netAddr = proc->netAddr;
        netAddr.sin_port = htons(port);

        //bind
        if (bind(proc->procSock, (struct sockaddr*)&netAddr, sizeof(netAddr)) < 0)
            success = false;

        //listen
        if (listen(proc->procSock, queueSize) < 0) 
            success = false;

        if (!syscallStackPush(proc, &success, callType)) break;
        break;

    case scAcctSock:;
        netAddr = proc->netAddr;
        int addrlen = sizeof(netAddr);

        int sock = accept(proc->procSock, (struct sockaddr*)& netAddr, (socklen_t*)&addrlen);

        if (0 <= sock)
        {
            struct stream* stm = createStream(content);
            stm->type = stmTypSocket;
            stm->meta = sock;
            id = injectStream(stm, ouch);
        }
        else
            id = 0x0;


        if (!syscallStackPush(proc, &id, callType)) break;
        break;
    }


}
