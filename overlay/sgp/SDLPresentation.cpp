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
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "SDLPresentation.h"
#include "himage.h"
#include <limits>

namespace {
SDL_DisplayID selectDisplay(sgp::NativeDisplayTarget target, SDL_Rect& bounds)
{
    SDL_DisplayID selected=0;
    SDL_Rect chosen{};
    if (target==sgp::NativeDisplayTarget::Primary) {
        selected=SDL_GetPrimaryDisplay();
        if (!selected || !SDL_GetDisplayBounds(selected,&chosen)) return 0;
    } else if (target==sgp::NativeDisplayTarget::Upper) {
        int count=0;
        SDL_DisplayID* displays=SDL_GetDisplays(&count);
        if (!displays) return 0;
        for (int i=0;i<count;++i) {
            SDL_Rect rect{};
            if (!SDL_GetDisplayBounds(displays[i],&rect) || rect.w<=0 || rect.h<=0) continue;
            if (!selected || rect.y<chosen.y || (rect.y==chosen.y && rect.x<chosen.x)) {
                selected=displays[i];chosen=rect;
            }
        }
        SDL_free(displays);
    }
    if (!selected || chosen.w<=0 || chosen.h<=0) return 0;
    bounds=chosen;return selected;
}
}

bool sgp::QueryNativeDisplayBounds(NativeDisplayTarget target, SDL_Rect& bounds)
{
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) return false;
    SDL_Rect chosen{};
    const bool ok=SDL_IsMainThread() && selectDisplay(target,chosen)!=0;
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    if (ok) bounds=chosen;
    return ok;
}

sgp::SDLPresentation::~SDLPresentation() { Close(); }

bool sgp::SDLPresentation::Open(const char* title, int width, int height, int scale,
                               NativePresentationOptions options)
{
    Close();
    if (width <= 0 || height <= 0 || scale <= 0 ||
        width > (std::numeric_limits<int>::max)() / scale ||
        height > (std::numeric_limits<int>::max)() / scale)
        return false;
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) return false;
    initialized_ = true;
    const auto failed=[&](const char* stage) {
        char error[512];SDL_snprintf(error,sizeof(error),"%s: %s",stage,SDL_GetError());
        Close();SDL_SetError("%s",error);return false;
    };
    SDL_Rect bounds{};
    const auto display=selectDisplay(options.display,bounds);
    if (!display) return failed("select display");
    const SDL_WindowFlags flags=SDL_WINDOW_HIDDEN | (options.borderless ? SDL_WINDOW_BORDERLESS : 0);
    window_ = SDL_CreateWindow(title ? title : "JA2 v1.13 native Linux", width * scale, height * scale, flags);
    if (!window_) return failed("create presentation window");
    if (!SDL_SetWindowPosition(window_,SDL_WINDOWPOS_CENTERED_DISPLAY(display),SDL_WINDOWPOS_CENTERED_DISPLAY(display))) return failed("select window position");
    // A null mode requests the existing desktop, never an exclusive mode switch.
    if (options.borderless && !SDL_SetWindowFullscreenMode(window_,nullptr)) return failed("select desktop fullscreen mode");
    if (options.borderless && !SDL_SetWindowFullscreen(window_,true)) return failed("request desktop fullscreen");
    if (!SDL_ShowWindow(window_)) return failed("show presentation window");
    if (!SDL_SyncWindow(window_)) return failed("synchronize presentation window");
    if (options.borderless) {
        int x=0,y=0,w=0,h=0;
        if (!(SDL_GetWindowFlags(window_)&SDL_WINDOW_FULLSCREEN) ||
            SDL_GetDisplayForWindow(window_)!=display ||
            !SDL_GetWindowPosition(window_,&x,&y) || !SDL_GetWindowSize(window_,&w,&h) ||
            x!=bounds.x || y!=bounds.y || w!=bounds.w || h!=bounds.h) {
            SDL_SetError("Desktop fullscreen bounds were not applied: got %dx%d at %d,%d; expected %dx%d at %d,%d",w,h,x,y,bounds.w,bounds.h,bounds.x,bounds.y);
            return failed("verify selected fullscreen display");
        }
    }
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
    int ww=0,wh=0;
    NativePresentationGeometry geometry{};
    bool result=destination && SDL_GetWindowSize(window_,&ww,&wh) &&
        MakeNativePresentationGeometry(width_,height_,ww,wh,destination->w,destination->h,geometry);
    if (result) {
        SDL_Rect rect{geometry.contentX,geometry.contentY,geometry.contentWidth,geometry.contentHeight};
        if (rect.w!=destination->w || rect.h!=destination->h)
            result=SDL_FillSurfaceRect(destination,nullptr,SDL_MapSurfaceRGB(destination,0,0,0));
        result=result && SDL_BlitSurfaceScaled(source,nullptr,destination,&rect,SDL_SCALEMODE_NEAREST) &&
                        SDL_UpdateWindowSurface(window_);
    }
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

bool sgp::SDLPresentation::Geometry(NativePresentationGeometry& out) const
{
    if (!window_ || !SDL_IsMainThread()) return false;
    int ww=0,wh=0;
    SDL_Surface* surface=SDL_GetWindowSurface(window_);
    return surface && SDL_GetWindowSize(window_,&ww,&wh) &&
        MakeNativePresentationGeometry(width_,height_,ww,wh,surface->w,surface->h,out);
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
