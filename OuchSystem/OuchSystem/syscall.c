#include "syscall.h"
#include "files.h"

void runSyscall(enum S1Syscall callType, struct system* ouch)
{
    sprintf(cTemp, "Syscall %x\n", callType);
    log(cTemp);

    switch (callType)
    {
    case scNoop:
        break;

    }

}
