#include "syscall.h"

<<<<<<< HEAD
void usememory(){
	Exec("../test/memory");
}

int main()
{
	int i=0;

	for (i = 0; i < 5; i++) {
		Fork(usememory);
		Yield();
	}
	Exit(0);
}

=======

int main() {
    int ok = Exec("hello.coff");
    if (ok == -1)
        Exit(-1);
       
    // If Exec succeeds, this line never executes
    Exit(0);
}
>>>>>>> 5c55802273dc838c0b094423ebf962968475244f
