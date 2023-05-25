
#include "vm.h"

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


//runs process, advancing by n instructions
enum returnCodes runProcess(struct process* proc, int iterLimit)
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
    }

    return rtNormal;
}