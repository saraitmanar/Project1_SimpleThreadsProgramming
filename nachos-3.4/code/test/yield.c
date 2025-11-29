#include "syscall.h"

int
main()
{
    int i;

    for (i = 0; i < 5; i++) {
        Yield();    // ask the kernel to switch to another thread
    }

    Halt();         // stop the machine so we can see it worked
    /* or: Exit(0); if your Exit is working */
    return 0;       // never reached
}