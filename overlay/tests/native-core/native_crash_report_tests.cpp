/*
 * P710 native snapshot update: tests/native-core/native_crash_report_tests.cpp
 * Native Linux source/build/test adaptations captured in the November 20-21 save-load and crash-report checkpoint.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-21.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "native_crash_report.h"

#include <array>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {

unsigned checks = 0;

void check(bool condition, const char* reason)
{
    ++checks;
    if (!condition) throw std::runtime_error(reason);
}

std::string readFile(const fs::path& path)
{
    std::ifstream input(path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

struct TestDirectory {
    fs::path previous = fs::current_path();
    fs::path path;

    TestDirectory()
    {
        char pattern[] = "/tmp/p710-native-crash-report-XXXXXX";
        char* result = mkdtemp(pattern);
        if (!result) throw std::runtime_error("temporary test directory");
        path = result;
        fs::current_path(path);
    }

    ~TestDirectory()
    {
        fs::current_path(previous);
        fs::remove_all(path);
    }
};

void waitForSignal(pid_t child, int expected, const char* reason)
{
    int status = 0;
    check(waitpid(child, &status, 0) == child, "crash-report child reaped");
    check(WIFSIGNALED(status) && WTERMSIG(status) == expected, reason);
}

void controlledReports()
{
    sgp::WriteNativeRuntimeFailure("test runtime failure", "first line\nforged field");
    const fs::path runtime = "CrashReports/crash_report_001.txt";
    check(fs::is_regular_file(runtime), "runtime report created in profile-local directory");
    const std::string runtimeText = readFile(runtime);
    check(runtimeText.find("build native-test-build") != std::string::npos,
          "report identifies exact native build");
    check(runtimeText.find("elf_build_id ") != std::string::npos &&
          runtimeText.find("elf_build_id unavailable") == std::string::npos,
          "report identifies the exact ELF executable");
    check(runtimeText.find("kind test runtime failure") != std::string::npos,
          "runtime failure kind recorded");
    check(runtimeText.find("detail first line forged field") != std::string::npos,
          "runtime detail is one sanitized line");
    check(runtimeText.find("frames_raw\n") != std::string::npos &&
          runtimeText.find("frames_symbols\n") != std::string::npos &&
          runtimeText.find("modules_base_end_name\n") != std::string::npos,
          "report retains stack and module information for symbolization");
    check(runtimeText.find("p710_native_crash_report_tests") != std::string::npos,
          "module table identifies the actual executable");

    sgp::WriteNativeAssertionFailure(417, "assertFunction",
        "/private/build/tree/source.cpp", "bad\nstate");
    const fs::path assertion = "CrashReports/crash_report_002.txt";
    check(fs::is_regular_file(assertion), "assertion creates a separate numbered report");
    const std::string assertionText = readFile(assertion);
    check(assertionText.find("kind assertion failure") != std::string::npos &&
          assertionText.find("source source.cpp") != std::string::npos &&
          assertionText.find("/private/build/tree") == std::string::npos &&
          assertionText.find("line 417") != std::string::npos &&
          assertionText.find("function assertFunction") != std::string::npos,
          "assert report keeps useful source data without build-directory disclosure");
}

void terminateReport()
{
    const pid_t child = fork();
    check(child >= 0, "terminate child fork");
    if (child == 0) {
        const rlimit noCore{0, 0};
        setrlimit(RLIMIT_CORE, &noCore);
        std::terminate();
    }
    waitForSignal(child, SIGABRT, "terminate handler preserves SIGABRT death");
    const std::string text = readFile("CrashReports/crash_report_003.txt");
    check(text.find("kind std::terminate") != std::string::npos,
          "terminate handler leaves a readable local report");
}

void fatalSignalReport()
{
    if (!sgp::NativeCrashSignalHandlersEnabled()) return;
    const pid_t child = fork();
    check(child >= 0, "fatal-signal child fork");
    if (child == 0) {
        const rlimit noCore{0, 0};
        setrlimit(RLIMIT_CORE, &noCore);
        raise(SIGSEGV);
        _exit(91);
    }
    waitForSignal(child, SIGSEGV, "signal handler preserves original fatal signal");
    const std::string text = readFile("CrashReports/crash_report_004.txt");
    check(text.find("kind fatal signal") != std::string::npos &&
          text.find("signal SIGSEGV") != std::string::npos &&
          text.find("signal_address unavailable") != std::string::npos &&
          text.find("instruction_pointer 0x") != std::string::npos &&
          text.find("modules_base_end_name\n") != std::string::npos &&
          text.find("frames_raw\n") == std::string::npos,
          "fatal signal report records minimal machine context without unsafe unwinding");
}

void synchronousFaultReport()
{
    if (!sgp::NativeCrashSignalHandlersEnabled()) return;
    const pid_t child = fork();
    check(child >= 0, "synchronous-fault child fork");
    if (child == 0) {
        const rlimit noCore{0, 0};
        setrlimit(RLIMIT_CORE, &noCore);
        void* page = mmap(nullptr, 4096, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (page == MAP_FAILED) _exit(92);
        *static_cast<volatile unsigned char*>(page) = 1;
        _exit(93);
    }
    waitForSignal(child, SIGSEGV, "hardware fault preserves original SIGSEGV death");
    const std::string text = readFile("CrashReports/crash_report_005.txt");
    check(text.find("kind fatal signal") != std::string::npos &&
          text.find("signal SIGSEGV") != std::string::npos &&
          text.find("signal_address 0x") != std::string::npos &&
          text.find("signal_address unavailable") == std::string::npos &&
          text.find("instruction_pointer 0x") != std::string::npos,
          "hardware fault report retains the original address and program counter");
}

} // namespace

int main()
{
    try {
        TestDirectory directory;
        sgp::WriteNativeRuntimeFailure("pre-install", "must not escape the selected profile");
        check(!fs::exists("CrashReports"), "reporting is disabled before profile installation");
        fs::create_directory("outside");
        fs::create_directory_symlink("outside", "CrashReports");
        sgp::InstallNativeCrashReporter("native-test-build");
        sgp::WriteNativeRuntimeFailure("symlink test", "must not follow profile links");
        check(fs::is_empty("outside"), "report directory symlinks are rejected");
        fs::remove("CrashReports");
        sgp::InstallNativeCrashReporter("native-test-build");
        check(fs::is_directory("CrashReports"), "install creates the profile-local report directory");
        controlledReports();
        if (sgp::NativeCrashSignalHandlersEnabled()) {
            terminateReport();
            fatalSignalReport();
            synchronousFaultReport();
        }
        std::cout << "native_crash_report_tests: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "native_crash_report_tests failed: " << error.what() << '\n';
        return 1;
    }
}
