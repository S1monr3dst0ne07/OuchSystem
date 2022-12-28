#ifndef HPROCESS
#define HPROCESS

#define rawInstBufferLimit 128
#define c16bitIntLimit (1 << 16)

typedef unsigned short int S1Int;
char cTemp[2048];

#define S1IntBufferSize (c16bitIntLimit * sizeof(S1Int))

enum s1Insts
{
    invalid = 0,
    set     = 1,
    add,
    sub,
    shg,
    shs,
    lor,
    and,
    xor,
    not,
    lDA,
    lDR,
    sAD,
    sRD,
    lPA,
    lPR,
    sAP,
    sRP,
    out,
    got,
    jm0,
    jmA,
    jmG,
    jmL,
    jmS,
    ret,
    pha,
    pla,
    brk,
    clr,
    putstr,
    ahm,
    fhm,
};


enum returnCodes
{
    rtNormal = 0,
    rtExit,

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


struct process
{
	int ip;
	struct inst* prog;

    S1Int mem[c16bitIntLimit];
    S1Int stack[c16bitIntLimit];
    int stackPtr;

    S1Int acc;
    S1Int reg;
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
void RunPool(struct system* ouch);

#endif