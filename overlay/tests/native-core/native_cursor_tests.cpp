/*
 * P710 native snapshot update: tests/native-core/native_cursor_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "native_video.h"
#include "native_input.h"
#include "Cursor Control.h"
#include "clip_state.h"
#include <array>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
namespace {
unsigned assertions=0;
void check(bool value,const char* why) { ++assertions; if (!value) throw std::runtime_error(why); }
SDL_Window* owned()
{
    SDL_WindowID id=0; int w=0,h=0,ww=0,wh=0;
    check(sgp::NativeVideoInputInfo(id,w,h,ww,wh),"owned SDL window geometry");
    return SDL_GetWindowFromID(id);
}
void focus()
{
    SDL_Event event{}; event.type=SDL_EVENT_WINDOW_FOCUS_GAINED; event.window.windowID=SDL_GetWindowID(owned());
    check(SDL_PushEvent(&event),"authored focus event"); sgp::PumpNativeVideoEvents(); DequeueAllKeyBoardEvents();
}
void motion(float x,float y)
{
    // Consume pending native resize/barrier events before the authored motion.
    sgp::PumpNativeVideoEvents();
    SDL_Event event{}; event.type=SDL_EVENT_MOUSE_MOTION; event.motion.windowID=SDL_GetWindowID(owned());
    event.motion.x=x; event.motion.y=y;
    check(SDL_PushEvent(&event),"authored owned-window motion"); sgp::PumpNativeVideoEvents();
}
UINT16 pixel(unsigned x,unsigned y)
{
    UINT32 pitch=0; auto* pixels=static_cast<const BYTE*>(LockMouseBuffer(&pitch));
    check(pixels && pitch>=MAX_CURSOR_WIDTH*2,"real padded mouse bitmap lock");
    UINT16 color=0; std::memcpy(&color,pixels+std::size_t(y)*pitch+x*2,2); UnlockMouseBuffer(); return color;
}
void hook1(UINT16,UINT16,UINT16,BOOLEAN,BOOLEAN) {}
void hook2(UINT16,UINT16,UINT16,BOOLEAN,BOOLEAN) {}
}
int main()
{
    try {
        check(sgp::ShutdownNativeVideo(),"initial shutdown");
        const auto before=sgp::NativeVideoGeneration();
        check(sgp::ConfigureNativeVideo(32,20,16,4,"P710 authored cursor contract"),"configure");
        check(InitializeVideoManager(nullptr,0,nullptr),"initialize owned SDL video");
        const bool isDummy=std::strcmp(sgp::NativeVideoDriver(),"dummy")==0;
        check(isDummy || std::strcmp(sgp::NativeVideoDriver(),"x11")==0,"explicit private X11 or dummy only");
        check(sgp::NativeVideoGeneration()>before,"new lifecycle generation");
        UINT16 width=0,height=0; UINT8 depth=0;
        check(GetVideoSurfaceDescription(MOUSE_BUFFER,&width,&height,&depth) && width==64 && height==64 && depth==16,"separate 64x64 RGB565 mouse surface");
        const UINT32 backing=sgp::ReservedVideoSurfaceBackingId(MOUSE_BUFFER);
        check(backing && !DeleteVideoSurfaceFromIndex(backing) && !DeleteVideoSurfaceFromIndex(MOUSE_BUFFER),"mouse backing is owned and deletion protected");
        check(InitializeVideoObjectManager(),"object manager");
        SGPRect full{0,0,64,64}; SetClippingRect(&full);
        std::array<SGPPaletteEntry,256> palette{};
        palette[1].peRed=255; palette[2].peGreen=255; palette[3].peBlue=255;
        UINT8 bytes[]={2,1,2,0,0x81,1,3,0};
        ETRLEObject frame{}; frame.uiDataLength=sizeof(bytes); frame.usWidth=2; frame.usHeight=2;
        image_type image{}; image.usWidth=2; image.usHeight=2; image.ubBitDepth=8;
        image.fFlags=IMAGE_BITMAPDATA|IMAGE_PALETTE|IMAGE_TRLECOMPRESSED;
        image.pPalette=palette.data(); image.pPixData8=bytes; image.uiSizePixData=sizeof(bytes);
        image.pETRLEObject=&frame; image.usNumberOfObjects=1;
        VOBJECT_DESC description{}; description.fCreateFlags=VOBJECT_CREATE_FROMHIMAGE; description.hImage=&image;
        UINT32 object=0; check(AddVideoObject(&description,&object),"authored ETRLE production manager object");
        check(SetMouseCursorFromObject(object,0,1,1),"original object cursor API");
        check(pixel(0,0)==0xf800 && pixel(1,0)==0x07e0 && pixel(0,1)==0 && pixel(1,1)==0x001f,"literal bitmap RGB565 and skipped transparency");
        check(guiMouseBufferState==BUFFER_DIRTY,"original dirty state");
        SDL_Cursor* previous=SDL_GetCursor();
        const bool installed=sgp::RefreshNativeCursor();
        // SDL's dummy backend can retain a software color cursor. Its driver
        // name alone does not imply failure or an actual visible device.
        if (!installed) check(isDummy && guiMouseBufferState==BUFFER_DIRTY && SDL_GetCursor()==previous,"unsupported backend retains pending bitmap and SDL cursor");
        else check(guiMouseBufferState==BUFFER_READY && SDL_GetCursor()!=previous && SDL_CursorVisible(),"SDL color cursor installed and shown");
        SDL_Cursor* installedCursor=SDL_GetCursor();
        check(SetMouseCursorProperties(32767,32767,2,2),"original signed offset accepted for pending cursor"); DirtyCursor();
        check(!sgp::RefreshNativeCursor() && guiMouseBufferState==BUFFER_DIRTY && SDL_GetCursor()==installedCursor && pixel(0,0)==0xf800,"unrepresentable cursor retains previous SDL cursor and bitmap");
        check(SetMouseCursorProperties(1,1,2,2),"restore representable cursor geometry"); DirtyCursor();
        if (installed) check(sgp::RefreshNativeCursor(),"pending cursor retry succeeds");
        UINT32 pitch=0; auto* locked=static_cast<BYTE*>(LockMouseBuffer(&pitch)); check(locked,"hold mouse lock");
        check(!EraseMouseCursor() && !SetMouseCursorFromObject(object,0,0,0),"locked cursor mutation refused");
        DirtyCursor(); check(!sgp::RefreshNativeCursor() && !sgp::RefreshNativeVideo() && !sgp::ShutdownNativeVideo(),"mouse lock preflights cursor refresh, all framebuffer work and close");
        check(locked[0]==0 && locked[1]==0xf8,"locked bytes unchanged");
        UnlockMouseBuffer();
        check(!SetMouseCursorProperties(0,0,65,2) && !SetMouseCursorProperties(0,0,2,65),"oversized geometry rejected");
        check(SetMouseCursorProperties(-2,-3,2,2),"signed hotspot properties"); DirtyCursor();
        if (!isDummy) check(sgp::RefreshNativeCursor(),"signed hotspot represented by transparent SDL padding");
        // Exercise the original controller and composite blitter, not a stand-in.
        HVOBJECT handle=nullptr; check(GetVideoObject(&handle,object),"borrow authored object");
        CursorFileData files[1]{}; files[0].fLoaded=TRUE; files[0].hVObject=handle; files[0].ubFlags=USE_EXTERN_VO_CURSOR;
        CursorData database[1]{}; database[0].usNumComposites=1; database[0].sOffsetX=CENTER_CURSOR; database[0].sOffsetY=CENTER_CURSOR;
        InitCursorDatabase(files,database,1);
        check(SetCurrentCursorFromDatabase(0),"original cursor database composition");
        check(pixel(0,0)==0xf800 && pixel(1,1)==0x001f && gsCurMouseWidth==2 && gsCurMouseHeight==2,"controller real bitmap and geometry");
        check(SetCurrentCursorFromDatabase(VIDEO_NO_CURSOR),"original no-cursor controller path");
        check(pixel(0,0)==0 && pixel(1,1)==0,"controller cleared cursor pixels");
        check(BltToMouseCursor(object,0,1,1) && pixel(1,1)==0xf800,"original direct cursor blit");
        UINT8 outlineBytes[]={4,1,254,2,254,0};
        ETRLEObject outlineFrame{}; outlineFrame.uiDataLength=sizeof(outlineBytes); outlineFrame.usWidth=4; outlineFrame.usHeight=1;
        image_type outlineImage=image; outlineImage.usWidth=4; outlineImage.usHeight=1;
        outlineImage.pPixData8=outlineBytes; outlineImage.uiSizePixData=sizeof(outlineBytes); outlineImage.pETRLEObject=&outlineFrame;
        palette[254].peRed=255; palette[254].peBlue=255;
        VOBJECT_DESC outlineDescription=description; outlineDescription.hImage=&outlineImage;
        UINT32 outlined=0; check(AddVideoObject(&outlineDescription,&outlined),"authored outline-marker production object");
        check(ColorFillVideoSurfaceArea(MOUSE_BUFFER,0,0,4,1,0x1234) &&
            BltVideoObjectOutlineFromIndex(MOUSE_BUFFER,outlined,0,0,0,0x07ff,FALSE),"outline marker skip path");
        check(pixel(0,0)==0xf800 && pixel(1,0)==0x1234 && pixel(2,0)==0x07e0 && pixel(3,0)==0x1234,"254 preserves destination without outline");
        check(BltVideoObjectOutlineFromIndex(MOUSE_BUFFER,outlined,0,0,0,0x07ff,TRUE) && pixel(1,0)==0x07ff && pixel(3,0)==0x07ff,"254 writes explicit outline color");
        check(BltVideoObjectFromIndex(MOUSE_BUFFER,outlined,0,0,0,VO_BLT_SRCTRANSPARENCY,nullptr) && pixel(1,0)==0xf81f,"ordinary transparent blit treats 254 as palette color");
        check(BltVideoObjectOutlineFromIndex(MOUSE_BUFFER,outlined,0,-1,0,0x07ff,TRUE) && pixel(0,0)==0x07ff && pixel(1,0)==0x07e0,"negative destination clipped to actual surface");
        check(!BltVideoObjectFromIndex(MOUSE_BUFFER,outlined,0,0,0,VO_BLT_SHADOW,nullptr) &&
            !BltVideoObjectFromIndex(MOUSE_BUFFER,0,0,0,0,VO_BLT_SRCTRANSPARENCY,nullptr),"unsupported mode and missing object fail honestly");
        HVOBJECT outlineHandle=nullptr; check(GetVideoObject(&outlineHandle,outlined),"borrow outline object");
        auto* encoding=static_cast<UINT8*>(outlineHandle->pPixData); const UINT8 saved=encoding[0]; encoding[0]=5;
        const UINT16 beforeMalformed=pixel(0,0);
        check(!BltVideoObjectOutlineFromIndex(MOUSE_BUFFER,outlined,0,0,0,0x07ff,TRUE) && pixel(0,0)==beforeMalformed,"malformed outline rejected before writes");
        encoding[0]=saved; check(DeleteVideoObjectFromIndex(outlined),"delete outline object");
        check(InitializeInputManager(),"owned input init"); focus();
        check(!IsCursorRestricted(),"starts unrestricted");
        motion(-4,-8); check(gusMouseXPos==-1 && gusMouseYPos==-2,"outside signed coordinates never alias an edge");
        motion(140,100); check(gusMouseXPos==35 && gusMouseYPos==25,"positive outside coordinate retained");
        motion(std::numeric_limits<float>::infinity(),0); check(gusMouseXPos==35 && gusMouseYPos==25,"nonfinite rejected");
        SGPRect clip{3,4,10,12}; RestrictMouseCursor(&clip);
        if (isDummy) {
            check(!IsCursorRestricted() && !SDL_GetWindowMouseRect(owned()),"dummy unsupported barrier is not claimed");
        } else {
        check(IsCursorRestricted() && SDL_GetWindowMouseRect(owned()),"actual owned SDL barrier readback");
        SGPRect observed{}; GetRestrictedClipCursor(&observed);
        check(observed.iLeft==3 && observed.iTop==4 && observed.iRight==10 && observed.iBottom==12,"logical restriction readback");
        motion(-40,400); check(gusMouseXPos==3 && gusMouseYPos==11,"only explicit restriction clamps authored input");
        check(SDL_SetWindowSize(owned(),100,70),"fractional-scale owned window resize");
        check(SDL_SyncWindow(owned()),"fractional resize has completed");
        sgp::PumpNativeVideoEvents(); focus();
        RestrictMouseCursor(&clip); motion(1000,1000);
        check(IsCursorRestricted() && gusMouseXPos==9 && gusMouseYPos==11,"last logical restriction pixel reachable at fractional scale");
        SGPRect single{3,4,4,5}; RestrictMouseCursor(&single);
        motion(-1000,1000);
        check(IsCursorRestricted() && gusMouseXPos==3 && gusMouseYPos==4,"single logical pixel restriction has ordered bounds");
        FreeMouseCursor(FALSE); check(SDL_SetWindowSize(owned(),128,80),"restore integer scaling");
        check(SDL_SyncWindow(owned()),"integer resize has completed");
        sgp::PumpNativeVideoEvents(); focus();
        }
        if (!isDummy) {
            // Reapply an already active restriction through a changed fit;
            // 32x20 in 128x112 is a 128x80 image at y=16.
            SGPRect fittedClip{3,4,10,12}; RestrictMouseCursor(&fittedClip);
            check(SDL_SetWindowSize(owned(),128,112) && SDL_SyncWindow(owned()),"letterbox restricted window resize");
            motion(1000,1000);
            const SDL_Rect* barrier=SDL_GetWindowMouseRect(owned());
            check(barrier && barrier->x==12 && barrier->y==32 && barrier->w==28 && barrier->h==32,
                  "resize automatically reapplies restriction inside fitted content");
            check(gusMouseXPos==9 && gusMouseYPos==11,"fitted restriction retains last logical pixel");
            SGPRect observed{}; GetRestrictedClipCursor(&observed);
            check(observed.iLeft==3 && observed.iTop==4 && observed.iRight==10 && observed.iBottom==12,
                  "fitted restriction readback is original logical rectangle");
            FreeMouseCursor(FALSE);
            SDL_WarpMouseInWindow(owned(),20,20); SDL_PumpEvents(); DequeueAllKeyBoardEvents();
            SimulateMouseMovement(7,8); SDL_PumpEvents();
            float wx=0,wy=0; SDL_GetMouseState(&wx,&wy);
            check(wx>=28 && wx<32 && wy>=48 && wy<52,"fitted warp includes letterbox origin");
            check(SDL_SetWindowSize(owned(),128,80) && SDL_SyncWindow(owned()),"restore exact-aspect cursor window");
            sgp::PumpNativeVideoEvents(); focus();
        }
        FreeMouseCursor(FALSE); check(!IsCursorRestricted() && !SDL_GetWindowMouseRect(owned()),"actual SDL barrier released");
        motion(-4,-8); check(gusMouseXPos==-1 && gusMouseYPos==-2,"outside preserved after free");
        SetMouseInputHook(hook1); SetMouseInputHook(hook2); sgp::ClearNativeMouseInputHook(hook1);
        check(MouseSystemHook==hook2,"old owner cannot unbind a replacement"); sgp::ClearNativeMouseInputHook(hook2); check(!MouseSystemHook,"current owner can unbind");
        if (!isDummy) {
            // Private X11 warp: inspect SDL's own position after synchronization.
            SDL_WarpMouseInWindow(owned(),20,20); SDL_PumpEvents(); DequeueAllKeyBoardEvents();
            SimulateMouseMovement(7,8); SDL_PumpEvents();
            float x=0,y=0; SDL_GetMouseState(&x,&y);
            check(x>=28 && x<32 && y>=32 && y<36,"real SDL owned-window warp reached requested logical pixel");
        }
        ShutdownInputManager();
        if (!isDummy) { check(HideMouseCursor() && !SDL_CursorVisible(),"actual cursor hidden"); EnableCursor(FALSE); check(SDL_CursorVisible(),"historical FALSE enables"); EnableCursor(TRUE); check(!SDL_CursorVisible(),"historical TRUE disables"); }
        check(DeleteVideoObjectFromIndex(object),"object delete");
        check(sgp::ShutdownNativeVideo(),"clean shutdown");
        HVSURFACE absent=nullptr; check(!GetVideoSurface(&absent,MOUSE_BUFFER) && !GetVideoSurface(&absent,backing),"cursor alias and old backing invalidated");
        check(InitializeVideoManager(nullptr,0,nullptr),"new video generation");
        check(sgp::NativeVideoGeneration()>before && sgp::ReservedVideoSurfaceBackingId(MOUSE_BUFFER)!=backing,"fresh cursor backing and generation");
        check(guiMouseBufferState==BUFFER_DISABLED && pixel(0,0)==0,"new lifecycle clears cursor state");
        check(sgp::ShutdownNativeVideo() && ShutdownVideoObjectManager() && ShutdownVideoSurfaceManager(),"final teardown");
        std::cout<<"PASS: "<<assertions<<" assertions; authored cursor resources; "<<(isDummy?"offscreen SDL cursor state":"private owned X11 cursor/barrier/warp")<<"; no desktop or human-input claim\n"; return 0;
    } catch (const std::exception& error) {
        const std::string sdlError=SDL_GetError();
        SDL_WindowID id=0; int w=0,h=0,ww=0,wh=0;
        const bool geometry=sgp::NativeVideoInputInfo(id,w,h,ww,wh);
        std::cerr<<"Observed input="<<gusMouseXPos<<","<<gusMouseYPos<<" geometry="<<geometry
                 <<" logical="<<w<<"x"<<h<<" window="<<ww<<"x"<<wh<<"\n";
        InitCursorDatabase(nullptr,nullptr,0);
        ShutdownInputManager(); (void)sgp::ShutdownNativeVideo();
        (void)ShutdownVideoObjectManager(); (void)ShutdownVideoSurfaceManager();
        std::cerr<<"FAIL after "<<assertions<<" assertions: "<<error.what()<<" (SDL: "<<sdlError<<")\n"; return 1;
    }
}
