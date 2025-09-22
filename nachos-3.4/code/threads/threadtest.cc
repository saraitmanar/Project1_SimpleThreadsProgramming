// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "synch.h"
int SharedVariable = 0;

#ifdef HW1_SEMAPHORES
Semaphore * sharedVar = new Semaphore("sharedVar", 1);
Semaphore * barrierSem = new Semaphore("barrierSem", 1);
#endif

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void SimpleThread(int which) {
int num, val;
for(num = 0; num < 5; num++) {

#ifdef HW1_SEMAPHORES
    //enter
sharedVar->P();
#endif

val = SharedVariable;
printf("*** thread %d sees value %d\n", which, val);
currentThread->Yield();
SharedVariable = val+1;

#ifdef HW1_SEMAPHORES
sharedVar->V();
    //exit
#endif

currentThread->Yield();
}
#ifdef HW1_SEMAPHORES
sharedVar->P(); // protect numThreadsActive
numThreadsActive--;
if (numThreadsActive == 0) {
    barrierSem->V(); // release the barrier
}
sharedVar->V();

barrierSem->P(); // wait on the barrier
barrierSem->V(); // release for others
#endif


val = SharedVariable;
printf("Thread %d sees final value %d\n", which, val);
}


//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------
#ifdef HW1_SEMAPHORES

int numThreadsActive; // used to implement barrier upon completion

void ThreadTest(int n){
    DEBUG('t', "Entering SimpleTest");
    numThreadsActive = n;
    printf("NumthreadsActive = %d\n", numThreadsActive);

    for (int i = 1; i <= n; i++) {
        Thread *t = new Thread("forked thread");
        t->Fork(SimpleThread, i);
    }
    SimpleThread(0);
}

#else

void
ThreadTest(int n)
{
    for (int i = 1; i <= n; i++) {
        Thread *t = new Thread("forked thread");
        t->Fork(SimpleThread, i);
    }
    SimpleThread(0);  
}

#endif
