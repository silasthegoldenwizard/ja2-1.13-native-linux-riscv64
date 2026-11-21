# Native Linux crash reports

The current native Linux source includes a local crash reporter. It helps
maintainers identify the exact executable and failure. Reports stay on the
machine that runs the game.

## Find a report

Reports are stored in the selected writable profile:

```text
PROFILE/CrashReports/crash_report_001.txt
```

The reporter creates numbered files with mode `0600`. It does not upload a
report or send telemetry.

## Read a report locally

If the build includes `platform/linux/symbolize_crash_report.py`, use the exact
unstripped executable that produced the report:

```text
python3 platform/linux/symbolize_crash_report.py \
  PROFILE/CrashReports/crash_report_001.txt \
  --binary BUILD/p710_native_game
```

The tool rejects a different ELF build ID. Renaming a byte-identical executable
does not prevent symbolization.

## Before sharing

Treat crash reports as private until you review them. Controlled-failure and
assertion reports can contain local executable or module paths. Remove or
redact private paths and identifying details from the text before sharing it.
Never include:

- proprietary game data;
- profiles or publicly uploaded saved games;
- account names, access tokens, or other private files.

Share a minimal saved game privately only if a maintainer requests it and you
agree.

For a useful bug report, include the exact action before the failure, the game
screen and sector, the selected options, and whether the problem repeats after
loading the same save. Add the reviewed crash report and executable build ID
when available.

## Current limits

- Report availability depends on the source revision used for the build.
- Fatal-signal reports contain signal-safe essential context. Controlled
  failures and assertions can include a fuller stack trace.
- Sanitizer builds leave fatal-signal handling to the sanitizer runtime.
- A library loaded after reporter initialization can be absent from the frozen
  module map.
- The crash reporter passed a Linux x86-64 main-menu test with a controlled
  signal. Native RISC-V hardware acceptance remains open.
