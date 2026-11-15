/*
 * P710 native snapshot update: tests/native-core/native_clock_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "timer.h"
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
namespace {
unsigned checks = 0;
void check(bool condition, const char* message)
{
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
}
int main()
{
    try {
        check(GetClock() == 0 && ClockIsTicking(0) == 0, "cold clock is zero");
        check(InitializeClockManager(), "monotonic SGP clock initializes without window messages");
        const auto hostBefore = std::chrono::steady_clock::now();
        const TIMER before = GetClock();
        const TIMER deadline = SetCountdownClock(1000);
        check(ClockIsTicking(deadline) > 0 && ClockIsTicking(deadline) <= 1000, "countdown is a remaining interval");
        const TIMER shortDeadline = SetCountdownClock(5);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        const TIMER after = GetClock();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - hostBefore).count();
        check(after >= before + 20, "clock advances without event-loop or timer callbacks");
        check(after - before <= static_cast<unsigned long long>(elapsed) + 2, "elapsed time matches independent monotonic observation");
        check(ClockIsTicking(shortDeadline) == 0 && ClockIsTicking(0) == 0, "elapsed countdown saturates at zero");
        check(ClockIsTicking(deadline) < 1000, "long countdown advances");
        ShutdownClockManager();
        const TIMER stopped = GetClock();
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        check(GetClock() == stopped, "shutdown freezes SGP clock");
        const auto resetBefore = std::chrono::steady_clock::now();
        check(InitializeClockManager(), "clock reinitializes");
        const auto resetClock = GetClock();
        const auto resetElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - resetBefore).count();
        check(resetClock <= static_cast<unsigned long long>(resetElapsed) + 1, "reinitialization starts a new epoch");
        ShutdownClockManager();
        std::cout << "PASS: " << checks << " SGP monotonic clock assertions; gameplay timer and rollover deadlines separate\n";
        return 0;
    } catch (const std::exception& error) {
        ShutdownClockManager();
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
