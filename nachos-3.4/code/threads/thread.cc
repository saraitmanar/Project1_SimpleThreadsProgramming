// thread.cc 
//  Routines to manage threads.  There are four main operations:
//
//  Fork -- create a thread to run a procedure concurrently
//  Finish -- called when the forked procedure finishes, to clean up
//  Yield -- relinquish control over the CPU to another ready thread
//  Sleep -- relinquish control over the CPU, but thread is now blocked.
//
//  See comments in the original Nachos distribution for details.

#include "copyright.h"
#include "thread.h"
#include "switch.h"
#include "synch.h"
#include "system.h"

#define STACK_FENCEPOST 0xdeadbeef  // put at top of the execution stack

//----------------------------------------------------------------------
// Thread::Thread
//  Initialize a thread control block, so that we can then call
//  Thread::Fork.
//----------------------------------------------------------------------

Thread::Thread(const char* threadName)
{
    name = threadName;
    stackTop = NULL;
    stack = NULL;
    status = JUST_CREATED;

#ifdef USER_PROGRAM
    space = NULL;
    spaceId = -1;                       // “no pid” initially
    exitCode = 0;                       // default exit status

 
    joinSem = new Semaphore((char*)"joinSem", 0);
    waitingThread = NULL;
#endif
}

//----------------------------------------------------------------------
// Thread::~Thread
//  De-allocate a thread.
//----------------------------------------------------------------------

Thread::~Thread()
{
    DEBUG('t', "Deleting thread \"%s\"\n", name);

    ASSERT(this != currentThread);
    if (stack != NULL)
        DeallocBoundedArray((char *) stack, StackSize * sizeof(int));

#ifdef USER_PROGRAM
    if (joinSem != NULL) {
        delete joinSem;
        joinSem = NULL;
    }
#endif
}

//----------------------------------------------------------------------
// Thread::Fork
//----------------------------------------------------------------------

void 
Thread::Fork(VoidFunctionPtr func, int arg)
{
    DEBUG('t', "Forking thread \"%s\" with func = 0x%x, arg = %d\n",
          name, (int) func, arg);
    
    StackAllocate(func, arg);

    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    scheduler->ReadyToRun(this);    
                                    
    (void) interrupt->SetLevel(oldLevel);
}    

//----------------------------------------------------------------------
// Thread::CheckOverflow
//----------------------------------------------------------------------

void
Thread::CheckOverflow()
{
    if (stack != NULL)
#ifdef HOST_SNAKE          // Stacks grow upward on the Snakes
        ASSERT(stack[StackSize - 1] == STACK_FENCEPOST);
#else
        ASSERT((int) *stack == (int) STACK_FENCEPOST);
#endif
}

//----------------------------------------------------------------------
// Thread::Finish
//  Called by ThreadRoot when a thread is done executing the 
//  forked procedure.
//----------------------------------------------------------------------

void
Thread::Finish ()
{
    (void) interrupt->SetLevel(IntOff);     
    ASSERT(this == currentThread);
    
    DEBUG('t', "Finishing thread \"%s\"\n", getName());
    
    threadToBeDestroyed = currentThread;

    // NOTE: we do NOT do any Join signaling here. Exit() will
    // handle recording exit status; our simplified Join
    // implementation uses a separate array instead of semaphores.

    Sleep();                    // invokes SWITCH
    // not reached
}

//----------------------------------------------------------------------
// Thread::Yield
//----------------------------------------------------------------------

void
Thread::Yield ()
{
    Thread *nextThread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    
    ASSERT(this == currentThread);
    
    DEBUG('t', "Yielding thread \"%s\"\n", getName());
    
    nextThread = scheduler->FindNextToRun();
    if (nextThread != NULL) {
        scheduler->ReadyToRun(this);
        scheduler->Run(nextThread);
    }
    (void) interrupt->SetLevel(oldLevel);
}

//----------------------------------------------------------------------
// Thread::Sleep
//----------------------------------------------------------------------

void
Thread::Sleep ()
{
    Thread *nextThread;
    
    ASSERT(this == currentThread);
    ASSERT(interrupt->getLevel() == IntOff);
    
    DEBUG('t', "Sleeping thread \"%s\"\n", getName());

    status = BLOCKED;
    while ((nextThread = scheduler->FindNextToRun()) == NULL)
        interrupt->Idle();  // no one to run, wait for an interrupt
        
    scheduler->Run(nextThread); // returns when we've been signalled
}

//----------------------------------------------------------------------
// ThreadFinish, InterruptEnable, ThreadPrint
//----------------------------------------------------------------------

static void ThreadFinish()    { currentThread->Finish(); }
static void InterruptEnable() { interrupt->Enable(); }
void ThreadPrint(int arg){ Thread *t = (Thread *)arg; t->Print(); }

//----------------------------------------------------------------------
// Thread::StackAllocate
//----------------------------------------------------------------------

void
Thread::StackAllocate (VoidFunctionPtr func, int arg)
{
    stack = (int *) AllocBoundedArray(StackSize * sizeof(int));

#ifdef HOST_SNAKE
    // HP stack works from low addresses to high addresses
    stackTop = stack + 16;  // HP requires 64-byte frame marker
    stack[StackSize - 1] = STACK_FENCEPOST;
#else
    // i386 & MIPS & SPARC stack works from high addresses to low addresses
#ifdef HOST_SPARC
    // SPARC stack must contains at least 1 activation record to start with.
    stackTop = stack + StackSize - 96;
#else  // HOST_MIPS  || HOST_i386
    stackTop = stack + StackSize - 4;   // -4 to be on the safe side!
#ifdef HOST_i386
    // the 80386 passes the return address on the stack.  In order for
    // SWITCH() to go to ThreadRoot when we switch to this thread, the
    // return addres used in SWITCH() must be the starting address of
    // ThreadRoot.
    *(--stackTop) = (int)ThreadRoot;
#endif
#endif  // HOST_SPARC
    *stack = STACK_FENCEPOST;
#endif  // HOST_SNAKE
    
    machineState[PCState]        = (int) ThreadRoot;
    machineState[StartupPCState] = (int) InterruptEnable;
    machineState[InitialPCState] = (int) func;
    machineState[InitialArgState]= arg;
    machineState[WhenDonePCState]= (int) ThreadFinish;
}

#ifdef USER_PROGRAM
#include "machine.h"

//----------------------------------------------------------------------
// Thread::SaveUserState
//----------------------------------------------------------------------

void
Thread::SaveUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
        userRegisters[i] = machine->ReadRegister(i);
}

//----------------------------------------------------------------------
// Thread::RestoreUserState
//----------------------------------------------------------------------

void
Thread::RestoreUserState()
{
    for (int i = 0; i < NumTotalRegs; i++)
        machine->WriteRegister(i, userRegisters[i]);
}
#endif
