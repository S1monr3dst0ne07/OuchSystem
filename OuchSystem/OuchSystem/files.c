
#include "files.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "utils.h"

//temporary buffer for parsing and generating
struct rawImage
{
	char* rawContent;
	int index;

};

struct fileNode* allocFileNode()
{
	return (struct fileNode*)malloc(sizeof(struct fileNode));
}


struct rawImage* allocRawImage(char* rawContent)
{
	struct rawImage* image = (struct rawImage*)malloc(sizeof(struct rawImage));
	image->index = 0;
	image->rawContent = rawContent;
	return image;
}
void freeRawImage(struct rawImage* image)
{
	free(image->rawContent);
	free(image);
}


//-------------------
//system stuff

//consumes character from rawImage
char consImage(struct rawImage* image)
{ return image->rawContent[image->index++]; }

void pushImage(struct rawImage* image, char c)
{ image->rawContent[image->index++] = c; }

//reads 0x01 terminated string
char* readTermed(struct rawImage* image)
{
	//save string start
	int stringOrigin = image->index;

	//read length of string
	int len = 1; //start at one for terminator
	while (consImage(image) != imageTermi) len++;

	char* output = (char*)malloc(len * sizeof(char*));
	for (int i = 0; i < len; i++)
		output[i] = image->rawContent[stringOrigin + i];

	//set terminator at end of string
	output[len - 1] = 0;

	return output;
}

//parses node, without 0x10 header
struct fileNode* parseNode(struct rawImage* image)
{
	char* name = readTermed(image); //<name> 0x01
	char* prior = readTermed(image); //<prior> 0x01

	struct fileNode* newNode = allocFileNode();
	newNode->name = name;
	newNode->prior = (int)(*prior);
	newNode->content = NULL;
	newNode->count = 0;

	free(prior);

	//type of node
	int type = (int)consImage(image);
	switch (type)
	{
		//directory
	case 0x11:
		newNode->type = 1;

		//iterate subnodes, parse recursively
		//calling a function with a side effect of beheading the subnodes if fine
		//because parseNode doesn't want the head of a node
		for (int offset = 0; consImage(image) != imageTermi; offset++)
		{
			newNode->subNodes[offset] = parseNode(image);
			newNode->count++;
		}

		break;

		//file
	case 0x12:
		newNode->type = 2;
		newNode->content = readTermed(image);
		break;
	}

	return newNode;
}

void writeTermed(struct rawImage* image, char* str)
{
	for (int i = 0; str[i]; i++) pushImage(image, str[i]);
	pushImage(image, imageTermi); //termi
}

//mounts file system from host system image file
struct fileNode* mountRootImage(char* path)
{
	sprintf(cTemp, "Parsing root image from %s\n", path);
	logg(cTemp);

	FILE* fp = fopen(path, "rb");
	if (!fp)
	{
		logg("Root image file not found\n");
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	sprintf(cTemp, "Root image found: %d bytes\n", size);
	logg(cTemp);

	char* rawContent = (char*)malloc(sizeof(char) * size);
	if (!rawContent) return NULL;
	fread(rawContent, size, sizeof(char), fp);
	struct rawImage* image = allocRawImage(rawContent);


	//consImageme header and check image contains nodes
	//parseNode doesn't want the header
	if (!consImage(image))
	{
		logg("Root image empty\n");
		return NULL;
	}

	struct fileNode* root = parseNode(image);
	freeRawImage(image);

	logg("Root image parsed successfully\n");
	fclose(fp);
	return root;
}

void freeFileSystem(struct fileNode* root)
{
	switch (root->type)
	{	
	case 1:
		for (int i = 0; i < root->count; i++)
			freeFileSystem(root->subNodes[i]);
		break;
	case 2:
		free(root->content);
		break;
	}

	free(root->name);
	free(root);
}

int calcImageSize(struct fileNode* node)
{
	//0x10 name 0x01 0xff 0x01 0x11 subdata 0x01
	// 1    X    2    3    4    5      X     6

	int headerSize = strlen(node->name) + 6;
	int bodySize = 0;

	//count subdata
	switch (node->type)
	{
	case 1:
		for (int i = 0; i < node->count; i++)
			bodySize += calcImageSize(node->subNodes[i]);
		break;
	case 2:
		bodySize = strlen(node->content);
		break;
	}

	return headerSize + bodySize;
}

void genNode(struct fileNode* node, struct rawImage* image)
{
	pushImage(image, 0x10);
	writeTermed(image, node->name);
	pushImage(image, (char)node->prior);
	pushImage(image, imageTermi);

	switch (node->type)
	{
	case 1:
		pushImage(image, 0x11);
		for (int i = 0; i < node->count; i++)
			genNode(node->subNodes[i], image);
		pushImage(image, imageTermi);
		break;

	case 2:
		pushImage(image, 0x12);
		writeTermed(image, node->content);
		break;
	default:
		pushImage(image, 0x13);
		pushImage(image, imageTermi);
		break;
	}

}

void unmountRootImage(char* path, struct fileNode* root)
{
	sprintf(cTemp, "Unmounting file system to %s\n", path);
	logg(cTemp);

	if (!root)
	{
		logg("Root node corrupted, unable to generate image\n");
		return;
	}

	FILE* fp = fopen(path, "wb");
	if (!fp)
	{
		logg("Root image file not found\n");
		return;
	}

	//generate image from file system
	int size = calcImageSize(root);
	char* rawContent = (char*)malloc(sizeof(char) * size);
	memset(rawContent, 0x0, size);
	struct rawImage* image = allocRawImage(rawContent);
	genNode(root, image);

	//write image to disk
	fwrite(rawContent, sizeof(char), size, fp);

	//free raw image
	freeRawImage(image);

	logg("File system unmounted successfully\n");
	fclose(fp);
}


//----------------
//getters
struct fileNode* getSubNodeByName(struct fileNode* top, char* name)
{
	for (int i = 0; i < top->count; i++)
	{
		struct fileNode* sub = top->subNodes[i];
		if (!strcmp(sub->name, name)) return sub;
	}
	return NULL;
}

struct fileNode* getNodeByPath(struct fileNode* root, struct filePath* path)
{
	struct fileNode* temp = root;
	for (int i = 0; i < path->len; i++)
		temp = getSubNodeByName(temp, path->dirPath[i]);
	
	return temp;
}



struct filePath* parseFilePath(char* path)
{
	char buffer[defBufferSize];
	memset(buffer, 0, sizeof(buffer));

	struct filePath* output = (struct filePath*)malloc(sizeof(struct filePath));

	int pathIndex = 0;
	int bufferIndex = 0;
	for (int i = 0;;i++)
	{
		if (path[i] == '/' || path[i] == 0)
		{
			//if a subterminator is found, relocate the buffer
			char* temp = (char*)malloc(sizeof(char) * (bufferIndex + 1));

			strncpy(temp, buffer, bufferIndex);
			temp[bufferIndex] = 0;
			bufferIndex = 0;

			output->dirPath[pathIndex] = temp;
			pathIndex++;

			if (path[i] == 0) break;
		}
		else 
		{ 
			buffer[bufferIndex++] = path[i];
		}
	}

	output->len = pathIndex;
	return output;
}


void freeFilePath(struct filePath* path)
{
	for (int i = 0; i < path->len; i++)
		free(path->dirPath[i]);

	free(path);
}

//-----------------------------
//io routines


//THIS WILL ALLOCATE MEMORY
char* readFileContent(struct system* ouch, struct filePath* path)
{
	struct fileNode* root = ouch->root;
	if (!root) return NULL; 

	struct fileNode* temp = getNodeByPath(root, path);
	if (!temp) return NULL;
	
	return strdup(temp->content);
}

bool writeFileContent(struct system* ouch, struct filePath* path, char* content)
{
	struct fileNode* root = ouch->root;
	if (!root) return false; 

	struct fileNode* temp = getNodeByPath(root, path);
	if (!temp) return false;

	//free old content
	free(temp->content);
	//copy new content
	temp->content = strdup(content);

	return true;
}


void printImage(struct fileNode* ptr, int l)
{
	for (int i = 0; i < l; i++) printf("\t");
	printf("%s %d\n", ptr->name, ptr->prior);

	if (ptr->type == 2) printf("%s\n", ptr->content);

	for (int i = 0; i < ptr->count; i++) printImage(ptr->subNodes[i], l + 1);
	
}
