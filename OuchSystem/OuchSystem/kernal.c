#include <signal.h>
#include <stdbool.h>

#include "kernal.h"
#include "utils.h"
#include "files.h"

static volatile bool isRunning = true;


//only one instance of struct system can exist
struct system boot(char* imagePath)
{
	log("Booting Ouch 1.0 ...\n");

	struct fileNode* root = mountRootImage(imagePath);

	struct filePath* autoStartupPath = parseFilePath("auto.och");
	char* autoStartupFile = readFileContent(root, autoStartupPath);


	printf("%s\n", autoStartupFile);

	free(autoStartupFile);
	free(autoStartupPath);


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
	boot(imagePath);

	while (isRunning);

}