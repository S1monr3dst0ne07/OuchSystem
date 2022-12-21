#include <stdio.h>

struct fileNode
{
	char* name;	
	int type; //(1 -> dir, 2 -> file)
	int prior;

	//only used if type == 0
	struct fileNodeList* dirs;

	//only used if type == 1
	char* content;

	//temporary id for files
	int tID;


};
struct fileNodeList
{
	struct fileNode* content;
	struct fileNodeList* next;
};




//mounts file system from host system image file
struct dir* mountRootImage(char* path)
{
	FILE* fp = fopen(path, "r");


}

struct dir* parseNode(FILE* fp)
{
	

}

//reads 0x00 terminated string from file ptr
char* readContent(FILE* fp)
{
	//get length of string
	FILE* temp = fp;
	int strLen = 0;
	while (fgetc(temp)) strLen++;

	char* output = (char*)malloc(sizeof(char) * strLen);

	//read string
	int i = 0;
	while (output[i++] = fgetc(temp));

	return output;
}
