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
#ifdef HW1_ELEVATOR
#include "elevator.h"
#endif

int testnum = 1;

void SimpleThread(int which)
{
    for (int num = 0; num < 5; num++) {
        printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

void ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");
    Thread *t = new Thread((char*)"forked thread");
    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

#ifdef HW1_LOCKS
void LockTest(); 
#endif

void ThreadTest(int n)
{
    switch (testnum) {
        case 1:
            ThreadTest1();
            break;
#ifdef HW1_LOCKS
        case 2:
            LockTest();
            break;
#endif
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
        Thread *t = new Thread((char*)"lock tester");
        t->Fork(LockTestThread, i);
    }
    LockTestThread(0);
}
#endif

#ifdef HW1_ELEVATOR

static const int E_CAPACITY = 5;
static const int MAX_FLOORS = 64;

static Lock *E_lock = new Lock((char*)"elev-lock");
static int   E_numFloors = 0;
static int   E_currentFloor = 1;
static bool  E_doorsOpen = false;
static int   E_onboard = 0;
static bool E_spawnerActive = false;

static Semaphore* floorArriveSem[MAX_FLOORS+1];
static Semaphore* spaceSem = new Semaphore((char*)"space", 0);

static int waitingAt[MAX_FLOORS+1];
static int wantOffAt[MAX_FLOORS+1];

static int  E_totalPeople = 0;

struct PersonArgs {
    int id;
    int atFloor;
    int toFloor;
};

static void InitIfNeeded() {
    if (E_numFloors == 0) {
        E_numFloors = MAX_FLOORS - 1;
    }
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

    floorArriveSem[at]->P();

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

  if (E_totalPeople > 0) {
    E_totalPeople--;
    
}
    E_lock->Release();

    delete p;
}

void ArrivingGoingFromTo(int atFloor, int toFloor) {
    InitIfNeeded();
    if (atFloor < 1) atFloor = 1;
    if (toFloor < 1) toFloor = 1;
    if (atFloor > E_numFloors) atFloor = E_numFloors;
    if (toFloor > E_numFloors) toFloor = E_numFloors;

    E_lock->Acquire();
    E_totalPeople++;
    E_lock->Release();

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
        if (next < 1)  { dir = +1; next = 2; }
        if (next > E_numFloors) { dir = -1; next = E_numFloors - 1; }
        E_currentFloor = next;
        E_lock->Release();

        for (int i = 0; i < 50; i++) currentThread->Yield();

        printf("Elevator arrives on floor %d.\n", E_currentFloor);

        E_lock->Acquire();
        E_doorsOpen = true;

        for (int i = 0; i < waitingAt[E_currentFloor]; i++)
            floorArriveSem[E_currentFloor]->V();

        for (int i = 0; i < wantOffAt[E_currentFloor]; i++)
            floorArriveSem[E_currentFloor]->V();

        for (int i = 0; i < E_CAPACITY; i++)
            spaceSem->V();

        E_lock->Release();

        for (int i = 0; i < 50; i++) currentThread->Yield();

        E_lock->Acquire();
        E_doorsOpen = false;

       int pending = E_onboard;
for (int f = 1; f <= E_numFloors; f++) {
    pending += waitingAt[f] + wantOffAt[f];
}
bool shouldStop = (!E_spawnerActive && pending == 0);

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
        int toFloor = -1;
        do { toFloor = (Random() % numFloors) + 1; } while (atFloor == toFloor);
        ArrivingGoingFromTo(atFloor, toFloor);
        for (int j = 0; j < 1000000; j++) { currentThread->Yield(); }
    }

E_spawnerActive = false;
}

#endif

void ThreadTest() {
#ifdef HW1_ELEVATOR
    ThreadTest(0);
#endif
}
