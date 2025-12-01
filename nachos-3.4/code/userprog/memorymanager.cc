#include "memorymanager.h"
#include "system.h"

MemoryManager::MemoryManager(int numPages)
{
    bitmap = new BitMap(numPages);
    lock = new Lock("memoryManagerLock");
}

MemoryManager::~MemoryManager()
{
    delete bitmap;
    delete lock;
}

int MemoryManager::getPage()
{
    lock->Acquire();
    int page = bitmap->Find();   // returns first free bit index
    lock->Release();

    return page;   // -1 means no free pages
}

void MemoryManager::clearPage(int pageID)
{
    lock->Acquire();
    bitmap->Clear(pageID);
    lock->Release();
}
