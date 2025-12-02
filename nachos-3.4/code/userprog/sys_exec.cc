#include "system.h"
#include "addrspace.h"
#include "memmanage.h"

static void ExecStart(int arg)
{
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();
    machine->Run();          // run user code, never returns
    ASSERT(FALSE);           // just in case
}

// -------------------------------------------------------------
// do_Exec(filenameAddr)
// filenameAddr: user virtual address of a null-terminated string
// Returns (in r2) the pid of the new process, or -1 on failure.
// -------------------------------------------------------------
void do_Exec(int filenameAddr)
{
    char filename[256];
    int value;
    int i;

    for (i = 0; i < 255; i++) {
        if (!machine->ReadMem(filenameAddr + i, 1, &value)) {
            // On read failure, return -1
            machine->WriteRegister(2, -1);
            return;
        }
        filename[i] = (char)value;
        if (filename[i] == '\0')
            break;
    }
    filename[255] = '\0';

    OpenFile *executable = fileSystem->Open(filename);
    if (executable == NULL) {
        printf("Unable to open file %s\n", filename);
        machine->WriteRegister(2, -1);
        return;
    }

    Thread *t = new Thread(filename);   


    static int nextPid = 1;
    t->pid = nextPid++;


    AddrSpace *space = new AddrSpace(executable);
    delete executable;

    t->space = space;

    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    t->Fork(ExecStart, 0);
    interrupt->SetLevel(oldLevel);

    machine->WriteRegister(2, t->pid);
}
