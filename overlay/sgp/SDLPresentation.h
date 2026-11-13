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

#ifndef SGP_SDL_PRESENTATION_H
#define SGP_SDL_PRESENTATION_H

#include <SDL3/SDL.h>
#include <cstddef>

namespace sgp
{
// Native software-presentation boundary for the engine's existing RGB565 pixels.
// This owns its window/video reference, and never takes ownership of frame data.
class SDLPresentation
{
public:
    SDLPresentation() = default;
    ~SDLPresentation();
    SDLPresentation(const SDLPresentation&) = delete;
    SDLPresentation& operator=(const SDLPresentation&) = delete;

    bool Open(const char* title, int width, int height, int scale = 1);
    bool Present565(const void* pixels, std::size_t bytes, std::size_t pitch);
    bool SaveWindowSurface(const char* bmpPath);
    bool QuitRequested();
    void Close();
    const char* VideoDriver() const;

private:
    SDL_Window* window_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    bool initialized_ = false;
};
}

#endif
