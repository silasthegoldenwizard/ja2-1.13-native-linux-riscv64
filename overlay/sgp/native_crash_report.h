/*
 * P710 native snapshot update: sgp/native_crash_report.h
 * Native Linux source/build/test adaptations captured in the November 20-21 save-load and crash-report checkpoint.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-21.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef P710_NATIVE_CRASH_REPORT_H
#define P710_NATIVE_CRASH_REPORT_H

namespace sgp {

// Installs the local Linux crash reporter after the process has entered its
// isolated writable profile. Reports never leave the machine.
void InstallNativeCrashReporter(const char* buildId) noexcept;

// Records failures that reach a controlled exit path. The report is written to
// CrashReports/ under the active profile and the path is printed to stderr.
void WriteNativeRuntimeFailure(const char* kind, const char* detail) noexcept;
void WriteNativeAssertionFailure(unsigned line, const char* function,
    const char* sourceFile, const char* message) noexcept;

// ASan owns fatal-signal reporting in sanitizer builds; this exposes that
// distinction to focused tests and diagnostics.
bool NativeCrashSignalHandlersEnabled() noexcept;

} // namespace sgp

#endif
