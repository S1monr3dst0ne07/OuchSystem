#ifndef HSTREAM
#define HSTREAM

#define streamBufferSize (1 << 16)
#define pipeBufferSize 2048

enum streamType
{
    stmInvailed = 0,
    stmTypFile = 1,
    stmTypDir,
    stmTypSocket,
    stmTypPipe,
    stmTypRootProc, //process, stdio attached to host stdio
    stmTypArgs,
    stmTypWork,
};

struct stream
{
    S1Int id;

    char* readContent;
    int readSize;
    int readIndex;

    char writeContent[streamBufferSize];
    int writeIndex;

    enum streamType type;

    //general metadata
    // stmTypFile   -> file path
    // stmTypSocket -> socket fd 
    // stmTypPipe   -> pointer to mirror stream
    void* meta;
};

#endif