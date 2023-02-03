
#include "process.h"
#include "kernal.h"
#include "utils.h"
#include "syscall.h"

#include <stdlib.h>


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
    temp->ptr = NULL;
    temp->size = 0;
    return temp;
}

#define ENTRY(x) {x, #x}
enum s1Insts str2s1(char* str)
{
    static s1Entry map[] = {
        ENTRY(invalid),
        ENTRY(set),
        ENTRY(add),
        ENTRY(sub),
        ENTRY(shg),
        ENTRY(shs),
        ENTRY(lor),
        ENTRY(and),
        ENTRY(xor),
        ENTRY(not),
        ENTRY(lDA),
        ENTRY(lDR),
        ENTRY(sAD),
        ENTRY(sRD),
        ENTRY(lPA),
        ENTRY(lPR),
        ENTRY(sAP),
        ENTRY(sRP),
        ENTRY(out),
        ENTRY(got),
        ENTRY(jm0),
        ENTRY(jmA),
        ENTRY(jmG),
        ENTRY(jmL),
        ENTRY(jmS),
        ENTRY(ret),
        ENTRY(pha),
        ENTRY(pla),
        ENTRY(brk),
        ENTRY(clr),
        ENTRY(putstr),
        ENTRY(ahm),
        ENTRY(fhm),
        ENTRY(syscall),
    };

    static const unsigned size = sizeof(map) / sizeof(map[0]);
    for (unsigned i = 0; i < size; i++)
    {
        if (strcmp(map[i].str, str) == 0)
            return map[i].inst;
    }

    return invalid;
}
#undef ENTRY



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
    if (!success) return NULL;

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
                return NULL;
            }

            prog[progIndex].arg = address;
        }
        

        progIndex++;
    }


    struct process* proc = (struct process*)malloc(sizeof(struct process));
    proc->ip = 0;
    proc->prog = prog;
    proc->progSize = instCount;
   

    //zero the rest of the process
    memset(proc->mem,   0, S1IntBufferSize);
    memset(proc->stack, 0, S1IntBufferSize);

    proc->stackPtr = 0;
    proc->acc = 0;
    proc->reg = 0;

    proc->heap = allocS1HeapChunk();

    free(rawInstBuffer);
    free(labelMapper);
    return proc;
}



struct procPool* allocProcPool()
{
    struct procPool* pool = (struct procPool*)malloc(sizeof(struct procPool));
    pool->procs = NULL;
    pool->procCount = 0;
    pool->execPtr = NULL;
    return pool;
}

void launchProcess(struct process* proc, struct system* ouch)
{
    struct procPool* pool = ouch->pool;
    struct procList* last = pool->procs;

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
    struct procList* procs = pool->procs;

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
    while (cur = pool->procs)
        removeProcessList(cur, ouch);


    free(pool);
}

bool runPool(struct system* ouch)
{
    struct procPool* pool = ouch->pool;
    struct procList* curList = pool->execPtr;
    if (curList)
    {
        struct process* curProc = curList->proc;
        enum returnCodes rt = runProcess(curProc);

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

    int op  = curInst.op;
    int arg = curInst.arg;

    S1Int* mem = proc->mem;
    S1Int* stack = proc->stack;
    int* stackPtr = &proc->stackPtr;

    S1Int* acc = &proc->acc;
    S1Int* reg = &proc->reg;
    
    bool success; //temp

    switch (op)
    {
    case set:
        *reg = (S1Int)arg;
        break;

    case add:
        *acc += *reg;
        break;

    case sub:
        *acc -= *reg;
        break;

    case shg:
        *acc <<= 1;
        break;

    case shs:
        *acc >>= 1;
        break;

    case lor:
        *acc |= *reg;
        break;

    case and:
        *acc &= *reg;
        break;

    case xor:
        *acc ^= *reg;
        break;

    case not:
        *acc = ~*acc;
        break;

    case lDA:
        *acc = mem[arg];
        break;

    case lDR:
        *reg = mem[arg];
        break;

    case sAD:
        mem[arg] = *acc;
        break;

    case sRD:
        mem[arg] = *reg;
        break;


    case lPA:
        *acc = mem[(int)mem[arg]];
        break;

    case lPR:
        *reg = mem[(int)mem[arg]];
        break;

    case sAP:
        mem[(int)mem[arg]] = *acc;
        break;

    case sRP:
        mem[(int)mem[arg]] = *reg;
        break;

    case out:
        sprintf(cTemp, "%d\n", mem[arg]);
        logg(cTemp);
        break;

    case got:
        *ip = (int)arg;
        break;

    case jm0:
        if (*acc == 0) *ip = arg;
        break;

    case jmA:
        if (*acc == *reg) *ip = arg;
        break;

    case jmG:
        if (*acc > *reg) *ip = arg;
        break;

    case jmL:
        if (*acc < *reg) *ip = arg;
        break;

    case jmS:
        //save the execPtr on the stack
        stack[(*stackPtr)++] = *ip;
        *ip = arg;
        break;

    case ret:
        *ip = stack[--(*stackPtr)];
        break;

    case pha:;
        success = stackPush(stack, stackPtr, acc);
        if (!success)
        {
            logg("Stackoverflow, killing process\n");
            return rtExit;
        }
        break;

    case pla:;
        success = stackPull(stack, stackPtr, acc);
        if (!success)
        {
            logg("Stackunderflow, killing process\n");
            return rtExit;
        }
        break;

    case brk:
        return rtExit;

    case clr:
        *acc = 0;
        *reg = 0;
        break;

    case putstr:        
        printf("%c", (char)*acc);
        break;

    case ahm:;
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

    case fhm:;
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


    case syscall:
        proc->lastSyscall = (enum S1Syscall)arg;
        return rtSyscall;

    default:
        break;
    }

    return rtNormal;
}



bool stackPush(S1Int* stack, int* stackPtr, S1Int* value)
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

bool processStackPush(struct process* proc, S1Int* value)
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
