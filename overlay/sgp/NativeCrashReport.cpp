/*
 * P710 native snapshot update: sgp/NativeCrashReport.cpp
 * Native Linux source/build/test adaptations captured in the November 20-21 save-load and crash-report checkpoint.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-21.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "native_crash_report.h"

#include <array>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <elf.h>
#include <execinfo.h>
#include <fcntl.h>
#include <link.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <ucontext.h>
#include <unistd.h>

#if defined(__has_feature)
#  if __has_feature(address_sanitizer) || __has_feature(undefined_behavior_sanitizer)
#    define P710_NATIVE_SANITIZER_BUILD 1
#  endif
#endif
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_UNDEFINED__)
#  define P710_NATIVE_SANITIZER_BUILD 1
#endif
#ifndef P710_NATIVE_SANITIZER_BUILD
#  define P710_NATIVE_SANITIZER_BUILD 0
#endif

namespace {

constexpr char kReportDirectory[] = "CrashReports";
constexpr char kReportTemplate[] = "CrashReports/crash_report_000.txt";
constexpr std::size_t kReportDirectoryPrefixLength = sizeof(kReportDirectory);
constexpr std::size_t kMaxFrames = 64;

char buildId[64] = "local";
char executableBuildId[129] = "unavailable";
char mainImageName[128] = "p710_native_game";
char moduleTable[32 * 1024] = {};
std::size_t moduleTableLength = 0;
#if !P710_NATIVE_SANITIZER_BUILD
thread_local volatile std::sig_atomic_t handlingSignal = 0;
thread_local volatile std::sig_atomic_t skipNextAbort = 0;
alignas(16) std::array<unsigned char, 64 * 1024> alternateSignalStack{};
#endif
bool signalsInstalled = false;
int reportDirectoryDescriptor = -1;

std::size_t textLength(const char* text) noexcept
{
    std::size_t length = 0;
    while (text && text[length]) ++length;
    return length;
}

void writeAll(int descriptor, const char* data, std::size_t size) noexcept
{
    while (size) {
        const ssize_t written = ::write(descriptor, data, size);
        if (written > 0) {
            data += written;
            size -= static_cast<std::size_t>(written);
        } else if (written < 0 && errno == EINTR) {
            continue;
        } else {
            break;
        }
    }
}

void emit(int descriptor, const char* text) noexcept
{
    writeAll(descriptor, text, textLength(text));
}

void emitUnsigned(int descriptor, std::uint64_t value) noexcept
{
    char digits[32];
    std::size_t used = 0;
    do {
        digits[used++] = static_cast<char>('0' + value % 10);
        value /= 10;
    } while (value && used < sizeof(digits));
    while (used) writeAll(descriptor, &digits[--used], 1);
}

#if !P710_NATIVE_SANITIZER_BUILD
void emitSigned(int descriptor, std::int64_t value) noexcept
{
    if (value < 0) {
        emit(descriptor, "-");
        const auto magnitude = static_cast<std::uint64_t>(-(value + 1)) + 1;
        emitUnsigned(descriptor, magnitude);
    } else {
        emitUnsigned(descriptor, static_cast<std::uint64_t>(value));
    }
}
#endif

void emitHex(int descriptor, std::uintptr_t value) noexcept
{
    static constexpr char hex[] = "0123456789abcdef";
    char output[2 + sizeof(value) * 2];
    output[0] = '0';
    output[1] = 'x';
    for (std::size_t i = 0; i < sizeof(value) * 2; ++i) {
        const unsigned shift = static_cast<unsigned>((sizeof(value) * 2 - i - 1) * 4);
        output[2 + i] = hex[(value >> shift) & 0xf];
    }
    writeAll(descriptor, output, sizeof(output));
}

void emitCleanLine(int descriptor, const char* text, std::size_t limit) noexcept
{
    char output[1024];
    if (limit > sizeof(output) - 1) limit = sizeof(output) - 1;
    std::size_t used = 0;
    for (; text && *text && used < limit; ++text) {
        const unsigned char value = static_cast<unsigned char>(*text);
        output[used++] = value >= 0x20 && value <= 0x7e ? static_cast<char>(value) : ' ';
    }
    output[used++] = '\n';
    writeAll(descriptor, output, used);
}

const char* baseName(const char* path) noexcept
{
    const char* result = path ? path : "";
    for (const char* cursor = result; *cursor; ++cursor)
        if (*cursor == '/' || *cursor == '\\') result = cursor + 1;
    return result;
}

void copyBuildId(const char* source) noexcept
{
    std::size_t used = 0;
    for (; source && *source && used < sizeof(buildId) - 1; ++source) {
        const unsigned char value = static_cast<unsigned char>(*source);
        if (value >= 0x20 && value <= 0x7e) buildId[used++] = static_cast<char>(value);
    }
    buildId[used] = 0;
    if (!used) std::memcpy(buildId, "local", 6);
}

int openReport(char (&path)[sizeof(kReportTemplate)]) noexcept
{
    for (unsigned number = 1; number <= 999; ++number) {
        for (std::size_t i = 0; i < sizeof(kReportTemplate); ++i) path[i] = kReportTemplate[i];
        const std::size_t digit = sizeof(kReportTemplate) - sizeof("000.txt");
        path[digit] = static_cast<char>('0' + number / 100);
        path[digit + 1] = static_cast<char>('0' + (number / 10) % 10);
        path[digit + 2] = static_cast<char>('0' + number % 10);
        const int descriptor = ::openat(reportDirectoryDescriptor,
            path + kReportDirectoryPrefixLength,
            O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC | O_NOFOLLOW, 0600);
        if (descriptor >= 0) return descriptor;
        if (errno != EEXIST) break;
    }
    path[0] = 0;
    return -1;
}

void announceReport(const char* path) noexcept
{
    if (!path || !*path) return;
    emit(STDERR_FILENO, "P710 crash report written: ");
    emit(STDERR_FILENO, path);
    emit(STDERR_FILENO, "\n");
}

struct ModuleWriter {
    char* output;
    std::size_t capacity;
    std::size_t used;
};

std::size_t alignNote(std::size_t value) noexcept
{
    return (value + 3U) & ~std::size_t{3U};
}

void captureExecutableBuildId(const dl_phdr_info* info) noexcept
{
    if (info->dlpi_name && *info->dlpi_name) return;
    static constexpr char hex[] = "0123456789abcdef";
    for (std::size_t index = 0; index < info->dlpi_phnum; ++index) {
        const auto& header = info->dlpi_phdr[index];
        if (header.p_type != PT_NOTE) continue;
        const auto* cursor = reinterpret_cast<const unsigned char*>(
            static_cast<std::uintptr_t>(info->dlpi_addr) + header.p_vaddr);
        const auto* end = cursor + header.p_memsz;
        while (cursor + sizeof(ElfW(Nhdr)) <= end) {
            ElfW(Nhdr) note{};
            std::memcpy(&note, cursor, sizeof(note));
            cursor += sizeof(note);
            const std::size_t nameSize = alignNote(note.n_namesz);
            const std::size_t descriptionSize = alignNote(note.n_descsz);
            if (nameSize > static_cast<std::size_t>(end - cursor)) break;
            const auto* name = cursor;
            cursor += nameSize;
            if (descriptionSize > static_cast<std::size_t>(end - cursor)) break;
            const auto* description = cursor;
            cursor += descriptionSize;
            if (note.n_type != NT_GNU_BUILD_ID || note.n_namesz < 3 ||
                std::memcmp(name, "GNU", 3) != 0 || note.n_descsz == 0 || note.n_descsz > 64)
                continue;
            for (std::size_t byte = 0; byte < note.n_descsz; ++byte) {
                executableBuildId[byte * 2] = hex[description[byte] >> 4];
                executableBuildId[byte * 2 + 1] = hex[description[byte] & 0xf];
            }
            executableBuildId[note.n_descsz * 2] = 0;
            return;
        }
    }
}

void appendModuleText(ModuleWriter& writer, const char* text) noexcept
{
    while (text && *text && writer.used + 1 < writer.capacity)
        writer.output[writer.used++] = *text++;
}

void appendModuleHex(ModuleWriter& writer, std::uintptr_t value) noexcept
{
    static constexpr char hex[] = "0123456789abcdef";
    appendModuleText(writer, "0x");
    for (std::size_t i = 0; i < sizeof(value) * 2 && writer.used + 1 < writer.capacity; ++i) {
        const unsigned shift = static_cast<unsigned>((sizeof(value) * 2 - i - 1) * 4);
        writer.output[writer.used++] = hex[(value >> shift) & 0xf];
    }
}

int collectModule(dl_phdr_info* info, std::size_t, void* opaque) noexcept
{
    auto& writer = *static_cast<ModuleWriter*>(opaque);
    captureExecutableBuildId(info);
    const std::uintptr_t base = static_cast<std::uintptr_t>(info->dlpi_addr);
    std::uintptr_t end = base;
    for (std::size_t i = 0; i < info->dlpi_phnum; ++i) {
        const auto& header = info->dlpi_phdr[i];
        if (header.p_type != PT_LOAD) continue;
        const std::uintptr_t segmentEnd = base + static_cast<std::uintptr_t>(header.p_vaddr) +
            static_cast<std::uintptr_t>(header.p_memsz);
        if (segmentEnd > end) end = segmentEnd;
    }
    appendModuleHex(writer, base);
    appendModuleText(writer, " ");
    appendModuleHex(writer, end);
    appendModuleText(writer, " ");
    appendModuleText(writer, info->dlpi_name && *info->dlpi_name ? baseName(info->dlpi_name) : mainImageName);
    appendModuleText(writer, "\n");
    return writer.used + 64 >= writer.capacity ? 1 : 0;
}

void refreshModuleTable() noexcept
{
    ModuleWriter writer{moduleTable, sizeof(moduleTable), 0};
    dl_iterate_phdr(&collectModule, &writer);
    moduleTableLength = writer.used;
    moduleTable[moduleTableLength] = 0;
}

#if !P710_NATIVE_SANITIZER_BUILD
const char* signalName(int signal) noexcept
{
    switch (signal) {
        case SIGABRT: return "SIGABRT";
        case SIGBUS: return "SIGBUS";
        case SIGFPE: return "SIGFPE";
        case SIGILL: return "SIGILL";
        case SIGSEGV: return "SIGSEGV";
        default: return "SIGNAL";
    }
}

std::uintptr_t instructionPointer(void* context) noexcept
{
#if defined(__x86_64__) && defined(REG_RIP)
    return context ? static_cast<std::uintptr_t>(
        static_cast<ucontext_t*>(context)->uc_mcontext.gregs[REG_RIP]) : 0;
#elif defined(__aarch64__)
    return context ? static_cast<std::uintptr_t>(
        static_cast<ucontext_t*>(context)->uc_mcontext.pc) : 0;
#else
    (void)context;
    return 0;
#endif
}
#endif

void writeCommonHeader(int descriptor, const char* kind) noexcept
{
    emit(descriptor, "*** P710 NATIVE CRASH REPORT ***\n");
    emit(descriptor, "kind "); emitCleanLine(descriptor, kind, 128);
    emit(descriptor, "time_unix "); emitUnsigned(descriptor, static_cast<std::uint64_t>(::time(nullptr))); emit(descriptor, "\n");
    emit(descriptor, "build "); emitCleanLine(descriptor, buildId, sizeof(buildId) - 1);
    emit(descriptor, "elf_build_id "); emitCleanLine(descriptor, executableBuildId, sizeof(executableBuildId) - 1);
    emit(descriptor, "main_image "); emitCleanLine(descriptor, mainImageName, sizeof(mainImageName) - 1);
    emit(descriptor, "pid "); emitUnsigned(descriptor, static_cast<std::uint64_t>(::getpid())); emit(descriptor, "\n");
}

void writeFrames(int descriptor) noexcept
{
    void* frames[kMaxFrames];
    const int count = ::backtrace(frames, static_cast<int>(kMaxFrames));
    emit(descriptor, "frames_raw\n");
    for (int i = 0; i < count; ++i) {
        emit(descriptor, "  ["); emitUnsigned(descriptor, static_cast<unsigned>(i)); emit(descriptor, "] ");
        emitHex(descriptor, reinterpret_cast<std::uintptr_t>(frames[i])); emit(descriptor, "\n");
    }
    emit(descriptor, "frames_symbols\n");
    ::backtrace_symbols_fd(frames, count, descriptor);
    emit(descriptor, "modules_base_end_name\n");
    writeAll(descriptor, moduleTable, moduleTableLength);
}

void writeControlledReport(const char* kind, const char* detail,
    unsigned line, const char* function, const char* sourceFile) noexcept
{
    if (reportDirectoryDescriptor < 0) return;
    char path[sizeof(kReportTemplate)];
    const int descriptor = openReport(path);
    if (descriptor < 0) return;
    writeCommonHeader(descriptor, kind);
    if (detail && *detail) { emit(descriptor, "detail "); emitCleanLine(descriptor, detail, 1023); }
    if (sourceFile && *sourceFile) {
        emit(descriptor, "source "); emitCleanLine(descriptor, baseName(sourceFile), 255);
        emit(descriptor, "line "); emitUnsigned(descriptor, line); emit(descriptor, "\n");
    }
    if (function && *function) { emit(descriptor, "function "); emitCleanLine(descriptor, function, 255); }
    writeFrames(descriptor);
    ::fsync(descriptor);
    ::close(descriptor);
    announceReport(path);
}

#if !P710_NATIVE_SANITIZER_BUILD
bool isSynchronousFault(int signal, const siginfo_t* info) noexcept
{
    if (!info) return false;
    switch (signal) {
        case SIGSEGV:
            return info->si_code == SEGV_MAPERR || info->si_code == SEGV_ACCERR;
        case SIGBUS:
            return info->si_code == BUS_ADRALN || info->si_code == BUS_ADRERR;
        case SIGILL:
            return info->si_code == ILL_ILLOPC || info->si_code == ILL_ILLOPN ||
                info->si_code == ILL_ILLADR || info->si_code == ILL_ILLTRP ||
                info->si_code == ILL_PRVOPC || info->si_code == ILL_PRVREG ||
                info->si_code == ILL_COPROC || info->si_code == ILL_BADSTK;
        case SIGFPE:
            return info->si_code == FPE_INTDIV || info->si_code == FPE_INTOVF ||
                info->si_code == FPE_FLTDIV || info->si_code == FPE_FLTOVF ||
                info->si_code == FPE_FLTUND || info->si_code == FPE_FLTRES ||
                info->si_code == FPE_FLTINV || info->si_code == FPE_FLTSUB;
        default:
            return false;
    }
}

void restoreDefaultAction(int signal) noexcept
{
    struct sigaction action{};
    action.sa_handler = SIG_DFL;
    sigemptyset(&action.sa_mask);
    sigaction(signal, &action, nullptr);
}

[[noreturn]] void reraiseOnCurrentThread(int signal) noexcept
{
    sigset_t unblocked;
    sigemptyset(&unblocked);
    sigaddset(&unblocked, signal);
    sigprocmask(SIG_UNBLOCK, &unblocked, nullptr);
    raise(signal);
    _exit(128 + signal);
}

void fatalSignalHandler(int signal, siginfo_t* info, void* context) noexcept
{
    const bool synchronous = isSynchronousFault(signal, info);
    if ((signal == SIGABRT && skipNextAbort) || handlingSignal) {
        restoreDefaultAction(signal);
        if (synchronous) return;
        reraiseOnCurrentThread(signal);
    }
    handlingSignal = 1;
    char path[sizeof(kReportTemplate)];
    const int descriptor = openReport(path);
    if (descriptor >= 0) {
        emit(descriptor, "*** P710 NATIVE CRASH REPORT ***\n");
        emit(descriptor, "kind fatal signal\n");
        emit(descriptor, "build "); emitCleanLine(descriptor, buildId, sizeof(buildId) - 1);
        emit(descriptor, "elf_build_id "); emitCleanLine(descriptor, executableBuildId, sizeof(executableBuildId) - 1);
        emit(descriptor, "main_image "); emitCleanLine(descriptor, mainImageName, sizeof(mainImageName) - 1);
        emit(descriptor, "pid "); emitUnsigned(descriptor, static_cast<std::uint64_t>(::getpid())); emit(descriptor, "\n");
        emit(descriptor, "signal "); emit(descriptor, signalName(signal)); emit(descriptor, " (");
        emitUnsigned(descriptor, static_cast<unsigned>(signal)); emit(descriptor, ")\n");
        emit(descriptor, "signal_code "); emitSigned(descriptor,
            info ? static_cast<std::int64_t>(info->si_code) : 0); emit(descriptor, "\n");
        emit(descriptor, "signal_address ");
        if (synchronous)
            emitHex(descriptor, reinterpret_cast<std::uintptr_t>(info->si_addr));
        else
            emit(descriptor, "unavailable");
        emit(descriptor, "\n");
        emit(descriptor, "instruction_pointer "); emitHex(descriptor, instructionPointer(context)); emit(descriptor, "\n");
        emit(descriptor, "modules_base_end_name\n");
        writeAll(descriptor, moduleTable, moduleTableLength);
        ::close(descriptor);
    }
    restoreDefaultAction(signal);
    if (synchronous) return;
    reraiseOnCurrentThread(signal);
}

void terminateHandler() noexcept
{
    const char* detail = "std::terminate without an active exception";
    try {
        const auto exception = std::current_exception();
        if (exception) std::rethrow_exception(exception);
    } catch (const std::exception& error) {
        detail = error.what();
    } catch (...) {
        detail = "uncaught non-standard C++ exception";
    }
    sgp::WriteNativeRuntimeFailure("std::terminate", detail);
    skipNextAbort = 1;
    std::abort();
}
#endif

} // namespace

namespace sgp {

void InstallNativeCrashReporter(const char* id) noexcept
{
    copyBuildId(id);
    char executable[1024];
    const ssize_t executableLength = ::readlink("/proc/self/exe", executable, sizeof(executable) - 1);
    if (executableLength > 0) {
        executable[executableLength] = 0;
        const char* name = baseName(executable);
        std::size_t used = 0;
        while (name[used] && used < sizeof(mainImageName) - 1) {
            mainImageName[used] = name[used];
            ++used;
        }
        mainImageName[used] = 0;
    }
    // Prime libgcc's unwinder before a damaged process needs it. glibc documents
    // that the first backtrace call may load libgcc and allocate.
    void* warmup[1];
    (void)::backtrace(warmup, 1);
    ::mkdir(kReportDirectory, 0700);
    reportDirectoryDescriptor = ::open(kReportDirectory,
        O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (reportDirectoryDescriptor < 0) return;
    refreshModuleTable();
#if !P710_NATIVE_SANITIZER_BUILD
    std::set_terminate(&terminateHandler);
    stack_t stack{};
    stack.ss_sp = alternateSignalStack.data();
    stack.ss_size = alternateSignalStack.size();
    stack.ss_flags = 0;
    if (sigaltstack(&stack, nullptr) != 0) return;

    struct sigaction action{};
    sigemptyset(&action.sa_mask);
    action.sa_sigaction = &fatalSignalHandler;
    action.sa_flags = static_cast<int>(SA_SIGINFO | SA_ONSTACK | SA_RESETHAND);
    const int fatalSignals[] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT};
    for (const int signal : fatalSignals)
        if (sigaction(signal, &action, nullptr) != 0) return;
    signalsInstalled = true;
#endif
}

void WriteNativeRuntimeFailure(const char* kind, const char* detail) noexcept
{
    writeControlledReport(kind ? kind : "runtime failure", detail, 0, nullptr, nullptr);
}

void WriteNativeAssertionFailure(unsigned line, const char* function,
    const char* sourceFile, const char* message) noexcept
{
    writeControlledReport("assertion failure", message, line, function, sourceFile);
#if !P710_NATIVE_SANITIZER_BUILD
    skipNextAbort = 1;
#endif
}

bool NativeCrashSignalHandlersEnabled() noexcept
{
    return signalsInstalled;
}

} // namespace sgp
