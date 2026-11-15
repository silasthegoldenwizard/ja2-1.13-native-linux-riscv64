// Completion-only regression. No game data is required.
// AttachObject below supplies synthetic outcomes, not game attachment rules.
// The immutable includes retain production statements and source line records.
// Verify the supplied full-file hashes and extraction records with the runner.
#include <types.h>
#include <cstring>
#include <Item Types.h>
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <tuple>
#include <vector>

#include "/contract/production/object_methods.inc"

namespace {
void check(bool value)
{
    if (!value) {
        std::fputs("FAIL attachment completion fixture\n", stderr);
        std::abort();
    }
}

// These values select isolated branches. They are not game UI definitions.
constexpr UINT32 INTERFACE_MAPSCREEN = 1;
constexpr UINT32 INTERFACE_SHOPKEEP_INTERFACE = 2;
constexpr UINT32 TEST_TACTICAL_SCREEN = 11;
constexpr UINT32 EXTERN_CURSOR = 20;
constexpr UINT32 CURSOR_NORMAL = 21;
constexpr UINT8 DIRTYLEVEL2 = 2;
struct Region {};
Region gSMPanelRegion, gMPanelRegion;
OBJECTTYPE* gpItemPointer = nullptr;
OBJECTTYPE* gpItemDescObject = nullptr;
SOLDIERTYPE* gpItemDescSoldier = nullptr;
UINT32 guiTacticalInterfaceFlags = 0;
UINT32 guiCurrentItemDescriptionScreen = TEST_TACTICAL_SCREEN;
UINT32 guiExternVo = 0;
UINT16 gusExternVoSubIndex = 0;
BOOLEAN fMapInventoryItem = FALSE;
BOOLEAN fTeamPanelDirty = FALSE;
UINT8 fInterfacePanelDirty = 0;
BOOLEAN gfReEvaluateEveryonesNothingToDo = FALSE;

enum Event {
    Attach, MapEnd, Buttons, TacticalRegion, DatabaseCursor, MovingClear,
    SkiCursor, Reload, Graphic, MapRegion, CurrentCursor, Hatches,
    StatCoords, Tooltips, TooltipRegion
};
std::vector<Event> events;
void observe(Event event) { events.push_back(event); }
struct MovingItemDouble {
    void initialize() { observe(MovingClear); }
} gMoveingItem;

enum class Outcome { TransferOne, Fail };
Outcome outcome = Outcome::Fail;
OBJECTTYPE* expectedSource = nullptr;
OBJECTTYPE* expectedTarget = nullptr;

void MAPEndItemPointer()
{
    check(gpItemPointer == expectedSource);
    check(!gpItemPointer->exists());
    observe(MapEnd);
    gpItemPointer = nullptr;
}
void EnableSMPanelButtons(BOOLEAN first, BOOLEAN second)
{
    check(first == TRUE && second == TRUE);
    observe(Buttons);
}
void MSYS_ChangeRegionCursor(Region* region, UINT32 cursor)
{
    if (region == &gSMPanelRegion) {
        check(cursor == CURSOR_NORMAL);
        observe(TacticalRegion);
    } else {
        check(region == &gMPanelRegion && cursor == EXTERN_CURSOR);
        observe(MapRegion);
    }
}
void SetCurrentCursorFromDatabase(UINT32 cursor)
{
    check(cursor == CURSOR_NORMAL);
    observe(DatabaseCursor);
}
void SetSkiCursor(UINT32 cursor)
{
    check(cursor == CURSOR_NORMAL);
    observe(SkiCursor);
}
void DeleteItemDescriptionBox()
{
    // These fixtures retain their target. Closing it is not an expected path.
    check(false);
}
void ReloadItemDesc() { observe(Reload); }
UINT32 GetInterfaceGraphicForItem(INVTYPE* item)
{
    check(gpItemPointer != nullptr);
    check(item == &Item[gpItemPointer->usItem]);
    observe(Graphic);
    return 1234;
}
void MSYS_SetCurrentCursor(UINT32 cursor)
{
    check(cursor == EXTERN_CURSOR);
    observe(CurrentCursor);
}
void UpdateItemHatches() { observe(Hatches); }
void InitDescStatCoords(OBJECTTYPE* object)
{
    check(object == expectedTarget);
    observe(StatCoords);
}
void UpdateAttachmentTooltips(OBJECTTYPE* object, UINT32 index)
{
    check(object == expectedTarget && index == 1);
    observe(Tooltips);
}
void InternalInitEDBTooltipRegion(OBJECTTYPE* object, UINT32 screen)
{
    check(object == expectedTarget);
    check(screen == guiCurrentItemDescriptionScreen);
    observe(TooltipRegion);
}
} // namespace

INVTYPE Item[MAXITEMS] = {};
std::list<LBENODE> LBEArray;

// Synthetic member outcome: transfer exactly one authored node, or do nothing.
// Splice retains node identity, metadata, and nested attachments. No real
// attachment compatibility, merge, AP, or equipment policy is executed.
BOOLEAN OBJECTTYPE::AttachObject(SOLDIERTYPE* soldier, OBJECTTYPE* source,
    BOOLEAN sound, UINT8 subObject, INT32 position, BOOLEAN removeProhibited,
    std::vector<UINT16> slots)
{
    check(this == expectedTarget && source == expectedSource);
    check(soldier == nullptr && sound == TRUE);
    check(subObject == 1 && position == 1);
    check(removeProhibited == TRUE && slots.empty());
    observe(Attach);
    if (outcome == Outcome::Fail)
        return FALSE;

    check(source->exists());
    check(source->objectStack.size() == source->ubNumberOfObjects);
    auto selected = objectStack.begin();
    ++selected;
    auto& attachments = selected->attachments;
    attachments.emplace_back();
    OBJECTTYPE& received = attachments.back();
    received.usItem = source->usItem;
    received.ubMission = source->ubMission;
    received.fFlags = source->fFlags;
    received.ubNumberOfObjects = 1;
    received.objectStack.clear();
    received.objectStack.splice(received.objectStack.end(),
        source->objectStack, source->objectStack.begin());
    --source->ubNumberOfObjects;
    if (source->ubNumberOfObjects == 0)
        source->initialize();
    return TRUE;
}

#include "/contract/production/do_attachment.inc"

static void complete()
{
    UINT32 ubStatusIndex = 1;
    UINT32 uiItemPos = 1;
#ifdef BUILD_BEFORE
#include "/contract/production/completion_before.inc"
#else
#include "/contract/production/completion_fixed.inc"
#endif
}

namespace {
// Capture all ObjectData bytes. Its real initializer clears these bytes.
// Include each node address and object header to detect loss or duplication.
using DataBytes = std::array<unsigned char, sizeof(ObjectData)>;
using Record = std::tuple<std::uintptr_t, UINT16, UINT8, UINT8, DataBytes>;
using Inventory = std::vector<Record>;

void capture(const OBJECTTYPE& object, Inventory& inventory)
{
    if (!object.exists()) {
        check(object.ubNumberOfObjects == 0);
        check(object.objectStack.size() == 1);
        check(object.objectStack.front().attachments.empty());
        return;
    }
    check(object.objectStack.size() == object.ubNumberOfObjects);
    for (const auto& node : object.objectStack) {
        DataBytes bytes{};
        std::memcpy(bytes.data(), &node.data, bytes.size());
        inventory.emplace_back(reinterpret_cast<std::uintptr_t>(&node),
            object.usItem, object.ubMission, object.fFlags, bytes);
        for (const auto& attachment : node.attachments)
            capture(attachment, inventory);
    }
}
Inventory capture(const OBJECTTYPE& target, const OBJECTTYPE& source)
{
    Inventory inventory;
    capture(target, inventory);
    capture(source, inventory);
    std::sort(inventory.begin(), inventory.end());
    return inventory;
}

void author(OBJECTTYPE& object, UINT16 item, UINT8 count, UINT8 seed)
{
    object.initialize();
    if (count == 0)
        return;
    object.usItem = item;
    object.ubNumberOfObjects = count;
    object.ubMission = seed;
    object.fFlags = OBJECT_MODIFIED;
    object.objectStack.resize(count);
    UINT8 index = 0;
    for (auto& node : object.objectStack) {
        auto& data = node.data;
        data.objectStatus = static_cast<INT16>(61 + index);
        data.bTrap = static_cast<INT8>(index + 1);
        data.fUsed = seed;
        data.ubImprintID = static_cast<UINT8>(seed + index);
        data.bTemperature = 12.5f + index;
        data.ubDirection = index;
        data.ubWireNetworkFlag = 100 + seed + index;
        data.bDefuseFrequency = static_cast<INT8>(index + 2);
        data.sRepairThreshold = static_cast<INT16>(80 + index);
        data.bFiller = 3.5f;
        data.sObjectFlag = static_cast<UINT64>(seed) << 32;
        ++index;
    }
}

unsigned completions = 0;
void run(UINT32 flags, UINT8 count, Outcome selectedOutcome, unsigned repeats)
{
    OBJECTTYPE target;
    OBJECTTYPE source;
    author(target, 100, 2, 4);
    author(source, 101, count, 8);
    auto targetNode = target.objectStack.begin();
    ++targetNode;
    targetNode->attachments.emplace_back();
    author(targetNode->attachments.back(), 102, 1, 12);
    if (count) {
        source.objectStack.front().attachments.emplace_back();
        author(source.objectStack.front().attachments.back(), 103, 1, 16);
    }
    const Inventory original = capture(target, source);
    const auto* firstTargetNode = &target.objectStack.front();
    const auto* selectedTargetNode = &*targetNode;
    expectedSource = &source;
    expectedTarget = &target;
    gpItemPointer = &source;
    gpItemDescObject = &target;
    outcome = selectedOutcome;
    guiTacticalInterfaceFlags = flags;
    const bool map = (flags & INTERFACE_MAPSCREEN) != 0;
    const bool shop = (flags & INTERFACE_SHOPKEEP_INTERFACE) != 0;
    guiCurrentItemDescriptionScreen = map ? MAP_SCREEN : TEST_TACTICAL_SCREEN;
    Item[101].ubGraphicNum = 77;

    for (unsigned iteration = 0; iteration < repeats; ++iteration) {
        check(gpItemPointer == &source);
        const UINT8 priorCount = source.ubNumberOfObjects;
        const bool success = selectedOutcome == Outcome::TransferOne;
        check(!success || priorCount > 0);
        const UINT8 remaining = static_cast<UINT8>(priorCount - (success ? 1 : 0));
        const bool cleared = success && remaining == 0;
        const bool refresh = remaining != 0;
        events.clear();
        fMapInventoryItem = FALSE;
        fTeamPanelDirty = FALSE;
        fInterfacePanelDirty = 0;
        gfReEvaluateEveryonesNothingToDo = FALSE;
        guiExternVo = 900;
        gusExternVoSubIndex = 901;

        // BUILD_BEFORE reaches the real null member call here on the first
        // fixture. Do not catch it or replace it with a test-side check.
        complete();

        std::vector<Event> expected{Attach};
        if (cleared) {
            if (map) {
                expected.push_back(MapEnd);
            } else {
                expected.push_back(Buttons);
                expected.push_back(TacticalRegion);
                expected.push_back(DatabaseCursor);
                if (shop) {
                    expected.push_back(MovingClear);
                    expected.push_back(SkiCursor);
                }
            }
        }
        if (success)
            expected.push_back(Reload);
        if (refresh) {
            if (map) {
                expected.push_back(Graphic);
                expected.push_back(MapRegion);
                expected.push_back(CurrentCursor);
            }
            expected.push_back(Hatches);
        }
        expected.push_back(StatCoords);
        expected.push_back(Tooltips);
        expected.push_back(TooltipRegion);
        check(events == expected);
        check(gpItemPointer == (cleared ? nullptr : &source));
        check(source.ubNumberOfObjects == remaining);
        check(gpItemDescObject == &target && target.exists());
        check(&target.objectStack.front() == firstTargetNode);
        check(&*targetNode == selectedTargetNode);
        check(target.objectStack.front().attachments.empty());
        check(targetNode->attachments.size() ==
            1 + (success ? iteration + 1 : 0));
        check(fInterfacePanelDirty == ((success || refresh) ? DIRTYLEVEL2 : 0));
        check(gfReEvaluateEveryonesNothingToDo == TRUE);
        check(fMapInventoryItem == ((map && refresh) ? TRUE : FALSE));
        check(fTeamPanelDirty == ((map && refresh) ? TRUE : FALSE));
        check(guiExternVo == ((map && refresh) ? 1234u : 900u));
        check(gusExternVoSubIndex == ((map && refresh) ? 77 : 901));
        check(capture(target, source) == original);
        ++completions;
    }
    gpItemPointer = nullptr;
    gpItemDescObject = nullptr;
    expectedSource = nullptr;
    expectedTarget = nullptr;
}
} // namespace

int main()
{
    // Keep this consumed tactical fixture first for the pre-fix UBSan control.
    run(0, 1, Outcome::TransferOne, 1);
    run(0, 3, Outcome::TransferOne, 3);
    run(0, 0, Outcome::Fail, 2);
    run(0, 2, Outcome::Fail, 2);
    for (UINT32 flags : {INTERFACE_MAPSCREEN, INTERFACE_SHOPKEEP_INTERFACE}) {
        run(flags, 1, Outcome::TransferOne, 1);
        run(flags, 3, Outcome::TransferOne, 3);
        run(flags, 0, Outcome::Fail, 2);
        run(flags, 2, Outcome::Fail, 2);
    }
    check(completions == 24);
    std::printf("PASS %u completion-only/outcome-double coverage\n", completions);
    return 0;
}
