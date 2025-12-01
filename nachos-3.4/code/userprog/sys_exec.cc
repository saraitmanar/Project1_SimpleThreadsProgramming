#include "system.h"
#include "syscall.h"


extern void AdvancePC();


static void copyUserString(int virtAddr, char *buffer, int max)
{
    int ch;
    for (int i = 0; i < max - 1; i++) {
        machine->ReadMem(virtAddr + i, 1, &ch);
        buffer[i] = (char)ch;
        if (ch == '\0') return;
    }
    buffer[max - 1] = '\0';
}


int do_Exec(int filenameAddr)
{
    char filename[256];
    copyUserString(filenameAddr, filename, 256);


    OpenFile *executable = fileSystem->Open(filename);
    if (executable == NULL) {
        printf("Exec: cannot open %s\n", filename);
        return -1;
    }


    AddrSpace *oldSpace = currentThread->space;
    delete oldSpace;


    AddrSpace *newSpace = new AddrSpace(executable);
    if (newSpace == NULL) {
        printf("Exec: cannot create address space for %s\n", filename);
        delete executable;
        return -1;
    }
    delete executable;


    currentThread->space = newSpace;


    newSpace->InitRegisters();
    newSpace->RestoreState();

    machine->WriteRegister(2, 1);

    machine->Run();


    return -1;
}
