/*
 * P710 native snapshot update: tests/native-core/native_quantizer_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Actual original octree and palette mapping with authored, unequal channels.
#include "types.h"
#include "image_types.h"
#include "Quantize.h"
#include "Quantize Wrap.h"
#include <array>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <stdexcept>
namespace {
unsigned checks=0;
void check(bool ok,const char* why) { ++checks; if(!ok) throw std::runtime_error(why); }
}
int main() {
    try {
        static_assert(sizeof(QuantizerColor)==4 && offsetof(QuantizerColor,rgbBlue)==0 &&
                      offsetof(QuantizerColor,rgbGreen)==1 && offsetof(QuantizerColor,rgbRed)==2 &&
                      offsetof(QuantizerColor,rgbReserved)==3,"original B,G,R,reserved quantizer layout");
        static_assert(sizeof(QuantizerBool)==4 && sizeof(QuantizerUInt)==4 && sizeof(QuantizerDWord)==4,"original scalar widths");
        BYTE sameBin[]{12,20,32, 13,23,34};
        CQuantizer average(255,6); check(average.ProcessImage(sameBin,2,1) && average.GetColorCount()==1,"original six-bit octree merges same-bin colors");
        QuantizerColor mean[256]{}; average.GetColorTable(mean);
        check(mean[0].rgbBlue==12 && mean[0].rgbGreen==21 && mean[0].rgbRed==33 && mean[0].rgbReserved==0,"unequal channels retain integer-averaged B,G,R layout");
        BYTE source[]{255,0,0, 0,255,0, 0,0,255, 0,0,0, 255,0,0, 0,255,0};
        std::array<UINT8,6> destination{}; std::array<SGPPaletteEntry,256> palette{};
        check(QuantizeImage(destination.data(),source,3,2,palette.data()),"actual original wrapper and palette mapper");
        for (std::size_t i=0;i<destination.size();++i) {
            const auto& entry=palette[destination[i]];
            check(entry.peRed==source[i*3] && entry.peGreen==source[i*3+1] && entry.peBlue==source[i*3+2] && !entry.peFlags,"reconstructed RGB pixels match original exact-color source");
        }
        check(destination[0]==destination[4] && destination[1]==destination[5],"repeated colors map to identical palette entries");
        // Force actual original ReduceTree/free branches, without asserting a new quality metric.
        CQuantizer reduced(2,6); check(reduced.ProcessImage(source,6,1) && reduced.GetColorCount()<=2,"original octree reduction respects requested color count");
        QuantizerColor colors[256]{}; reduced.GetColorTable(colors);
        for(unsigned i=0;i<reduced.GetColorCount();++i) check(colors[i].rgbReserved==0,"reduced palette reserved bytes remain zero");
        std::cout<<"PASS: "<<checks<<" original quantizer/channel checks; authored pixels only\n"; return 0;
    } catch(const std::exception& error) { std::cerr<<error.what()<<'\n'; return 1; }
}
