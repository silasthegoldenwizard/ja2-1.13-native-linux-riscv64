/*
 * P710 native snapshot update: sgp/NativeFileMan.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native adaptation, 2026-11-12, original sgp/FileMan.cpp policy at
// 3394695a387a14c78e92b158cfdcf50e4068c96b. bfVFS remains the provider;
// FileManIO remains the sole opaque handle registry. No Windows emulation.
#include "native_fileman.h"
#include "FileManBackend.h"
#include "native_text.h"
#include <vfs/Core/vfs.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <ctime>
#include <sys/stat.h>

namespace {
std::unique_ptr<vfs::CVirtualFileSystem::Iterator> fileIterator;
bool valid(const char* path) { return path && *path; }
bool copyPath(char* output, std::size_t capacity, const std::string& path) {
    if (!output || path.size() >= capacity) return false;
    std::memcpy(output, path.c_str(), path.size() + 1);
    return true;
}
BOOLEAN currentEntry(GETFILESTRUCT* output) {
    if (!output || !fileIterator || fileIterator->end()) return FALSE;
    auto* file = fileIterator->value();
    const std::string name = file->getName().to_string();
    const auto size = file->getSize();
    GETFILESTRUCT result{};
    if (size > std::numeric_limits<UINT32>::max() || !copyPath(result.zFileName, sizeof(result.zFileName), name)) {
        fileIterator.reset();
        return FALSE;
    }
    result.iFindHandle = 0;
    result.uiFileSize = static_cast<UINT32>(size);
    result.uiFileAttribs = file->implementsWritable() ? FILE_IS_NORMAL : FILE_IS_READONLY;
    *output = result;
    return TRUE;
}
}

BOOLEAN InitializeFileManager(STR /*indexFilename*/) {
    // The original no longer initializes a database: bfVFS profiles are mounted
    // by the caller. Own only the public enumeration session and I/O shutdown.
    fileIterator.reset();
    return TRUE;
}
void ShutdownFileManager() {
    fileIterator.reset();
    fileman::CloseAll();
}
BOOLEAN FileExists(const char* filename) {
    if (!valid(filename)) return FALSE;
    try { return getVFS()->fileExists(vfs::Path(filename)); }
    catch (...) { return FALSE; }
}
BOOLEAN FileExistsNoDB(const char* filename) { return FileExists(filename); }
BOOLEAN FileDelete(const char* filename) {
    if (!valid(filename)) return FALSE;
    try { return getVFS()->removeFileFromFS(vfs::Path(filename)); }
    catch (...) { return FALSE; }
}
BOOLEAN FileLoad(const char* filename, PTR destination, UINT32 bytes, UINT32* bytesRead) {
    if (!valid(filename) || (bytes && !destination)) return FALSE;
    // FileOpen keeps the original SF_TOP profile policy through FileManVFS.
    const HWFILE file = FileOpen(const_cast<char*>(filename), FILE_ACCESS_READ);
    if (!file) return FALSE;
    UINT32 count = 0;
    try {
        const BOOLEAN result = FileRead(file, destination, bytes, &count);
        FileClose(file);
        // Original FileLoad publishes the count only for a complete read.
        if (!result) return FALSE;
        if (bytesRead) *bytesRead = count;
        return TRUE;
    } catch (...) {
        try { FileClose(file); } catch (...) {}
        return FALSE;
    }
}
BOOLEAN FilePrintf(HWFILE file, const char* format, ...) {
    if (!format) return FALSE;
    char text[160];
    va_list arguments;
    va_start(arguments, format);
    const int formatted = std::vsnprintf(text, sizeof(text), format, arguments);
    va_end(arguments);
    if (formatted < 0) return FALSE;
    text[sizeof(text) - 1] = 0;
    // Preserve the original intentional159-byte truncation and FileWrite path.
    return FileWrite(file, text, static_cast<UINT32>(std::strlen(text)), nullptr);
}
UINT32 FileSize(const char* filename) {
    if (!valid(filename)) return 0;
    try {
        auto* file = getVFS()->getFile(vfs::Path(filename));
        if (!file) return 0;
        const auto size = file->getSize();
        return size <= std::numeric_limits<UINT32>::max() ? static_cast<UINT32>(size) : 0;
    } catch (...) { return 0; }
}
BOOLEAN SetFileManCurrentDirectory(const char* directory) {
    if (!valid(directory)) return FALSE;
    std::error_code error;
    std::filesystem::current_path(directory, error);
    return !error;
}
BOOLEAN GetFileManCurrentDirectory(STRING512 directory) {
    if (!directory) return FALSE;
    try { return copyPath(directory, 512, std::filesystem::current_path().u8string()); }
    catch (...) { return FALSE; }
}
BOOLEAN GetExecutableDirectory(STRING512 directory) {
    if (!directory) return FALSE;
    try {
        // Linux provides the actual current executable independently of CWD.
        const auto executable = std::filesystem::read_symlink("/proc/self/exe");
        return copyPath(directory, 512, executable.parent_path().u8string());
    } catch (...) { return FALSE; }
}
BOOLEAN RemoveFileManDirectory(const char* directory, BOOLEAN /*recursive*/) {
    if (!valid(directory)) return FALSE;
    try { return getVFS()->removeDirectoryFromFS(vfs::Path(directory)); }
    catch (...) { return FALSE; }
}
BOOLEAN EraseDirectory(const char* directory) { return RemoveFileManDirectory(directory, FALSE); }
BOOLEAN GetFileFirst(const char* specification, GETFILESTRUCT* output) {
    if (!valid(specification) || !output) return FALSE;
    fileIterator.reset();
    try {
        // Direct initialization uses C++17 guaranteed elision. bfVFS Iterator
        // has an unsafe implicit copy constructor and leaky assignment, so do
        // not copy/assign it or retain pointers after replacing its session.
        fileIterator.reset(new vfs::CVirtualFileSystem::Iterator(getVFS()->begin(vfs::Path(specification))));
        return currentEntry(output);
    } catch (...) { fileIterator.reset(); return FALSE; }
}
BOOLEAN GetFileNext(GETFILESTRUCT* output) {
    if (!output || !fileIterator || fileIterator->end()) return FALSE;
    try { fileIterator->next(); return currentEntry(output); }
    catch (...) { fileIterator.reset(); return FALSE; }
}
void GetFileClose(GETFILESTRUCT* /*output*/) { fileIterator.reset(); }
BOOLEAN SoundFileExists(const char* filename, char* found, std::size_t capacity) {
    if (!valid(filename) || !found || capacity < 5) return FALSE;
    try {
        const std::string stem(filename);
        if (stem.size() > capacity - 5) return FALSE;
        for (const char* extension : {".mp3", ".ogg", ".wav"}) {
            const std::string candidate = stem + extension;
            if (FileExists(candidate.c_str())) {
                return copyPath(found, capacity, candidate);
            }
        }
        // Original leaves a .wav candidate in output even when no format exists.
        copyPath(found, capacity, stem + ".wav");
        return FALSE;
    } catch (...) { return FALSE; }
}

// This is deliberately a host-directory API, not GetFileFirst's singleton VFS
// search. Its caller has already resolved the profile path through COpenWriteFile.
bool EnumerateHostDirectoryFiles(const std::string& directory, std::vector<std::string>& output) noexcept {
    if (directory.empty() || directory.find('\0') != std::string::npos) return false;
    try {
        std::error_code error;
        std::filesystem::directory_iterator iterator(std::filesystem::u8path(directory), error), end;
        if (error) return false;
        std::vector<std::string> names;
        while (iterator != end) {
            const bool isDirectory = iterator->is_directory(error);
            if (error) return false;
            if (!isDirectory) names.push_back(iterator->path().filename().u8string());
            iterator.increment(error);
            if (error) return false;
        }
        output.swap(names);
        return true;
    } catch (...) { return false; }
}

bool MakeNativeWideFilePath(const std::string& prefix, const wchar_t* name,
                            const std::string& suffix, std::string& output) noexcept {
    if (prefix.find('\0') != std::string::npos || suffix.find('\0') != std::string::npos) return false;
    const int required = sgp::NativeWideToUtf8(name, -1, nullptr, 0);
    if (!required) return false;
    try {
        std::string converted(static_cast<std::size_t>(required), '\0');
        if (!sgp::NativeWideToUtf8(name, -1, &converted[0], converted.size())) return false;
        converted.pop_back(); // conversion count includes the terminator
        std::string path = prefix + converted + suffix;
        output.swap(path);
        return true;
    } catch (...) { return false; }
}

// bfVFS has no timestamp interface. Resolve its selected real file first rather
// than statting an arbitrary current-directory name or probing lower profiles.
bool GetNativeFileModificationTime(const char* filename, NativeFileDateTime& output) noexcept {
    if (!valid(filename)) return false;
    try {
        auto* file = getVFS()->getFile(vfs::Path(filename), vfs::CVirtualFile::SF_TOP);
        vfs::Path selected;
        if (!file || !file->_getRealPath(selected)) return false;
        const std::string hostPath = selected.to_string();
        if (hostPath.empty() || hostPath.find('\0') != std::string::npos) return false;
        struct stat metadata{};
        if (::stat(hostPath.c_str(), &metadata) != 0 || !S_ISREG(metadata.st_mode)) return false;
        const std::time_t seconds = metadata.st_mtime;
        std::tm local{};
        if (!::localtime_r(&seconds, &local) || local.tm_year < -1899 || local.tm_year > 8099) return false;
        NativeFileDateTime result{};
        result.year = local.tm_year + 1900;
        result.month = local.tm_mon + 1;
        result.day = local.tm_mday;
        result.hour = local.tm_hour;
        result.minute = local.tm_min;
        result.second = local.tm_sec;
        static_assert(sizeof(std::time_t) <= sizeof(std::int64_t), "native epoch must fit the metadata record");
        result.epochSeconds = static_cast<std::int64_t>(seconds);
        result.nanosecond = static_cast<INT32>(metadata.st_mtim.tv_nsec);
        output = result;
        return true;
    } catch (...) { return false; }
}
