/*
 * P710 native notice preparation: tests/native-core/fileman_tests.cpp
 * New candidate test/build/documentation/metadata path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: add native FileMan backend, handle lifetime and I/O contract tests.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

#include "FileManIO.h"
#include "FileManBackend.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
namespace
{
static_assert(sizeof(HWFILE) == 4, "The public handle stays 32 bits");
static_assert(sizeof(void*) == 8, "This is the native 64-bit readiness test");

struct NativeFile
{
    fs::path path;
    std::FILE* stream = nullptr;
    unsigned closes = 0;
    bool failRead = false, failWrite = false, failClose = false, failSeek = false;
    bool overRead = false, overWrite = false;
    UINT32 writeLimit = std::numeric_limits<UINT32>::max();
    std::optional<std::uint64_t> sizeOverride, positionOverride;
    std::int64_t lastOffset = 0;
    fileman::Origin lastOrigin = fileman::Origin::Begin;
    bool lastWrite = false;
    ~NativeFile() { if (stream) std::fclose(stream); }
};

fs::path root;
std::map<std::string, std::unique_ptr<NativeFile>> pool;
std::vector<fileman::Search> searches;
bool writableAvailable = true;
std::uintptr_t largestContext = 0;
unsigned checks = 0;

void check(bool value, const char* label)
{
    ++checks;
    if (!value) throw std::runtime_error(label);
}

template<class Function> bool throws(Function function)
{
    try { function(); }
    catch (const std::runtime_error&) { return true; }
    return false;
}

NativeFile& native(void* context) { return *static_cast<NativeFile*>(context); }

void closeNative(void* context)
{
    auto& file = native(context);
    ++file.closes;
    if (!file.stream) throw std::runtime_error("backend closed twice");
    std::fclose(file.stream);
    file.stream = nullptr;
    if (file.failClose) throw std::runtime_error("injected close failure");
}

std::uint64_t sizeNative(void* context)
{
    auto& file = native(context);
    if (file.sizeOverride) return *file.sizeOverride;
    std::fflush(file.stream);
    return fs::file_size(file.path);
}

std::uint64_t tellNative(void* context, bool write)
{
    auto& file = native(context);
    file.lastWrite = write;
    if (file.positionOverride) return *file.positionOverride;
    const auto position = ::ftello(file.stream);
    if (position < 0) throw std::runtime_error("native tell failed");
    return static_cast<std::uint64_t>(position);
}

void seekNative(void* context, std::int64_t offset, fileman::Origin origin, bool write)
{
    auto& file = native(context);
    file.lastOffset = offset;
    file.lastOrigin = origin;
    file.lastWrite = write;
    if (file.failSeek) throw std::runtime_error("injected seek failure");
    const int how = origin == fileman::Origin::Begin ? SEEK_SET :
                    origin == fileman::Origin::End ? SEEK_END : SEEK_CUR;
    if (::fseeko(file.stream, static_cast<off_t>(offset), how))
        throw std::runtime_error("native seek failed");
}

std::size_t readNative(void* context, void* dest, UINT32 bytes)
{
    auto& file = native(context);
    if (file.failRead) throw std::runtime_error("injected read failure");
    if (file.overRead) return static_cast<std::size_t>(bytes) + 1;
    return std::fread(dest, 1, bytes, file.stream);
}

std::size_t writeNative(void* context, const void* source, UINT32 bytes)
{
    auto& file = native(context);
    if (file.failWrite) throw std::runtime_error("injected write failure");
    if (file.overWrite) return static_cast<std::size_t>(bytes) + 1;
    return std::fwrite(source, 1, std::min(bytes, file.writeLimit), file.stream);
}

bool lineNative(void* context, std::string& dest)
{
    auto& file = native(context);
    dest.clear();
    int c;
    while ((c = std::fgetc(file.stream)) != EOF && c != '\n')
        dest.push_back(static_cast<char>(c));
    return c != EOF || !dest.empty();
}

const fileman::Operations operations{
    closeNative, sizeNative, tellNative, seekNative, readNative, writeNative, lineNative};

void fixture(const char* profile, const char* name, const std::string& contents)
{
    const auto path = root / profile / name;
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << contents;
    if (!out) throw std::runtime_error("cannot create fixture");
}

HWFILE open(std::string name, UINT32 mode = FILE_ACCESS_READ, std::string profile = {})
{
    return FileOpen(name.data(), mode, FALSE, profile.empty() ? nullptr : profile.data());
}

NativeFile& context(const char* profile, const char* name)
{
    return *pool.at((root / profile / name).string());
}

std::string contents(const char* profile, const char* name)
{
    std::ifstream in(root / profile / name, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void invalidHandles()
{
    const auto before = searches.size();
    check(FileOpen(nullptr, FILE_ACCESS_READ) == 0, "null filename");
    check(open("") == 0, "empty filename");
    check(open("data", 0) == 0, "missing access mode");
    check(searches.size() == before, "invalid opens do not reach backend");
    check(open("missing") == 0, "missing file");
    check(open("throw-open") == 0, "backend open exception becomes zero");
    for (HWFILE id : {HWFILE{0}, HWFILE{0x80000000}, std::numeric_limits<HWFILE>::max()})
    {
        char buffer[8]{};
        UINT32 count = 42;
        check(!FileRead(id, buffer, 1, &count) && count == 0, "invalid read");
        count = 42;
        check(!FileWrite(id, buffer, 1, &count) && count == 0, "invalid write");
        check(!FileRead(id, nullptr, 0, nullptr), "invalid zero-byte read");
        check(!FileWrite(id, nullptr, 0, nullptr), "invalid zero-byte write");
        check(!FileSeek(id, 0, FILE_SEEK_FROM_START), "invalid seek");
        check(FileGetPos(id) == -1, "invalid position");
        check(FileGetSize(id) == 0, "invalid size");
        check(!FileCheckEndOfFile(id), "invalid EOF");
        check(!FileReadLine(id, nullptr), "invalid line");
        FileClose(id);
        FileClose(id);
    }
}

void readAndSeek()
{
    const auto id = open("data");
    auto& file = context("base", "data");
    check(id != 0, "read open");
    check(searches.back() == fileman::Search::Top, "default read search");
    check(largestContext > std::numeric_limits<UINT32>::max(), "real native context exceeds 32 bits");
    check(id != static_cast<HWFILE>(reinterpret_cast<std::uintptr_t>(&file)), "ID is not truncated context");
    check(FileGetSize(id) == 6 && FileGetPos(id) == 0, "initial size and position");
    check(!FileCheckEndOfFile(id), "initial EOF false");
    UINT32 count = 99;
    char buffer[16]{};
    check(FileRead(id, buffer, 3, &count) && count == 3 && std::string(buffer, 3) == "abc", "read content");
    check(!FileRead(id, buffer, 8, &count) && count == 3 && std::string(buffer, 3) == "def", "short read count");
    check(FileCheckEndOfFile(id) && FileGetPos(id) == 6, "EOF after short read");
    check(FileRead(id, nullptr, 0, nullptr), "zero-byte read accepts nulls");
    check(!FileRead(id, nullptr, 1, &count) && count == 0, "nonempty read requires buffer");
    check(!FileWrite(id, "x", 1, &count) && count == 0, "read mode rejects write");
    check(!FileWrite(id, nullptr, 0, nullptr), "read mode rejects empty write");
    check(FileSeek(id, 2, FILE_SEEK_FROM_START) && FileGetPos(id) == 2, "seek from start");
    check(FileSeek(id, static_cast<UINT32>(-1), FILE_SEEK_FROM_CURRENT) && FileGetPos(id) == 1, "signed relative seek");
    check(file.lastOffset == -1 && file.lastOrigin == fileman::Origin::Current && !file.lastWrite, "relative seek decoded");
    check(FileSeek(id, 2, FILE_SEEK_FROM_END) && FileGetPos(id) == 4, "positive end distance is backward");
    check(FileSeek(id, static_cast<UINT32>(-3), FILE_SEEK_FROM_END) && FileGetPos(id) == 3, "negative end distance");
    check(FileSeek(id, 1, 0) && FileGetPos(id) == 4, "legacy unknown origin means current");
    file.failSeek = true;
    check(throws([&] { FileSeek(id, 0x80000000U, FILE_SEEK_FROM_CURRENT); }), "seek exception propagates");
    check(file.lastOffset == std::numeric_limits<INT32>::min(), "INT32_MIN distance is exact");
    file.failSeek = false;
    check(FileGetPos(id) == 4, "seek failure preserves live handle");
    file.positionOverride = static_cast<std::uint64_t>(std::numeric_limits<INT32>::max());
    check(FileGetPos(id) == std::numeric_limits<INT32>::max(), "max representable position");
    file.positionOverride = std::uint64_t{1} << 31;
    check(FileGetPos(id) == -1, "position overflow rejected");
    file.sizeOverride = std::numeric_limits<UINT32>::max();
    check(FileGetSize(id) == std::numeric_limits<UINT32>::max(), "max representable size");
    file.sizeOverride = std::uint64_t{1} << 32;
    check(FileGetSize(id) == 0, "size overflow rejected");
    file.sizeOverride.reset();
    file.positionOverride.reset();
    FileClose(id);
    check(file.closes == 1, "close once");
    FileClose(id);
    check(file.closes == 1 && FileGetPos(id) == -1, "repeat close and stale position");
    const auto next = open("data");
    check(next != id && next != 0, "new ID for reused native address");
    check(&context("base", "data") == &file, "fixture reused exact context address");
    check(!FileRead(id, buffer, 1, &count) && FileGetPos(next) == 0, "stale ID cannot read reopened file");
    FileClose(id);
    check(FileRead(next, buffer, 1, &count) && buffer[0] == 'a', "stale close cannot close reopened file");
    FileClose(next);
}

void aliasesAndCallbacks()
{
    const auto a = open("alias");
    UINT32 count = 0;
    char value = 0;
    check(FileRead(a, &value, 1, &count) && value == 'z', "first alias read");
    const auto b = open("alias");
    auto& file = context("base", "alias");
    check(a && b && a != b, "two IDs for one VFS-style context");
    FileClose(a);
    check(file.closes == 0 && file.stream, "first alias close retains backend");
    check(FileGetPos(b) == 1, "repeated open preserves backend shared cursor");
    check(FileRead(b, &value, 1, &count) && value == 'y', "remaining alias usable");
    FileClose(b);
    check(file.closes == 1, "last alias closes backend once");
    check(FileHandleFromCallbackToken(FileHandleToCallbackToken(a)) == a, "callback ID round trip");
    check(FileHandleFromCallbackToken(nullptr) == 0, "null callback");
    check(FileHandleToCallbackToken(0) == nullptr, "zero callback");
    check(FileHandleToCallbackToken(std::numeric_limits<HWFILE>::max()) == nullptr, "reserved callback encode");
    check(FileHandleFromCallbackToken(reinterpret_cast<void*>(std::uintptr_t{0xffffffffU})) == 0, "reserved callback decode");
    check(FileHandleFromCallbackToken(reinterpret_cast<void*>((std::uintptr_t{1} << 32) | a)) == 0, "callback rejects high bits");
    check(FileHandleFromCallbackToken(FileHandleToCallbackToken(0xfffffffeU)) == 0xfffffffeU, "largest allowed callback ID");
}

void writeAndProfiles()
{
    auto id = open("overlay");
    char buffer[16]{};
    UINT32 count = 0;
    check(FileRead(id, buffer, 8, &count) && std::string(buffer, 8) == "writable", "fixture top overlay read");
    FileClose(id);
    id = open("overlay", FILE_ACCESS_READ, "mod");
    check(searches.back() == fileman::Search::NamedProfile, "named profile request");
    check(FileRead(id, buffer, 3, &count) && std::string(buffer, 3) == "mod", "fixture named profile read");
    FileClose(id);
    check(open("overlay", FILE_ACCESS_READ, "absent") == 0, "missing named profile has no fallback");
    id = open("overlay", FILE_ACCESS_READWRITE | FILE_CREATE_ALWAYS, "base");
    auto& file = context("writable", "overlay");
    check(id && searches.back() == fileman::Search::StopOnWritableProfile, "write precedence and writable-profile policy");
    check(FileGetSize(id) == 8, "legacy create flags do not truncate existing file");
    check(FileWrite(id, nullptr, 0, nullptr), "empty write safely accepts null count");
    count = 9;
    check(FileWrite(id, nullptr, 0, &count) && count == 0, "empty write count");
    check(!FileWrite(id, nullptr, 1, &count) && count == 0, "nonempty write requires buffer");
    check(!FileRead(id, buffer, 1, &count) && count == 0, "legacy readwrite selects write mode");
    check(!FileReadLine(id, nullptr), "write handle rejects line read");
    file.writeLimit = 2;
    check(!FileWrite(id, "HELLO", 5, &count) && count == 2, "short write reports actual count");
    file.writeLimit = std::numeric_limits<UINT32>::max();
    check(FileSeek(id, 0, FILE_SEEK_FROM_START) && file.lastWrite, "write-position seek");
    check(FileWrite(id, "NEW", 3, nullptr), "write with optional output count");
    check(FileGetPos(id) == 3 && file.lastWrite, "write-position query");
    check(FileSeek(id, 0, FILE_SEEK_FROM_END) && FileCheckEndOfFile(id), "write EOF");
    FileClose(id);
    check(contents("base", "overlay") == "base", "base profile preserved");
    check(contents("mod", "overlay") == "mod", "mod profile preserved");
    check(contents("writable", "overlay") == "NEWtable", "only writable fixture changed");
    writableAvailable = false;
    check(open("overlay", FILE_ACCESS_WRITE) == 0, "no writable profile fails closed");
    check(contents("base", "overlay") == "base", "failed write cannot fall through to base");
    writableAvailable = true;
    id = open("new", FILE_ACCESS_WRITE | FILE_OPEN_ALWAYS);
    check(id && FileWrite(id, "created", 7, &count) && count == 7, "create writable fixture");
    FileClose(id);
    check(contents("writable", "new") == "created", "created bytes");
}

void linesAndFailures()
{
    auto id = open("lines");
    std::string line;
    check(!FileReadLine(id, nullptr), "line requires destination");
    check(FileReadLine(id, &line) && line == "first", "first line");
    check(FileReadLine(id, &line) && line == "second", "last unterminated line");
    check(!FileReadLine(id, &line), "line EOF");
    FileClose(id);
    id = open("empty");
    check(FileCheckEndOfFile(id) && !FileReadLine(id, &line), "empty-file EOF");
    FileClose(id);
    for (bool writing : {false, true})
    {
        for (bool excessiveCount : {false, true})
        {
            const UINT32 mode = writing ? FILE_ACCESS_WRITE : FILE_ACCESS_READ;
            id = open(writing ? "write-failure" : "read-failure", mode);
            const auto alias = open(writing ? "write-failure" : "read-failure", mode);
            auto& file = context(writing ? "writable" : "base", writing ? "write-failure" : "read-failure");
            const auto before = file.closes;
            file.failRead = !writing && !excessiveCount;
            file.failWrite = writing && !excessiveCount;
            file.overRead = !writing && excessiveCount;
            file.overWrite = writing && excessiveCount;
            file.failClose = true;
            char buffer[4]{};
            UINT32 count = 99;
            check(throws([&] {
                if (writing) FileWrite(id, "xx", 2, &count);
                else FileRead(id, buffer, 2, &count);
            }), "I/O failure propagates despite close failure");
            check(count == 0 && file.closes == before + 1, "I/O failure closes once and clears count");
            check(FileGetPos(id) == -1 && FileGetPos(alias) == -1, "I/O failure invalidates every alias");
            FileClose(id); FileClose(alias);
            check(file.closes == before + 1, "exception-stale closes harmless");
            file.failRead = file.failWrite = file.overRead = file.overWrite = file.failClose = false;
        }
    }
    id = open("close-failure");
    auto& file = context("base", "close-failure");
    file.failClose = true;
    check(throws([&] { FileClose(id); }), "explicit close exception propagates");
    check(FileGetPos(id) == -1 && file.closes == 1, "throwing close leaves stale ID");
    FileClose(id);
    file.failClose = false;
}

void threadedAndShutdown()
{
    constexpr unsigned workers = 4, rounds = 32;
    for (unsigned i = 0; i < workers; ++i)
        fixture("base", ("thread-" + std::to_string(i)).c_str(), "q");
    std::mutex idsMutex;
    std::set<HWFILE> ids;
    std::atomic<bool> passed{true};
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < workers; ++i)
        threads.emplace_back([&, i] {
            for (unsigned j = 0; j < rounds; ++j)
            {
                const auto id = open("thread-" + std::to_string(i));
                char byte = 0; UINT32 count = 0;
                if (!id || !FileRead(id, &byte, 1, &count) || byte != 'q' || count != 1)
                    passed = false;
                {
                    std::lock_guard<std::mutex> lock(idsMutex);
                    if (!ids.insert(id).second) passed = false;
                }
                FileClose(id);
            }
        });
    for (auto& thread : threads) thread.join();
    check(passed && ids.size() == workers * rounds, "parallel open/read/close keeps unique valid IDs");
    const auto a = open("shutdown"), b = open("shutdown"), c = open("shutdown-other");
    auto& first = context("base", "shutdown");
    auto& second = context("base", "shutdown-other");
    first.failClose = true;
    fileman::CloseAll();
    check(first.closes == 1 && second.closes == 1, "shutdown closes distinct contexts once despite failure");
    check(FileGetPos(a) == -1 && FileGetPos(b) == -1 && FileGetPos(c) == -1, "shutdown invalidates all IDs");
    FileClose(a); FileClose(b); FileClose(c);
    fileman::CloseAll();
    check(first.closes == 1 && second.closes == 1, "repeat shutdown is harmless");
    first.failClose = false;
    const auto next = open("shutdown");
    check(next > c && next != a && next != b, "shutdown never resets ID generation");
    FileClose(next);
}
}

fileman::BackendFile fileman::OpenBackend(const fileman::OpenRequest& request)
{
    searches.push_back(request.search);
    if (std::string(request.filename) == "throw-open") throw std::runtime_error("injected open failure");
    fs::path path;
    if (request.write)
    {
        if (request.search != Search::StopOnWritableProfile || request.profile)
            throw std::runtime_error("incorrect write policy");
        if (!writableAvailable) return {};
        path = root / "writable" / request.filename;
    }
    else if (request.search == Search::NamedProfile)
    {
        if (!request.profile) throw std::runtime_error("missing profile");
        path = root / request.profile / request.filename;
    }
    else
    {
        for (const auto* profile : {"writable", "mod", "base"})
            if (fs::exists(root / profile / request.filename))
            { path = root / profile / request.filename; break; }
    }
    if (path.empty() || (!request.write && !fs::exists(path))) return {};
    auto& owner = pool[path.string()];
    if (!owner) { owner = std::make_unique<NativeFile>(); owner->path = path; }
    auto& file = *owner;
    if (!file.stream)
    {
        file.stream = std::fopen(path.c_str(), request.write ? "r+b" : "rb");
        if (!file.stream && request.write) file.stream = std::fopen(path.c_str(), "w+b");
        if (!file.stream) return {};
    }
    // An already-open bfVFS-style object retains its shared cursor.
    largestContext = std::max(largestContext, reinterpret_cast<std::uintptr_t>(&file));
    return {&file, &operations};
}

int main(int argc, char** argv)
{
    try
    {
        if (argc != 2) throw std::runtime_error("expected exact fixture-output directory");
        root = fs::absolute(argv[1]) / "fixtures";
        // Never remove a caller-selected directory. Every run uses owned fixture
        // leaves, and the build runner supplies a fresh evidence directory.
        fs::create_directories(root / "writable");
        fixture("base", "data", "abcdef");
        fixture("base", "alias", "zy");
        fixture("base", "overlay", "base");
        fixture("mod", "overlay", "mod");
        fixture("writable", "overlay", "writable");
        fixture("base", "lines", "first\nsecond");
        for (const auto* name : {"empty", "read-failure", "close-failure", "shutdown", "shutdown-other"})
            fixture("base", name, std::string(name) == "empty" ? "" : "fixture");
        fixture("writable", "write-failure", "fixture");
        invalidHandles();
        readAndSeek();
        aliasesAndCallbacks();
        writeAndProfiles();
        linesAndFailures();
        threadedAndShutdown();
        fileman::CloseAll();
        for (const auto& item : pool) check(!item.second->stream, "no backend stream remains open");
        std::ofstream result(fs::path(argv[1]) / "result.json");
        result << "{\"passed\":true,\"checks\":" << checks
               << ",\"pointer_bits\":" << sizeof(void*) * 8
               << ",\"handle_bits\":" << sizeof(HWFILE) * 8
               << ",\"largest_native_context\":" << largestContext << "}\n";
        std::cout << "PASS: " << checks << " FileMan checks; 64-bit native contexts, 32-bit opaque IDs\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        fileman::CloseAll();
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
