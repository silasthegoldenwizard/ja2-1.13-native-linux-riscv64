/*
 * P710 native snapshot update: tests/native-core/native_sound_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Independent RIFF/PCM fixtures authored here, without game files, a codec
// encoder, desktop audio, or a fixture decoded by the code under test.
#include "soundman.h"
#include "FileMan.h"
#include <SDL3/SDL.h>
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>
namespace fs=std::filesystem;
namespace {
unsigned checks=0;
void check(bool condition,const char* why) { ++checks; if(!condition) throw std::runtime_error(why); }
void near(float got,float expected,const char* why) { check(std::abs(got-expected)<0.00001f,why); }
void put16(std::vector<unsigned char>& b,unsigned value) { b.push_back(value&255); b.push_back((value>>8)&255); }
void put32(std::vector<unsigned char>& b,unsigned value) { put16(b,value&65535); put16(b,value>>16); }
void tag(std::vector<unsigned char>& b,const char* s) { b.insert(b.end(),s,s+4); }
std::vector<unsigned char> wav(unsigned frames=8) {
    std::vector<unsigned char> b;
    tag(b,"RIFF"); put32(b,36+frames*4); tag(b,"WAVE"); tag(b,"fmt "); put32(b,16);
    put16(b,1); put16(b,2); put32(b,48000); put32(b,192000); put16(b,4); put16(b,16);
    tag(b,"data"); put32(b,frames*4);
    for(unsigned i=0;i<frames;++i) { put16(b,8192); put16(b,16384); }
    return b;
}
SOUNDPARMS parms() { SOUNDPARMS p; std::memset(&p,0xff,sizeof(p)); p.uiLoop=1; p.uiVolume=127; return p; }
struct Temporary {
    fs::path root;
    Temporary() { char name[]="/tmp/p710-native-sound-XXXXXX"; char* p=mkdtemp(name); if(!p) throw std::runtime_error("mkdtemp"); root=p; }
    ~Temporary() { std::error_code ec; fs::remove_all(root,ec); }
};
void mount(const fs::path& root) {
    auto profile=std::make_unique<vfs::CVirtualProfile>("sound-fixtures",vfs::Path(root.u8string()),false);
    auto directory=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
    check(directory->init(),"real read-only VFS fixture tree initializes");
    auto* location=directory.get(); profile->addLocation(directory.release()); auto* p=profile.get();
    getVFS()->getProfileStack()->pushProfile(profile.release());
    check(getVFS()->addLocation(location,p),"real VFS fixture profile mounts");
}
std::thread::id mainThread;
unsigned eosCount=0;
UINT32 callbackSound=NO_SAMPLE;
void eos(void* data) { check(std::this_thread::get_id()==mainThread,"EOS remains on servicing thread"); ++eosCount; if(data) ++*static_cast<unsigned*>(data); }
void reentrant(void*) { ++eosCount; check(!SoundStop(callbackSound),"retired ID cannot dispatch callback twice"); }
void deterministic(const std::vector<unsigned char>& fixture) {
    NativeSoundSetManualRendering(TRUE);
    check(InitializeSoundManager() && SoundGetDriverHandle()!=nullptr,"real paused SDL dummy device stream opens");
    check(!NativeSoundLastError()[0],"initialization has no diagnostic");
    auto p=parms(); p.EOSCallback=eos; unsigned callbackData=0; p.pCallbackData=&callbackData;
    auto id=SoundPlayFromBuffer("independent.wav",fixture.data(),fixture.size(),&p);
    check(id!=SOUND_ERROR && SoundIsPlaying(id),"buffer sound starts");
    std::array<float,16> pcm{};
    check(NativeSoundRender(pcm.data(),8),"production PCM render path runs deterministically");
    for(unsigned i=0;i<8;++i) { near(pcm[i*2],0.25f*254/255,"independent left PCM expectation"); near(pcm[i*2+1],0.5f*254/255,"independent right PCM expectation"); }
    check(!SoundIsPlaying(id) && eosCount==0,"EOS deferred after exact last frame");
    SoundServiceStreams(); SoundServiceStreams();
    check(eosCount==1 && callbackData==1,"EOS exactly once with caller data");
    check(!SoundStop(id) && SoundGetVolume(id)==SOUND_ERROR,"expired IDs rejected");
    p=parms(); p.uiLoop=2; p.uiPan=0; p.uiVolume=63;
    id=SoundPlayFromBuffer("loop.wav",fixture.data(),fixture.size(),&p);
    std::array<float,32> twice{}; NativeSoundRender(twice.data(),16);
    for(unsigned i=0;i<16;++i) { near(twice[i*2],0.25f*126/255,"finite loop and gain"); near(twice[i*2+1],0,"left pan mutes right"); }
    check(!SoundIsPlaying(id),"two complete iterations expire"); SoundServiceStreams();
    p=parms(); p.uiLoop=0;
    id=SoundPlayFromBuffer("infinite.wav",fixture.data(),fixture.size(),&p);
    check(SoundSetPan(id,255) && SoundSetVolume(id,127),"live pan and volume controls");
    NativeSoundRender(twice.data(),16);
    for(unsigned i=0;i<16;++i) { near(twice[i*2],0,"right pan mutes left"); near(twice[i*2+1],0.5f*254/255,"right output preserved"); }
    check(SoundIsPlaying(id),"zero loop count remains infinite"); check(SoundStop(id),"explicit stop");
    p=parms(); p.uiSpeed=96000;
    id=SoundPlayFromBuffer("rate.wav",fixture.data(),fixture.size(),&p);
    NativeSoundRender(pcm.data(),4); check(SoundIsPlaying(id),"original ignored speed preserves source timing");
    NativeSoundRender(pcm.data(),4); check(!SoundIsPlaying(id),"ignored speed finishes at original eight frames"); SoundServiceStreams();
    p=parms(); p.EOSCallback=reentrant;
    callbackSound=SoundPlayFromBuffer("reentrant.wav",fixture.data(),fixture.size(),&p);
    check(SoundStop(callbackSound) && eosCount==2,"stop callback can reenter manager");
    p=parms(); p.uiLoop=0;
    std::array<UINT32,128> ids{};
    for(auto& sound:ids) { sound=SoundPlayFromBuffer("bound.wav",fixture.data(),fixture.size(),&p); check(sound!=SOUND_ERROR,"bounded voice allocated"); }
    check(NativeSoundActiveVoices()==128,"128 active voices exactly");
    check(SoundPlayFromBuffer("overflow.wav",fixture.data(),fixture.size(),&p)==SOUND_ERROR,"129th voice is explicit failure");
    NativeSoundRender(pcm.data(),1); near(pcm[0],1,"mixer clips summed left"); near(pcm[1],1,"mixer clips summed right");
    SoundStopAll(); check(NativeSoundActiveVoices()==0,"stop all releases voices");
    check(SoundPlayFromBuffer("compressed.ogg","OggSnot-a-wave",13,&p)==SOUND_ERROR,"unsupported compressed format explicitly fails");
    check(std::strstr(NativeSoundLastError(),"MP3/OGG")!=nullptr,"decoder gap diagnostic is specific");
    std::vector<unsigned char> malformed=fixture; malformed.resize(20);
    check(SoundPlayFromBuffer("short.wav",malformed.data(),malformed.size(),&p)==SOUND_ERROR,"truncated WAV rejected");
    check(SoundPlayFromBuffer("null",nullptr,100,&p)==SOUND_ERROR,"null source rejected");
    SoundSetDefaultVolume(200); check(SoundGetDefaultVolume()==127,"default gain clamps");
    ShutdownSoundManager(); check(!SoundGetDriverHandle() && NativeSoundCachedBytes()==0,"shutdown releases device/cache");
}
void positionWallClock(const std::vector<unsigned char>& fixture) {
    NativeSoundSetManualRendering(TRUE);
    check(InitializeSoundManager(),"position test initializes paused dummy device");
    auto p=parms(); p.uiLoop=0;
    const auto before=static_cast<UINT32>(SDL_GetTicks());
    const auto id=SoundPlayFromBuffer("position.wav",fixture.data(),fixture.size(),&p);
    check(id!=SOUND_ERROR,"position test voice starts");
    const auto startedBy=static_cast<UINT32>(SDL_GetTicks());
    const auto first=SoundGetPosition(id);
    SDL_Delay(25);
    const auto observedBefore=static_cast<UINT32>(SDL_GetTicks());
    const auto position=SoundGetPosition(id);
    const auto observedAt=static_cast<UINT32>(SDL_GetTicks());
    check(position>=25 && position>=first,"position advances with wall time while paused and with no PCM rendered");
    check(position>=static_cast<UINT32>(observedBefore-startedBy) &&
          position<=static_cast<UINT32>(observedAt-before),
          "position is bounded by independently observed voice start wall times");
    check(SoundGetPosition(NO_SAMPLE)==0 && SoundGetPosition(id+1)==0,"invalid and unknown position IDs return zero");
    check(SoundStop(id) && SoundGetPosition(id)==0,"stopped retired position ID returns zero");
    p=parms();
    const auto finished=SoundPlayFromBuffer("finished-position.wav",fixture.data(),fixture.size(),&p);
    std::array<float,16> pcm{}; NativeSoundRender(pcm.data(),8);
    check(!SoundIsPlaying(finished),"position fixture finishes through independent PCM rendering");
    SDL_Delay(20);
    check(SoundGetPosition(finished)>=20,"finished unretired voice retains original wall-clock position");
    SoundServiceStreams();
    check(SoundGetPosition(finished)==0,"stream-service retirement clears position ID");
    ShutdownSoundManager();
}
void fileCacheAndRandom() {
    NativeSoundSetManualRendering(TRUE); check(InitializeSoundManager(),"cache test initialization");
    char name[]="tone.wav", alias[]="TONE.WAV";
    const auto sample=SoundLoadSample(name);
    check(sample!=NO_SAMPLE && NativeSoundCachedBytes()==64,"FileMan/VFS source decoded into64-byte PCM cache");
    check(SoundLoadSample(alias)==sample,"cache names preserve case-insensitive lookup");
    check(!SoundSetMemoryLimit(63) && SoundSetMemoryLimit(64),"memory limit cannot undercut live cache");
    check(SoundLockSample(name)==sample,"cache lock");
    check(SoundUnlockSample(name)==sample,"cache unlock");
    check(SoundLockSample(name)==sample,"cache relock"); SoundEmptyCache();
    check(NativeSoundCachedBytes()==0,"explicit empty preserves original clearing even locked samples");
    check(SoundSetMemoryLimit(63) && SoundLoadSample(name)==NO_SAMPLE,"decoded cache accounting rejects sample over budget");
    check(SoundSetMemoryLimit(1024),"restore cache budget");
    auto p=parms(); auto id=SoundPlay(name,&p);
    check(id!=SOUND_ERROR && SoundFreeSample(name)==NO_SAMPLE,"active cache owner prevents free");
    SoundStop(id); check(SoundFreeSample(name)!=NO_SAMPLE,"stopped cached sample can be freed");
    check(SoundSetCacheThreshhold(77),"threshold configured just above76-byte source");
    id=SoundPlay(name,&p); check(id!=SOUND_ERROR,"threshold-minus-one source remains cached playback"); SoundStop(id);
    check(SoundSetCacheThreshhold(76) && SoundPlay(name,&p)==SOUND_ERROR,"exact threshold source requires streamed API");
    check(SoundSetCacheThreshhold(0),"zero threshold resets default");
    id=SoundPlay(name,&p); check(id!=SOUND_ERROR,"default reset admits a normal nonempty cached source"); SoundStop(id);
    SoundSetCacheThreshhold(1); check(SoundPlay(name,&p)==SOUND_ERROR,"cache threshold preserves streamed API distinction");
    id=SoundPlayStreamedFile(name,&p); check(id!=SOUND_ERROR,"streamed filename uses actual VFS source"); SoundStop(id);
    SoundSetCacheThreshhold(256*8024);
    RANDOMPARMS random; std::memset(&random,0xff,sizeof(random)); random.uiTimeMin=0; random.uiTimeMax=0;
    random.uiVolMin=random.uiVolMax=127; random.uiMaxInstances=1;
    const auto registered=SoundPlayRandom(name,&random); check(registered!=NO_SAMPLE,"random WAV registered");
    check(SoundServiceRandom() && NativeSoundActiveVoices()==1,"due random sample starts");
    check(!SoundServiceRandom() && NativeSoundActiveVoices()==1,"random max instance bound");
    std::array<float,16> pcm{}; NativeSoundRender(pcm.data(),8);
    check(SoundServiceRandom() && NativeSoundActiveVoices()==1,"completed random voice retires then restarts");
    SoundStopAllRandom(); check(NativeSoundActiveVoices()==0 && !SoundServiceRandom(),"stop random clears registration and voice");
    const auto manualSample=SoundPlayRandom(name,&random); SoundSetSampleFlags(manualSample,SAMPLE_RANDOM_MANUAL);
    check(!SoundServiceRandom(),"manual random flag suppresses service");
    SoundRemoveSampleFlags(manualSample,SAMPLE_RANDOM_MANUAL); check(SoundServiceRandom(),"manual flag removal resumes registration");
    SoundStopAllRandom();
    NativeSoundSetSimultaneousLimit(TRUE); ResetSoundMap();
    id=SoundPlay(name,&p); check(id!=SOUND_ERROR,"first option-limited sound plays");
    check(SoundPlay(name,&p)==0,"original50ms same-name throttle returns zero");
    ResetSoundMap(); check(SoundPlay(name,&p)!=SOUND_ERROR,"reset clears same-name throttle");
    NativeSoundSetSimultaneousLimit(FALSE); SoundStopAll(); ShutdownSoundManager();
}
void allocationBudget() {
    constexpr unsigned MiB=1024u*1024u;
    NativeSoundSetManualRendering(TRUE);
    check(InitializeSoundManager(),"allocation-budget fixture initializes paused dummy device");
    auto p=parms(); p.uiLoop=0;
    // These decoded payloads together fit64MiB, but the first crosses a
    // geometric allocation boundary and retains64MiB by itself.
    const auto first=wav((32u*MiB+16384u)/8u);
    const auto id=SoundPlayFromBuffer("capacity-boundary.wav",first.data(),first.size(),&p);
    check(id!=SOUND_ERROR,"valid PCM just above32MiB is admitted");
    check(NativeSoundRetainedBytes()==64u*MiB,"retained accounting includes unused capacity above payload");
    const auto second=wav(31u*MiB/8u);
    check(SoundPlayFromBuffer("aggregate-boundary.wav",second.data(),second.size(),&p)==SOUND_ERROR,
          "second payload rejected against actual aggregate allocation");
    check(NativeSoundRetainedBytes()==64u*MiB && NativeSoundActiveVoices()==1,
          "failed admission retains only the first64MiB voice");
    check(SoundStop(id) && NativeSoundRetainedBytes()==0,"voice retirement releases all retained PCM capacity");
    ShutdownSoundManager();
}
void deviceLifecycle() {
    NativeSoundSetManualRendering(FALSE); check(InitializeSoundManager(),"real running SDL dummy stream opens");
    const auto fixture=wav(4800); auto p=parms(); unsigned callbackData=0; p.EOSCallback=eos; p.pCallbackData=&callbackData;
    const auto id=SoundPlayFromBuffer("dummy-lifecycle.wav",fixture.data(),fixture.size(),&p);
    check(id!=NO_SAMPLE,"real stream lifecycle voice starts");
    const auto deadline=SDL_GetTicks()+2000;
    while(SoundIsPlaying(id) && SDL_GetTicks()<deadline) SDL_Delay(5);
    check(!SoundIsPlaying(id),"SDL audio callback consumes PCM under dummy device");
    check(callbackData==0,"device callback never calls game EOS");
    SoundServiceStreams(); check(callbackData==1,"main service observes real stream completion");
    ShutdownSoundManager();
    SoundEnableSound(FALSE); check(InitializeSoundManager() && !SoundGetDriverHandle(),"disabled startup is successful with no device");
    check(SoundPlayFromBuffer("disabled",fixture.data(),fixture.size(),&p)==SOUND_ERROR,"disabled manager cannot play");
    ShutdownSoundManager(); SoundEnableSound(TRUE);
}
}
int main() {
    try {
        // Set before any audio initialization: test must never touch host speakers.
        check(SDL_SetEnvironmentVariable(SDL_GetEnvironment(),"SDL_AUDIO_DRIVER","dummy",true),"dummy audio driver selected");
        mainThread=std::this_thread::get_id(); Temporary temporary;
        const auto fixture=wav(); std::ofstream file(temporary.root/"tone.wav",std::ios::binary);
        file.write(reinterpret_cast<const char*>(fixture.data()),fixture.size()); file.close(); check(bool(file),"independent WAV fixture written");
        mount(temporary.root); check(InitializeFileManager(nullptr),"FileMan services initialize");
        deterministic(fixture); positionWallClock(fixture); fileCacheAndRandom(); allocationBudget(); deviceLifecycle();
        ShutdownFileManager();
        std::cout<<"native_sound_tests: "<<checks<<" checks passed; SDL dummy only; physical audio unverified\n";
        return 0;
    } catch(const std::exception& e) { ShutdownSoundManager(); std::cerr<<"native sound failure: "<<e.what()<<"; "<<NativeSoundLastError()<<'\n'; return 1; }
}
