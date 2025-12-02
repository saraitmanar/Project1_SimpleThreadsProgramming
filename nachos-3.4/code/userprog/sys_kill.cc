// sys_kill.cc
// Simple stub for do_Kill so the project compiles.

#include "system.h"

#ifdef USER_PROGRAM
#include "addrspace.h"
#include "memmanage.h"
#endif

void do_Kill(int pid)
{
    int callerPid = -1;
#ifdef USER_PROGRAM
    callerPid = currentThread->spaceId;   // use spaceId as our pid
#endif

    printf("System Call: [%d] invoked Kill(%d) (do_Kill stub)\n",
           callerPid, pid);
}
