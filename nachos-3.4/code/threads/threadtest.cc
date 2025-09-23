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

//test for Lock 
// void LockTest() {
//     printf("Starting LockTest...\n");
//     Lock *myLock = new Lock("test lock");
//     printf("Created lock: %s\n", myLock->getName());
//     delete myLock;
//     printf("Deleted lock\n");
// }

void SimpleThread(int which) {
int num, val;
for(num = 0; num < 5; num++) {

#ifdef HW1_LOCKS
    //enter
testLock->Acquire();
#endif

val = SharedVariable;
printf("*** thread %d sees value %d\n", which, val);
currentThread->Yield();
SharedVariable = val+1;

#ifdef HW1_LOCKS
testLock->Release();
    //exit
#endif

currentThread->Yield();
}
#ifdef HW1_LOCKS
testLock->Acquire(); // protect numThreadsActive
numThreadsActive--;
if (numThreadsActive == 0) {
    barrierSem->V(); // release the barrier
}
testLock->Release();

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
#ifdef HW1_LOCKS
void LockTest();   // tells compiler it exists
#endif

void
ThreadTest(int n)
{
    switch (testnum) {
    case 1:
	ThreadTest1();
	break;
    case 2:
    LockTest();
    break;
    default:
	printf("No test specified.\n");
	break;
    }
}

#ifdef HW1_LOCKS
static Lock *testLock = new Lock((char*)"testLock");

void LockTestThread(int which) {
    printf("Thread %d: trying to acquire lock\n", which);
    testLock->Acquire();
    printf("Thread %d: inside critical section\n", which);

    for (int i = 0; i < 3; i++) currentThread->Yield();

    printf("Thread %d: releasing lock\n", which);
    testLock->Release();
}

void LockTest() {
    for (int i = 1; i <= 3; i++) {
        Thread *t = new Thread("lock tester");
        t->Fork(LockTestThread, i);
    }
    LockTestThread(0);  // main thread also tries
}
#endif


void ThreadTest() {
    ThreadTest(0);   // n isn’t used for the lock test anyway
}
