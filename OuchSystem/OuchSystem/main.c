
#include "kernal.h"
#include "process.h"
#include "files.h"
#include "utils.h"


int main()
{
	const char* imagePath = "D:\\ProjekteC\\OuchSystem\\image.bin";
	//ouch("D:\\ProjekteC\\OuchSystem\\image.bin");
	test(imagePath, 10000);

	return 0;
}