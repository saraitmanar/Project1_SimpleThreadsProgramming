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

// threadtest.cc — HW1 tests & elevator

#include "copyright.h"
#include "system.h"
#include "synch.h"

int testnum = 1;   // used by stock nachos samples if needed

// -------------------------------
// Exercise 1: shared variable test
// -------------------------------
#ifdef HW1_THREADTEST

static int SharedValue = 0;

#ifdef HW1_SEMAPHORES
static Semaphore *mutex = NULL;  // created in ThreadTest()
#endif

void SimpleThread(int which) {
    for (int num = 0; num < 5; num++) {
#ifndef HW1_SEMAPHORES
        // UNSYNCHRONIZED version (should demonstrate races)
        int val = SharedValue;
        printf("*** thread %d sees value %d\n", which, val);
        currentThread->Yield();
        SharedValue = val + 1;
#else
        // SYNCHRONIZED version using a semaphore as a mutex
        mutex->P();
        int val = SharedValue;
        printf("*** thread %d sees value %d\n", which, val);
        SharedValue = val + 1;
        mutex->V();
#endif
    }

#ifndef HW1_SEMAPHORES
    printf("Thread %d sees final value %d\n", which, SharedValue);
#else
    mutex->P();
    printf("Thread %d sees final value %d\n", which, SharedValue);
    mutex->V();
#endif
}

static void LaunchThreads(int n) {
    if (n < 2) n = 2;  // at least two threads
    SharedValue = 0;

#ifdef HW1_SEMAPHORES
    if (mutex == NULL) mutex = new Semaphore((char*)"mutex", 1);
#endif

    for (int i = 1; i < n; i++) {
        Thread *t = new Thread((char*)"worker");
        t->Fork(SimpleThread, i);
    }
    SimpleThread(0);
}

#endif // HW1_THREADTEST

// -------------------------------
// Elevator (HW1_ELEVATOR)
// -------------------------------
#ifdef HW1_ELEVATOR

// limits
static const int E_CAPACITY   = 5;
static const int MAX_FLOORS   = 64;

// state
static Lock *E_lock           = new Lock((char*)"elev-lock");
static int   E_numFloors      = 0;
static int   E_currentFloor   = 1;
static bool  E_doorsOpen      = false;
static int   E_onboard        = 0;
static bool  E_spawnerActive  = false;

// signals
static Semaphore* floorArriveSem[MAX_FLOORS + 1];
static Semaphore* spaceSem     = new Semaphore((char*)"space", 0);

// counters
static int waitingAt[MAX_FLOORS + 1];
static int wantOffAt[MAX_FLOORS + 1];

struct PersonArgs {
    int id;
    int atFloor;
    int toFloor;
};

static void E_InitIfNeeded() {
    if (E_numFloors == 0) E_numFloors = 10;
    for (int f = 0; f <= E_numFloors; f++) {
        if (floorArriveSem[f] == 0) {
            floorArriveSem[f] = new Semaphore((char*)"arrive", 0);
            waitingAt[f] = 0;
            wantOffAt[f] = 0;
        }
    }
}

static void PersonMain(int arg) {
    PersonArgs* p = (PersonArgs*)arg;
    int id = p->id, at = p->atFloor, to = p->toFloor;

    printf("Person %d wants to go to floor %d from floor %d.\n", id, to, at);

    E_lock->Acquire();
    waitingAt[at]++;
    E_lock->Release();

    // wait for elevator to arrive at 'at'
    floorArriveSem[at]->P();

    // board when doors open and capacity allows
    E_lock->Acquire();
    while (!(E_doorsOpen && E_currentFloor == at && E_onboard < E_CAPACITY)) {
        E_lock->Release();
        spaceSem->P();
        E_lock->Acquire();
    }
    E_onboard++;
    waitingAt[at]--;
    wantOffAt[to]++;
    printf("Person %d got into the elevator.\n", id);
    E_lock->Release();

    // wait for 'to'
    floorArriveSem[to]->P();

    E_lock->Acquire();
    while (!(E_doorsOpen && E_currentFloor == to)) {
        E_lock->Release();
        floorArriveSem[to]->P();
        E_lock->Acquire();
    }

    E_onboard--;
    wantOffAt[to]--;
    printf("Person %d got out of the elevator.\n", id);
    spaceSem->V();
    E_lock->Release();

    delete p;
}

void ArrivingGoingFromTo(int atFloor, int toFloor) {
    E_InitIfNeeded();
    if (atFloor < 1) atFloor = 1;
    if (toFloor < 1) toFloor = 1;
    if (atFloor > E_numFloors) atFloor = E_numFloors;
    if (toFloor > E_numFloors) toFloor = E_numFloors;

    static int nextId = 0;
    PersonArgs* p = new PersonArgs;
    p->id = nextId++;
    p->atFloor = atFloor;
    p->toFloor = toFloor;

    Thread* t = new Thread((char*)"person");
    t->Fork(PersonMain, (int)p);
}

static void ElevatorMain(int) {
    int dir = +1;
    if (E_numFloors < 2) E_numFloors = 10;

    while (true) {
        E_lock->Acquire();
        int next = E_currentFloor + dir;
        if (next < 1)            { dir = +1; next = 2; }
        if (next > E_numFloors)  { dir = -1; next = E_numFloors - 1; }
        E_currentFloor = next;
        E_lock->Release();

        for (int i = 0; i < 50; i++) currentThread->Yield();

        printf("Elevator arrives on floor %d.\n", E_currentFloor);

        E_lock->Acquire();
        E_doorsOpen = true;

        // wake people waiting to get on/off here
        for (int i = 0; i < waitingAt[E_currentFloor]; i++) floorArriveSem[E_currentFloor]->V();
        for (int i = 0; i < wantOffAt[E_currentFloor]; i++) floorArriveSem[E_currentFloor]->V();

        // advertise space up to capacity
        for (int i = 0; i < E_CAPACITY; i++) spaceSem->V();

        E_lock->Release();

        for (int i = 0; i < 50; i++) currentThread->Yield();

        E_lock->Acquire();
        E_doorsOpen = false;

        bool anyPending = (E_onboard > 0);
        for (int f = 1; f <= E_numFloors && !anyPending; f++) {
            if (waitingAt[f] > 0 || wantOffAt[f] > 0) anyPending = true;
        }
        bool shouldStop = (!E_spawnerActive && !anyPending);
        E_lock->Release();

        if (shouldStop) {
            printf("Elevator shutting down.\n");
            break;
        }
    }
    currentThread->Finish();
}

void Elevator(int numFloors) {
    E_lock->Acquire();
    if (numFloors >= 2) E_numFloors = (numFloors <= MAX_FLOORS ? numFloors : MAX_FLOORS);
    for (int f = 0; f <= E_numFloors; f++) {
        if (floorArriveSem[f] == 0) {
            floorArriveSem[f] = new Semaphore((char*)"arrive", 0);
            waitingAt[f] = 0;
            wantOffAt[f] = 0;
        }
    }
    E_currentFloor = 1;
    E_doorsOpen = false;
    E_onboard = 0;
    E_lock->Release();

    Thread* t = new Thread((char*)"elevator");
    t->Fork(ElevatorMain, 0);
}

void ElevatorTest(int numFloors, int numPersons) {
    E_spawnerActive = true;
    Elevator(numFloors);

    for (int i = 0; i < numPersons; i++) {
        int atFloor = (Random() % numFloors) + 1;
        int toFloor;
        do { toFloor = (Random() % numFloors) + 1; } while (atFloor == toFloor);
        ArrivingGoingFromTo(atFloor, toFloor);
        for (int j = 0; j < 1000000; j++) currentThread->Yield();
    }

    E_spawnerActive = false;
}
#endif // HW1_ELEVATOR

// ---------------------------------------
// Unified entry called by main: ThreadTest
// ---------------------------------------
void ThreadTest(int n) {
#ifdef HW1_ELEVATOR
    if (n < 1) n = 3;
    ElevatorTest(10, n);

#elif defined(HW1_THREADTEST)
    LaunchThreads(n);

#else
    // fallback: do nothing here for other configurations
#endif
}
