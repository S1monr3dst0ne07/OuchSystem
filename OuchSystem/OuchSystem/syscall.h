#ifndef HSYSCALL
#define HSYSCALL

#include "utils.h"
#include "process.h"
#include "types.h"
#include "stream.h"

#include <stdlib.h>

#ifndef _WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#define riverListSize 65535
#define networkBufferSize 1024

#define i2id(x) x + 1
#define id2i(x) x - 1

//contains stream, id given by the position in container
//id and array index are synonymus (index 0 -> id 1)
struct streamPool
{
    struct stream* container[riverListSize];
    int count;
};


struct stream* createStream(unsigned char* content, int len);
struct stream* createPipe();
void freeStream(struct stream* stm);
void removeStream(struct stream* stm, struct streamPool* river);

bool readStream(struct stream* stm, S1Int* data);
bool writeStream(struct stream* stm, S1Int val);

struct streamPool* allocStreamPool();
void freeStreamPool(struct system* ouch);
void runSyscall(enum S1Syscall callType, struct process* proc, struct system* ouch);
void updateStreams(struct system* ouch);
S1Int injectStream(struct stream* stm, struct system* ouch);

#endif