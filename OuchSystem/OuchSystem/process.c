
#include "utils.h"
#include "process.h"
#include "kernal.h"

#include <stdbool.h>
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
    };

    static const unsigned size = sizeof(map) / sizeof(map[0]);
    for (int i = 0; i < size; i++)
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
    bool isEmptyLine = true;

    for (int i = 0; source[i]; i++)
    {
        switch (source[i])
        {

        case '\n':;
            if (!isEmptyLine) instCount++;
            isEmptyLine = true;

        case ' ':;
            break;

        case 0:;
            instCount++;

        default:;
            isEmptyLine = false;
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
            //strip
            while (source[sourceIndex] == ' ') sourceIndex++;

        while (source[sourceIndex++] == '\n');
        sourceIndex--; //step back the sourceIndex, because it get's moved by the empty line remover

        //operator
        readStringCustomDelim(target->op, source, &sourceIndex, " \n");
        while (source[sourceIndex] == ' ') sourceIndex++;

        //argument
        readStringCustomDelim(target->arg, source, &sourceIndex, " \n");
        while (source[sourceIndex] == ' ') sourceIndex++;

        //now the next character should be a newline
        if (consu(source, &sourceIndex) != '\n')
        {
            sprintf(cTemp, "Error while parsing process\n\tLast process found (%s, %s)", target->op, target->arg);
            log(cTemp);
            return false;
        }
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
    sprintf(cTemp, "Parsing process, found %d insts found\n", rawInstCount);
    log(cTemp);


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
            log(cTemp);
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
                sprintf(cTemp, "Undefined label '%s' found whule parsing\n", argStr);
                log(cTemp);
                return NULL;
            }

            prog[progIndex].arg = address;
        }
        

        progIndex++;
    }


    struct process* proc = (struct process*)malloc(sizeof(struct process));
    proc->ip = 0;
    proc->prog = prog;


    //zero the rest of the process
    memset(proc->mem,   0, S1IntBufferSize);
    memset(proc->stack, 0, S1IntBufferSize);

    proc->stackPtr = 0;
    proc->acc = 0;
    proc->reg = 0;


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

void RunPool(struct system* ouch)
{
    struct procPool* pool = ouch->pool;
    struct procList* curList = pool->execPtr;
    if (curList)
    {
        struct proc* curProc = curList->proc;
        enum returnCodes rt = RunProcess(curProc);

        //advance execPtr
        pool->execPtr = curList->next;

        switch (rt)
        {
        case rtNormal:
            break;

        case rtExit:
            //remove 
            log("Process finished, removing\n");
            removeProcessList(curList, ouch);

            printf("");
            break;
        }
    }
    else
        //if execPtr is NULL reset it back to the begining
        pool->execPtr = pool->procs;

}

//runs process, advancing by one instruction
enum returnCodes RunProcess(struct process* proc)
{
    int* ip = &proc->ip;
    struct inst curInst = proc->prog[(*ip)++];

    int op  = curInst.op;
    int arg = curInst.arg;

    S1Int* mem = proc->mem;
    S1Int* stack = proc->mem;
    int* stackPtr = &proc->stackPtr;

    S1Int* acc = &proc->acc;
    S1Int* reg = &proc->reg;


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
        *acc = mem[mem[arg]];
        break;

    case lPR:
        *reg = mem[mem[arg]];
        break;

    case sAP:
        mem[mem[arg]] = *acc;
        break;

    case sRP:
        mem[mem[arg]] = *reg;
        break;

    case out:
        printf("%d\n", mem[arg]);
        break;

    case got:
        *ip = arg;
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

    case pha:
        stack[(*stackPtr)++] = *acc;
        break;

    case pla:
        *acc = *stackPtr > 0 ? stack[--(*stackPtr)] : 0;
        break;

    case brk:
        return rtExit;

    case clr:
        *acc = 0;
        *reg = 0;
        break;

    case putstr:        
        printf("%c", (char)acc);
        break;

    case ahm:;
        break;

    case fhm:;
        break;

    default:
        break;
    }

    return rtNormal;
}
