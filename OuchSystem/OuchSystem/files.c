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
	FILE* fp = fopen(path, "r");


}


//parses node, without 0x10 header
struct fileNode* parseNode(char* ptr)
{
	char* name  = readTermed(ptr);
	char* prior = readTermed(ptr);

	struct fileNode* newNode = allocFileNode();
	newNode->name = name;
	newNode->prior = (int)(*prior);
	newNode->subNodes = NULL;

	int type = (int)(*(ptr++));
	switch (type)
	{
	case 0x11:
		newNode->type = 1;

		//init linked list
		struct nodeList* listHead = allocNodeList();
		newNode->subNodes = listHead;

		while (*(ptr++) != imageTermi)
		{
			struct nodeList* listNew = allocNodeList();
			listHead->content = parseNode(ptr);
			listHead->next = listNew;
			listHead = listNew;
		}

		listHead->isEnd = true;
		break;


	case 0x12:
		newNode->type = 2;
		newNode->content = readTermed(ptr);
		break;
	}

	return newNode;
}

//reads 0x01 terminated string
char* readTermed(char* ptr)
{
	char* temp = ptr;
	int len = 0;
	while (*(temp++) != imageTermi) len++;

	char* output = (char*)malloc(len * sizeof(char*));
	int i = 0;
	while (*(ptr++) != imageTermi) output[i++] = *(ptr-1);
	
	return output;
}
