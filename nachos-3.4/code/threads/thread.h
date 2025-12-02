// thread.h 
//  Data structures for managing threads.  A thread represents
//  sequential execution of code within a program.
//  So the state of a thread includes the program counter,
//  the processor registers, and the execution stack.
//  
//  See comments in the original Nachos distribution for details.

#ifndef THREAD_H
#define THREAD_H

#include "copyright.h"
#include "utility.h"

#ifdef USER_PROGRAM
#include "machine.h"
#include "addrspace.h"
#endif

// CPU register state to be saved on context switch.  
#define MachineStateSize 18 

// Size of the thread's private execution stack.
#define StackSize   (4 * 1024)    // in words
#define MaxThreads  128

// Thread state
enum ThreadStatus { JUST_CREATED, RUNNING, READY, BLOCKED };

// external function, dummy routine whose sole job is to call Thread::Print
extern void ThreadPrint(int arg);     

class Semaphore;    // forward declaration

// The following class defines a "thread control block" -- which
// represents a single thread of execution.
class Thread {
  private:
    // NOTE: DO NOT CHANGE the order of these first two members.
    // THEY MUST be in this position for SWITCH to work.
    int* stackTop;                        // the current stack pointer
    int machineState[MachineStateSize];   // all registers except for stackTop

    int* stack;                           // Bottom of the stack 
                                          // NULL if this is the main thread
                                          // (If NULL, don't deallocate stack)
    ThreadStatus status;                  // ready, running or blocked
    const char* name;

    void StackAllocate(VoidFunctionPtr func, int arg);
                                          // Allocate a stack for thread.
                                          // Used internally by Fork()

#ifdef USER_PROGRAM
    int userRegisters[NumTotalRegs];
#endif

  public:
    Thread(const char* debugName);        // initialize a Thread 
    ~Thread();                            // deallocate a Thread
                                          // NOTE -- thread being deleted
                                          // must not be running when delete 
                                          // is called

    // basic thread operations
    void Fork(VoidFunctionPtr func, int arg);  // Make thread run (*func)(arg)
    void Yield();                              // Relinquish the CPU if any 
                                               // other thread is runnable
    void Sleep();                              // Put the thread to sleep and 
                                               // relinquish the processor
    void Finish();                             // The thread is done executing
    
    void CheckOverflow();                      // Check if thread has 
                                               // overflowed its stack
    void setStatus(ThreadStatus st) { status = st; }
    const char* getName() { return (name); }
    void Print() { printf("%s, ", name); }

#ifdef USER_PROGRAM
    // A thread running a user program actually has *two* sets of CPU regs
    void SaveUserState();          // save user-level register state
    void RestoreUserState();       // restore user-level register state

    AddrSpace *space;              // User code this thread is running.
    int spaceId;                   // “pid” for this address space

    int exitCode;                  // exit status from Exit(status)

    // These are kept for compatibility, but we will NOT use them in Join()
    // in this simplified design.
    Semaphore *joinSem;           
    Thread *waitingThread;
#endif
};

// Magical machine-dependent routines, defined in switch.s
extern "C" {
// First frame on thread execution stack; 
//      enable interrupts
//  call "func"
//  (when func returns, if ever) call ThreadFinish()
void ThreadRoot();

// Stop running oldThread and start running newThread
void SWITCH(Thread *oldThread, Thread *newThread);
}

#endif // THREAD_H
