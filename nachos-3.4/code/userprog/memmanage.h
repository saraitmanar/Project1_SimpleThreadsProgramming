#ifndef MEMMANAGE_H
#define MEMMANAGE_H

#include "bitmap.h"
#include "synch.h"

class MemoryManager {
private:
    bool *bitmap;      // bitmap[i] = whether phys page i is allocated
    int numPages;

public:
    MemoryManager(int totalPages);

    int getPage();     // returns a free physical page index
    void clearPage(int page);  // marks the physical page free again
};

#endif
