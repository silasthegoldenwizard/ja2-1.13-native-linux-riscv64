/*
 * P710 native snapshot update: tests/native-core/native_dialogue_audio_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Authored PCM/GAP fixtures exercise the original GAP APIs and real native
// FileMan/bfVFS/streamed audio. No game assets, engine stubs, or host speakers.
#include "GAP.H"
#include "Sound Control.h"
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
#include <vector>
namespace fs = std::filesystem;
namespace {
unsigned checks = 0;
void check(bool value, const char* why) { ++checks; if (!value) throw std::runtime_error(why); }
void put16(std::vector<unsigned char>& b, unsigned v) { b.push_back(v & 255); b.push_back((v >> 8) & 255); }
void put32(std::vector<unsigned char>& b, unsigned v) { put16(b, v & 65535); put16(b, v >> 16); }
void tag(std::vector<unsigned char>& b, const char* text) { b.insert(b.end(), text, text + 4); }
std::vector<unsigned char> wave() {
    // 0.1s stereo PCM, independent constant amplitudes for gain/pan checks.
    constexpr unsigned frames = 4800;
    std::vector<unsigned char> b;
    tag(b,"RIFF"); put32(b,36 + frames*4); tag(b,"WAVE"); tag(b,"fmt "); put32(b,16);
    put16(b,1); put16(b,2); put32(b,48000); put32(b,192000); put16(b,4); put16(b,16);
    tag(b,"data"); put32(b,frames*4);
    for (unsigned i=0;i<frames;++i) { put16(b,8192); put16(b,16384); }
    return b;
}
std::vector<unsigned char> gaps(std::initializer_list<std::pair<unsigned,unsigned>> records) {
    std::vector<unsigned char> b;
    for (const auto& r:records) { put32(b,r.first); put32(b,r.second); }
    return b;
}
struct Temporary {
    fs::path root;
    Temporary() { char path[]="/tmp/p710-dialogue-audio-XXXXXX"; char* p=mkdtemp(path); if(!p) throw std::runtime_error("temporary directory"); root=p; }
    ~Temporary() { std::error_code ec; fs::remove_all(root,ec); }
};
void write(const fs::path& path, const std::vector<unsigned char>& bytes) {
    fs::create_directories(path.parent_path());
    std::ofstream stream(path,std::ios::binary);
    stream.write(reinterpret_cast<const char*>(bytes.data()), bytes.size()); stream.close();
    check(bool(stream),"authored fixture written");
}
void mount(const fs::path& root) {
    auto profile=std::make_unique<vfs::CVirtualProfile>("dialogue-fixtures",vfs::Path(root.u8string()),false);
    auto directory=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
    check(directory->init(),"actual read-only VFS fixture tree initializes");
    auto* location=directory.get(); profile->addLocation(directory.release()); auto* p=profile.get();
    getVFS()->getProfileStack()->pushProfile(profile.release());
    check(getVFS()->addLocation(location,p),"actual VFS fixture profile mounts");
}
void empty(const AudioGapList& list,const char* reason) {
    check(!list.size && !list.pHead && !list.pCurrent && !list.current_time && !list.audio_gap_active,reason);
}
void parser() {
    AudioGapList list{};
    char valid[]="speech.wav", second[]="second.wav";
    AudioGapListInit(valid,&list);
    check(list.size==2 && list.pCurrent==list.pHead,"two actual GAP records loaded");
    check(list.pHead->uiStart==5 && list.pHead->uiEnd==10000 && list.pHead->pNext->uiStart==20000 && list.pHead->pNext->uiEnd==30000,"explicit little-endian fixture fields decode");
    // Faces replaces GAP data between queued utterances without Done.
    for(unsigned i=0;i<20;++i) {
        AudioGapListInit(second,&list);
        check(list.size==1 && list.pHead->uiStart==0x01020304 && list.pHead->uiEnd==0xf1020304,"replacement frees prior list and decodes full32bit fields");
        AudioGapListInit(valid,&list);
        check(list.size==2,"replacement restores two-node list");
    }
    const char* invalid[]={"absent.wav","empty.wav","truncated.wav","backwards.wav","later-invalid.wav","extensionless","folder.dot/absent.wav"};
    for(const char* name:invalid) {
        AudioGapListInit(valid,&list);
        AudioGapListInit(const_cast<char*>(name),&list);
        empty(list,"missing/invalid/empty sidecar leaves empty list without prior-node leak");
    }
    char large[]="large.wav";
    AudioGapListInit(large,&list); check(list.size==65537,"record count retains original32bit range beyond65536 nodes");
    char overlap[]="overlap.wav";
    AudioGapListInit(overlap,&list); check(list.size==2 && list.pHead->pNext->uiStart==5,"overlapping records retain original order");
    char unordered[]="unordered.wav";
    AudioGapListInit(unordered,&list); check(list.size==2 && list.pHead->pNext->uiStart==1,"out-of-order records retain original order and polling policy");
    char dotted[]="folder.dot/speech.part.wav";
    AudioGapListInit(dotted,&list); check(list.size==1 && list.pHead->uiEnd==77,"basename final extension is replaced through dotted paths");
    char maximum[256]; std::memset(maximum,'a',sizeof(maximum)); maximum[251]='.'; maximum[252]='w'; maximum[253]='a'; maximum[254]='v'; maximum[255]=0;
    AudioGapListInit(maximum,&list); check(list.size==1,"255-byte source path admits exact256-byte sidecar output");
    char tooLate[256]; std::memset(tooLate,'a',sizeof(tooLate)); tooLate[253]='.'; tooLate[254]='x'; tooLate[255]=0;
    AudioGapListInit(tooLate,&list); empty(list,"extension growth beyond filename capacity fails safely");
    char noTerminator[256]; std::memset(noTerminator,'x',sizeof(noTerminator));
    AudioGapListInit(noTerminator,&list); empty(list,"unterminated bounded filename fails safely");
    AudioGapListInit(nullptr,&list); empty(list,"null filename has empty sidecar");
    AudioGapListInit(valid,nullptr); AudioGapListDone(nullptr); PollAudioGap(NO_SAMPLE,nullptr);
    AudioGapListInit(valid,&list); list.current_time=99; list.audio_gap_active=TRUE;
    AudioGapListDone(&list); AudioGapListDone(&list); empty(list,"cleanup is complete and repeatable");
}
void pollEndpoints(UINT32 sound,AudioGapList& list) {
    SDL_Delay(10);
    PollAudioGap(sound,&list); check(list.audio_gap_active,"actual streamed sound wall-clock enters authored GAP");
    auto* first=list.pHead; auto* second=first->pNext;
    // Exact tick boundaries are checked only when independently bracketed
    // positions agree. Retrying avoids assuming SDL's millisecond scheduling.
    for(bool endpointIsEnd:{false,true}) {
        bool observed=false;
        for(unsigned attempt=0;attempt<100 && !observed;++attempt) {
            const auto t=SoundGetPosition(sound);
            first->uiStart=endpointIsEnd?0:t;
            first->uiEnd=endpointIsEnd?t:0xffffffffu;
            PollAudioGap(sound,&list);
            if(SoundGetPosition(sound)==t) { check(!list.audio_gap_active,"strict GAP endpoint excluded at exact observed tick"); observed=true; }
        }
        check(observed,"exact endpoint tick bracket obtained");
    }
    first->uiStart=first->uiEnd=0; second->uiStart=0; second->uiEnd=0xffffffffu;
    PollAudioGap(sound,&list); check(list.audio_gap_active,"poll catches up past first gap into next");
    second->uiEnd=0;
    PollAudioGap(sound,&list); check(!list.audio_gap_active,"poll clears activity past final gap");
    first->uiStart=0; first->uiEnd=10000;
    PollAudioGap(NO_SAMPLE,&list); check(!list.audio_gap_active,"unknown sound position zero excludes strict start");
}
void streamed() {
    NativeSoundSetManualRendering(TRUE);
    check(InitializeSoundManager() && SoundGetDriverHandle(),"real paused SDL dummy device initializes");
    SetSpeechVolume(79); check(GetSpeechVolume()==79,"actual original speech-volume setter/getter retained");
    AudioGapList list{}; char filename[]="speech.wav";
    const auto id=PlayJA2GapSample(filename,RATE_11025,63,1,0,&list);
    check(id!=SOUND_ERROR && SoundIsPlaying(id) && list.size==2,"actual GAP wrapper starts streamed FileMan WAV with sidecar");
    check(NativeSoundStreamBufferedFrames(id)>0 && NativeSoundStreamSourceBytesRead(id)>0,"actual FFmpeg stream has prefetched source data");
    check(SoundGetVolume(id)==39,"original speech volume policy truncates63/127*79 to39");
    std::array<float,2> pcm{};
    check(NativeSoundRender(pcm.data(),1),"production stream PCM renders");
    check(std::abs(pcm[0]-0.25f*78/255)<0.00001f && pcm[1]==0,"actual streamed output preserves requested gain and left pan");
    const auto before=SoundGetPosition(id); pollEndpoints(id,list);
    check(SoundGetPosition(id)>before,"stream progress advances while dummy device is paused");
    std::vector<float> remaining(4799*2); check(NativeSoundRender(remaining.data(),4799),"remaining one-shot stream renders");
    check(!SoundIsPlaying(id),"actual stream finishes after4800frames");
    SoundServiceStreams(); check(NativeSoundActiveVoices()==0 && NativeSoundRetainedBytes()==0,"service retires finished stream and frees retained buffers");
    AudioGapListDone(&list);
    char noGap[]="no-sidecar.wav";
    const auto noGapId=PlayJA2GapSample(noGap,RATE_11025,HIGHVOLUME,1,MIDDLEPAN,&list);
    check(noGapId!=SOUND_ERROR,"missing optional sidecar allows actual speech playback"); empty(list,"missing optional sidecar has no gap state");
    SoundStop(noGapId);
    char missing[]="missing.wav";
    check(PlayJA2GapSample(missing,RATE_11025,HIGHVOLUME,1,MIDDLEPAN,&list)==SOUND_ERROR,"missing stream fails through real sound backend"); empty(list,"failed playback owns no GAP nodes");
    check(PlayJA2GapSample(nullptr,RATE_11025,HIGHVOLUME,1,MIDDLEPAN,&list)==SOUND_ERROR,"null stream name rejected");
    char badName[256]; std::memset(badName,'x',sizeof(badName));
    check(PlayJA2GapSample(badName,RATE_11025,HIGHVOLUME,1,MIDDLEPAN,&list)==SOUND_ERROR,"unterminated bounded stream name rejected");
    SetSpeechVolume(1000); check(GetSpeechVolume()==127,"actual original speech setter clamps127");
    const auto withoutList=PlayJA2GapSample(filename,RATE_11025,HIGHVOLUME,1,MIDDLEPAN,nullptr);
    check(withoutList!=SOUND_ERROR,"speech playback permits null sidecar list"); SoundStop(withoutList);
    ShutdownSoundManager();
}
void multiple() {
    NativeSoundSetManualRendering(TRUE); check(InitializeSoundManager(),"queue test initializes real paused dummy device");
    SetSpeechVolume(79);
    char files[11][64]{};
    for(unsigned i=0;i<11;++i) std::strcpy(files[i],i%2?"second.wav":"speech.wav");
    AudioGapList list{};
    auto id=PlayJA2MultipleGapSample(files,10,RATE_11025,63,1,255,&list);
    check(id!=SOUND_ERROR && subsequentsounds.ubSndCounter==1 && subsequentsounds.ubMaxSndCounter==10,"ten-slot queue starts first actual stream");
    check(std::strcmp(subsequentsounds.zSoundFiles[9],files[9])==0,"all ten queue names copied within capacity");
    check(subsequentsounds.spParams.uiVolume==39 && subsequentsounds.spParams.uiPan==255 && subsequentsounds.spParams.uiLoop==1 && subsequentsounds.spParams.uiPriority==GROUP_PLAYER,"queue retains original gain/pan/loop/priority policy");
    std::strcpy(files[1],"missing.wav");
    check(std::strcmp(subsequentsounds.zSoundFiles[1],"second.wav")==0,"continuation queue owns filename bytes independently");
    SoundStop(id);
    // The exact existing Faces continuation call; Faces itself is not linked.
    id=PlayJA2GapSample(subsequentsounds.zSoundFiles[subsequentsounds.ubSndCounter],RATE_11025,HIGHVOLUME,1,MIDDLEPAN,&list);
    ++subsequentsounds.ubSndCounter;
    check(id!=SOUND_ERROR && list.size==1 && list.pHead->uiStart==0x01020304,"actual second stream replaces prior utterance GAP ownership");
    check(SoundGetVolume(id)==79,"existing Faces continuation uses full speech volume");
    SoundStop(id);
    const UINT8 invalidCounts[]={0,11,255};
    for(const auto count:invalidCounts) {
        check(PlayJA2MultipleGapSample(files,count,RATE_11025,HIGHVOLUME,1,MIDDLEPAN,&list)==SOUND_ERROR,"invalid continuation count rejected before indexing");
        check(!subsequentsounds.ubSndCounter && !subsequentsounds.ubMaxSndCounter,"invalid call clears stale queue"); empty(list,"invalid queue call frees previous GAP state");
    }
    std::memset(files[0],'b',59); std::strcpy(files[0]+59,".wav");
    id=PlayJA2MultipleGapSample(files,1,RATE_11025,HIGHVOLUME,1,MIDDLEPAN,&list);
    check(id!=SOUND_ERROR && std::strlen(subsequentsounds.zSoundFiles[0])==63,"maximum63-byte terminated queue name starts actual stream");
    SoundStop(id); AudioGapListDone(&list);
    std::memset(files[0],'x',64);
    check(PlayJA2MultipleGapSample(files,1,RATE_11025,HIGHVOLUME,1,MIDDLEPAN,&list)==SOUND_ERROR,"unterminated64-byte queue name rejected");
    files[0][0]=0;
    check(PlayJA2MultipleGapSample(files,1,RATE_11025,HIGHVOLUME,1,MIDDLEPAN,&list)==SOUND_ERROR,"empty queue name rejected");
    check(PlayJA2MultipleGapSample(nullptr,1,RATE_11025,HIGHVOLUME,1,MIDDLEPAN,&list)==SOUND_ERROR,"null queue rejected");
    std::strcpy(files[0],"missing.wav");
    check(PlayJA2MultipleGapSample(files,1,RATE_11025,HIGHVOLUME,1,MIDDLEPAN,&list)==SOUND_ERROR && !subsequentsounds.ubMaxSndCounter,"first-stream failure clears pending continuation state");
    empty(list,"failed multiple playback leaves empty GAP state");
    AudioGapListDone(&list); SoundStopAll();
    check(NativeSoundActiveVoices()==0 && NativeSoundRetainedBytes()==0,"queue playback cleanup releases all native stream resources");
    ShutdownSoundManager();
}
}
int main() {
    try {
        check(SDL_SetEnvironmentVariable(SDL_GetEnvironment(),"SDL_AUDIO_DRIVER","dummy",true),"SDL dummy driver selected before initialization");
        Temporary temp;
        const auto wav=wave();
        for(const char* name:{"speech.wav","second.wav","no-sidecar.wav"}) write(temp.root/name,wav);
        write(temp.root/"speech.gap",gaps({{5,10000},{20000,30000}}));
        write(temp.root/"second.gap",gaps({{0x01020304,0xf1020304}}));
        write(temp.root/"empty.gap",{});
        auto truncated=gaps({{1,2},{3,4}}); truncated.pop_back(); write(temp.root/"truncated.gap",truncated);
        write(temp.root/"backwards.gap",gaps({{10,5}}));
        write(temp.root/"later-invalid.gap",gaps({{1,2},{10,5}}));
        write(temp.root/"missing.gap",gaps({{1,2}}));
        write(temp.root/(std::string(59,'b')+".wav"),wav);
        write(temp.root/"overlap.gap",gaps({{0,10},{5,20}}));
        write(temp.root/"unordered.gap",gaps({{50,100},{1,2}}));
        write(temp.root/"large.gap",std::vector<unsigned char>(65537*8));
        write(temp.root/"folder.dot/speech.part.gap",gaps({{1,77}}));
        write(temp.root/(std::string(251,'a')+".gap"),gaps({{1,77}}));
        mount(temp.root); check(InitializeFileManager(nullptr),"actual FileMan initializes");
        parser(); streamed(); multiple(); ShutdownFileManager();
        std::cout<<"native_dialogue_audio_tests: "<<checks<<" checks passed; real GAP/FileMan/streamed native audio; SDL dummy only\n";
        return 0;
    } catch(const std::exception& e) {
        ShutdownSoundManager(); ShutdownFileManager();
        std::cerr<<"dialogue audio failure: "<<e.what()<<"; "<<NativeSoundLastError()<<'\n';
        return 1;
    }
}
