#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#include <stdio.h>

struct Frame
{
    word_t frameIndex;
    word_t pageIndex;
};

void clearTable(uint64_t frameIndex);

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


Frame findFrameToEvict()
{
    int maxDepth = 0;
    int curDepth = 0;
    Frame farestFrame = Frame();
    farestFrame.frameIndex = 0;
    farestFrame.pageIndex = 0;

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
    //uint64_t pageNumber = address >> (TABLES_DEPTH - depth); //TODO verify calculations

    uint64_t offsetMask = 0;
    for (uint64_t i = 0; i < OFFSET_WIDTH; i++)
    {
        offsetMask += 1LL << i;
    }
    uint64_t pageNumber = (address >> ((TABLES_DEPTH - (depth + 1)) * OFFSET_WIDTH));
    pageNumber = pageNumber & (offsetMask);
    return pageNumber;
}


word_t findAddress(uint64_t virtualAddr)
{
    word_t tempWord = 0;
    word_t victim;
    uint64_t curPageNumber = findPageNumber(virtualAddr, 1);      // Top level table
    PMread(0 + curPageNumber, &tempWord);

    if (tempWord == 0)
    {
        victim = findNextFrameIndex();
        clearTable(victim);         // findNextFrameIndex returns word_t as it should, but clearTable accepts uint64_t so maybe something is missing
        PMwrite(0 + curPageNumber, victim);
        tempWord = victim;
    }

   for (uint64_t depth = 2; depth < TABLES_DEPTH; depth++)
   {
       curPageNumber = findPageNumber(virtualAddr, depth);
       PMread(tempWord*PAGE_SIZE + curPageNumber, &tempWord);
       if (tempWord == 0)
       {
           victim = findNextFrameIndex();
           clearTable(victim);
           if (curPageNumber < 16)     //
           {
               PMrestore(victim, virtualAddr);
           }
           PMwrite(tempWord*PAGE_SIZE + curPageNumber, victim);
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
    a = 618;
    b = findPageNumber(a, 3);
    printf("%lu\n",b);
    return 0;
}