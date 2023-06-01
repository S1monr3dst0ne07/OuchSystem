
#include "vm.h"
#include "files.h"

#include <string.h>


bool stackPush(S1Int* stack, int* stackPtr, const S1Int* value)
{
    if (*stackPtr >= Bit16IntLimit) return false;
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




bool loadFileMap(struct process* proc, struct fileMap* fmap, struct system* ouch)
{
    while (fmap)
    {
        char* fileBase = getFileContentPtr(ouch, fmap->filePath);
        if (!fileBase) return false;

        S1Int size = fmap->size;
        char* file = fileBase + fmap->offset;

        //bounds check, sizeLimit needs to be recalculated because the file can change
        int sizeLimit = min(Bit16IntLimit - fmap->addr, strlen(file));
        if (size > sizeLimit) return false;

        S1Int* mem = proc->mem + fmap->addr;
        for (int i = 0; i < size; i++) mem[i] = file[i];

        fmap = fmap->next;
    } 

    return true;
}

bool saveFileMap(struct process* proc, struct fileMap* fmap, struct system* ouch)
{
    while (fmap)
    {
        char* fileBase = getFileContentPtr(ouch, fmap->filePath);
        if (!fileBase) return false;

        S1Int size = fmap->size;
        char* file = fileBase + fmap->offset;

        //bounds check
        int sizeLimit = min(Bit16IntLimit - fmap->addr, strlen(file));
        if (size > sizeLimit) return false;

        S1Int* mem = proc->mem + fmap->addr;
        for (int i = 0; i < size; i++) file[i] = mem[i];

        fmap = fmap->next;
    }

    return true;
}

//inject new file map into process
void injectFileMap(struct process* proc, struct fileMap* fmap)
{
    //base case
    if (!proc->fMaps)
    {
        proc->fMaps = fmap;
        return;
    }

    //get last fMap and inject
    struct fileMap* tmp = proc->fMaps;
    while (tmp->next)
        tmp = tmp->next;

    tmp->next = fmap;
}

struct fileMap* createFileMap(char* filePath, S1Int size, S1Int addr, S1Int offset)
{

    struct fileMap* fmap = malloc(sizeof(struct fileMap));

    fmap->filePath = parseFilePath(filePath);
    fmap->size     = size;
    fmap->addr     = addr;
    fmap->offset   = offset;
    fmap->next     = NULL;

    return fmap;
}

void freeFileMaps(struct fileMap* fmaps)
{
    while (fmaps)
    {
        struct fileMap* tmp = fmaps->next;

        freeFilePath(fmaps->filePath);
        free(fmaps);

        fmaps = tmp;
    }
}


struct fileMap* cloneFileMap(struct fileMap* src)
{
    if (!src) return NULL;

    struct fileMap* dst = malloc(sizeof(struct fileMap));
    if (!dst) return NULL;

    struct fileMap* tmp = dst;

    while (src)
    {
        memcpy(tmp, src, sizeof(struct fileMap));
        tmp->filePath = cloneFilePath(src->filePath);

        //step
        src = src->next;
        tmp = tmp->next;
    }

    return dst;
}


//simulates process, advancing by n instructions
enum returnCodes simProcess(struct process* proc, int iterLimit)
{
    int* ip = &proc->ip;

    S1Int* mem = proc->mem;
    S1Int* stack = proc->stack;
    int* stackPtr = &proc->stackPtr;

    S1Int* acc = &proc->acc;
    S1Int* reg = &proc->reg;

    bool success; //temp

    for (int iter = 0; iter < iterLimit; iter++)
    {
        //check if ip is out bound
        if (*ip > proc->progSize) return rtExit;

        struct inst curInst = proc->prog[(*ip)++];
        int op = curInst.op;
        int arg = curInst.arg;

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
            flog("%d\n", mem[arg]);
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
                flog("Stackoverflow, killing process\n");
                return rtExit;
            }
            break;

        case s1Pla:;
            success = stackPull(stack, stackPtr, acc);
            if (!success)
            {
                flog("Stackunderflow, killing process\n");
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
                flog("Chunk (ptr: %d, size: %d) could not be found", freeBase, freeSize);

            break;


        case s1Syscall:
            proc->lastSyscall = (enum S1Syscall)arg;
            return rtSyscall;

        default:
            break;
        }
    }

    return rtNormal;
}







//runs process, given cycle window
enum returnCodes runProcess(struct process* proc, int iterLimit, struct system* ouch)
{
    //load data from file map
    if (!loadFileMap(proc, proc->fMaps, ouch))
    {
        flog("LoadFileMap failed, killing process\n");
        return rtExit;
    }

    enum returnCodes rt = simProcess(proc, iterLimit);

    //save data back to file map
    if (!saveFileMap(proc, proc->fMaps, ouch))
    {
        flog("SaveFileMap failed, killing process\n");
        return rtExit;
    }

    return rt;
}



