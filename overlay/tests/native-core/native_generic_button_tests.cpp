/*
 * P710 native snapshot update: tests/native-core/native_generic_button_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Original generic-border/text/icon API contract; authored synthetic STIs only.
#include "native_buttons.h"
#include "native_font.h"
#include "clip_state.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <cstring>
#include <array>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
namespace {
unsigned checks=0;
void check(bool ok,const char* why){++checks;if(!ok)throw std::runtime_error(why);}
UINT32 target=0;
void clear(){
    SGPRect before{};GetClippingRect(&before);SGPRect all{0,0,32,24};SetClippingRect(&all);
    const bool ok=ColorFillVideoSurfaceArea(target,0,0,32,24,0x2104)!=FALSE;SetClippingRect(&before);
    check(ok,"clear authored target");
}
std::vector<UINT16> pixels(){
    UINT32 pitch=0;auto* p=LockVideoSurface(target,&pitch);check(p!=nullptr,"pixel readback exclusive lock");
    std::vector<UINT16> result(32*24);
    for(unsigned y=0;y<24;++y){
        for(unsigned x=0;x<32;++x)std::memcpy(&result[y*32+x],p+y*pitch+x*2,2);
        for(unsigned x=64;x<pitch;++x)check(p[y*pitch+x]==0xa5,"surface padding preserved");
    }
    UnLockVideoSurface(target);return result;
}
UINT16 color(char c){
    switch(c){case 'R':return 0xf800;case 'G':return 0x07e0;case 'B':return 0x001f;
    case 'Y':return 0xffe0;case 'M':return 0xf81f;case 'C':return 0x07ff;
    case 'W':return 0xffff;case 'a':return 0x8410;case '.':return 0x4208;default:return 0x2104;}
}
void grid(const std::vector<UINT16>& p,const std::vector<std::string>& rows,bool white=false,SGPRect clip={0,0,32,24}){
    for(INT32 y=0;y<24;++y)for(INT32 x=0;x<32;++x){
        UINT16 expected=0x2104;
        if(y>=2 && std::size_t(y-2)<rows.size() && x>=2 && std::size_t(x-2)<rows[y-2].size() &&
           x>=clip.iLeft && x<clip.iRight && y>=clip.iTop && y<clip.iBottom){
            char c=rows[y-2][x-2];expected=white && c!='.'?0xffff:color(c);
        }
        check(p[y*32+x]==expected,"literal complete generic border/fill/clip grid");
    }
}
INT16 skin(bool gray=false,bool background=false){
    auto id=LoadGenericButtonImages(gray?"generic_on.sti":nullptr,"generic_off.sti","generic_on.sti","generic_on.sti","generic_on.sti",
        background?"generic_background.sti":nullptr,0,0,0);
    check(id>=0,"real nine-subimage skin load");return id;
}
void borderStates(){
    const auto image=skin();
    auto id=CreateTextButton(nullptr,0,0,-1,image,2,2,12,8,BUTTON_TOGGLE,31,nullptr,nullptr);
    check(id>=0,"generic textless creation");
    auto* b=GetButtonPtr(id);check((b->uiFlags&BUTTON_GENERIC)!=0,"generic type preserved");
    const std::vector<std::string> rows{"RRRGGGGGGBBB","RRRGGGGGGBBB","YYY......MMM","YYY......MMM","YYY......MMM","YYY......MMM","CCCWWWWWWaaa","CCCWWWWWWaaa"};
    clear();check(DrawButton(id),"nine-piece off draw");grid(pixels(),rows);
    b->uiFlags|=BUTTON_CLICKED_ON;clear();check(DrawButton(id),"pressed generic draw");grid(pixels(),rows,true);
    b->uiFlags&=~UINT32(BUTTON_CLICKED_ON);b->Area.uiFlags|=MSYS_MOUSE_IN_AREA;
    clear();check(DrawButton(id),"hover generic draw");grid(pixels(),rows,true);
    DisableHilightsAndHelpText();clear();check(DrawButton(id),"hilite switch off");grid(pixels(),rows);EnableHilightsAndHelpText();
    b->Area.uiFlags&=~UINT32(MSYS_MOUSE_IN_AREA);
    SGPRect clip{4,3,12,8};SetClippingRect(&clip);clear();check(DrawButton(id),"partial generic clip");grid(pixels(),rows,false,clip);
    SGPRect after{};GetClippingRect(&after);check(after.iLeft==4&&after.iTop==3&&after.iRight==12&&after.iBottom==8,"global clip unchanged");
    SGPRect all{0,0,32,24};SetClippingRect(&all);
    UINT32 pitch=0;check(LockVideoSurface(target,&pitch)!=nullptr,"caller lock");check(!DrawButton(id),"generic draw rejects caller lock");UnLockVideoSurface(target);
    check(!UnloadGenericButtonImage(image),"referenced generic skin unload rejected");
    RemoveButton(id);check(UnloadGenericButtonImage(image),"last generic release frees real objects");
    check(!UnloadGenericButtonImage(image),"repeated unload rejected");
    const auto explicitGray=skin(true);id=CreateTextButton(nullptr,0,0,-1,explicitGray,2,2,12,8,0,31,nullptr,nullptr);
    check(DisableButton(id),"disable generic with explicit gray");SpecifyDisabledButtonStyle(id,DISABLED_STYLE_HATCHED);
    clear();check(DrawButton(id),"explicit generic gray suppresses hatch");grid(pixels(),rows,true);
    RemoveButton(id);check(UnloadGenericButtonImage(explicitGray),"gray skin cleanup");
}
void remainderAndBackground(){
    auto image=skin();auto id=CreateTextButton(nullptr,0,0,-1,image,2,2,13,9,0,31,nullptr,nullptr);
    clear();check(DrawButton(id),"3x2 remainder overlap rendering");
    grid(pixels(),{"RRRGGGGGGGBBB","RRRGGGGGGGBBB","YYY.......MMM","YYY.......MMM","YYY.......MMM","YYY.......MMM","YYY.......MMM","CCCWWWWWWWaaa","CCCWWWWWWWaaa"});
    RemoveButton(id);check(UnloadGenericButtonImage(image),"remainder cleanup");
    image=skin(false,true);id=CreateTextButton(nullptr,0,0,-1,image,2,2,12,8,0,31,nullptr,nullptr);
    clear();check(DrawButton(id),"tiled image generic background");
    grid(pixels(),{"RRRGGGGGGBBB","RRRGGGGGGBBB","YYYBRBRBRMMM","YYYRBRBRBMMM","YYYBRBRBRMMM","YYYRBRBRBMMM","CCCWWWWWWaaa","CCCWWWWWWaaa"});
    auto* b=GetButtonPtr(id);b->uiFlags|=BUTTON_CLICKED_ON;
    clear();check(DrawButton(id),"pressed background tile phase shift");
    // Both axes shift by one, preserving this authored checker parity.
    grid(pixels(),{"WWWWWWWWWWWW","WWWWWWWWWWWW","WWWBRBRBRWWW","WWWRBRBRBWWW","WWWBRBRBRWWW","WWWRBRBRBWWW","WWWWWWWWWWWW","WWWWWWWWWWWW"});
    RemoveButton(id);check(UnloadGenericButtonImage(image),"background cleanup");
}
INT16 removalSkin=-1;
unsigned removalCalls=0;
bool pendingSkinRetained=false;
void removing(GUI_BUTTON* b,INT32 reason){
    if(!(reason&MSYS_CALLBACK_REASON_LBUTTON_DWN))return;
    ++removalCalls;RemoveButton(b->IDNum);
    pendingSkinRetained=!UnloadGenericButtonImage(removalSkin);
}
void eventLifetime(){
    removalSkin=skin();
    const auto id=CreateTextButton(nullptr,0,0,-1,removalSkin,2,2,12,8,0,31,nullptr,removing);
    check(id>=0,"generic callback button");
    QuickButtonCallbackMButn(&GetButtonPtr(id)->Area,MSYS_CALLBACK_REASON_LBUTTON_DWN);
    check(removalCalls==1&&GetButtonPtr(id)==nullptr,"self-removing callback defers free through dispatch");
    check(pendingSkinRetained,"callback-pending button retains referenced skin");
    check(UnloadGenericButtonImage(removalSkin),"post-callback generic image release");
    // Default -1 is a real resource-backed load with the original filenames.
    const auto automatic=CreateTextButton(nullptr,0,0,-1,-1,2,2,12,8,0,31,nullptr,nullptr);
    check(automatic>=0,"lazy original-named default skin admission");clear();check(DrawButton(automatic),"default skin genuine drawing");
    grid(pixels(),{"RRRGGGGGGBBB","RRRGGGGGGBBB","YYY......MMM","YYY......MMM","YYY......MMM","YYY......MMM","CCCWWWWWWaaa","CCCWWWWWWaaa"});
    RemoveButton(automatic);check(UnloadGenericButtonImage(0),"default skin lifetime");
}
std::array<UINT32,12> soundTrace{};
unsigned soundCount=0;
INT32 soundRemoves=-1;
UINT32 tracedSound(UINT32 sample,UINT32,UINT32,UINT32,UINT32){
    if(soundCount<soundTrace.size())soundTrace[soundCount++]=sample;
    if(soundRemoves>=0)RemoveButton(soundRemoves);
    return 17;
}
void noClick(GUI_BUTTON*,INT32){}
void anchorSounds(){
    NativeButtonSoundServices services{};services.playSample=tracedSound;services.samples[0]={101,102};
    NativeBindButtonSoundServices(services);
    const auto image=skin();
    auto id=CreateTextButton(nullptr,0,0,-1,image,2,2,12,8,BUTTON_NO_TOGGLE,31,DEFAULT_MOVE_CALLBACK,noClick);
    check(id>=0,"anchored generic sound button");SpecifyButtonSoundScheme(id,BUTTON_SOUND_SCHEME_VERYSMALLSWITCH1);
    auto* b=GetButtonPtr(id);soundCount=0;
    QuickButtonCallbackMButn(&b->Area,MSYS_CALLBACK_REASON_LBUTTON_DWN);
    QuickButtonCallbackMMove(&b->Area,MSYS_CALLBACK_REASON_LOST_MOUSE);
    check(!(b->uiFlags&BUTTON_CLICKED_ON),"anchored leave restores initially-up visual");
    QuickButtonCallbackMMove(&b->Area,MSYS_CALLBACK_REASON_GAIN_MOUSE);
    check((b->uiFlags&BUTTON_CLICKED_ON)!=0,"anchored reentry restores press visual");
    MSYS_CurrentMX=3;MSYS_CurrentMY=3;ReleaseAnchorMode();
    QuickButtonCallbackMButn(&b->Area,MSYS_CALLBACK_REASON_LBUTTON_UP);
    check(soundCount==4&&soundTrace[0]==101&&soundTrace[1]==102&&soundTrace[2]==101&&soundTrace[3]==102,
        "original anchored sound trace ON OFF ON OFF");
    b->uiFlags|=BUTTON_CLICKED_ON;soundCount=0;
    QuickButtonCallbackMButn(&b->Area,MSYS_CALLBACK_REASON_LBUTTON_DWN);
    QuickButtonCallbackMMove(&b->Area,MSYS_CALLBACK_REASON_LOST_MOUSE);
    check((b->uiFlags&BUTTON_CLICKED_ON)!=0&&soundCount==0,"initially-down anchor leave retains state and stays silent");
    QuickButtonCallbackMMove(&b->Area,MSYS_CALLBACK_REASON_GAIN_MOUSE);
    check(soundCount==1&&soundTrace[0]==101,"source initially-down reentry still emits ON");
    ReleaseAnchorMode();RemoveButton(id);
    // External playback removes the anchored button during the leave hook.
    id=CreateTextButton(nullptr,0,0,-1,image,2,2,12,8,BUTTON_NO_TOGGLE,31,DEFAULT_MOVE_CALLBACK,noClick);
    SpecifyButtonSoundScheme(id,BUTTON_SOUND_SCHEME_VERYSMALLSWITCH1);b=GetButtonPtr(id);
    QuickButtonCallbackMButn(&b->Area,MSYS_CALLBACK_REASON_LBUTTON_DWN);
    soundRemoves=id;QuickButtonCallbackMMove(&b->Area,MSYS_CALLBACK_REASON_LOST_MOUSE);soundRemoves=-1;
    check(GetButtonPtr(id)==nullptr,"anchor-leave playback self-removal frees only after callback");
    check(UnloadGenericButtonImage(image),"anchor playback removal releases skin lifetime");
    NativeBindButtonSoundServices({});
}
void helpDone(){}
void textIcons(INT32 font){
    const auto image=skin();const auto icon=LoadGenericButtonIcon("generic_icon.sti");check(icon>=0,"real icon load");
    auto iconButton=CreateIconButton(icon,0,image,2,2,12,8,0,31,nullptr,nullptr);
    check(iconButton>=0,"genuine iconic generic creation");
    check(SpecifyButtonIcon(iconButton,icon,0,5,3,FALSE),"explicit authored icon coordinates");
    clear();check(DrawButton(iconButton),"icon nine-piece composition");
    grid(pixels(),{"RRRGGGGGGBBB","RRRGGGGGGBBB","YYY......MMM","YYY...RRRMMM","YYY...R.RMMM","YYY......MMM","CCCWWWWWWaaa","CCCWWWWWWaaa"});
    RemoveButton(iconButton);
    wchar_t text[]=L"A B";
    auto id=CreateTextButton(text,font,2,0,image,2,2,24,18,0,31,nullptr,nullptr);check(id>=0,"generic font/text creation");
    text[0]=L'B';check(GetButtonPtr(id)->string[0]==L'A',"text ownership copy");
    check(SpecifyButtonIcon(id,icon,0,5,5,FALSE),"generic icon composition binding");
    check(GetButtonPtr(id)->fShiftImage,"source ignores false shift parameter");
    check(!UnloadGenericButtonIcon(icon),"referenced icon unload rejected");
    SetBtnHelpEndCallback(id,helpDone);check(GetButtonPtr(id)->Area.HelpDoneCallback==helpDone,"real MSYS help callback attached");
    SpecifyButtonTextOffsets(id,5,3,FALSE);SpecifyButtonTextJustification(id,BUTTON_TEXT_LEFT);SpecifyButtonTextWrappedWidth(id,4);
    clear();check(DrawButton(id),"actual wrapped native font and icon draw");const auto p=pixels();
    // A glyph starts at (7,5), with authored offset +1, and its foreground
    // top-center pixel at (9,5). B is on next baseline (font height3+gap1),
    // authored B offsetY+1 gives first foreground at (7,10).
    check(p[5*32+9]==0x07e0,"wrapped A uses real native font foreground");
    check(p[10*32+7]==0x07e0,"word-space break places B on next real font row");
    check(p[8*32+8]==0xf800,"icon under text uses actual object palette");
    SpecifyButtonTextWrappedWidth(id,0);check(GetButtonPtr(id)->sWrappedWidth==4,"invalid zero wrap leaves previous width");
    SpecifyButtonTextWrappedWidth(id,-1);check(GetButtonPtr(id)->sWrappedWidth==-1,"wrap reset");
    check(!SpecifyButtonIcon(id,icon,99,0,0,TRUE),"invalid frame rejected");
    check(!SpecifyButtonIcon(id,-1,0,0,0,FALSE)&&GetButtonPtr(id)->iIconID==-1,"source icon removal false return");
    check(UnloadGenericButtonIcon(icon),"unbound icon frees actual object");
    RemoveButton(id);check(UnloadGenericButtonImage(image),"text ownership release");
}
void invalid(){
    check(LoadGenericButtonImages(nullptr,nullptr,nullptr,"generic_on.sti",nullptr,nullptr,0,0,0)==-1,"missing mandatory skin filename");
    check(LoadGenericButtonImages(nullptr,"generic_short.sti",nullptr,"generic_on.sti",nullptr,nullptr,0,0,0)==-1,"reject missing ninth fill frame");
    check(LoadGenericButtonImages(nullptr,"generic_off.sti",nullptr,"missing.sti",nullptr,nullptr,0,0,0)==-1,"partial load rollback");
    const auto image=skin();check(image==0,"failed loads leave slot reusable");
    auto id=CreateTextButton(nullptr,0,0,-1,image,2,2,1,1,0,31,nullptr,nullptr);
    check(id>=0&&GetButtonWidth(id)==4&&GetButtonHeight(id)==3,"source minimum generic dimensions");RemoveButton(id);
    check(CreateTextButton(nullptr,0,0,-1,image,2,2,12,8,BUTTON_SAVEBACKGROUND,31,nullptr,nullptr)==-1,"unsupported saved-background request rejects honestly");
    check(CreateTextButton(nullptr,0,0,-1,39,2,2,12,8,0,31,nullptr,nullptr)==-1,"unloaded nondefault skin rejects honestly");
    check(UnloadGenericButtonImage(image),"invalid-case cleanup");
}
void cleanup(){ShutdownButtonSystem();MSYS_Shutdown();ShutdownFontManager();ShutdownVideoSurfaceManager();ShutdownVideoObjectManager();fileman::CloseAll();vfs::Log::flushDeleteAll();vfs::CVirtualFileSystem::shutdownVFS();vfs::ObjectAllocator::clear();}
}
int main(int argc,char** argv){try{
    if(argc!=2)throw std::runtime_error("expected authored generic fixture root");
    const auto root=std::filesystem::canonical(argv[1]);auto directory=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
    check(directory->init(),"fixture directory");auto profile=std::make_unique<vfs::CVirtualProfile>("generic-buttons",vfs::Path(root.u8string()),false);
    profile->addLocation(directory.get());auto* location=directory.release();auto* mounted=profile.get();getVFS()->getProfileStack()->pushProfile(profile.release());
    check(getVFS()->addLocation(location,mounted),"fixture VFS mount");check(InitializeVideoObjectManager()&&InitializeVideoSurfaceManager(),"real resource managers");
    UINT16 symbols[]{L'A',L'B',L' '};FontTranslationTable table{3,symbols};check(InitializeFontManager(16,&table),"real font manager");
    const auto font=LoadFontFile("generic_font.sti");check(font>=0,"authored font load");MSYS_Init();check(InitButtonSystem(),"buttons init");
    VSURFACE_DESC d{};d.fCreateFlags=VSURFACE_CREATE_DEFAULT;d.usWidth=32;d.usHeight=24;d.ubBitDepth=16;
    check(AddVideoSurface(&d,&target)&&SetButtonDestBuffer(target),"real offscreen RGB565 destination");SGPRect all{0,0,32,24};SetClippingRect(&all);
    borderStates();remainderAndBackground();textIcons(font);invalid();eventLifetime();anchorSounds();
    cleanup();std::cout<<"PASS: "<<checks<<" generic nine-piece/icon/wrapped-font assertions; authored data only\n";return 0;
}catch(const std::exception& e){cleanup();std::cerr<<"FAIL after "<<checks<<": "<<e.what()<<'\n';return 1;}}
