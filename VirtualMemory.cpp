#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#include <stdio.h>

struct PageTable
{
    uint64_t baseAddr;
    uint64_t length;
};


bool isFrameEmpty(uint64_t frameIndex)
{
    word_t  value;
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMread(frameIndex * PAGE_SIZE + i, &value);
        if (value != 0)
        {
            return false;
        }
    }
    return true;
}

word_t findNextFrameIndex()
{
    for (auto frame = 1; frame < NUM_FRAMES; ++frame)
    {
        if(isFrameEmpty(frame))
        {
            return frame;
        }
    }
    // TODO need to find a page to evict
    // finding the deepest frame index and page index of this frame (virtual memory address)
    word_t index = 1; //just numbers to try for now
    uint64_t pageIndex = 5;
    PMevict(index, pageIndex);
    return index;

}

uint64_t findPageNumber(uint64_t address, uint64_t depth)
{
    uint64_t pageNumber = address >> ((TABLES_DEPTH - depth) * OFFSET_WIDTH);
    return pageNumber;
}

word_t findAddress(uint64_t virtualAddr)
{
    word_t tempWord = 0;
    uint64_t tempAddress = findPageNumber(virtualAddr, 1);
    PMread(0 + tempAddress, &tempWord);
   for (uint64_t depth = 2; depth < TABLES_DEPTH; depth++)
   {
       tempAddress = findPageNumber(tempAddress, depth);
       if (tempAddress == 0)
       {
//           TODO evict n'stuff
       }
       PMread(tempWord*PAGE_SIZE + tempAddress, &tempWord);
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