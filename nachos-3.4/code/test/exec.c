#include "syscall.h"


int main() {
    int ok = Exec("hello.coff");
    if (ok == -1)
        Exit(-1);
       
    // If Exec succeeds, this line never executes
    Exit(0);
}
