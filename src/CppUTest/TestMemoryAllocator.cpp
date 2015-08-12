/*
 * Copyright (c) 2007, Michael Feathers, James Grenning and Bas Vodde
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE EARLIER MENTIONED AUTHORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "CppUTest/TestHarness.h"
#include "CppUTest/TestMemoryAllocator.h"
#include "CppUTest/PlatformSpecificFunctions.h"
#include "CppUTest/MemoryLeakDetector.h"

static char* checkedMalloc(size_t size)
{
    char* mem = (char*) PlatformSpecificMalloc(size);
    if (mem == 0)
    FAIL("malloc returned null pointer");
    return mem;
}

static TestMemoryAllocator* currentNewAllocator = 0;
static TestMemoryAllocator* currentNewArrayAllocator = 0;
static TestMemoryAllocator* currentMallocAllocator = 0;

void setCurrentNewAllocator(TestMemoryAllocator* allocator)
{
    currentNewAllocator = allocator;
}

TestMemoryAllocator* getCurrentNewAllocator()
{
    if (currentNewAllocator == 0) setCurrentNewAllocatorToDefault();
    return currentNewAllocator;
}

void setCurrentNewAllocatorToDefault()
{
    currentNewAllocator = defaultNewAllocator();
}

TestMemoryAllocator* defaultNewAllocator()
{
    static TestMemoryAllocator allocator("Standard New Allocator", "new", "delete");
    return &allocator;
}

void setCurrentNewArrayAllocator(TestMemoryAllocator* allocator)
{
    currentNewArrayAllocator = allocator;
}

TestMemoryAllocator* getCurrentNewArrayAllocator()
{
    if (currentNewArrayAllocator == 0) setCurrentNewArrayAllocatorToDefault();
    return currentNewArrayAllocator;
}

void setCurrentNewArrayAllocatorToDefault()
{
    currentNewArrayAllocator = defaultNewArrayAllocator();
}

TestMemoryAllocator* defaultNewArrayAllocator()
{
    static TestMemoryAllocator allocator("Standard New [] Allocator", "new []", "delete []");
    return &allocator;
}

void setCurrentMallocAllocator(TestMemoryAllocator* allocator)
{
    currentMallocAllocator = allocator;
}

TestMemoryAllocator* getCurrentMallocAllocator()
{
    if (currentMallocAllocator == 0) setCurrentMallocAllocatorToDefault();
    return currentMallocAllocator;
}

void setCurrentMallocAllocatorToDefault()
{
    currentMallocAllocator = defaultMallocAllocator();
}

TestMemoryAllocator* defaultMallocAllocator()
{
    static TestMemoryAllocator allocator("Standard Malloc Allocator", "malloc", "free");
    return &allocator;
}

/////////////////////////////////////////////

TestMemoryAllocator::TestMemoryAllocator(const char* name_str, const char* alloc_name_str, const char* free_name_str)
    : name_(name_str), alloc_name_(alloc_name_str), free_name_(free_name_str), hasBeenDestroyed_(false)
{
}

TestMemoryAllocator::~TestMemoryAllocator()
{
    hasBeenDestroyed_ = true;
}

bool TestMemoryAllocator::hasBeenDestroyed()
{
    return hasBeenDestroyed_;
}

bool TestMemoryAllocator::isOfEqualType(TestMemoryAllocator* allocator)
{
    return SimpleString::StrCmp(this->name(), allocator->name()) == 0;
}

char* TestMemoryAllocator::allocMemoryLeakNode(size_t size)
{
    return alloc_memory(size, "MemoryLeakNode", 1);
}

void TestMemoryAllocator::freeMemoryLeakNode(char* memory)
{
    free_memory(memory, "MemoryLeakNode", 1);
}

char* TestMemoryAllocator::alloc_memory(size_t size, const char*, int)
{
    return checkedMalloc(size);
}

void TestMemoryAllocator::free_memory(char* memory, const char*, int)
{
    PlatformSpecificFree(memory);
}
const char* TestMemoryAllocator::name()
{
    return name_;
}

const char* TestMemoryAllocator::alloc_name()
{
    return alloc_name_;
}

const char* TestMemoryAllocator::free_name()
{
    return free_name_;
}

CrashOnAllocationAllocator::CrashOnAllocationAllocator() : allocationToCrashOn_(0)
{
}

void CrashOnAllocationAllocator::setNumberToCrashOn(unsigned allocationToCrashOn)
{
    allocationToCrashOn_ = allocationToCrashOn;
}

char* CrashOnAllocationAllocator::alloc_memory(size_t size, const char* file, int line)
{
    if (MemoryLeakWarningPlugin::getGlobalDetector()->getCurrentAllocationNumber() == allocationToCrashOn_)
        UT_CRASH();

    return TestMemoryAllocator::alloc_memory(size, file, line);
}


char* NullUnknownAllocator::alloc_memory(size_t /*size*/, const char*, int)
{
    return 0;
}

void NullUnknownAllocator::free_memory(char* /*memory*/, const char*, int)
{
}

NullUnknownAllocator::NullUnknownAllocator()
    : TestMemoryAllocator("Null Allocator", "unknown", "unknown")
{
}


TestMemoryAllocator* NullUnknownAllocator::defaultAllocator()
{
    static NullUnknownAllocator allocator;
    return &allocator;
}

FailableMemoryAllocator::FailableMemoryAllocator(const char* name_str, const char* alloc_name_str, const char* free_name_str)
: TestMemoryAllocator(name_str, alloc_name_str, free_name_str)
, toFailCount_(0), locationToFailCount_(0), currentAllocNumber_(0)
{
    PlatformSpecificMemset(allocsToFail_, 0, sizeof(allocsToFail_));
    PlatformSpecificMemset(locationAllocsToFail_, 0, sizeof(locationAllocsToFail_));
    PlatformSpecificMemset(locationActualAllocs_, 0, sizeof(locationActualAllocs_));

}

void FailableMemoryAllocator::failAllocNumber(int number)
{
    if (toFailCount_ >= MAX_NUMBER_OF_FAILED_ALLOCS)
        FAIL("Maximum number of failed memory allocations exceeded");
    allocsToFail_[toFailCount_++] = number;
}

void FailableMemoryAllocator::failNthAllocationAt(int allocationNumber, const char* file, int line)
{
    locationAllocsToFail_[locationToFailCount_].allocationNumber = allocationNumber;
    locationAllocsToFail_[locationToFailCount_].file = file;
    locationAllocsToFail_[locationToFailCount_].line = line;

    locationActualAllocs_[locationToFailCount_].allocationNumber = 0;
    locationActualAllocs_[locationToFailCount_].file = file;
    locationActualAllocs_[locationToFailCount_].line = line;
    locationToFailCount_++;
}

char* FailableMemoryAllocator::alloc_memory(size_t size, const char* file, int line)
{
    if (shouldBeFailedAlloc_() || shouldBeFailedLocationAlloc_(file, line))
        return NULL;
    return TestMemoryAllocator::alloc_memory(size, file, line);
}

bool FailableMemoryAllocator::shouldBeFailedAlloc_()
{
    currentAllocNumber_++;
    for (int i = 0; i < toFailCount_; i++)
        if (currentAllocNumber_ == allocsToFail_[i])
            return true;
    return false;
}

bool FailableMemoryAllocator::shouldBeFailedLocationAlloc_(const char* file, int line)
{
    SimpleString allocBaseName = getBaseName_(file);

    for (int i = 0; i < locationToFailCount_; i++) {
        SimpleString toFailBasename = getBaseName_(locationAllocsToFail_[i].file);
        if (allocBaseName == toFailBasename && locationAllocsToFail_[i].line == line) {
            locationActualAllocs_[i].allocationNumber++;
            if (locationAllocsToFail_[i].allocationNumber == locationActualAllocs_[i].allocationNumber)
                return true;
        }
    }
    return false;
}

SimpleString FailableMemoryAllocator::getBaseName_(const char* file)
{
    SimpleString fileFullPath(file);
    fileFullPath.replace('\\', '/');
    SimpleStringCollection pathElements;
    fileFullPath.split("/", pathElements);
    return pathElements[pathElements.size() - 1];
}

char* FailableMemoryAllocator::allocMemoryLeakNode(size_t size)
{
    return (char*)PlatformSpecificMalloc(size);
}

void FailableMemoryAllocator::clearFailedAllocations()
{
    toFailCount_ = 0;
    currentAllocNumber_ = 0;
    PlatformSpecificMemset(allocsToFail_, 0, sizeof(allocsToFail_));
    PlatformSpecificMemset(locationAllocsToFail_, 0, sizeof(locationAllocsToFail_));
    PlatformSpecificMemset(locationActualAllocs_, 0, sizeof(locationActualAllocs_));
}

