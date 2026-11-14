/*
 * P710 native notice preparation: sgp/FileManIO.cpp
 * New project path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: implement portable file-handle dispatch and lifetime rules.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/FileManIO.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "FileManIO.h"
#include "FileManBackend.h"

#include <limits>
#include <map>
#include <mutex>
#include <stdexcept>

namespace
{
struct Entry
{
    fileman::BackendFile file;
    bool write;
};

// Serialize FileMan calls, including backend access. A VFS/logging callback may
// reenter FileMan on the same thread, so the lock must be recursive.
std::recursive_mutex mutex;
std::map<HWFILE, Entry> files;
std::uint64_t nextId = 1;
constexpr HWFILE invalidId = std::numeric_limits<HWFILE>::max();

bool hasContext(void* context)
{
    for (const auto& item : files)
        if (item.second.file.context == context) return true;
    return false;
}

void invalidateContext(void* context)
{
    for (auto it = files.begin(); it != files.end();)
        if (it->second.file.context == context) it = files.erase(it);
        else ++it;
}

void failIO(const Entry& entry) noexcept
{
    invalidateContext(entry.file.context);
    try { entry.file.operations->close(entry.file.context); }
    catch (...) {} // Preserve the original I/O exception, with all aliases stale.
}
}

HWFILE FileOpen(STR filename, UINT32 options, BOOLEAN /*deleteOnClose*/, STR profile)
{
    if (!filename || !filename[0] || !(options & FILE_ACCESS_READWRITE)) return 0;
    std::lock_guard<std::recursive_mutex> lock(mutex);
    // Reserve both zero and all-ones; fail closed rather than wrap into stale IDs.
    if (nextId >= invalidId) return 0;
    const bool write = (options & FILE_ACCESS_WRITE) != 0;
    const bool named = !write && profile && profile[0];
    const fileman::OpenRequest request{
        filename, named ? profile : nullptr, write,
        write ? fileman::Search::StopOnWritableProfile :
        named ? fileman::Search::NamedProfile : fileman::Search::Top};
    fileman::BackendFile file;
    try
    {
        file = fileman::OpenBackend(request);
        if (!file.context || !file.operations) return 0;
        const HWFILE id = static_cast<HWFILE>(nextId++);
        files.emplace(id, Entry{file, write});
        return id;
    }
    catch (...)
    {
        // Reopening a VFS path can return the same VFS-owned object. An open
        // failure must not close a context still held by another live ID.
        if (file.context && file.operations && !hasContext(file.context))
        {
            try { file.operations->close(file.context); } catch (...) {}
        }
        return 0;
    }
}

void FileClose(HWFILE file)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    const auto it = files.find(file);
    if (it == files.end()) return;
    const Entry entry = it->second;
    files.erase(it); // A throwing close must still leave this ID invalid.
    if (!hasContext(entry.file.context))
        entry.file.operations->close(entry.file.context);
}

BOOLEAN FileRead(HWFILE file, PTR dest, UINT32 bytes, UINT32* bytesRead)
{
    if (bytesRead) *bytesRead = 0;
    std::lock_guard<std::recursive_mutex> lock(mutex);
    const auto it = files.find(file);
    if (it == files.end() || it->second.write || (bytes && !dest)) return FALSE;
    if (!bytes) return TRUE;
    const Entry entry = it->second;
    try
    {
        const auto count = entry.file.operations->read(entry.file.context, dest, bytes);
        if (count > bytes) throw std::runtime_error("FileMan read count exceeds request");
        if (bytesRead) *bytesRead = static_cast<UINT32>(count);
        return count == bytes;
    }
    catch (...) { failIO(entry); throw; }
}

BOOLEAN FileReadLine(HWFILE file, std::string* dest)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    const auto it = files.find(file);
    if (it == files.end() || it->second.write || !dest) return FALSE;
    const Entry entry = it->second;
    if (entry.file.operations->tell(entry.file.context, false) >=
        entry.file.operations->size(entry.file.context)) return FALSE;
    return entry.file.operations->readLine(entry.file.context, *dest);
}

BOOLEAN FileWrite(HWFILE file, const void* source, UINT32 bytes, UINT32* bytesWritten)
{
    if (bytesWritten) *bytesWritten = 0;
    std::lock_guard<std::recursive_mutex> lock(mutex);
    const auto it = files.find(file);
    if (it == files.end() || !it->second.write || (bytes && !source)) return FALSE;
    if (!bytes) return TRUE;
    const Entry entry = it->second;
    try
    {
        const auto count = entry.file.operations->write(entry.file.context, source, bytes);
        if (count > bytes) throw std::runtime_error("FileMan write count exceeds request");
        if (bytesWritten) *bytesWritten = static_cast<UINT32>(count);
        return count == bytes;
    }
    catch (...) { failIO(entry); throw; }
}

BOOLEAN FileSeek(HWFILE file, UINT32 distance, UINT8 how)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    const auto it = files.find(file);
    if (it == files.end()) return FALSE;
    const Entry entry = it->second;
    // Preserve the signed 32-bit distance carried in the legacy UINT32 API.
    std::int64_t offset = distance;
    if (distance > static_cast<UINT32>(std::numeric_limits<INT32>::max()))
        offset -= (std::int64_t{1} << 32);
    auto origin = fileman::Origin::Current;
    if (how == FILE_SEEK_FROM_START) origin = fileman::Origin::Begin;
    else if (how == FILE_SEEK_FROM_END)
    {
        origin = fileman::Origin::End;
        if (offset > 0) offset = -offset;
    }
    entry.file.operations->seek(entry.file.context, offset, origin, entry.write);
    return TRUE;
}

INT32 FileGetPos(HWFILE file)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    const auto it = files.find(file);
    if (it == files.end()) return -1;
    const Entry entry = it->second;
    const auto position = entry.file.operations->tell(entry.file.context, entry.write);
    if (position > static_cast<std::uint64_t>(std::numeric_limits<INT32>::max())) return -1;
    return static_cast<INT32>(position);
}

UINT32 FileGetSize(HWFILE file)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    const auto it = files.find(file);
    if (it == files.end()) return 0;
    const Entry entry = it->second;
    const auto size = entry.file.operations->size(entry.file.context);
    if (size > std::numeric_limits<UINT32>::max()) return 0;
    return static_cast<UINT32>(size);
}

BOOLEAN FileCheckEndOfFile(HWFILE file)
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    const auto it = files.find(file);
    if (it == files.end()) return FALSE;
    const Entry entry = it->second;
    return entry.file.operations->tell(entry.file.context, entry.write) >=
           entry.file.operations->size(entry.file.context);
}

void fileman::CloseAll() noexcept
{
    std::lock_guard<std::recursive_mutex> lock(mutex);
    // Removing all aliases before invoking close also makes reentrant stale
    // callbacks harmless. Do not allocate during shutdown/exception cleanup.
    while (!files.empty())
    {
        const Entry entry = files.begin()->second;
        invalidateContext(entry.file.context);
        try { entry.file.operations->close(entry.file.context); } catch (...) {}
    }
}
