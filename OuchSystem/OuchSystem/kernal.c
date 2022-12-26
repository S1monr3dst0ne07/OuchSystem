#include <signal.h>
#include <stdbool.h>

#include "kernal.h"
#include "utils.h"
#include "files.h"
#include "process.h"

static volatile bool isRunning = true;


//only one instance of struct system can exist
struct system boot(char* imagePath)
{
	log("Booting Ouch 1.0 ...\n");

	struct fileNode* root = mountRootImage(imagePath);

	struct filePath* autoStartupPath = parseFilePath("auto.och");
	char* autoStartupFile = readFileContent(root, autoStartupPath);


	struct system ouch = {
		.root = root,
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
	if (source)
		parseProcess(source);

	free(testPath);

	while (isRunning);

}