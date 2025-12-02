#include "syscall.h"

int main() {
    int pid;

    pid = Fork();
    if (pid == 0) {
        // Child loop forever until killed
        while (1) {
            Write("Child alive...\n", 15, ConsoleOutput);
            Yield();
        }
    } else {
        // Parent waits a bit
        Write("Parent: created child, will sleep\n", 36, ConsoleOutput);
        Yield();
        Yield();

        Write("Parent: killing child now.\n", 28, ConsoleOutput);
        Kill(pid);

        Write("Parent: kill returned, exiting.\n", 32, ConsoleOutput);
        Exit(0);
    }

    return 0;
}
