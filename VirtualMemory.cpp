#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#include <cstdio>
#include <cstdlib>
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
    word_t toEvict;
    word_t maxFrameIndex;
    word_t pathToDeepest;
    uint64_t pathToEvict;
    word_t evictParent;
};

void debugPrintPM(char *bpMessage = (char *) (""))
{
    printf ("--------- %s ------------\n", bpMessage);
    word_t a = 0;
    for (int i = 0; i < RAM_SIZE; i++)
    {
        PMread(i, &a);
        printf("%d\t", a);
    }
    printf("\n---------------------------------------\n");
}

void clearTable(uint64_t frameIndex);
uint64_t findPageNumber(uint64_t address, uint64_t depth);

/**
 * finding cyclic distance
 * @param from place
 * @param to another place
 * @return
 */
word_t findCyclicDistance(int from, int to)
{
    if (abs(from - to) < NUM_PAGES - abs(from - to))
        return abs(from - to);
    return NUM_PAGES - abs(from - to);
}


/**
 * checking if frame is empty
 * @param frameIndex
 * @return true if it's empty
 */
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

/**
 * recursive dfs function that going through all frames and returns a lot of answers
 * @param parent num of frame that we need to find a baby frame
 * @param curFrame num of cur frame
 * @param depth current depth
 * @param pathToCurrent how to get to the cur frame
 * @param virtualAddress have no idea what the fuck it is
 * @param curParent the previous frame
 * @return
 */
DFSResult DFSRecursive(word_t parent, word_t curFrame, word_t depth, word_t pathToCurrent, uint64_t virtualAddress, uint64_t curParent)
{
    DFSResult result({false,0,0,0, 0,
                      0, curParent,0});

    word_t tempWord;
    if (depth > TABLES_DEPTH)       //debug
    {
        result.maxDepth = depth ;
        result.maxFrameIndex = curFrame;
        result.deepestFrameIndex = curFrame;
        result.pathToDeepest = pathToCurrent;
        result.toEvict = curFrame;
        result.evictParent = curParent;
        result.pathToEvict = pathToCurrent;
        return result;
    }


    //base case 1

    if (isFrameEmpty(curFrame))
    {
        if (curFrame == parent)
        {
            result.maxFrameIndex = curFrame;
            return result;
        }
        result.empty = true;
        result.maxDepth = depth;
        result.maxFrameIndex = curFrame;
        result.deepestFrameIndex = curFrame;
        result.pathToDeepest = pathToCurrent % PAGE_SIZE;
        result.toEvict = curFrame;
        result.pathToEvict = pathToCurrent;
        result.evictParent = curParent;
        return result;
    }

    for (word_t frame = 0; frame < PAGE_SIZE; frame++)
    {
        uint64_t pathToNext = ((pathToCurrent << OFFSET_WIDTH) | (frame));
        PMread(curFrame*PAGE_SIZE + frame, &tempWord);
        if (tempWord != 0)
        {

            DFSResult curResult = DFSRecursive(parent, tempWord, depth + 1, pathToNext, virtualAddress, curFrame);
            if (curResult.empty && !result.empty)
            {
                result.maxFrameIndex = curResult.maxFrameIndex;
                result.empty = true;
                result.deepestFrameIndex = curResult.deepestFrameIndex;
                result.pathToDeepest = curResult.pathToDeepest;
                result.maxDepth = curResult.maxDepth;
                result.toEvict = curResult.toEvict;
                result.evictParent = curResult.evictParent;
                result.pathToEvict =curResult.pathToEvict;
            }
            else if ((curResult.empty && result.empty) || (!curResult.empty && !result.empty))
            {
                if (curResult.maxFrameIndex > result.maxFrameIndex)
                {
                    result.maxFrameIndex = curResult.maxFrameIndex;

                }
                if (findCyclicDistance(virtualAddress, curResult.pathToEvict) >=
                    findCyclicDistance(virtualAddress, result.pathToEvict))
                {
                    result.toEvict = curResult.toEvict;
                    result.evictParent = curResult.evictParent;
                    result.pathToEvict = curResult.pathToEvict;
                }

                if (curResult.maxDepth > result.maxDepth)
                {
                    result.maxDepth = curResult.maxDepth;
                    result.deepestFrameIndex = curResult.deepestFrameIndex;
                    result.pathToDeepest = pathToNext;
                    result.pathToEvict = curResult.pathToEvict;
                    result.toEvict = curResult.toEvict;
                }
            }
        }
    }

    return result;
}



word_t findNextFrameIndex(word_t parent, uint64_t VAddr)
{
    DFSResult result = DFSRecursive(parent, 0, 1, 0, VAddr, 0);
    uint64_t targetFrame;
    if (result.empty)
    {
        targetFrame = (result.pathToDeepest);
        PMwrite(result.evictParent*PAGE_SIZE + targetFrame, 0);
        return result.deepestFrameIndex;
    }
    if (result.maxFrameIndex + 1 < NUM_FRAMES)
    {
        return result.maxFrameIndex + 1;
    }

    debugPrintPM();
    printf("lulululullululu %lu %lu \n", result.toEvict, result.pathToEvict);
    PMevict(result.toEvict, result.pathToEvict);
    debugPrintPM();

    targetFrame = (result.pathToEvict & (PAGE_SIZE-1));
    PMwrite(result.evictParent*PAGE_SIZE + targetFrame, 0);
    return result.toEvict;
}

uint64_t findPath(uint64_t VAddress)
{
    return VAddress >> OFFSET_WIDTH;
}


uint64_t findPageNumber(uint64_t address, uint64_t depth)
{
    uint64_t pageNumber = (address >> ((TABLES_DEPTH - (depth)) * OFFSET_WIDTH)) & (PAGE_SIZE - 1LL);

    return pageNumber;
}


word_t findAddress(uint64_t virtualAddr)
{
    word_t tempWord = 0;
    word_t victim;
    word_t lastPage = 0;
    uint64_t curPageNumber;
   for (uint64_t depth = 0; depth < TABLES_DEPTH; depth++)
   {
       curPageNumber = findPageNumber(virtualAddr, depth);
       PMread(tempWord*PAGE_SIZE + curPageNumber, &tempWord);
       if (tempWord == 0)
       {
           victim = findNextFrameIndex(lastPage, findPath(virtualAddr));
           clearTable(victim);
           if (depth == TABLES_DEPTH - 1)     // only if actual page
           {
               PMrestore(victim, virtualAddr/ PAGE_SIZE);
           }
           PMwrite(lastPage*PAGE_SIZE + curPageNumber, victim);
           tempWord = victim;

       }
       lastPage = word_t(tempWord);
   }
   uint64_t  outAddress = tempWord * PAGE_SIZE + findPageNumber(virtualAddr, TABLES_DEPTH);

   return outAddress;
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
    if (virtualAddress >= VIRTUAL_MEMORY_SIZE)
    {
        return 0;
    }
    word_t physicalAddr = findAddress(virtualAddress);
    PMread(physicalAddr, value);
    return 1;
}


int VMwrite(uint64_t virtualAddress, word_t value) {
    if (virtualAddress >= VIRTUAL_MEMORY_SIZE)
    {
        return 0;
    }
    word_t physicalAddr = findAddress(virtualAddress);
    PMwrite(physicalAddr, value);
    return 1;
}
