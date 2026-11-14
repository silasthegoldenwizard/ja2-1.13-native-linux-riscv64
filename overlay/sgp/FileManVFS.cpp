/*
 * P710 native notice preparation: sgp/FileManVFS.cpp
 * New path extracting/adapting inherited engine material.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: extract original v1.13 VFS provider from Win32 metadata services.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Inherited extraction/translation source: sgp/FileMan.cpp.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/FileManVFS.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Original v1.13 FileMan provider, separated from Win32 metadata services.
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_file_raii.h>
#include <vfs/Tools/vfs_parser_tools.h>
#include <cstdio>
#include <exception>

#ifdef _WIN32
#include "types.h"
#include "DEBUG.H"
#include "sgp_logger.h"
#endif

#if defined(_WIN32) && defined(JA2TESTVERSION)
	extern UINT32 uiTotalFileReadTime;
	extern UINT32 uiTotalFileReadCalls;
	#include "Timer Control.h"

class TimeCounter
{
public:
	TimeCounter() : start_time(GetJA2Clock()) {}
	~TimeCounter()
	{
		uiTotalFileReadTime += GetJA2Clock() - start_time;
		uiTotalFileReadCalls++;
	}
private:
	UINT32 start_time;
};

#endif

namespace
{
void reportOpenError(const char* message)
{
#ifdef _WIN32
    SGP_ERROR(message);
#else
    std::fprintf(stderr, "FileMan: %s\n", message);
#endif
}

[[noreturn]] void rethrowBackendError(std::exception& exception)
{
#ifdef _WIN32
    SGP_RETHROW(L"", exception);
#else
    VFS_RETHROW(L"", exception);
#endif
}
}

// bfVFS remains the production provider. FileManIO owns only opaque handle IDs;
// the VFS retains file-object ownership and its existing profile/search policy.
namespace
{
vfs::IBaseFile* backendBase(void* file)
{
    return static_cast<vfs::IBaseFile*>(file);
}

void backendClose(void* file) { backendBase(file)->close(); }
std::uint64_t backendSize(void* file) { return backendBase(file)->getSize(); }

std::uint64_t backendTell(void* file, bool write)
{
    if (write) return vfs::tWritableFile::cast(backendBase(file))->getWritePosition();
    return vfs::tReadableFile::cast(backendBase(file))->getReadPosition();
}

void backendSeek(void* file, std::int64_t offset, fileman::Origin origin, bool write)
{
    const auto direction = origin == fileman::Origin::Begin ? vfs::IBaseFile::SD_BEGIN :
                           origin == fileman::Origin::End ? vfs::IBaseFile::SD_END :
                           vfs::IBaseFile::SD_CURRENT;
    // FileManIO has already decoded the legacy signed 32-bit distance.
    const auto distance = static_cast<vfs::offset_t>(offset);
    if (write)
    {
        try { vfs::tWritableFile::cast(backendBase(file))->setWritePosition(distance, direction); }
        catch (std::exception& ex) { rethrowBackendError(ex); }
    }
    else
    {
        try { vfs::tReadableFile::cast(backendBase(file))->setReadPosition(distance, direction); }
        catch (std::exception& ex) { rethrowBackendError(ex); }
    }
}

std::size_t backendRead(void* file, void* dest, UINT32 bytes)
{
#if defined(_WIN32) && defined(JA2TESTVERSION)
    TimeCounter timer;
#endif
    try { return vfs::tReadableFile::cast(backendBase(file))->read(static_cast<vfs::Byte*>(dest), bytes); }
    catch (std::exception& ex) { rethrowBackendError(ex); }
}

std::size_t backendWrite(void* file, const void* source, UINT32 bytes)
{
    try { return vfs::tWritableFile::cast(backendBase(file))->write(static_cast<const vfs::Byte*>(source), bytes); }
    catch (std::exception& ex) { rethrowBackendError(ex); }
}

bool backendReadLine(void* file, std::string& dest)
{
    vfs::CReadLine line(*vfs::tReadableFile::cast(backendBase(file)), false);
    line.getLine(dest);
    return true; // Preserve the legacy non-EOF FileReadLine return contract.
}

const fileman::Operations backendOperations{
    backendClose, backendSize, backendTell, backendSeek,
    backendRead, backendWrite, backendReadLine};
}

fileman::BackendFile fileman::OpenBackend(const fileman::OpenRequest& request)
{
    try
    {
        const vfs::Path path(request.filename);
        vfs::IBaseFile* file = NULL;
        if (request.write)
        {
            vfs::COpenWriteFile open(path, true, false, vfs::CVirtualFile::SF_STOP_ON_WRITABLE_PROFILE);
            file = &open.file();
            open.release();
        }
        else if (request.search == fileman::Search::NamedProfile)
        {
            vfs::COpenReadFile open(vfs::tReadableFile::cast(getVFS()->getFile(path, request.profile)));
            file = &open.file();
            open.release();
        }
        else
        {
            vfs::COpenReadFile open(path, vfs::CVirtualFile::SF_TOP);
            file = &open.file();
            open.release();
        }
        return {file, &backendOperations};
    }
    catch (vfs::Exception& ex) { reportOpenError(ex.what()); }
    catch (...) { reportOpenError("Caught undefined exception"); }
    return {};
}
