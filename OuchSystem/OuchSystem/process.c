
#include "kernal.h"
#include "utils.h"
#include "syscall.h"
#include "process.h"

#include <stdlib.h>
#include <string.h>

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

    proc->stackPtr = 0;
    proc->acc = 0;
    proc->reg = 0;

    proc->heap = allocS1HeapChunk();

    proc->procNap = NULL;

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



int getInstCount(char* source)
{
    int instCount = 0;
    char last = '\n';
    bool isComment = false;

    for (int i = 0; last; i++)
    {
        switch (source[i])
        {
        case 0:
        case '\n':
            if (last != '\n' && !isComment) instCount++;
            isComment = false;

        default:
            last = source[i];
            break;

        case '"':
            isComment = true;
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
        struct rawInst* target = &dst[rawInstIndex];

        //remove empty lines
        do
        {
            //strip
            while (source[sourceIndex] == ' ') sourceIndex++;

            //comments
            if (source[sourceIndex] == '"')
            {
                while (source[sourceIndex++] == '\n');
                continue;
            }

        }
        while (source[sourceIndex++] == '\n');
        sourceIndex--; //step back the sourceIndex, because it get's moved by the empty line remover

        //operator
        char delimChar = readStringCustomDelim(target->op, source, &sourceIndex, " \n");
        while (source[sourceIndex] == ' ') sourceIndex++;

        //the delim of operator should really not be eof
        if (!delimChar) return false;

        //argument
        readStringCustomDelim(target->arg, source, &sourceIndex, " \n");
        while (source[sourceIndex] == ' ') sourceIndex++;

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
    sprintf(cTemp, "Parsing process, %d insts found\n", rawInstCount);
    logg(cTemp);


    //alloc
    struct rawInst* rawInstBuffer = (struct rawInst*)malloc(sizeof(struct rawInst) * rawInstCount);

    //tokenize to rawInst
    bool success = tokenize(source, rawInstCount, rawInstBuffer);
    if (!success)
    {
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
        if (!inst)
        {
            sprintf(cTemp, "Invaild operation '%s' found while parsing\n", opStr);
            logg(cTemp);
            return NULL;
        }

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
                sprintf(cTemp, "Undefined label '%s' found while parsing\n", argStr);
                logg(cTemp);

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
        logg("Failed to open socket\n");
        freeProcess(proc);
        return NULL;
    }

    int opt = 1;
    if (setsockopt(proc->procSock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
        &opt, sizeof(opt)))
    {
        logg("Failed to config socket\n");
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
        dst = src;

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
    memcpy(dst, src, sizeof(struct process));

    //copy dynamics
    dst->prog = malloc(sizeof(struct inst) * src->progSize);
    memcpy(dst->prog, src->prog, src->progSize);

    dst->heap = cloneHeap(src->heap);
    dst->procNap = cloneProcNap(src->procNap);

    return dst;
}



struct procPool* allocProcPool()
{
    struct procPool* pool = (struct procPool*)malloc(sizeof(struct procPool));
    pool->procs = NULL;
    pool->procCount = 0;
    pool->execPtr = NULL;
    return pool;
}

bool stackPush(S1Int* stack, int* stackPtr, const S1Int* value)
{
    if (*stackPtr >= c16bitIntLimit) return false;
    stack[(*stackPtr)++] = *value;
    return true;
}

bool stackPull(S1Int* stack, int* stackPtr, S1Int* value)
{
    if (*stackPtr <= 0) return false;
    *value = stack[--(*stackPtr)];
    return true;
}

struct S1HeapChunk* findSpace(unsigned short int neededSpace, struct S1HeapChunk* startChunk)
{
    struct S1HeapChunk* iter = startChunk;

    while (true)
    {
        //check for end of list
        if (iter->next == NULL) break;

        //check for space in between two chunks
        S1Int iterEndPtr = iter->ptr + iter->size;
        S1Int nextStartPtr = iter->next->ptr;

        if ((nextStartPtr - iterEndPtr) > neededSpace) break;

        iter = iter->next;
    }

    return iter;
}

//finds the chunk that fits ptr and size, and returns the previouse in the list
struct S1HeapChunk* findPrevChunkByPtrAndSize(S1Int ptr, S1Int size, struct S1HeapChunk* startChunk)
{
    struct S1HeapChunk* iter = startChunk;
    struct S1HeapChunk* last = NULL;

    while (iter)
    {
        if (iter->ptr == ptr && iter->size == size) return last;
        last = iter;
        iter = iter->next;
    }

    return NULL;
}


//runs process, advancing by one instruction
enum returnCodes runProcess(struct process* proc)
{
    int* ip = &proc->ip;

    //check if ip is out bound
    if (*ip > proc->progSize) return rtExit;

    struct inst curInst = proc->prog[(*ip)++];

    int op = curInst.op;
    int arg = curInst.arg;

    S1Int* mem = proc->mem;
    S1Int* stack = proc->stack;
    int* stackPtr = &proc->stackPtr;

    S1Int* acc = &proc->acc;
    S1Int* reg = &proc->reg;

    bool success; //temp

    switch (op)
    {
    case s1Set:
        *reg = (S1Int)arg;
        break;

    case s1Add:
        *acc += *reg;
        break;

    case s1Sub:
        *acc -= *reg;
        break;

    case s1Shg:
        *acc <<= 1;
        break;

    case s1Shs:
        *acc >>= 1;
        break;

    case s1Lor:
        *acc |= *reg;
        break;

    case s1And:
        *acc &= *reg;
        break;

    case s1Xor:
        *acc ^= *reg;
        break;

    case s1Not:
        *acc = ~*acc;
        break;

    case s1LDA:
        *acc = mem[arg];
        break;

    case s1LDR:
        *reg = mem[arg];
        break;

    case s1SAD:
        mem[arg] = *acc;
        break;

    case s1SRD:
        mem[arg] = *reg;
        break;


    case s1LPA:
        *acc = mem[(int)mem[arg]];
        break;

    case s1LPR:
        *reg = mem[(int)mem[arg]];
        break;

    case s1SAP:
        mem[(int)mem[arg]] = *acc;
        break;

    case s1SRP:
        mem[(int)mem[arg]] = *reg;
        break;

    case s1Out:
        sprintf(cTemp, "%d\n", mem[arg]);
        logg(cTemp);
        break;

    case s1Got:
        *ip = (int)arg;
        break;

    case s1Jm0:
        if (*acc == 0) *ip = arg;
        break;

    case s1JmA:
        if (*acc == *reg) *ip = arg;
        break;

    case s1JmG:
        if (*acc > *reg) *ip = arg;
        break;

    case s1JmL:
        if (*acc < *reg) *ip = arg;
        break;

    case s1JmS:
        //save the execPtr on the stack
        stack[(*stackPtr)++] = *ip;
        *ip = arg;
        break;

    case s1Ret:
        *ip = stack[--(*stackPtr)];
        break;

    case s1Pha:;
        success = stackPush(stack, stackPtr, acc);
        if (!success)
        {
            logg("Stackoverflow, killing process\n");
            return rtExit;
        }
        break;

    case s1Pla:;
        success = stackPull(stack, stackPtr, acc);
        if (!success)
        {
            logg("Stackunderflow, killing process\n");
            return rtExit;
        }
        break;

    case s1Brk:
        return rtExit;

    case s1Clr:
        *acc = 0;
        *reg = 0;
        break;

    case s1Putstr:
        printf("%c", (char)*acc);
        break;

    case s1Ahm:;
        S1Int allocSize = *reg;

        struct S1HeapChunk* insertBase = findSpace(allocSize, proc->heap);
        struct S1HeapChunk* newChunk = allocS1HeapChunk();

        //insert the new chunk into the link list
        S1Int newChunkBasePtr = insertBase->ptr + insertBase->size;
        newChunk->ptr = newChunkBasePtr;
        newChunk->size = allocSize;
        newChunk->next = insertBase->next;

        //link the new chunk as the next for the insertBase, to fully insert the new chunk and override the last next (which now is the next->next)
        insertBase->next = newChunk;

        *acc = newChunkBasePtr | (1 << 15);

        break;

    case s1Fhm:;
        S1Int freeSize = *reg;
        S1Int freeBaseRaw = *acc;
        S1Int freeBase = freeBaseRaw & ~(1 << 15);

        //the previouse is needed to delete the next chunk from the list, because the next pointer of the previouse need to be overritten to exclude the chunk for the list
        struct S1HeapChunk* foundPreviouseChunk = findPrevChunkByPtrAndSize(freeBase, freeSize, proc->heap);
        struct S1HeapChunk* foundChunk = NULL;

        if (foundPreviouseChunk != NULL)
        {
            foundChunk = foundPreviouseChunk->next;

            //unlink the foundChunk for the list, so it can be freed
            foundPreviouseChunk->next = foundChunk->next;
            free(foundChunk);

            //clear memory
            for (int i = 0; i < freeSize; i++) mem[freeBaseRaw + i] = 0;
        }
        else
        {
            sprintf(cTemp, "Chunk (ptr: %d, size: %d) could not be found", freeBase, freeSize);
            logg(cTemp);
        }

        break;


    case s1Syscall:
        proc->lastSyscall = (enum S1Syscall)arg;
        return rtSyscall;

    default:
        break;
    }

    return rtNormal;
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

void freeProcess(struct process* proc)
{
    //network
    shutdown(proc->procSock, SHUT_RDWR);

    //free heap
    struct S1HeapChunk* temp = proc->heap;
    while (temp != NULL)
    {
        struct S1HeapChunk* next = temp->next;
        free(temp);
        temp = next;
    }

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

bool runPool(struct system* ouch)
{
    struct procPool* pool = ouch->pool;
    struct procList* curList = pool->execPtr;
    if (curList)
    {
        enum returnCodes rt = rtNormal;
        struct process* curProc = curList->proc;
        
        //check if process is napping
        if (curProc->procNap)
            updateProcNap(curProc);
        else
            rt = runProcess(curProc);

        //advance execPtr
        pool->execPtr = curList->next;

        switch (rt)
        {
        case rtNormal:
            break;

        case rtExit:
            //remove 
            logg("Process finished, removing\n");
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
    while ((size + ptr) <= c16bitIntLimit && mem[(size++) + ptr]);
 
    char* str = (char*)malloc(sizeof(char) * size);
    for (int i = 0; i < size; i++)
        str[i] = (int)mem[i + ptr];

    return str;
}
