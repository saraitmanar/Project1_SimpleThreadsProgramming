#include "system.h"
#include "addrspace.h"
#include "memmanage.h"

void do_Kill(int pid) {
    if (pid <= 0 || pid >= 128) {
        printf("Kill: invalid pid %d\n", pid);
        return;
    }

    Thread* target = processTable[pid];

    if (target == NULL) {
        printf("Kill: pid %d does not exist or already exited\n", pid);
        return;
    }

    printf("System Call: [%d] invoked Kill on [%d]\n",
            currentThread->pid, pid);

    AddrSpace* space = target->space;
    if (space != NULL) {
        for (unsigned int i = 0; i < space->numPages; i++) {
            int phys = space->pageTable[i].physicalPage;
            memoryManager->clearPage(phys);
        }
        delete space;
        target->space = NULL;
    }

    processTable[pid] = NULL;

    if (target == currentThread) {
        // Self-kill == Exit
        currentThread->Finish();
    } else {
        scheduler->ReadyToRun(target); // ensure safe transition
        target->Finish();
    }
}
