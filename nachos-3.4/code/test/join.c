#include "syscall.h"

int global_cnt=0;

void sum(){
	int i;
	
	for (i=0;i<100;i++) global_cnt++;
	Yield();
	
	Exit(global_cnt);
}

void fast_exit(){
	Exit(global_cnt);
}

int main()
{
	int i=0;
	SpaceId pid1,pid2,pid3;
	
	global_cnt++;
	pid1 = Fork(fast_exit);
	
	Yield();

	global_cnt++;
	pid2 = Fork(sum);

	Yield();
	
	global_cnt++;
	pid3 = Fork(sum);

	Yield();
	
	Join(pid1);
	Join(pid2);
	Join(pid3);
	
	Exit(global_cnt);
}

