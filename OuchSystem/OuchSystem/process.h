#ifndef HPROCESS
#define HPROCESS

#include "kernal.h"
#include "timing.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>


#define rawInstBufferLimit 128
#define c16bitIntLimit (1 << 16)

typedef unsigned short int S1Int;

#define S1IntBufferSize (c16bitIntLimit * sizeof(S1Int))

enum s1Insts
{
    s1Invalid = 0,
    s1Set     = 1,
    s1Add,
    s1Sub,
    s1Shg,
    s1Shs,
    s1Lor,
    s1And,
    s1Xor,
    s1Not,
    s1LDA,
    s1LDR,
    s1SAD,
    s1SRD,
    s1LPA,
    s1LPR,
    s1SAP,
    s1SRP,
    s1Out,
    s1Got,
    s1Jm0,
    s1JmA,
    s1JmG,
    s1JmL,
    s1JmS,
    s1Ret,
    s1Pha,
    s1Pla,
    s1Brk,
    s1Clr,
    s1Putstr,
    s1Ahm,
    s1Fhm,
    s1Syscall,
};

enum S1Syscall
{
    scNoop = 0x0000,
    scCloseStm = 0x0001,
    scReadStm,
    scWriteStm,
    scStmInfo,
    scOpenFileObj = 0x0010,
    scCreateObj,
    scDelObj,
    scSleepMs = 0x0020,
    scUnixEpoch,
    scFLocTime,
    scBindPort = 0x0030,
    scAcctSock,
    scGetPid = 0x0040,

};

enum returnCodes
{
    rtNormal = 0,
    rtExit,
    rtSyscall,
};


struct procPool
{
	struct procList* procs;
	int procCount;

	struct procList* execPtr;
};

struct procList
{
	struct process*  proc;
	struct procList* next;
	struct procList* prev;

};

struct S1HeapChunk
{
    int ptr;
    int size;
    struct S1HeapChunk* next;
};


struct process
{
    unsigned int pid;

    //internals
	int ip;
	struct inst* prog;
    int progSize;

    S1Int mem[c16bitIntLimit];
    S1Int stack[c16bitIntLimit];
    int stackPtr;

    struct S1HeapChunk* heap;

    S1Int acc;
    S1Int reg;

    //syscall
    enum S1Syscall lastSyscall;

    //network hell
    int procSock;
    struct sockaddr_in netAddr;

    //timing hell
    //if procNap is NULL, the process isn't napping
    struct processNap* procNap;

};


struct inst
{
	int op;
	int arg;
};

void freeProcess(struct process* proc);
struct process* parseProcess(char* source);

struct procPool* allocProcPool();
void launchProcess(struct process* proc, struct system* ouch);

bool removeProcess(struct process* proc, struct system* ouch);
void freeProcPool(struct system* ouch);
bool runPool(struct system* ouch);

bool processStackPush(struct process* proc, S1Int* value);
bool processStackPull(struct process* proc, S1Int* value);

char* readStringFromProcessMemory(struct process* proc, S1Int ptr);

#endif