/*
 * P710 native snapshot update: platform/linux/NativeRuntime.h
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef P710_NATIVE_RUNTIME_H
#define P710_NATIVE_RUNTIME_H
#include <filesystem>
#include <string>
#include <utility>
#include <vector>
namespace p710 {
struct NativeOptions {
    bool help=false, checkProfile=false, noSound=false;
    std::filesystem::path profile, temp, settings;
    std::vector<std::filesystem::path> assets;
    std::vector<std::pair<std::string,std::string>> overrides;
    std::string gameCommandLine;
};
// Parsing and profile validation never initialize the engine, change cwd, or write.
NativeOptions ParseNativeOptions(int argc, const char* const* argv);
void ValidateNativeProfile(const NativeOptions&);
// Uses the original VFS parser/profile/library machinery. Config files are
// explicit profile-local files; asset profiles are read-only, the one original
// write-profile is redirected to the explicit isolated native profile root.
void InitializeNativeVFS(const NativeOptions&, const std::vector<std::filesystem::path>& configs);
// Actual outer-loop continuation policy. Focus changes input acceptance only;
// original windowed game frames continue alongside clocks until stop/quit.
bool ContinueNativeWindowedFrame(bool running, bool quitRequested, bool inputTerminated, bool inputFocused) noexcept;
int RunNativeMain(int argc, const char* const* argv);
const char* NativeUsage();
}
#endif
