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
    word_t toEvict;
    word_t maxFrameIndex;
    word_t pathToDeepest;
    word_t pathToEvict;
    word_t evictParent;
};

void debugPrintPM()
{
    printf ("--------- physical memory ------------\n");
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

word_t findCyclicDistance(uint64_t from, uint64_t to)
{
    if (from - to < NUM_PAGES - (from - to))
        return from - to;
    return NUM_PAGES - (from - to);
}



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

DFSResult DFSRecursive(word_t parent, word_t curFrame, word_t depth, word_t pathToCurrent, uint64_t virtualAddress, uint64_t curParent)
{
    DFSResult result({false,0,0,0, 0,
                      0, parent,0});

//    if (curFrame == parent)
//    {
//        result.maxFrameIndex = parent;
//        return result;
//    }

    word_t tempWord;
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
        result.toEvict = curFrame;
        result.pathToEvict = pathToCurrent;
        result.evictParent = curParent;
        return result;
    }
    //base case 2
//    if (depth >= TABLES_DEPTH)
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
                result.maxDepth = curResult.maxDepth;
                result.toEvict = curFrame;
                result.evictParent = curParent;
                result.pathToEvict =pathToCurrent;
            }
            else if ((curResult.empty && result.empty) || (!curResult.empty && !result.empty))
            {
                if (curResult.maxFrameIndex > result.maxFrameIndex)
                {
                    result.maxFrameIndex = curResult.maxFrameIndex;

                }
                if (findCyclicDistance(virtualAddress, curResult.pathToEvict) >
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
//                    result.pathToEvict = pathToNext;
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
        targetFrame = (result.pathToDeepest & (PAGE_SIZE-1));
        PMwrite(result.evictParent*PAGE_SIZE + targetFrame, 0);
        return result.deepestFrameIndex;
    }
    if (result.maxFrameIndex + 1 < NUM_FRAMES)
//    if (result.maxFrameIndex  < NUM_FRAMES)     //debug
    {
        return result.maxFrameIndex + 1;
    }

    PMevict(result.toEvict, result.pathToEvict);
    targetFrame = (result.pathToEvict & (PAGE_SIZE-1));
    PMwrite(result.evictParent*PAGE_SIZE + targetFrame, 0);
    debugPrintPM();

//    for (auto frame = 1; frame < NUM_FRAMES; ++frame)
//    {
//        if(isFrameEmpty(frame) && frame != parent)
//        {
//            return frame;
//        }
//    }

    return result.toEvict;
}

uint64_t findPath(uint64_t VAddress)
{
    return VAddress >> OFFSET_WIDTH;
}


uint64_t findPageNumber(uint64_t address, uint64_t depth)
{
    //uint64_t pageNumber = address >> (TABLES_DEPTH - depth); //TODO verify calculations


//    uint64_t pageNumber = (address >> ((TABLES_DEPTH - (depth + 1)) * OFFSET_WIDTH)) & (PAGE_SIZE - 1LL);
    uint64_t pageNumber = (address >> ((TABLES_DEPTH - (depth)) * OFFSET_WIDTH)) & (PAGE_SIZE - 1LL);

    return pageNumber;
}

//uint64_t getVirtualParent(uint64_t virtualAddr)
//{
//    uint64_t parent = 0;
//    uint64_t path = 0;
//    uint64_t curAddr = 0;
//    word_t curWord = 0;
//    word_t depth = 0;
//    while(virtualAddr != curWord)
//    {
//        curAddr = findPageNumber(virtualAddr, depth);
//        parent = curWord;
//        path = (path << OFFSET_WIDTH) | curAddr;
//        PMread(parent*PAGE_SIZE + curAddr, &curWord);
//        depth++;
//    }
//    return parent;
//}


word_t findAddress(uint64_t virtualAddr)
{
    word_t tempWord = 0;
    word_t victim;
    word_t lastPage = 0;
    uint64_t curPageNumber;
//    uint64_t curPageNumber = findPageNumber(virtualAddr, 0);      // Top level table
//    printf("first Page number = %lu\n", curPageNumber);
//    uint64_t virtualPath = findPath(virtualAddr);
//    PMread(0 + curPageNumber, &tempWord);
//    if (tempWord == 0)
//    {
//        victim = findNextFrameIndex(curPageNumber);
//        clearTable(victim);         // findNextFrameIndex returns word_t as it should, but clearTable accepts uint64_t so maybe something is missing
//        PMwrite(0 + curPageNumber, victim);
//        debugPrintPM();
//        tempWord = victim;
//    }
//    lastPage = word_t(tempWord);

   for (uint64_t depth = 0; depth < TABLES_DEPTH; depth++)
   {
       curPageNumber = findPageNumber(virtualAddr, depth);
       PMread(tempWord*PAGE_SIZE + curPageNumber, &tempWord);
       if (tempWord == 0)
       {
           victim = findNextFrameIndex(lastPage, findPath(virtualAddr));
//           victim = findNextFrameIndex(lastPage);
           if (victim == lastPage)
           {
               //TODO pick second last
           }
           //           TODO evict n'stuff
           clearTable(victim);
           debugPrintPM();
           if (depth == TABLES_DEPTH)     // only if actual page
           {
               PMrestore(victim, virtualAddr/ PAGE_SIZE);
           }
           PMwrite(lastPage*PAGE_SIZE + curPageNumber, victim);
//           PMwrite(tempWord*PAGE_SIZE + curPageNumber, victim);
           debugPrintPM();
           tempWord = victim;

       }
       lastPage = word_t(tempWord);
   }
   uint64_t  outAdress = tempWord * PAGE_SIZE + findPageNumber(virtualAddr, TABLES_DEPTH);
   return outAdress;
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
//    printf("physicalAddr = %u\n", physicalAddr);
    PMwrite(physicalAddr, value);
    return 1;
}

int main(int argc, char **argv) {
    printf("RAM SIZE  = %lld\n", RAM_SIZE);
    printf("Page Size = %lld\n", PAGE_SIZE);
    printf("VIRTUAL_MEMORY_SIZE = %lld\n", VIRTUAL_MEMORY_SIZE);
    printf("NUM_FRAMES = %lld\n", NUM_FRAMES);
    printf("NUM_PAGES = %lld\n", NUM_PAGES);
    VMinitialize();

//    printf("findNextPath  %lu\n", findNextPath(13));

//    for (uint64_t i = 0; i < ( NUM_FRAMES); ++i) {
        uint64_t target = 13;
        word_t value = 3;
        printf("==================writing %llu to %llu ================\n", (long long int)value, (long long int) target);
        VMwrite(target, value);
        printf("-----------------------------------------------\n");
    debugPrintPM();

    target = 6;
    value = 10;
    printf("==================writing %llu to %llu ================\n", (long long int)value, (long long int) target);
    VMwrite(target, value);
    printf("-----------------------------------------------\n");
    debugPrintPM();
//    printf("VParent = %lu\n", getVirtualParent(4));

    target = 31;
    value = 9;
    printf("==================writing %llu to %llu ================\n", (long long int)value, (long long int) target);
    VMwrite(target, value);
    printf("-----------------------------------------------\n");
    debugPrintPM();


//    }
//    for (uint64_t i = 0; i < ( NUM_FRAMES); ++i) {
//        word_t value;
//        VMread(5 * i * PAGE_SIZE, &value);
//        printf("reading from %llu %d\n", (long long int) i, value);
////        assert(uint64_t(value) == i);
//    }



//    VMinitialize();
//    for (uint64_t i = 0; i < ( NUM_FRAMES); ++i) {
//        printf("writing to %llu\n", (long long int) i);
//        VMwrite(5 * i * PAGE_SIZE, i);
//    }
//
//    for (uint64_t i = 0; i < ( NUM_FRAMES); ++i) {
//        word_t value;
//        VMread(5 * i * PAGE_SIZE, &value);
//        printf("reading from %llu %d\n", (long long int) i, value);
////        assert(uint64_t(value) == i);
//    }
//    printf("success\n");

    return 0;
}