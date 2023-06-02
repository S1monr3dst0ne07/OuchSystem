
#include "utils.h"
#include "files.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

struct fileNodeHeader
{
	char type;
	char name[nodeNameLimit];
	unsigned int contLen;
}
#ifndef _WIN32
__attribute__((packed))
#endif
;


#define fileNodeSize(x) (sizeof(struct fileNodeHeader) + x->contLen)

//-------------------
//system stuff

struct fileNode* parseNode(const char* image)
{
	fguard(image, "", NULL);

	struct fileNodeHeader* head = (struct fileNodeHeader*)image;
	const char* contPtr = sizeof(struct fileNodeHeader) + image;

	struct fileNode* node = malloc(sizeof(struct fileNode));
	fguard(node, msgMallocGuard, NULL);
	memset(node, 0x0, sizeof(struct fileNode));

	memcpy(&node->name, &head->name, nodeNameLimit);
	node->type = head->type;
	node->contLen = head->contLen;

	int subIndex = 0;

	switch (node->type)
	{
	case fileNodeDir:
		while (subIndex < node->contLen)
		{
			//parse subnode
			struct fileNode* subNode = parseNode(contPtr + subIndex);

			//move subIndex
			//subIndex += sizeof(struct fileNodeHeader) + subNode->contLen;
			subIndex += fileNodeSize(subNode);

			//link subnode
			node->subNodes[node->subCount++] = subNode;
		}
		break;

	case fileNodeFile:
		node->contPtr = malloc(node->contLen);
		fguard(node->contPtr, msgMallocGuard, NULL);
		memcpy(node->contPtr, contPtr, node->contLen);

		break;

	case fileNodeInvaild:
		flog("fileNodeInvaild!\n");
		break;
	}

	return node;
}

//mounts file system from host system image file
struct fileNode* mountRootImage(char* path)
{
	flog("Parsing root image from %s\n", path);

	FILE* fp = fopen(path, "rb");
	fguard(fp, "Root image file not found\n", NULL);

	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	flog("Root image found: %d bytes\n", size);

	//read image
	char* rawContent = (char*)malloc(sizeof(char) * size);
	if (!rawContent) return NULL;
	fread(rawContent, size, sizeof(char), fp);
	fclose(fp);

	//parse
	struct fileNode* root = parseNode(rawContent);
	free(rawContent);

	flog("Root image parsed successfully\n");
	return root;
}

void freeFileSystem(struct fileNode* root)
{
	switch (root->type)
	{	
	case fileNodeDir:
		for (int i = 0; i < root->subCount; i++)
			freeFileSystem(root->subNodes[i]);
		break;
	case fileNodeFile:
		free(root->contPtr);
		break;

	case fileNodeInvaild:
		break;
	}

	free(root);
}


char* genNode(struct fileNode* node)
{
	//only vaild nodes can be gen'd
	fguard((node->type != fileNodeInvaild), "Invaild node, unable to generate image\n", NULL);

	//alloc iamge
	int imageSize = sizeof(struct fileNodeHeader) + node->contLen;
	char* image = malloc(imageSize);
	char* contPtr = sizeof(struct fileNodeHeader) + image;

	//populate metadata
	struct fileNodeHeader* head = (struct fileNodeHeader*)image;
	fguard(head, msgMallocGuard, NULL);
	memcpy(&head->name, &node->name, nodeNameLimit);
	head->type    = node->type;
	head->contLen = node->contLen;

	//content
	int subIndex = 0;

	switch (node->type)
	{
	case fileNodeFile:
		memcpy(contPtr, node->contPtr, node->contLen);
		break;
	case fileNodeDir:
		for (int subCount = 0; subCount < node->subCount; subCount++)
		//while (subCount > node->subCount)
		{
			//get node and len
			struct fileNode* subNode = node->subNodes[subCount];
			int subLen = fileNodeSize(subNode);

			//gen subnode
			char* subImage = genNode(subNode);

			//link into supernode
			memcpy(contPtr + subIndex, subImage, subLen);

			//free copied subImage
			free(subImage);

			//move subIndex
			subIndex += subLen;
		}
		break;
	case fileNodeInvaild:
		flog("fileNodeInvaild!\n");
		break;
	}

	return image;
}

void unmountRootImage(char* path, struct fileNode* root)
{
	flog("Unmounting file system to %s\n", path);
	fguard(root, "Root node corrupted, unable to generate image\n", );

	FILE* fp = fopen(path, "wb");
	fguard(fp, "Root image file not found\n", );

	//generate image from file system
	char* image = genNode(root);

	//write image to disk
	fwrite(image, sizeof(char), fileNodeSize(root), fp);
	fclose(fp);

	//free raw image
	free(image);

	flog("File system unmounted successfully\n");
}


//----------------
//getters
struct fileNode* getSubNodeByName(struct fileNode* super, char* name)
{
	fguard((strlen(name) < nodeNameLimit), "getSubNodeByName: name over limit\n", NULL);

	for (int i = 0; i < super->subCount; i++)
	{
		struct fileNode* sub = super->subNodes[i];
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
	fguard(output, msgMallocGuard, NULL);

	int pathIndex = 0;
	int bufferIndex = 0;
	for (int i = 0; ; i++)
	{
		if (path[i] == '/' || path[i] == 0)
		{
			//if a subterminator is found, relocate the buffer
			char* temp = (char*)malloc(bufferIndex + 1);
			fguard(temp, msgMallocGuard, NULL);
			memset(temp, 0x0, bufferIndex + 1);

			strncat(temp, buffer, bufferIndex);
			bufferIndex = 0;

			output->dirPath[pathIndex] = temp;
			pathIndex++;

			if (!path[i]) break;
		}
		else 
			buffer[bufferIndex++] = path[i];
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

struct filePath* cloneFilePath(struct filePath* src)
{
	struct filePath* dst = malloc(sizeof(struct filePath));
	if (!dst) return NULL;
	dst->len = src->len;

	for (int i = 0; i < src->len; i++)
	{ 
		char* srcDir = src->dirPath[i];
		if (!srcDir) continue;

		char* dstDir = malloc(strlen(srcDir));
		if (!dstDir) return NULL;
		strcpy(dstDir, srcDir);
		dst->dirPath[i] = dstDir;
	}

	return dst;
}

//-----------------------------
//io routines


//THIS WILL ALLOCATE MEMORY
struct file readFile(struct system* ouch, struct filePath* path)
{
	struct file out = { 0 };

	struct fileNode* root = ouch->root;
	fguard(root, "", out);

	struct fileNode* temp = getNodeByPath(root, path);
	fguard(temp, "", out);

	out.contLen = temp->contLen;	
	char* contPtr = malloc(out.contLen);
	fguard(contPtr, msgMallocGuard, out);
	memcpy(contPtr, temp->contPtr, out.contLen);
	out.contPtr = contPtr;

	return out;
}

char* readFileContent(struct system* ouch, struct filePath* path)
{
	struct file f = readFile(ouch, path);
	char* out = malloc(f.contLen + 1);
	fguard(out, msgMallocGuard, NULL);
	memcpy(out, f.contPtr, f.contLen);
	out[f.contLen] = '\0';

	free(f.contPtr);
	return out;
}


bool writeFile(struct system* ouch, struct filePath* path, struct file f)
{
	struct fileNode* root = ouch->root;
	fguard(root, "", false);

	struct fileNode* temp = getNodeByPath(root, path);
	fguard(temp, "", false);

	//free old content
	free(temp->contPtr);

	char* contPtr = malloc(f.contLen);
	fguard(contPtr, msgMallocGuard, false);
	memcpy(contPtr, f.contPtr, f.contLen);
	temp->contLen = f.contLen;
	temp->contPtr = f.contPtr;

	return true;
}

//BE CAREFUL WITH THIS
char* getFileContentPtr(struct system* ouch, struct filePath* path)
{
	struct fileNode* root = ouch->root;
	fguard(root, "", NULL);

	struct fileNode* temp = getNodeByPath(root, path);
	fguard(temp, "", NULL);

	return temp->contPtr;
}

bool isFile(struct system* ouch, struct filePath* path)
{ 
	return getFileContentPtr(ouch, path) ? true : false; 
}

/*
void printImage(struct fileNode* ptr, int l)
{
	for (int i = 0; i < l; i++) printf("\t");
	printf("%s %d\n", ptr->name, ptr->contLen);

	//if (ptr->type == 2) printf("%s\n", ptr->contPtr);

	for (int i = 0; i < ptr->subCount; i++) printImage(ptr->subNodes[i], l + 1);
	
}
*/