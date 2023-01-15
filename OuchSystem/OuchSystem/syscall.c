#include "syscall.h"
#include "files.h"



//allocates new stream, content must be zero terminated
struct stream* allocStream(S1Int* content)
{
    struct stream* stm = (struct stream*)malloc(sizeof(struct stream));
    stm->readContent = content;
    stm->readSize = strlen(content);

    stm->readIndex  = 0;
    stm->writeIndex = 0;


    return stm;
}

void createStream(S1Int* content, struct streamPool* target)
{
    
}




void runSyscall(enum S1Syscall callType, struct process* proc, struct system* ouch)
{
    sprintf(cTemp, "Syscall %x\n", callType);
    log(cTemp);

    switch (callType)
    {
    case scNoop:
        break;

    }

}
