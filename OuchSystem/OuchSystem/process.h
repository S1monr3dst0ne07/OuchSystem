#ifndef HPROCESS
#define HPROCESS

#include "kernal.h"
#include "timing.h"
#include "vm.h"
#include "types.h"
#include "config.h"
#include "stream.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef _WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#define rawInstBufferLimit 128
#define S1IntBufferSize (Bit16IntLimit * sizeof(S1Int))

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
    scStmSend,
    scStmProcStd,
    scStmGetArgs,
    scStmGetWork,
    scPeekStm,
    scOpenFileObj = 0x0010,
    scCreateObj,
    scDelObj,
    scNapMs = 0x0020,
    scNapS,
    scUnixEpoch,
    scFLocTime,
    scBindPort = 0x0030,
    scAcctSock,
    scConnect,
    scGetPid = 0x0040,
    scLaunchProc,
    scKillProc,
    scProcMeta,
    scForkProc,
    scAllPids,
    scMMap = 0x0050,
    scTSL,

    scSOS = 0xffff,
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

    int napMs;
};

struct procList
{
	struct process*  proc;
	struct procList* next;
	struct procList* prev;

};

struct S1HeapChunk
{
    S1Int ptr;
    S1Int size;
    struct S1HeapChunk* next;
};


struct process
{
    S1Int         pid;  //process id   (reusable)
    unsigned long uuid; //universal id (session unique)

    //internals
	int ip;
	struct inst* prog;
    int progSize;

    S1Int mem[Bit16IntLimit];
    S1Int stack[Bit16IntLimit];
    int stackPtr;

    struct S1HeapChunk* heap;

    S1Int acc;
    S1Int reg;

    //syscall
    enum S1Syscall lastSyscall;

    //stdio 
    struct stream* stdio;

    //network hell
    int procSock;

    //timing hell
    //if procNap is NULL, the process isn't napping
    struct processNap* procNap;

    //thread / shared mem hell
    struct fileMap* fMaps;

    //basically the wd and args for cmd line
    struct filePath* workPath;
    char* args;

    //fork tracking (anti-bomb system)
    int forkDepth; //how many fork the process is away from autoLaunch
    unsigned long uuidGroup; //all processes forked from one, share this uuid with the original

};


struct inst
{
	int op;
	int arg;
};

struct S1HeapChunk* allocS1HeapChunk();

void freeProcess(struct process* proc);
struct process* parseProcess(char* source);
struct process* cloneProcess(struct process* src);

struct procPool* allocProcPool();
void launchProcess(struct process* proc, struct system* ouch);
struct process* launchPath(char* pathStr, struct system* ouch, char* workPath, char* args);
struct process* getProcByPID(S1Int pid, struct procPool* pool);

bool removeProcess(struct process* proc, struct system* ouch);
void freeProcPool(struct system* ouch);
bool runPool(struct system* ouch);

bool processStackPush(struct process* proc, const S1Int* value);
bool processStackPull(struct process* proc,       S1Int* value);

char* readStringFromProcessMemory(struct process* proc, S1Int ptr);







//--- definitions to make visual studio shut up ---
// (sorry for using windows, i should be hanged)

#if _WIN32

#define AF_INET         0
#define SOCK_STREAM     0
#define SOCK_NONBLOCK   0
#define SOL_SOCKET      0
#define SO_REUSEPORT    0
#define SO_REUSEADDR    0
#define INADDR_ANY      0
#define MSG_DONTWAIT    0

#define SHUT_RDWR       0

typedef void socklen_t;

struct in_addr 
{
    int s_addr;
};
struct sockaddr_in {
    short int          sin_family;  // Address family, AF_INET
    unsigned short int sin_port;    // Port number
    struct in_addr     sin_addr;    // Internet address
    unsigned char      sin_zero[8]; // Same size as struct sockaddr
};

#endif


#endif