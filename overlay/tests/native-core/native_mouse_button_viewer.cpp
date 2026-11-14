/*
 * P710 native snapshot update: tests/native-core/native_mouse_button_viewer.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native UI integration added 2026-11-12. Authored resources only; existing
// input, MSYS/button interfaces, bitmap font and video managers are exercised.
#include "Button System.h"
#include "Font.h"
#include "input.h"
#include "native_video.h"
#include "clip_state.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
unsigned checks=0, activations=0, presses=0;
SDL_WindowID windowId=0;
INT32 buttonId=-1;
void require(bool pass,const char* message)
{
    ++checks;
    if (!pass) throw std::runtime_error(message);
}
GUI_BUTTON* button()
{
    auto* result=GetButtonPtr(buttonId);
    require(result!=nullptr,"button remains registered");
    return result;
}
void click(GUI_BUTTON*,INT32 reason)
{
    if ((reason&MSYS_CALLBACK_REASON_LBUTTON_DWN)!=0) ++presses;
    if ((reason&MSYS_CALLBACK_REASON_LBUTTON_UP)!=0) ++activations;
}
void push(SDL_Event event)
{
    require(SDL_PushEvent(&event),"push owned-window test event");
}
void windowEvent(Uint32 type)
{
    SDL_Event e{}; e.type=type; e.window.windowID=windowId; push(e);
}
void motion(float x,float y)
{
    SDL_Event e{}; e.type=SDL_EVENT_MOUSE_MOTION;
    e.motion.windowID=windowId; e.motion.x=x*8.0f; e.motion.y=y*8.0f; push(e);
}
void left(bool down,float x,float y)
{
    SDL_Event e{};
    e.type=down?SDL_EVENT_MOUSE_BUTTON_DOWN:SDL_EVENT_MOUSE_BUTTON_UP;
    e.button.windowID=windowId; e.button.button=SDL_BUTTON_LEFT;
    e.button.down=down; e.button.x=x*8.0f; e.button.y=y*8.0f; push(e);
}
void consume()
{
    InputAtom atom{};
    while (DequeueEvent(&atom)) {
        if (atom.usEvent==MOUSE_POS || atom.usEvent==LEFT_BUTTON_DOWN || atom.usEvent==LEFT_BUTTON_UP) {
            require(sgp::DispatchNativeMouseEvent(atom),"mouse atom dispatched");
            std::cout<<"UI event="<<atom.usEvent<<" at="<<GETXPOS(&atom)<<','<<GETYPOS(&atom)
                     <<" activations="<<activations<<'\n';
        }
    }
}
void frame(const std::string& base,const char* state)
{
    require(ColorFillVideoSurfaceArea(FRAME_BUFFER,0,0,32,20,0x2104),"scene background");
    MarkButtonsDirty();
    require(DrawButton(buttonId),"actual quick-button image draw");
    require(SetFontDestBuffer(FRAME_BUFFER,0,0,32,20,FALSE),"font destination");
    SetRGBFontForeground(static_cast<UINT16>(0xffff));
    SetRGBFontShadow(static_cast<UINT16>(0x07e0)); SetFontBackground(0);
    require(mprintf(16,5,L"AB")==0,"existing bitmap font caption");
    require(ColorFillVideoSurfaceArea(FRAME_BUFFER,26,3,27,4,
                activations==0?0xf800:0x07e0),"callback activation marker");
    InvalidateScreen(); require(sgp::RefreshNativeVideo(),"video manager presentation");
    const std::string path=base+'-'+state;
    require(sgp::SaveNativeVideoWindow((path+".bmp").c_str()),"actual window capture");
    UINT32 pitch=0;
    const auto* pixels=static_cast<const BYTE*>(LockPrimarySurface(&pitch));
    require(pixels!=nullptr,"primary readback");
    std::ofstream output(path+".rgb565",std::ios::binary);
    output.write(reinterpret_cast<const char*>(pixels),static_cast<std::streamsize>(pitch)*20);
    output.close(); const bool ok=static_cast<bool>(output);
    bool padding=true;
    for (unsigned y=0;y<20;++y) for (UINT32 x=64;x<pitch;++x)
        if (pixels[static_cast<std::size_t>(y)*pitch+x]!=0xa5) padding=false;
    UnlockPrimarySurface();
    require(ok && pitch==72 && padding,"complete guarded/padded raw capture");
    std::cout<<"FRAME "<<state<<" flags="<<button()->uiFlags
             <<" presses="<<presses<<" activations="<<activations<<'\n';
    SDL_Delay(20);
}
void cleanup()
{
    ShutdownButtonSystem(); MSYS_Shutdown(); ShutdownInputManager(); ShutdownFontManager();
    (void)sgp::ShutdownNativeVideo(); (void)ShutdownVideoSurfaceManager();
    (void)ShutdownVideoObjectManager(); fileman::CloseAll(); vfs::Log::flushDeleteAll();
    vfs::CVirtualFileSystem::shutdownVFS(); vfs::ObjectAllocator::clear(); SDL_Quit();
}
}
int main(int argc,char** argv)
{
    try {
        if (argc!=3) throw std::runtime_error("expected authored fixture directory and capture base");
        const auto root=std::filesystem::canonical(argv[1]);
        const auto base=std::filesystem::absolute(argv[2]);
        std::filesystem::create_directories(base.parent_path());
        auto dir=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
        require(dir->init(),"fixture directory");
        auto profile=std::make_unique<vfs::CVirtualProfile>("native-buttons",vfs::Path(root.u8string()),false);
        profile->addLocation(dir.get()); auto* location=dir.release(); auto* mounted=profile.get();
        getVFS()->getProfileStack()->pushProfile(profile.release());
        require(getVFS()->addLocation(location,mounted),"fixture VFS mount");
        require(InitializeVideoObjectManager(),"video objects");
        require(sgp::ConfigureNativeVideo(32,20,16,8,"JA2 native mouse regions and buttons"),"video configuration");
        require(InitializeVideoManager(nullptr,0,nullptr),"native video");
        require(InitializeInputManager(),"native input");
        int lw=0,lh=0,ww=0,wh=0;
        require(sgp::NativeVideoInputInfo(windowId,lw,lh,ww,wh),"owned geometry");
        require(lw==32 && lh==20 && ww==256 && wh==160,"logical/actual geometry");
        gfTrackMousePos=TRUE;
        SGPRect clip{0,0,32,20}; SetClippingRect(&clip);
        require(MSYS_Init()!=0,"MSYS init"); require(InitButtonSystem(),"button init");
        require(SetButtonDestBuffer(FRAME_BUFFER),"button target");
        UINT16 symbols[]={65,66,32}; FontTranslationTable table{3,symbols};
        require(InitializeFontManager(16,&table),"font manager");
        char fontFile[]="font.sti"; const INT32 font=LoadFontFile(fontFile);
        require(font>=0 && SetFont(font),"authored font");
        char imageFile[]="buttons.sti";
        const INT32 image=LoadButtonImage(imageFile,4,0,1,2,3);
        require(image>=0,"five-state authored quick image");
        buttonId=QuickCreateButton(static_cast<UINT32>(image),2,3,BUTTON_NO_TOGGLE,
                                  MSYS_PRIORITY_NORMAL,DEFAULT_MOVE_CALLBACK,click);
        require(buttonId>=0,"quick button");
        std::cout<<"mode=scripted; owned SDL queue -> MSYS -> quick button; not physical-human acceptance\n";
        windowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED); motion(20,15); consume();
        frame(base.string(),"idle");
        motion(4,5); consume(); require((button()->Area.uiFlags&MSYS_MOUSE_IN_AREA)!=0,"hover region");
        frame(base.string(),"hover");
        left(true,4,5); consume(); require((button()->uiFlags&BUTTON_CLICKED_ON)!=0,"pressed state");
        require(presses==1 && activations==0,"down callback only"); frame(base.string(),"pressed");
        left(false,4,5); consume(); require((button()->uiFlags&BUTTON_CLICKED_ON)==0,"push released state");
        require(activations==1,"inside release activates once"); frame(base.string(),"released");
        left(true,4,5); motion(20,15); left(false,20,15); consume();
        require((button()->uiFlags&BUTTON_CLICKED_ON)==0 && activations==1,"outside release cancels");
        frame(base.string(),"cancelled");
        motion(4,5); left(true,4,5); consume();
        require((button()->uiFlags&BUTTON_CLICKED_ON)!=0,"pressed before focus loss");
        windowEvent(SDL_EVENT_WINDOW_FOCUS_LOST); consume();
        require(activations==1 && (button()->uiFlags&BUTTON_CLICKED_ON)==0,"focus loss cancels without activation");
        windowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED); motion(20,15); motion(4,5); consume();
        require(DisableButton(buttonId),"disable returns old enabled state");
        left(true,4,5); left(false,4,5); consume();
        require(activations==1 && (button()->uiFlags&BUTTON_ENABLED)==0,"disabled button suppresses activation");
        frame(base.string(),"disabled");
        RemoveButton(buttonId); buttonId=-1; UnloadButtonImage(image);
        cleanup();
        std::cout<<"PASS: "<<checks<<" native UI viewer assertions; six actual frames; one activation; outside/focus cancel\n";
        return 0;
    } catch (const std::exception& error) {
        cleanup(); std::cerr<<"FAIL after "<<checks<<" UI viewer assertions: "<<error.what()<<'\n'; return 1;
    }
}
