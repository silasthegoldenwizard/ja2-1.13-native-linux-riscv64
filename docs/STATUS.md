# Source update status

Updated November 21, 2026.

The current source checkpoint includes the [November 21 save-load and crash-report changes](CHANGELOG.md#november-21-save-load-and-crash-report-checkpoint).
The downloadable source preview is [v0.1.0-alpha.1](https://github.com/silasthegoldenwizard/ja2-1.13-native-linux-riscv64/releases/tag/v0.1.0-alpha.1).

The native Linux build writes local crash reports under the selected profile.
Each report includes the exact ELF build ID. The reporter sends no reports or telemetry.

## Source changes

- Vehicle, squad, and militia loaders publish the read position only after a successful load.
- The top-level loader stops if a vehicle-movement read or obsolete vehicle-definition read fails.
- The native crash reporter records local diagnostics. The offline symbolizer checks the exact build ID.
- The default build uses the single-player network provider and does not link RakNet.

Earlier source changes remain in the current reconstruction.

- Battle-end cleanup skips empty team slots and continues with later live soldiers. Victory updates use the configured player-team ID range.
- World teardown resolves soldiers by runtime ID and skips empty and inactive slots. It preserves player soldier objects and clears their world-node pointers.
- The usable-mercenary scan and pre-battle wake-up both reach the final configured player slot. They skip empty and inactive slots.
- The scripted SDL/X11 button viewer drains ambient input before each scripted step. The change is in the test viewer.

The engine uses bfVFS and the pinned dependencies.
The [November 18 native checkpoint](CHANGELOG.md#november-18-native-checkpoint) and the [stream and text record](STREAM-TEXT-OWNERSHIP-20261117.md) describe earlier changes.

## Validation

The Owner's latest development summary records 334 of 334 Debug tests and 336
of 336 strict sanitizer tests. The strict lane used AddressSanitizer,
UndefinedBehaviorSanitizer, and LeakSanitizer and recorded no sanitizer
diagnostic markers.

A disposable live Linux x86-64 session reached the rendered main menu on a
private X11 display. A controlled `SIGABRT` then produced a mode-`0600` report
with the expected signal and the same ELF build ID as the exact game binary.
The offline symbolizer accepted that binary. A separate automated hardware
fault fixture resolves the faulting instruction in its test executable to the
fixture source line.

The 334 and 336 results are local development-suite counts. The hosted checks
cover a smaller source and component test set. The export has separate
focused checks for the save loaders and the VFS-enabled crash reporter.
See [CI scope](CI.md) and [Native Linux crash reports](CRASH-REPORTING.md).
A public player package, RISC-V hardware, and full campaign acceptance remain open.

The November 19 published source checkpoint recorded 268 native passes and 270
strict sanitizer passes. Those suites ran serially on a real X11 display. Live
strategic gameplay and sector transitions remain open for that checkpoint.

The two focused final-slot regressions passed in each build. They call the affected engine paths with a controlled roster.
The scripted viewer checks authored events in its SDL/X11 button scene. Focused synthetic tests use controlled inputs and test doubles.

The [hosted checks](CI.md) cover reconstruction, selected native components, source-aware guards, and the [attachment fixture](../tests/attachment-completion/README.md).
The fixture binds real production fragments and uses controlled attachment outcomes and UI effects.
Full exported-game build and gameplay results are separate validation steps.
Full campaign testing, Windows, and native RISC-V validation remain open for
the latest development checkpoint.
Full sound, device, and live multiplayer coverage remain open.

The [runtime-boundary record](RUNTIME-BOUNDARIES-20261117.md), [crash-hardening record](CRASH-HARDENING-20261117.md), and [inventory batch](INVENTORY-20261116.md) retain their historical evidence.
The [magazine checkpoint](MAGAZINE-20261116.md) and [earlier checkpoint](UPDATE-20261116.md) retain their own test scopes.
