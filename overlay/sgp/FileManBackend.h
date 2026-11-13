/*
 * P710 native notice preparation: sgp/FileManBackend.h
 * New project path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: define file-backend ownership and callback boundary.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

#ifndef SGP_FILEMAN_BACKEND_H
#define SGP_FILEMAN_BACKEND_H

#include "core_types.h"
#include <cstddef>
#include <string>

namespace fileman
{
enum class Search { Top, NamedProfile, StopOnWritableProfile };
enum class Origin { Begin, Current, End };

struct OpenRequest
{
    const char* filename;
    const char* profile;
    bool write;
    Search search;
};

// The provider retains ownership of context (bfVFS owns its IBaseFile objects).
// A successful open transfers close responsibility to the handle registry.
struct Operations
{
    void (*close)(void*);
    std::uint64_t (*size)(void*);
    std::uint64_t (*tell)(void*, bool write);
    void (*seek)(void*, std::int64_t, Origin, bool write);
    std::size_t (*read)(void*, void*, UINT32);
    std::size_t (*write)(void*, const void*, UINT32);
    bool (*readLine)(void*, std::string&);
};

struct BackendFile
{
    void* context = nullptr;
    const Operations* operations = nullptr;
};

// Production definition uses bfVFS; the focused test links a fixture provider.
BackendFile OpenBackend(const OpenRequest& request);
// Shutdown invalidates all IDs and closes each distinct context once.
// IDs are deliberately never reset or recycled during the process lifetime.
void CloseAll() noexcept;
}

#endif
