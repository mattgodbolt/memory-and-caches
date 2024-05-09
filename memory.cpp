#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sys/mman.h>

struct Elem {
    struct Elem* next;
    uint64_t value;
};

bool write = false;
bool randomize = false;
bool hugePages = false;
const size_t log2MaxElemSize = 12;
size_t log2MaxWorkingSet = 28;

static void visitRead(Elem* elem) {
    asm ("":"=m"(*elem));
}

static uint64_t visitWrite(Elem* elem) {
    elem->value++;
    return elem->value;
}

inline uint64_t ReadInitialTSC() {
    union {
        uint64_t i64;
        uint32_t i32[2];
    };
    __asm__ __volatile__("cpuid\n\trdtsc" : "=a"(i32[0]), "=d"(i32[1]) :: "%rbx", "%rcx");
    return i64;
}

inline uint64_t ReadFinalTSC() {
    union {
        uint64_t i64;
        uint32_t i32[2];
    };
    __asm__ __volatile__("rdtscp\n\tmov %%eax, %0\n\tmov %%edx, %1\n\tcpuid" : "=r"(i32[0]), "=r"(
        i32[1]) :: "%rax", "%rbx", "%rcx", "%rdx");
    return i64;
}

struct SortKey {
    uint32_t random;
    uint32_t index;
};

int compare(const void* lhs_, const void* rhs_) {
    const SortKey* lhs = (const SortKey*) lhs_;
    const SortKey* rhs = (const SortKey*) rhs_;
    if (lhs->random > rhs->random) return 1;
    if (lhs->random < rhs->random) return -1;
    return 0;
}

SortKey* allocKeys(size_t numElems) {
    uint16_t seed[] = { 0xdead, 0xcafe, 0xbabe };
    SortKey* sortKey = (SortKey*) malloc(sizeof(SortKey) * numElems);
    for (size_t i = 0; i < numElems; ++i) {
        sortKey[i].random = nrand48(seed);
        sortKey[i].index = i;
    }
    return sortKey;
}

char* theMemory = NULL;

Elem* initialize(size_t elemSize, size_t numElems) {
    SortKey* sortKeys = allocKeys(numElems);
    sortKeys[0].random = 0; // Force zeroth to be the first
    if (randomize) qsort(sortKeys, numElems, sizeof(SortKey), compare);

    size_t sizeNeeded = elemSize * numElems;
    for (size_t count = 0; count < numElems; ++count) {
        size_t i = sortKeys[count].index;
        Elem* elemI = (Elem*)(theMemory + elemSize * i);
        if (count == numElems-1) {
            elemI->next = (Elem*)theMemory;
        } else {
            elemI->next = (Elem*)(theMemory + elemSize * sortKeys[count+1].index);
        }
    }
    free(sortKeys);

    return (Elem*) theMemory;
}

volatile uint64_t ensure_written = 0;
void runThrough(Elem* first, size_t nElem) __attribute__((noinline));
void runThrough(Elem* first, size_t nElem) {
    Elem* p = first;
    if (!write) {
        while (nElem--) {
            visitRead(p);
            p = p->next;
        }
    } else {
        uint64_t total = 0;
        while (nElem--) {
            total += visitWrite(p);
            p = p->next;
        }
        ensure_written += total;
    }
    asm ("":::"memory");
}

uint64_t runTest(size_t log2ElemSize, size_t log2WorkingSet, int numReps) {
    size_t elemSize = 1 << log2ElemSize;
    size_t workingSet = 1 << log2WorkingSet;
    size_t numElems = workingSet / elemSize;
    Elem* first = initialize(elemSize, numElems);

    size_t numElemsTimesReps = numElems * numReps;
    uint64_t startTime = ReadInitialTSC();
    runThrough(first, numElemsTimesReps);
    uint64_t endTime = ReadFinalTSC();

    return endTime - startTime;
}

void initMem() {
    const size_t mmapSize = 2UL * 1024 * 1024 * 1024;
    size_t flags = MAP_PRIVATE|MAP_ANONYMOUS|MAP_NORESERVE|MAP_POPULATE;
    if (hugePages) flags|=MAP_HUGETLB;
    theMemory = (char*)mmap(NULL, mmapSize, PROT_READ|PROT_WRITE,
            flags, -1, 0);
    if (theMemory == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
}

const int numOuterReps = 4;
int main(int argc, const char* argv[]) {
    if (argc >= 2) {
        hugePages = strcmp(argv[1], "huge") == 0;
    }
    if (argc >= 3) {
        randomize = strcmp(argv[2], "random") == 0;
    }
    if (argc >= 4) {
        write = strcmp(argv[3], "write") == 0;
    }
    fprintf(stderr, "Initialised for %s pages, %s order, %s\n", 
        hugePages ? "huge" : "normal", 
        randomize ? "random" : "sequential",
        write ? "write" : "readonly");
    initMem();

    uint64_t overheadTime = (uint64_t)-1;
    for (int i = 0; i < 1024; ++i) {
        uint64_t thisTime = runTest(4, 4, 1024) / 1024;
        if (thisTime < overheadTime) overheadTime = thisTime;
    }
    fprintf(stderr, "Overhead: %ld cycles\n", overheadTime);

    for (size_t log2ElemSize = 4; log2ElemSize <= log2MaxElemSize; ++log2ElemSize) {
        for (size_t log2WS = log2ElemSize > 12 ? log2ElemSize : 12; 
                log2WS <= log2MaxWorkingSet; ++log2WS) {
            fprintf(stderr, "Running test log2(elemSize)=%ld, log2(WorkingSet)=%ld\n", log2ElemSize, log2WS);
            uint64_t timeTaken = (uint64_t)-1;
            uint64_t numOperations = 1<<(log2WS - log2ElemSize);
            int numInnerReps = 64;
            for (int i = 0; i < numOuterReps; ++i) {
                uint64_t thisTime = runTest(log2ElemSize, log2WS, numInnerReps) / numInnerReps;
                if (thisTime < timeTaken) timeTaken = thisTime;
            }
            if (timeTaken >= overheadTime)
                timeTaken -= overheadTime;
            else
                timeTaken = 0;
            double cyclesPerOp = (double)timeTaken / numOperations ;
            printf("%ld,%ld,%ld,%f\n", log2ElemSize, log2WS, timeTaken, cyclesPerOp);
        }
    }

    return 0;
}
