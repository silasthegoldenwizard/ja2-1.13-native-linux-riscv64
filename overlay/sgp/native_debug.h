/*
 * P710 native snapshot update: sgp/native_debug.h
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_NATIVE_DEBUG_H
#define SGP_NATIVE_DEBUG_H

#include "core_types.h"
#include "TopicOps.h"
#include "TopicIDs.h"
#include <exception>
#include <list>
#include <string>

#define MAX_TOPICS_ALLOTED 1024
extern BOOLEAN gfRecordToFile, gfRecordToDebugger;
extern UINT32 guiProfileStart, guiExecutions, guiProfileTime;
extern INT32 giProfileCount;
extern CHAR8 gubAssertString[512];
extern bool gfDebugTopics[MAX_TOPICS_ALLOTED];

// Eight per-thread 512-byte slots, valid until that thread reuses the slot.
// printf formats follow the host C runtime; truncation is reported to stderr.
#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 1, 2)))
#endif
STR8 String(const char* format, ...);

bool DbgInitialize(); // Starts a fresh ./debug.txt; failure is reported to stderr.
void DbgShutdown();
bool DbgGetLogFileName(char* name); // Legacy caller supplies at least 512 bytes.
void RemoveDebugText();
void DbgMessageReal(unsigned topic, unsigned command, unsigned level, const char* message);
bool DbgSetDebugLevel(unsigned topic, unsigned level);
void DbgTopicRegistration(unsigned command, unsigned topic, const char* message);
void DbgClearAllTopics();
void _DebugMessage(const char* message, unsigned line, const char* file);
void _DebugRecordToFile(BOOLEAN state);
void _DebugRecordToDebugger(BOOLEAN state);
UINT32 NativeDebugMilliseconds();
void NativeDebugProfileReport();
[[noreturn]] void NativeDebugBreakpoint();
// Native failure reports and aborts. It does not attempt an engine save/UI loop.
[[noreturn]] void _FailMessage(const char* message, unsigned line, const char* function, const char* file);
[[noreturn]] inline void _FailMessage(const char* message, unsigned line, const char* file)
{
    _FailMessage(message, line, nullptr, file);
}

#define PROFILE(x) guiProfileStart=NativeDebugMilliseconds(); guiExecutions=(x); for(giProfileCount=0; giProfileCount < static_cast<INT32>(guiExecutions); ++giProfileCount)
#define PROFILE_REPORT() NativeDebugProfileReport()
#if defined(_DEBUG) && !defined(FORCE_ASSERTS_ON)
#define FORCE_ASSERTS_ON
#endif
#ifdef FORCE_ASSERTS_ON
#define SGP_NATIVE_ASSERT(condition, message) ((condition) ? (void)0 : _FailMessage((message), __LINE__, __func__, __FILE__))
#define Assert(a) SGP_NATIVE_ASSERT((a), #a)
#define AssertMsg(a,b) SGP_NATIVE_ASSERT((a), (b))
#define AssertNotNIL(a) SGP_NATIVE_ASSERT((a)!=nullptr, #a " is a null pointer")
#define AssertT(a) SGP_NATIVE_ASSERT((a), #a " is false but should be true")
#define AssertF(a) SGP_NATIVE_ASSERT(!(a), #a " is true but should be false")
#define AssertEQ(a,b) SGP_NATIVE_ASSERT((a)==(b), #a " != " #b)
#define AssertNE(a,b) SGP_NATIVE_ASSERT((a)!=(b), #a " == " #b)
#define AssertLT(a,b) SGP_NATIVE_ASSERT((a)<(b), #a " >= " #b)
#define AssertLE(a,b) SGP_NATIVE_ASSERT((a)<=(b), #a " > " #b)
#define AssertGT(a,b) SGP_NATIVE_ASSERT((a)>(b), #a " <= " #b)
#define AssertGE(a,b) SGP_NATIVE_ASSERT((a)>=(b), #a " < " #b)
#else
#define Assert(a) ((void)0)
#define AssertMsg(a,b) ((void)0)
#define AssertNotNIL(a) ((void)0)
#define AssertT(a) ((void)0)
#define AssertF(a) ((void)0)
#define AssertEQ(a,b) ((void)0)
#define AssertNE(a,b) ((void)0)
#define AssertLT(a,b) ((void)0)
#define AssertLE(a,b) ((void)0)
#define AssertGT(a,b) ((void)0)
#define AssertGE(a,b) ((void)0)
#endif
#define InitializeDebugManager() DbgInitialize()
#define ShutdownDebugManager() DbgShutdown()
// Errors remain observable in release builds, independently of topic filters.
#define ErrorMsg(a) _DebugMessage((a), __LINE__, __FILE__)
#ifdef SGP_DEBUG
#define DebugBreakpoint() NativeDebugBreakpoint()
#define DbgMessage(a,b,c) DbgMessageReal((a), TOPIC_MESSAGE, (b), (c))
#define DebugMsg(a,b,c) DbgMessageReal((a), TOPIC_MESSAGE, (b), (c))
#define FastDebugMsg(a) _DebugMessage((a), __LINE__, __FILE__)
#define UnRegisterDebugTopic(a,b) DbgTopicRegistration(TOPIC_UNREGISTER, (a), (b))
#define ClearAllDebugTopics() DbgClearAllTopics()
#define RegisterJA2DebugTopic(a,b) DbgTopicRegistration(TOPIC_REGISTER, (a), (b))
// Unlike the disabled legacy compatibility macro, native registrations are real.
#define RegisterDebugTopic(a,b) DbgTopicRegistration(TOPIC_REGISTER, (a), (b))
#else
#define DebugBreakpoint() ((void)0)
#define DbgMessage(a,b,c) ((void)0)
#define DebugMsg(a,b,c) ((void)0)
#define FastDebugMsg(a) ((void)0)
#define UnRegisterDebugTopic(a,b) ((void)0)
#define ClearAllDebugTopics() ((void)0)
#define RegisterJA2DebugTopic(a,b) ((void)0)
#define RegisterDebugTopic(a,b) ((void)0)
#endif
#ifdef DEBUG_ATTACKBUSY
#define DebugAttackBusy(x) _DebugMessage((x), __LINE__, __FILE__)
#else
#define DebugAttackBusy(x) ((void)0)
#endif
#define SGP_CALLER_LOCATION , int line=0, const char* function=nullptr, const char* file=nullptr
#define SGP_CALLER_LOCATION_IMPL , int line, const char* function, const char* file
#define SGP_GET_CALLER_LOCATION , __LINE__, __func__, __FILE__
#define SGP_SET_CALLER_LOCATION , line, function, file
namespace vfs { class Exception; }
namespace sgp {
class WString {
public:
    WString(const std::string& text);
    WString(const char* text);
    WString(const std::wstring& text) : str(text) {}
    WString(const wchar_t* text) : str(text ? text : L"") {}
    std::wstring str;
};
class Exception : public std::exception {
public:
    Exception(const WString& msg SGP_CALLER_LOCATION);
    Exception(const WString& msg, Exception& ex SGP_CALLER_LOCATION);
    Exception(const WString& msg, std::exception& ex SGP_CALLER_LOCATION);
    Exception(const WString& msg, vfs::Exception& ex SGP_CALLER_LOCATION);
    const char* what() const noexcept override;
    struct Excp { int line; std::string function, file; std::wstring msg; };
    std::list<Excp> _msg;
private:
    mutable std::string rendered_;
};
}
#define SGP_THROW(msg) throw sgp::Exception((msg) SGP_GET_CALLER_LOCATION)
#define SGP_RETHROW(msg,ex) throw sgp::Exception((msg), (ex) SGP_GET_CALLER_LOCATION)
#define SGP_TRYCATCH_RETHROW(expr,msg) do { try { (expr); } catch(std::exception& ex) { SGP_RETHROW((msg),ex); } } while(false)
#define SGP_THROW_IFFALSE(cond,msg) do { if(!(cond)) { SGP_THROW(msg); } } while(false)
#define SGP_THROW_IFNULL(ptr,msg) do { if((ptr)==nullptr) { SGP_THROW(msg); } } while(false)
[[noreturn]] void _ExceptionMessage(sgp::Exception& ex);
[[noreturn]] void _ExceptionMessage(vfs::Exception& ex);
#endif
