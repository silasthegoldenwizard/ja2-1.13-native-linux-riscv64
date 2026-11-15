/*
 * P710 native snapshot update: tests/native-core/native_checkbox_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native checkbox contracts, 2026-11-13. Authored fixtures only; no game data.
#include "Button System.h"
#include "vsurface.h"
#include "clip_state.h"
#include "shading.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <array>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {
unsigned checks=0;
UINT32 target=0;
constexpr UINT16 background=0x2104;
void check(bool ok,const char* why) { ++checks; if (!ok) throw std::runtime_error(why); }
struct Observation { INT32 reason; bool on; };
std::vector<Observation> trace;
void clicked(GUI_BUTTON* b,INT32 reason) { trace.push_back({reason,(b->uiFlags&BUTTON_CLICKED_ON)!=0}); }
void send(UINT16 kind,int x=4,int y=4) {
    InputAtom atom{}; atom.usEvent=kind;
    atom.uiParam=static_cast<UINT32>(x)|(static_cast<UINT32>(y)<<16);
    check(sgp::DispatchNativeMouseEvent(atom),"event dispatch accepted");
}
void fresh() {
    ShutdownButtonSystem(); MSYS_Shutdown(); MSYS_Init();
    check(InitButtonSystem(),"button system init");
    check(SetButtonDestBuffer(target),"offscreen destination");
    SGPRect clip{0,0,12,10}; SetClippingRect(&clip); trace.clear();
}
INT32 create(GUI_CALLBACK callback=clicked) {
    const INT32 id=CreateCheckBoxButton(2,2,"buttons.sti",31,callback);
    check(id>=0,"four-frame checkbox creation"); return id;
}
bool on(INT32 id) { return (GetButtonPtr(id)->uiFlags&BUTTON_CLICKED_ON)!=0; }
void clear() { check(ColorFillVideoSurfaceArea(target,0,0,12,10,background),"clear destination"); }
using Grid=std::array<const char*,6>;
constexpr Grid blue={".WWWWWWW","WBBBBBBW","WBBBBBBW","WBBBBBBW","WBBBBBBW","WWWWWWWW"};
constexpr Grid green={".WWWWWWW","WGGGGGGW","WGGGGGGW","WGGGGGGW","WGGGGGGW","WWWWWWWW"};
constexpr Grid red={".WWWWWWW","WRRRRRRW","WRRRRRRW","WRRRRRRW","WRRRRRRW","WWWWWWWW"};
constexpr Grid yellow={".WWWWWWW","WYYYYYYW","WYYYYYYW","WYYYYYYW","WYYYYYYW","WWWWWWWW"};
constexpr Grid gray={".WWWWWWW","WAAAAAAW","WAAAAAAW","WAAAAAAW","WAAAAAAW","WWWWWWWW"};
// Literal source hatch: always-filled first column, clipped-origin phase.
constexpr Grid greenHatch={"0W0W0W0W","00G0G0G0","0G0G0G0W","00G0G0G0","0G0G0G0W","00W0W0W0"};
constexpr Grid yellowHatch={"0W0W0W0W","00Y0Y0Y0","0Y0Y0Y0W","00Y0Y0Y0","0Y0Y0Y0W","00W0W0W0"};
constexpr Grid greenShade={".sssssss","sggggggs","sggggggs","sggggggs","sggggggs","ssssssss"};
UINT16 color(char c) {
    switch(c) {
    case '.': return background; case 'W': return 0xffff; case 'B': return 0x001f;
    case 'G': return 0x07e0; case 'R': return 0xf800; case 'Y': return 0xffe0;
    case 'A': return 0x8410; case '0': return 0; case 's': return 0x7bcf;
    case 'g': return 0x03c0; case 'd': return 0x1062;
    default: throw std::runtime_error("invalid expected-grid symbol");
    }
}
void pixels(const Grid& grid,const char* why) {
    UINT32 pitch=0; BYTE* buffer=LockVideoSurface(target,&pitch);
    check(buffer!=nullptr,"readback lock");
    bool matches=true;
    for (unsigned y=0;y<10;++y) for (unsigned x=0;x<12;++x) {
        UINT16 actual=0; std::memcpy(&actual,buffer+std::size_t(y)*pitch+std::size_t(x)*2,2);
        const UINT16 expected=x>=2&&x<10&&y>=2&&y<8?color(grid[y-2][x-2]):background;
        if (actual!=expected) matches=false;
    }
    UnLockVideoSurface(target); check(matches,why);
}
void draw(INT32 id,const Grid& grid,const char* why) { clear(); check(DrawButton(id),"draw actual checkbox"); pixels(grid,why); }
void mappingAndForcedDrawing() {
    fresh(); const INT32 id=create(); auto* b=GetButtonPtr(id);
    const auto& image=ButtonPictures[b->ImageNum];
    check(image.Grayed==-1&&image.OffNormal==0&&image.OffHilite==1&&image.OnNormal==2&&image.OnHilite==3,
          "original four-frame checkbox mapping");
    check((b->uiFlags&(BUTTON_CHECKBOX|BUTTON_SELFDELETE_IMAGE))==(BUTTON_CHECKBOX|BUTTON_SELFDELETE_IMAGE),"checkbox owns image slot");
    check(GetButtonWidth(id)==8&&GetButtonHeight(id)==6,"frame-derived checkbox extent");
    draw(id,blue,"off normal literal grid including untouched transparent/outside pixels");
    send(MOUSE_POS); draw(id,blue,"unpressed hover remains off normal");
    send(LEFT_BUTTON_DOWN); check(!on(id),"press preserves logical off"); draw(id,green,"off pressed uses frame one");
    send(LEFT_BUTTON_UP); check(on(id),"release toggles logical on"); draw(id,red,"on released uses frame two");
    send(LEFT_BUTTON_DOWN); check(on(id),"second press preserves logical on"); draw(id,yellow,"on pressed uses frame three");
    send(LEFT_BUTTON_UP); check(!on(id),"second release toggles off");
    check(trace.size()==4&&trace[0].reason==4&&!trace[0].on&&trace[1].reason==8&&trace[1].on&&
          trace[2].reason==4&&trace[2].on&&trace[3].reason==8&&!trace[3].on,"callbacks observe exact before/after toggle sequence");
    // Forced drawing preserves all held bits and logical state; source leaves hover set.
    b->Area.uiFlags&=~static_cast<UINT32>(MSYS_MOUSE_IN_AREA);
    b->Area.ButtonState=MSYS_RIGHT_BUTTON|MSYS_X1_BUTTON;
    clear(); DrawCheckBoxButtonOn(id); pixels(green,"forced On visually presses off checkbox");
    check(!on(id)&&b->Area.ButtonState==(MSYS_RIGHT_BUTTON|MSYS_X1_BUTTON)&&
          (b->Area.uiFlags&MSYS_MOUSE_IN_AREA),"forced On preserves held replay/logical state and leaves source hover");
    b->Area.ButtonState=MSYS_LEFT_BUTTON|MSYS_RIGHT_BUTTON;
    clear(); DrawCheckBoxButtonOff(id); pixels(blue,"forced Off visually releases off checkbox");
    check(b->Area.ButtonState==(MSYS_LEFT_BUTTON|MSYS_RIGHT_BUTTON)&&!on(id),"forced Off restores held left and right state");
    b->uiFlags|=BUTTON_CLICKED_ON;
    clear(); DrawCheckBoxButtonOn(id); pixels(yellow,"forced On respects logically on frame mapping");
    clear(); DrawCheckBoxButtonOff(id); pixels(red,"forced Off does not uncheck logical on");
    gfRenderHilights=FALSE; clear(); DrawCheckBoxButtonOn(id); pixels(red,"source global highlight switch applies to forced drawing");
    gfRenderHilights=TRUE;
    check(trace.size()==4,"forced rendering invokes no callbacks");
    DrawCheckBoxButtonOn(-1); DrawCheckBoxButtonOff(MAX_BUTTONS);
    // Destination failure must still restore the held state.
    const UINT32 saved=ButtonDestBuffer; ButtonDestBuffer=0;
    DrawCheckBoxButtonOff(id); ButtonDestBuffer=saved;
    check(b->Area.ButtonState==(MSYS_LEFT_BUTTON|MSYS_RIGHT_BUTTON)&&on(id),"failed draw restores all replay state");
}
void disabledRendering() {
    fresh(); const INT32 id=create(); auto* b=GetButtonPtr(id); check(DisableButton(id),"disable checkbox");
    draw(id,greenHatch,"disabled off DEFAULT hatches off hilite literal grid");
    b->uiFlags|=BUTTON_CLICKED_ON; draw(id,yellowHatch,"disabled on DEFAULT hatches on hilite literal grid");
    SpecifyDisabledButtonStyle(id,DISABLED_STYLE_NONE); draw(id,yellow,"disabled NONE keeps on hilite image");
    b->uiFlags&=~static_cast<UINT32>(BUTTON_CLICKED_ON);
    SpecifyDisabledButtonStyle(id,DISABLED_STYLE_SHADED);
    Grid shaded=greenShade; shaded[0]="dsssssss";
    draw(id,shaded,"disabled SHADED shades sprite and transparent background after draw");
    SpecifyDisabledButtonStyle(id,DISABLED_STYLE_HATCHED); draw(id,greenHatch,"explicit hatch matches default checkbox hatch");
    const INT32 image=LoadButtonImage("buttons.sti",4,0,1,2,3);
    check(image>=0,"explicit gray image");
    check(SpecifyButtonImage(id,static_cast<UINT32>(image)),"rebind actual checkbox image");
    draw(id,gray,"explicit gray suppresses checkbox overlay");
}
void interactionsAndOwnership() {
    fresh(); INT32 id=create();
    send(LEFT_BUTTON_DOWN); send(MOUSE_POS,11,9); send(LEFT_BUTTON_UP,11,9);
    check(!on(id)&&trace.size()==1&&trace[0].reason==4,"release outside cancels toggle and release callback");
    send(LEFT_BUTTON_DOWN); send(MOUSE_POS,11,9); send(MOUSE_POS); send(LEFT_BUTTON_UP);
    check(on(id)&&trace.size()==3&&trace.back().on,"drag out and back accepts one release toggle");
    const auto count=trace.size(); send(LEFT_BUTTON_UP); check(on(id)&&trace.size()==count,"unpaired release cannot activate checkbox");
    send(LEFT_BUTTON_DOWN); DisableButton(id); send(LEFT_BUTTON_UP);
    check(on(id)&&trace.size()==count+1,"disable during press preserves prepress on and cancels enabled release");
    EnableButton(id); send(LEFT_BUTTON_DOWN); HideButton(id); send(LEFT_BUTTON_UP); ShowButton(id);
    check(on(id),"hide during press preserves prepress logical on");
    const auto image=GetButtonPtr(id)->ImageNum;
    const INT32 alias=UseLoadedButtonImage(static_cast<INT32>(image),-1,0,1,2,3);
    check(alias>=0,"alias owned checkbox resource");
    UnloadButtonImage(static_cast<INT32>(image)); check(ButtonPictures[image].vobj!=nullptr,"live checkbox protects owned image");
    RemoveButton(id);
    check(!GetButtonPtr(id)&&!ButtonPictures[image].vobj&&ButtonPictures[static_cast<std::size_t>(alias)].vobj,
          "checkbox removal unloads image and transfers actual object ownership to alias");
    const INT32 ordinary=QuickCreateButton(static_cast<UINT32>(alias),2,2,0,31,nullptr,nullptr);
    check(ordinary>=0,"alias survives checkbox deletion"); draw(ordinary,blue,"surviving alias still draws actual object");
    RemoveButton(ordinary); UnloadButtonImage(alias);
    check(!ButtonPictures[static_cast<std::size_t>(alias)].vobj,"last shared owner unloads once");
    const auto occupied=[] { std::size_t n=0; for (const auto& p:ButtonPictures) if(p.vobj) ++n; return n; };
    check(CreateCheckBoxButton(-1,2,"buttons.sti",31,clicked)==-1&&occupied()==0,"invalid geometry rolls back loaded image");
    check(CreateCheckBoxButton(2,2,"bad_buttons.sti",31,clicked)==-1&&occupied()==0,"malformed image fails without resource slot");
    id=create(nullptr); send(LEFT_BUTTON_DOWN); check(on(id),"pinned null-callback fallback XOR still occurs on DOWN");
    send(LEFT_BUTTON_UP); check(!on(id),"pinned null-callback checkbox XOR on UP retained");
}
void removeOnRelease(GUI_BUTTON* b,INT32 reason) {
    clicked(b,reason);
    if (reason&MSYS_CALLBACK_REASON_LBUTTON_UP) RemoveButton(b->IDNum);
}
void helpAndCallbackLifetime() {
    fresh(); const INT32 id=create(removeOnRelease); auto* b=GetButtonPtr(id);
    CHAR16 supplied[]=L"Owned tooltip"; SetButtonFastHelpText(id,supplied); supplied[0]=L'X';
    check(b->Area.FastHelpText&&std::wcscmp(b->Area.FastHelpText,L"Owned tooltip")==0,"button help delegates copied text to actual region owner");
    SetButtonFastHelpText(id,b->Area.FastHelpText+6);
    check(std::wcscmp(b->Area.FastHelpText,L"tooltip")==0,"help can safely rebind its owned substring");
    SetButtonFastHelpText(id,nullptr); check(!b->Area.FastHelpText,"null help clears actual region ownership");
    SetButtonFastHelpText(id,const_cast<STR16>(L"")); check(!b->Area.FastHelpText,"empty help clears actual region ownership");
    AllowDisabledButtonFastHelp(id,FALSE);
    check(b->Area.uiFlags&MSYS_ALLOW_DISABLED_FASTHELP,"source button helper enables flag even with FALSE");
    MSYS_AllowDisabledRegionFastHelp(&b->Area,FALSE);
    check(!(b->Area.uiFlags&MSYS_ALLOW_DISABLED_FASTHELP),"underlying region service retains real Boolean setter");
    AllowDisabledButtonFastHelp(id,TRUE); check(b->Area.uiFlags&MSYS_ALLOW_DISABLED_FASTHELP,"TRUE button help permission delegates to region");
    SetButtonFastHelpText(id,const_cast<STR16>(L"Lifetime"));
    const auto image=b->ImageNum; send(LEFT_BUTTON_DOWN); send(LEFT_BUTTON_UP);
    check(trace.size()==2&&trace.back().on&&!GetButtonPtr(id)&&!ButtonPictures[image].vobj,
          "release callback sees toggled state then safely removes help/button/image after dispatch");
    SetButtonFastHelpText(id,const_cast<STR16>(L"Removed")); AllowDisabledButtonFastHelp(id,TRUE);
    SetButtonFastHelpText(-1,nullptr); AllowDisabledButtonFastHelp(MAX_BUTTONS,FALSE);
    check(!GetButtonPtr(id),"invalid/removed helper calls do not revive slot");
}
void cleanup() {
    ShutdownButtonSystem(); MSYS_Shutdown(); ShutdownVideoSurfaceManager(); ShutdownVideoObjectManager(); fileman::CloseAll();
    vfs::Log::flushDeleteAll(); vfs::CVirtualFileSystem::shutdownVFS(); vfs::ObjectAllocator::clear();
}
}
int main(int argc,char** argv) {
    try {
        if(argc!=2) throw std::runtime_error("expected existing authored button fixture directory");
        const auto root=std::filesystem::canonical(argv[1]);
        auto directory=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
        check(directory->init(),"fixture directory");
        auto profile=std::make_unique<vfs::CVirtualProfile>("checkboxes",vfs::Path(root.u8string()),false);
        profile->addLocation(directory.get()); auto* location=directory.release(); auto* mounted=profile.get();
        getVFS()->getProfileStack()->pushProfile(profile.release()); check(getVFS()->addLocation(location,mounted),"mount fixture directory");
        check(InitializeVideoObjectManager()&&InitializeVideoSurfaceManager(),"resource managers");
        VSURFACE_DESC desc{}; desc.fCreateFlags=VSURFACE_CREATE_DEFAULT; desc.usWidth=12; desc.usHeight=10; desc.ubBitDepth=16;
        check(AddVideoSurface(&desc,&target),"offscreen surface"); BuildShadeTable();
        mappingAndForcedDrawing(); disabledRendering(); interactionsAndOwnership(); helpAndCallbackLifetime(); cleanup();
        std::cout<<"PASS: "<<checks<<" checkbox pixel/state/lifetime/help assertions; authored fixtures only\n"; return 0;
    } catch(const std::exception& e) { cleanup(); std::cerr<<"FAIL after "<<checks<<" assertions: "<<e.what()<<'\n'; return 1; }
}
