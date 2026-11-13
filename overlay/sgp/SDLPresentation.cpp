/*
 * P710 native notice preparation: sgp/SDLPresentation.cpp
 * New project path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: implement SDL3 software-presentation boundary.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

#include "SDLPresentation.h"
#include "himage.h"
#include <limits>

sgp::SDLPresentation::~SDLPresentation() { Close(); }

bool sgp::SDLPresentation::Open(const char* title, int width, int height, int scale)
{
    Close();
    if (width <= 0 || height <= 0 || scale <= 0 ||
        width > (std::numeric_limits<int>::max)() / scale ||
        height > (std::numeric_limits<int>::max)() / scale)
        return false;
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) return false;
    initialized_ = true;
    window_ = SDL_CreateWindow(title ? title : "JA2 v1.13 native Linux", width * scale, height * scale, 0);
    if (!window_) { Close(); return false; }
    width_ = width;
    height_ = height;
    ConfigureRGB565ImageFormat();
    return true;
}

bool sgp::SDLPresentation::Present565(const void* pixels, std::size_t bytes, std::size_t pitch)
{
    if (!window_ || !pixels || pitch > static_cast<std::size_t>((std::numeric_limits<int>::max)()) ||
        pitch < static_cast<std::size_t>(width_) * 2)
        return false;
    const auto rows = static_cast<std::size_t>(height_ - 1);
    if (rows && pitch > ((std::numeric_limits<std::size_t>::max)() - static_cast<std::size_t>(width_) * 2) / rows)
        return false;
    if (bytes < rows * pitch + static_cast<std::size_t>(width_) * 2) return false;
    SDL_Surface* source = SDL_CreateSurfaceFrom(width_, height_, SDL_PIXELFORMAT_RGB565,
        const_cast<void*>(pixels), static_cast<int>(pitch));
    if (!source) return false;
    SDL_Surface* destination = SDL_GetWindowSurface(window_);
    const bool result = destination && SDL_BlitSurfaceScaled(source, nullptr, destination, nullptr, SDL_SCALEMODE_NEAREST) &&
                        SDL_UpdateWindowSurface(window_);
    SDL_DestroySurface(source);
    return result;
}

bool sgp::SDLPresentation::SaveWindowSurface(const char* bmpPath)
{
    if (!window_ || !bmpPath) return false;
    SDL_Surface* surface = SDL_GetWindowSurface(window_);
    return surface && SDL_SaveBMP(surface, bmpPath);
}

bool sgp::SDLPresentation::QuitRequested()
{
    if (!window_) return true;
    SDL_Event event;
    while (SDL_PollEvent(&event))
        if (event.type == SDL_EVENT_QUIT ||
            (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window_)))
            return true;
    return false;
}

const char* sgp::SDLPresentation::VideoDriver() const
{
    const char* driver = SDL_GetCurrentVideoDriver();
    return driver ? driver : "uninitialized";
}

void sgp::SDLPresentation::Close()
{
    if (window_) SDL_DestroyWindow(window_);
    window_ = nullptr;
    if (initialized_) SDL_QuitSubSystem(SDL_INIT_VIDEO);
    initialized_ = false;
    width_ = height_ = 0;
}
