
#include "utils.h"
#include "files.h"
#include "process.h"
#include "syscall.h"
#include "kernal.h"

#include <signal.h>
#include <string.h>
#include <stdio.h>

static volatile bool isRunning = true;
char cTemp[2048];

/*
This is the ouch operating system, meant to run and server s1asm processes
Ouch has three major components: The file system, the stream manager and the process manager

The file system contains files, like in conventional system, for data and executables
The stream manager and subsequently stream in general, are used for
interaction between a process and the operating system itself (file io, networking, etc.)


*/


void launchAutoProcesses(struct filePath* autoPath, struct system* ouch)
{
	logg("Launching auto startup processes\n\n");
	char* autoStartupFile = readFileContent(ouch, autoPath);
	
	if (autoStartupFile == NULL)
	{
		logg("Unable to read auto.och file\n");
		return;
	}

	char* splitPtr = strtok(autoStartupFile, "\n");

	while (splitPtr != NULL)
	{
		sprintf(cTemp, "Launching '%s' ... \n", splitPtr);
		logg(cTemp);

		bool success = launchProcessFromPath(splitPtr, ouch);
		if (success) logg("OK\n");
		else         logg("Failed\n");
		
		splitPtr = strtok(NULL, "\n");
	}

	free(autoStartupFile);
	logg("\nOK\n");
}

bool launchProcessFromPath(char* pathStr, struct system* ouch)
{
	struct filePath* path = parseFilePath(pathStr);
	char* source = readFileContent(ouch, path);
	if (!source) return false;

	struct process* proc = parseProcess(source);
	if (!proc) return false;

	launchProcess(proc, ouch);
	
	freeFilePath(path);
	free(source);
	return true;
}

//only one instance of struct system can exist
struct system bootOuch(char* imagePath)
{
	logg("Booting Ouch 1.0 ...\n");

	logg("\n");

	struct system ouch = {
		.root  = mountRootImage(imagePath),
		.pool  = allocProcPool(),
		.river = allocStreamPool(),
	};


	logg("\n");

	struct filePath* autoPath = parseFilePath("auto.och");
	launchAutoProcesses(autoPath, &ouch);
	freeFilePath(autoPath);

	logg("\n");

	logg("Booting complete\n");
	return ouch;
}

void shutdownOuch(char* imagePath, struct system* ouch)
{
	logg("Shutting down\n");

	//file system writeback and dealloc
	struct fileNode* root = ouch->root;
	unmountRootImage(imagePath, root);
	if (root) freeFileSystem(root);

	//dealloc rest of system
	freeStreamPool(ouch);	
	freeProcPool(ouch);
}


void sigHandler(int sig)
{
	sprintf(cTemp, "Signal detected: %d", sig);
	logg(cTemp);
	isRunning = false;
}

void ouch(char* imagePath)
{
	signal(SIGINT, sigHandler);
	signal(SIGTERM, sigHandler);
	struct system ouch = bootOuch(imagePath);
	struct system* ouchPtr = &ouch;
	logg("\n");

	//check for root
	if (!ouchPtr->root)
		logg("Booting failed, shuting down\n");	
	else
		while (isRunning && runPool(ouchPtr));

	logg("\n");
	shutdownOuch(imagePath, ouchPtr);
}

void test(char* imagePath, int l)
{
	for (int i = 0; i < l; i++)
		ouch(imagePath);
}
