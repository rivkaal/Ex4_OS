#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#include <stdio.h>

#define OFFSET_MAX 15

void clearTable(uint64_t frameIndex);

struct PageTable
{
    uint64_t baseAddr;
    uint64_t length;
};

uint64_t findPageNumber(uint64_t address, uint64_t depth)
{
    uint64_t pageNumber = address >> (TABLES_DEPTH - depth); //TODO verify calculations
    return pageNumber;
}


word_t findAddress(uint64_t virtualAddr)
{
    word_t tempWord = 0;
    uint64_t victim;
    uint64_t tempAddress = findPageNumber(virtualAddr, 1);
    PMread(0 + tempAddress, &tempWord);

    if (tempWord == 0)
    {
        victim = findNextFrame();
        clearTable(victim);
        PMwrite(0 + tempAddress, victim);
        tempWord = victim;
    }

   for (uint64_t depth = 2; depth < TABLES_DEPTH; depth++)
   {
       tempAddress = findPageNumber(tempAddress, depth);
       PMread(tempWord*PAGE_SIZE + tempAddress, &tempWord);
       if (tempWord == 0)
       {
            victim = findNextFrame();
           clearTable(victim);
           if (tempAddress <= 16)
           {
               PMrestore(victim, virtualAddr);
           }
           PMwrite(tempWord*PAGE_SIZE + tempAddress, victim);
//           TODO evict n'stuff
       }

   }
   return tempWord;
}

void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

void VMinitialize() {
    clearTable(0);
}


int VMread(uint64_t virtualAddress, word_t* value) {
    word_t physicalAddr = findAddress(virtualAddress);
    PMread(physicalAddr, value);
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    word_t physicalAddr = findAddress(virtualAddress);
    PMwrite(physicalAddr, value);
    return 1;
}

int main(int argc, char **argv)
{
    uint64_t a,b,c;
    word_t x,y,z;
    a= 20;
    x = 1355;
    z=3;
    VMwrite(a, 1355);
    VMread(a, &y);
    printf("%u\n", y);
    return 0;
}