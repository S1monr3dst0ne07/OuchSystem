#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "files.h"

struct fileNode
{
	char* name;	
	int type; //(1 -> dir, 2 -> file)
	int prior;

	//only used if type == 0
	struct nodeList* subNodes;

	//only used if type == 1
	char* content;

	//temporary id for files
	int tID;


};
struct nodeList
{
	struct fileNode* content;
	struct nodeList* next;
	bool isEnd;
};

struct fileNode* allocFileNode()
{
	return (struct fileNode*)malloc(sizeof(struct fileNode*));
}
struct nodeList* allocNodeList()
{
	return (struct nodeList*)malloc(sizeof(struct nodeList*));
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

	char* content = (char*)malloc(sizeof(char) * size);
	fread(content, size, sizeof(char), fp);

	int i = 0;
	if (!content[i++])
	{
		log("Root image empty");
		return NULL;
	}

	return parseNode(content, &i);
}


//parses node, without 0x10 header
struct fileNode* parseNode(char* ptr, int* i)
{
	char* name  = readTermed(ptr, i);
	char* prior = readTermed(ptr, i);
	
	struct fileNode* newNode = allocFileNode();
	newNode->name = name;
	newNode->prior = (int)(*prior);
	newNode->subNodes = NULL;

	int type = (int)ptr[(*i)++];
	switch (type)
	{
	case 0x11:
		newNode->type = 1;

		//init linked list
		struct nodeList* listHead = allocNodeList();
		newNode->subNodes = listHead;

		while (ptr[(*i)++] != imageTermi)
		{
			struct nodeList* listNew = allocNodeList();
			listHead->content = parseNode(ptr, i);
			listHead->next = listNew;
			listHead = listNew;
		}

		listHead->isEnd = true;
		break;


	case 0x12:
		newNode->type = 2;
		newNode->content = readTermed(ptr, i);
		break;
	}

	return newNode;
}

//reads 0x01 terminated string
char* readTermed(char* ptr, int* i)
{
	int len = 0;
	while (ptr[*i + (len++)] != imageTermi);
	len--;

	char* output = (char*)malloc((len+1) * sizeof(char*));
	int x = 0;
	char c;
	while ((c = ptr[(*i)++]) != imageTermi) output[x++] = c;
	output[x] = 0;

	return output;
}



void printImage(struct fileNode* ptr, int l)
{
	for (int i = 0; i < l; i++) printf("\t");
	printf("%s %d\n", ptr->name, ptr->prior);

	struct nodeList* temp = ptr->subNodes;
	while (temp && !temp->isEnd)
	{
		printImage(temp->content, l+1);
		temp = temp->next;
	}
}
