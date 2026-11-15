/*
 * P710 native snapshot update: tests/native-core/native_button_image_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native adaptation tests, 2026-11-12. Reuses authored BUTTON_FIXTURES.md data.
// Tests shared quick-image ownership, not game assets or full menu startup.
#include "Button System.h"
#include "vsurface.h"
#include "clip_state.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
unsigned checks = 0;
void check(bool condition, const char* why) {
    ++checks;
    if (!condition) throw std::runtime_error(why);
}
UINT16 pic(INT32 id) { return static_cast<UINT16>(id); }
std::size_t images() {
    return static_cast<std::size_t>(std::count_if(ButtonPictures.begin(), ButtonPictures.end(),
        [](const BUTTON_PICS& image) { return image.vobj != nullptr; }));
}
void fresh() {
    ShutdownButtonSystem();
    MSYS_Shutdown();
    MSYS_Init();
    check(InitButtonSystem(), "button init");
}
INT32 load() {
    const INT32 id = LoadButtonImage("buttons.sti", 4, 0, 1, 2, 3);
    check(id >= 0, "authored five-frame button load");
    return id;
}
INT32 button(INT32 image, INT16 x) {
    const INT32 id = QuickCreateButton(static_cast<UINT32>(image), x, 2, 0, 31, nullptr, nullptr);
    check(id >= 0, "shared-image quick button");
    return id;
}
UINT16 pixel(UINT32 surface, unsigned x, unsigned y) {
    UINT32 pitch = 0;
    auto* buffer = LockVideoSurface(surface, &pitch);
    check(buffer != nullptr, "surface readback lock");
    UINT16 value = 0;
    std::memcpy(&value, buffer + y * pitch + x * sizeof(UINT16), sizeof(value));
    UnLockVideoSurface(surface);
    return value;
}
void sharingAndUnloadOrder() {
    fresh();
    const INT32 original = load();
    const INT32 first = UseLoadedButtonImage(original, 4, 2, 3, 0, 1);
    const INT32 second = UseLoadedButtonImage(original, -1, 1, -1, 3, -1);
    const INT32 chained = UseLoadedButtonImage(first, -1, 3, -1, 2, -1);
    check(first >= 0 && second >= 0 && chained >= 0 && images() == 4, "independent aliases including alias of alias");
    const HVOBJECT shared = ButtonPictures[static_cast<std::size_t>(original)].vobj;
    for (INT32 id : {first, second, chained})
        check(ButtonPictures[static_cast<std::size_t>(id)].vobj == shared, "aliases share the actual video object");
    check(ButtonPictures[static_cast<std::size_t>(original)].OffNormal == 0 &&
          ButtonPictures[static_cast<std::size_t>(first)].OffNormal == 2 &&
          ButtonPictures[static_cast<std::size_t>(second)].OffNormal == 1 &&
          ButtonPictures[static_cast<std::size_t>(chained)].OffNormal == 3, "state selections are independent");

    VSURFACE_DESC desc{};
    desc.fCreateFlags = VSURFACE_CREATE_DEFAULT;
    desc.usWidth = 40; desc.usHeight = 12; desc.ubBitDepth = 16;
    UINT32 surface = 0;
    check(AddVideoSurface(&desc, &surface) && SetButtonDestBuffer(surface), "offscreen destination");
    SGPRect clip{0, 0, 40, 12}; SetClippingRect(&clip);
    check(ColorFillVideoSurfaceArea(surface, 0, 0, 40, 12, 0x2104), "clear offscreen destination");
    const INT32 a = button(first, 2), b = button(second, 14), c = button(chained, 26);
    UnloadButtonImage(original);
    check(!ButtonPictures[static_cast<std::size_t>(original)].vobj && images() == 3,
          "original unload retains shared resource for live alias buttons");
    check(UseLoadedButtonImage(original, -1, 0, -1, -1, -1) == -1 &&
          GetWidthOfButtonPic(pic(original), 0) == 0, "unloaded original ID rejected before slot reuse");
    check(DrawButton(a) && DrawButton(b) && DrawButton(c), "all aliases still render after original unload");
    check(pixel(surface, 4, 4) == 0xf800 && pixel(surface, 16, 4) == 0x07e0 &&
          pixel(surface, 28, 4) == 0xffe0, "distinct authored frame colors prove independent state rendering");

    UnloadButtonImage(first);
    check(ButtonPictures[static_cast<std::size_t>(first)].vobj == shared, "live owner alias unload rejected");
    UnloadButtonImage(second);
    check(ButtonPictures[static_cast<std::size_t>(second)].vobj == shared, "live nonowner alias unload rejected");
    RemoveButton(b); UnloadButtonImage(second);
    check(!ButtonPictures[static_cast<std::size_t>(second)].vobj && DrawButton(a), "unused alias can unload before owner");
    RemoveButton(a); UnloadButtonImage(first);
    check(!ButtonPictures[static_cast<std::size_t>(first)].vobj && DrawButton(c), "ownership transfers again to last live alias");
    check(GetWidthOfButtonPic(pic(chained), 4) == 8 && GetHeightOfButtonPic(pic(chained), 4) == 6,
          "dimension queries use raw frame even when absent from alias state set");
    RemoveButton(c); UnloadButtonImage(chained); UnloadButtonImage(chained);
    check(images() == 0 && GetHeightOfButtonPic(pic(chained), 0) == 0, "last alias unload and repeated unload are safe");
    const INT32 retained = load();
    const INT32 earlyAlias = UseLoadedButtonImage(retained, -1, 1, -1, -1, -1);
    check(earlyAlias >= 0, "alias for opposite unload order");
    UnloadButtonImage(earlyAlias);
    check(GetWidthOfButtonPic(pic(retained), 0) == 8 && images() == 1, "alias unload before original retains owner resource");
    UnloadButtonImage(retained);
    check(DeleteVideoSurfaceFromIndex(surface), "offscreen destination cleanup");
}
void invalidSelectionsAndCapacity() {
    fresh();
    const INT32 original = load();
    const HVOBJECT object = ButtonPictures[static_cast<std::size_t>(original)].vobj;
    for (INT32 invalid : {-1, MAX_BUTTON_PICS, std::numeric_limits<INT32>::min(), std::numeric_limits<INT32>::max()}) {
        check(UseLoadedButtonImage(invalid, -1, 0, -1, -1, -1) == -1, "invalid source ID rejected");
        UnloadButtonImage(invalid);
    }
    for (INT32 invalid : {-2, 5, std::numeric_limits<INT32>::min(), std::numeric_limits<INT32>::max()}) {
        for (unsigned state = 0; state < 5; ++state) {
            INT32 selected[] = {4, 0, 1, 2, 3}; selected[state] = invalid;
            check(UseLoadedButtonImage(original, selected[0], selected[1], selected[2], selected[3], selected[4]) == -1,
                  "every invalid state index rejected without installation");
        }
        check(GetWidthOfButtonPic(pic(original), invalid) == 0 && GetHeightOfButtonPic(pic(original), invalid) == 0,
              "invalid raw frame dimension queries return zero");
    }
    check(UseLoadedButtonImage(original, -1, -1, -1, -1, -1) == -1 && images() == 1,
          "empty state selection rolls back without consuming image slots");
    check(GetWidthOfButtonPic(pic(original), -1) == 0 && GetHeightOfButtonPic(pic(original), -1) == 0,
          "absent-state sentinel is not a valid raw frame for dimension queries");
    check(ButtonPictures[static_cast<std::size_t>(original)].vobj == object &&
          ButtonPictures[static_cast<std::size_t>(original)].OffNormal == 0 &&
          ButtonPictures[static_cast<std::size_t>(original)].OnHilite == 3, "invalid selections preserve source");
    for (UINT16 invalid : {static_cast<UINT16>(MAX_BUTTON_PICS), std::numeric_limits<UINT16>::max()})
        check(GetWidthOfButtonPic(invalid, 0) == 0 && GetHeightOfButtonPic(invalid, 0) == 0, "invalid dimension image ID");
    std::vector<INT32> aliases;
    for (int i = 1; i < MAX_BUTTON_PICS; ++i) {
        const INT32 alias = UseLoadedButtonImage(original, -1, i % 5, -1, -1, -1);
        check(alias == i, "lowest image slot reused with original integer-ID semantics");
        aliases.push_back(alias);
    }
    check(UseLoadedButtonImage(original, -1, 0, -1, -1, -1) == -1 && images() == MAX_BUTTON_PICS,
          "full image table failure preserves all installed slots");
    UnloadButtonImage(original);
    for (INT32 alias : aliases) {
        check(GetWidthOfButtonPic(pic(alias), 0) == 8, "shared resource survives repeated ownership transfers");
        UnloadButtonImage(alias);
    }
    check(images() == 0, "full table cleanup leaves no images");
    const INT32 reused = load();
    check(reused == original, "public image IDs reuse freed slots; no generation-handle promise");
    const INT32 live = button(reused, 2);
    (void)live;
    check(UseLoadedButtonImage(reused, 4, 2, 3, 0, 1) >= 0, "alias for shutdown cleanup");
    ShutdownButtonSystem(); ShutdownButtonSystem();
    check(ButtonPictures.empty() && ButtonList.empty(), "shutdown cleans live buttons and shared ownership exactly once");
}
void borrowedAndRawDimensions() {
    fresh();
    VOBJECT_DESC desc{}; desc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
    std::strcpy(desc.ImageFile, "buttons.sti");
    HVOBJECT object = CreateVideoObject(&desc);
    check(object != nullptr, "caller-owned authored image");
    // Only change offsets, before borrowing. Raw frame dimensions stay 8x6;
    // button extents include offsets and therefore become 10x9.
    object->pETRLEObject[0].sOffsetX = 2;
    object->pETRLEObject[0].sOffsetY = 3;
    const INT32 borrowed = UseVObjAsButtonImage(object, -1, 0, -1, -1, -1);
    check(borrowed >= 0 && GetWidthOfButtonPic(pic(borrowed), 0) == 8 &&
          GetHeightOfButtonPic(pic(borrowed), 0) == 6, "raw dimensions exclude offsets");
    check(ButtonPictures[static_cast<std::size_t>(borrowed)].MaxWidth == 10 &&
          ButtonPictures[static_cast<std::size_t>(borrowed)].MaxHeight == 9, "button extents retain offsets");
    check(UseLoadedButtonImage(borrowed, -1, 0, -1, -1, -1) == -1 && images() == 1,
          "original external-object alias prohibition retained");
    UnloadButtonImage(borrowed);
    check(UseVObjAsButtonImage(object, 4, 0, 1, 2, 3) >= 0, "borrowed unload leaves object alive");
    ShutdownButtonSystem();
    check(object->usNumberOfObjects == 5 && DeleteVideoObject(object), "borrowed object stays caller-owned after shutdown");
}
void cleanup() {
    ShutdownButtonSystem(); MSYS_Shutdown();
    ShutdownVideoSurfaceManager(); ShutdownVideoObjectManager(); fileman::CloseAll();
    vfs::Log::flushDeleteAll(); vfs::CVirtualFileSystem::shutdownVFS(); vfs::ObjectAllocator::clear();
}
}
int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("expected existing synthetic button fixture root");
        const auto root = std::filesystem::canonical(argv[1]);
        auto directory = std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(), vfs::Path(root.u8string()));
        check(directory->init(), "fixture directory");
        auto profile = std::make_unique<vfs::CVirtualProfile>("button-images", vfs::Path(root.u8string()), false);
        profile->addLocation(directory.get());
        auto* location = directory.release(); auto* mounted = profile.get();
        getVFS()->getProfileStack()->pushProfile(profile.release());
        check(getVFS()->addLocation(location, mounted), "mount existing authored fixtures");
        check(InitializeVideoObjectManager() && InitializeVideoSurfaceManager(), "resource managers");
        sharingAndUnloadOrder(); invalidSelectionsAndCapacity(); borrowedAndRawDimensions(); cleanup();
        std::cout << "PASS: " << checks << " shared button-image assertions; authored fixtures only\n";
        return 0;
    } catch (const std::exception& error) {
        cleanup(); std::cerr << "FAIL after " << checks << " assertions: " << error.what() << '\n'; return 1;
    }
}
