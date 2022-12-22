#include "files.h"






int main()
{
	struct fileNode* root = mountRootImage("D:\\ProjekteC\\OuchSystem\\image.bin");
	printf("Root: %p", root);
	printImage(root, 0);


	return 0;
}