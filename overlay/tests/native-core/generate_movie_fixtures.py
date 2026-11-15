#!/usr/bin/env python3
"""Independent, hand-encoded AVI fixtures; stdlib only, no proprietary Bink data.

The AVI rawvideo fixture exercises the SAME native demux/decode/RGB565/present
pipeline as Bink, but does not prove the Bink bitstream decoder's correctness.
"""
# P710 native snapshot update: tests/native-core/generate_movie_fixtures.py
# Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
# This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
# Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
# Update notice inserted during source-only package preparation: 2026-11-15.
# This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
# Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.

import argparse
import pathlib
import struct

def u32(*n): return struct.pack('<'+'I'*len(n),*n)
def chunk(tag,data): return tag+u32(len(data))+data+(b'\0' if len(data)&1 else b'')
def listing(tag,data): return chunk(b'LIST',tag+data)
def avi(path,width=3,height=2,frames=3,audio=False,bad_audio=False,bad_video=False):
    stride=(width*3+3)&~3
    size=stride*height
    avih=u32(100000, size*10+(192000 if audio else 0),0,0,frames,0,2 if audio else 1,size,width,height,0,0,0,0)
    # AVISTREAMHEADER: type/handler/flags/priority/language/initial/scale/rate/
    # start/length/suggested/quality/sample-size/rect (56 bytes).
    strh=b'vidsDIB '+u32(0)+struct.pack('<HH',0,0)+u32(0,1,10,0,frames,size,0xffffffff,0)+struct.pack('<hhhh',0,0,width,height)
    strf=struct.pack('<IiiHHIIiiII',40,width,height,1,24,0,size,0,0,0,0)
    hdrl=chunk(b'avih',avih)+listing(b'strl',chunk(b'strh',strh)+chunk(b'strf',strf))
    if audio:
        ah=b'auds'+b'\0'*4+u32(0)+struct.pack('<HH',0,0)+u32(0,4,192000,0,frames*4800,19200,0xffffffff,4)+struct.pack('<hhhh',0,0,0,0)
        af=struct.pack('<HHIIHH',1,2,48000,192000,4,16)
        hdrl+=listing(b'strl',chunk(b'strh',ah)+chunk(b'strf',af))
    movi=b''
    colors=[(0,0,255),(0,255,0),(255,0,0)]
    for i in range(frames):
        row=bytes(colors[i%3])*width+b'\0'*(stride-width*3)
        movi+=chunk(b'00db',b'\x01' if bad_video and i==20 else row*height)
        if audio:
            # Explicit nonzero stereo PCM, no encoder or device assumptions.
            pcm=b''.join(struct.pack('<hh',2000 if j%2 else -2000,1000 if j%2 else -1000) for j in range(4800))
            movi+=chunk(b'01wb',b'\x01' if bad_audio and i==20 else pcm)
    body=b'AVI '+listing(b'hdrl',hdrl)+listing(b'movi',movi)
    path.write_bytes(b'RIFF'+u32(len(body))+body)

def sparse_oversize(source,target,tag,occurrence=0,zero_audio_sample=False):
    # Author a physically sized64MiB packet without a64MiB Python heap copy.
    data=source.read_bytes()
    movi=data.index(b'movi')
    pos=movi+4
    seen=0
    while pos<len(data):
        size=struct.unpack_from('<I',data,pos+4)[0]
        if data[pos:pos+4]==tag:
            if seen==occurrence: break
            seen+=1
        pos+=8+size+(size&1)
    else: raise ValueError('missing generated packet')
    old=struct.unpack_from('<I',data,pos+4)[0]
    huge=64*1024*1024
    prefix=bytearray(data[:pos+8])
    struct.pack_into('<I',prefix,pos+4,huge)
    struct.pack_into('<I',prefix,4,len(data)-8+huge-old)
    struct.pack_into('<I',prefix,movi-4,struct.unpack_from('<I',data,movi-4)[0]+huge-old)
    if zero_audio_sample:
        audio_header=prefix.index(b'auds')
        struct.pack_into('<I',prefix,audio_header+44,0)
    with target.open('wb') as out:
        out.write(prefix)
        out.write(data[pos+8:pos+8+old])
        out.seek(pos+8+huge)
        out.write(data[pos+8+old+(old&1):])

def main():
    p=argparse.ArgumentParser(); p.add_argument('output',type=pathlib.Path); args=p.parse_args()
    args.output.mkdir(parents=True,exist_ok=True)
    avi(args.output/'movie.avi')
    avi(args.output/'audio-movie.avi',audio=True)
    avi(args.output/'large-movie.avi',64,32,120)
    avi(args.output/'bad-audio-movie.avi',frames=30,audio=True,bad_audio=True)
    avi(args.output/'bad-video-movie.avi',frames=30,bad_video=True)
    sparse_oversize(args.output/'movie.avi',args.output/'oversize-probe-movie.avi',b'00db')
    avi(args.output/'long-audio-movie.avi',frames=30,audio=True)
    sparse_oversize(args.output/'long-audio-movie.avi',args.output/'oversize-skipped-audio.avi',b'01wb',20,True)
    (args.output/'bad-movie.avi').write_bytes(b'RIFF'+u32(24)+b'AVI LIST'+u32(100000)+b'hdrlbad')
    (args.output/'external-movie.avi').write_bytes(b'#EXTM3U\nhttps://example.invalid/private.bik\n')
if __name__=='__main__': main()
