#include "system.h"
#include "addrspace.h"
#include "filesys.h"

// Copy a user string (from virtual memory) into a kernel buffer.
static void
CopyUserString(int virtAddr, char *buffer, int maxLen)
{
    int ch;
    for (int i = 0; i < maxLen - 1; i++) {
        if (!machine->ReadMem(virtAddr + i, 1, &ch)) {
            buffer[i] = '\0';
            return;
        }
        buffer[i] = (char)ch;
        if (ch == 0) {
            return;
        }
    }
    buffer[maxLen - 1] = '\0';
}

// -------------------------------------------------------------
// do_Exec(filenameAddr)
// Steps from the implementation guide:
//
// 1. Read register r4 to get the executable path.
// 2. Replace the process memory with the content of the executable.
// 3. Init registers.
// 4. Write 1 to r2 indicating exec() invoked successfully.
// 5. On success, machine->Run() never returns; on failure, write -1.
// -------------------------------------------------------------
void do_Exec(int filenameAddr)
{
    int pid = -1;
#ifdef USER_PROGRAM
    pid = currentThread->spaceId;   // use spaceId just for printing/debug
#endif

    char filename[256];
    CopyUserString(filenameAddr, filename, 256);

    // Debug/required print
    printf("Exec Program: %d loading %s\n", pid, filename);

    // Open the executable
    OpenFile *executable = fileSystem->Open(filename);
    if (executable == NULL) {
        printf("Exec: cannot open %s\n", filename);
        machine->WriteRegister(2, -1);
        return;
    }

    // Replace old address space with new one
    AddrSpace *oldSpace = currentThread->space;
    if (oldSpace != NULL) {
        delete oldSpace;
    }

    AddrSpace *newSpace = new AddrSpace(executable);
    delete executable;

    if (newSpace == NULL) {
        printf("Exec: cannot create address space for %s\n", filename);
        machine->WriteRegister(2, -1);
        return;
    }

    currentThread->space = newSpace;

    // Initialize registers and restore state
    newSpace->InitRegisters();
    newSpace->RestoreState();

    // Success: write 1 to r2
    machine->WriteRegister(2, 1);

    // Start running the new program; never returns on success
    machine->Run();

    // If we get here, something went wrong
    machine->WriteRegister(2, -1);
}
