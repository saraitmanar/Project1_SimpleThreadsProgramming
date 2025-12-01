#include "system.h"
//#include "ProcessManager.h"     // CHANGE THIS IF NEEDED
//#include "MemoryManager.h"      // CHANGE THIS IF NEEDED
//#include "pcb.h"                // CHANGE THIS IF NEEDED


void do_Exit(int status)
{
    int myPid = currentThread->space->pid;   // CHANGE THIS IF NEEDED


    PCB *pcb = processManager->getPCB(myPid);  // CHANGE THIS IF NEEDED
    if (pcb == NULL) {
        printf("ERROR: Exit(): no PCB for pid %d\n", myPid);
        currentThread->Finish();
        return;
    }


    pcb->exited = true;        // CHANGE THIS IF NEEDED
    pcb->exitStatus = status;


    pcb->waitLock->Acquire();  // CHANGE THIS IF NEEDED


    // --------------------------------------------------------
    // 1. Handle children
    // --------------------------------------------------------
    for (ListElement *e = pcb->children.First(); e != NULL; e = e->next) {
        int childPid = (int)(intptr_t)e->item;


        PCB *child = processManager->getPCB(childPid);
        if (child != NULL)
            child->ppid = -1;
    }
    pcb->children.MakeEmpty();


    // --------------------------------------------------------
    // 2. Inform parent (if any)
    // --------------------------------------------------------
    if (pcb->ppid != -1) {
        PCB *parent = processManager->getPCB(pcb->ppid);


        if (parent != NULL) {
            parent->childExitValue = status;   // CHANGE THIS IF NEEDED
            parent->waitCond->Broadcast(parent->waitLock);
        }
    }


    pcb->waitCond->Broadcast(pcb->waitLock);
    pcb->waitLock->Release();


    // --------------------------------------------------------
    // 3. Free address space
    // --------------------------------------------------------
    AddrSpace *space = pcb->space;


    if (space != NULL) {
        for (unsigned i = 0; i < space->numPages; i++) {
            int phys = space->pageTable[i].physicalPage;
            memoryManager->clearPage(phys);  // CHANGE THIS IF NEEDED
        }
        delete space;
    }


    pcb->space = NULL;


    // --------------------------------------------------------
    // 4. Remove PID / PCB
    // --------------------------------------------------------
    processManager->clearPID(myPid);   // CHANGE THIS IF NEEDED


    // --------------------------------------------------------
    // 5. Finish thread (never returns)
    // --------------------------------------------------------
    currentThread->Finish();
}
