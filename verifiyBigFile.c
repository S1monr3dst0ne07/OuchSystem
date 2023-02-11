#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
Does sanity calculations for bigFile.baabnq
Result for 10000: 58267

*/

static short unsigned x = 1;

short unsigned xor()
{
  x = x ^ (x << 7);
  x = x ^ (x >> 9);
  x = x ^ (x << 8);
  return x;
}


int main() 
{
    int len = 0;
    char buffer[16];
    for (int i = 0; i < 10000; i++)
    {
      sprintf(buffer, "%d\n", xor());
      len += strlen(buffer);
    }

  
    printf("%d\n", len);
    return 0;
}
  