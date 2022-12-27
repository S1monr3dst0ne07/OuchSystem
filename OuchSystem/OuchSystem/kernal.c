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


	struct filePath* testPath = parseFilePath("test/test1.s1");
	char* source = readFileContent(ouch.root, testPath);
	if (!source) return;

	struct process* test = parseProcess(source);
	launchProcess(test, &ouch);
	launchProcess(test, &ouch);


	while (isRunning);

	free(source);
	free(testPath);
}