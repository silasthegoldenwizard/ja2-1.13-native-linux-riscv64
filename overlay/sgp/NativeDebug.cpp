/*
 * P710 native snapshot update: sgp/NativeDebug.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native implementation of the original SGP debug interface. No Win32 shim.
#include "native_debug.h"
#include <vfs/Aspects/vfs_settings.h>
#include <vfs/Core/vfs_debug.h>
#include <vfs/Core/vfs_string.h>
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <sstream>

BOOLEAN gfRecordToFile = FALSE;
BOOLEAN gfRecordToDebugger = TRUE;
UINT32 guiProfileStart = 0, guiExecutions = 0, guiProfileTime = 0;
INT32 giProfileCount = 0;
CHAR8 gubAssertString[512] = {};
bool gfDebugTopics[MAX_TOPICS_ALLOTED] = {};
namespace {
std::mutex logMutex;
FILE* logFile = nullptr;
std::array<unsigned, MAX_TOPICS_ALLOTED> levels{};
const auto clockStart = std::chrono::steady_clock::now();
constexpr const char* logName = "debug.txt";
const char* safe(const char* text) { return text ? text : "(null)"; }
void fileError(const char* action)
{
    const int error = errno;
    std::fprintf(stderr, "SGP debug: cannot %s %s: %s\n", action, logName, std::strerror(error));
    std::fflush(stderr);
}
// Caller holds logMutex. Message bytes are always data, never printf formats.
void emit(const std::string& message, bool forceStderr = false)
{
    if (gfRecordToDebugger || forceStderr) {
        std::fwrite(message.data(), 1, message.size(), stderr);
        std::fputc('\n', stderr);
        std::fflush(stderr);
    }
    if (!gfRecordToFile) return;
    if (!logFile) {
        logFile = std::fopen(logName, "a");
        if (!logFile) { fileError("open"); return; }
    }
    const bool writeFailed = std::fwrite(message.data(), 1, message.size(), logFile) != message.size();
    const bool newlineFailed = std::fputc('\n', logFile) == EOF;
    const bool flushFailed = std::fflush(logFile) != 0;
    if (writeFailed || newlineFailed || flushFailed) {
        fileError("write");
        std::fclose(logFile);
        logFile = nullptr;
        gfRecordToFile = FALSE;
    }
}
void closeLog()
{
    if (logFile && std::fclose(logFile) != 0) fileError("close");
    logFile = nullptr;
}
std::wstring widen(const std::string& text)
{
    if (vfs::Settings::getUseUnicode()) return vfs::String::as_utf16(text);
    std::wstring result;
    vfs::String::widen(text, result);
    return result;
}
std::string narrow(const std::wstring& text)
{
    if (vfs::Settings::getUseUnicode()) return vfs::String::as_utf8(text);
    std::string result;
    vfs::String::narrow(text, result);
    return result;
}
void append(sgp::Exception& target, const sgp::WString& message,
            int line, const char* function, const char* file)
{
    target._msg.push_back({line, function ? function : "", file ? file : "", message.str});
}
void import(sgp::Exception& target, const vfs::Exception& ex)
{
    for (const auto& frame : ex.m_CallStack)
        target._msg.push_back({frame.line, frame.function.utf8(), frame.file.utf8(), frame.message.c_wcs()});
}
}

STR8 String(const char* format, ...)
{
    thread_local std::array<std::array<char, 512>, 8> buffers{};
    thread_local unsigned index = 0;
    auto& output = buffers[index];
    index = (index + 1) % buffers.size();
    if (!format) {
        output[0] = '\0';
        _DebugMessage("String received a null format", 0, "NativeDebug.cpp");
        return output.data();
    }
    va_list args;
    va_start(args, format);
    const int length = std::vsnprintf(output.data(), output.size(), format, args);
    va_end(args);
    if (length < 0) {
        output[0] = '\0';
        _DebugMessage("String formatting failed", 0, "NativeDebug.cpp");
    } else if (static_cast<std::size_t>(length) >= output.size()) {
        _DebugMessage("String output truncated to 511 bytes", 0, "NativeDebug.cpp");
    }
    return output.data();
}
UINT32 NativeDebugMilliseconds()
{
    return static_cast<UINT32>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - clockStart).count());
}
void NativeDebugProfileReport()
{
    guiProfileTime = NativeDebugMilliseconds() - guiProfileStart;
    std::ostringstream out;
    out << "PROFILE REPORT: " << guiExecutions << " executions took " << guiProfileTime << "ms";
    if (guiExecutions) out << ", average " << static_cast<double>(guiProfileTime) / guiExecutions << "ms";
    std::lock_guard<std::mutex> lock(logMutex);
    emit(out.str());
}
bool DbgGetLogFileName(char* name)
{
    if (!name) return false;
    std::strcpy(name, logName);
    return true;
}
bool DbgInitialize()
{
    std::lock_guard<std::mutex> lock(logMutex);
    closeLog();
    std::fill(std::begin(gfDebugTopics), std::end(gfDebugTopics), false);
    levels.fill(DBG_LEVEL_3);
    gubAssertString[0] = '\0';
    gfRecordToDebugger = TRUE;
    gfRecordToFile = TRUE;
    logFile = std::fopen(logName, "w");
    if (!logFile) { fileError("open"); gfRecordToFile = FALSE; return false; }
    return true;
}
void DbgShutdown()
{
    std::lock_guard<std::mutex> lock(logMutex);
    emit("SGP Going Down");
    closeLog();
    gfRecordToFile = FALSE;
    std::fill(std::begin(gfDebugTopics), std::end(gfDebugTopics), false);
}
void RemoveDebugText()
{
    std::lock_guard<std::mutex> lock(logMutex);
    closeLog();
    if (std::remove(logName) != 0 && errno != ENOENT) fileError("remove");
}
void DbgClearAllTopics()
{
    std::lock_guard<std::mutex> lock(logMutex);
    std::fill(std::begin(gfDebugTopics), std::end(gfDebugTopics), false);
    levels.fill(DBG_LEVEL_3);
}
bool DbgSetDebugLevel(unsigned topic, unsigned level)
{
    std::lock_guard<std::mutex> lock(logMutex);
    if (topic >= MAX_TOPICS_ALLOTED || level > DBG_LEVEL_3) return false;
    levels[topic] = level;
    return true;
}
void DbgTopicRegistration(unsigned command, unsigned topic, const char* message)
{
    std::lock_guard<std::mutex> lock(logMutex);
    if (topic >= MAX_TOPICS_ALLOTED || (command != TOPIC_REGISTER && command != TOPIC_UNREGISTER)) {
        emit("SGP debug: invalid topic registration", true);
        return;
    }
    if (command == TOPIC_REGISTER) {
        gfDebugTopics[topic] = true;
        levels[topic] = DBG_LEVEL_3;
        emit(safe(message));
    } else {
        if (gfDebugTopics[topic]) emit(safe(message));
        gfDebugTopics[topic] = false;
    }
}
void DbgMessageReal(unsigned topic, unsigned command, unsigned level, const char* message)
{
    std::lock_guard<std::mutex> lock(logMutex);
    if (command == CLIENT_SHUTDOWN) { emit(safe(message)); return; }
    if (command != TOPIC_MESSAGE || topic >= MAX_TOPICS_ALLOTED) {
        emit("SGP debug: invalid message command or topic", true);
        return;
    }
    if (gfDebugTopics[topic] && level <= levels[topic]) emit(safe(message));
}
void _DebugMessage(const char* message, unsigned line, const char* file)
{
    std::ostringstream out;
    out << "{ " << NativeDebugMilliseconds() << " } " << safe(message)
        << " [Line " << line << " in " << safe(file) << "]";
    std::lock_guard<std::mutex> lock(logMutex);
    emit(out.str());
}
void _DebugRecordToFile(BOOLEAN state)
{
    std::lock_guard<std::mutex> lock(logMutex);
    gfRecordToFile = state;
    if (!state) closeLog();
}
void _DebugRecordToDebugger(BOOLEAN state)
{
    std::lock_guard<std::mutex> lock(logMutex);
    gfRecordToDebugger = state;
}
[[noreturn]] void _FailMessage(const char* message, unsigned line, const char* function, const char* file)
{
    // Keep the fatal path independent of locks, allocation and engine state.
    std::snprintf(gubAssertString, sizeof(gubAssertString), "%s", message ? message : "");
    std::fprintf(stderr, "Assertion Failure [Line %u in function %s in file %s]: %s\n",
        line, safe(function), safe(file), safe(message));
    std::fflush(stderr);
    std::abort();
}
[[noreturn]] void NativeDebugBreakpoint()
{
    std::raise(SIGTRAP);
    std::abort(); // A debugger may continue past SIGTRAP; never resume as success.
}
sgp::WString::WString(const std::string& text) : str(widen(text)) {}
sgp::WString::WString(const char* text) : str(widen(text ? text : "")) {}
sgp::Exception::Exception(const WString& message SGP_CALLER_LOCATION_IMPL)
{
    append(*this, message, line, function, file);
}
sgp::Exception::Exception(const WString& message, Exception& ex SGP_CALLER_LOCATION_IMPL) : _msg(ex._msg)
{
    append(*this, message, line, function, file);
}
sgp::Exception::Exception(const WString& message, vfs::Exception& ex SGP_CALLER_LOCATION_IMPL)
{
    import(*this, ex);
    append(*this, message, line, function, file);
}
sgp::Exception::Exception(const WString& message, std::exception& ex SGP_CALLER_LOCATION_IMPL)
{
    if (auto* nested = dynamic_cast<sgp::Exception*>(&ex)) _msg = nested->_msg;
    else if (auto* nested = dynamic_cast<vfs::Exception*>(&ex)) import(*this, *nested);
    else _msg.push_back({-1, "", "", widen(ex.what())});
    append(*this, message, line, function, file);
}
const char* sgp::Exception::what() const noexcept
{
    try {
        std::ostringstream out;
        for (auto it = _msg.rbegin(); it != _msg.rend(); ++it)
            out << "File     :  " << it->file << "\nLine     :  " << it->line
                << "\nLocation :  " << it->function << "\n  " << narrow(it->msg) << '\n';
        rendered_ = out.str();
        return rendered_.c_str();
    } catch (...) {
        return "SGP exception (could not render exception details)";
    }
}
[[noreturn]] void _ExceptionMessage(sgp::Exception& ex)
{
    std::fprintf(stderr, "%s", ex.what());
    if (!ex._msg.empty()) {
        const auto& frame = ex._msg.front();
        try {
            const auto message = narrow(frame.msg);
            _FailMessage(message.c_str(), static_cast<unsigned>(frame.line), frame.function.c_str(), frame.file.c_str());
        } catch (...) {
            _FailMessage("SGP exception (message conversion failed)", static_cast<unsigned>(frame.line),
                frame.function.c_str(), frame.file.c_str());
        }
    }
    _FailMessage(ex.what(), 0, nullptr, nullptr);
}
[[noreturn]] void _ExceptionMessage(vfs::Exception& ex)
{
    std::fprintf(stderr, "%s", ex.what());
    if (!ex.m_CallStack.empty()) {
        const auto& frame = ex.m_CallStack.front();
        try {
            const auto message = frame.message.utf8(), function = frame.function.utf8(), file = frame.file.utf8();
            _FailMessage(message.c_str(), static_cast<unsigned>(frame.line), function.c_str(), file.c_str());
        } catch (...) {
            _FailMessage("VFS exception (message conversion failed)", static_cast<unsigned>(frame.line), nullptr, nullptr);
        }
    }
    _FailMessage(ex.what(), 0, nullptr, nullptr);
}
