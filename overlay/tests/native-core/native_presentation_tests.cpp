/*
 * P710 native snapshot update: tests/native-core/native_presentation_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "SDLPresentation.h"
#include <array>
#include <climits>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {
unsigned checks=0;
void check(bool ok,const char* text) { ++checks;if (!ok) throw std::runtime_error(text); }
void geometry() {
    sgp::NativePresentationGeometry g{};
    check(sgp::MakeNativePresentationGeometry(32,20,256,192,256,192,g) &&
          g.contentX==0 && g.contentY==16 && g.contentWidth==256 && g.contentHeight==160,"literal letterbox rectangle");
    double x=0,y=0;
    check(sgp::NativeWindowToLogical(g,104,136,x,y) && x==13 && y==15,"visible target maps through letterbox");
    check(sgp::NativeLogicalToWindow(g,13,15,x,y) && x==104 && y==136,"inverse point transform");
    check(!sgp::NativePresentationContains(g,104,15.99) && sgp::NativePresentationContains(g,0,16) &&
          sgp::NativePresentationContains(g,255.99,175.99) && !sgp::NativePresentationContains(g,256,176),"half-open content edges exclude bars");
    check(sgp::MakeNativePresentationGeometry(32,20,320,160,320,160,g) &&
          g.contentX==32 && g.contentY==0 && g.contentWidth==256 && g.contentHeight==160,"literal pillarbox rectangle");
    check(sgp::NativeWindowToLogical(g,136,120,x,y) && x==13 && y==15,"pillarbox point maps exactly");
    check(sgp::MakeNativePresentationGeometry(32,20,160,96,320,192,g) &&
          g.contentY==0 && g.contentX==6 && g.contentWidth==307 && g.contentHeight==192,"HiDPI fit uses actual drawable aspect");
    // Same aspect as the first case with2x drawable density: events use points.
    check(sgp::MakeNativePresentationGeometry(32,20,256,192,512,384,g) &&
          g.contentY==32 && g.contentHeight==320,"HiDPI pixel rectangle");
    check(sgp::NativeWindowToLogical(g,104,136,x,y) && x==13 && y==15,"HiDPI input keeps window-point coordinates");
    check(sgp::MakeNativePresentationGeometry(1708,720,2560,1080,2560,1080,g) &&
          !g.contentX && !g.contentY && g.contentWidth==2560 && g.contentHeight==1079,"upper-monitor full-feature layout preserves proportions within one rounding row");
    check(sgp::MakeNativePresentationGeometry(1720,720,3440,1440,3440,1440,g) &&
          !g.contentX && !g.contentY && g.contentWidth==3440 && g.contentHeight==1440,"1440p ultrawide supports exact2x UI coverage");
    const auto before=g;
    check(!sgp::MakeNativePresentationGeometry(0,20,256,192,256,192,g) && g==before,"invalid geometry leaves output untouched");
    check(!sgp::NativeWindowToLogical(g,std::numeric_limits<double>::infinity(),0,x,y),"reject nonfinite pointer");
    check(sgp::MakeNativePresentationGeometry(INT_MAX,INT_MAX,INT_MAX,INT_MAX,INT_MAX,INT_MAX,g) &&
          g.contentWidth==INT_MAX && g.contentHeight==INT_MAX,"large geometry products do not overflow");
}
std::array<Uint8,3> rgb(int x,int y) {
    if (x<8) return {255,0,0};
    if (x<16) return {0,255,0};
    if (y<10) return {0,0,255};
    return {255,255,255};
}
void fittedPixels() {
    sgp::SDLPresentation p;
    check(p.Open("P710 authored fitted presentation",32,20,8),"open actual X11 software window");
    SDL_WindowID id=0;int lw=0,lh=0,ww=0,wh=0;
    check(p.InputInfo(id,lw,lh,ww,wh),"read owned window");
    SDL_Window* window=SDL_GetWindowFromID(id);
    std::vector<unsigned char> source(16+72*20+16,0xa7);
    for (int y=0;y<20;++y) for (int x=0;x<32;++x) {
        const Uint16 color=x<8?0xf800:x<16?0x07e0:y<10?0x001f:0xffff;
        std::memcpy(source.data()+16+y*72+x*2,&color,2);
    }
    const auto original=source;
    for (int caseIndex=0;caseIndex<2;++caseIndex) {
        const int width=caseIndex?320:256,height=caseIndex?160:192;
        const int left=caseIndex?32:0,top=caseIndex?0:16;
        check(SDL_SetWindowSize(window,width,height) && SDL_SyncWindow(window),"synchronize actual aspect-changing resize");
        check(p.Present565(source.data()+16,72*20,72),"present actual padded RGB565 without stretching");
        SDL_Surface* surface=SDL_GetWindowSurface(window);
        check(surface && surface->w==width && surface->h==height,"actual surface dimensions");
        for (int y=0;y<height;++y) for (int x=0;x<width;++x) {
            Uint8 r=0,g=0,b=0,a=0;
            check(SDL_ReadSurfacePixel(surface,x,y,&r,&g,&b,&a),"read actual presented pixel");
            const bool content=x>=left && x<left+256 && y>=top && y<top+160;
            const auto expected=content?rgb((x-left)/8,(y-top)/8):std::array<Uint8,3>{0,0,0};
            check(r==expected[0] && g==expected[1] && b==expected[2],"literal nearest pixels and black bars");
        }
        check(source==original,"source framebuffer, guards and padding remain unchanged");
    }
}
void resolutionMatrix() {
    // Physical size, logical canvas and literal fitted height. The odd aspect
    // cases deliberately retain their one-pixel rounding bar.
    const int cases[][5]={
        {1024,768,1280,960,768},{1280,720,1280,720,720},
        {1280,1024,1280,1024,1024},{1366,768,1282,720,767},
        {1600,900,1280,720,900},{1920,1080,1280,720,1080},
        {2560,1080,1708,720,1079},{2560,1440,1280,720,1440},
        {3440,1440,1720,720,1440},{3840,2160,1920,1080,2160}
    };
    for (const auto& c:cases) {
        sgp::SDLPresentation p;
        check(p.Open("P710 authored resolution matrix",c[2],c[3],1),"open full-size logical canvas");
        SDL_WindowID id=0;int lw=0,lh=0,ww=0,wh=0;
        check(p.InputInfo(id,lw,lh,ww,wh),"read matrix window");
        auto* window=SDL_GetWindowFromID(id);
        check(SDL_SetWindowSize(window,c[0],c[1]) && SDL_SyncWindow(window),"apply exact matrix output size");
        const int pitch=c[2]*2+8;
        std::vector<unsigned char> source(16+pitch*c[3]+16,0xa7);
        for (int y=0;y<c[3];++y) for (int x=0;x<c[2];++x) {
            const Uint16 color=y<c[3]/2?(x<c[2]/2?0xf800:0x07e0):(x<c[2]/2?0x001f:0xffff);
            std::memcpy(source.data()+16+y*pitch+x*2,&color,2);
        }
        const auto original=source;
        check(p.Present565(source.data()+16,pitch*c[3],pitch),"present full-size padded resolution frame");
        SDL_Surface* surface=SDL_GetWindowSurface(window);
        sgp::NativePresentationGeometry g{};
        check(surface && surface->w==c[0] && surface->h==c[1] && p.Geometry(g) &&
              !g.contentX && !g.contentY && g.contentWidth==c[0] && g.contentHeight==c[4],
              "actual full-size surface matches literal matrix rectangle");
        for (int y=0;y<c[1];++y) for (int x=0;x<c[0];++x) {
            // Sample an independent grid plus every outside-bar pixel. Avoid
            // ambiguous nearest-neighbor quadrant boundaries by two pixels.
            if (y<c[4] && x%31 && y%29) continue;
            if (y<c[4] && (std::abs(x-c[0]/2)<2 || std::abs(y-c[4]/2)<2)) continue;
            Uint8 red=0,green=0,blue=0,alpha=0;
            check(SDL_ReadSurfacePixel(surface,x,y,&red,&green,&blue,&alpha),"read matrix output pixel");
            const auto expected=y>=c[4]?std::array<Uint8,3>{0,0,0}:
                y<c[4]/2?(x<c[0]/2?std::array<Uint8,3>{255,0,0}:std::array<Uint8,3>{0,255,0}):
                (x<c[0]/2?std::array<Uint8,3>{0,0,255}:std::array<Uint8,3>{255,255,255});
            check(red==expected[0] && green==expected[1] && blue==expected[2],"literal matrix pixels and rounding bar");
        }
        double x=0,y=0;
        check(sgp::NativeWindowToLogical(g,c[0]*0.25,c[4]*0.75,x,y) &&
              std::abs(x-c[2]*0.25)<1e-6 && std::abs(y-c[3]*0.75)<1e-6,
              "physical matrix target maps to the expected logical position");
        check(source==original,"matrix source guards and row padding unchanged");
        std::cout<<"MATRIX "<<c[0]<<'x'<<c[1]<<" logical "<<c[2]<<'x'<<c[3]<<
            " content "<<g.contentWidth<<'x'<<g.contentHeight<<" PASS\n";
    }
}
void fullscreen(bool required) {
    const auto before=SDL_WasInit(SDL_INIT_VIDEO);
    SDL_Rect bounds{};
    check(sgp::QueryNativeDisplayBounds(sgp::NativeDisplayTarget::Upper,bounds),"query upper SDL display");
    check(SDL_WasInit(SDL_INIT_VIDEO)==before,"display query balances its SDL reference");
    const auto saved=bounds;
    check(!sgp::QueryNativeDisplayBounds(static_cast<sgp::NativeDisplayTarget>(99),bounds) &&
          bounds.x==saved.x && bounds.y==saved.y && bounds.w==saved.w && bounds.h==saved.h,"invalid selector cannot silently choose another display");
    sgp::SDLPresentation p;
    sgp::NativePresentationOptions options;options.borderless=true;options.display=sgp::NativeDisplayTarget::Upper;
    const bool opened=p.Open("P710 authored borderless desktop",80,45,1,options);
    if (!opened) {
        // The private Xvfb has no WM to acknowledge the SDL fullscreen request.
        // Exercise clean refusal here; the real desktop probe requires success.
        check(!required && std::strstr(SDL_GetError(),"synchronize presentation window")!=nullptr,
              "fullscreen must succeed on a window-managed desktop");
        check(SDL_WasInit(SDL_INIT_VIDEO)==before,"refused transition closes window and video reference");
        SDL_Rect unchanged{};
        check(sgp::QueryNativeDisplayBounds(sgp::NativeDisplayTarget::Upper,unchanged) &&
              unchanged.x==saved.x && unchanged.y==saved.y && unchanged.w==saved.w && unchanged.h==saved.h,
              "refused transition leaves desktop mode/bounds unchanged");
        std::cout<<"Headless WM absent: verified clean fullscreen refusal; real desktop success is a separate required probe\n";
        return;
    }
    check(opened,"open desktop fullscreen on selected display");
    SDL_WindowID id=0;int lw=0,lh=0,ww=0,wh=0;
    check(p.InputInfo(id,lw,lh,ww,wh) && lw==80 && lh==45 && ww==bounds.w && wh==bounds.h,"fullscreen keeps logical dimensions separate");
    SDL_Window* window=SDL_GetWindowFromID(id);int x=0,y=0;
    check((SDL_GetWindowFlags(window)&SDL_WINDOW_FULLSCREEN) && (SDL_GetWindowFlags(window)&SDL_WINDOW_BORDERLESS),"actual fullscreen and borderless flags");
    check(SDL_GetWindowPosition(window,&x,&y) && x==bounds.x && y==bounds.y,"actual selected monitor origin");
    std::vector<Uint16> source(80*45,0xf800);
    check(p.Present565(source.data(),source.size()*2,80*2),"present authored fullscreen frame");
    sgp::NativePresentationGeometry g{};
    check(p.Geometry(g) && g.windowWidth==bounds.w && g.windowHeight==bounds.h,"query actual fullscreen mapping");
    p.Close();
    SDL_Rect after{};check(sgp::QueryNativeDisplayBounds(sgp::NativeDisplayTarget::Upper,after) &&
        after.x==saved.x && after.y==saved.y && after.w==saved.w && after.h==saved.h,"desktop mode/bounds preserved after closing");
    check(SDL_WasInit(SDL_INIT_VIDEO)==before,"fullscreen lifecycle balances SDL reference");
}
}
int main(int argc,char** argv) {
    try {
        if (argc>1) fullscreen(std::strcmp(argv[1],"--fullscreen-required")==0); else {geometry();fittedPixels();resolutionMatrix();}
        std::cout<<"PASS: "<<checks<<" presentation geometry/pixel/desktop assertions\n";return 0;
    } catch (const std::exception& e) { std::cerr<<"FAIL after "<<checks<<": "<<e.what()<<"; SDL: "<<SDL_GetError()<<'\n';return 1; }
}
