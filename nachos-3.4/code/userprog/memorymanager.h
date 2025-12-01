#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include "bitmap.h"
#include "synch.h"

class MemoryManager {
public:
    MemoryManager(int numPages);   // constructor
    ~MemoryManager();

    int getPage();                 // returns free physical page index, or -1
    void clearPage(int pageID);    // frees a page

private:
    BitMap *bitmap;                // tracks free/used pages
    Lock *lock;                    // protects bitmap
};

#endif
