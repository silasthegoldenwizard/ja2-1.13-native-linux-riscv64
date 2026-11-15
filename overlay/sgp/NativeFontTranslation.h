/*
 * P710 native snapshot update: sgp/NativeFontTranslation.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef P710_NATIVE_FONT_TRANSLATION_H
#define P710_NATIVE_FONT_TRANSLATION_H
#include "Font.h"
FontTranslationTable* CreateEnglishTransTable();
void DestroyNativeFontTranslationTable(FontTranslationTable*) noexcept;
#endif
