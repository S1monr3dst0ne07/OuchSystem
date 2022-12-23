#include "files.h"
#include "utils.h"





int main()
{
	struct fileNode* root = mountRootImage("D:\\ProjekteC\\OuchSystem\\image.bin");

	struct filePath* autoStartupPath = parseFilePath("auto.och");
	char* autoStartupFile = readFileContent(root, autoStartupPath);
	

	printf("%s\n", autoStartupFile);

	free(autoStartupFile);
	free(autoStartupPath);

	return 0;
}