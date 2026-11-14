/*
 * P710 native snapshot update: tests/native-core/native_movie_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Generated, non-game AVI fixtures only: real FileMan/VFS, FFmpeg rawvideo,
// native RGB565 framebuffer and existing SDL streamed audio path. This does NOT
// establish real Bink bitstream compatibility or run the original game startup.
#include "NativeCinematicsBink.h"
#include "native_video_decoder.h"
#include "native_codec_limits.h"
#include "native_audio_decoder.h"
#include "native_video.h"
#include "clip_state.h"
#include "native_soundman.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <array>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
namespace {
unsigned checks=0;
void check(bool yes,const char* why) { ++checks; if(!yes) throw std::runtime_error(why); }
void mount(const char* root) {
    auto p=std::make_unique<vfs::CVirtualProfile>("generated-movie",vfs::Path(root),false);
    auto directory=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root));
    check(directory->init(),"generated movie directory initializes");
    auto* d=directory.get(); p->addLocation(directory.release()); auto* profile=p.get();
    getVFS()->getProfileStack()->pushProfile(p.release()); check(getVFS()->addLocation(d,profile),"generated movie VFS profile mounts");
}
UINT16 pixel(UINT32 x,UINT32 y) {
    UINT32 pitch=0; auto* p=LockVideoSurface(FRAME_BUFFER,&pitch); check(p!=nullptr,"real framebuffer read lock");
    UINT16 color=0; std::memcpy(&color,p+y*pitch+x*2,2); UnLockVideoSurface(FRAME_BUFFER); return color;
}
void seed() {
    blt_vs_fx fill{}; fill.ColorFill=0x1234;
    check(BltVideoSurface(FRAME_BUFFER,0,0,0,0,VS_BLT_COLORFILL,&fill),"unclipped seed of entire real frame");
    check(pixel(0,0)==0x1234 && pixel(7,5)==0x1234,"literal opposite borders seeded before movie drawing");
}
BINKFLIC* play(UINT32 left=1,UINT32 top=1,UINT32 flags=BINK_FLIC_AUTOCLOSE,const char* name="movie.avi") {
    auto* f=BinkPlayFlic(name,left,top,flags);
    if(!f) throw std::runtime_error(std::string("real movie play: ")+NativeBinkLastError()); return f;
}
void cleanup() {
    BinkShutdownVideo(); ShutdownSoundManager(); sgp::ShutdownNativeVideo(); ShutdownVideoSurfaceManager();
    fileman::CloseAll(); vfs::Log::flushDeleteAll(); vfs::CVirtualFileSystem::shutdownVFS();
}
}
int main(int argc,char** argv) {
    try {
        check(argc==2,"generated AVI directory argument"); mount(argv[1]);
        std::string error; auto a=NativeVideoDecoder::OpenFile("movie.avi",error);
        void* overAllocation=av_malloc(sgp::NativeCodecSingleAllocationLimit+1);
        const bool allocationRejected=overAllocation==nullptr; av_free(overAllocation);
        check(allocationRejected,"actual approved SDK rejects a single allocation beyond process policy");
        check(a && a->Width()==3 && a->Height()==2 && !a->HasAudio(),"rawvideo real dimensions and silent track");
        auto b=NativeVideoDecoder::OpenFile("movie.avi",error); check(b!=nullptr,"second logical cursor for same source");
        const UINT16 expected[]={0xf800,0x07e0,0x001f};
        NativeVideoDecoder::Frame frame{},other{};
        for(unsigned i=0;i<3;++i) {
            auto result=a->ReadFrame(frame); check(result.frame && !result.eof && !result.error,"genuine frame decode");
            check(frame.width==3 && frame.height==2 && frame.pitch>=6 && frame.timeMs==i*100 && frame.durationMs==100,"real dimensions/pitch/cadence");
            for(unsigned y=0;y<2;++y) for(unsigned x=0;x<3;++x) { UINT16 color=0; std::memcpy(&color,frame.pixels+y*frame.pitch+x*2,2); check(color==expected[i],"every literal decoded RGB565 pixel"); }
            auto resultB=b->ReadFrame(other); check(resultB.frame && other.timeMs==frame.timeMs,"independent VFS physical cursor restores");
        }
        check(a->ReadFrame(frame).eof && a->ReadFrame(frame).eof,"codec drain EOF stable");
        check(a->Rewind() && a->ReadFrame(frame).frame && frame.timeMs==0,"actual rewind recreates demux codec timeline");
        UINT16 first=0; std::memcpy(&first,frame.pixels,2); check(first==expected[0],"rewind exact first pixels");
        check(a->BufferedBytes()<40000,"single small RGB565 frame plus bounded AVIO");
        a.reset(); b.reset(); check(!NativeVideoDecoder::LiveInstances(),"decoder instances released");
        auto large=NativeVideoDecoder::OpenFile("large-movie.avi",error); check(large!=nullptr,"larger generated streaming movie opens");
        check(large->ReadFrame(frame).frame && large->BufferedBytes()<50000,"stream memory independent of total movie frames");
        check(large->SourceBytesRead()<std::filesystem::file_size(std::filesystem::path(argv[1])/"large-movie.avi"),"video frame arrives before complete source read");
        large.reset();
        auto corrupt=NativeVideoDecoder::OpenFile("bad-video-movie.avi",error); check(corrupt!=nullptr,"late corrupt movie opens valid prefix");
        bool decodeError=false; unsigned decoded=0;
        for(unsigned i=0;i<40;++i) { auto r=corrupt->ReadFrame(frame); if(r.error) { decodeError=true; break; } if(r.eof) break; if(r.frame) ++decoded; }
        check(decodeError && decoded>=20 && !corrupt->Error().empty(),"real late malformed frame yields error rather than EOF");
        check(corrupt->ReadFrame(frame).error,"movie decoder error remains sticky"); corrupt.reset();
        // The64MiB sparse selected-video packet is encountered by real AVI
        // probing; rejection can occur during open or the first actual read.
        auto probe=NativeVideoDecoder::OpenFile("oversize-probe-movie.avi",error);
        if(probe) {
            const auto result=probe->ReadFrame(frame);
            check(result.error && !result.eof,"oversized selected packet fails explicitly after probe");
            check(!probe->ReferencedPacketBytes(),"oversized selected packet immediately unreferenced");
        } else check(!error.empty(),"oversized probe allocation failure explicit");
        probe.reset(); check(!NativeVideoDecoder::LiveInstances(),"oversized probe decoder cleanup");
        auto skipped=NativeVideoDecoder::OpenFile("oversize-skipped-audio.avi",error);
        check(skipped!=nullptr,"oversized skipped audio fixture opens its legitimate video/audio prefix");
        bool skippedError=false; unsigned prefixFrames=0;
        const auto beforeSkipped=skipped->SourceBytesRead();
        for(unsigned i=0;i<40;++i) { const auto r=skipped->ReadFrame(frame); if(r.error) { skippedError=true; break; } if(r.eof) break; if(r.frame) ++prefixFrames; }
        check(skippedError && prefixFrames>=20 && !skipped->Error().empty(),"oversized skipped audio packet yields error rather than EOF");
        check(skipped->SourceBytesRead()-beforeSkipped<64u*1024u*1024u,"allocation rejection precedes complete oversized packet source read");
        check(!skipped->ReferencedPacketBytes() && skipped->ReadFrame(frame).error,"skipped packet released immediately and error sticky");
        skipped.reset(); check(!NativeVideoDecoder::LiveInstances(),"skipped oversized packet cleanup");
        check(!BinkPlayFlic("oversize-probe-movie.avi",0,0,BINK_FLIC_AUTOCLOSE) && !NativeVideoDecoder::LiveInstances(),"oversized probe never commits a playback slot");
        check(!NativeVideoDecoder::OpenFile("bad-movie.avi",error) && !error.empty(),"malformed movie fails explicitly");
        check(!NativeVideoDecoder::OpenFile("external-movie.avi",error) && !error.empty(),"external playlist denied by demux whitelist");
        check(!NativeVideoDecoder::OpenFile("absent.avi",error) && !error.empty(),"missing real VFS source rejected");
        check(InitializeVideoSurfaceManager(),"surface provider initialized");
        check(sgp::ConfigureNativeVideo(8,6,16,1,"P710 generated movie"),"native video fixture geometry");
        check(InitializeVideoManager(nullptr,0,nullptr),"real SDL presentation initialized");
        SoundEnableSound(FALSE); check(InitializeSoundManager(),"disabled sound nonfatal startup");
        BinkInitialize(nullptr,8,6); seed(); auto* f=play();
        check(NativeBinkPollAt(1000) && f->uiFrame==1,"first frame immediate");
        const auto drawn=pixel(1,1),borderA=pixel(0,0),borderB=pixel(4,1);
        if(drawn!=0xf800 || borderA!=0x1234 || borderB!=0x1234) {
            SGPRect clip{}; GetClippingRect(&clip);
            std::cerr<<"positioned frame actual RGB565: drawn="<<std::hex<<drawn<<" borderA="<<borderA<<" borderB="<<borderB<<std::dec
                     <<" clip="<<clip.iLeft<<","<<clip.iTop<<","<<clip.iRight<<","<<clip.iBottom<<"\n";
        }
        check(drawn==0xf800 && borderA==0x1234 && borderB==0x1234,"exact positioned frame and untouched border");
        check(NativeBinkPollAt(1099) && f->uiFrame==1 && pixel(1,1)==0xf800,"no early next frame");
        check(NativeBinkPollAt(1100) && f->uiFrame==2 && pixel(1,1)==0x07e0,"next frame exact100ms boundary");
        check(NativeBinkPollAt(1200) && f->uiFrame==3 && pixel(1,1)==0x001f,"final frame rendered");
        check(NativeBinkPollAt(1299) && (f->uiFlags&BINK_FLIC_OPEN),"last frame remains for its duration");
        check(NativeBinkPollAt(1300) && !f->uiFlags && !BinkPollFlics(),"autoclose and original poll pointer protocol");
        BinkCloseFlic(f); BinkCloseFlic(f); check(!NativeVideoDecoder::LiveInstances(),"Intro double close safe after autoclose");
        seed(); f=play(7,5); NativeBinkPollAt(0);
        check(pixel(7,5)==0xf800 && pixel(6,5)==0x1234 && pixel(7,4)==0x1234,"real surface right/bottom crop bounded"); BinkCloseFlic(f);
        seed(); f=play(1,99,BINK_FLIC_AUTOCLOSE|BINK_FLIC_CENTER_VERTICAL); NativeBinkPollAt(0);
        check(f->uiTop==2 && pixel(1,2)==0xf800 && pixel(1,1)==0x1234,"original vertical centering"); BinkCloseFlic(f);
        f=play(0,0,0); NativeBinkPollAt(0); NativeBinkPollAt(100); NativeBinkPollAt(200); NativeBinkPollAt(300);
        check((f->uiFlags&BINK_FLIC_LOOP) && f->uiFrame==1 && pixel(0,0)==0xf800,"non-autoclose actual rewind loop"); BinkCloseFlic(f);
        std::array<BINKFLIC*,4> slots{}; for(auto& s:slots) s=play();
        check(!BinkPlayFlic("movie.avi",0,0,0) && std::strlen(NativeBinkLastError()),"four slot limit explicit");
        for(auto* s:slots) BinkCloseFlic(s); check(!NativeVideoDecoder::LiveInstances(),"all four owned streams released");
        f=play(); UINT32 pitch=0; check(LockVideoSurface(FRAME_BUFFER,&pitch)!=nullptr,"foreign framebuffer lock acquired");
        NativeBinkPollAt(0); check(!f->uiFlags && std::strlen(NativeBinkLastError()),"locked frame fails without forged success"); UnLockVideoSurface(FRAME_BUFFER);
        // Clock wrap uses modular monotonic ticks, not wall-clock elapsed.
        f=play(); NativeBinkPollAt(0xfffffff0u); NativeBinkPollAt(84); check(f->uiFrame==2,"movie cadence across32-bit clock wrap"); BinkCloseFlic(f);
        // Disabled sound still permits a movie with an audio track.
        f=play(0,0,BINK_FLIC_AUTOCLOSE,"audio-movie.avi"); NativeBinkPollAt(0); check(f->uiFrame==1 && !NativeSoundActiveVoices(),"disabled audio movie renders silently"); BinkCloseFlic(f);
        ShutdownSoundManager(); SoundEnableSound(TRUE); NativeSoundSetManualRendering(TRUE);
        check(InitializeSoundManager(),"real SDL paused manual audio device");
        f=play(0,0,BINK_FLIC_AUTOCLOSE,"audio-movie.avi");
        check(NativeSoundActiveVoices()==1 && NativeAudioDecoder::LiveInstances()==1,"movie audio uses genuine streamed sound voice");
        std::array<float,960> pcm{}; check(NativeSoundRender(pcm.data(),480),"movie PCM through production mixer");
        bool audible=false; for(auto sample:pcm) if(sample!=0) audible=true; check(audible,"generated movie audio actually decoded");
        BinkCloseFlic(f); check(!NativeSoundActiveVoices() && !NativeAudioDecoder::LiveInstances(),"movie close stops and releases streamed audio");
        SoundEnableSound(FALSE); check(!NativeSoundPlaybackEnabled(),"runtime disabled sound state observable");
        f=play(0,0,BINK_FLIC_AUTOCLOSE,"audio-movie.avi"); NativeBinkPollAt(0);
        check(!NativeSoundActiveVoices() && f->uiFrame==1,"runtime disabled audio still permits genuine movie video"); BinkCloseFlic(f); SoundEnableSound(TRUE);
        SOUNDPARMS params; std::memset(&params,0xff,sizeof(params)); params.uiVolume=127; params.uiLoop=1; params.EOSCallback=nullptr; params.pCallbackData=nullptr;
        UINT32 id=SoundPlayStreamedFile(const_cast<char*>("audio-movie.avi"),&params);
        check(id!=SOUND_ERROR && NativeSoundGetStreamOutcome(id)==NativeSoundStreamOutcome::Playing,"actual stream outcome starts live");
        for(unsigned i=0;i<32;++i) { NativeSoundRender(pcm.data(),480); SoundServiceStreams(); }
        check(NativeSoundGetStreamOutcome(id)==NativeSoundStreamOutcome::Finished,"real EOF outcome survives EOS retirement");
        id=SoundPlayStreamedFile(const_cast<char*>("audio-movie.avi"),&params); check(id!=SOUND_ERROR,"second actual stream starts"); SoundStop(id);
        check(NativeSoundGetStreamOutcome(id)==NativeSoundStreamOutcome::Stopped,"explicit stop distinct from EOF");
        id=SoundPlayStreamedFile(const_cast<char*>("bad-audio-movie.avi"),&params); check(id!=SOUND_ERROR,"late corrupt PCM stream begins with valid bounded prefix");
        for(unsigned i=0;i<300 && NativeSoundGetStreamOutcome(id)==NativeSoundStreamOutcome::Playing;++i) { NativeSoundRender(pcm.data(),480); SoundServiceStreams(); }
        check(NativeSoundGetStreamOutcome(id)==NativeSoundStreamOutcome::Failed,"late decoder error distinct from EOF after retirement");
        f=play(0,0,BINK_FLIC_AUTOCLOSE,"bad-audio-movie.avi");
        for(unsigned i=0;i<300 && f->uiFlags;++i) { NativeSoundRender(pcm.data(),480); NativeBinkPollAt(i*10); }
        check(!f->uiFlags && std::strlen(NativeBinkLastError()),"movie observes real later streamed audio error");
        cleanup(); check(!NativeVideoDecoder::LiveInstances(),"movie cleanup complete");
        std::cout<<"native movie checks: "<<checks<<" (generated AVI only; Bink bitstream unverified)\n"; return 0;
    } catch(const std::exception& e) { std::cerr<<"native movie failure after "<<checks<<": "<<e.what()<<"\n"; cleanup(); return 1; }
}
