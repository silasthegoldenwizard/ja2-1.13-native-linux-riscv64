/*
 * P710 native snapshot update: sgp/NativeSoundMan.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native 1.13 SoundMan adapter. Original FMOD implementation is preserved.
// Reuse approved FFmpeg decoding and SDL device mixing transport;
// retain game-facing cache/voice/random/EOS ownership here. No codec subprocess.
#include "native_soundman.h"
#include "native_audio_decoder.h"
#include "FileManIO.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <vector>

namespace {
constexpr UINT32 Default = 0xffffffffu;
constexpr unsigned Rate = 48000, Slots = 128;
constexpr std::size_t HardMemory = 64u * 1024u * 1024u;
constexpr UINT32 StreamFrames=16384, RefillFrames=2048;
struct Streaming {
    std::unique_ptr<NativeAudioDecoder> decoder;
    std::array<float,StreamFrames*2> ring{};
    UINT32 head=0, count=0, remainingLoops=1;
    bool eof=false, failed=false, producedThisPass=false;
    UINT32 underruns=0;
    // Services are game/main-thread operations. This also serializes accidental
    // simultaneous service calls without holding the realtime mixer lock.
    std::mutex serviceGuard;
    std::size_t bytes() const { return sizeof(ring)+(decoder?decoder->BufferedBytes():0); }
};
struct PCM {
    std::vector<float> stereo;
    unsigned sourceRate = Rate;
    // Admission counts retained allocation, including unused vector capacity.
    std::size_t bytes() const { return stereo.capacity() * sizeof(float); }
    std::size_t frames() const { return stereo.size()/2; }
};
struct Cached {
    std::string name;
    std::shared_ptr<PCM> pcm;
    UINT32 flags=0, hits=0;
    RANDOMPARMS random{};
    Uint64 next=0;
};
struct Voice {
    std::shared_ptr<PCM> pcm;
    std::shared_ptr<Streaming> stream;
    UINT32 id=NO_SAMPLE, sample=NO_SAMPLE, volume255=254, pan=128, loops=1, priority=PRIORITY_MAX;
    double cursor=0, step=1;
    UINT32 startedAt=0; // original GetTickCount32 modulo elapsed wall time
    bool done=false, random=false;
    void (*callback)(void*)=nullptr;
    void* data=nullptr;
};
struct EOS {
    void (*callback)(void*)=nullptr; void* data=nullptr;
    // Destruction (including VFS close/FFmpeg free) happens on the service/stop
    // thread after releasing the realtime mixer mutex.
    std::shared_ptr<Streaming> releaseStream;
    std::shared_ptr<PCM> releasePCM;
};
std::mutex guard;
std::array<Cached, Slots> cache;
std::array<Voice, Slots> voices;
struct StreamTerminal { UINT32 id=NO_SAMPLE; NativeSoundStreamOutcome outcome=NativeSoundStreamOutcome::Unknown; };
std::array<StreamTerminal,256> streamTerminals{};
unsigned nextStreamTerminal=0;
SDL_AudioStream* output=nullptr;
bool enabled=true, initialized=false, ownAudio=false, manual=false;
UINT32 memoryLimit=8048u*1024u, threshold=256u*8024u, defaultVolume=127, nextID=0;
std::string lastError;
bool limitSimultaneous=false;
struct Throttle { std::string name; Uint64 until=0; };
std::array<Throttle,Slots> throttles;
unsigned nextThrottle=0;
std::mt19937 randomEngine{0x4a413231u}; // deterministic isolated-test fallback
UINT32 (*randomSource)(UINT32)=nullptr;

void fail(const char* message) { lastError=message; SDL_LogError(SDL_LOG_CATEGORY_AUDIO,"SoundMan: %s",message); }
std::string key(const char* name) {
    std::string result=name ? name : "";
    for (char& c:result) { if(c=='\\') c='/'; else if(c>='A' && c<='Z') c+= 'a'-'A'; }
    return result;
}
std::size_t cachedBytes() { std::size_t n=0; for(auto& c:cache) if(c.pcm) n+=c.pcm->bytes(); return n; }
std::size_t totalBytes() {
    std::size_t n=cachedBytes();
    for(auto& v:voices) {
        if(v.pcm && v.sample==NO_SAMPLE) n+=v.pcm->bytes();
        if(v.stream) n+=v.stream->bytes();
    }
    return n;
}
UINT32 cached(const char* filename) {
    if(!filename || !*filename) return NO_SAMPLE;
    const auto name=key(filename);
    for(UINT32 i=0;i<Slots;++i) if(cache[i].pcm && cache[i].name==name) return i;
    return NO_SAMPLE;
}
bool allocated(const Voice& v) { return bool(v.pcm) || bool(v.stream); }
bool inUse(UINT32 sample) { for(auto& v:voices) if(v.pcm && v.sample==sample) return true; return false; }
bool evict() {
    UINT32 victim=NO_SAMPLE;
    for(UINT32 i=0;i<Slots;++i) if(cache[i].pcm && !(cache[i].flags&SAMPLE_LOCKED) && !inUse(i))
        if(victim==NO_SAMPLE || cache[i].hits<cache[victim].hits) victim=i;
    if(victim==NO_SAMPLE) return false;
    cache[victim]=Cached{}; return true;
}
UINT32 vacantCache() { for(UINT32 i=0;i<Slots;++i) if(!cache[i].pcm) return i; return NO_SAMPLE; }
Voice* find(UINT32 id) { if(id==NO_SAMPLE) return nullptr; for(auto& v:voices) if(allocated(v) && v.id==id) return &v; return nullptr; }
EOS retire(Voice& v) {
    if(v.stream) {
        const auto outcome=v.stream->failed?NativeSoundStreamOutcome::Failed:
            (v.done && v.stream->eof && !v.stream->count?NativeSoundStreamOutcome::Finished:NativeSoundStreamOutcome::Stopped);
        streamTerminals[nextStreamTerminal++%streamTerminals.size()]={v.id,outcome};
    }
    EOS callback{v.callback,v.data,std::move(v.stream),std::move(v.pcm)};
    v=Voice{}; // retire before callback so reentrant Stop cannot dispatch twice
    return callback;
}
void invoke(EOS& e) {
    const auto callback=e.callback; void* const data=e.data; e.callback=nullptr;
    // Like original SoundStopIndex, release the stream before notifying game
    // code, while allowing reentrant callbacks without retaining old VFS IDs.
    e.releaseStream.reset(); e.releasePCM.reset();
    if(callback) callback(data);
}
UINT32 choose(UINT32 lo, UINT32 hi) {
    if(hi<=lo) return lo;
    if(randomSource) return lo+(randomSource(hi-lo)%(hi-lo));
    return std::uniform_int_distribution<UINT32>(lo,hi-1)(randomEngine);
}
// Cached/buffer playback may own a complete decoded sample, admitted only up
// to a finite budget. Filename streaming below never calls this whole decoder.
std::shared_ptr<PCM> decodeAll(std::unique_ptr<NativeAudioDecoder> decoder,std::size_t budget) {
    if(!decoder) return {};
    auto pcm=std::make_shared<PCM>();
    std::array<float,RefillFrames*2> chunk{};
    for(;;) {
        const auto result=decoder->ReadFrames(chunk.data(),RefillFrames);
        if(result.error) { fail(decoder->Error().c_str()); return {}; }
        if(result.frames) {
            const auto required=pcm->stereo.size()+result.frames*2;
            const auto limit=budget/sizeof(float);
            if(required>limit) { fail("Decoded sample exceeds its bounded cache/buffer budget"); return {}; }
            if(required>pcm->stereo.capacity()) {
                // Bound geometric growth before reserving. Aggregate admission
                // below counts this capacity, not only the decoded payload.
                const auto grown=std::min(limit,pcm->stereo.capacity()*2);
                pcm->stereo.reserve(std::max(required,grown));
                if(pcm->bytes()>budget) { fail("PCM allocation exceeds its bounded cache/buffer budget"); return {}; }
            }
            pcm->stereo.insert(pcm->stereo.end(),chunk.begin(),chunk.begin()+result.frames*2);
        }
        if(result.eof) break;
        if(!result.frames) { fail("Audio decoder returned no PCM without EOF"); return {}; }
    }
    if(pcm->stereo.empty()) { fail("Audio source contains no decoded frames"); return {}; }
    return pcm;
}
std::shared_ptr<PCM> decode(const void* bytes,UINT32 size,std::size_t budget) {
    std::string error;
    auto decoder=NativeAudioDecoder::OpenMemory(bytes,size,error);
    if(!decoder) { fail(error.c_str()); return {}; }
    return decodeAll(std::move(decoder),budget);
}
std::shared_ptr<PCM> load(const char* filename,std::size_t budget) {
    std::string error;
    auto decoder=NativeAudioDecoder::OpenFile(filename,error);
    if(!decoder) { fail(error.c_str()); return {}; }
    return decodeAll(std::move(decoder),budget);
}
UINT32 loadCached(const char* filename) {
    std::size_t budget=0;
    {
        std::lock_guard<std::mutex> lock(guard);
        const auto slot=cached(filename); if(slot!=NO_SAMPLE) return slot;
        budget=memoryLimit;
    }
    // Decode and VFS operations run outside the realtime mixer mutex.
    auto pcm=load(filename,budget); if(!pcm) return NO_SAMPLE;
    std::lock_guard<std::mutex> lock(guard);
    if(!initialized) return NO_SAMPLE;
    const auto existing=cached(filename); if(existing!=NO_SAMPLE) return existing;
    if(pcm->bytes()>memoryLimit) { fail("Decoded sample exceeds the configured cache memory limit"); return NO_SAMPLE; }
    while(cachedBytes()+pcm->bytes()>memoryLimit || totalBytes()+pcm->bytes()>HardMemory || vacantCache()==NO_SAMPLE)
        if(!evict()) { fail("No evictable audio cache space"); return NO_SAMPLE; }
    const auto slot=vacantCache(); auto& c=cache[slot]; c.name=key(filename); c.pcm=std::move(pcm); c.flags=SAMPLE_ALLOCATED;
    return slot;
}
bool refill(const std::shared_ptr<Streaming>& stream) {
    std::lock_guard<std::mutex> serviceLock(stream->serviceGuard);
    std::array<float,RefillFrames*2> chunk{};
    UINT32 remainingBudget=0;
    { std::lock_guard<std::mutex> lock(guard); remainingBudget=StreamFrames-stream->count; }
    for(unsigned work=0;work<StreamFrames*2+128;++work) {
        if(!remainingBudget) return true;
        UINT32 available=0;
        { std::lock_guard<std::mutex> lock(guard);
          if(stream->failed) return false;
          if(stream->eof || stream->count==StreamFrames) return true;
          available=std::min({RefillFrames,StreamFrames-stream->count,remainingBudget});
        }
        // FFmpeg, allocation, source reads and loop seeks are exclusively here
        // on the calling service thread. The SDL callback never enters here.
        const auto result=stream->decoder->ReadFrames(chunk.data(),available);
        if(result.error) {
            fail(stream->decoder->Error().c_str());
            std::lock_guard<std::mutex> lock(guard); stream->failed=true; return false;
        }
        if(result.frames) {
            std::lock_guard<std::mutex> lock(guard);
            for(UINT32 i=0;i<result.frames;++i) {
                const auto at=(stream->head+stream->count+i)%StreamFrames;
                stream->ring[at*2]=chunk[i*2]; stream->ring[at*2+1]=chunk[i*2+1];
            }
            stream->count+=result.frames; stream->producedThisPass=true; remainingBudget-=result.frames;
        }
        if(result.eof) {
            if(!stream->producedThisPass) {
                fail("Streamed source contains no decoded frames");
                std::lock_guard<std::mutex> lock(guard); stream->failed=true; return false;
            }
            if(stream->remainingLoops==1) {
                std::lock_guard<std::mutex> lock(guard); stream->eof=true; return true;
            }
            if(stream->remainingLoops>1) --stream->remainingLoops;
            if(!stream->decoder->Rewind()) {
                fail(stream->decoder->Error().c_str());
                std::lock_guard<std::mutex> lock(guard); stream->failed=true; return false;
            }
            stream->producedThisPass=false;
        } else if(!result.frames) {
            fail("Streamed audio made no progress before EOF");
            std::lock_guard<std::mutex> lock(guard); stream->failed=true; return false;
        }
    }
    fail("Streamed audio exceeded bounded refill work");
    std::lock_guard<std::mutex> lock(guard); stream->failed=true; return false;
}
void mix(float* dest, UINT32 frames) {
    std::fill(dest,dest+static_cast<std::size_t>(frames)*2,0.0f);
    for(auto& v:voices) {
        if(!allocated(v) || v.done) continue;
        // Linear balance: center preserves stereo, endpoints mute opposite ear.
        const float gain=static_cast<float>(v.volume255)/255.0f;
        const float left=gain*(v.pan>128 ? (255-v.pan)/127.0f:1.0f);
        const float right=gain*(v.pan<128 ? v.pan/128.0f:1.0f);
        if(v.stream) {
            auto& stream=*v.stream;
            for(UINT32 frame=0;frame<frames;++frame) {
                if(!stream.count) {
                    if(stream.eof || stream.failed) v.done=true;
                    else ++stream.underruns;
                    break;
                }
                dest[frame*2]+=stream.ring[stream.head*2]*left;
                dest[frame*2+1]+=stream.ring[stream.head*2+1]*right;
                stream.head=(stream.head+1)%StreamFrames; --stream.count;
            }
            if(!stream.count && (stream.eof || stream.failed)) v.done=true;
            continue;
        }
        const auto count=v.pcm->frames();
        for(UINT32 frame=0;frame<frames;++frame) {
            while(v.cursor>=count) {
                if(v.loops==1) { v.done=true; break; }
                if(v.loops>1) --v.loops;
                v.cursor-=static_cast<double>(count);
            }
            if(v.done) break;
            const auto a=static_cast<std::size_t>(v.cursor);
            const auto b=a+1<count ? a+1 : ((v.loops==0 || v.loops>1)?0:a);
            const float f=static_cast<float>(v.cursor-a);
            const auto& p=v.pcm->stereo;
            dest[frame*2]+=(p[a*2]+(p[b*2]-p[a*2])*f)*left;
            dest[frame*2+1]+=(p[a*2+1]+(p[b*2+1]-p[a*2+1])*f)*right;
            v.cursor+=v.step;
        }
        // Exact frame-boundary completion, without needing another callback.
        if(v.cursor>=count && v.loops==1) v.done=true;
    }
    for(std::size_t i=0;i<static_cast<std::size_t>(frames)*2;++i) dest[i]=std::clamp(dest[i],-1.0f,1.0f);
}
void SDLCALL feed(void*, SDL_AudioStream* stream, int additional, int) {
    std::array<float,1024*2> buffer{};
    while(additional>0) {
        const auto frames=static_cast<UINT32>(std::min(additional/8+((additional%8)!=0),1024));
        { std::lock_guard<std::mutex> lock(guard); mix(buffer.data(),frames); }
        if(!SDL_PutAudioStreamData(stream,buffer.data(),static_cast<int>(frames*8))) return;
        additional-=static_cast<int>(frames*8);
    }
}
UINT32 start(std::shared_ptr<PCM> pcm, UINT32 sample, const SOUNDPARMS* p, bool random=false, std::shared_ptr<Streaming> stream={}) {
    if(!initialized || (!pcm && !stream)) return SOUND_ERROR;
    Voice* voice=nullptr;
    // Like the original128 FMOD channels, priority is retained but does not
    // preempt another live sound. Exhaustion is an explicit SOUND_ERROR.
    for(auto& v:voices) if(!allocated(v)) { voice=&v; break; }
    if(!voice) { fail("All128 audio voices are occupied; service completed voices before retrying"); return SOUND_ERROR; }
    Voice v; v.pcm=std::move(pcm); v.stream=std::move(stream); v.sample=sample; v.random=random;
    v.volume255=p && p->uiVolume!=Default ? std::min(p->uiVolume,127u)*2:defaultVolume;
    v.pan=std::min(p && p->uiPan!=Default ? p->uiPan:128u,255u);
    v.loops=p && p->uiLoop!=Default ? p->uiLoop:1;
    v.priority=p && p->uiPriority!=Default ? p->uiPriority:PRIORITY_MAX;
    // Preserve pinned soundman.cpp1602 and1737: both original
    // sample/stream paths explicitly say "Speed and pitchbend don't use".
    // Requests remain ignored; decoder conversion retains source playback timing.
    if(p && p->EOSCallback && reinterpret_cast<std::uintptr_t>(p->EOSCallback)!=std::numeric_limits<std::uintptr_t>::max() &&
       reinterpret_cast<std::uintptr_t>(p->EOSCallback)!=Default) { v.callback=p->EOSCallback; v.data=p->pCallbackData; }
    do { v.id=nextID++; } while(v.id==NO_SAMPLE || find(v.id));
    for(auto& terminal:streamTerminals) if(terminal.id==v.id) terminal=StreamTerminal{};
    v.startedAt=static_cast<UINT32>(SDL_GetTicks());
    *voice=std::move(v); if(sample!=NO_SAMPLE) {
        ++cache[sample].hits;
        if(voice->loops==0) cache[sample].flags|=SAMPLE_LOCKED;
    }
    return voice->id;
}
}

const char* NativeSoundLastError() { return lastError.c_str(); }
BOOLEAN NativeSoundPlaybackEnabled() { std::lock_guard<std::mutex> lock(guard); return enabled && initialized && output; }
NativeSoundStreamOutcome NativeSoundGetStreamOutcome(UINT32 id) {
    std::lock_guard<std::mutex> lock(guard);
    if(auto* v=find(id)) {
        if(!v->stream) return NativeSoundStreamOutcome::Unknown;
        if(v->stream->failed) return NativeSoundStreamOutcome::Failed;
        if(v->done && v->stream->eof && !v->stream->count) return NativeSoundStreamOutcome::Finished;
        return NativeSoundStreamOutcome::Playing;
    }
    if(id!=NO_SAMPLE) for(const auto& terminal:streamTerminals) if(terminal.id==id) return terminal.outcome;
    return NativeSoundStreamOutcome::Unknown;
}
void NativeSoundSetManualRendering(BOOLEAN value) { std::lock_guard<std::mutex> lock(guard); if(!initialized) manual=value!=FALSE; }
BOOLEAN NativeSoundRender(float* stereo, UINT32 frames) {
    std::lock_guard<std::mutex> lock(guard);
    if(!initialized || !manual || !stereo || frames>Rate*60) return FALSE;
    mix(stereo,frames); return TRUE;
}
UINT32 NativeSoundActiveVoices() { std::lock_guard<std::mutex> lock(guard); UINT32 count=0; for(auto& v:voices) if(allocated(v) && !v.done) ++count; return count; }
UINT32 NativeSoundCachedBytes() { std::lock_guard<std::mutex> lock(guard); return static_cast<UINT32>(cachedBytes()); }
std::size_t NativeSoundRetainedBytes() { std::lock_guard<std::mutex> lock(guard); return totalBytes(); }
UINT32 NativeSoundStreamBufferedFrames(UINT32 id) { std::lock_guard<std::mutex> lock(guard); auto* v=find(id); return v && v->stream?v->stream->count:0; }
std::uint64_t NativeSoundStreamSourceBytesRead(UINT32 id) { std::lock_guard<std::mutex> lock(guard); auto* v=find(id); return v && v->stream?v->stream->decoder->SourceBytesRead():0; }
std::size_t NativeSoundStreamBufferedBytes(UINT32 id) { std::lock_guard<std::mutex> lock(guard); auto* v=find(id); return v && v->stream?v->stream->bytes():0; }
UINT32 NativeSoundStreamUnderruns(UINT32 id) { std::lock_guard<std::mutex> lock(guard); auto* v=find(id); return v && v->stream?v->stream->underruns:0; }
void SoundEnableSound(BOOLEAN value) { std::lock_guard<std::mutex> lock(guard); enabled=value!=FALSE; }
BOOLEAN InitializeSoundManager() {
    ShutdownSoundManager();
    std::lock_guard<std::mutex> lock(guard);
    lastError.clear(); memoryLimit=8048u*1024u; threshold=256u*8024u;
    if(!enabled) return TRUE; // original startup enable switch is intentionally nonfatal
    ownAudio=(SDL_WasInit(SDL_INIT_AUDIO)&SDL_INIT_AUDIO)==0;
    if(ownAudio && !SDL_InitSubSystem(SDL_INIT_AUDIO)) { fail(SDL_GetError()); ownAudio=false; return FALSE; }
    SDL_AudioSpec spec{SDL_AUDIO_F32,2,static_cast<int>(Rate)};
    output=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,feed,nullptr);
    if(!output) { fail(SDL_GetError()); if(ownAudio) SDL_QuitSubSystem(SDL_INIT_AUDIO); ownAudio=false; return FALSE; }
    initialized=true;
    if(!manual && !SDL_ResumeAudioStreamDevice(output)) { fail(SDL_GetError()); initialized=false; SDL_DestroyAudioStream(output); output=nullptr; if(ownAudio) SDL_QuitSubSystem(SDL_INIT_AUDIO); ownAudio=false; return FALSE; }
    return TRUE;
}
void ShutdownSoundManager() {
    SDL_AudioStream* old=nullptr; bool quit=false;
    { std::lock_guard<std::mutex> lock(guard); old=output; output=nullptr; initialized=false; quit=ownAudio; ownAudio=false; }
    // Never hold the mixer mutex while SDL waits for its audio callback.
    if(old) SDL_DestroyAudioStream(old);
    SoundStopAll();
    { std::lock_guard<std::mutex> lock(guard); for(auto& c:cache) c=Cached{}; }
    if(quit) SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
void* SoundGetDriverHandle() { std::lock_guard<std::mutex> lock(guard); return output; }
BOOLEAN SoundSetMemoryLimit(UINT32 limit) { std::lock_guard<std::mutex> lock(guard); if(limit<cachedBytes() || limit>HardMemory) return FALSE; memoryLimit=limit; return TRUE; }
BOOLEAN SoundSetCacheThreshhold(UINT32 value) { std::lock_guard<std::mutex> lock(guard); threshold=value?value:256u*8024u; return TRUE; }
void SoundSetDefaultVolume(UINT32 value) { std::lock_guard<std::mutex> lock(guard); defaultVolume=std::min(value,127u); }
UINT32 SoundGetDefaultVolume() { std::lock_guard<std::mutex> lock(guard); return defaultVolume; }
UINT32 SoundLoadSample(STR filename) { { std::lock_guard<std::mutex> lock(guard); if(!initialized) return NO_SAMPLE; } return loadCached(filename); }
UINT32 SoundFreeSample(STR filename) { std::lock_guard<std::mutex> lock(guard); const auto i=cached(filename); if(i==NO_SAMPLE || inUse(i)) return NO_SAMPLE; cache[i]=Cached{}; return i; }
UINT32 SoundLockSample(STR filename) { std::lock_guard<std::mutex> lock(guard); const auto i=cached(filename); if(i!=NO_SAMPLE) cache[i].flags|=SAMPLE_LOCKED; return i; }
UINT32 SoundUnlockSample(STR filename) { std::lock_guard<std::mutex> lock(guard); const auto i=cached(filename); if(i!=NO_SAMPLE) cache[i].flags&=~SAMPLE_LOCKED; return i; }
BOOLEAN SoundEmptyCache() {
    SoundStopAll();
    std::lock_guard<std::mutex> lock(guard);
    // Original1126-1138 empties even explicitly locked samples. Reentrant EOS
    // may create a replacement voice; keep its shared PCM ownership intact.
    for(auto& v:voices) if(v.pcm && v.sample!=NO_SAMPLE) v.sample=NO_SAMPLE;
    for(auto& c:cache) c=Cached{};
    return TRUE;
}
void SoundSetSampleFlags(UINT32 i, UINT32 flags) { std::lock_guard<std::mutex> lock(guard); if(i<Slots && cache[i].pcm) cache[i].flags|=flags; }
void SoundRemoveSampleFlags(UINT32 i, UINT32 flags) { std::lock_guard<std::mutex> lock(guard); if(i<Slots && cache[i].pcm) cache[i].flags&=~flags; }
UINT32 SoundPlay(STR filename, SOUNDPARMS* p) {
    SoundServiceStreams();
    { std::lock_guard<std::mutex> lock(guard); if(!initialized) return SOUND_ERROR; }
    // Preserve the exact original threshold boundary: >= is streamed only.
    const HWFILE file=filename?FileOpen(filename,FILE_ACCESS_READ,FALSE):0;
    if(!file) { fail("Audio file could not be opened through FileMan/VFS"); return SOUND_ERROR; }
    const UINT32 size=FileGetSize(file); FileClose(file);
    {
        std::lock_guard<std::mutex> lock(guard);
        if(size>=threshold) { fail("SoundPlay source reaches cache threshold; use SoundPlayStreamedFile"); return SOUND_ERROR; }
        if(limitSimultaneous) {
            const auto name=key(filename); const auto now=SDL_GetTicks();
            for(auto& entry:throttles) if(entry.name==name && entry.until>now) return 0;
            Throttle* slot=nullptr;
            for(auto& entry:throttles) if(entry.name==name || entry.until<=now) { slot=&entry; break; }
            if(!slot) slot=&throttles[nextThrottle++%Slots];
            slot->name=name; slot->until=now+50;
        }
    }
    const auto i=loadCached(filename); if(i==NO_SAMPLE) return SOUND_ERROR;
    std::lock_guard<std::mutex> lock(guard); return start(cache[i].pcm,i,p);
}
UINT32 SoundPlayStreamedFile(STR filename, SOUNDPARMS* p) {
    SoundServiceStreams();
    { std::lock_guard<std::mutex> lock(guard); if(!initialized) return SOUND_ERROR; }
    std::string error; auto decoder=NativeAudioDecoder::OpenFile(filename,error);
    if(!decoder) { fail(error.c_str()); return SOUND_ERROR; }
    auto stream=std::make_shared<Streaming>(); stream->decoder=std::move(decoder);
    stream->remainingLoops=p && p->uiLoop!=Default?p->uiLoop:1;
    if(!refill(stream)) return SOUND_ERROR;
    std::lock_guard<std::mutex> lock(guard);
    if(totalBytes()+stream->bytes()>HardMemory) { fail("Streamed audio exceeds aggregate bounded PCM/staging memory budget"); return SOUND_ERROR; }
    return start({},NO_SAMPLE,p,false,std::move(stream));
}
UINT32 SoundPlayFromBuffer(const char*, const void* bytes, UINT32 size, SOUNDPARMS* p) {
    SoundServiceStreams();
    { std::lock_guard<std::mutex> lock(guard); if(!initialized) return SOUND_ERROR; }
    auto pcm=decode(bytes,size,HardMemory); if(!pcm) return SOUND_ERROR;
    std::lock_guard<std::mutex> lock(guard);
    if(totalBytes()+pcm->bytes()>HardMemory) { fail("Buffer audio exceeds aggregate64MiB decoded memory bound"); return SOUND_ERROR; }
    return start(std::move(pcm),NO_SAMPLE,p);
}
void ResetSoundMap() { std::lock_guard<std::mutex> lock(guard); for(auto& entry:throttles) entry=Throttle{}; nextThrottle=0; }
void NativeSoundSetSimultaneousLimit(BOOLEAN value) { std::lock_guard<std::mutex> lock(guard); limitSimultaneous=value!=FALSE; }
void NativeSoundSetRandomSource(UINT32 (*source)(UINT32)) { std::lock_guard<std::mutex> lock(guard); randomSource=source; }
BOOLEAN SoundStop(UINT32 id) { EOS e; { std::lock_guard<std::mutex> lock(guard); auto* v=find(id); if(!v || v->done) return FALSE; e=retire(*v); } invoke(e); return TRUE; }
BOOLEAN SoundStopAll() {
    std::array<EOS,Slots> callbacks{};
    { std::lock_guard<std::mutex> lock(guard); for(unsigned i=0;i<Slots;++i) if(allocated(voices[i])) callbacks[i]=retire(voices[i]); }
    for(auto& e:callbacks) invoke(e); return TRUE;
}
BOOLEAN SoundIsPlaying(UINT32 id) { std::lock_guard<std::mutex> lock(guard); auto* v=find(id); return v && !v->done; }
BOOLEAN SoundSetVolume(UINT32 id, UINT32 volume) { std::lock_guard<std::mutex> lock(guard); auto* v=find(id); if(!v || v->done) return FALSE; v->volume255=std::min(volume,127u)*2; return TRUE; }
BOOLEAN SoundSetPan(UINT32 id, UINT32 pan) { std::lock_guard<std::mutex> lock(guard); auto* v=find(id); if(!v || v->done) return FALSE; v->pan=std::min(pan,255u); return TRUE; }
UINT32 SoundGetVolume(UINT32 id) { std::lock_guard<std::mutex> lock(guard); auto* v=find(id); return v && !v->done?v->volume255/2:SOUND_ERROR; }
UINT32 SoundGetPosition(UINT32 id) {
    std::lock_guard<std::mutex> lock(guard);
    auto* v=find(id);
    // Pinned soundman.cpp1037-1056 measures wall time, even when the voice
    // has finished rendering but has not yet been retired by stream service.
    // Unsigned32 subtraction preserves the original tick rollover behavior.
    return v ? static_cast<UINT32>(SDL_GetTicks())-v->startedAt : 0;
}
BOOLEAN SoundServiceStreams() {
    // Snapshot shared stream ownership under the mixer lock, then release it
    // before every FFmpeg/VFS operation. No decoder runs on SDL's callback.
    for(unsigned i=0;i<Slots;++i) {
        std::shared_ptr<Streaming> stream; UINT32 id=NO_SAMPLE;
        { std::lock_guard<std::mutex> lock(guard);
          if(voices[i].stream && !voices[i].done) { stream=voices[i].stream; id=voices[i].id; }
        }
        if(!stream) continue;
        const bool valid=refill(stream);
        std::lock_guard<std::mutex> lock(guard);
        auto* voice=find(id);
        if(voice && (!valid || (!stream->count && stream->eof))) voice->done=true;
        if(totalBytes()>HardMemory && voice) { fail("Stream staging exceeds aggregate native audio bound"); stream->failed=true; voice->done=true; }
    }
    std::array<EOS,Slots> callbacks{};
    { std::lock_guard<std::mutex> lock(guard); for(unsigned i=0;i<Slots;++i) if(allocated(voices[i]) && voices[i].done) callbacks[i]=retire(voices[i]); }
    for(auto& e:callbacks) invoke(e); return TRUE;
}
UINT32 SoundPlayRandom(STR filename, RANDOMPARMS* p) {
    UINT32 volume=0;
    { std::lock_guard<std::mutex> lock(guard); if(!initialized) return SOUND_ERROR; volume=defaultVolume; }
    if(!p || p->uiTimeMin==Default) return SOUND_ERROR;
    auto r=*p;
    if(r.uiTimeMax==Default) r.uiTimeMax=r.uiTimeMin;
    if(r.uiTimeMax<r.uiTimeMin) return SOUND_ERROR;
    if(r.uiVolMin==Default) r.uiVolMin=volume;
    if(r.uiVolMax==Default) r.uiVolMax=volume;
    if(r.uiPanMin==Default) r.uiPanMin=r.uiPanMax=128;
    else if(r.uiPanMax==Default) r.uiPanMax=r.uiPanMin;
    if(r.uiPriority==Default) r.uiPriority=PRIORITY_RANDOM;
    if(r.uiMaxInstances==Default) r.uiMaxInstances=1;
    if(r.uiVolMax<r.uiVolMin || r.uiPanMax<r.uiPanMin) return SOUND_ERROR;
    const auto i=loadCached(filename); if(i==NO_SAMPLE) return SOUND_ERROR;
    std::lock_guard<std::mutex> lock(guard);
    cache[i].random=r; cache[i].flags|=SAMPLE_RANDOM|SAMPLE_LOCKED;
    cache[i].next=SDL_GetTicks()+choose(r.uiTimeMin,r.uiTimeMax); return i;
}
BOOLEAN SoundServiceRandom() {
    SoundServiceStreams();
    std::lock_guard<std::mutex> lock(guard); if(!initialized) return FALSE;
    bool started=false; const auto now=SDL_GetTicks();
    for(UINT32 i=0;i<Slots;++i) {
        auto& c=cache[i]; if(!c.pcm || !(c.flags&SAMPLE_RANDOM) || (c.flags&SAMPLE_RANDOM_MANUAL) || c.next>now) continue;
        UINT32 count=0; for(auto& v:voices) if(v.pcm && !v.done && v.sample==i && v.random) ++count;
        const auto& r=c.random; if(count>=r.uiMaxInstances) continue;
        SOUNDPARMS p; std::memset(&p,0xff,sizeof(p));
        p.uiVolume=choose(r.uiVolMin,r.uiVolMax); p.uiPan=choose(r.uiPanMin,r.uiPanMax);
        p.uiLoop=1; p.uiPriority=r.uiPriority; // original random service ignores speed range
        if(start(c.pcm,i,&p,true)!=SOUND_ERROR) { started=true; c.next=now+choose(r.uiTimeMin,r.uiTimeMax); }
    }
    return started;
}
BOOLEAN SoundStopAllRandom() {
    std::array<EOS,Slots> callbacks{};
    { std::lock_guard<std::mutex> lock(guard);
      for(unsigned i=0;i<Slots;++i) if(voices[i].pcm && voices[i].sample!=NO_SAMPLE && (cache[voices[i].sample].flags&SAMPLE_RANDOM)) callbacks[i]=retire(voices[i]);
      for(auto& c:cache) if(c.flags&SAMPLE_RANDOM) c.flags&=~(SAMPLE_RANDOM|SAMPLE_LOCKED);
    }
    for(auto& e:callbacks) invoke(e); return TRUE;
}
void SoundLog(CHAR8* message) { if(message) SDL_Log("%s",message); }
