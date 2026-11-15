/*
 * P710 native snapshot update: sgp/NativeClock.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native SGP elapsed clock. The original JA2 gameplay timer remains separate.
// Reuse the standard monotonic clock; no window-message timer/thread is needed.
#include "timer.h"
#include <chrono>
#include <mutex>

UINT32 guiStartupTime = 0;
UINT32 guiCurrentTime = 0;

namespace {
using Clock = std::chrono::steady_clock;
std::mutex clockMutex;
Clock::time_point started;
bool running = false;
UINT32 current()
{
    if (running)
        guiCurrentTime = static_cast<UINT32>(
            std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - started).count());
    return guiCurrentTime;
}
}

BOOLEAN InitializeClockManager()
{
    std::lock_guard<std::mutex> lock(clockMutex);
    started = Clock::now();
    guiStartupTime = guiCurrentTime = 0;
    running = true;
    return TRUE;
}
void ShutdownClockManager()
{
    std::lock_guard<std::mutex> lock(clockMutex);
    current();
    running = false;
}
TIMER GetClock()
{
    std::lock_guard<std::mutex> lock(clockMutex);
    return current();
}
TIMER SetCountdownClock(UINT32 duration)
{
    std::lock_guard<std::mutex> lock(clockMutex);
    return current() + duration;
}
UINT32 ClockIsTicking(TIMER deadline)
{
    std::lock_guard<std::mutex> lock(clockMutex);
    const UINT32 now = current();
    // Retain the original absolute UINT32 deadline contract. This legacy
    // helper does not support a countdown crossing the 32-bit clock rollover.
    return deadline > now ? deadline - now : 0;
}
