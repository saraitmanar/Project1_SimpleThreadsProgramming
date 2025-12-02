// addrspace.cc 
//  Routines to manage address spaces (executing user programs).
//
//  In order to run a user program, you must:
//
//  1. link with the -N -T 0 option 
//  2. run coff2noff to convert the object file to Nachos format
//      (Nachos object code format is essentially just a simpler
//      version of the UNIX executable object code format)
//  3. load the NOFF file into the Nachos file system
//      (if you haven't implemented the file system yet, you
//      don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include "memmanage.h"     // for MemoryManager

#ifdef HOST_SPARC
#include <strings.h>
#endif

extern MemoryManager *memoryManager;

//----------------------------------------------------------------------
// SwapHeader
//  Do little endian to big endian conversion on the bytes in the 
//  object file header, in case the file was generated on a little
//  endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
    noffH->noffMagic = WordToHost(noffH->noffMagic);
    noffH->code.size = WordToHost(noffH->code.size);
    noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
    noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
    noffH->initData.size = WordToHost(noffH->initData.size);
    noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
    noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
    noffH->uninitData.size = WordToHost(noffH->uninitData.size);
    noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
    noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// Local helpers for virtual → physical translation and file loading
//----------------------------------------------------------------------

// Translate a virtual address (vaddr) into a physical address (paddr)
// using this address space's page table.
static bool
TranslateAddr(TranslationEntry *pageTable, unsigned int numPages,
              int vaddr, int *paddr)
{
    int vpn = vaddr / PageSize;
    int offset = vaddr % PageSize;

    if (vpn < 0 || (unsigned)vpn >= numPages) {
        return FALSE;
    }

    int ppn = pageTable[vpn].physicalPage;
    *paddr = ppn * PageSize + offset;
    return TRUE;
}

// Read "size" bytes from "file" at file offset "fileAddr" into virtual
// memory starting at "virtAddr", using the given page table.
static void
ReadAtVirtual(OpenFile *file, int virtAddr, int size, int fileAddr,
              TranslationEntry *pageTable, unsigned int numPages)
{
    if (size <= 0) return;

    char *buffer = new char[size];
    file->ReadAt(buffer, size, fileAddr);

    for (int i = 0; i < size; i++) {
        int vaddr = virtAddr + i;
        int paddr;
        bool ok = TranslateAddr(pageTable, numPages, vaddr, &paddr);
        ASSERT(ok);
        machine->mainMemory[paddr] = buffer[i];
    }

    delete [] buffer;
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
//  Create an address space to run a user program.
//  Load the program from a file "executable", and set everything
//  up so that we can start executing user instructions.
//
//  Assumes that the object code file is in NOFF format.
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
        (WordToHost(noffH.noffMagic) == NOFFMAGIC))
        SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

    // how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
           + UserStackSize;   // leave room for the stack

    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= (unsigned)NumPhysPages); // until we have VM

    // Required print (Project 2 PDF):
    // Loaded Program: [x] code | [y] data | [z] bss
    printf("Loaded Program: [%d] code | [%d] data | [%d] bss\n",
           noffH.code.size, noffH.initData.size, noffH.uninitData.size);

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
          numPages, size);

    // first, set up the translation 
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
        int physPage = memoryManager->getPage();
        ASSERT(physPage != -1);          // out of memory?

        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = physPage;
        pageTable[i].valid = TRUE;
        pageTable[i].use = FALSE;
        pageTable[i].dirty = FALSE;
        pageTable[i].readOnly = FALSE;   // we could mark code as read-only
    }
    
    // zero out the entire address space *owned by this process*,
    // not the whole mainMemory
    for (i = 0; i < numPages; i++) {
        int physPage = pageTable[i].physicalPage;
        bzero(&(machine->mainMemory[physPage * PageSize]), PageSize);
    }

    // then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", 
              noffH.code.virtualAddr, noffH.code.size);

        ReadAtVirtual(executable,
                      noffH.code.virtualAddr,
                      noffH.code.size,
                      noffH.code.inFileAddr,
                      pageTable,
                      numPages);
    }

    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", 
              noffH.initData.virtualAddr, noffH.initData.size);

        ReadAtVirtual(executable,
                      noffH.initData.virtualAddr,
                      noffH.initData.size,
                      noffH.initData.inFileAddr,
                      pageTable,
                      numPages);
    }
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
//  Deallocate an address space.
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
    // Free physical pages back to the memory manager
    for (unsigned int i = 0; i < numPages; i++) {
        memoryManager->clearPage(pageTable[i].physicalPage);
    }

    delete [] pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
//  Set the initial values for the user-level register set.
//
//  We write these directly into the "machine" registers, so
//  that we can immediately jump to user code.  Note that these
//  will be saved/restored into the currentThread->userRegisters
//  when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
        machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);    

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

    // Set the stack register to the end of the address space, where we
    // allocated the stack; but subtract off a bit, to make sure we don't
    // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n",
          numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
//  On a context switch, save any machine state, specific
//  to this address space, that needs saving.
//
//  For now, nothing.
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
//  On a context switch, restore the machine state so that
//  this address space can run.
//
//  For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}