#include "system.h"
#include "synch.h"

#ifdef HW1_LOCKS

static Lock *testLock = new Lock((char*)"testLock");

static void LockTestThread(int which) {
    printf("Thread %d: trying to acquire lock\n", which);
    testLock->Acquire();
    printf("Thread %d: inside critical section\n", which);
    for (int i = 0; i < 3; i++) currentThread->Yield();
    printf("Thread %d: releasing lock\n", which);
    testLock->Release();
}

void LockTest() {
    for (int i = 1; i <= 3; i++) {
        Thread *t = new Thread((char*)"lock tester");
        t->Fork(LockTestThread, i);
    }
    LockTestThread(0);
}

#endif 
