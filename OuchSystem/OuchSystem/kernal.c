
#include "kernal.h"
#include "utils.h"
#include "files.h"
#include "process.h"

#include <signal.h>
#include <string.h>

static volatile bool isRunning = true;
char cTemp[2048];

void launchAutoProcesses(struct filePath* autoPath, struct system* ouch)
{
	log("Launching auto startup processes\n\n");
	char* autoStartupFile = readFileContent(ouch->root, autoPath);
	
	if (autoStartupFile == NULL)
	{
		log("Unable to read auto.och file\n");
		return;
	}

	char* splitPtr = strtok(autoStartupFile, "\n");

	while (splitPtr != NULL)
	{
		sprintf(cTemp, "Launching '%s' ... \n", splitPtr);
		log(cTemp);

		bool success = launchProcessFromPath(splitPtr, ouch);
		if (success) log("OK\n");
		else         log("Failed\n");
		
		splitPtr = strtok(NULL, "\n");
	}

	//freeFilePath(autoStartupFile);
	free(autoStartupFile);
	log("\nOK\n");
}

bool launchProcessFromPath(char* pathStr, struct system* ouch)
{
	struct filePath* path = parseFilePath(pathStr);
	char* source = readFileContent(ouch->root, path);
	if (!source) return false;

	struct process* proc = parseProcess(source);
	if (!proc) return false;

	launchProcess(proc, ouch);
	
	freeFilePath(path);
	free(source);
	return true;
}

//only one instance of struct system can exist
struct system boot(char* imagePath)
{
	log("Booting Ouch 1.0 ...\n");

	log("\n");

	struct fileNode* root = mountRootImage(imagePath);
	struct system ouch = {
		.root = root,
		.pool = allocProcPool(),
	};

	log("\n");

	struct filePath* autoPath = parseFilePath("auto.och");
	launchAutoProcesses(autoPath, &ouch);
	freeFilePath(autoPath);

	log("\n");

	log("Booting complete\n");
	return ouch;
}

void shutdown(struct system* ouch)
{
	//todo: write back!!!

	struct fileNode* root = ouch->root;
	if (root) freeFileSystem(root);
	printf("root: %p\n", ouch->root);
	removeProcPool(ouch);
	printf("pool: %p\n", ouch->pool);

}


void sigHandler(int sig)
{
	sprintf(cTemp, "Signal detected: %d", sig);
	log(cTemp);
	isRunning = false;
}

void ouch(char* imagePath)
{
	signal(SIGINT, sigHandler);
	struct system ouch = boot(imagePath);
	struct system* ouchPtr = &ouch;

	//check for root
	if (!ouchPtr->root)
		log("Booting failed, shuting down\n");
	
	/*else
		while (isRunning && runPool(ouchPtr));
	*/

	shutdown(ouchPtr);
}


void test(char* imagePath)
{
	for (int i = 0; i < 1000; i++)
	{
		printf("ITER: %d\n", i);

		ouch(imagePath);
	}
	for (;;) Sleep(100);
}