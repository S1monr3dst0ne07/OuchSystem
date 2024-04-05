
#include "kernel.h"
#include "process.h"
#include "files.h"
#include "utils.h"

#if _WIN32
#define imgPath "D:\\ProjekteC\\OuchSystem\\image.bin"
#elif HETTY
#define imgPath "/home/s1mon/OuchSystem/image.bin"
#else
#define imgPath "/data/ouch/image.bin"
#endif

int main()
{
	ouch(imgPath);
	return 0;
}
