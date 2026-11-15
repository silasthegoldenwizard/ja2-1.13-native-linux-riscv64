/*
 * P710 native snapshot update: tests/native-core/native_audio_decoder_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Generated, non-game media only. Host ffmpeg is an explicit TEST fixture
// encoder, never a game/runtime dependency. Product decoding uses the pinned
// SDK and real FileMan/VFS custom AVIO. All playback uses SDL dummy audio.
#include "native_audio_decoder.h"
#include "native_soundman.h"
#include "FileMan.h"
#include <SDL3/SDL.h>
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <spawn.h>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>
extern char** environ;
namespace fs=std::filesystem;
namespace {
constexpr unsigned Rate=48000, Frames=48000;
constexpr double Pi=3.14159265358979323846;
unsigned checks=0;
void check(bool condition,const char* why) { ++checks; if(!condition) throw std::runtime_error(why); }
void closeEnough(float got,float expected,const char* why) { check(std::abs(got-expected)<0.000002f,why); }
void put16(std::vector<UINT8>& b,UINT32 n) { b.push_back(n&255); b.push_back((n>>8)&255); }
void put32(std::vector<UINT8>& b,UINT32 n) { put16(b,n&65535); put16(b,n>>16); }
void tag(std::vector<UINT8>& b,const char* s) { b.insert(b.end(),s,s+4); }
std::vector<UINT8> header(UINT32 frames,unsigned rate=Rate) {
    std::vector<UINT8> b; tag(b,"RIFF"); put32(b,36+frames*4); tag(b,"WAVE"); tag(b,"fmt "); put32(b,16);
    put16(b,1); put16(b,2); put32(b,rate); put32(b,rate*4); put16(b,4); put16(b,16); tag(b,"data"); put32(b,frames*4); return b;
}
INT16 value(unsigned frame,unsigned channel,unsigned rate=Rate) {
    return static_cast<INT16>(std::lround((channel?12000.0:8000.0)*std::sin(2*Pi*(channel?880.0:440.0)*frame/rate)));
}
std::vector<UINT8> wave(unsigned frames=Frames,unsigned rate=Rate) {
    auto bytes=header(frames,rate);
    for(unsigned i=0;i<frames;++i) for(unsigned c=0;c<2;++c) put16(bytes,static_cast<UINT16>(value(i,c,rate)));
    return bytes;
}
struct Temporary {
    fs::path root;
    Temporary() { char name[]="/tmp/p710-ffmpeg-audio-XXXXXX"; char* p=mkdtemp(name); if(!p) throw std::runtime_error("unique fixture directory"); root=p; }
    ~Temporary() { std::error_code ec; fs::remove_all(root,ec); }
};
void write(const fs::path& name,const std::vector<UINT8>& data) {
    std::ofstream out(name,std::ios::binary); out.write(reinterpret_cast<const char*>(data.data()),data.size());
    check(bool(out),"independent fixture bytes written");
}
std::vector<UINT8> read(const fs::path& name) { std::ifstream in(name,std::ios::binary); return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()}; }
void encode(const std::string& executable,const fs::path& input,const fs::path& output,const char* codec) {
    // Exact argument array: no shell, URLs, inherited game assets, or encoder in
    // production. The existing host CLI only authors these independent fixtures.
    std::vector<std::string> args{executable,"-nostdin","-v","error","-y","-protocol_whitelist","file,pipe","-i",input.string(),"-map","0:a:0","-c:a",codec};
    if(std::string(codec)=="libmp3lame") { args.emplace_back("-b:a"); args.emplace_back("192k"); }
    else { args.emplace_back("-q:a"); args.emplace_back("5"); }
    args.push_back(output.string());
    std::vector<char*> pointers; for(auto& arg:args) pointers.push_back(arg.data()); pointers.push_back(nullptr);
    pid_t child=0;
    check(posix_spawn(&child,executable.c_str(),nullptr,nullptr,pointers.data(),environ)==0,"authorized host fixture encoder starts");
    int status=0; check(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0,"host encoder creates independent compressed fixture");
    check(fs::file_size(output)>0,"encoded fixture is nonempty");
}
void mount(const fs::path& root) {
    auto profile=std::make_unique<vfs::CVirtualProfile>("audio-generated",vfs::Path(root.u8string()),false);
    auto directory=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
    check(directory->init(),"independent media VFS tree initializes");
    auto* location=directory.get(); profile->addLocation(directory.release()); auto* p=profile.get();
    getVFS()->getProfileStack()->pushProfile(profile.release()); check(getVFS()->addLocation(location,p),"independent media profile mounts");
}
std::unique_ptr<NativeAudioDecoder> open(const char* name) {
    std::string error; auto decoder=NativeAudioDecoder::OpenFile(name,error);
    if(!decoder) throw std::runtime_error(std::string("decoder open ")+name+": "+error);
    return decoder;
}
std::vector<float> decode(NativeAudioDecoder& decoder,UINT32 chunk=733) {
    std::array<float,NativeAudioDecoder::MaxReadFrames*2> buffer{};
    std::vector<float> result;
    for(unsigned calls=0;calls<4096;++calls) {
        const auto readResult=decoder.ReadFrames(buffer.data(),chunk);
        if(readResult.error) throw std::runtime_error("decoder read: "+decoder.Error());
        check(readResult.frames<=chunk,"decoder honors requested frame capacity");
        check(decoder.BufferedBytes()<=32768+65536*2*sizeof(float),"decoder adapter staging remains within bound");
        for(unsigned i=0;i<readResult.frames*2;++i) check(std::isfinite(buffer[i]),"decoded PCM is finite");
        result.insert(result.end(),buffer.begin(),buffer.begin()+readResult.frames*2);
        if(readResult.eof) {
            const auto drained=decoder.ReadFrames(buffer.data(),chunk);
            check(drained.eof && !drained.frames && !drained.error,"codec/resampler EOF remains drained");
            return result;
        }
        check(readResult.frames>0,"decoder progresses until explicit EOF");
    }
    throw std::runtime_error("decoder failed bounded fixture completion");
}
void lossySignal(const std::vector<float>& pcm) {
    check(pcm.size()/2>=Frames-2048 && pcm.size()/2<=Frames+2048,"compressed duration preserves authored signal");
    const std::size_t end=pcm.size()/2-2048, begin=2048;
    for(unsigned channel=0;channel<2;++channel) {
        double squares=0; unsigned crossings=0;
        for(std::size_t i=begin;i<end;++i) {
            const auto sample=pcm[i*2+channel]; squares+=sample*sample;
            if(i>begin && pcm[(i-1)*2+channel]<=0 && sample>0) ++crossings;
        }
        const double rms=std::sqrt(squares/(end-begin));
        const double expected=(channel?12000.0:8000.0)/32768/std::sqrt(2.0);
        check(std::abs(rms-expected)<expected*0.08,"lossy decoder retains independent channel RMS amplitude");
        const double frequency=static_cast<double>(crossings)*Rate/(end-begin);
        check(std::abs(frequency-(channel?880.0:440.0))<3.0,"lossy decoder retains independently authored tone frequency");
    }
}
std::vector<float> verifyFormat(const char* name,bool exact) {
    auto decoder=open(name);
    check(NativeAudioDecoder::LiveInstances()==1,"one decoder owns one independent source");
    const auto first=decode(*decoder);
    check(first.size()>0 && decoder->SourceBytesRead()>0,"actual encoded bytes produce PCM");
    if(exact) {
        check(first.size()==Frames*2,"PCM fixture frame count is exact");
        for(unsigned i=0;i<Frames;++i) for(unsigned c=0;c<2;++c)
            closeEnough(first[i*2+c],value(i,c)/32768.0f,"PCM matches independently calculated waveform");
    } else lossySignal(first);
    const auto priorReads=decoder->SourceBytesRead();
    check(decoder->Rewind(),"decoder can seek/reset original source");
    const auto second=decode(*decoder,137);
    check(first==second,"full demux/codec/resampler reset repeats identical decoded PCM");
    check(decoder->SourceBytesRead()>priorReads,"rewind actually reads the owned source again");
    decoder.reset(); check(NativeAudioDecoder::LiveInstances()==0,"decoder release closes all owned state");
    return first;
}
void cursorIsolation(const std::vector<float>& reference) {
    auto a=open("tone.mp3"); auto b=open("tone.mp3");
    check(NativeAudioDecoder::LiveInstances()==2,"same VFS path gets independent decoder instances");
    std::array<float,2048> one{},two{};
    auto ra=a->ReadFrames(one.data(),123); auto rb=b->ReadFrames(two.data(),257);
    check(ra.frames==123 && rb.frames==257 && !ra.error && !rb.error,"interleaved decoders advance independently");
    for(unsigned i=0;i<246;++i) closeEnough(one[i],reference[i],"first source starts at its own zero");
    for(unsigned i=0;i<514;++i) closeEnough(two[i],reference[i],"second source starts at its own zero");
    // Disturb the VFS object's physical position through its ORIGINAL public
    // registry. Custom AVIO must honor its own logical cursor on the next read.
    char name[]="tone.mp3"; const auto disturbed=FileOpen(name,FILE_ACCESS_READ,FALSE);
    check(disturbed!=0 && FileSeek(disturbed,17,FILE_SEEK_FROM_START),"independent public FileMan cursor disturbance");
    char garbage[13]; UINT32 readCount=0; check(FileRead(disturbed,garbage,sizeof(garbage),&readCount),"disturbance reads actual encoded bytes");
    FileClose(disturbed);
    ra=a->ReadFrames(one.data(),499); check(ra.frames==499 && !ra.error,"first decoder survives external cursor movement");
    for(unsigned i=0;i<998;++i) closeEnough(one[i],reference[246+i],"first decoder preserves its logical PCM continuation");
    a.reset();
    const auto remaining=decode(*b,251);
    check(remaining.size()==reference.size()-514,"closing first VFS alias retains second source lifetime");
    for(std::size_t i=0;i<remaining.size();++i) closeEnough(remaining[i],reference[514+i],"second decoder continuation is isolated through EOF");
    b.reset(); check(NativeAudioDecoder::LiveInstances()==0,"all source aliases released");
}
SOUNDPARMS parms() { SOUNDPARMS p; std::memset(&p,0xff,sizeof(p)); p.uiVolume=127; p.uiLoop=1; return p; }
unsigned eosCount=0;
std::thread::id mainThread;
void eos(void*) { check(std::this_thread::get_id()==mainThread,"stream EOS stays on main service thread"); ++eosCount; }
void streamingLoop(const char* name,const std::vector<float>& reference) {
    auto p=parms(); p.uiLoop=2; p.EOSCallback=eos;
    const auto countBefore=eosCount;
    const auto id=SoundPlayStreamedFile(const_cast<char*>(name),&p);
    check(id!=SOUND_ERROR,"actual WAV/MP3/OGG streamed voice starts");
    std::vector<float> pcm;
    std::array<float,257*2> block{};
    for(unsigned calls=0;calls<2048 && SoundIsPlaying(id);++calls) {
        SoundServiceStreams(); if(!SoundIsPlaying(id)) break;
        const auto ready=NativeSoundStreamBufferedFrames(id);
        check(ready>0 && ready<=16384,"stream publishes a bounded finite ring");
        const auto count=std::min(ready,257u);
        const auto reads=NativeSoundStreamSourceBytesRead(id);
        check(NativeSoundRender(block.data(),count),"production stream PCM transport renders");
        check(NativeSoundStreamSourceBytesRead(id)==reads,"render callback performs no source I/O");
        pcm.insert(pcm.end(),block.begin(),block.begin()+count*2);
    }
    check(!SoundIsPlaying(id),"finite streamed loops reach EOF after codec/resampler drain");
    check(eosCount==countBefore,"render completion defers EOS until service");
    check(pcm.size()==reference.size()*2,"two loops preserve all decoded frames including drain tails");
    for(std::size_t i=0;i<pcm.size();++i) closeEnough(pcm[i],reference[i%reference.size()]*254/255,"ring/seek/loop PCM matches independent decoder result");
    SoundServiceStreams(); SoundServiceStreams();
    check(eosCount==countBefore+1,"stream EOS dispatches exactly once");
    check(NativeAudioDecoder::LiveInstances()==0,"EOF retirement frees stream decoder/source");
}
void largeStreaming() {
    char name[]="large.wav"; auto p=parms();
    const auto id=SoundPlayStreamedFile(name,&p);
    check(id!=SOUND_ERROR,"greater-than64MiB source starts incrementally");
    check(NativeSoundCachedBytes()==0,"large stream does not enter full decoded cache");
    check(NativeSoundStreamBufferedBytes(id)<1024*1024,"large source retains less-than1MiB adapter-owned PCM/AVIO buffers");
    check(NativeSoundStreamSourceBytesRead(id)<1024*1024,"large source opening/prefill reads only a bounded prefix/probe");
    std::array<float,4096*2> block{};
    const auto initialReads=NativeSoundStreamSourceBytesRead(id);
    bool refilled=false;
    for(unsigned i=0;i<8;++i) {
        const auto before=NativeSoundStreamSourceBytesRead(id);
        check(NativeSoundRender(block.data(),4096),"large stream transports bounded chunk");
        check(NativeSoundStreamSourceBytesRead(id)==before,"large stream render never touches source cursor");
        SoundServiceStreams();
        refilled|=NativeSoundStreamSourceBytesRead(id)>initialReads;
        check(NativeSoundStreamBufferedFrames(id)<=16384,"stream remains bounded across ring wraps");
    }
    check(refilled,"main service actually refills incremental source beyond initial ring");
    const auto before=NativeSoundStreamSourceBytesRead(id);
    std::vector<float> starved(32768*2);
    NativeSoundRender(starved.data(),32768);
    check(SoundIsPlaying(id) && NativeSoundStreamUnderruns(id)>0,"starvation is live observable underflow rather than false EOF");
    check(NativeSoundStreamSourceBytesRead(id)==before,"underflow does not trigger realtime decode/read");
    SoundServiceStreams(); check(NativeSoundStreamBufferedFrames(id)>0,"main-thread service recovers starved transport");
    check(SoundStop(id),"early stream stop releases a large owned source");
    check(NativeAudioDecoder::LiveInstances()==0,"early stop closes decoder and AVIO");
}
void automaticDevice() {
    ShutdownSoundManager(); NativeSoundSetManualRendering(FALSE); check(InitializeSoundManager(),"actual running dummy device initialized");
    char name[]="tone.ogg"; auto p=parms(); p.EOSCallback=eos;
    const auto countBefore=eosCount; const auto id=SoundPlayStreamedFile(name,&p);
    check(id!=SOUND_ERROR,"compressed stream starts on real SDL dummy transport");
    const auto initialReads=NativeSoundStreamSourceBytesRead(id);
    SDL_Delay(60);
    check(NativeSoundStreamSourceBytesRead(id)==initialReads,"actual SDL thread does not invoke decoder/source callbacks");
    const auto deadline=SDL_GetTicks()+4000;
    while(SoundIsPlaying(id) && SDL_GetTicks()<deadline) { SoundServiceStreams(); SDL_Delay(5); }
    SoundServiceStreams();
    check(!SoundIsPlaying(id) && eosCount==countBefore+1,"actual compressed SDL dummy lifecycle drains then dispatches main-thread EOS");
    ShutdownSoundManager();
}
void failures(const fs::path& root) {
    std::string error;
    const char bad[]="not encoded audio";
    check(!NativeAudioDecoder::OpenMemory(bad,sizeof(bad),error) && !error.empty(),"malformed memory is explicit decoder failure");
    check(!NativeAudioDecoder::OpenMemory(nullptr,17,error),"null input fails without decoder ownership");
    check(!NativeAudioDecoder::OpenFile("missing.ogg",error),"missing VFS source explicitly fails");
    const auto bytes=read(root/"tone.mp3");
    auto memory=NativeAudioDecoder::OpenMemory(bytes.data(),bytes.size(),error);
    check(bool(memory),"encoded caller buffer opens via custom memory AVIO");
    std::array<float,2> tiny{};
    check(memory->ReadFrames(tiny.data(),NativeAudioDecoder::MaxReadFrames+1).error,"oversized caller request fails before writing");
    memory.reset();
    check(NativeAudioDecoder::LiveInstances()==0,"failed reads release every decoder instance");
    for(unsigned i=0;i<8;++i) { auto owned=open("tone.ogg"); check(owned->Rewind(),"repeated ownership/reset succeeds"); }
    check(NativeAudioDecoder::LiveInstances()==0,"repeated ownership/reset has no retained instances");
}
}
int main(int argc,char** argv) {
    try {
        const std::string encoder=argc>1?argv[1]:"/usr/bin/ffmpeg";
        check(argc<=2,"CLI accepts only explicit host fixture encoder path");
        check(SDL_SetEnvironmentVariable(SDL_GetEnvironment(),"SDL_AUDIO_DRIVER","dummy",true),"dummy audio exclusively selected");
        mainThread=std::this_thread::get_id(); Temporary temporary;
        write(temporary.root/"tone.wav",wave());
        write(temporary.root/"resample.wav",wave(44100,44100));
        encode(encoder,temporary.root/"tone.wav",temporary.root/"tone.mp3","libmp3lame");
        encode(encoder,temporary.root/"tone.wav",temporary.root/"tone.ogg","libvorbis");
        // A valid sparse70MiB WAV proves source-size independence without large
        // fixture allocations. Initial samples are independently authored PCM.
        constexpr UINT32 largeBytes=70u*1024u*1024u;
        std::ofstream large(temporary.root/"large.wav",std::ios::binary);
        const auto prefix=header(largeBytes/4); const auto seed=wave(4096);
        large.write(reinterpret_cast<const char*>(prefix.data()),prefix.size());
        large.write(reinterpret_cast<const char*>(seed.data()+44),seed.size()-44);
        large.seekp(44+largeBytes-1); large.put(0); large.close(); check(bool(large),"large sparse WAV fixture authored");
        mount(temporary.root); check(InitializeFileManager(nullptr),"original FileMan services initialize");
        const auto wav=verifyFormat("tone.wav",true);
        const auto mp3=verifyFormat("tone.mp3",false);
        const auto ogg=verifyFormat("tone.ogg",false);
        auto resampled=open("resample.wav"); const auto resampledPCM=decode(*resampled); resampled.reset();
        check(resampledPCM.size()==Frames*2,"44.1k codec plus resampler drain yields exact48k duration"); lossySignal(resampledPCM);
        cursorIsolation(mp3); failures(temporary.root);
        NativeSoundSetManualRendering(TRUE); check(InitializeSoundManager(),"real paused SDL dummy stream initializes");
        streamingLoop("tone.wav",wav); streamingLoop("tone.mp3",mp3); streamingLoop("tone.ogg",ogg);
        largeStreaming(); automaticDevice();
        check(NativeAudioDecoder::LiveInstances()==0,"final shutdown releases all decoder ownership"); ShutdownFileManager();
        std::cout<<"native_audio_decoder_tests: "<<checks<<" checks passed; WAV/MP3/OGG custom AVIO, incremental ring, SDL dummy; physical audio unverified\n";
        return 0;
    } catch(const std::exception& e) { ShutdownSoundManager(); std::cerr<<"native decoder failure: "<<e.what()<<"; "<<NativeSoundLastError()<<'\n'; return 1; }
}
