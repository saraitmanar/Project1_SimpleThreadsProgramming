#include "system.h"
#include "addrspace.h"
#include "memmanage.h"

// -------------------------------------------------------------
// do_Exit(status)
// Called when a user program invokes Exit(status).
// -------------------------------------------------------------
void do_Exit(int status)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    printf("System Call: [%d] invoked Exit.\n", currentThread->pid);
    printf("Process [%d] exits with [%d]\n", currentThread->pid, status);

    AddrSpace *space = currentThread->space;

    if (space != NULL)
    {
        unsigned int num = space->getNumPages();
        TranslationEntry *pt = space->getPageTable();

        for (unsigned int i = 0; i < num; i++) {
            int phys = pt[i].physicalPage;
            memoryManager->clearPage(phys);
        }

        delete space;
        currentThread->space = NULL;
    }

    currentThread->Finish();

    interrupt->SetLevel(oldLevel);
}
