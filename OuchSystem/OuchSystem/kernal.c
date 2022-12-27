#include <signal.h>
#include <stdbool.h>

#include "kernal.h"
#include "utils.h"
#include "files.h"
#include "process.h"

static volatile bool isRunning = true;
char cTemp[2048];


//only one instance of struct system can exist
struct system boot(char* imagePath)
{
	log("Booting Ouch 1.0 ...\n");

	struct fileNode* root = mountRootImage(imagePath);

	struct filePath* autoStartupPath = parseFilePath("auto.och");
	char* autoStartupFile = readFileContent(root, autoStartupPath);


	struct system ouch = {
		.root = root,
		.pool = allocProcPool(),
	};

	printf("%s\n", autoStartupFile);

	free(autoStartupFile);
	free(autoStartupPath);

	return ouch;
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

	struct filePath* testPath1 = parseFilePath("test/test1.s1");
	struct filePath* testPath2 = parseFilePath("test/test2.s1");
	char* source1 = readFileContent(ouchPtr->root, testPath1);
	char* source2 = readFileContent(ouchPtr->root, testPath2);
	if (!source1) return;
	if (!source2) return;

	struct process* test1 = parseProcess(source1);
	struct process* test2 = parseProcess(source2);
	launchProcess(test1, ouchPtr);
	launchProcess(test2, ouchPtr);


	while (isRunning) RunPool(ouchPtr);

	free(source1);
	free(source2);
	free(testPath1);
	free(testPath2);
}