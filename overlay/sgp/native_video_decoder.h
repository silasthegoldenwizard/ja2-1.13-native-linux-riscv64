/*
 * P710 native snapshot update: sgp/native_video_decoder.h
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// P710 authored native movie boundary; no FFmpeg types escape this header.
#ifndef SGP_NATIVE_VIDEO_DECODER_H
#define SGP_NATIVE_VIDEO_DECODER_H
#include "core_types.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
class NativeVideoDecoder {
public:
    struct Frame {
        // Borrowed until the next ReadFrame/Rewind/destruction. RGB565 native
        // endian, padded rows, bounded against the real decoded dimensions.
        const UINT8* pixels=nullptr;
        UINT32 width=0, height=0, pitch=0;
        std::size_t bytes=0;
        std::uint64_t timeMs=0, durationMs=0;
    };
    struct ReadResult { bool frame=false, eof=false, error=false; };
    static std::unique_ptr<NativeVideoDecoder> OpenFile(const char*,std::string& error);
    ~NativeVideoDecoder();
    NativeVideoDecoder(const NativeVideoDecoder&)=delete;
    NativeVideoDecoder& operator=(const NativeVideoDecoder&)=delete;
    // Caller/main thread only. A single owned frame; finite demux/decode work.
    ReadResult ReadFrame(Frame&);
    // Reopen the same owned VFS source at byte zero, resetting codec timestamps.
    bool Rewind();
    UINT32 Width() const;
    UINT32 Height() const;
    bool HasAudio() const;
    const std::string& Error() const;
    std::uint64_t SourceBytesRead() const;
    std::size_t BufferedBytes() const;
    // Actual referenced AVPacket buffer extent; zero after packet rejection.
    std::size_t ReferencedPacketBytes() const;
    static UINT32 LiveInstances();
private:
    struct Impl;
    explicit NativeVideoDecoder(std::unique_ptr<Impl>);
    std::unique_ptr<Impl> impl_;
};
#endif
