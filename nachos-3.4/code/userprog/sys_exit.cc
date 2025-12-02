#include "system.h"
#include "addrspace.h"
#include "memmanage.h"

// -------------------------------------------------------------
// do_Exit(status)
// Called when a user program invokes Exit(status).
// -------------------------------------------------------------
void do_Exit(int status)
{
    // Turn off interrupts while we tear down the process.
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    int pid = -1;
#ifdef USER_PROGRAM
    pid = currentThread->spaceId;  // use spaceId as the process id
#endif

    printf("System Call: [%d] invoked Exit.\n", pid);
    printf("Process [%d] exits with [%d]\n", pid, status);

    // Free this process's address space and physical pages.
    AddrSpace *space = currentThread->space;
    if (space != NULL) {
        unsigned int numPages = space->getNumPages();
        TranslationEntry *pt  = space->getPageTable();

        for (unsigned int i = 0; i < numPages; i++) {
            int phys = pt[i].physicalPage;
            memoryManager->clearPage(phys);
        }

        delete space;
        currentThread->space = NULL;
    }

    // Finish the current thread. This NEVER returns.
    currentThread->Finish();

    // If Finish() ever returned (it shouldn't), we would restore interrupts:
    // interrupt->SetLevel(oldLevel);
}
