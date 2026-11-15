/*
 * P710 native snapshot update: sgp/NativeVideoDecoder.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// P710 adapter: approved FFmpeg public APIs; custom AVIO exclusively backed by
// the actual FileMan/VFS registry. No pathname, protocol, external resource or
// subprocess is handed to FFmpeg. Only Bink and generated AVI demuxers admitted.
#include "native_video_decoder.h"
#include "native_codec_limits.h"
#include "FileManIO.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
#include <libswscale/swscale.h>
}
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <limits>
#include <mutex>
#include <utility>
#include <vector>
namespace {
constexpr int IOBytes=32768, MaxDimension=4096, MaxPixels=4096*2160;
constexpr int MaxPacketBytes=sgp::NativeCodecContainerPacketLimit;
std::atomic<UINT32> live{0};
// FileMan/VFS is a main-thread service. Logical position belongs to each
// decoder; every physical read is preceded by restoring that logical cursor.
std::mutex sourceGuard;
int denyExtraIO(AVFormatContext*,AVIOContext**,const char*,int,AVDictionary**) { return AVERROR(EACCES); }
std::string describe(int code) { char b[AV_ERROR_MAX_STRING_SIZE]{}; av_strerror(code,b,sizeof(b)); return b; }
bool dimensions(int w,int h) { return w>0 && h>0 && w<=MaxDimension && h<=MaxDimension && static_cast<int64_t>(w)*h<=MaxPixels; }
}
struct NativeVideoDecoder::Impl {
    HWFILE file=0;
    std::uint64_t size=0,position=0,bytesRead=0;
    AVFormatContext* format=nullptr;
    AVIOContext* io=nullptr;
    AVCodecContext* codec=nullptr;
    AVPacket* packet=nullptr;
    AVFrame* frame=nullptr;
    SwsContext* scaler=nullptr;
    int stream=-1,width=0,height=0,pitch=0;
    bool audio=false,inputEof=false,flushSent=false,finished=false,callbackError=false;
    int64_t firstPts=AV_NOPTS_VALUE;
    std::uint64_t nextTime=0,lastTime=0,defaultDuration=0;
    std::vector<UINT8> rgb;
    std::string error;
    ~Impl() { close(); if(file) { try { FileClose(file); } catch(...) {} } }
    void close() {
        sws_freeContext(scaler); scaler=nullptr;
        av_frame_free(&frame); av_packet_free(&packet); avcodec_free_context(&codec);
        avformat_close_input(&format);
        if(io) av_freep(&io->buffer);
        avio_context_free(&io);
    }
    bool fail(const char* text,int code=0) {
        if(error.empty()) { error=text; if(code<0) error+=": "+describe(code); if(callbackError) error+=" (FileMan/VFS callback failed)"; }
        return false;
    }
    static int read(void* opaque,UINT8* dest,int requested) {
        auto& s=*static_cast<Impl*>(opaque);
        if(requested<=0 || !dest) return AVERROR(EINVAL);
        if(s.position>=s.size) return AVERROR_EOF;
        const auto count=static_cast<UINT32>(std::min<std::uint64_t>(requested,s.size-s.position));
        std::lock_guard<std::mutex> lock(sourceGuard);
        try {
            const auto first=static_cast<UINT32>(std::min<std::uint64_t>(s.position,INT32_MAX));
            if(!FileSeek(s.file,first,FILE_SEEK_FROM_START)) { s.callbackError=true; return AVERROR(EIO); }
            auto remaining=s.position-first;
            while(remaining) {
                const auto step=static_cast<UINT32>(std::min<std::uint64_t>(remaining,INT32_MAX));
                if(!FileSeek(s.file,step,FILE_SEEK_FROM_CURRENT)) { s.callbackError=true; return AVERROR(EIO); }
                remaining-=step;
            }
            UINT32 got=0;
            if(!FileRead(s.file,dest,count,&got) || got!=count) { s.callbackError=true; return AVERROR(EIO); }
        } catch(...) { s.callbackError=true; return AVERROR(EIO); }
        s.position+=count; s.bytesRead+=count; return static_cast<int>(count);
    }
    static int64_t seek(void* opaque,int64_t offset,int whence) {
        auto& s=*static_cast<Impl*>(opaque);
        if(whence&AVSEEK_SIZE) return static_cast<int64_t>(s.size);
        whence&=~AVSEEK_FORCE;
        int64_t base=0;
        if(whence==SEEK_CUR) base=static_cast<int64_t>(s.position);
        else if(whence==SEEK_END) base=static_cast<int64_t>(s.size);
        else if(whence!=SEEK_SET) return AVERROR(EINVAL);
        if(offset>static_cast<int64_t>(s.size)-base || offset < -base) return AVERROR(EINVAL);
        s.position=static_cast<std::uint64_t>(base+offset); return static_cast<int64_t>(s.position);
    }
    bool open() {
        close(); error.clear(); position=0; stream=-1; firstPts=AV_NOPTS_VALUE; nextTime=lastTime=0;
        audio=inputEof=flushSent=finished=callbackError=false;
        auto* buffer=static_cast<UINT8*>(av_malloc(IOBytes));
        if(!buffer) return fail("Cannot allocate bounded video AVIO buffer");
        io=avio_alloc_context(buffer,IOBytes,0,this,read,nullptr,seek);
        if(!io) { av_free(buffer); return fail("Cannot allocate video custom AVIO"); }
        format=avformat_alloc_context(); if(!format) return fail("Cannot allocate video demuxer");
        format->pb=io; format->flags|=AVFMT_FLAG_CUSTOM_IO; format->io_open=denyExtraIO;
        format->probesize=128*1024; format->max_analyze_duration=AV_TIME_BASE;
        format->max_probe_packets=128; format->max_index_size=256*1024; format->max_streams=16;
        AVDictionary* options=nullptr; av_dict_set(&options,"format_whitelist","bink,avi",0);
        int result=avformat_open_input(&format,nullptr,nullptr,&options); av_dict_free(&options);
        if(result<0) return fail("Unsupported or malformed Bink/AVI movie",result);
        result=avformat_find_stream_info(format,nullptr);
        if(result<0) return fail("Cannot inspect movie streams",result);
        const AVCodec* implementation=nullptr;
        stream=av_find_best_stream(format,AVMEDIA_TYPE_VIDEO,-1,-1,&implementation,0);
        if(stream<0 || !implementation) return fail("No approved movie video decoder",stream);
        auto* selected=format->streams[stream]; width=selected->codecpar->width; height=selected->codecpar->height;
        if(!dimensions(width,height)) return fail("Movie exceeds bounded native dimensions");
        for(unsigned i=0;i<format->nb_streams;++i) if(format->streams[i]->codecpar->codec_type==AVMEDIA_TYPE_AUDIO) audio=true;
        auto rate=av_guess_frame_rate(format,selected,nullptr);
        if(rate.num<=0 || rate.den<=0 || av_q2d(rate)>1000 || av_q2d(rate)<0.1) return fail("Invalid movie frame cadence");
        defaultDuration=std::max<int64_t>(1,av_rescale_q(1,av_inv_q(rate),AVRational{1,1000}));
        codec=avcodec_alloc_context3(implementation); if(!codec) return fail("Cannot allocate movie codec");
        result=avcodec_parameters_to_context(codec,selected->codecpar);
        if(result<0) return fail("Cannot apply movie codec parameters",result);
        codec->thread_count=1; codec->max_pixels=MaxPixels;
        codec->err_recognition=AV_EF_CRCCHECK|AV_EF_CAREFUL|AV_EF_EXPLODE;
        result=avcodec_open2(codec,implementation,nullptr);
        if(result<0) return fail("Cannot open approved movie codec",result);
        packet=av_packet_alloc(); frame=av_frame_alloc();
        if(!packet || !frame) return fail("Cannot allocate bounded movie packet/frame");
        pitch=(width*2+63)&~63;
        rgb.resize(static_cast<std::size_t>(pitch)*height+AV_INPUT_BUFFER_PADDING_SIZE);
        return true;
    }
    bool convert(NativeVideoDecoder::Frame& out) {
        if(frame->width!=width || frame->height!=height || !dimensions(frame->width,frame->height) || frame->decode_error_flags || (frame->flags&AV_FRAME_FLAG_CORRUPT))
            return fail("Invalid, corrupt or changing movie frame dimensions");
        scaler=sws_getCachedContext(scaler,width,height,static_cast<AVPixelFormat>(frame->format),width,height,AV_PIX_FMT_RGB565,SWS_POINT|SWS_BITEXACT,nullptr,nullptr,nullptr);
        if(!scaler) return fail("Cannot allocate movie RGB565 converter");
        UINT8* output[4]={rgb.data(),nullptr,nullptr,nullptr}; const int pitches[4]={pitch,0,0,0};
        if(sws_scale(scaler,frame->data,frame->linesize,0,height,output,pitches)!=height) return fail("Movie RGB565 conversion failed");
        std::uint64_t at=nextTime,duration=defaultDuration;
        const auto pts=frame->best_effort_timestamp;
        if(pts!=AV_NOPTS_VALUE) {
            if(firstPts==AV_NOPTS_VALUE) firstPts=pts;
            // Avoid signed subtraction overflow on hostile timestamp metadata.
            if(pts<firstPts || (firstPts<0 && pts>INT64_MAX+firstPts)) return fail("Invalid movie presentation timestamp");
            const auto ms=av_rescale_q(pts-firstPts,format->streams[stream]->time_base,AVRational{1,1000});
            if(ms<0 || static_cast<std::uint64_t>(ms)<lastTime) return fail("Nonmonotonic movie timestamps");
            at=static_cast<std::uint64_t>(ms);
        }
        if(frame->duration>0) {
            const auto ms=av_rescale_q(frame->duration,format->streams[stream]->time_base,AVRational{1,1000});
            if(ms>0) duration=static_cast<std::uint64_t>(ms);
        }
        if(duration>10000 || at>UINT32_MAX || duration>UINT32_MAX-at) return fail("Movie timeline exceeds bounded playback clock");
        lastTime=at; nextTime=at+duration;
        out={rgb.data(),static_cast<UINT32>(width),static_cast<UINT32>(height),static_cast<UINT32>(pitch),rgb.size(),at,duration};
        return true;
    }
    NativeVideoDecoder::ReadResult next(NativeVideoDecoder::Frame& out) {
        if(!error.empty()) return {false,false,true};
        if(finished) return {false,true,false};
        for(unsigned work=0;work<65536;++work) {
            int result=avcodec_receive_frame(codec,frame);
            if(result>=0) { bool valid=convert(out); av_frame_unref(frame); return {valid,false,!valid}; }
            if(result==AVERROR_EOF) { finished=true; return {false,true,false}; }
            if(result!=AVERROR(EAGAIN)) { fail("Movie video decode failed",result); return {false,false,true}; }
            if(inputEof) {
                if(flushSent) { fail("Movie decoder stalled while draining"); return {false,false,true}; }
                result=avcodec_send_packet(codec,nullptr); flushSent=true;
                if(result<0 && result!=AVERROR_EOF) { fail("Cannot drain movie codec",result); return {false,false,true}; }
                continue;
            }
            result=av_read_frame(format,packet);
            if(result==AVERROR_EOF) { av_packet_unref(packet); inputEof=true; continue; }
            if(result<0) { av_packet_unref(packet); fail("Movie demux/read failed",result); return {false,false,true}; }
            // Admission applies to every stream before any skip. The shared
            // libavutil allocation gate also applies during stream probing.
            if(packet->size<0 || packet->size>MaxPacketBytes || (packet->flags&AV_PKT_FLAG_CORRUPT)) {
                av_packet_unref(packet); fail("Movie packet corrupt or exceeds bound"); return {false,false,true};
            }
            if(packet->stream_index!=stream) { av_packet_unref(packet); continue; }
            result=avcodec_send_packet(codec,packet); av_packet_unref(packet);
            if(result<0) { fail("Movie codec rejected packet",result); return {false,false,true}; }
        }
        fail("Movie decoder made no bounded progress"); return {false,false,true};
    }
};
NativeVideoDecoder::NativeVideoDecoder(std::unique_ptr<Impl> p):impl_(std::move(p)) { ++live; }
NativeVideoDecoder::~NativeVideoDecoder() { --live; }
std::unique_ptr<NativeVideoDecoder> NativeVideoDecoder::OpenFile(const char* filename,std::string& error) {
    sgp::ApplyNativeCodecMemoryPolicy();
    error.clear(); if(!filename || !*filename) { error="Missing movie filename"; return {}; }
    auto p=std::make_unique<Impl>();
    try { p->file=FileOpen(const_cast<char*>(filename),FILE_ACCESS_READ|FILE_OPEN_EXISTING,FALSE); if(p->file) p->size=FileGetSize(p->file); }
    catch(...) { error="FileMan/VFS movie open failed"; return {}; }
    if(!p->file || !p->size) { error="Movie source absent or empty"; return {}; }
    if(!p->open()) { error=p->error; return {}; }
    return std::unique_ptr<NativeVideoDecoder>(new NativeVideoDecoder(std::move(p)));
}
NativeVideoDecoder::ReadResult NativeVideoDecoder::ReadFrame(Frame& frame) { return impl_->next(frame); }
bool NativeVideoDecoder::Rewind() { return impl_->open(); }
UINT32 NativeVideoDecoder::Width() const { return impl_->width; }
UINT32 NativeVideoDecoder::Height() const { return impl_->height; }
bool NativeVideoDecoder::HasAudio() const { return impl_->audio; }
const std::string& NativeVideoDecoder::Error() const { return impl_->error; }
std::uint64_t NativeVideoDecoder::SourceBytesRead() const { return impl_->bytesRead; }
std::size_t NativeVideoDecoder::BufferedBytes() const { return impl_->rgb.capacity()+IOBytes; }
std::size_t NativeVideoDecoder::ReferencedPacketBytes() const { return impl_->packet && impl_->packet->buf?impl_->packet->buf->size:0; }
UINT32 NativeVideoDecoder::LiveInstances() { return live.load(); }
