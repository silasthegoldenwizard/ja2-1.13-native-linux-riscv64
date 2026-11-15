/*
 * P710 native notice preparation: sgp/SDLPresentation.h
 * New project path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: declare SDL3 presentation ownership boundary.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: sgp/SDLPresentation.h
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#ifndef SGP_SDL_PRESENTATION_H
#define SGP_SDL_PRESENTATION_H

#include <SDL3/SDL.h>
#include <cstddef>
#include "native_presentation.h"

namespace sgp
{
enum class NativeDisplayTarget { Primary, Upper };
struct NativePresentationOptions {
    bool borderless=false;
    NativeDisplayTarget display=NativeDisplayTarget::Primary;
};
// Resolve again at open: SDL display IDs can change across video initialization.
bool QueryNativeDisplayBounds(NativeDisplayTarget, SDL_Rect&);
// Optional borrowed main-thread sink. nullptr event means the window is closing;
// the registration is already removed before that final notification.
using PresentationEventSink = void (*)(void*, const SDL_Event*);
// Native software-presentation boundary for the engine's existing RGB565 pixels.
// This owns its window/video reference, and never takes ownership of frame data.
class SDLPresentation
{
public:
    SDLPresentation() = default;
    ~SDLPresentation();
    SDLPresentation(const SDLPresentation&) = delete;
    SDLPresentation& operator=(const SDLPresentation&) = delete;

    bool Open(const char* title, int width, int height, int scale = 1,
              NativePresentationOptions options = {});
    bool Present565(const void* pixels, std::size_t bytes, std::size_t pitch);
    bool SaveWindowSurface(const char* bmpPath);
    bool QuitRequested();
    void PumpEvents();
    bool InputInfo(SDL_WindowID& id, int& width, int& height, int& windowWidth, int& windowHeight) const;
    bool Geometry(NativePresentationGeometry&) const;
    // One owner; an identical attachment is idempotent, a competitor is refused.
    bool AttachInput(PresentationEventSink, void*);
    void DetachInput(PresentationEventSink, void*);
    void Close();
    const char* VideoDriver() const;

private:
    SDL_Window* window_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    bool initialized_ = false;
    bool quit_ = true, pumping_ = false;
    PresentationEventSink sink_ = nullptr;
    void* sinkContext_ = nullptr;
};
}

#endif
