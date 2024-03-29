
#include "utils.h"
#include "files.h"
#include "process.h"
#include "syscall.h"
#include "kernal.h"

#include <signal.h>
#include <string.h>
#include <stdio.h>

static volatile bool isRunning = true;

/*
This is the ouch operating system, meant to run and server s1asm processes
Ouch has three major components: The file system, the stream manager and the process manager

The file system contains files, like in conventional system, for data and executables
The stream manager and subsequently stream in general, are used for
interaction between a process and the operating system itself (file io, networking, etc.)


*/


void launchAutoProcesses(struct filePath* autoPath, struct system* ouch)
{
	flog("Launching auto startup processes\n\n");
	char* autoStartupFile = readFileContent(ouch, autoPath);
	fguard(autoStartupFile, "Unable to read auto file\n", );


	flog("'%s'\n", autoStartupFile);

	char* splitPtr = strtok(autoStartupFile, "\n");

	while (splitPtr != NULL)
	{
		flog("Launching '%s' ... \n", splitPtr);

		struct process* proc = launchPath(splitPtr, ouch, "", "");
		if (proc) proc->stdio->type = stmTypRootProc;

		if (proc) flog("OK\n");
		else      flog("Failed\n");
		
		splitPtr = strtok(NULL, "\n");
	}

	free(autoStartupFile);
	flog("\nOK\n");
}

//only one instance of struct system can exist
struct system bootOuch(char* imagePath)
{
	flog("Booting Ouch 1.0 ...\n");

	flog("\n");

	struct system ouch = {
		.root  = mountRootImage(imagePath),
		.pool  = allocProcPool(),
		.river = allocStreamPool(),
	};


	flog("\n");

	struct filePath* autoPath = parseFilePath("auto");
	launchAutoProcesses(autoPath, &ouch);
	freeFilePath(autoPath);

	flog("\n");

	flog("Booting complete\n");
	return ouch;
}

void shutdownOuch(char* imagePath, struct system* ouch)
{
	flog("Shutting down\n");

	//file system writeback and dealloc
	struct fileNode* root = ouch->root;
	unmountRootImage(imagePath, root);
	if (root) freeFileSystem(root);

	//dealloc rest of system
	freeProcPool(ouch); //processes go first, because they need to dealloc their stdio streams
	freeStreamPool(ouch);
}


void sigHandler(int sig)
{
	flog("Signal detected: %d", sig);
	isRunning = false;
}

void ouch(char* imagePath)
{
	signal(SIGINT, sigHandler);
	signal(SIGTERM, sigHandler);
	struct system ouch = bootOuch(imagePath);
	struct system* ouchPtr = &ouch;
	flog("\n");

	//check for root
	if (!ouchPtr->root)
		flog("Booting failed, shuting down\n");	
	else
		while (isRunning && runPool(ouchPtr));

	flog("\n");
	shutdownOuch(imagePath, ouchPtr);
}

void test(char* imagePath, int l)
{
	for (int i = 0; i < l; i++)
		ouch(imagePath);
}
