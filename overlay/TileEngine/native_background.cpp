/*
 * P710 native snapshot update: TileEngine/native_background.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "native_background.h"
#include "clip_state.h"
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>
#include <vector>
namespace sgp {
namespace {
std::map<void*,BackgroundView> allocations;
std::map<void*,BackgroundView> zAllocations;
struct SurfaceLock { BackgroundView view; HVSURFACE handle; };
std::map<UINT32,SurfaceLock> locks;
using I=std::int64_t;
}
bool ValidBackgroundView(const BackgroundView& v)
{
    if (!v.data || !v.width || !v.height || (v.pitch&1u) ||
        std::uint64_t(v.width)*2>v.pitch) return false;
    const std::uint64_t end=std::uint64_t(v.height-1)*v.pitch+std::uint64_t(v.width)*2;
    return end<=v.bytes && end<=std::uint64_t(std::numeric_limits<std::ptrdiff_t>::max());
}
bool CopyBackgroundRect(BackgroundView d,BackgroundView s,
    INT32 dx,INT32 dy,INT32 sx,INT32 sy,UINT32 width,UINT32 height)
{
    if (!ValidBackgroundView(d) || !ValidBackgroundView(s)) return false;
    const I l=std::max<I>({0,-I(dx),-I(sx)}),t=std::max<I>({0,-I(dy),-I(sy)});
    const I r=std::min<I>({width,I(d.width)-dx,I(s.width)-sx});
    const I b=std::min<I>({height,I(d.height)-dy,I(s.height)-sy});
    if (l>=r || t>=b) return true;
    const auto rowBytes=std::size_t(r-l)*2;
    const auto sourceAddress=reinterpret_cast<std::uintptr_t>(s.data);
    const auto destAddress=reinterpret_cast<std::uintptr_t>(d.data);
    // Compare offsets without pointer ordering or overflowing address sums.
    const bool overlap=sourceAddress<=destAddress ? destAddress-sourceAddress<s.bytes : sourceAddress-destAddress<d.bytes;
    std::vector<BYTE> saved;
    if (overlap)
    {
        const auto rows=std::size_t(b-t);
        if (rows>std::numeric_limits<std::size_t>::max()/rowBytes) return false;
        try { saved.resize(rows*rowBytes); } catch (...) { return false; }
        for (I y=t;y<b;++y)
            std::memcpy(saved.data()+std::size_t(y-t)*rowBytes,
                s.data+std::size_t(I(sy)+y)*s.pitch+std::size_t(I(sx)+l)*2,rowBytes);
    }
    for (I y=t;y<b;++y)
    {
        const BYTE* source=saved.empty() ? s.data+std::size_t(I(sy)+y)*s.pitch+std::size_t(I(sx)+l)*2 : saved.data()+std::size_t(y-t)*rowBytes;
        std::memcpy(d.data+std::size_t(I(dy)+y)*d.pitch+std::size_t(I(dx)+l)*2,source,rowBytes);
    }
    return true;
}
INT16* AllocateBackgroundBuffer(UINT32 width,UINT32 height)
{
    if (!width || !height || width>std::numeric_limits<UINT32>::max()/2) return nullptr;
    const UINT32 pitch=width*2;
    const auto size=std::uint64_t(pitch)*height;
    if (size>std::uint64_t(std::numeric_limits<std::ptrdiff_t>::max()) || size>std::numeric_limits<std::size_t>::max()) return nullptr;
    auto* data=static_cast<BYTE*>(std::calloc(1,static_cast<std::size_t>(size)));
    if (!data) return nullptr;
    try { allocations.emplace(data,BackgroundView{data,static_cast<std::size_t>(size),width,height,pitch}); }
    catch (...) { std::free(data); return nullptr; }
    return reinterpret_cast<INT16*>(data);
}
bool FreeBackgroundBuffer(void* data)
{
    if (!data) return true;
    const auto it=allocations.find(data);
    if (it==allocations.end()) return false;
    std::free(data); allocations.erase(it); return true;
}
BYTE* LockBackgroundSurface(UINT32 id,UINT32* pitch)
{
    if (pitch) *pitch=0;
    if (!pitch || locks.count(id)) return nullptr;
    UINT16 width=0,height=0;UINT8 depth=0;HVSURFACE surface=nullptr;
    if (!GetVideoSurfaceDescription(id,&width,&height,&depth) || depth!=16 || !GetVideoSurface(&surface,id)) return nullptr;
    UINT32 actualPitch=0;auto* data=LockVideoSurfaceBuffer(surface,&actualPitch);
    if (!data) return nullptr;
    const auto size=std::uint64_t(actualPitch)*height;
    if (size>std::numeric_limits<std::size_t>::max()) { UnLockVideoSurfaceBuffer(surface);return nullptr; }
    try { locks.emplace(id,SurfaceLock{{data,static_cast<std::size_t>(size),width,height,actualPitch},surface}); }
    catch (...) { UnLockVideoSurfaceBuffer(surface);return nullptr; }
    *pitch=actualPitch;return data;
}
void UnlockBackgroundSurface(UINT32 id)
{
    const auto it=locks.find(id);if (it==locks.end()) return;
    UnLockVideoSurfaceBuffer(it->second.handle);locks.erase(it);
}
bool OwnedBackgroundBufferView(void* pointer,BackgroundView* out)
{
    if (!out) return false;
    *out={};const auto found=allocations.find(pointer);
    if (found==allocations.end()) return false;
    *out=found->second;return true;
}
bool OwnedZBufferView(void* pointer,BackgroundView* out)
{
    if (!out) return false;
    *out={};const auto found=zAllocations.find(pointer);
    if (found==zAllocations.end()) return false;
    *out=found->second;return true;
}
bool BackgroundBufferView(void* pointer,BackgroundView* out)
{
    if (!out) return false;
    *out={};const auto a=allocations.find(pointer);
    if (a!=allocations.end()) { *out=a->second;return true; }
    const auto z=zAllocations.find(pointer);
    if (z!=zAllocations.end()) { *out=z->second;return true; }
    for (const auto& item:locks) if (item.second.view.data==pointer) { *out=item.second.view;return true; }
    NativeSurfaceView surface;
    if (LockedNativeSurfaceView(pointer, &surface)) {
        *out={surface.data,surface.bytes,surface.width,surface.height,surface.pitch};
        return true;
    }
    return false;
}
BOOLEAN CopyBackgroundBuffers(UINT16* dest,UINT32 dp,UINT16* source,UINT32 sp,
    INT32 dx,INT32 dy,INT32 sx,INT32 sy,UINT32 width,UINT32 height)
{
    BackgroundView d,s;
    if (!BackgroundBufferView(dest,&d) || !BackgroundBufferView(source,&s) || d.pitch!=dp || s.pitch!=sp) return FALSE;
    return CopyBackgroundRect(d,s,dx,dy,sx,sy,width,height) ? TRUE : FALSE;
}
}

UINT16* InitZBuffer(UINT32 pitch,UINT32 height)
{
    UINT16 width=0,actualHeight=0;UINT8 depth=0;HVSURFACE frame=nullptr;
    if (!GetVideoSurfaceDescription(FRAME_BUFFER,&width,&actualHeight,&depth) || depth!=16 ||
        height!=actualHeight || !GetVideoSurface(&frame,FRAME_BUFFER)) return nullptr;
    // Original MainGameScreenInit already owns the framebuffer lock here.
    // Allocation needs only its layout; never acquire/release the caller's lock.
    UINT32 actualPitch=0;
    if (!sgp::GetNativeSurfacePitch(frame,&actualPitch) || pitch!=actualPitch) return nullptr;
    const auto size=std::uint64_t(pitch)*height;
    if (!size || size>std::numeric_limits<std::size_t>::max() ||
        size>std::uint64_t(std::numeric_limits<std::ptrdiff_t>::max())) return nullptr;
    auto* bytes=static_cast<BYTE*>(std::calloc(1,static_cast<std::size_t>(size)));
    if (!bytes) return nullptr;
    try { sgp::zAllocations.emplace(bytes,sgp::BackgroundView{bytes,static_cast<std::size_t>(size),width,height,pitch}); }
    catch (...) { std::free(bytes);return nullptr; }
    SGPRect clip{0,0,width,actualHeight};SetClippingRect(&clip);
    return reinterpret_cast<UINT16*>(bytes);
}
BOOLEAN ShutdownZBuffer(UINT16* pointer)
{
    const auto found=sgp::zAllocations.find(pointer);
    if (found==sgp::zAllocations.end()) return FALSE;
    std::free(pointer);sgp::zAllocations.erase(found);return TRUE;
}
