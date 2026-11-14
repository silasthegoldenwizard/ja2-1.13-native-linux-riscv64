/*
 * P710 native snapshot update: Utils/native_game_timer.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "native_game_timer.h"
#include <algorithm>
#include <chrono>
#include <limits>

namespace ja2::game_timer {
namespace {
bool valid(const Configuration& c)
{
    return c.updateMicroseconds > 0 && c.updateMicroseconds <= static_cast<std::uint32_t>(INT32_MAX) &&
        c.fastForwardMicroseconds > 0 && c.fastForwardMicroseconds <= static_cast<std::uint32_t>(INT32_MAX);
}
std::uint64_t interval(const Configuration& c)
{
    if (c.highSpeed) return c.fastForward ? c.fastForwardMicroseconds : c.updateMicroseconds;
    return c.explicitFastForward ? 1000 : std::max<std::uint64_t>(1000, (c.updateMicroseconds / 1000) * 1000);
}
bool equal(const Configuration& a, const Configuration& b)
{
    return a.highSpeed == b.highSpeed && a.fastForward == b.fastForward &&
        a.explicitFastForward == b.explicitFastForward && a.paused == b.paused &&
        a.updateMicroseconds == b.updateMicroseconds && a.fastForwardMicroseconds == b.fastForwardMicroseconds &&
        a.notifyMicroseconds == b.notifyMicroseconds;
}
}
std::uint64_t MonotonicMicroseconds()
{
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}
Decision AcceptedTick(const Configuration& c)
{
    return {true, c.highSpeed ? c.fastForward : !c.fastForward,
        c.highSpeed ? static_cast<std::int32_t>(c.fastForward ? c.fastForwardMicroseconds : c.updateMicroseconds) : 0};
}
Decision DecideTick(const Configuration& c, std::uint64_t now, std::uint64_t& deadline)
{
    if (c.highSpeed) {
        if (now <= deadline) return {};
        const std::uint32_t next = c.fastForward ? c.fastForwardMicroseconds : c.updateMicroseconds;
        deadline = now + next;
        return AcceptedTick(c);
    }
    return AcceptedTick(c);
}
std::uint32_t NextWait(std::uint64_t now, std::uint64_t& deadline)
{
    const std::int64_t wait = deadline > now ? static_cast<std::int64_t>(deadline - now) : -static_cast<std::int64_t>(now - deadline);
    if (wait > 15000 || wait < -15000) { deadline = now + 125; return 125; }
    return wait > 0 ? static_cast<std::uint32_t>(wait) : 0;
}
bool AdvanceClocks(std::uint32_t& game, std::uint32_t& noPause, bool paused)
{
    noPause += BaseSlice;
    if (paused) return false;
    const auto before = game;
    game += BaseSlice;
    // The original code resets at the signed32 boundary, not UINT32_MAX.
    if (game > static_cast<std::uint32_t>(INT32_MAX)) game = 0;
    return before > game;
}
bool UpdateCountdown(std::int32_t& counter, std::int32_t& timeLeft)
{
    if (counter == 0) return false;
    // Widen only the subtraction to avoid signed overflow on invalid negatives;
    // all original defined counter values keep their exact decrement semantics.
    if (static_cast<std::int64_t>(counter) - BaseSlice < 0) { counter = 0; return true; }
    counter -= BaseSlice;
    if (counter < timeLeft) timeLeft = counter;
    // An exact10->0 step returns false, as in the original implementation.
    return false;
}
Driver::~Driver() { Stop(); }
bool Driver::Start(const Configuration& c, Wake wake, void* state)
{
    if (!valid(c) || !wake) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_ || worker_.joinable()) return false;
    configuration_ = c; wake_ = wake; state_ = state; owner_ = std::this_thread::get_id();
    pending_.clear(); queryDeadline_ = 0; deadline_ = c.highSpeed ? 0 : MonotonicMicroseconds() + interval(c);
    stopping_ = false; healthy_ = true; running_ = true; ++generation_;
    try { worker_ = std::thread(&Driver::Run, this); }
    catch (...) { running_ = false; healthy_ = false; return false; }
    return true;
}
bool Driver::Configure(const Configuration& c)
{
    if (!valid(c)) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    if (owner_ != std::this_thread::get_id()) return false;
    if (!equal(configuration_, c)) {
        if (configuration_.highSpeed != c.highSpeed || (!c.highSpeed && interval(configuration_) != interval(c)))
            deadline_ = c.highSpeed ? 0 : MonotonicMicroseconds() + interval(c);
        configuration_ = c; ++generation_; changed_.notify_all();
    }
    return true;
}
bool Driver::Stop()
{
    { std::lock_guard<std::mutex> lock(mutex_);
      if (worker_.joinable() && worker_.get_id() == std::this_thread::get_id()) return false;
      stopping_ = true; changed_.notify_all(); }
    if (worker_.joinable()) worker_.join();
    std::lock_guard<std::mutex> lock(mutex_); running_ = false; return true;
}
std::deque<Pulse> Driver::Take()
{
    std::lock_guard<std::mutex> lock(mutex_);
    std::deque<Pulse> result; result.swap(pending_); return result;
}
bool Driver::Healthy() const { std::lock_guard<std::mutex> lock(mutex_); return healthy_; }
bool Driver::IsOwnerThread() const { std::lock_guard<std::mutex> lock(mutex_); return owner_ == std::this_thread::get_id(); }
std::uint32_t Driver::NextWaitMicroseconds()
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto now = MonotonicMicroseconds();
    auto& queried = configuration_.highSpeed ? deadline_ : queryDeadline_;
    const auto before = queried;
    const auto wait = NextWait(now, queried);
    if (queried != before) changed_.notify_all();
    return wait;
}
void Driver::Run()
{
    std::unique_lock<std::mutex> lock(mutex_);
    std::uint64_t notifyAt = MonotonicMicroseconds();
    while (!stopping_) {
        const auto c = configuration_;
        const auto now = MonotonicMicroseconds();
        bool accepted = c.highSpeed ? now > deadline_ : now >= deadline_;
        bool wake = false;
        if (accepted) {
            auto d = DecideTick(c, now, deadline_);
            if (!c.highSpeed) deadline_ = now + interval(c);
            else if (!c.fastForward) NextWait(now, deadline_);
            if (d.tick) {
                try {
                    if (!pending_.empty() && equal(pending_.back().configuration, c) && pending_.back().count != UINT64_MAX)
                        ++pending_.back().count;
                    else pending_.push_back({c, 1});
                } catch (...) { healthy_ = false; stopping_ = true; }
                wake = true;
            }
        }
        if (now >= notifyAt) {
            wake = true;
            notifyAt = now + std::max<std::uint64_t>(1000, (c.notifyMicroseconds / 1000) * 1000);
        }
        if (wake) {
            const auto callback = wake_; void* state = state_;
            lock.unlock();
            try { callback(state); } catch (...) { lock.lock(); healthy_ = false; stopping_ = true; lock.unlock(); }
            lock.lock();
        }
        if (stopping_) break;
        const auto generation = generation_;
        const auto until = std::min(deadline_ + (c.highSpeed ? 1 : 0), notifyAt);
        changed_.wait_until(lock, std::chrono::steady_clock::time_point(std::chrono::microseconds(until)),
            [&] { return stopping_ || generation != generation_; });
    }
    running_ = false;
}
void Notifications::Add(Notify callback, void* state)
{
    for (const auto& e : entries_) if (e.callback == callback && e.state == state) return;
    entries_.push_back({callback, state, ++nextIdentity_});
}
void Notifications::Remove(Notify callback, void* state)
{
    entries_.erase(std::remove_if(entries_.begin(), entries_.end(), [&](const Entry& e) {
        return e.callback == callback && e.state == state; }), entries_.end());
}
void Notifications::Clear() { entries_.clear(); }
bool Notifications::Empty() const { return entries_.empty(); }
bool Notifications::Broadcast(std::int32_t timer)
{
    try {
        const auto snapshot = entries_;
        for (const auto& e : snapshot) {
            const bool present = std::any_of(entries_.begin(), entries_.end(), [&](const Entry& current) { return current.identity == e.identity; });
            if (present && e.callback) e.callback(timer, e.state);
        }
    } catch (...) { return false; }
    return true;
}
}
