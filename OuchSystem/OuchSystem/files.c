#include <stdio.h>

struct fileNode
{
	char* name;	
	int type; //(0 -> dir, 1 -> file)

	//only used if type == 0
	struct fileNodeList* dirs;  

	//only used if type == 1
	char* content;				
	int ext;

	//temporary id for files
	int tID;


};
struct fileNodeList
{
	struct fileNode* content;
	struct fileNodeList* next;
};



/*
mounts file system from host system image file
0x00 -> terminator

0x01 -> directory : recursiv
0x02 -> file      : recursiv

0x10 -> name : str
0x11 -> type : int (0 -> .txt, 1 -> .s1)
*/
struct dir* mountRootImage(char* path)
{
	FILE* fp = fopen(path, "r");


}

struct 

struct dir* parseDir(FILE* fp)
{
	
}

struct file* parseFile(FILE* fp)
{


}



