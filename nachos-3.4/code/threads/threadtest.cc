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
int SharedVariable = 0;

#ifdef HW1_SEMAPHORES
Semaphore *sharedVarSemaphore = NULL;
Semaphore *barrierSemaphore = NULL;
int totalThreads =0;
int threadsCompleted = 0;
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

#ifdef HW1_SEMAPHORES
    if (which == 0 && sharedVarSemaphore == NULL){
        sharedVarSemaphore = new Semaphore("SharedVarSemaphore", 1);
        barrierSemaphore = new Semaphore("BarrierSemaphore", 0);
        totalThreads = testnum + 1;  // +1 for the main thread (thread 0)
    }
#endif

    for(num = 0; num < 5; num++) {
        val = SharedVariable;

#ifdef HW1_SEMAPHORES
        sharedVarSemaphore->P();
#endif

        printf("*** thread %d sees value %d\n", which, val);
        SharedVariable = val+1;

#ifdef HW1_SEMAPHORES
        sharedVarSemaphore->V();
#endif
        currentThread->Yield();
    }

    val = SharedVariable;
    printf("Thread %d sees final value %d\n", which, val);

#ifdef HW1_SEMAPHORES
    sharedVarSemaphore->P();
    threadsCompleted++;

    if(threadsCompleted == totalThreads) {
        for (int i = 0; i < totalThreads - 1; i++){
        barrierSemaphore-V();
    }
    threadsCompleted = 0;
    } else {
        sharedVarSemaphore->V();
        barrierSemaphore-P();
    }
#endif
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

void
ThreadTest(int n)
{
#ifdef HW1_SEMAPHORES
    sharedVairable = 0;
    threadsCompleted =0;
    totalThreads = n + 1;

    if(sharedVarSemaphore !=NULL){
        delete sharedVarSemaphore;
        sharedVarSemaphore = NULL;
    }
    if (barrierSemaphore != NULL){
        delete barrierSemaphore;
        barrierSemaphore = NULL;
    }
#endif

    for (int i = 1; i <= n; i++) {
        Thread *t = new Thread("forked thread");
        t->Fork(SimpleThread, i);
    }
    SimpleThread(0);  
}

