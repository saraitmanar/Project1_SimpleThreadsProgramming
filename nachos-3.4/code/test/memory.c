#include "syscall.h"

int array[128];

int main()
{
	int i;
	for (i = 0; i < 128; i++) 
		array[i] = 42;

	Exit(0);
}

