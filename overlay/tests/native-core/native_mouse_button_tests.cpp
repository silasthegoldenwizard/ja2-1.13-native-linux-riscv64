/*
 * P710 native snapshot update: tests/native-core/native_mouse_button_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: tests/native-core/native_mouse_button_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted magazine and UI cleanup checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native change, 2026-11-12: synthetic original-contract UI manager tests.
#include "Button System.h"
#include "input.h"
#include "native_video.h"
#include "vsurface.h"
#include "clip_state.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>
namespace {
unsigned checks=0;
void check(bool ok,const char* why) { ++checks; if(!ok) throw std::runtime_error(why); }
struct Region : MOUSE_REGION { Region():MOUSE_REGION{} {} ~Region() { MSYS_RemoveRegion(this); } };
struct Observation { NativeUIPayload tag; INT32 reason; INT16 x,y; UINT16 state; bool inside; };
std::vector<Observation> trace;
void record(MOUSE_REGION* r,INT32 why) { trace.push_back({r->UserData[0],why,r->MouseXPos,r->MouseYPos,r->ButtonState,(r->uiFlags&MSYS_MOUSE_IN_AREA)!=0}); }
InputAtom event(UINT16 type,int x,int y) { InputAtom a{}; a.usEvent=type; a.uiParam=static_cast<UINT32>(x)|(static_cast<UINT32>(y)<<16); return a; }
void send(UINT16 type,int x,int y) { check(sgp::DispatchNativeMouseEvent(event(type,x,y)),"supported atom accepted"); }
void define(MOUSE_REGION& r,int tag,int x,int y,int right,int bottom,INT8 priority) {
    MSYS_DefineRegion(&r,static_cast<UINT16>(x),static_cast<UINT16>(y),static_cast<UINT16>(right),static_cast<UINT16>(bottom),priority,MSYS_NO_CURSOR,record,record);
    MSYS_SetRegionUserData(&r,0,tag);
}
void fresh() { ShutdownButtonSystem(); MSYS_Shutdown(); MSYS_Init(); check(InitButtonSystem(),"button init"); trace.clear(); }
MOUSE_REGION* rawOther=nullptr; unsigned rawMutations=0; bool rawNested=true;
void rawMutation(MOUSE_REGION* r,INT32 why) {
    if(why!=MSYS_CALLBACK_REASON_GAIN_MOUSE) return;
    ++rawMutations; rawNested=sgp::DispatchNativeMouseEvent(event(MOUSE_POS,4,5));
    MSYS_RemoveRegion(rawOther); MSYS_RemoveRegion(r);
    MSYS_DefineRegion(r,2,3,9,8,31,MSYS_NO_CURSOR,record,record);
    MSYS_SetRegionUserData(r,0,99);
}
void rawLifetime() {
    fresh(); Region a{},b{}; define(a,1,20,0,25,5,15);
    MSYS_DefineRegion(&b,2,3,9,8,31,MSYS_NO_CURSOR,rawMutation,record);
    rawOther=&a; rawMutations=0; rawNested=true; trace.clear(); send(MOUSE_POS,4,5);
    check(rawMutations==1 && !rawNested && trace.empty(),"GAIN removal/address reuse aborts event before stale MOVE");
    check(!a.IDNumber && b.IDNumber && b.UserData[0]==99,"callback removes other and safely redefines same address");
    RefreshMouseRegions(); check(!trace.empty() && trace.back().tag==99,"later explicit refresh reaches replacement only");
    MSYS_RemoveRegion(&b);
}
void gainConfiguration(MOUSE_REGION* r,INT32 why) {
    record(r,why);
    if(why==MSYS_CALLBACK_REASON_GAIN_MOUSE) {
        MSYS_ChangeRegionCursor(r,MSYS_NO_CURSOR);
        MSYS_EnableRegion(r);
    }
}
void gainConfigurationRegression() {
    fresh(); Region r{};
    MSYS_DefineRegion(&r,2,3,9,8,31,MSYS_NO_CURSOR,gainConfiguration,record);
    MSYS_SetRegionUserData(&r,0,7); trace.clear();
    send(MOUSE_POS,4,5); send(LEFT_BUTTON_DOWN,4,5); send(LEFT_BUTTON_UP,4,5);
    check(trace.size()==4 && trace[0].reason==128 && trace[1].reason==2 && trace[2].reason==4 && trace[3].reason==8,
          "GAIN cursor/enable configuration preserves progress through MOVE and DOWN/UP without repeated GAIN");
    check(trace[0].x==0 && trace[0].y==0 && !trace[0].inside && trace[1].x==4 && trace[1].y==5 && trace[1].inside,
          "GAIN configuration keeps old-coordinate observation and later committed MOVE");
}
void regions() {
    fresh(); Region a{},b{}; define(a,1,2,3,9,8,31); define(b,2,2,3,9,8,31);
    check(a.IDNumber && b.IDNumber && a.IDNumber!=b.IDNumber,"unique live 16-bit region IDs");
    check(MSYS_AddRegion(&a)==1,"legacy AddRegion successful no-op");
    trace.clear(); send(MOUSE_POS,9,8);
    check(trace.size()==2 && trace[0].tag==2 && trace[0].reason==128 && trace[1].reason==2,"newest tie wins inclusive lower-right edge");
    check(trace[0].x==0 && trace[0].y==0 && !trace[0].inside,"GAIN sees old coordinates and membership");
    check(trace[1].x==9 && trace[1].y==8 && trace[1].inside,"MOVE sees new coordinates and membership");
    trace.clear(); send(MOUSE_POS,9,8); check(trace.empty(),"unchanged mouse position ignored");
    MSYS_ChangeRegionPriority(&a,127); send(MOUSE_POS,8,7); check(trace.size()==1 && trace[0].tag==2,"priority mutation preserves original list order");
    trace.clear(); send(MOUSE_POS,20,15); check(trace.size()==1 && trace[0].tag==2 && trace[0].reason==64,"enabled region LOST on leaving"); send(MOUSE_POS,8,7);
    MSYS_DisableRegion(&b); trace.clear(); send(MOUSE_POS,7,6);
    check(trace.size()==2 && trace[0].tag==1 && trace[0].reason==128 && trace[1].reason==2,"disabled raw region passes through without its own LOST callback");
    MSYS_AllowDisabledRegionFastHelp(&b,TRUE); trace.clear(); send(MOUSE_POS,6,5);
    check(trace.size()==1 && trace[0].tag==1 && trace[0].reason==64,"disabled help occludes with no own callback");
    MSYS_RemoveRegion(&b); check(b.IDNumber==0 && b.uiFlags==0 && b.next==nullptr,"remove zeros caller region");
    MSYS_MoveMouseRegionTo(&a,12,4); check(a.RegionTopLeftX==12 && a.RegionBottomRightX==19,"move retains inclusive extent");
    MSYS_MoveMouseRegionBy(&a,-2,1); check(a.RegionTopLeftX==10 && a.RegionTopLeftY==5,"relative move");
    MSYS_SetRegionUserData(&a,3,-123); MSYS_SetRegionUserData(&a,4,99); check(MSYS_GetRegionUserData(&a,3)==-123 && MSYS_GetRegionUserData(&a,4)==0,"integer userdata bounds");
    check(MSYS_GetRegionUserData(nullptr,0)==0,"null region userdata"); MSYS_RemoveRegion(&a);
    fresh(); define(a,1,0,0,9,9,31); define(b,2,10,0,19,9,31);
    send(LEFT_BUTTON_DOWN,3,3); trace.clear(); send(MOUSE_POS,13,3); send(LEFT_BUTTON_UP,13,3);
    bool hover=false,wrongUp=false; for(const auto& t:trace) { hover|=t.tag==2 && t.reason==2; wrongUp|=t.tag==2 && t.reason==8; }
    check(hover && !wrongUp,"clicked ID gate permits hover but suppresses foreign release");
    trace.clear(); send(LEFT_BUTTON_DOWN,13,3); send(LEFT_BUTTON_UP,13,3);
    check(trace.size()==2 && trace[0].reason==4 && trace[1].reason==8,"fresh target works after shared gate release");
    trace.clear(); send(LEFT_BUTTON_DOWN,3,4); send(RIGHT_BUTTON_DOWN,3,4); send(LEFT_BUTTON_UP,13,4); send(RIGHT_BUTTON_UP,13,4);
    bool rightUp=false; for(const auto& t:trace) rightUp|=t.tag==2 && t.reason==32;
    check(rightUp,"either UP releases shared clicked gate despite other held button");
    trace.clear(); send(LEFT_BUTTON_DOWN,2,2); send(LEFT_BUTTON_UP,2,2);
    check(trace.back().state==0,"adapter replays release state independent of polling globals");
    check(!sgp::DispatchNativeMouseEvent(event(KEY_DOWN,2,2)) && !sgp::DispatchNativeMouseEvent(event(0xffffu,2,2)),"keyboard/unknown tags are not bitmasked into mouse events");
    check(MSYS_GrabMouse(&a)==MSYS_GRABBED_OK && MSYS_GrabMouse(&b)==MSYS_ALREADY_GRABBED,"logical grab ownership");
    trace.clear(); send(MOUSE_POS,15,8); check(!trace.empty() && trace.back().tag==1,"logical grab selects owner outside its bounds"); MSYS_ReleaseMouse(&a);
    MSYS_RemoveRegion(&a); MSYS_RemoveRegion(&b);
}
INT32 image=-1; unsigned ups=0,downs=0,disabled=0; bool callbackOn=false;
void click(GUI_BUTTON* b,INT32 reason) {
    if(static_cast<UINT32>(reason)&BUTTON_DISABLED_CALLBACK) { ++disabled; return; }
    if(reason&4) ++downs;
    if(reason&8) { ++ups; callbackOn=(b->uiFlags&BUTTON_CLICKED_ON)!=0; }
}
INT32 make(INT32 flags=BUTTON_NO_TOGGLE,GUI_CALLBACK movement=DEFAULT_MOVE_CALLBACK,GUI_CALLBACK callback=click,int x=2) {
    auto id=QuickCreateButton(static_cast<UINT32>(image),static_cast<INT16>(x),3,flags,31,movement,callback); check(id>=0,"quick creation"); return id;
}
bool on(INT32 id) { auto* b=GetButtonPtr(id); return b && (b->uiFlags&BUTTON_CLICKED_ON); }
void buttons() {
    fresh(); image=LoadButtonImage("buttons.sti",4,0,1,2,3); check(image>=0,"real button STI load");
    check(LoadButtonImage("bad_buttons.sti",4,0,1,2,3)==-1 && LoadButtonImage("buttons.sti",4,0,1,99,3)==-1,"invalid resource/state rejected");
    INT32 id=make(); check(id==0 && GetButtonWidth(id)==8 && GetButtonHeight(id)==6,"first slot and actual image geometry");
    SGPRect area{}; check(GetButtonArea(id,&area) && area.iLeft==2 && area.iTop==3,"button area query");
    check(!GetButtonArea(-1,&area) && !GetButtonArea(id,nullptr) && !GetButtonPtr(400),"button range/null outputs");
    MSYS_SetBtnUserData(id,3,-9); check(MSYS_GetBtnUserData(GetButtonPtr(id),3)==-9 && MSYS_GetBtnUserData(nullptr,0)==0,"button userdata");
    ups=downs=0; send(LEFT_BUTTON_DOWN,4,5); check(on(id) && downs==1,"default movement anchors press");
    send(MOUSE_POS,20,12); check(!on(id),"outside restores prior off state"); send(LEFT_BUTTON_UP,20,12); check(ups==0 && !on(id),"outside release cancels activation");
    send(LEFT_BUTTON_DOWN,4,5); send(MOUSE_POS,20,12); send(MOUSE_POS,4,5); check(on(id),"reentry restores anchored on state"); send(LEFT_BUTTON_UP,4,5); check(ups==1 && !on(id),"accepted push release callback exactly once and clears on");
    send(LEFT_BUTTON_UP,4,5); check(ups==1,"stray inside UP not activated");
    check(DisableButton(id) && !DisableButton(id),"disable returns prior state"); check(GetButtonPtr(id)->Area.uiFlags&MSYS_REGION_ENABLED,"disabled button region still occludes");
    Region under{}; define(under,8,2,3,9,8,15); trace.clear(); send(LEFT_BUTTON_DOWN,4,5); send(LEFT_BUTTON_UP,4,5); check(ups==1 && trace.empty(),"disabled quick button blocks underlying control without callback");
    GetButtonPtr(id)->uiFlags|=BUTTON_ALLOW_DISABLED_CALLBACK; disabled=0; send(LEFT_BUTTON_DOWN,4,5); send(LEFT_BUTTON_UP,4,5); check(disabled==2,"disabled callback retains high-bit unsigned flag");
    check(!EnableButton(id) && EnableButton(id),"enable returns prior state");
    HideButton(id); check(!(GetButtonPtr(id)->Area.uiFlags&MSYS_REGION_ENABLED),"hide disables region"); ShowButton(id); check(GetButtonPtr(id)->Area.uiFlags&MSYS_REGION_ENABLED,"show restores region");
    SetButtonPosition(id,11,2); check(GetButtonX(id)==11 && GetButtonY(id)==2,"position moves button and region");
    UnloadButtonImage(image); check(ButtonPictures[static_cast<std::size_t>(image)].vobj!=nullptr,"referenced image unload rejected");
    RemoveButton(id); MSYS_RemoveRegion(&under); id=make(0,nullptr,nullptr); check(id==0,"lowest freed slot reused");
    send(LEFT_BUTTON_DOWN,4,5); check(on(id),"null-click fallback XOR on DOWN"); send(LEFT_BUTTON_UP,4,5); check(on(id),"ordinary null-click toggle persists on UP"); RemoveButton(id);
    id=make(0,DEFAULT_MOVE_CALLBACK,nullptr); send(LEFT_BUTTON_DOWN,4,5); check(!on(id),"default anchor ON then null fallback XOR restores OFF"); send(LEFT_BUTTON_UP,4,5); RemoveButton(id);
    id=make(BUTTON_NEWTOGGLE,nullptr,nullptr); send(LEFT_BUTTON_DOWN,4,5); check(!on(id),"NEWTOGGLE precedes null fallback XOR"); send(MOUSE_POS,20,12); send(LEFT_BUTTON_UP,20,12); RemoveButton(id);
    id=make(BUTTON_IGNORE_CLICKS,nullptr,nullptr); send(LEFT_BUTTON_DOWN,4,5); send(LEFT_BUTTON_UP,4,5); check(!on(id),"ignore clicks suppresses fallback XOR"); RemoveButton(id);
    id=make(0,nullptr,click); send(LEFT_BUTTON_DOWN,4,5); send(LEFT_BUTTON_UP,4,5); check(!on(id),"custom click without default move owns ordinary toggle state"); RemoveButton(id);
    id=make(BUTTON_NEWTOGGLE,nullptr,click); send(LEFT_BUTTON_DOWN,4,5); check(on(id),"NEWTOGGLE acts before explicit callback"); send(MOUSE_POS,20,12); check(!on(id),"NEWTOGGLE lost restores prior state"); send(LEFT_BUTTON_UP,20,12); RemoveButton(id);
    id=CreateCheckBoxButton(2,3,"buttons.sti",31,click); check(id>=0,"checkbox source convention"); callbackOn=false; send(LEFT_BUTTON_DOWN,4,5); check(!on(id),"checkbox does not toggle on press"); send(LEFT_BUTTON_UP,4,5); check(on(id) && callbackOn,"checkbox toggles before accepted release callback"); RemoveButton(id);
    id=CreateHotSpot(2,3,8,6,63,nullptr,click); check(id>=0,"hotspot creation");
    const auto priorUps=ups; send(LEFT_BUTTON_DOWN,4,5); send(LEFT_BUTTON_UP,4,5); check(ups==priorUps+1,"hotspot callback without image"); RemoveButton(id);
    check(QuickCreateButton(static_cast<UINT32>(image),2,3,BUTTON_GENERIC,31,nullptr,nullptr)==-1,"unsupported generic creation rejected");
}
INT32 victim=-1,replacement=-1; unsigned mutationCalls=0; bool nestedAccepted=false;
void mutate(GUI_BUTTON* b,INT32 reason) {
    if(!(reason&(4|2|128)) && !(static_cast<UINT32>(reason)&BUTTON_DISABLED_CALLBACK)) return;
    ++mutationCalls;
    nestedAccepted=sgp::DispatchNativeMouseEvent(event(LEFT_BUTTON_DOWN,4,5));
    RemoveButton(victim); RemoveButton(b->IDNum);
    replacement=QuickCreateButton(static_cast<UINT32>(image),2,3,0,31,nullptr,click);
}
void mutation() {
    for(int mode=0;mode<3;++mode) {
        fresh(); image=LoadButtonImage("buttons.sti",4,0,1,2,3); check(image>=0,"mutation image");
        victim=make(0,nullptr,click,20); auto id=make(mode==2?BUTTON_ALLOW_DISABLED_CALLBACK:0,mode==1?mutate:nullptr,mode==1?click:mutate);
        if(mode==2) DisableButton(id);
        mutationCalls=0; replacement=-1; nestedAccepted=true;
        send(mode==1?MOUSE_POS:LEFT_BUTTON_DOWN,4,5);
        check(mutationCalls==1 && !nestedAccepted,"one callback; recursive dispatch bounded");
        check(replacement>=0 && replacement!=id && replacement!=victim,"callback deletion defers slot reuse until outer dispatch");
        check(!GetButtonPtr(victim) && !GetButtonPtr(id) && GetButtonPtr(replacement),"self/other removal preserves newly created control");
        if(mode!=1) send(LEFT_BUTTON_UP,4,5);
        auto reused=make(0,nullptr,click,20); check(reused==0,"released pending slots become reusable after outer event");
    }
}
INT32 retiringOwner=-1,retiringAlias=-1;
void retireImages(GUI_BUTTON* b,INT32 reason) {
    if (!(reason & MSYS_CALLBACK_REASON_LBUTTON_UP)) return;
    RemoveButton(b->IDNum);
    UnloadButtonImage(retiringAlias);
    UnloadButtonImage(retiringOwner);
}
void callbackImageRetirement() {
    fresh();
    for (int cycle=0;cycle<MAX_BUTTON_PICS+1;++cycle) {
        retiringOwner=LoadButtonImage("buttons.sti",4,0,1,2,3);
        retiringAlias=UseLoadedButtonImage(retiringOwner,-1,0,-1,2,-1);
        check(retiringOwner>=0 && retiringAlias>=0,"callback image resources remain available");
        const auto id=QuickCreateButton(static_cast<UINT32>(retiringAlias),2,3,0,31,nullptr,retireImages);
        check(id>=0,"callback retirement button");
        send(LEFT_BUTTON_DOWN,4,5); send(LEFT_BUTTON_UP,4,5);
        check(!GetButtonPtr(id),"callback retirement finishes button deletion");
        check(std::none_of(ButtonPictures.begin(),ButtonPictures.end(),
                          [](const BUTTON_PICS& p){return p.vobj!=nullptr;}),
              "callback retirement releases both original and alias images after dispatch");
    }
}
std::vector<BYTE> pixels(UINT32 id) {
    UINT16 w=0,h=0; UINT8 bits=0; check(GetVideoSurfaceDescription(id,&w,&h,&bits),"surface description");
    UINT32 pitch=0; auto* p=LockVideoSurface(id,&pitch); check(p!=nullptr,"surface lock");
    std::vector<BYTE> result(p,p+static_cast<std::size_t>(pitch)*h); UnLockVideoSurface(id); return result;
}
void imagesAndDraw() {
    fresh(); image=LoadButtonImage("buttons.sti",4,0,1,2,3); check(image>=0,"draw image");
    VSURFACE_DESC desc{}; desc.fCreateFlags=VSURFACE_CREATE_DEFAULT; desc.usWidth=32; desc.usHeight=20; desc.ubBitDepth=16; UINT32 surface=0;
    check(AddVideoSurface(&desc,&surface),"draw destination"); SGPRect clip{0,0,32,20}; SetClippingRect(&clip);
    check(ColorFillVideoSurfaceArea(surface,0,0,32,20,0x2104) && SetButtonDestBuffer(surface),"draw setup"); auto id=make();
    auto before=pixels(surface); check(!SetButtonDestBuffer(0) && ButtonDestBuffer==surface,"rejected destination preserves prior target");
    check(!DrawButton(-1) && pixels(surface)==before,"bad ID draw leaves allocation unchanged");
    UINT32 pitch=0; check(LockVideoSurface(surface,&pitch)!=nullptr,"lock target before failure"); check(!DrawButton(id),"locked destination draw fails"); UnLockVideoSurface(surface); check(pixels(surface)==before,"locked failure keeps pixels/padding");
    check(DrawButton(id),"actual object blit"); auto after=pixels(surface);
    UINT16 corner=0,border=0,center=0; std::memcpy(&corner,after.data()+3*pitch+2*2,2); std::memcpy(&border,after.data()+3*pitch+3*2,2); std::memcpy(&center,after.data()+4*pitch+3*2,2);
    check(corner==0x2104 && border==0xffff && center==0x001f,"transparent pixel, authored border and idle image color");
    for(unsigned y=0;y<20;++y) for(unsigned x=64;x<pitch;++x) check(after[y*pitch+x]==0xa5,"draw preserves all row padding");
    RemoveButton(id); UnloadButtonImage(image); check(!ButtonPictures[static_cast<std::size_t>(image)].vobj,"owned image unload clears slot");
    VOBJECT_DESC objectDesc{}; objectDesc.fCreateFlags=VOBJECT_CREATE_FROMFILE; std::strcpy(objectDesc.ImageFile,"buttons.sti");
    auto* borrowed=CreateVideoObject(&objectDesc); check(borrowed!=nullptr,"caller-owned video object");
    image=UseVObjAsButtonImage(borrowed,4,0,1,2,3); check(image>=0,"borrowed image"); UnloadButtonImage(image);
    check(UseVObjAsButtonImage(borrowed,4,0,1,2,3)>=0,"borrowed unload does not delete object");
    ShutdownButtonSystem(); check(DeleteVideoObject(borrowed),"caller deletes borrowed object after UI shutdown"); check(DeleteVideoSurfaceFromIndex(surface),"remove destination");
}
SDL_WindowID owned() { SDL_WindowID id=0; int a=0,b=0,c=0,d=0; check(sgp::NativeVideoInputInfo(id,a,b,c,d),"owned input window"); return id; }
void windowEvent(Uint32 type) { SDL_Event e{}; e.type=type; e.window.windowID=owned(); check(SDL_PushEvent(&e),"push window event"); }
void buttonEvent(bool down) { SDL_Event e{}; e.type=down?SDL_EVENT_MOUSE_BUTTON_DOWN:SDL_EVENT_MOUSE_BUTTON_UP; e.button.windowID=owned(); e.button.button=SDL_BUTTON_LEFT; e.button.down=down; e.button.x=32; e.button.y=40; check(SDL_PushEvent(&e),"push owned button event"); }
void drain() { InputAtom a{}; while(DequeueEvent(&a)) (void)sgp::DispatchNativeMouseEvent(a); }
void open() { check(sgp::ConfigureNativeVideo(32,20,16,8,"P710 UI contracts") && InitializeVideoManager(nullptr,0,nullptr),"open actual UI window"); check(std::strcmp(sgp::NativeVideoDriver(),"x11")==0,"X11 required"); check(InitializeInputManager(),"bind input"); windowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED); sgp::PumpNativeVideoEvents(); DequeueAllKeyBoardEvents(); }
void focus() {
    open(); fresh(); image=LoadButtonImage("buttons.sti",4,0,1,2,3); check(image>=0,"focus image"); auto id=make(); ups=downs=0;
    buttonEvent(true); buttonEvent(false); sgp::PumpNativeVideoEvents(); check(!gfLeftButtonState,"batch final polling state released"); drain(); check(downs==1 && ups==1,"queued batch DOWN+UP replays both states");
    buttonEvent(true); drain(); check(on(id),"press before focus loss"); const auto generation=sgp::NativeInputResetGeneration(); windowEvent(SDL_EVENT_WINDOW_FOCUS_LOST); drain();
    check(sgp::NativeInputResetGeneration()!=generation && !on(id) && ups==1,"focus synthetic release cancels without activation");
    windowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED); buttonEvent(true); drain(); check(on(id),"fresh valid press after focus recovery");
    check(sgp::ShutdownNativeVideo(),"destroy pressed video"); drain(); check(!on(id) && ups==1,"video destruction cancels pressed UI"); ShutdownInputManager();
    open(); buttonEvent(true); windowEvent(SDL_EVENT_WINDOW_CLOSE_REQUESTED); sgp::PumpNativeVideoEvents(); drain(); check(!on(id) && ups==1,"close before draining queued DOWN cannot activate");
    ShutdownButtonSystem(); MSYS_Shutdown(); ShutdownInputManager(); check(sgp::ShutdownNativeVideo(),"close video teardown");
    fresh(); check(GetButtonPtr(0)==nullptr,"fresh lifecycle has no stale slots"); ShutdownButtonSystem(); ShutdownButtonSystem(); MSYS_Shutdown(); MSYS_Shutdown();
}
void cleanup() {
    ShutdownButtonSystem(); MSYS_Shutdown(); ShutdownInputManager(); (void)sgp::ShutdownNativeVideo();
    ShutdownVideoSurfaceManager(); ShutdownVideoObjectManager(); fileman::CloseAll(); vfs::Log::flushDeleteAll(); vfs::CVirtualFileSystem::shutdownVFS(); vfs::ObjectAllocator::clear(); SDL_Quit();
}
}
int main(int argc,char** argv) {
    try {
        if(argc!=2) throw std::runtime_error("expected synthetic UI fixture root");
        const auto root=std::filesystem::canonical(argv[1]);
        auto dir=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string())); check(dir->init(),"fixture directory");
        auto profile=std::make_unique<vfs::CVirtualProfile>("ui",vfs::Path(root.u8string()),false); profile->addLocation(dir.get()); auto* location=dir.release(); auto* mounted=profile.get();
        getVFS()->getProfileStack()->pushProfile(profile.release()); check(getVFS()->addLocation(location,mounted),"mount synthetic fixtures");
        check(InitializeVideoObjectManager() && InitializeVideoSurfaceManager(),"resource managers");
        regions(); rawLifetime(); gainConfigurationRegression(); buttons(); mutation(); callbackImageRetirement(); imagesAndDraw(); focus(); cleanup();
        std::cout<<"PASS: "<<checks<<" UI contract assertions; synthetic owned-window SDL injection, no human-input claim\n"; return 0;
    } catch(const std::exception& e) { cleanup(); std::cerr<<"FAIL after "<<checks<<" assertions: "<<e.what()<<'\n'; return 1; }
}
