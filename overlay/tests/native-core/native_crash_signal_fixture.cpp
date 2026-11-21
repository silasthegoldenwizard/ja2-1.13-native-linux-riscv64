/*
 * P710 native snapshot update: tests/native-core/native_crash_signal_fixture.cpp
 * Native Linux source/build/test adaptations captured in the November 20-21 save-load and crash-report checkpoint.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-21.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "native_crash_report.h"

#include <filesystem>
#include <sys/mman.h>
#include <sys/resource.h>

int main(int argc, char** argv)
{
    if (argc != 2) return 64;
    std::filesystem::current_path(argv[1]);
    const rlimit noCore{0, 0};
    setrlimit(RLIMIT_CORE, &noCore);
    sgp::InstallNativeCrashReporter("native-crash-symbolizer-fixture");
    if (!sgp::NativeCrashSignalHandlersEnabled()) return 77;
    void* page = mmap(nullptr, 4096, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (page == MAP_FAILED) return 65;
    *static_cast<volatile unsigned char*>(page) = 1;
    return 66;
}
