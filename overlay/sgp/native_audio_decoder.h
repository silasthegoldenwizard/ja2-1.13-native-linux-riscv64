/*
 * P710 native snapshot update: sgp/native_audio_decoder.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// P710 native decoder boundary. The approved pinned FFmpeg SDK is private to
// NativeAudioDecoder.cpp; callers need no FFmpeg headers or opaque AV structs.
#ifndef SGP_NATIVE_AUDIO_DECODER_H
#define SGP_NATIVE_AUDIO_DECODER_H
#include "core_types.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

class NativeAudioDecoder {
public:
    static constexpr UINT32 OutputRate=48000;
    static constexpr UINT32 OutputChannels=2;
    static constexpr UINT32 MaxReadFrames=16384;
    struct ReadResult { UINT32 frames=0; bool eof=false; bool error=false; };
    // File sources exclusively use the original FileMan/VFS opaque registry.
    // No FFmpeg filenames/protocol opens or subprocesses are used. Per-decoder
    // logical cursors isolate multiple open instances of the same VFS source.
    static std::unique_ptr<NativeAudioDecoder> OpenFile(const char* filename, std::string& error);
    // Original SoundPlayFromBuffer ownership: caller retains the encoded bytes
    // and must keep them valid until the voice stops. No full source copy.
    static std::unique_ptr<NativeAudioDecoder> OpenMemory(const void* bytes, UINT32 size, std::string& error);
    ~NativeAudioDecoder();
    NativeAudioDecoder(const NativeAudioDecoder&)=delete;
    NativeAudioDecoder& operator=(const NativeAudioDecoder&)=delete;
    // Caller/main-thread only. Decode bounded chunks into caller-owned stereo
    // interleaved float48k output. EOF follows codec AND resampler drain. A
    // result can contain final PCM and eof=true; errors are never silence-success.
    ReadResult ReadFrames(float* stereo, UINT32 capacity);
    // Seek the owned AVIO source back to byte0 and recreate demux/codec/resampler
    // state, including MP3/Vorbis preroll/skip metadata, for exact loop restarts.
    bool Rewind();
    const std::string& Error() const;
    std::uint64_t SourceBytesRead() const;
    std::uint64_t SourceSize() const;
    // Conservative adapter-owned output staging bound (not FFmpeg-internal
    // codec/demux allocation). One frame is admitted only within documented caps.
    std::size_t BufferedBytes() const;
    static UINT32 LiveInstances();
private:
    struct Impl;
    explicit NativeAudioDecoder(std::unique_ptr<Impl> impl);
    std::unique_ptr<Impl> impl_;
};
#endif
