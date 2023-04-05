

#include "files.h"
#include "kernal.h"
#include "syscall.h"

#include <errno.h>
#include <string.h>

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
    free(stm);
}

void freeStreamPool(struct system* ouch)
{
    struct streamPool* river = ouch->river;

    if (river->count)
    {        
        struct stream* temp;
        for (int i = 0; i < riverListSize; i++)
            if ((temp = river->container[i])) freeStream(temp);
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
        if (!isVaildStream(i2id(i), ouch)) continue;
        struct stream* stm = river->container[i];
        if (stm->type != stmTypSocket) continue;

        //read socket discriptor
        const int socketFd = *(int*)stm->meta;

        //read
        memset(buffer, 0x0, networkBufferSize);
        int recvByteSize = recv(socketFd, buffer, networkBufferSize, MSG_DONTWAIT);
        int recvErrno = errno;

        if (0 < recvByteSize)
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
            stm->readContent = readContentNew;
            stm->readIndex = 0;
        }
        else if (recvErrno != EAGAIN || recvByteSize == 0)//kill stream if socket error
        {
            river->count--;
            freeStream(stm);
            river->container[i] = NULL;
            continue;
        }


        //write
        if (stm->writeIndex > 0 &&
            0 <= send(socketFd, stm->writeContent, strlen(stm->writeContent), 0))
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
bool syscallStackPush(struct process* proc, const S1Int* val, enum S1Syscall callType)
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

    S1Int id, pid = 0;
    struct stream* stm;
    S1Int success;
    S1Int data = 0;
    struct sockaddr_in netAddr;

    switch (callType)
    {
    case scNoop:
        break;

    //--- streams ---
    case scCloseStm:;
        S1Int doWriteBack = 0;
        if (!syscallStackPull(proc, &doWriteBack, callType)) break;
        if (!syscallStackPull(proc, &id, callType)) break;

        stm = getStream(id, ouch);
        if (stm)
        {
            
            switch (stm->type)
            {
            case stmInvailed:
                break;
            case stmTypFile:;
                //writeback
                if (doWriteBack)
                {
                    struct filePath* path = parseFilePath(stm->meta);
                    success = writeFileContent(ouch, path, stm->writeContent);
                    freeFilePath(path);
                }

                free(stm->meta);
                break;
            case stmTypSocket:;
                //close socket
                int* sockPtr = (int*)stm->meta;
                success = (0 <= close(*sockPtr));
                free(sockPtr);
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
        updateStreams(ouch);

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

        updateStreams(ouch);
        break;
    
    case scStmInfo:;
        if (!syscallStackPull(proc, &id, callType)) break;
        stm = getStream(id, ouch);

        S1Int type = (S1Int)(stm ? stm->type : stmInvailed);
        S1Int size = (S1Int)(stm ? stm->readSize : 0);

        if (!syscallStackPush(proc, &size, callType)) break;
        if (!syscallStackPush(proc, &type, callType)) break;
        break;

    //--- files ---
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

    //--- time ---
    case scNapMs:;
        S1Int durMs = 0;
        if (!syscallStackPull(proc, &durMs, callType)) break;
        procNap(durMs, proc);
        break;

    case scFLocTime:;
        time_t now = time(NULL);
        struct tm* time = localtime(&now);

        //pull time into array and cast
        #define timePartsSize 8
        #define t(x) (S1Int)time->x
        const S1Int timeParts[timePartsSize] = {
            t(tm_sec),  t(tm_min),  t(tm_hour),
            t(tm_wday), t(tm_mday), t(tm_yday),
            t(tm_mon) + 1, t(tm_year) + 1900
        };
        #undef t

        for (int i = timePartsSize - 1; i >= 0; i--)
            if (!syscallStackPush(proc, &timeParts[i], callType)) break;
        
        #undef timePartsSize
        break;

    //--- network ---
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
            //generate pointer to socket for stream, metadata of stream is only stored as void ptr
            int* sockPtr = malloc(sizeof(int));
            *sockPtr = sock;

            char* content = (char*)malloc(sizeof(char));
            content[0] = '\x0';

            struct stream* stm = createStream(content);
            stm->type = stmTypSocket;
            stm->meta = sockPtr;
            id = injectStream(stm, ouch);
        }
        else
            id = 0x0;


        if (!syscallStackPush(proc, &id, callType)) break;
        break;
    
    case scGetPid:;
        pid = (S1Int)proc->pid;
        if (!syscallStackPush(proc, &pid, callType)) break;
        break;


    case scForkProc:;
        struct process* procNew = cloneProcess(proc);
        launchProcess(procNew, ouch);

        //sub process pid
        pid = (S1Int)procNew->pid;
        if (!syscallStackPush(procNew, &pid, callType)) break;

        //super process pid
        pid = (S1Int)proc->pid;
        if (!syscallStackPush(proc, &pid, callType)) break;

        break;

    default:
        break;

    }


}
