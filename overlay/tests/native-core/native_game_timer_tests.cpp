/*
 * P710 native snapshot update: tests/native-core/native_game_timer_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "native_game_timer.h"
#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
using namespace ja2::game_timer;
namespace {
unsigned checks = 0;
void check(bool result, const char* message) { ++checks; if (!result) throw std::runtime_error(message); }
void core()
{
    Configuration c; c.highSpeed = true;
    std::uint64_t deadline = 100;
    check(!DecideTick(c, 100, deadline).tick && deadline == 100, "strict QPC-style deadline equality");
    auto accepted = DecideTick(c, 101, deadline);
    check(accepted.tick && !accepted.notify && accepted.timeLeft == 10000 && deadline == 10101, "normal high-speed accepted slice");
    check(!DecideTick(c, 10100, deadline).tick, "no early high-speed slice");
    check(DecideTick(c, 25000, deadline).tick && deadline == 35000, "late worker observation does not synthesize missed slices");
    c.fastForward = true; c.fastForwardMicroseconds = 1000;
    accepted = DecideTick(c, 35001, deadline);
    check(accepted.tick && accepted.notify && accepted.timeLeft == 1000 && deadline == 36001, "fast-forward high-speed period and notify");
    c.highSpeed = false;
    check(DecideTick(c, 0, deadline).tick && !DecideTick(c, 0, deadline).notify, "normal-mode key-fast pulse keeps timerDone false");
    c.fastForward = false;
    check(DecideTick(c, 0, deadline).notify && DecideTick(c, 0, deadline).timeLeft == 0, "normal periodic accepted slice notifies");
    deadline = 11000; check(NextWait(1000, deadline) == 10000 && deadline == 11000, "normal positive microsecond wait hint");
    deadline = 999; check(NextWait(1000, deadline) == 0 && deadline == 999, "slightly missed deadline needs no wait");
    deadline = 20000; check(NextWait(1000, deadline) == 125 && deadline == 1125, "too-distant deadline preserves original125us repair");
    deadline = 0; check(NextWait(20000, deadline) == 125 && deadline == 20125, "too-late deadline repair");
    std::uint32_t game = 20, noPause = 50;
    check(!AdvanceClocks(game, noPause, true) && game == 20 && noPause == 60, "pause retains gameplay and advances no-pause clock");
    check(!AdvanceClocks(game, noPause, false) && game == 30 && noPause == 70, "every accepted slice adds original ten milliseconds");
    game = INT32_MAX - 4; noPause = UINT32_MAX - 4;
    check(AdvanceClocks(game, noPause, false) && game == 0 && noPause == 5, "signed-boundary game reset and unsigned no-pause wrap");
    std::int32_t left = 10000, counter = 5;
    check(UpdateCountdown(counter, left) && counter == 0 && left == 10000, "subslice completion notifies without altering time-left hint");
    counter = 10;
    check(!UpdateCountdown(counter, left) && counter == 0 && left == 0, "exact slice-to-zero retains original false result");
    left = 123; check(!UpdateCountdown(counter, left) && left == 123, "zero counter remains inactive");
    counter = 35; left = 100;
    check(!UpdateCountdown(counter, left) && counter == 25 && left == 25, "countdown and minimum wait hint");
    counter = INT32_MIN; check(UpdateCountdown(counter, left) && counter == 0, "invalid negative counter safely finishes without signed overflow");
}
struct NotifyState { Notifications* registry{}; int first{}, second{}, third{}; };
void third(std::int32_t timer, void* pointer) { check(timer == -1, "notify identifier"); ++static_cast<NotifyState*>(pointer)->third; }
void second(std::int32_t, void* pointer) { ++static_cast<NotifyState*>(pointer)->second; }
void first(std::int32_t, void* pointer)
{
    auto& s = *static_cast<NotifyState*>(pointer); ++s.first;
    s.registry->Remove(second, pointer); s.registry->Add(third, pointer);
}
void throwing(std::int32_t, void*) { throw std::runtime_error("intentional callback exception"); }
void notifications()
{
    Notifications registry; NotifyState state{&registry};
    registry.Add(first, &state); registry.Add(first, &state); registry.Add(second, &state);
    check(registry.Broadcast(-1) && state.first == 1 && state.second == 0 && state.third == 0, "duplicate suppression, reentrant remove, deferred addition");
    check(registry.Broadcast(-1) && state.first == 2 && state.third == 1, "borrowed state persists into next dispatch");
    registry.Clear(); registry.Add(throwing, nullptr); registry.Add(second, &state);
    check(!registry.Broadcast(-1) && state.second == 0, "exception stops current dispatch and reports failure");
    registry.Remove(throwing, nullptr); check(registry.Broadcast(-1) && state.second == 1, "registry usable after callback exception");
    registry.Clear(); registry.Add(nullptr, nullptr); check(registry.Broadcast(-1), "null callback is skipped");
    registry.Clear(); check(registry.Empty(), "clear borrowed registrations");
}
struct WakeState { std::atomic<unsigned> calls{0}; std::thread::id thread; };
void wake(void* state) { auto& s = *static_cast<WakeState*>(state); s.thread = std::this_thread::get_id(); ++s.calls; }
void failingWake(void* state) { ++static_cast<WakeState*>(state)->calls; throw std::runtime_error("intentional wake failure"); }
void lifecycle()
{
    Driver driver; WakeState state; Configuration c; c.highSpeed = true;
    check(!driver.Start(c, nullptr, nullptr), "missing real wake callback rejected");
    auto invalid = c; invalid.updateMicroseconds = 0;
    check(!driver.Start(invalid, wake, &state), "zero update interval rejected");
    check(driver.Start(c, wake, &state) && driver.IsOwnerThread(), "real worker starts with owner thread");
    check(!driver.Start(c, wake, &state), "duplicate worker initialization rejected");
    bool wrongThreadConfigured = true;
    std::thread wrongThread([&] { wrongThreadConfigured = driver.Configure(c); }); wrongThread.join();
    check(!wrongThreadConfigured, "worker configuration is owner-thread only");
    const auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    std::deque<Pulse> pulses;
    while (pulses.empty() && std::chrono::steady_clock::now() < timeout) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1)); pulses = driver.Take();
    }
    check(!pulses.empty() && state.calls.load() > 0, "actual lifecycle delivers accepted slices and wakes");
    c.paused = true; check(driver.Configure(c), "live pause snapshot configuration accepted");
    const auto pauseCalls = state.calls.load();
    const auto pauseTimeout = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (state.calls.load() < pauseCalls + 3 && std::chrono::steady_clock::now() < pauseTimeout)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    check(state.calls.load() >= pauseCalls + 3, "paused scheduler continues real lifecycle delivery");
    check(driver.Stop() && driver.Healthy(), "stop joins real worker without dropping backlog");
    auto backlog = driver.Take();
    bool observedPaused = false;
    for (const auto& pulse : backlog) if (pulse.configuration.paused && pulse.count) observedPaused = true;
    check(observedPaused, "queued accepted pulses preserve pause snapshots");
    check(state.thread != std::this_thread::get_id(), "wake delivered on scheduler worker, never gameplay owner");
    const auto stoppedCalls = state.calls.load(); std::this_thread::sleep_for(std::chrono::milliseconds(10));
    check(state.calls.load() == stoppedCalls && driver.Take().empty(), "no worker delivery after joined shutdown");
    check(driver.Start(c, failingWake, &state), "clean lifecycle restarts");
    const auto failureTimeout = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (driver.Healthy() && std::chrono::steady_clock::now() < failureTimeout) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    check(!driver.Healthy() && driver.Stop(), "wake exception becomes observable failed lifecycle rather than escaping thread");
}
}
int main()
{
    try { core(); notifications(); lifecycle(); std::cout << checks << " production gameplay timer checks passed\n"; return 0; }
    catch (const std::exception& e) { std::cerr << "Gameplay timer fixture failed: " << e.what() << '\n'; return 1; }
}
