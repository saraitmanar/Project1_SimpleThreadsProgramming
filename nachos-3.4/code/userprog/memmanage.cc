#include "memmanage.h"
#include "system.h"

MemoryManager::MemoryManager(int totalPages) {
    numPages = totalPages;
    bitmap = new bool[numPages];

    for (int i = 0; i < numPages; i++) {
        bitmap[i] = false; // all pages initially free
    }
}

int MemoryManager::getPage() {
    for (int i = 0; i < numPages; i++) {
        if (!bitmap[i]) {
            bitmap[i] = true;    // mark allocated
            return i;
        }
    }
    return -1; // no free pages available
}

void MemoryManager::clearPage(int page) {
    if (page >= 0 && page < numPages) {
        bitmap[page] = false;    // mark free
    }
}
