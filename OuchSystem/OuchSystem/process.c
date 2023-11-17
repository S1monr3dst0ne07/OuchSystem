
#include "timing.h"
#include "kernal.h"
#include "utils.h"
#include "syscall.h"
#include "process.h"
#include "vm.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>


#define endOfInst '\n'

//counter for uuids, only counts up
static unsigned long uuidCount = 1;


//c is a good language, but some parts are just so fucking dumb
//like why do have to do this shit when we want to make a mapper from string to enum
typedef struct
{
    enum s1Insts inst;
    const char* str;
} s1Entry;


struct labelMap
{
    char* label;
    int address;

};

struct rawInst
{
	char  op[rawInstBufferLimit];
	char arg[rawInstBufferLimit];

};

struct S1HeapChunk* allocS1HeapChunk()
{
    struct S1HeapChunk* temp = (struct S1HeapChunk*)malloc(sizeof(struct S1HeapChunk));
    temp->next = NULL;
    temp->ptr  = 0;
    temp->size = 0;
    return temp;
}

struct process* allocProcess()
{
    struct process* proc = (struct process*)malloc(sizeof(struct process));
    proc->ip = 0;
    proc->pid = (unsigned int)-1;

    proc->prog = NULL;
    proc->progSize = 0;

    memset(proc->mem, 0, S1IntBufferSize);
    memset(proc->stack, 0, S1IntBufferSize);
    proc->heap = allocS1HeapChunk();

    proc->stackPtr = 0;
    proc->acc = 0;
    proc->reg = 0;

    proc->procNap = NULL;

    proc->fMaps = NULL;

    return proc;
}

enum s1Insts str2s1(char* str)
{
    static s1Entry map[] = {
        {s1Set, "set"},
        {s1Add, "add"},
        {s1Sub, "sub"},
        {s1Shg, "shg"},
        {s1Shs, "shs"},
        {s1Lor, "lor"},
        {s1And, "and"},
        {s1Xor, "xor"},
        {s1Not, "not"},
        {s1LDA, "lDA"},
        {s1LDR, "lDR"},
        {s1SAD, "sAD"},
        {s1SRD, "sRD"},
        {s1LPA, "lPA"},
        {s1LPR, "lPR"},
        {s1SAP, "sAP"},
        {s1SRP, "sRP"},
        {s1Out, "out"},
        {s1Got, "got"},
        {s1Jm0, "jm0"},
        {s1JmA, "jmA"},
        {s1JmG, "jmG"},
        {s1JmL, "jmL"},
        {s1JmS, "jmS"},
        {s1Ret, "ret"},
        {s1Pha, "pha"},
        {s1Pla, "pla"},
        {s1Brk, "brk"},
        {s1Clr, "clr"},
        {s1Putstr, "putstr"},
        {s1Ahm, "ahm"},
        {s1Fhm, "fhm"},
        {s1Syscall, "syscall"},
    };

    static const unsigned size = sizeof(map) / sizeof(map[0]);
    for (unsigned i = 0; i < size; i++)
    {
        if (strcmp(map[i].str, str) == 0)
            return map[i].inst;
    }

    return s1Invalid;
}



int getInstCount(char* s)
{
    int instCount = 0;
    char last = endOfInst;
    bool isVaild = true;

    for (int i = 0; last; i++)
    {
        switch (s[i])
        {
        case 0:
        case endOfInst:
            if (last != endOfInst && isVaild) instCount++;
            isVaild = true;

        default:
            last = s[i];
            break;

        case '"':
            isVaild = false;
            break;

        case '\r':
            break;
        }
    }

    return instCount;
}

int getLabelCount(struct rawInst* rawInstBuffer, int instCount)
{
    int labelCount = 0;

    for (int i = 0; i < instCount; i++)
        if (strcmp(rawInstBuffer[i].op, "lab") == 0) labelCount++;

    return labelCount;
}

bool tokenize(char* source, int instCount, struct rawInst* dst)
{
    int sourceIndex = 0;
    for (int rawInstIndex = 0; rawInstIndex < instCount; rawInstIndex++)
    {
        //printf("%d\n", rawInstIndex);
        //printf("%d\n", instCount);
        struct rawInst* target = &dst[rawInstIndex];

        //remove empty lines
        do
        {
            //strip
            //while (source[sourceIndex] == ' ') sourceIndex++;
            while (charInString(" \r", source[sourceIndex])) sourceIndex++;

            //comments
            if (source[sourceIndex] == '"')
            {
                while (source[sourceIndex++] != '\n');
                continue;
            }

        }
        while (source[sourceIndex++] == '\n');
        sourceIndex--; //step back the sourceIndex, because it gets moved by the empty line remover


        //operator
        char delimChar = readStringCustomDelim(target->op, source, &sourceIndex, " \n\r");
        //while (source[sourceIndex] == ' ') sourceIndex++;
        while (charInString(" \r", source[sourceIndex])) sourceIndex++;

        //printf("%s\n", target->op);

        //operator without arg
        if (delimChar != ' ') continue;

        //the delim of operator should really not be eof
        if (!delimChar) return false;

        //argument
        readStringCustomDelim(target->arg, source, &sourceIndex, " \n\r");
        //while (source[sourceIndex] == ' ') sourceIndex++;
        while (charInString(" \r", source[sourceIndex])) sourceIndex++;

    }
    return true;
}

void readLabels(struct rawInst* rawInstBuffer, int instCount, struct labelMap* labelMapper)
{
    int trueIndex = 0;
    int labelIndex = 0;
    for (int i = 0; i < instCount; i++)
    {
        if (strcmp(rawInstBuffer[i].op, "lab") == 0)
        {
            labelMapper[labelIndex].label = rawInstBuffer[i].arg;
            labelMapper[labelIndex].address = trueIndex;
            labelIndex++;
        }
        else trueIndex++;
    }
}


int label2address(char* label, struct labelMap* labelMapper, int labelCount)
{
    for (int i = 0; i < labelCount; i++)
        if (strcmp(labelMapper[i].label, label) == 0) return labelMapper[i].address;

    return -1;
}


struct process* parseProcess(char* source)
{
    //get instruction count
    int rawInstCount = getInstCount(source);
    flog("Parsing process, %d insts found\n", rawInstCount);

    //alloc
    struct rawInst* rawInstBuffer = (struct rawInst*)malloc(sizeof(struct rawInst) * rawInstCount);

    //tokenize to rawInst
    bool success = tokenize(source, rawInstCount, rawInstBuffer);
    if (!success)
    {
        flog("Tokenizing failed\n");
        free(rawInstBuffer);
        return NULL;
    }

    //count labels
    int labelCount = getLabelCount(rawInstBuffer, rawInstCount);
    struct labelMap* labelMapper = (struct labelMap*)malloc(sizeof(struct labelMap) * labelCount);

    //populate labelMapper
    readLabels(rawInstBuffer, rawInstCount, labelMapper);

    //account for labels, because they don't count as instructions (well, they do, but are in this case irrelevant)
    int instCount = rawInstCount - labelCount;
    struct inst* prog = (struct inst*)malloc(sizeof(struct inst) * (instCount));

    //parse the rawInsts into insts in prog
    int progIndex = 0;
    for (int i = 0; i < rawInstCount; i++)
    {
        char* opStr  = rawInstBuffer[i].op;
        char* argStr = rawInstBuffer[i].arg;

        //skip labels
        if (strcmp(opStr, "lab") == 0)
            continue;

        //convert
        enum s1Insts inst = str2s1(opStr);
        fguard(inst, "Invaild operation '%s' found while parsing\n" COMMA opStr, NULL);

        prog[progIndex].op = (int)inst;

        //check for label reference
        if (!*argStr)
            prog[progIndex].arg = 0;
        else if (isOnlyDigits(argStr))
            prog[progIndex].arg = atoi(argStr);
        else
        {
            int address = label2address(argStr, labelMapper, labelCount);
            if (address < 0)
            {
                flog("Undefined label '%s' found while parsing\n", argStr);

                free(rawInstBuffer);
                free(labelMapper);
                free(prog);
                return NULL;
            }

            prog[progIndex].arg = address;
        }
        

        progIndex++;
    }


    struct process* proc = allocProcess();
    proc->prog = prog;
    proc->progSize = instCount;
   
    free(rawInstBuffer);
    free(labelMapper);

    //network
    if ((proc->procSock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
    {
        flog("Failed to open socket\n");
        freeProcess(proc);
        return NULL;
    }

    int opt = 1;
    if (setsockopt(proc->procSock, SOL_SOCKET, SO_REUSEPORT | SO_REUSEADDR,
        &opt, sizeof(opt)))
    {
        flog("Failed to config socket\n");
        freeProcess(proc);
        return NULL;
    }

    struct sockaddr_in* netAddr = &proc->netAddr;
    netAddr->sin_family = AF_INET;
    netAddr->sin_addr.s_addr = INADDR_ANY;
    netAddr->sin_port = 0;

    return proc;
}

//clones heap, given a start ptr
struct S1HeapChunk* cloneHeap(struct S1HeapChunk* src)
{
    if (!src) return NULL;
    struct S1HeapChunk* out = allocS1HeapChunk();
    struct S1HeapChunk* dst = out;

    //move throught heap and copy
    for (;;)
    {
        //copy data
        memcpy(dst, src, sizeof(struct S1HeapChunk));

        //check if the end is reached
        if (!src->next) break;

        //allocate new chunk for dst and move ptrs
        dst->next = allocS1HeapChunk();

        dst = dst->next;
        src = src->next;
    }

    return out;
}

struct process* cloneProcess(struct process* src)
{
    //alloc new process and copy static data
    struct process* dst = allocProcess();
    free(dst->heap); //allocProcess generates a base heap chunk that must be free before cloning
    memcpy(dst, src, sizeof(struct process));

    //remove socket file descriptor, ik this is buggy, idc
    dst->procSock = 0;

    //copy dynamics
    int progMemSize = sizeof(struct inst) * src->progSize;
    dst->prog = malloc(progMemSize);
    memcpy(dst->prog, src->prog, progMemSize);

    dst->heap = cloneHeap(src->heap);
    dst->procNap = cloneProcNap(src->procNap);

    dst->fMaps = cloneFileMap(src->fMaps);

    //reset uuid, just in case
    dst->uuid = 0x0;


    return dst;
}



struct procPool* allocProcPool()
{
    struct procPool* pool = (struct procPool*)malloc(sizeof(struct procPool));
    pool->procs = NULL;
    pool->procCount = 0;
    pool->execPtr = NULL;
    pool->napMs = 0;
    return pool;
}

//THIS WILL ALLOCATE MEMORY
unsigned int* getAllPids(struct system* ouch)
{
    struct procPool* pool = ouch->pool;
    struct procList* scanPtr = pool->procs;
    
    //create list
    unsigned int* pidList = (unsigned int*)malloc(sizeof(unsigned int) * pool->procCount);

    //scan procList
    int i = 0;
    while (scanPtr)
    {
        pidList[i++] = scanPtr->proc->pid;
        scanPtr = scanPtr->next;
    }

    return pidList;
}


void launchProcess(struct process* proc, struct system* ouch)
{
    struct procPool* pool = ouch->pool;
    struct procList* last = pool->procs;

    //generate pid
    unsigned int* pids = getAllPids(ouch);
    unsigned int newPid = getSmallPosivNumNotInList(pids, pool->procCount);
    proc->pid = newPid;
    free(pids);

    //assign uuid
    proc->uuid = uuidCount++;

    //new procList
    struct procList* newProcList = (struct procList*)malloc(sizeof(struct procList));

    //retarget
    pool->procs = newProcList;
    if (last) last->prev = newProcList;

    newProcList->next = last;
    newProcList->prev = NULL;

    //inject process
    newProcList->proc = proc;

    //inc process count
    pool->procCount++;

}


void freeS1Heap(struct S1HeapChunk* ptr)
{
    while (ptr != NULL)
    {
        struct S1HeapChunk* next = ptr->next;
        free(ptr);
        ptr = next;
    }
}


void freeProcess(struct process* proc)
{
    //network
    shutdown(proc->procSock, SHUT_RDWR);

    //free heap
    freeS1Heap(proc->heap);

    //check for procNap
    if (proc->procNap) freeProcNap(proc->procNap);

    //free fileMaps
    freeFileMaps(proc->fMaps);

    free(proc->prog);
    free(proc);

}

//removes element of the process list
void removeProcessList(struct procList* list, struct system* ouch)
{
    struct procPool* pool = ouch->pool;

    //link previouse to next
    if (list->prev)
        list->prev->next = list->next;
    else
        pool->procs = list->next;


    //link next to previouse 
    if (list->next)
        list->next->prev = list->prev;

    //clean up
    pool->procCount--;
    freeProcess(list->proc);
    free(list);
}

//removes process from pool
bool removeProcess(struct process* proc, struct system* ouch)
{
    struct procPool* pool = ouch->pool;
    struct procList* procs = pool->procs;
    
    struct procList* iter = procs;

    //find process in pool list
    for (int i = 0; i < pool->procCount; i++)
    { 
        if (iter->proc == proc)
        {
            removeProcessList(iter, ouch);
            return true;
        }

        iter = iter->next;
    }

    return false;
}

void freeProcPool(struct system* ouch)
{
    struct procPool* pool = ouch->pool;
    struct procList* cur;
    while ((cur = pool->procs))
        removeProcessList(cur, ouch);


    free(pool);
}




bool forkChecker(struct system* ouch, struct process* proc)
{
    bool procThresHold = ouch->pool->procCount    > forkCheckerProcThreshold;
    bool forkThresHold = proc->forkDepth > forkCheckerDepthThreshold;
    bool danger        = procThresHold && forkThresHold; //big fucky wucky >w<

    //if dangerous, iterate super processes and kill them
    if (danger)
    {
        
        //TODO: do this

    }

    return danger;
}



//TODO: explain how in the fuck this thing works
bool runPool(struct system* ouch)
{
    struct procPool* pool = ouch->pool;
    struct procList* curList = pool->execPtr;

    //the task switcher keeps track of how long it can sleep after it's gone through all the processes
    //it does this by keeping track of the minimun each process has to remain sleeping
    //however it will not sleep longer then a second
    int* napMs = &pool->napMs;

    if (curList)
    {
        enum returnCodes rt = rtNormal;
        struct process*  curProc  = curList->proc;
        struct procList* nextList = curList->next;

        //fork checker (skip procress run, if removed)
        if (forkChecker(ouch, curProc)) goto skipRun;

        //check if process is napping and if so update the napMs
        if (curProc->procNap)
        {
            int napProcMs = updateProcNap(curProc);
            if (*napMs > napProcMs) *napMs = napProcMs;
        }
        else
        {
            int iterLimit = procIterCycle / pool->procCount;
            rt = runProcess(curProc, iterLimit, ouch);
            *napMs = 0; //if not all processes are napping, the switch can nap either
        }

        //*screams of terror* A LABEL !?
        skipRun:

        //advance execPtr
        pool->execPtr = nextList;

        switch (rt)
        {
        case rtNormal:
            break;

        case rtExit:
            //remove 
            flog("Process finished, removing\n");
            removeProcessList(curList, ouch);
            break;

        case rtSyscall:;
            enum S1Syscall callType = curProc->lastSyscall;
            runSyscall(callType, curProc, ouch);
            break;

        }
    }
    else
    {
        //if execPtr is NULL reset it back to the begining
        pool->execPtr = pool->procs;

        //if the process pool is empty, shutdown the system
        if (pool->procs == NULL)
            return false;

        //nap according to napMs
        if (*napMs) usleep(*napMs * 1000);
        *napMs = 200;
    }

    return true;
}


bool processStackPush(struct process* proc, const S1Int* value)
{ return stackPush(proc->stack, &proc->stackPtr, value); }

bool processStackPull(struct process* proc, S1Int* value)
{ return stackPull(proc->stack, &proc->stackPtr, value); }

//THIS WILL ALLOCATE MEMORY
//reads string out of proess memory, string must be null terminated
char* readStringFromProcessMemory(struct process* proc, S1Int ptr)
{
    S1Int* mem = proc->mem;
    
    //count size, including null terminator
    int size = 0;
    while ((size + ptr) <= Bit16IntLimit && mem[(size++) + ptr]);
 
    char* str = (char*)malloc(sizeof(char) * size);
    for (int i = 0; i < size; i++)
        str[i] = (int)mem[i + ptr];

    return str;
}