#include "syscall.h"

void infinity(){
	int i = 0;
	for (; ;) Yield();
}

int main()
{
	int ret;
	int id = Fork(infinity);
	Yield();
	ret = Kill(id);
	Exit(ret);
        return ret;
}
