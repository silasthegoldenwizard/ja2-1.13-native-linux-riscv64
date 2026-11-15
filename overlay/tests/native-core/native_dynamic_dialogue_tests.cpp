/*
 * P710 native snapshot update: tests/native-core/native_dynamic_dialogue_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Production dialogue ownership with authored state; no game resources or display.
#include "DynamicDialogueWidget.h"
#include "GameSettings.h"
#include "Timer Control.h"
#include "renderworld.h"
#include "screenids.h"
#include <cstdlib>
#include <iostream>

extern DDBox* gDDBoxList[10];
extern UINT32 guiCurrentScreen;

namespace {
unsigned checks = 0;
void expect(bool ok, const char* why) {
    ++checks;
    if (!ok) { std::cerr << "FAIL: " << why << '\n'; std::abort(); }
}
unsigned count() {
    unsigned result = 0;
    for (const auto* box : gDDBoxList) if (box) ++result;
    return result;
}
DDBox* create(UINT8 id) {
    auto* box = GetDDBox(id);
    expect(box && box->GetID() == id, "registry creates requested ID");
    DynamicOpinionSpeechEvent event{};
    box->SetEvent(event);
    wchar_t text[] = L"";
    box->SetText(text);
    box->Create(10, 20);
    expect(box->IsInit() && box->IsDisplayed(), "production Create initializes box");
    return box;
}
}

int main() {
    guiBaseJA2Clock = 1000;
    expect(InitMyBoxes() && count() == 0, "empty registry initialization");
    expect(!RemoveDDBox(255), "missing removal is harmless");
    auto* unused = GetDDBox(4);
    expect(unused && !unused->IsInit(), "new box need not yet be initialized");
    expect(RemoveDDBox(4) && count() == 0, "C++ allocation has matching registry deletion");
    expect(!RemoveDDBox(4), "repeated removal is harmless");

    auto* box = create(1);
    expect(GetDDBox(1) == box && count() == 1, "same ID preserves identity");
    box->Create(30, 40);
    expect(GetDDBox(1) == box && box->GetX() == 30 && box->GetY() == 40,
           "recreation retains the live object");
    box->Destroy();
    expect(GetDDBox(1) == box && !box->IsInit() && !box->IsDisplayed(),
           "visual cleanup leaves registry ownership intact");
    box->Destroy();
    box->Create(50, 60);
    expect(box->IsInit() && count() == 1, "cleaned box can be recreated");
    box->SetEndTime(999);
    box->Refresh();
    expect(count() == 0 && !RemoveDDBox(1), "expiration removes registry ownership once");

    box = create(2);
    box->SetStartTime(2000); box->SetEndTime(3000);
    box->Refresh();
    expect(count() == 1 && box->IsDisplayed(), "future box is retained without painting");
    DelayBoxDestructionBy(100);
    expect(box->GetStartTime() == 2100 && box->GetEndTime() == 3100,
           "hidden-screen timing retains original delay");
    guiBaseJA2Clock = 4000;
    guiCurrentScreen = GAME_SCREEN;
    gGameExternalOptions.fDynamicDialogue = TRUE;
    gfScrollPending = gfScrollInertia = FALSE;
    RefreshBoxes();
    expect(count() == 0, "actual frame refresh retires expired box safely");

    for (UINT8 id = 10; id < 20; ++id)
        expect(GetDDBox(id) != nullptr, "all ten slots are usable");
    expect(count() == 10, "registry reaches fixed capacity");
    for (unsigned n = 0; n < 16; ++n)
        expect(GetDDBox(200) == nullptr && count() == 10, "full registry rejects without allocation leak");
    expect(GetDDBox(14) != nullptr && count() == 10, "existing ID works at capacity");
    expect(RemoveDDBox(14) && GetDDBox(200) && count() == 10, "freed slot is reusable");
    DestroyAllDynamicDialogueBoxes();
    expect(count() == 0, "bulk cleanup deletes uninitialized boxes");
    create(3); create(4);
    DestroyAllDynamicDialogueBoxes();
    expect(count() == 0, "bulk cleanup deletes displayed boxes once");
    DestroyAllDynamicDialogueBoxes();
    expect(count() == 0, "bulk cleanup is repeatable");
    create(5);
    expect(InitMyBoxes() && count() == 0, "reinitialization releases existing ownership");
    DestroyAllDynamicDialogueBoxes();
    std::cout << checks << " original dynamic-dialogue lifecycle assertions passed\n";
}
