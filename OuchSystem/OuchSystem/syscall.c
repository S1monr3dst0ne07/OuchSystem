

#include "files.h"
#include "kernal.h"
#include "syscall.h"
#include "vm.h"

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
    //meta
    switch (stm->type)
    {
    case stmTypSocket:
        free(stm->meta);
        break;
    default:
    }

    free(stm->readContent);
    free(stm);
}

void removeStream(struct stream* stm, struct streamPool* river, int id)
{
    river->count--;
    freeStream(stm);
    river->container[id2i(id)] = NULL;
}

void freeStreamPool(struct system* ouch)
{
    struct streamPool* river = ouch->river;

    if (river->count)
    {        
        struct stream* temp;
        for (int i = 0; i < riverListSize; i++)
            if ((temp = river->container[i]))
                freeStream(temp);
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
struct stream* createStream(unsigned char* content, int len)
{
    if (!content)
    {
        content = (unsigned char*)malloc(sizeof(char));
        *content = 0x0;
    }

    struct stream* stm = (struct stream*)malloc(sizeof(struct stream));
    stm->readContent = content;
    stm->readSize = len;

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
            return (stm->id = i2id(i)); //S1Int id is currently not in use
            //return i2id(i);
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

bool sendStream(struct stream* stm, S1Int ptr, int size, struct process* proc)
{

    bool succ = (ptr + size < Bit16IntLimit) &&             //src bounds
                (stm->writeIndex + size < streamOutputSize); //dst bounds
    if (succ)
    {
        S1Int* srcPtr = &proc->mem[ptr];
        int charSize = size * 2;

        memcpy(stm->writeContent + stm->writeIndex, (char*)srcPtr, charSize);
        stm->writeIndex += charSize;
    }

    return succ;
}



void updateNetwork(struct stream* stm, int index, struct streamPool* river)
{
    char buffer[networkBufferSize];

    //read socket discriptor
    const int socketFd = *(int*)stm->meta;

    //read
    memset(buffer, 0x0, networkBufferSize);
    int recvByteSize = recv(socketFd, buffer, networkBufferSize, MSG_DONTWAIT);
    int recvErrno = errno;

    if (0 < recvByteSize)
    {
        int readRemain = stm->readSize - stm->readIndex;

        //alloc new content
        int readSize = recvByteSize + readRemain;
        unsigned char* readContentNew = (unsigned char*)malloc(readSize);
        fguard(readContentNew, msgMallocGuard, );

        memset(readContentNew, 0x0, readSize);

        unsigned char* p = readContentNew;
        memcpy(p, stm->readContent + stm->readIndex, readRemain);
        memcpy(p + readRemain, buffer, recvByteSize);

        //inject
        free(stm->readContent);
        stm->readSize = readSize;
        stm->readContent = readContentNew;
        stm->readIndex = 0;
    }
    else if (recvErrno != EAGAIN || recvByteSize == 0)//kill stream if socket error
    {
        //free((int*)stm->meta); //important: free socket fd
        
        removeStream(stm, river, i2id(index));
        close(socketFd);

        //river->count--;
        //freeStream(stm);
        //river->container[index] = NULL;
        return;
    }

    //write
    if (stm->writeIndex > 0 &&
        0 <= send(socketFd, stm->writeContent, stm->writeIndex, 0))
    {
        memset(stm->writeContent, 0x0, streamOutputSize);
        stm->writeIndex = 0;
    }

}

void updateStreams(struct system* ouch)
{
    struct streamPool* river = ouch->river;

    //iterate river and count streams
    for (int i = 0, c = 0; c < river->count && i < riverListSize; i++)
    {
        struct stream* stm = getStream(i2id(i), ouch);
        if (!stm) continue;

        //incmentent count of stream if found
        c++;

        switch (stm->type)
        {
        case stmTypSocket:
            updateNetwork(stm, i, river);
            break;

        case stmTypPipe:
            //printf("stmTypPipe UNIMPL!!!!\n");
            break;

        case stmTypRootProc:
            for (int i = 0; i < stm->writeIndex; i++)
                flog("%c", stm->writeContent[i]);
            stm->writeIndex = 0;
            break;

        default:
            break;

        }



    }
}



void logStackCorrSc(enum S1Syscall sc)
{
    flog("Syscall 0x%x: Stack corrupted\n", sc);
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


#define guardPull(x) if (!processStackPull(proc, &x)) { logStackCorrSc(callType); break; }
#define guardPush(x) if (!processStackPush(proc, &x)) { logStackCorrSc(callType); break; }


void runSyscall(enum S1Syscall callType, struct process* proc, struct system* ouch)
{
    struct streamPool* river = ouch->river;

    S1Int id, pid, ptr = 0;
    
    struct stream* stm;
    S1Int success;
    S1Int data = 0;
    struct sockaddr_in netAddr = { 0 };
    struct process* procNew;

    switch (callType)
    {
    case scNoop:
        break;

    //--- streams ---
    case scCloseStm:;
        S1Int doWriteBack = 0;
        guardPull(doWriteBack);
        guardPull(id);

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
                    //success = writeFileContent(ouch, path, stm->writeContent);
                    struct file f = { .contLen = stm->writeIndex, .contPtr = stm->writeContent };
                    success = writeFile(ouch, path, f);
                    freeFilePath(path);
                }

                free(stm->meta);
                break;
            case stmTypSocket:;
                //close socket
                int* sockPtr = (int*)stm->meta;
                success = (0 <= close(*sockPtr));
                //free(sockPtr);
                break;
            default:
                success = true;
                break;
            }

            //free and reset container
            removeStream(stm, river, id);

        }
        else success = false;        

        //if (!syscallStackPush(proc, &success, callType)) break;
        guardPush(success);
        break;

    case scReadStm:;
        updateStreams(ouch);

        guardPull(id);

        stm = getStream(id, ouch);
        if (stm)
            success = readStream(stm, &data) ? 1 : 2;
        else
            success = 0;

        guardPush(data);
        guardPush(success);
        break;

    case scWriteStm:;
        guardPull(data);
        guardPull(id);

        stm = getStream(id, ouch);
        success = (bool)stm && writeStream(stm, data);

        guardPush(success);

        updateStreams(ouch);
        break;
    
    case scStmInfo:;
        //if (!syscallStackPull(proc, &id, callType)) break;
        guardPull(id);
        stm = getStream(id, ouch);

        S1Int type = (S1Int)(stm ? stm->type : stmInvailed);
        S1Int size = (S1Int)(stm ? stm->readSize : 0);

        //if (!syscallStackPush(proc, &size, callType)) break;
        //if (!syscallStackPush(proc, &type, callType)) break;
        guardPush(size);
        guardPush(type);
        break;

    case scStmSend:;
        guardPull(size);
        guardPull(ptr);
        guardPull(id);

        stm = getStream(id, ouch);
        success = (bool)stm && sendStream(stm, ptr, size, proc);

        guardPush(success);

        updateStreams(ouch);
        break;




    //--- files ---
    case scOpenFileObj:;
        S1Int pathPtr = 0;
        //if (!syscallStackPull(proc, &pathPtr, callType)) break;
        guardPull(pathPtr);

        char* pathStr = readStringFromProcessMemory(proc, pathPtr);
        struct filePath* path = parseFilePath(pathStr);

        //char* content = readFileContent(ouch, path);
        struct file f = readFile(ouch, path);

        if (f.contPtr)
        {
            struct stream* stm = createStream((unsigned char*)f.contPtr, f.contLen);
            stm->type = stmTypFile;
            stm->meta = pathStr;
            id = injectStream(stm, ouch);
        }
        else
            id = 0x0;

        //if (!syscallStackPush(proc, &id, callType)) break;
        guardPush(id);

        freeFilePath(path);
        break;

    //--- time ---
    case scNapMs:;
        S1Int durMs = 0;
        //if (!syscallStackPull(proc, &durMs, callType)) break;
        guardPull(durMs);
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
            //if (!syscallStackPush(proc, &timeParts[i], callType)) break;
            guardPush(timeParts[i]);

        #undef timePartsSize
        break;

    //--- network ---
    case scBindPort:;
        S1Int port = 0;
        S1Int queueSize = 0;
        success = true;

        //if (!syscallStackPull(proc, &queueSize, callType)) break;
        //if (!syscallStackPull(proc, &port, callType)) break;
        guardPull(queueSize);
        guardPull(port);


        //set sockaddr_in
        netAddr.sin_port = htons(port);
        netAddr.sin_family = AF_INET;
        netAddr.sin_addr.s_addr = INADDR_ANY;

        //bind
        if (bind(proc->procSock, (struct sockaddr*)&netAddr, sizeof(netAddr)) < 0)
            success = false;

        //listen
        if (listen(proc->procSock, queueSize) < 0) 
            success = false;

        //if (!syscallStackPush(proc, &success, callType)) break;
        guardPush(success);
        break;

    case scAcctSock:;
        netAddr = (struct sockaddr_in){.sin_addr.s_addr = 0x0};
        int addrlen = sizeof(netAddr);

        int sock = accept(proc->procSock, (struct sockaddr*)& netAddr, (socklen_t*)&addrlen);

        if (0 <= sock)
        {
            //generate pointer to socket for stream, metadata of stream is only stored as void ptr
            int* sockPtr = malloc(sizeof(int));
            *sockPtr = sock;

            stm = createStream(NULL, 0);
            stm->type = stmTypSocket;
            stm->meta = sockPtr;
            id = injectStream(stm, ouch);
        }
        else
            id = 0x0;


        unsigned int ip = netBitOrderFix(netAddr.sin_addr.s_addr);
        unsigned short ipLow  = ip         & 0xFFFF;
        unsigned short ipHigh = (ip >> 16) & 0xFFFF;

        guardPush(ipHigh);
        guardPush(ipLow);
        guardPush(id);
        break;
    
    case scConnect:;
        guardPull(ipHigh);
        guardPull(ipLow);
        guardPull(port);

        ip = ipLow | (ipHigh << 16);

        netAddr.sin_port = htons(port);
        netAddr.sin_family = AF_INET;
        netAddr.sin_addr.s_addr = netBitOrderFix(ip);

        sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

        if (0 <= sock && 
            connect(sock, (struct sockaddr*)(&netAddr), sizeof(struct sockaddr_in)) < 0)
        {
            stm = createStream(NULL, 0);
            stm->type = stmTypSocket;
            stm->meta = malloc(sizeof(int));
            *(int*)(stm->meta) = sock;

            id = injectStream(stm, ouch);
        }
        else
            id = 0x0;

        guardPush(id);
        break;


    //--- process ---
    case scGetPid:;
        pid = (S1Int)proc->pid;
        //if (!syscallStackPush(proc, &pid, callType)) break;
        guardPush(pid);
        break;

    case scLaunchProc:;
        guardPull(pathPtr);

        pathStr = readStringFromProcessMemory(proc, pathPtr);
        procNew = launchPath(pathStr, ouch);

        success = (bool)(procNew)&1;
        pid = procNew ? procNew->pid : 0;

        guardPush(pid);
        guardPush(success);

        free(pathStr);
        break;

    case scForkProc:;
        procNew = cloneProcess(proc);
        launchProcess(procNew, ouch);

        //adjust fork depth
        procNew->forkDepth++;

        //sub process pid
        pid = (S1Int)procNew->pid;
        if (!syscallStackPush(procNew, &pid, callType)) break;

        //super process pid
        pid = (S1Int)proc->pid;
        if (!syscallStackPush(proc, &pid, callType)) break;

        break;

    case scMMap:;
        S1Int offset, addr = 0;

        guardPull(offset);
        guardPull(size);
        guardPull(addr);
        guardPull(pathPtr);

        pathStr = readStringFromProcessMemory(proc, pathPtr);
        struct fileMap* fmap = createFileMap(pathStr, size, addr, offset);
        free(pathStr);

        //check if filePath is valid
        if ((success = isFile(ouch, fmap->filePath)))
            injectFileMap(proc, fmap);
        else
            freeFileMaps(fmap);

        guardPush(success);

        break;

    case scTSL:;
        guardPull(addr);
        
        S1Int* ptr = &(proc->mem[addr]);
        S1Int copy = *ptr;
        *ptr = 0x0;

        saveFileMap(proc, proc->fMaps, ouch);
        guardPush(copy);

        break;

    default:
        break;

    }


}
