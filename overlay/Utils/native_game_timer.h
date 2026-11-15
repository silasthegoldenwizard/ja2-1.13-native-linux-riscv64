/*
 * P710 native snapshot update: Utils/native_game_timer.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef JA2_NATIVE_GAME_TIMER_H
#define JA2_NATIVE_GAME_TIMER_H
#include <cstdint>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

namespace ja2::game_timer {
constexpr std::int32_t BaseSlice = 10;
struct Configuration {
    bool highSpeed{}, fastForward{}, explicitFastForward{}, paused{};
    std::uint32_t updateMicroseconds{10000}, fastForwardMicroseconds{1000};
    std::uint32_t notifyMicroseconds{16000};
};
struct Pulse {
    Configuration configuration;
    std::uint64_t count{1};
};
struct Decision {
    bool tick{}, notify{};
    std::int32_t timeLeft{};
};
// Shared production decisions: high-speed uses a strict deadline comparison;
// a late observation schedules from now, without inventing missed worker ticks.
Decision AcceptedTick(const Configuration&);
Decision DecideTick(const Configuration&, std::uint64_t now, std::uint64_t& deadline);
std::uint32_t NextWait(std::uint64_t now, std::uint64_t& deadline);
bool AdvanceClocks(std::uint32_t& game, std::uint32_t& noPause, bool paused);
bool UpdateCountdown(std::int32_t& counter, std::int32_t& timeLeft);
std::uint64_t MonotonicMicroseconds();

// Worker owns scheduling only. It queues accepted 10ms logical slices and calls
// wake with no game state access. Owner drains/executes every accepted slice.
// Config changes do not rewrite a queued pulse's pause/speed snapshot.
class Driver {
public:
    using Wake = void (*)(void*);
    Driver() = default;
    ~Driver();
    Driver(const Driver&) = delete;
    Driver& operator=(const Driver&) = delete;
    bool Start(const Configuration&, Wake, void*);
    bool Configure(const Configuration&);
    bool Stop(); // Joins worker; leaves accepted backlog available to Take().
    std::deque<Pulse> Take();
    bool Healthy() const;
    bool IsOwnerThread() const;
    std::uint32_t NextWaitMicroseconds();
private:
    void Run();
    mutable std::mutex mutex_;
    std::condition_variable changed_;
    std::thread worker_;
    std::thread::id owner_;
    Configuration configuration_;
    Wake wake_{};
    void* state_{};
    std::deque<Pulse> pending_;
    std::uint64_t generation_{}, deadline_{}, queryDeadline_{};
    bool running_{}, stopping_{}, healthy_{true};
};
using Notify = void (*)(std::int32_t, void*);
// Main-thread registry. Payloads are borrowed. Dispatch snapshots identities,
// skips entries removed before their turn and defers additions to next dispatch.
class Notifications {
public:
    void Add(Notify, void*);
    void Remove(Notify, void*);
    void Clear();
    bool Empty() const;
    bool Broadcast(std::int32_t); // Stops this dispatch on exception; reports false.
private:
    struct Entry { Notify callback; void* state; std::uint64_t identity; };
    std::vector<Entry> entries_;
    std::uint64_t nextIdentity_{};
};
}
#endif
