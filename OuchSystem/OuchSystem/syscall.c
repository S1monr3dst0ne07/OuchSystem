

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

struct stream* allocStream()
{
    return (struct stream*)malloc(sizeof(struct stream));
}



void freeStream(struct stream* stm)
{
    //meta
    switch (stm->type)
    {
    case stmTypSocket:
        free(stm->meta);
        break;

    case stmTypPipe:;
        struct stream* mrr = stm->meta;
        if (!mrr || mrr->type != stmTypPipe) break;

        free(mrr->readContent);
        free(mrr);

        break;

    default:
    }

    free(stm->readContent);
    free(stm);
}

void removeRiverEntry(struct streamPool* river, int id)
{
    river->count--;
    river->container[id2i(id)] = NULL;
}

void removeStream(struct stream* stm, struct streamPool* river)
{
    if (stm->type == stmTypPipe && stm->meta)
        removeRiverEntry(river, ((struct stream*)stm->meta)->id);

    removeRiverEntry(river, stm->id);
    freeStream(stm);
}

void freeStreamPool(struct system* ouch)
{
    struct streamPool* river = ouch->river;

    if (river->count)
    {        
        struct stream* temp;
        for (int i = 0; i < riverListSize; i++)
            if ((temp = river->container[i]))
                removeStream(temp, ouch->river);
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
struct stream* createStream(char* content, int len)
{
    if (!content)
    {
        content = (char*)malloc(sizeof(char));
        *content = 0x0;
    }

    struct stream* stm = allocStream();
    stm->readContent = content;
    stm->readSize = len;

    stm->readIndex  = 0;
    stm->writeIndex = 0;

    stm->type = stmInvailed;

    memset(stm->writeContent, 0x0, streamBufferSize * sizeof(char));

    return stm;
}

//allocates two new streams and links them using stmTypPipe
struct stream* createPipe()
{
    struct stream* stm = allocStream();
    struct stream* mrr = allocStream(); //mirror
    stm->type = stmTypPipe;
    mrr->type = stmTypPipe;

    stm->readIndex = 0;
    stm->writeIndex = 0;
    mrr->readIndex = 0;
    mrr->writeIndex = 0;


    stm->readContent = (char*)malloc(pipeBufferSize);
    mrr->readContent = (char*)malloc(pipeBufferSize);

    //link 'em up
    stm->meta = (void*)mrr;
    mrr->meta = (void*)stm;

    //mrr can be accessed through the link
    return stm;
}



S1Int injectStream(struct stream* stm, struct system* ouch)
{
    struct streamPool* river = ouch->river;
    struct stream** conts = river->container;

    //search for free spot
    for (S1Int i = 0; i < riverListSize; i++)
        if (conts[i] == stm) return 0; //stream already injected
        else if (!conts[i])
        {
            river->count++;
            river->container[i] = stm;

            //inject mirror if 
            if (stm->type == stmTypPipe)
                injectStream((struct stream*)stm->meta, ouch);

            return (stm->id = i2id(i));
        }

    //if no spot was found, deallocate the stream
    freeStream(stm);
    return 0;
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

//like read, but non-destructive
bool peekStream(struct stream* stm, S1Int* data)
{
    //read only succeeds if there's remainder in readContent
    bool succ = (stm->readIndex < stm->readSize);
    *data = succ ? (S1Int)stm->readContent[stm->readIndex] : 0;
    return succ;
}


bool writeStream(struct stream* stm, S1Int val)
{
    //write only succeeds if there's space in the writeContent
    bool succ = (stm->writeIndex < streamBufferSize);
    if (succ) stm->writeContent[stm->writeIndex++] = (char)val;
    return succ;
}

bool sendStream(struct stream* stm, S1Int ptr, int size, struct process* proc)
{

    bool succ = (ptr + size < Bit16IntLimit) &&              //src bounds
                (stm->writeIndex + size < streamBufferSize); //dst bounds
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
        char* readContentNew = (char*)malloc(readSize);
        fguard(readContentNew, msgMallocGuard, );

        memset(readContentNew, 0x0, readSize);

        char* p = readContentNew;
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
        
        removeStream(stm, river);
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
        memset(stm->writeContent, 0x0, streamBufferSize);
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

        case stmTypRootProc:
            for (int i = 0; i < stm->writeIndex; i++)
                flog("%c", stm->writeContent[i]);
            stm->writeIndex = 0;
            break;

        case stmTypPipe:;
            struct stream* mirror = (struct stream*)stm->meta;
            if (!mirror) break;

            int remSize = stm->readSize - stm->readIndex; //size of old remaining content
            int newSize = remSize + mirror->writeIndex;

            if (pipeBufferSize < newSize) break; //bounds

            memcpy(stm->readContent, stm->readContent + stm->readIndex, remSize);           //discard already read content
            memcpy(stm->readContent + remSize, mirror->writeContent, mirror->writeIndex);   //add new content from mirror

            stm->readIndex = 0;
            stm->readSize = newSize;
            mirror->writeIndex = 0;

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
        updateStreams(ouch);
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
            removeStream(stm, river);

        }
        else success = false;        

        guardPush(success);
        break;

    case scReadStm:;
        updateStreams(ouch);

        guardPull(id);

        stm = getStream(id, ouch);
        if (stm) success = readStream(stm, &data) ? 1 : 2;
        else     success = 0;

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
        guardPull(id);

        stm = getStream(id, ouch);

        S1Int type = (S1Int)(stm ? stm->type : stmInvailed);
        S1Int size = (S1Int)(stm ? stm->readSize : 0);

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


    case scStmProcStd:;
        guardPull(pid);

        procNew = getProcByPID(pid, ouch->pool);
        success = (bool)procNew;
        id = success ? ((struct stream*)procNew->stdio->meta)->id : 0x0;

        guardPush(id);
        guardPush(success);
        
        break;

    case scStmGetArgs:;
        struct stream* stm = createStream(proc->args, strlen(proc->args));
        stm->type = stmTypArgs;
        stm->meta = NULL;
        id = injectStream(stm, ouch);

        guardPush(id);
        break;

    case scStmGetWork:;
        char* pathStr = renderFilePath(proc->workPath);
        stm = createStream(pathStr, strlen(pathStr));
        stm->type = stmTypWork;
        stm->meta = NULL;
        id = injectStream(stm, ouch);

        guardPush(id);
        break;

    case scPeekStm:;
        updateStreams(ouch);

        guardPull(id);

        stm = getStream(id, ouch);
        if (stm) success = peekStream(stm, &data) ? 1 : 2;
        else     success = 0;

        guardPush(data);
        guardPush(success);
        break;


    //--- files ---
    case scOpenFileObj:;
        S1Int pathPtr = 0;
        guardPull(pathPtr);

        pathStr = readStringFromProcessMemory(proc, pathPtr);
        struct filePath* path = parseFilePath(pathStr);

        switch (getNodeTypeByPath(ouch, path))
        {
        case fileNodeFile:;
            //char* content = readFileContent(ouch, path);
            struct file f = readFile(ouch, path);

            if (f.contPtr)
            {
                struct stream* stm = createStream((char*)f.contPtr, f.contLen);
                stm->type = stmTypFile;
                stm->meta = pathStr;
                id = injectStream(stm, ouch);
            }
            else
                id = 0x0;

            break;

        case fileNodeDir:;
            char* list = listFileNode(ouch, path);
            struct stream* stm = createStream(list, strlen(list));
            stm->type = stmTypDir;
            stm->meta = NULL;
            id = injectStream(stm, ouch);
            break;

        case fileNodeInvaild:
            id = 0x0;
            break;

        }


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
        pid = proc->pid;
        //if (!syscallStackPush(proc, &pid, callType)) break;
        guardPush(pid);
        break;

    case scLaunchProc:;
        S1Int workPtr;
        S1Int argsPtr;
        guardPull(argsPtr);
        guardPull(workPtr);
        guardPull(pathPtr);

        pathStr       = readStringFromProcessMemory(proc, pathPtr);
        char* workStr = readStringFromProcessMemory(proc, workPtr);
        char* argsStr = readStringFromProcessMemory(proc, argsPtr);
        procNew = launchPath(pathStr, ouch, workStr, argsStr);

        success = (bool)(procNew)&1;
        pid = procNew ? procNew->pid : 0;

        guardPush(pid);
        guardPush(success);

        free(pathStr);
        free(workStr);
        break;

    case scForkProc:;
        procNew = cloneProcess(proc);
        launchProcess(procNew, ouch);

        //adjust fork depth
        procNew->forkDepth++;

        //sub process pid
        pid = procNew->pid;
        if (!syscallStackPush(procNew, &pid, callType)) break;

        //super process pid
        pid = proc->pid;
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
