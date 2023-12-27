#ifndef HSTREAM
#define HSTREAM

#define streamOutputSize (1 << 16)

enum streamType
{
    stmInvailed = 0,
    stmTypFile = 1,
    stmTypDir,
    stmTypSocket,
    stmTypPipe,
    stmTypRootProc, //process, stdio attached to host stdio
};

struct stream
{
    S1Int id;

    unsigned char* readContent;
    int readSize;
    int readIndex;

    char writeContent[streamOutputSize];
    int writeIndex;

    enum streamType type;

    //general metadata
    // stmTypFile   -> file path
    // stmTypSocket -> socket fd 
    // stmTypPipe   -> pointer to mirror stream
    void* meta;
};

#endif