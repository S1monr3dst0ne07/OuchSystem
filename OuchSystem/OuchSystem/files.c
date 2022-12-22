#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "files.h"
#include "utils.h"

struct fileNode
{
	char* name;	
	int type; //(1 -> dir, 2 -> file)
	int prior;

	//only used if type == 0
	struct fileNode* subNodes[subNodeCount];
	int count; //number of subnodes

	//only used if type == 1
	char* content;

	//temporary id for files
	int tID;


};

//keeps content of image and pointer to currently parsing point
struct rawImage
{
	char* rawContent;
	int parseIndex;

};

struct fileNode* allocFileNode()
{
	return (struct fileNode*)malloc(sizeof(struct fileNode));
}
struct rawImage* allocRawImage(char* rawContent)
{
	struct rawImage* image = (struct rawImage*)malloc(sizeof(struct rawImage));
	image->parseIndex = 0;
	image->rawContent = rawContent;
	return image;
}

//consumes charactor from rawImage
char consu(struct rawImage* image)
{
	return image->rawContent[image->parseIndex++];
}


//mounts file system from host system image file
struct fileNode* mountRootImage(char* path)
{
	sprintf(cTemp, "Parsing root image from %s\n", path);
	log(cTemp);

	FILE* fp = fopen(path, "rb");
	if (!fp)
	{
		log("Root image file not found\n");
		return NULL;
	}

	fseek(fp, 0, SEEK_END);
	int size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	sprintf(cTemp, "Root image found: %d bytes\n", size);
	log(cTemp);

	char* rawContent = (char*)malloc(sizeof(char) * size);
	if (!rawContent) return NULL;
	fread(rawContent, size, sizeof(char), fp);
	struct rawImage* image = allocRawImage(rawContent);


	//consume header and check image contains nodes
	//parseNode doesn't want the header
	if (!consu(image))
	{
		log("Root image empty");
		return NULL;
	}


	struct fileNode* root = parseNode(image);
	log("Root image parsed successfully\n");
	return root;
}


//parses node, without 0x10 header
struct fileNode* parseNode(struct rawImage* image)
{
	char* name  = readTermed(image); //<name> 0x01
	char* prior = readTermed(image); //<prior> 0x01

	struct fileNode* newNode = allocFileNode();
	newNode->name = name;
	newNode->prior = (int)(*prior);
	newNode->content = NULL;
	newNode->count = 0;

	//type of node
	int type = (int)consu(image);
	switch (type)
	{
	//directory
	case 0x11:
		newNode->type = 1;

		//iterate subnodes, parse recursively
		//calling a function with a side effect of beheading the subnodes if fine
		//because parseNode doesn't want the head of a node
		for (int offset = 0; consu(image) != imageTermi; offset++)
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

//reads 0x01 terminated string
char* readTermed(struct rawImage* image)
{
	//save string start
	int stringOrigin = image->parseIndex;

	//read length of string
	int len = 1; //start at one for terminator
	while (consu(image) != imageTermi) len++;

	char* output = (char*)malloc(len * sizeof(char*));
	for (int i = 0; i < len; i++)
		output[i] = image->rawContent[stringOrigin + i];

	//set terminator at end of string
	output[len - 1] = 0;

	return output;
}


void printImage(struct fileNode* ptr, int l)
{
	for (int i = 0; i < l; i++) printf("\t");
	printf("%s %d\n", ptr->name, ptr->prior);

	if (ptr->type == 2) printf("%p\n", ptr->content);

	for (int i = 0; i < ptr->count; i++) printImage(ptr->subNodes[i], l + 1);
	
}
