// exception.cc 
//  Entry point into the Nachos kernel from user programs.
//  There are two kinds of things that can cause control to
//  transfer back to here from user code:
//
//  syscall -- The user code explicitly requests to call a procedure
//  in the Nachos kernel.  Right now, the only function we support is
//  "Halt".
//
//  exceptions -- The user code does something that the CPU can't handle.
//  For instance, accessing memory that doesn't exist, arithmetic errors,
//  etc.  
//
//  Interrupts (which can also cause control to transfer from user
//  code into the Nachos kernel) are handled elsewhere.
//
//  For now, this only handles the Halt() system call, plus
//  Yield, Exit, Exec, and Join that we added.
//  Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

#ifdef USER_PROGRAM
#include "addrspace.h"
#include "filesys.h"
#endif

// ----------------------------------------------------------------------
// Simple bookkeeping for Exit/Join. We store the exit status per pid,
// and Join keeps checking if that pid is finished, yielding in between.
// ----------------------------------------------------------------------
static int  exitStatus[MaxThreads];
static bool finished[MaxThreads];    // defaults to false at startup

// Advance the user program counters so we don't repeat the same syscall.
static void
AdvancePC()
{
    int pc     = machine->ReadRegister(PCReg);
    int nextPC = machine->ReadRegister(NextPCReg);

    machine->WriteRegister(PrevPCReg, pc);
    machine->WriteRegister(PCReg, nextPC);
    machine->WriteRegister(NextPCReg, nextPC + 4);
}

// Copy a user string (from virtual memory) into a kernel buffer.
// Used by Exec to read the filename from r4.
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

//----------------------------------------------------------------------
// ExceptionHandler
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);   // r2 = syscall code

    if (which == SyscallException) {

        // current process id (SpaceId); if not set yet, just -1.
        int pid = -1;
#ifdef USER_PROGRAM
        pid = currentThread->spaceId;
#endif

        switch (type) {

        case SC_Halt:
            DEBUG('a', "System Call: %d invoked Halt\n", pid);
            DEBUG('a', "Shutdown, initiated by user program.\n");
            interrupt->Halt();
            break;

        case SC_Yield:
            // User-level Yield(): just yield the current Nachos thread
            DEBUG('a', "System Call: %d invoked Yield\n", pid);
            currentThread->Yield();
            AdvancePC();   // move past the Yield() syscall
            break;

        case SC_Exit:
        {
            int status = machine->ReadRegister(4);   // arg1 = exit status
            DEBUG('a', "System Call: %d invoked Exit(%d)\n", pid, status);

            // Required print from project spec:
            // Process [pid] exits with [status]
            printf("Process %d exits with %d\n", pid, status);

#ifdef USER_PROGRAM
            // Record exit status and mark as finished so Join() can see it.
            if (pid >= 0 && pid < MaxThreads) {
                exitStatus[pid] = status;
                finished[pid]   = true;
            }
#endif
            // Finish this thread (never returns).
            currentThread->Finish();

            // If Finish() ever returned (it shouldn't), don't re-execute syscall.
            AdvancePC();
            break;
        }

        case SC_Exec:
        {
#ifdef USER_PROGRAM
            int filenameAddr = machine->ReadRegister(4);
            DEBUG('a', "System Call: %d invoked Exec\n", pid);

            char filename[256];
            CopyUserString(filenameAddr, filename, 256);

            // Required debug print
            printf("Exec Program: %d loading %s\n", pid, filename);

            OpenFile *executable = fileSystem->Open(filename);
            if (executable == NULL) {
                printf("Exec: cannot open %s\n", filename);
                machine->WriteRegister(2, -1);
                AdvancePC();
                break;
            }

            // Replace old address space with a new one for this program.
            AddrSpace *oldSpace = currentThread->space;
            if (oldSpace != NULL) {
                delete oldSpace;
            }

            AddrSpace *newSpace = new AddrSpace(executable);
            delete executable;

            if (newSpace == NULL) {
                printf("Exec: cannot create address space for %s\n", filename);
                machine->WriteRegister(2, -1);
                AdvancePC();
                break;
            }

            currentThread->space = newSpace;

            newSpace->InitRegisters();
            newSpace->RestoreState();

            // Spec: write 1 to r2 indicating Exec() succeeded.
            machine->WriteRegister(2, 1);

            // Start the new program. Does not return on success.
            machine->Run();

            // If we reach here, machine->Run() returned unexpectedly.
            machine->WriteRegister(2, -1);
            AdvancePC();
#else
            machine->WriteRegister(2, -1);
            AdvancePC();
#endif
            break;
        }

        case SC_Join:
        {
            SpaceId childPid = machine->ReadRegister(4);  // arg1 in r4
            DEBUG('a', "System Call: %d invoked Join(%d)\n", pid, childPid);

#ifdef USER_PROGRAM
            // Basic range check
            if (childPid < 0 || childPid >= MaxThreads) {
                machine->WriteRegister(2, -1);  // error
                AdvancePC();
                break;
            }

            // Simple implementation guide behavior:
            // "keep on checking if the requested process is finished.
            //  if not, yield the current process."
            while (!finished[childPid]) {
                currentThread->Yield();
            }

            // Return the child's exit code in r2
            machine->WriteRegister(2, exitStatus[childPid]);
#else
            // If somehow no USER_PROGRAM, just return -1
            machine->WriteRegister(2, -1);
#endif

            AdvancePC();
            break;
        }

        case SC_Fork:
        {
            // Temporary stub for Fork; real implementation is teammate's job.
            DEBUG('a', "System Call: %d invoked Fork (stub, not implemented)\n", pid);

            machine->WriteRegister(2, -1);  // indicate failure
            AdvancePC();
            break;
        }

        // other syscalls (Kill, etc.) go here later

        default:
            printf("Unexpected system call %d\n", type);
            ASSERT(FALSE);
        }

    } else {
        // Non-syscall user-mode exceptions (page faults, illegal op, etc.)
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}
