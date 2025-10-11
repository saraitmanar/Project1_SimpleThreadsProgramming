#include "copyright.h"
#include "system.h"
#include "synch.h"
#include "elevator.h"

// -------------------------------
// Elevator Data Structure
// -------------------------------
struct ElevatorThread {
    int numFloors;          // total number of floors
    int currentFloor;       // elevator's current floor
    int numPeopleIn;        // number of people currently in elevator (max 5)
    Lock *elevatorLock;     // protects shared state
    Condition *floorArrived;// used to signal when elevator arrives at a floor
    Condition *spaceAvailable;
    bool movingUp;          // direction flag
};

// Global elevator instance (since only one elevator)
static ElevatorThread *elevator = NULL;

// -------------------------------
// Elevator Thread Function
// -------------------------------
static void ElevatorThreadFunc(int dummy) {
    while (true) {
        elevator->elevatorLock->Acquire();

        // Update floor
        if (elevator->movingUp) {
            elevator->currentFloor++;
            if (elevator->currentFloor == elevator->numFloors) {
                elevator->movingUp = false;  // reached top, go down
            }
        } else {
            elevator->currentFloor--;
            if (elevator->currentFloor == 1) {
                elevator->movingUp = true;   // reached bottom, go up
            }
        }

        // Simulate travel delay (50 ticks)
        for (int t = 0; t < 50; t++) {
            currentThread->Yield();
        }

        printf("Elevator arrives on floor %d.\n", elevator->currentFloor);


        // Wake waiting people
        elevator->floorArrived->Broadcast(elevator->elevatorLock);
        elevator->elevatorLock->Release();
    }
}


// -------------------------------
// Elevator Initialization
// -------------------------------
void Elevator(int numFloors) {
    elevator = new ElevatorThread; // allocate first!

    elevator->numFloors = numFloors;
    elevator->currentFloor = 1;
    elevator->numPeopleIn = 0;
    elevator->elevatorLock = new Lock((char*)"ElevatorLock");
    elevator->floorArrived = new Condition((char*)"FloorArrived");
    elevator->spaceAvailable = new Condition((char*)"SpaceAvailable");
    elevator->movingUp = true;

    Thread *t = new Thread("ElevatorThread");
    t->Fork(ElevatorThreadFunc, 0);
}

// Called by a person to wait until the elevator reaches a specific floor
void WaitForElevatorAtFloor(int floor) {
    elevator->elevatorLock->Acquire();

    // Wait only if elevator not currently at this floor
    while (elevator->currentFloor != floor) {
        elevator->floorArrived->Wait(elevator->elevatorLock);
    }

    // When we reach here, elevator is at the requested floor
    elevator->elevatorLock->Release();
}


// Called by a person to attempt entering the elevator (respect capacity)
// Person tries to enter elevator; waits if full
bool TryEnterElevator() {
    elevator->elevatorLock->Acquire();

    while (elevator->numPeopleIn >= 5) {
        // Wait until someone leaves and signals spaceAvailable
        elevator->spaceAvailable->Wait(elevator->elevatorLock);
    }

    elevator->numPeopleIn++;
    elevator->elevatorLock->Release();
    return true;
}

// Called by a person when leaving at their destination
void LeaveElevator() {
    elevator->elevatorLock->Acquire();
    elevator->numPeopleIn--;
    elevator->spaceAvailable->Signal(elevator->elevatorLock);
    elevator->elevatorLock->Release();
}

