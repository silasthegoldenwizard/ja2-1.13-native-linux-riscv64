/*
 * P710 native snapshot update: tests/native-core/native_battle_sound_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Actual NPC battle-sound cache with authored actors and an empty VFS.
#include "Soldier Control.h"
#include "Animation Data.h"
#include "Soldier Profile.h"
#include "Timer Control.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Tools/vfs_log.h>
#include <cstdlib>
#include <iostream>
#include <memory>

extern UINT32 numBattleSounds_Npc[3][12][NUM_MERC_BATTLE_SOUNDS];
extern bool BattleSoundSearchDone_Npc[3][12][NUM_MERC_BATTLE_SOUNDS];
namespace {
unsigned checks = 0;
void expect(bool ok, const char* why) {
    ++checks;
    if (!ok) { std::cerr << "FAIL: " << why << '\n'; std::abort(); }
}
}
int main() {
    guiBaseJA2Clock = 1000;
    auto actor = std::make_unique<SOLDIERTYPE>();
    actor->ubProfile = NO_PROFILE;
    actor->bTeam = ENEMY_TEAM;
    actor->ubBodyType = REGMALE;
    // Soldier Create assigns ordinary human sets 6 + Random(6), including 9.
    // This first call reproduces the actual Alt+O incapacitation diagnostic.
    actor->ubBattleSoundID = 9;
    expect(!actor->InternalDoMercBattleSound(BATTLE_SOUND_DIE1, 0), "absent authored death sample is silent");
    for (unsigned category = 0; category < 2; ++category) {
        actor->ubBodyType = category ? KIDCIV : REGMALE;
        for (unsigned voice = 0; voice < 12; ++voice) {
            actor->ubBattleSoundID = voice;
            for (UINT8 sound : {UINT8(BATTLE_SOUND_DIE1), UINT8(BATTLE_SOUND_HIT1)}) {
                actor->bOldBattleSnd = -1; // Exercise search, rather than repeat suppression.
                expect(!actor->InternalDoMercBattleSound(sound, 0), "missing sample preserves silent fallback");
                expect(BattleSoundSearchDone_Npc[category][voice][sound], "each valid voice owns a completed search slot");
                expect(numBattleSounds_Npc[category][voice][sound] == 1, "missing variants retain original fallback count");
            }
        }
    }
    for (UINT8 invalid : {UINT8(12), UINT8(255)}) {
        actor->ubBattleSoundID = invalid;
        actor->bOldBattleSnd = -1;
        expect(!actor->InternalDoMercBattleSound(BATTLE_SOUND_DIE1, 0), "out-of-domain saved voice is rejected");
    }
    fileman::CloseAll(); vfs::Log::flushDeleteAll();
    vfs::CVirtualFileSystem::shutdownVFS(); vfs::ObjectAllocator::clear();
    std::cout << checks << " original NPC battle-sound assertions passed\n";
}
