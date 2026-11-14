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

/*
 * P710 native snapshot update: sgp/SDLPresentation.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
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
    if (!SDL_SyncWindow(window_)) { Close(); return false; }
    width_ = width;
    height_ = height;
    quit_ = false;
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
    PumpEvents();
    return !window_ || quit_;
}

bool sgp::SDLPresentation::InputInfo(SDL_WindowID& id, int& w, int& h, int& ww, int& wh) const
{
    if (!window_ || !SDL_IsMainThread() || !SDL_GetWindowSize(window_,&ww,&wh)) return false;
    id=SDL_GetWindowID(window_); w=width_; h=height_; return id!=0 && ww>0 && wh>0;
}

bool sgp::SDLPresentation::AttachInput(PresentationEventSink fn, void* context)
{
    if (!window_ || !SDL_IsMainThread() || !fn || !context) return false;
    if (sink_) return sink_==fn && sinkContext_==context;
    sink_=fn; sinkContext_=context; return true;
}

void sgp::SDLPresentation::DetachInput(PresentationEventSink fn, void* context)
{
    if (sink_==fn && sinkContext_==context) { sink_=nullptr; sinkContext_=nullptr; }
}

void sgp::SDLPresentation::PumpEvents()
{
    if (!window_ || pumping_ || !SDL_IsMainThread()) return;
    pumping_=true;
    SDL_Event event;
    // The only application SDL polling loop. Never return early on quit.
    while (SDL_PollEvent(&event)) {
        SDL_WindowID id=0;
        switch(event.type) {
        case SDL_EVENT_QUIT: break;
        case SDL_EVENT_KEY_DOWN: case SDL_EVENT_KEY_UP: id=event.key.windowID; break;
        case SDL_EVENT_MOUSE_MOTION: id=event.motion.windowID; break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN: case SDL_EVENT_MOUSE_BUTTON_UP: id=event.button.windowID; break;
        case SDL_EVENT_MOUSE_WHEEL: id=event.wheel.windowID; break;
        case SDL_EVENT_WINDOW_FOCUS_GAINED: case SDL_EVENT_WINDOW_FOCUS_LOST:
        case SDL_EVENT_WINDOW_HIDDEN: case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        case SDL_EVENT_WINDOW_DESTROYED: id=event.window.windowID; break;
        default: continue;
        }
        if (event.type!=SDL_EVENT_QUIT && (!window_ || id!=SDL_GetWindowID(window_))) continue;
        if (event.type==SDL_EVENT_QUIT || event.type==SDL_EVENT_WINDOW_CLOSE_REQUESTED || event.type==SDL_EVENT_WINDOW_DESTROYED) quit_=true;
        const auto fn=sink_; void* context=sinkContext_;
        if (fn) fn(context,&event);
    }
    pumping_=false;
}

const char* sgp::SDLPresentation::VideoDriver() const
{
    const char* driver = SDL_GetCurrentVideoDriver();
    return driver ? driver : "uninitialized";
}

void sgp::SDLPresentation::Close()
{
    const auto fn=sink_; void* context=sinkContext_;
    sink_=nullptr; sinkContext_=nullptr;
    quit_=true;
    if (fn) fn(context,nullptr);
    if (window_) SDL_DestroyWindow(window_);
    window_ = nullptr;
    if (initialized_) SDL_QuitSubSystem(SDL_INIT_VIDEO);
    initialized_ = false;
    width_ = height_ = 0;
}
