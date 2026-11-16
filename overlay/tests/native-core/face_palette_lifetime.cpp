/*
 * P710 native snapshot update: tests/native-core/face_palette_lifetime.cpp
 * Native Linux source/build/test adaptations captured in the accepted native development checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Exact production palette initialization and face-gear placement routines.
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using UINT8 = std::uint8_t;
using INT8 = std::int8_t;
using UINT16 = std::uint16_t;
using INT16 = std::int16_t;
using UINT32 = std::uint32_t;
using BOOLEAN = bool;
constexpr BOOLEAN TRUE = true, FALSE = false;
constexpr UINT32 VSURFACE_CREATE_FROMFILE = 1;
constexpr UINT32 FLASH_PORTRAIT_NOSHADE = 0;
constexpr UINT32 FLASH_PORTRAIT_STARTSHADE = 1;
constexpr UINT32 FLASH_PORTRAIT_ENDSHADE = 2;
constexpr UINT32 FLASH_PORTRAIT_DARKSHADE = 3;
constexpr UINT32 FLASH_PORTRAIT_GRAYSHADE = 4;
constexpr UINT32 FLASH_PORTRAIT_LITESHADE = 5;
constexpr unsigned HVOBJECT_SHADE_TABLES = 48;
#define SCSTI ".STI"
#define SCSTI_IMP "_IMP.STI"
#define CHECKF(value) do { if (!(value)) return FALSE; } while (0)

struct SGPPaletteEntry {
    UINT8 peRed = 0, peGreen = 0, peBlue = 0, peFlags = 0;
};
struct ETRLEObject {
    UINT16 usHeight = 0, usWidth = 0;
};
struct VideoObject {
    SGPPaletteEntry* pPaletteEntry = nullptr;
    ETRLEObject* pETRLEObject = nullptr;
    UINT16* pShades[HVOBJECT_SHADE_TABLES]{};
    UINT16* pShadeCurrent = nullptr;
};
using HVOBJECT = VideoObject*;
struct VOBJECT_DESC {
    UINT32 fCreateFlags = 0;
    char ImageFile[512]{};
};
struct FACETYPE {
    UINT16 usFaceHeight = 0;
};
struct FACE_GEAR_VALUES {
    UINT16 Type = 0;
    UINT32 uiIndex = 0;
    char szFile[500]{};
};

namespace {
unsigned checks = 0, allocationAttempts = 0, allocations = 0, frees = 0;
UINT32 nextObject = 1;
std::unordered_set<UINT16*> ownedPalettes;
struct ObjectRecord {
    std::array<SGPPaletteEntry, 256> palette{};
    std::array<ETRLEObject, 1> frames{{{7, 9}}};
    VideoObject object{};
    ObjectRecord() {
        object.pPaletteEntry = palette.data();
        object.pETRLEObject = frames.data();
    }
};
std::unordered_map<UINT32, ObjectRecord*> objects;

void require(bool value, const char* reason) {
    ++checks;
    if (!value) throw std::runtime_error(reason);
}
}

UINT32 gMAXITEMS_READ = 2;
FACE_GEAR_VALUES zNewFaceGear[2];
FACE_GEAR_VALUES zNewFaceGearIMP[2];

UINT16* Create16BPPPaletteShaded(SGPPaletteEntry* palette, UINT32, UINT32,
                                 UINT32, BOOLEAN) {
    ++allocationAttempts;
    require(palette != nullptr, "palette source exists");
    auto* table = new UINT16[256]{};
    ownedPalettes.insert(table);
    ++allocations;
    return table;
}

BOOLEAN AddVideoObject(VOBJECT_DESC* description, UINT32* index) {
    require(description && index && description->fCreateFlags == VSURFACE_CREATE_FROMFILE,
            "face gear uses a file-backed video object");
    auto* record = new ObjectRecord;
    *index = nextObject++;
    objects.emplace(*index, record);
    return TRUE;
}

BOOLEAN GetVideoObject(HVOBJECT* object, UINT32 index) {
    const auto found = objects.find(index);
    if (!object || found == objects.end()) return FALSE;
    *object = &found->second->object;
    return TRUE;
}

BOOLEAN SetObjectShade(HVOBJECT object, UINT32 shade) {
    if (!object || shade >= HVOBJECT_SHADE_TABLES || !object->pShades[shade]) return FALSE;
    object->pShadeCurrent = object->pShades[shade];
    return TRUE;
}

#include "face_palette_lifetime.inc"

namespace {
void destroyObjects() {
    for (auto& item : objects) {
        std::unordered_set<UINT16*> released;
        for (auto*& table : item.second->object.pShades) {
            if (table && released.insert(table).second) {
                require(ownedPalettes.erase(table) == 1, "palette is released exactly once");
                delete[] table;
                ++frees;
            }
            table = nullptr;
        }
        delete item.second;
    }
    objects.clear();
}
}

int main() {
    try {
        zNewFaceGear[1].Type = zNewFaceGearIMP[1].Type = 1;
        std::strcpy(zNewFaceGear[1].szFile, "gear");
        std::strcpy(zNewFaceGearIMP[1].szFile, "unused");
        require(InitializeFaceGearGraphics(), "face-gear graphics initialize");
        require(objects.size() == 2, "normal and IMP gear each own one video object");
        require(allocationAttempts == 12 && allocations == 12,
                "initialization creates six palettes per fresh gear object");

        HVOBJECT normal = nullptr, imp = nullptr;
        require(GetVideoObject(&normal, zNewFaceGear[1].uiIndex) &&
                GetVideoObject(&imp, zNewFaceGearIMP[1].uiIndex),
                "both initialized gear objects remain registered");
        std::array<UINT16*, 6> normalTables{}, impTables{};
        for (unsigned shade = 0; shade < 6; ++shade) {
            normalTables[shade] = normal->pShades[shade];
            impTables[shade] = imp->pShades[shade];
            require(normalTables[shade] && impTables[shade],
                    "every portrait shade is initialized");
        }

        FACETYPE first{20}, second{24};
        for (unsigned pass = 0; pass < 64; ++pass) {
            for (unsigned shade = 0; shade < 6; ++shade) {
                INT16 x = -1, y = -1;
                GetXYForRightIconPlacement_FaceGear(
                    pass & 1 ? &first : &second, 0, 100, 200, &x, &y, 2,
                    zNewFaceGear[1].uiIndex, shade);
                require(x == 118 && y == (pass & 1 ? 213 : 217),
                        "placement retains the original geometry");
                require(normal->pShadeCurrent == normalTables[shade],
                        "redraw selects the initialized normal gear shade");
                require(normal->pShades[shade] == normalTables[shade],
                        "normal gear table identity survives redraw");
            }
        }
        for (unsigned shade = 0; shade < 6; ++shade) {
            INT16 x = 0, y = 0;
            GetXYForRightIconPlacement_FaceGear(
                &first, 0, 5, 6, &x, &y, 1, zNewFaceGearIMP[1].uiIndex, shade);
            require(imp->pShadeCurrent == impTables[shade] &&
                    imp->pShades[shade] == impTables[shade],
                    "IMP gear retains and selects its initialized shade");
        }
        require(allocationAttempts == 12 && allocations == 12,
                "repeated placement performs no palette allocation");

        destroyObjects();
        require(ownedPalettes.empty() && frees == 12,
                "manager-style destruction releases every owned table");
        std::cout << "PASS " << checks << " face palette lifetime checks\n";
        return 0;
    } catch (const std::exception& error) {
        for (auto* table : ownedPalettes) delete[] table;
        for (auto& item : objects) delete item.second;
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
