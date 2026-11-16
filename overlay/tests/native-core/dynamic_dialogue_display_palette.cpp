/*
 * P710 native snapshot update: tests/native-core/dynamic_dialogue_display_palette.cpp
 * Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Exact production Display/Destroy routines with synthetic drawing services.
// This fixture models ownership and callback behavior, not the complete ABI.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <unordered_set>

using UINT8 = std::uint8_t;
using UINT16 = std::uint16_t;
using UINT32 = std::uint32_t;
using BOOLEAN = bool;
constexpr BOOLEAN TRUE = true, FALSE = false;
constexpr unsigned VOBJECT_CREATE_FROMFILE = 1, PROFILETYPE_IMP = 1;
constexpr unsigned FRAME_BUFFER = 0, VO_BLT_SRCTRANSPARENCY = 1, RENDER_FLAG_FULL = 1;
#define FROMRGB(r, g, b) ((r) | ((g) << 8) | ((b) << 16))

struct PaletteEntry {};
struct VideoObject {
    PaletteEntry* pPaletteEntry;
    UINT16* pShades[1];
    UINT16* pShadeCurrent;
};
using HVOBJECT = VideoObject*;
struct VOBJECT_DESC { unsigned fCreateFlags; char ImageFile[128]; };
struct Profile { unsigned Type = 0, ubFaceIndex = 0; };
Profile gMercProfiles[1];

namespace {
unsigned checks = 0, allocations = 0, attempts = 0, frees = 0, deletes = 0, blits = 0;
bool dead = false, failAllocation = false, faceExists = false;
PaletteEntry entries[256];
UINT16 originalShade[256];
VideoObject face{entries, {originalShade}, originalShade};
std::unordered_set<UINT16*> owned;
void require(bool value, const char* reason) {
    ++checks;
    if (!value) throw std::runtime_error(reason);
}
}

BOOLEAN AddVideoObject(VOBJECT_DESC* description, UINT32* index) {
    require(description->fCreateFlags == VOBJECT_CREATE_FROMFILE, "load actual face path branch");
    require(!faceExists, "load face once per box creation");
    faceExists = true; *index = 41; return TRUE;
}
BOOLEAN GetVideoObject(HVOBJECT* object, UINT32 index) {
    require(index == 41 && faceExists, "display resolves its live face");
    *object = &face; return TRUE;
}
void FilenameForBPP(char* input, char* output) { std::strcpy(output, input); }
BOOLEAN IsMercDead(UINT16) { return dead; }
UINT16* Create16BPPPaletteShaded(PaletteEntry* palette, UINT32 red, UINT32 green,
                               UINT32 blue, BOOLEAN mono) {
    ++attempts;
    require(palette == entries && red == 255 && green == 55 && blue == 55 && mono,
            "dead face retains original color parameters");
    if (failAllocation) { failAllocation = false; return nullptr; }
    auto* result = new UINT16[256]{};
    owned.insert(result); ++allocations; return result;
}
void MemFree(void* memory) {
    auto* palette = static_cast<UINT16*>(memory);
    require(owned.erase(palette) == 1, "free the box-owned palette exactly once");
    delete[] palette; ++frees;
}
void DeleteVideoObjectFromIndex(UINT32 index) {
    require(index == 41 && faceExists, "destroy the live face once");
    require(face.pShades[0] == originalShade, "face table ownership remains unchanged at deletion");
    faceExists = false; face.pShadeCurrent = originalShade; ++deletes;
}
void SetObjectHandleShade(UINT32, UINT32 shade) { face.pShadeCurrent = face.pShades[shade]; }
void BltVideoObject(UINT32, HVOBJECT object, unsigned, unsigned, unsigned, UINT32, void*) {
    require(object == &face, "paint the live face"); ++blits;
}
void ColorFillVideoSurfaceArea(UINT32, unsigned, unsigned, unsigned, unsigned, UINT16) {}
UINT16 Get16BPPColor(unsigned color) { return static_cast<UINT16>(color); }
void SetRenderFlags(unsigned) {}

class DDBox {
public:
    BOOLEAN mfInit = TRUE, mfDisplayed = TRUE, mfFaceImageExists = FALSE;
    UINT32 musFaceImage = 0;
    UINT16* mpDeadFacePalette = nullptr;
    unsigned musWidth = 10, musFontHeight = 10;
    struct { UINT16 usSpeaker = 0; } mEvent;
    BOOLEAN IsInit() { return mfInit; }
    BOOLEAN IsDisplayed() { return mfDisplayed; }
    unsigned GetX() { return 10; }
    unsigned GetY() { return 10; }
    unsigned GetX_Text() { return 20; }
    unsigned GetY_Text() { return 10; }
    void DrawTopEntry() {}
    void Display();
    void Destroy();
};

#include "dynamic_dialogue_display.inc"

int main() {
    try {
        DDBox box;
        box.Display();
        require(attempts == 0 && face.pShadeCurrent == originalShade, "live face uses its original palette");
        dead = true; failAllocation = true;
        box.Display();
        require(attempts == 1 && allocations == 0 && !box.mpDeadFacePalette,
                "allocation failure retains retry state");
        require(face.pShades[0] == originalShade && face.pShadeCurrent == originalShade,
                "allocation failure preserves the original face table and selection");
        box.Display();
        auto* red = box.mpDeadFacePalette;
        require(red && owned.count(red) == 1 && face.pShadeCurrent == red,
                "successful allocation selects the box-owned red palette");
        for (unsigned repaint = 0; repaint < 8; ++repaint) {
            box.Display();
            require(box.mpDeadFacePalette == red && face.pShadeCurrent == red,
                    "repaint retains the same red palette");
            require(face.pShades[0] == originalShade && allocations == 1 && attempts == 2,
                    "repaint neither replaces a face table nor allocates another palette");
        }
        require(blits == 11, "all live, failed-allocation, and dead repaints reach the blitter");
        box.Destroy();
        require(owned.empty() && frees == 1 && deletes == 1 && !box.mpDeadFacePalette,
                "destroy releases the face and its separate palette");
        box.Destroy(); box.Display();
        require(frees == 1 && deletes == 1 && blits == 11, "repeated cleanup and hidden display are harmless");
        box.mfInit = box.mfDisplayed = TRUE;
        box.Display();
        require(allocations == 2 && attempts == 3 && face.pShades[0] == originalShade,
                "recreated box obtains its own fresh palette");
        box.Destroy();
        require(owned.empty() && frees == 2 && deletes == 2, "recreated palette is released once");
        std::cout << "PASS " << checks << " dynamic-dialogue display palette checks\n";
        return 0;
    } catch (const std::exception& error) {
        for (auto* palette : owned) delete[] palette;
        std::cerr << error.what() << '\n'; return 1;
    }
}
