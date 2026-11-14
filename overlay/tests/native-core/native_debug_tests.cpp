/*
 * P710 native snapshot update: tests/native-core/native_debug_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Source-only native diagnostic contract tests; no game assets or SDL required.
#ifndef FORCE_ASSERTS_ON
#define FORCE_ASSERTS_ON
#endif
#ifndef SGP_DEBUG
#define SGP_DEBUG
#endif
#include "DEBUG.H"
#include <vfs/Core/vfs_debug.h>
#include <array>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <thread>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {
unsigned checks = 0;
void check(bool condition, const char* reason)
{
    ++checks;
    if (!condition) throw std::runtime_error(reason);
}
std::string readLog()
{
    std::ifstream input("debug.txt");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
struct TestDirectory {
    std::filesystem::path previous = std::filesystem::current_path();
    std::filesystem::path path;
    TestDirectory() {
        char pattern[] = "/tmp/p710-native-debug-XXXXXX";
        char* result = mkdtemp(pattern);
        if (!result) throw std::runtime_error("temporary test directory");
        path = result;
        std::filesystem::current_path(path);
    }
    ~TestDirectory() {
        DbgShutdown();
        std::filesystem::current_path(previous);
        std::filesystem::remove_all(path);
    }
};
void strings()
{
    check(std::strcmp(String("%s %d %.1f %%", "value", 17, 2.5), "value 17 2.5 %") == 0,
          "printf arguments and literal percent");
    std::array<char*, 8> slots{};
    for (unsigned i = 0; i < slots.size(); ++i) slots[i] = String("slot %u", i);
    for (unsigned i = 0; i < slots.size(); ++i)
        check(std::string(slots[i]) == "slot " + std::to_string(i), "eight String slots remain live");
    char* reused = String("reused");
    check(reused == slots[0] && std::strcmp(slots[0], "reused") == 0, "ninth call reuses first slot");
    const std::string longMessage(4096, 'x');
    const char* limited = String("%s", longMessage.c_str());
    check(std::strlen(limited) == 511 && std::string(limited) == longMessage.substr(0, 511),
          "long formatting is bounded and null terminated");
    check(readLog().find("truncated") != std::string::npos, "String truncation is reported");
    check(String(nullptr)[0] == '\0', "null format has defined empty result");
    char* mainString = String("main thread");
    std::thread worker([] { for (unsigned i = 0; i < 16; ++i) String("worker %u", i); });
    worker.join();
    check(std::strcmp(mainString, "main thread") == 0, "thread has independent String ring");
}
void topics()
{
    check(DbgInitialize(), "debug initialization opens a fresh log");
    _DebugRecordToDebugger(FALSE);
    check(!gfDebugTopics[TOPIC_GAME], "initial topics cleared");
    DbgMessage(TOPIC_GAME, DBG_LEVEL_1, "unregistered-hidden");
    RegisterJA2DebugTopic(TOPIC_GAME, "register 100% %s");
    check(gfDebugTopics[TOPIC_GAME], "topic registered");
    DbgMessage(TOPIC_GAME, DBG_LEVEL_1, "visible %s %n");
    check(DbgSetDebugLevel(TOPIC_GAME, DBG_LEVEL_1), "set verbosity");
    DebugMsg(TOPIC_GAME, DBG_LEVEL_2, "verbosity-hidden");
    DebugMsg(TOPIC_GAME, DBG_LEVEL_1, "verbosity-visible");
    check(!DbgSetDebugLevel(MAX_TOPICS_ALLOTED, DBG_LEVEL_1), "reject invalid topic level");
    check(!DbgSetDebugLevel(TOPIC_GAME, DBG_LEVEL_3 + 1), "reject invalid verbosity");
    DbgTopicRegistration(TOPIC_REGISTER, MAX_TOPICS_ALLOTED, "invalid");
    DbgMessageReal(INVALID_TOPIC, TOPIC_MESSAGE, DBG_LEVEL_1, "invalid");
    check(gfDebugTopics[TOPIC_GAME], "invalid identifiers do not alter valid topics");
    UnRegisterDebugTopic(TOPIC_GAME, "unregister");
    check(!gfDebugTopics[TOPIC_GAME], "topic unregistered");
    DbgMessage(TOPIC_GAME, DBG_LEVEL_1, "after-unregister-hidden");
    ErrorMsg("error 100% %n %s");
    const std::string content = readLog();
    check(content.find("register 100% %s") != std::string::npos && content.find("visible %s %n") != std::string::npos,
          "log messages preserve literal percent without interpreting formats");
    check(content.find("verbosity-visible") != std::string::npos && content.find("verbosity-hidden") == std::string::npos,
          "verbosity filter applied");
    check(content.find("unregistered-hidden") == std::string::npos && content.find("after-unregister-hidden") == std::string::npos,
          "registration lifecycle filters messages");
    check(content.find("error 100% %n %s") != std::string::npos && content.find("native_debug_tests.cpp") != std::string::npos,
          "error includes source location and literal message");
    RegisterDebugTopic(0, "zero topic");
    check(gfDebugTopics[0], "topic zero is a valid bounded index");
    ClearAllDebugTopics();
    check(!gfDebugTopics[0], "clear topics");
    _DebugRecordToFile(FALSE);
    ErrorMsg("file-disabled-hidden");
    check(readLog().find("file-disabled-hidden") == std::string::npos, "file recording switch");
    _DebugRecordToFile(TRUE);
    ErrorMsg("file-reenabled");
    check(readLog().find("file-reenabled") != std::string::npos, "file recording resumes");
    char name[512]{};
    check(DbgGetLogFileName(name) && std::strcmp(name, "debug.txt") == 0, "documented native log location");
    check(!DbgGetLogFileName(nullptr), "null log filename rejected");
    DbgShutdown();
    check(!gfRecordToFile && !gfDebugTopics[TOPIC_GAME], "shutdown closes recording and clears topics");
    check(readLog().find("SGP Going Down") != std::string::npos, "shutdown is recorded");
    check(DbgInitialize() && readLog().empty(), "reinitialization starts fresh log");
    _DebugRecordToDebugger(FALSE);
    DbgShutdown();
    RemoveDebugText();
    check(!std::filesystem::exists("debug.txt"), "remove debug text closes and removes log");
    std::filesystem::create_directory("debug.txt");
    check(!DbgInitialize() && !gfRecordToFile, "unwritable log reports initialization failure");
    DbgShutdown();
    std::filesystem::remove("debug.txt");
    check(DbgInitialize(), "initialization recovers after file failure");
    _DebugRecordToDebugger(FALSE);
}
void assertDeath(void (*action)(), const char* expected)
{
    int descriptors[2];
    check(pipe(descriptors) == 0, "assert capture pipe");
    const pid_t pid = fork();
    check(pid >= 0, "assert child fork");
    if (pid == 0) {
        close(descriptors[0]);
        if (dup2(descriptors[1], STDERR_FILENO) < 0) _exit(90);
        close(descriptors[1]);
        const rlimit noCore{0, 0};
        setrlimit(RLIMIT_CORE, &noCore);
        gfRecordToDebugger = FALSE;
        gfRecordToFile = FALSE;
        action();
        _exit(91);
    }
    close(descriptors[1]);
    std::string output;
    std::array<char, 1024> chunk{};
    ssize_t length;
    while ((length = read(descriptors[0], chunk.data(), chunk.size())) > 0)
        output.append(chunk.data(), static_cast<std::size_t>(length));
    close(descriptors[0]);
    int status = 0;
    check(waitpid(pid, &status, 0) == pid, "assert child reaped");
    check(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT, "enabled assertion aborts deterministically");
    check(output.find("Assertion Failure") != std::string::npos && output.find(expected) != std::string::npos,
          "assertion emits actual source/message even when recording disabled");
}
void assertions()
{
    int evaluated = 0;
    Assert(++evaluated == 1);
    AssertMsg(true, "not used");
    AssertT(true); AssertF(false); AssertNotNIL(&evaluated);
    AssertEQ(2,2); AssertNE(2,3); AssertLT(2,3); AssertLE(2,2); AssertGT(3,2); AssertGE(2,2);
    check(evaluated == 1, "passing assertions evaluate condition once");
    assertDeath([] { AssertMsg(false, "assert 100% %s %n"); }, "assert 100% %s %n");
    assertDeath([] { AssertF(true); }, "true is true but should be false");
    assertDeath([] { AssertEQ(2, 3); }, "2 != 3");
}
void exceptions()
{
    sgp::Exception first("inner 100%", 17, "innerFunction", "inner.cpp");
    sgp::Exception second("other exception", 2, "other", "other.cpp");
    const char* firstText = first.what();
    second.what();
    check(std::string(firstText).find("inner 100%") != std::string::npos,
          "exception text belongs to each object, not a shared static buffer");
    sgp::Exception nested("outer", first, 23, "outerFunction", "outer.cpp");
    check(nested._msg.size() == 2 && nested._msg.front().line == 17 && nested._msg.back().line == 23,
          "SGP exception preserves inner and outer locations");
    const std::string text = nested.what();
    check(text.find("outer") < text.find("innerFunction"), "exception rendering is outer to inner");
    std::runtime_error standard("standard details");
    sgp::Exception wrapped("wrapper", standard, 5, "wrap", "wrap.cpp");
    check(std::string(wrapped.what()).find("standard details") != std::string::npos, "standard exception details retained");
    vfs::Exception vfsError("vfs detail", "vfsFunction", 29, "vfs.cpp");
    sgp::Exception vfsWrapped("vfs wrapper", vfsError, 31, "sgpFunction", "sgp.cpp");
    check(vfsWrapped._msg.size() == 2 && vfsWrapped._msg.front().line == 29,
          "VFS exception bridge retains source frames");
    assertDeath([] {
        sgp::Exception error("inner failure", 37, "failingFunction", "failing.cpp");
        sgp::Exception wrapper("outer failure", error, 39, "wrapper", "wrapper.cpp");
        _ExceptionMessage(wrapper);
    }, "Line 37 in function failingFunction in file failing.cpp");
}
}
int main()
{
    try {
        TestDirectory directory;
        check(DbgInitialize(), "initial log initialization");
        _DebugRecordToDebugger(FALSE);
        strings(); topics(); assertions(); exceptions();
        PROFILE(3) { /* Exercise the original profile loop contract. */ }
        PROFILE_REPORT();
        check(giProfileCount == 3 && guiExecutions == 3, "profile loop uses native monotonic timer");
        std::cout << "native_debug_tests: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "native_debug_tests failed: " << error.what() << '\n';
        return 1;
    }
}
