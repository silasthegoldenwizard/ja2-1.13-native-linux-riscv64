/*
 * P710 native snapshot update: tests/native-core/native_clipboard_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "types.h"
#include "native_text.h"
#include <SDL3/SDL.h>
#include <cstring>
#include <cwchar>
#include <iostream>
#include <stdexcept>

extern STR16 szClipboard;
void InitClipboard();
void KillClipboard();
UINT32 PasteClipboardText();
void CopyToClipboard();
namespace {
unsigned checks=0;
void check(bool value, const char* why) {
    ++checks;
    if (!value) throw std::runtime_error(why);
}
}
int main()
{
    try {
        check(SDL_Init(SDL_INIT_VIDEO), "initialize private software display");
        InitClipboard();
        check(SDL_SetClipboardText("A\xc3\xa9\xf0\x9f\x99\x82"), "publish authored Unicode clipboard");
        check(PasteClipboardText()==3 && szClipboard &&
              std::wcscmp(szClipboard,L"A\u00e9\U0001f642")==0,
              "actual original paste publishes three native Unicode code points");
        char* text=SDL_GetClipboardText();
        check(text && !*text,"paste consumes external clipboard"); SDL_free(text);
        CopyToClipboard();
        text=SDL_GetClipboardText();
        check(text && !*text,"original external availability precondition retained"); SDL_free(text);
        check(SDL_SetClipboardText("seed"),"prepare existing external clipboard");
        CopyToClipboard();
        text=SDL_GetClipboardText();
        check(text && std::strcmp(text,"A\xc3\xa9\xf0\x9f\x99\x82")==0,
              "actual original copy publishes UTF8 without changing owned native text"); SDL_free(text);
        for (int i=0;i<64;++i) {
            check(SDL_SetClipboardText("replacement"),"publish replacement");
            check(PasteClipboardText()==11 && std::wcscmp(szClipboard,L"replacement")==0,
                  "repeated paste replaces actual owned allocation");
        }
        check(PasteClipboardText()==0 && szClipboard &&
              std::wcscmp(szClipboard,L"replacement")==0,
              "empty external clipboard preserves existing internal text");
        KillClipboard();
        check(szClipboard==nullptr,"actual kill releases clipboard ownership");
        KillClipboard();
        check(SDL_SetClipboardText("seed"),"prepare no-internal-text copy");
        CopyToClipboard();
        text=SDL_GetClipboardText();
        check(text && std::strcmp(text,"seed")==0,"null internal clipboard leaves external unchanged");
        SDL_free(text);
        SDL_SetClipboardText("");
        SDL_Quit();
        std::cout << checks << " original native clipboard checks passed; authored text on private X11 only\n";
        return 0;
    } catch (const std::exception& e) {
        KillClipboard(); SDL_Quit(); std::cerr << e.what() << '\n'; return 1;
    }
}
