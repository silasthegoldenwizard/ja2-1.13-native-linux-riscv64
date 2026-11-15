/*
 * P710 native snapshot update: Utils/NativeCinematicsBink.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Original Cinematics Bink interface, implemented with genuine bounded native
// video decode + locked framebuffer + existing streamed native sound service.
#include "NativeCinematicsBink.h"
#include "native_video_decoder.h"
#include "native_surface.h"
#include "native_video.h"
#include "native_soundman.h"
#include "timer.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <string>
namespace {
constexpr unsigned Slots=4;
std::array<BINKFLIC,Slots> list{};
struct Playback {
    std::unique_ptr<NativeVideoDecoder> decoder;
    NativeVideoDecoder::Frame pending{};
    std::string filename;
    UINT32 sound=NO_SAMPLE,previous=0;
    std::uint64_t elapsed=0,endTime=0;
    bool anchored=false,hasFrame=false,eof=false;
};
std::array<Playback,Slots> state{};
UINT32 screenHeight=0;
std::string lastError;
int slot(BINKFLIC* p) { for(unsigned i=0;i<Slots;++i) if(p==&list[i]) return static_cast<int>(i); return -1; }
bool readNext(Playback& s) {
    const auto result=s.decoder->ReadFrame(s.pending);
    s.hasFrame=result.frame; s.eof=result.eof;
    if(result.error) { lastError=s.decoder->Error(); return false; }
    if(!result.frame && !result.eof) { lastError="Movie decoder made no progress"; return false; }
    return true;
}
bool startSound(Playback& s) {
    // A silent video and the original disabled-before-init sound option require
    // no audio stream. All enabled audio uses the existing finite PCM ring.
    if(!s.decoder->HasAudio() || !NativeSoundPlaybackEnabled()) return true;
    SOUNDPARMS params; std::memset(&params,0xff,sizeof(params));
    params.uiVolume=SoundGetDefaultVolume(); params.uiLoop=1;
    params.EOSCallback=nullptr; params.pCallbackData=nullptr;
    s.sound=SoundPlayStreamedFile(const_cast<char*>(s.filename.c_str()),&params);
    if(s.sound==SOUND_ERROR) { lastError=std::string("Movie audio: ")+NativeSoundLastError(); return false; }
    return true;
}
bool draw(const BINKFLIC& f,const NativeVideoDecoder::Frame& frame) {
    UINT32 pitch=0; auto* pixels=LockVideoSurface(FRAME_BUFFER,&pitch);
    if(!pixels) { lastError="Movie framebuffer is unavailable or already locked"; return false; }
    sgp::NativeSurfaceView view{};
    const bool valid=sgp::LockedNativeSurfaceView(pixels,&view) && view.pitch==pitch && view.width && view.height &&
        view.pitch>=view.width*2 && view.bytes>=static_cast<std::size_t>(view.height-1)*view.pitch+view.width*2 &&
        frame.pixels && frame.width && frame.height && frame.pitch>=frame.width*2 &&
        frame.bytes>=static_cast<std::size_t>(frame.height-1)*frame.pitch+frame.width*2;
    if(!valid) { UnLockVideoSurface(FRAME_BUFFER); lastError="Movie requires a real locked RGB565 framebuffer"; return false; }
    // Positions remain unsigned as in the public original interface. An origin
    // outside the real surface is fully clipped, never an unchecked span.
    const UINT32 width=f.uiLeft<view.width?std::min(frame.width,view.width-f.uiLeft):0;
    const UINT32 height=f.uiTop<view.height?std::min(frame.height,view.height-f.uiTop):0;
    if(width && height) for(UINT32 y=0;y<height;++y)
        std::memcpy(view.data+static_cast<std::size_t>(f.uiTop+y)*view.pitch+static_cast<std::size_t>(f.uiLeft)*2,
                    frame.pixels+static_cast<std::size_t>(y)*frame.pitch,static_cast<std::size_t>(width)*2);
    UnLockVideoSurface(FRAME_BUFFER);
    if(width && height) InvalidateRegion(f.uiLeft,f.uiTop,f.uiLeft+width,f.uiTop+height);
    return true;
}
}
void BinkInitialize(void*,UINT32 width,UINT32 height) { BinkShutdown(); (void)width; screenHeight=height; lastError.clear(); }
void BinkShutdown() { for(auto& f:list) BinkCloseFlic(&f); }
void BinkShutdownVideo() { BinkShutdown(); screenHeight=0; }
void BinkSetupVideo() { /* Surfaces belong to the existing native video manager. */ }
const char* NativeBinkLastError() { return lastError.c_str(); }
BINKFLIC* BinkGetFreeFlic() { for(auto& f:list) if(!(f.uiFlags&BINK_FLIC_OPEN)) return &f; return nullptr; }
BINKFLIC* BinkOpenFlic(const CHAR8* filename) {
    lastError.clear(); auto* f=BinkGetFreeFlic();
    if(!f) { lastError="Bink movie slot limit reached (four)"; return nullptr; }
    const int i=slot(f); auto decoder=NativeVideoDecoder::OpenFile(filename,lastError);
    if(!decoder) return nullptr;
    Playback candidate; candidate.decoder=std::move(decoder); candidate.filename=filename;
    if(!readNext(candidate) || !candidate.hasFrame) { if(lastError.empty()) lastError="Movie has no decodable frames"; return nullptr; }
    state[i]=std::move(candidate); *f={}; f->cFilename=state[i].filename.c_str(); f->uiFlags=BINK_FLIC_OPEN;
    return f;
}
void BinkSetBlitPosition(BINKFLIC* f,UINT32 left,UINT32 top) { if(slot(f)>=0 && (f->uiFlags&BINK_FLIC_OPEN)) { f->uiLeft=left; f->uiTop=top; } }
void BinkCloseFlic(BINKFLIC* f) {
    const int i=slot(f); if(i<0) return;
    if(state[i].sound!=NO_SAMPLE) SoundStop(state[i].sound);
    state[i]=Playback{}; *f={};
}
BINKFLIC* BinkPlayFlic(const CHAR8* filename,UINT32 left,UINT32 top,UINT32 flags) {
    auto* f=BinkOpenFlic(filename); if(!f) return nullptr;
    auto& s=state[slot(f)];
    // Original centers vertically only. Oversize movies are cropped from row0,
    // avoiding the original unsigned underflow when movie height>screen height.
    if(flags&BINK_FLIC_CENTER_VERTICAL) top=s.decoder->Height()<screenHeight?(screenHeight-s.decoder->Height())/2:0;
    BinkSetBlitPosition(f,left,top);
    f->uiFlags|=BINK_FLIC_PLAYING|((flags&BINK_FLIC_AUTOCLOSE)?BINK_FLIC_AUTOCLOSE:BINK_FLIC_LOOP);
    if(!startSound(s)) { BinkCloseFlic(f); return nullptr; }
    return f;
}
BOOLEAN NativeBinkPollAt(UINT32 now) {
    BOOLEAN playing=FALSE;
    SoundServiceStreams();
    for(unsigned i=0;i<Slots;++i) {
        auto& f=list[i]; auto& s=state[i];
        if(!(f.uiFlags&BINK_FLIC_PLAYING)) continue;
        // Like the original, the poll that closes the final movie still returns
        // TRUE. Intro observes FALSE on its next poll and then closes the pointer.
        playing=TRUE;
        if(s.sound!=NO_SAMPLE) {
            const auto outcome=NativeSoundGetStreamOutcome(s.sound);
            if(outcome==NativeSoundStreamOutcome::Failed || outcome==NativeSoundStreamOutcome::Unknown) {
                lastError="Movie streamed audio failed or lost its terminal outcome"; BinkCloseFlic(&f); continue;
            }
            if(outcome==NativeSoundStreamOutcome::Finished || outcome==NativeSoundStreamOutcome::Stopped) s.sound=NO_SAMPLE;
        }
        if(!s.anchored) { s.previous=now; s.anchored=true; }
        else { s.elapsed+=static_cast<UINT32>(now-s.previous); s.previous=now; }
        if(s.eof && s.elapsed>=s.endTime) {
            if(f.uiFlags&BINK_FLIC_LOOP) {
                if(s.sound!=NO_SAMPLE) { SoundStop(s.sound); s.sound=NO_SAMPLE; }
                if(!s.decoder->Rewind()) { lastError=s.decoder->Error(); BinkCloseFlic(&f); continue; }
                s.elapsed=0; s.endTime=0; f.uiFrame=0;
                if(!readNext(s) || !s.hasFrame || !startSound(s)) { BinkCloseFlic(&f); continue; }
            } else { BinkCloseFlic(&f); continue; }
        }
        if(s.hasFrame && s.elapsed>=s.pending.timeMs) {
            if(!draw(f,s.pending)) { BinkCloseFlic(&f); continue; }
            s.endTime=s.pending.timeMs+s.pending.durationMs; ++f.uiFrame;
            if(!readNext(s)) BinkCloseFlic(&f);
        }
    }
    return playing;
}
BOOLEAN BinkPollFlics() { return NativeBinkPollAt(GetClock()); }
