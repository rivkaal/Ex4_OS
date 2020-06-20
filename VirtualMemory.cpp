#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#include <cstdio>
#include <cassert>
struct Frame
{
    word_t frameIndex;
    word_t pageIndex;
};

struct DFSResult
{
    bool empty;
    word_t maxDepth;
    word_t deepestFrameIndex;
    word_t deepestPageIndex;
    word_t maxFrameIndex;
    word_t pathToDeepest;
};

void clearTable(uint64_t frameIndex);
uint64_t findPageNumber(uint64_t address, uint64_t depth);

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

DFSResult DFSRecursive(word_t parent, word_t curFrame, word_t depth, word_t pathToCurrent)
{
    DFSResult result({false,0,0,0, 0,
                      0});

    if (curFrame == parent)
    {
        return result;
    }

    word_t tempWord;
    //base case 1
    if (isFrameEmpty(curFrame))
    {
        result.empty = true;
        result.maxDepth = depth;
        result.maxFrameIndex = curFrame;
        result.deepestFrameIndex = curFrame;
        return result;
    }
    //base case 2
    if (depth >= TABLES_DEPTH)
    {
        result.maxDepth = depth;
        result.maxFrameIndex = curFrame;
        result.deepestFrameIndex = curFrame;
        result.pathToDeepest = pathToCurrent;
        return result;
    }

    for (word_t frame = 0; frame < PAGE_SIZE; frame++)
    {
        uint64_t pathToNext = ((pathToCurrent << OFFSET_WIDTH) | (frame));
        PMread(curFrame*PAGE_SIZE + frame, &tempWord);
        if (tempWord != 0)
        {
            DFSResult curResult = DFSRecursive(parent, tempWord, depth + 1, pathToNext);
            if (curResult.empty && !result.empty)
            {
                result.maxFrameIndex = curResult.maxFrameIndex;
                result.empty = true;
                result.deepestFrameIndex = curResult.deepestFrameIndex;
                result.maxDepth = curResult.maxDepth;
            }
            else if ((curResult.empty && result.empty) || (!curResult.empty && !result.empty))
            {
                if (curResult.maxFrameIndex > result.maxFrameIndex)
                {
                    result.maxFrameIndex = curResult.maxFrameIndex;
                }

                if (curResult.maxDepth > result.maxDepth)
                {
                    result.maxDepth = curResult.maxDepth;
                    result.deepestFrameIndex = curResult.deepestFrameIndex;
                    result.pathToDeepest = pathToNext;
                }
            }
        }
    }
    return result;
}

//uint64_t findLeaf(uint64_t VAddress)
//{
//    uint64_t curFrame = 0;
//    uint64_t curPageNum =
//    for (uint64_t i = 1; i < TABLES_DEPTH; i++)
//    {
//        if (tempWord == 0)
//        {
//            victim = findNextFrameIndex();
//            clearTable(victim);         // findNextFrameIndex returns word_t as it should, but clearTable accepts uint64_t so maybe something is missing
//            PMwrite(0 + curPageNumber, victim);
//            tempWord = victim;
//        }
//        lastPage = word_t(tempWord);
//    }
//}

word_t findCyclicDistance(uint64_t from, uint64_t to)
{
    if (from - to > NUM_PAGES - (from - to))
        return from - to;
    return NUM_PAGES - (from - to);
}

word_t findNextFrameIndex(word_t parent)
{
    DFSResult result = DFSRecursive(parent, 0, 1, 0 );
    if (result.empty)
    {
        return result.deepestFrameIndex;
    }
    if (result.maxFrameIndex + 1 < NUM_FRAMES)
    {
        return result.maxFrameIndex + 1;
    }

    PMevict(result.deepestFrameIndex, result.pathToDeepest);

//    for (auto frame = 1; frame < NUM_FRAMES; ++frame)
//    {
//        if(isFrameEmpty(frame) && frame != parent)
//        {
//            return frame;
//        }
//    }

    // TODO need to find a page to evict
    // finding the deepest frame index and page index of this frame (virtual memory address)
    return result.deepestFrameIndex;
}

uint64_t findPath(uint64_t VAddress)
{
    return VAddress >> OFFSET_WIDTH;
}

uint64_t findPageNumber(uint64_t address, uint64_t depth)
{
    //uint64_t pageNumber = address >> (TABLES_DEPTH - depth); //TODO verify calculations


    uint64_t pageNumber = (address >> ((TABLES_DEPTH - (depth + 1)) * OFFSET_WIDTH));
    if (depth > 1)
    {
        uint64_t offsetMask = 0;
        for (uint64_t i = 0; i < OFFSET_WIDTH; i++)
        {
            offsetMask += 1LL << i;
        }
        pageNumber = pageNumber & (offsetMask);
    }
    return pageNumber;
}


word_t findAddress(uint64_t virtualAddr)
{
    word_t tempWord = 0;
    word_t victim;
    word_t lastPage = 0;

    uint64_t curPageNumber = findPageNumber(virtualAddr, 1);      // Top level table
    uint64_t virtualPath = findPath(virtualAddr);
            PMread(0 + curPageNumber, &tempWord);
    if (tempWord == 0)
    {
        victim = findNextFrameIndex(virtualPath);
        clearTable(victim);         // findNextFrameIndex returns word_t as it should, but clearTable accepts uint64_t so maybe something is missing
        PMwrite(0 + curPageNumber, victim);
        tempWord = victim;
    }
    lastPage = word_t(tempWord);

   for (uint64_t depth = 2; depth < TABLES_DEPTH; depth++)
   {
       curPageNumber = findPageNumber(virtualAddr, depth);
       PMread(tempWord*PAGE_SIZE + curPageNumber, &tempWord);
       if (tempWord == 0)
       {
           victim = findNextFrameIndex(virtualPath);
           if (victim == lastPage)
           {
               //TODO pick second last
           }
           //           TODO evict n'stuff
           clearTable(victim);
           if (curPageNumber < 16)     // only if actual page
           {
               PMrestore(victim, virtualAddr);
           }
           PMwrite(tempWord*PAGE_SIZE + curPageNumber, victim);
           tempWord = victim;

       }
       lastPage = word_t(tempWord);
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

int main(int argc, char **argv) {
    VMinitialize();
    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
        printf("writing to %llu\n", (long long int) i);
        VMwrite(5 * i * PAGE_SIZE, i);
    }

    for (uint64_t i = 0; i < (2 * NUM_FRAMES); ++i) {
        word_t value;
        VMread(5 * i * PAGE_SIZE, &value);
        printf("reading from %llu %d\n", (long long int) i, value);
        assert(uint64_t(value) == i);
    }
    printf("success\n");

    return 0;
}