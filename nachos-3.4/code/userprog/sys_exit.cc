#include "system.h"
//#include "ProcessManager.h"     // CHANGE THIS IF NEEDED
//#include "MemoryManager.h"      // CHANGE THIS IF NEEDED
//#include "pcb.h"                // CHANGE THIS IF NEEDED


void do_Exit(int status)
{
    int myPid = currentThread->space->pid; // CHANGE THIS IF NEEDED
   


    PCB *pcb = processManager->getPCB(myPid); // CHANGE THIS IF NEEDED
   


    if (pcb == NULL) {
        printf("ERROR: Exit(): PCB missing for pid %d\n", myPid);
        currentThread->Finish();
        return;
    }


    printf("Process %d exits with %d\n", myPid, status);


    pcb->waitLock->Acquire(); // CHANGE THIS IF NEEDED


    pcb->exited = true; // CHANGE THIS IF NEEDED


    pcb->exitStatus = status; // CHANGE THIS IF NEEDED


    // --------------------------------------------------------
    // 1. Remove process as parent from all children
    // --------------------------------------------------------
    for (ListElement *e = pcb->children.First(); e != NULL; e = e->next) {
        int childPid = (int)(intptr_t)e->item;


        PCB *child = processManager->getPCB(childPid);
        if (child != NULL)
            child->ppid = -1;  
            // CHANGE THIS IF NEEDED
    }


    pcb->children.MakeEmpty();
    // CHANGE THIS IF NEEDED


    // --------------------------------------------------------
    // 2. If parent waiting → wake them
    // --------------------------------------------------------
    if (pcb->ppid != -1) {
        PCB *parent = processManager->getPCB(pcb->ppid);


        if (parent != NULL) {
            parent->waitCond->Broadcast(parent->waitLock);
            // CHANGE THIS IF NEEDED
        }
    }


    // Wake Join() waiters
    pcb->waitCond->Broadcast(pcb->waitLock);  
    // CHANGE THIS IF NEEDED


    pcb->waitLock->Release();


    // --------------------------------------------------------
    // 3. FREE ALL PHYSICAL PAGES
    // --------------------------------------------------------
    AddrSpace *space = pcb->space;
    // CHANGE THIS IF NEEDED


    if (space != NULL) {
        for (unsigned i = 0; i < space->numPages; i++) {
            int phys = space->pageTable[i].physicalPage;
            memoryManager->clearPage(phys);   // CHANGE THIS IF NEEDED
        }
    }


    // --------------------------------------------------------
    // 4. Remove PID & PCB
    // --------------------------------------------------------
    processManager->clearPID(myPid);
    // CHANGE THIS IF NEEDED


    // --------------------------------------------------------
    // 5. End the kernel thread
    // --------------------------------------------------------
    currentThread->Finish();  
    
}
