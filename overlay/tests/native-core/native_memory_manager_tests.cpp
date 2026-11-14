/*
 * P710 native snapshot update: tests/native-core/native_memory_manager_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "MemMan.h"
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>

extern BOOLEAN fMemManagerInit;
namespace {
unsigned checks = 0;
void check(bool result, const char* reason)
{
    ++checks;
    if (!result) throw std::runtime_error(reason);
}
UINT32 expectedTotalMemory()
{
    const long pages = ::sysconf(_SC_PHYS_PAGES), pageSize = ::sysconf(_SC_PAGESIZE);
    if (pages < 0 || pageSize <= 0) return 0;
    const std::uint64_t limit = std::numeric_limits<UINT32>::max();
    if (static_cast<std::uint64_t>(pages) > limit / static_cast<std::uint64_t>(pageSize))
        return static_cast<UINT32>(limit);
    return static_cast<UINT32>(static_cast<std::uint64_t>(pages) * pageSize);
}
}
int main()
{
    try
    {
        MemDebug(FALSE);
        check(InitializeMemoryManager() && fMemManagerInit, "initialization");
        check(MemDebugCounter == 0 && guiMemTotal == 0 && guiMemAlloced == 0 && guiMemFreed == 0, "initial counters");
        check(MemAllocLocked(0) == nullptr && MemDebugCounter == 0, "zero-size allocation fails without accounting");
        const long pageSize = ::sysconf(_SC_PAGESIZE);
        check(pageSize > 0 && static_cast<unsigned long>(pageSize) < UINT32_MAX / 2, "page size");
        const UINT32 bytes = static_cast<UINT32>(pageSize) + 17;
        auto* first = static_cast<unsigned char*>(MemAllocLocked(bytes));
        check(first != nullptr, "real mapping allocated");
        for (UINT32 i = 0; i < bytes; ++i) check(first[i] == 0, "fresh anonymous mapping is zero-filled");
        std::memset(first, 0xa5, bytes);
        auto* second = static_cast<unsigned char*>(MemAllocLocked(13));
        check(second && second != first && second[0] == 0, "second independent mapping");
        check(guiMemTotal == bytes + 13 && guiMemAlloced == bytes + 13 && MemDebugCounter == 2, "successful mapping counters");
        check(InitializeMemoryManager() && guiMemTotal == bytes + 13, "repeated initialization preserves live accounting");
        check(MemCheckPool(), "owned mappings actually mapped");
        MemFreeLocked(first, bytes - 1);
        MemFreeLocked(first + 1, bytes);
        check(guiMemTotal == bytes + 13 && MemDebugCounter == 2 && first[bytes - 1] == 0xa5, "invalid ownership/size cannot unmap or change accounting");
        PTR ordinary = MemAlloc(23);
        check(ordinary != nullptr, "existing malloc macro works");
        MemFree(ordinary);
        check(MemDebugCounter == 2 && guiMemTotal == bytes + 13, "release malloc macros remain outside locked-map accounting");
        MemFreeLocked(first, bytes);
        unsigned char residency = 0;
        errno = 0;
        check(::mincore(first, static_cast<std::size_t>(pageSize), &residency) == -1 && errno == ENOMEM, "released mapping is genuinely unmapped");
        check(guiMemTotal == 13 && guiMemFreed == bytes && MemDebugCounter == 1, "release counters");
        check(MemGetTotalSystem() != 0, "physical memory query succeeds");
        const UINT32 expected = expectedTotalMemory();
        if (expected) check(MemGetTotalSystem() == expected, "physical total bytes saturate to original UINT32 domain");
        check(MemGetFree() <= MemGetTotalSystem(), "available physical memory bounded by total");
        ShutdownMemoryManager();
        check(!fMemManagerInit && guiMemTotal == 13, "shutdown leaves owner memory intact and records lifecycle");
        check(!InitializeMemoryManager(), "outstanding mapping prevents destructive counter reset");
        MemFreeLocked(second, 13);
        check(MemDebugCounter == 0 && guiMemTotal == 0 && guiMemFreed == bytes + 13, "owner can release after shutdown");
        check(MemCheckPool(), "empty release pool makes no global heap-validation claim");
        check(InitializeMemoryManager() && guiMemAlloced == 0 && guiMemFreed == 0, "clean lifecycle can restart");
        MemFreeLocked(nullptr, 1);
        check(MemDebugCounter == UINT32_MAX, "original NULL-free debug-error decrement retained");
        ShutdownMemoryManager();
        check(InitializeMemoryManager() && MemDebugCounter == 0, "restart resets legacy error counter");
        ShutdownMemoryManager();
        std::cout << checks << " native memory-manager checks passed\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Native memory-manager fixture failed: " << error.what() << '\n';
        return 1;
    }
}
