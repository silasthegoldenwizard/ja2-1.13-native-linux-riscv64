/*
 * P710 native snapshot update: sgp/NativeAudioDecoder.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Adapter authored for P710 against the approved FFmpeg7.1.5 public APIs.
// libavformat custom AVIO -> original FileMan/VFS, libavcodec send/receive,
// libswresample stereo float48k conversion. No CLI, protocol, or OS-file path.
#include "native_audio_decoder.h"
#include "native_codec_limits.h"
#include "FileManIO.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
#include <libswresample/swresample.h>
}
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <mutex>
#include <utility>
#include <vector>

namespace {
constexpr int IOBytes=32768;
constexpr int MaxInputSamples=32768;
constexpr int MaxOutputSamples=65536;
constexpr int MaxPacketBytes=2*1024*1024;
std::atomic<UINT32> liveInstances{0};
// FileMan can give several IDs for one VFS-owned physical file object. A
// decoder's seek+read pair must be indivisible relative to other decoders.
// Logical cursors below are independent of that shared physical position.
std::mutex sourceGuard;
int denyExtraIO(AVFormatContext*, AVIOContext**, const char*, int, AVDictionary**) { return AVERROR(EACCES); }
std::string describe(int code) {
    char text[AV_ERROR_MAX_STRING_SIZE]{};
    av_strerror(code,text,sizeof(text)); return text;
}
}

struct NativeAudioDecoder::Impl {
    HWFILE file=0;
    const UINT8* memory=nullptr;
    std::uint64_t size=0, position=0, bytesRead=0;
    bool callbackError=false;
    AVFormatContext* format=nullptr;
    AVIOContext* io=nullptr;
    AVCodecContext* codec=nullptr;
    AVPacket* packet=nullptr;
    AVFrame* frame=nullptr;
    SwrContext* resampler=nullptr;
    AVChannelLayout inputLayout{};
    AVSampleFormat inputFormat=AV_SAMPLE_FMT_NONE;
    int inputRate=0, audioIndex=-1;
    bool packetPending=false, inputEof=false, flushSent=false, codecEof=false, finished=false;
    std::vector<float> staging;
    std::size_t stagePosition=0, stageFrames=0;
    std::string error;

    ~Impl() {
        closeContexts();
        if(file) { std::lock_guard<std::mutex> lock(sourceGuard); try { FileClose(file); } catch(...) {} }
    }
    void closeContexts() {
        swr_free(&resampler); av_channel_layout_uninit(&inputLayout);
        av_frame_free(&frame); av_packet_free(&packet); avcodec_free_context(&codec);
        avformat_close_input(&format);
        if(io) av_freep(&io->buffer);
        avio_context_free(&io);
    }
    bool fail(const char* what, int code=0) {
        if(error.empty()) {
            error=what;
            if(code<0) error+=": "+describe(code);
            if(callbackError) error+=" (FileMan/VFS callback failed)";
        }
        return false;
    }
    static int read(void* opaque, UINT8* dest, int requested) {
        auto& self=*static_cast<Impl*>(opaque);
        if(requested<=0 || !dest) return AVERROR(EINVAL);
        if(self.position>=self.size) return AVERROR_EOF;
        const auto count=static_cast<UINT32>(std::min<std::uint64_t>(requested,self.size-self.position));
        if(self.memory) std::memcpy(dest,self.memory+self.position,count);
        else {
            std::lock_guard<std::mutex> lock(sourceGuard);
            try {
                // The legacy public FileSeek has signed32 distances. Two
                // positive seeks represent every public UINT32-sized source.
                const auto first=static_cast<UINT32>(std::min<std::uint64_t>(self.position,INT32_MAX));
                if(!FileSeek(self.file,first,FILE_SEEK_FROM_START)) { self.callbackError=true; return AVERROR(EIO); }
                auto remaining=self.position-first;
                while(remaining) {
                    const auto step=static_cast<UINT32>(std::min<std::uint64_t>(remaining,INT32_MAX));
                    if(!FileSeek(self.file,step,FILE_SEEK_FROM_CURRENT)) { self.callbackError=true; return AVERROR(EIO); }
                    remaining-=step;
                }
                UINT32 got=0;
                if(!FileRead(self.file,dest,count,&got) || got!=count) { self.callbackError=true; return AVERROR(EIO); }
            } catch(...) { self.callbackError=true; return AVERROR(EIO); }
        }
        self.position+=count; self.bytesRead+=count; return static_cast<int>(count);
    }
    static int64_t seek(void* opaque, int64_t offset, int whence) {
        auto& self=*static_cast<Impl*>(opaque);
        if(whence&AVSEEK_SIZE) return static_cast<int64_t>(self.size);
        whence&=~AVSEEK_FORCE;
        int64_t base=0;
        if(whence==SEEK_CUR) base=static_cast<int64_t>(self.position);
        else if(whence==SEEK_END) base=static_cast<int64_t>(self.size);
        else if(whence!=SEEK_SET) return AVERROR(EINVAL);
        if(offset>static_cast<int64_t>(self.size)-base || offset < -base) return AVERROR(EINVAL);
        self.position=static_cast<std::uint64_t>(base+offset);
        return static_cast<int64_t>(self.position);
    }
    bool open() {
        closeContexts();
        position=0; callbackError=false; packetPending=inputEof=flushSent=codecEof=finished=false;
        stagePosition=stageFrames=0; audioIndex=-1; inputRate=0; inputFormat=AV_SAMPLE_FMT_NONE;
        error.clear();
        auto* buffer=static_cast<UINT8*>(av_malloc(IOBytes));
        if(!buffer) return fail("Cannot allocate bounded audio AVIO buffer");
        io=avio_alloc_context(buffer,IOBytes,0,this,read,nullptr,seek);
        if(!io) { av_free(buffer); return fail("Cannot allocate custom audio AVIO context"); }
        format=avformat_alloc_context();
        if(!format) return fail("Cannot allocate audio demuxer");
        format->pb=io; format->flags|=AVFMT_FLAG_CUSTOM_IO;
        format->io_open=denyExtraIO;
        format->probesize=64*1024;
        format->max_analyze_duration=AV_TIME_BASE;
        format->max_probe_packets=128;
        format->max_index_size=256*1024;
        format->max_streams=8;
        // This adapter handles the game sound formats. Intro-video demuxing is
        // a separate owner path; chained/external resource opens are denied.
        AVDictionary* options=nullptr;
        av_dict_set(&options,"format_whitelist","wav,mp3,ogg,bink,avi",0);
        int result=avformat_open_input(&format,nullptr,nullptr,&options);
        av_dict_free(&options);
        if(result<0) return fail("Unsupported or malformed WAV/MP3/OGG/Bink/AVI audio",result);
        result=avformat_find_stream_info(format,nullptr);
        if(result<0) return fail("Cannot inspect audio stream",result);
        const AVCodec* implementation=nullptr;
        audioIndex=av_find_best_stream(format,AVMEDIA_TYPE_AUDIO,-1,-1,&implementation,0);
        if(audioIndex<0 || !implementation) return fail("No supported audio decoder",audioIndex);
        codec=avcodec_alloc_context3(implementation);
        if(!codec) return fail("Cannot allocate audio codec context");
        result=avcodec_parameters_to_context(codec,format->streams[audioIndex]->codecpar);
        if(result<0) return fail("Cannot apply audio codec parameters",result);
        codec->thread_count=1;
        codec->max_samples=MaxInputSamples;
        codec->err_recognition=AV_EF_CRCCHECK|AV_EF_CAREFUL|AV_EF_EXPLODE;
        result=avcodec_open2(codec,implementation,nullptr);
        if(result<0) return fail("Cannot open approved audio codec",result);
        packet=av_packet_alloc(); frame=av_frame_alloc();
        if(!packet || !frame) return fail("Cannot allocate audio packet/frame");
        return true;
    }
    void reserveStage(int frames) {
        const auto count=static_cast<std::size_t>(frames)*2;
        // Avoid geometric vector growth exceeding the declared staging cap.
        if(staging.capacity()<count) { std::vector<float> replacement(count); staging.swap(replacement); }
        else staging.resize(count);
    }
    bool convertFrame() {
        if(frame->nb_samples<=0 || frame->nb_samples>MaxInputSamples || frame->sample_rate<8000 || frame->sample_rate>192000 ||
           frame->ch_layout.nb_channels<1 || frame->ch_layout.nb_channels>8 || frame->decode_error_flags)
            return fail("Invalid audio frame or frame exceeds bounded native decoder limits");
        if(!resampler) {
            AVChannelLayout stereo=AV_CHANNEL_LAYOUT_STEREO;
            int result=swr_alloc_set_opts2(&resampler,&stereo,AV_SAMPLE_FMT_FLT,OutputRate,
                                          &frame->ch_layout,static_cast<AVSampleFormat>(frame->format),frame->sample_rate,0,nullptr);
            if(result<0 || !resampler) return fail("Cannot allocate audio resampler",result);
            result=swr_init(resampler);
            if(result<0) return fail("Cannot initialize audio resampler",result);
            if(av_channel_layout_copy(&inputLayout,&frame->ch_layout)<0) return fail("Cannot retain audio channel layout");
            inputRate=frame->sample_rate; inputFormat=static_cast<AVSampleFormat>(frame->format);
        } else if(inputRate!=frame->sample_rate || inputFormat!=frame->format || av_channel_layout_compare(&inputLayout,&frame->ch_layout)!=0)
            return fail("Audio changes format midstream; refusing an unverified conversion");
        const int required=swr_get_out_samples(resampler,frame->nb_samples);
        if(required<=0 || required>MaxOutputSamples) return fail("Audio resampling frame exceeds bounded output staging");
        reserveStage(required);
        UINT8* destination=reinterpret_cast<UINT8*>(staging.data());
        const UINT8* inputs[8]{};
        const int planes=av_sample_fmt_is_planar(static_cast<AVSampleFormat>(frame->format))?frame->ch_layout.nb_channels:1;
        for(int i=0;i<planes;++i) inputs[i]=frame->extended_data[i];
        const int produced=swr_convert(resampler,&destination,required,inputs,frame->nb_samples);
        if(produced<0) return fail("Audio resampling failed",produced);
        stagePosition=0; stageFrames=static_cast<std::size_t>(produced);
        for(std::size_t i=0;i<stageFrames*2;++i) if(!std::isfinite(staging[i])) return fail("Audio decoder produced nonfinite PCM");
        return true;
    }
    bool nextStage() {
        stagePosition=stageFrames=0;
        // Bound work on adversarial metadata/empty packets, independently of
        // source length. Normal packets/frames always return before this cap.
        for(unsigned work=0;work<65536;++work) {
            if(codecEof) {
                if(!resampler) { finished=true; return true; }
                const int required=std::max(swr_get_out_samples(resampler,0),128);
                if(required>MaxOutputSamples) return fail("Audio resampler drain exceeds bound");
                reserveStage(required);
                UINT8* destination=reinterpret_cast<UINT8*>(staging.data());
                const int count=swr_convert(resampler,&destination,required,nullptr,0);
                if(count<0) return fail("Cannot drain audio resampler",count);
                stageFrames=static_cast<std::size_t>(count);
                for(std::size_t i=0;i<stageFrames*2;++i) if(!std::isfinite(staging[i])) return fail("Audio resampler drain produced nonfinite PCM");
                if(!count) finished=true;
                return true;
            }
            int result=avcodec_receive_frame(codec,frame);
            if(result>=0) {
                const bool valid=convertFrame(); av_frame_unref(frame);
                if(!valid) return false;
                if(stageFrames) return true;
                continue;
            }
            if(result==AVERROR_EOF) { codecEof=true; continue; }
            if(result!=AVERROR(EAGAIN)) return fail("Audio codec decode failed",result);
            if(packetPending) {
                result=avcodec_send_packet(codec,packet);
                if(result<0) return fail("Audio codec rejected packet",result);
                av_packet_unref(packet); packetPending=false; continue;
            }
            if(inputEof) {
                if(flushSent) return fail("Audio codec stalled while draining");
                result=avcodec_send_packet(codec,nullptr);
                if(result<0 && result!=AVERROR_EOF) return fail("Cannot drain audio codec",result);
                flushSent=true; continue;
            }
            result=av_read_frame(format,packet);
            if(result==AVERROR_EOF) { av_packet_unref(packet); inputEof=true; continue; }
            if(result<0) { av_packet_unref(packet); return fail("Audio demux/read failed",result); }
            // Check and release rejected packets from every stream, including
            // video packets skipped by movie-audio playback.
            const int packetLimit=packet->stream_index==audioIndex?MaxPacketBytes:sgp::NativeCodecContainerPacketLimit;
            if(packet->size<0 || packet->size>packetLimit || (packet->flags&AV_PKT_FLAG_CORRUPT)) {
                av_packet_unref(packet); return fail("Audio packet corrupt or exceeds bounded decoder limit");
            }
            if(packet->stream_index!=audioIndex) { av_packet_unref(packet); continue; }
            packetPending=true;
        }
        return fail("Audio demux/decoder made no bounded progress");
    }
};

NativeAudioDecoder::NativeAudioDecoder(std::unique_ptr<Impl> impl):impl_(std::move(impl)) { ++liveInstances; }
NativeAudioDecoder::~NativeAudioDecoder() { --liveInstances; }
std::unique_ptr<NativeAudioDecoder> NativeAudioDecoder::OpenFile(const char* filename,std::string& error) {
    sgp::ApplyNativeCodecMemoryPolicy();
    error.clear();
    if(!filename || !*filename) { error="Missing audio filename"; return {}; }
    auto impl=std::make_unique<Impl>();
    {
        std::lock_guard<std::mutex> lock(sourceGuard);
        try {
            impl->file=FileOpen(const_cast<char*>(filename),FILE_ACCESS_READ|FILE_OPEN_EXISTING,FALSE);
            if(impl->file) impl->size=FileGetSize(impl->file);
        } catch(...) { error="FileMan/VFS source open failed"; return {}; }
    }
    if(!impl->file || !impl->size) { error="Audio source is absent, empty, or outside the public FileMan32-bit size range"; return {}; }
    if(!impl->open()) { error=impl->error; return {}; }
    return std::unique_ptr<NativeAudioDecoder>(new NativeAudioDecoder(std::move(impl)));
}
std::unique_ptr<NativeAudioDecoder> NativeAudioDecoder::OpenMemory(const void* bytes,UINT32 size,std::string& error) {
    sgp::ApplyNativeCodecMemoryPolicy();
    error.clear();
    if(!bytes || !size) { error="Missing encoded audio buffer"; return {}; }
    auto impl=std::make_unique<Impl>(); impl->memory=static_cast<const UINT8*>(bytes); impl->size=size;
    if(!impl->open()) { error=impl->error; return {}; }
    return std::unique_ptr<NativeAudioDecoder>(new NativeAudioDecoder(std::move(impl)));
}
NativeAudioDecoder::ReadResult NativeAudioDecoder::ReadFrames(float* stereo,UINT32 capacity) {
    auto& state=*impl_; ReadResult result;
    if(!state.error.empty()) { result.error=true; return result; }
    if(!stereo || !capacity || capacity>MaxReadFrames) { state.fail("Invalid bounded audio read buffer/capacity"); result.error=true; return result; }
    while(result.frames<capacity) {
        if(state.stagePosition==state.stageFrames) {
            if(state.finished) break;
            if(!state.nextStage()) { result.error=true; break; }
            if(state.finished && !state.stageFrames) break;
        }
        const auto count=std::min<std::size_t>(capacity-result.frames,state.stageFrames-state.stagePosition);
        std::memcpy(stereo+result.frames*2,state.staging.data()+state.stagePosition*2,count*2*sizeof(float));
        state.stagePosition+=count; result.frames+=static_cast<UINT32>(count);
    }
    result.eof=state.finished && state.stagePosition==state.stageFrames;
    return result;
}
bool NativeAudioDecoder::Rewind() { return impl_->open(); }
const std::string& NativeAudioDecoder::Error() const { return impl_->error; }
std::uint64_t NativeAudioDecoder::SourceBytesRead() const { return impl_->bytesRead; }
std::uint64_t NativeAudioDecoder::SourceSize() const { return impl_->size; }
std::size_t NativeAudioDecoder::BufferedBytes() const { return impl_->staging.capacity()*sizeof(float)+IOBytes; }
UINT32 NativeAudioDecoder::LiveInstances() { return liveInstances.load(); }
