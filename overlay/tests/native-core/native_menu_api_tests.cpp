/*
 * P710 native snapshot update: tests/native-core/native_menu_api_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native change 2026-11-13. Original Button System.cpp/DrawTextOnButton and
// vsurface.cpp/InternalShadowVideoSurfaceRect contracts; authored fixtures only.
#include "native_buttons.h"
#include "native_font.h"
#include "clip_state.h"
#include "shading.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>
namespace {
unsigned checks=0;
void check(bool ok,const char* label) { ++checks; if(!ok) throw std::runtime_error(label); }
UINT32 surface(unsigned w,unsigned h) {
    VSURFACE_DESC d{}; d.fCreateFlags=VSURFACE_CREATE_DEFAULT; d.usWidth=w; d.usHeight=h; d.ubBitDepth=16;
    UINT32 id=0; check(AddVideoSurface(&d,&id),"create padded RGB565 surface"); return id;
}
struct Pixels { UINT32 pitch; unsigned w,h; std::vector<BYTE> bytes; };
Pixels read(UINT32 id) {
    UINT16 w,h; UINT8 depth; check(GetVideoSurfaceDescription(id,&w,&h,&depth),"read dimensions");
    UINT32 pitch=0; BYTE* p=LockVideoSurface(id,&pitch); check(p!=nullptr,"read exclusive lock");
    Pixels out{pitch,w,h,{p,p+std::size_t(pitch)*h}}; UnLockVideoSurface(id); return out;
}
UINT16 pixel(const Pixels& p,unsigned x,unsigned y) {
    UINT16 c=0; std::memcpy(&c,p.bytes.data()+y*p.pitch+x*2,2); return c;
}
void padding(const Pixels& p) {
    for(unsigned y=0;y<p.h;++y) for(unsigned x=p.w*2;x<p.pitch;++x)
        check(p.bytes[y*p.pitch+x]==0xa5,"row padding preserved");
}
bool same(const sgp::NativeFontState& a,const sgp::NativeFontState& b) {
    return a.font==b.font && a.generation==b.generation && a.dest==b.dest && a.pitch==b.pitch && a.bpp==b.bpp &&
        a.wrap==b.wrap && a.foreground==b.foreground && a.background==b.background && a.shadow==b.shadow &&
        a.region.iLeft==b.region.iLeft && a.region.iTop==b.region.iTop && a.region.iRight==b.region.iRight && a.region.iBottom==b.region.iBottom;
}
void shadows(UINT32 target) {
    SGPRect all{0,0,32,24}; SetClippingRect(&all); BuildShadeTable(); BuildIntensityTable();
    check(ShadeTable[0xffff]==0x7bcf && IntensityTable[0xffff]==0xce59,"source 0.48 and 0.80 quantized table values");
    check(ColorFillVideoSurfaceArea(target,0,0,32,24,0xffff),"shadow clear");
    check(ShadowVideoSurfaceRect(target,2,3,4,5),"normal inclusive shadow"); auto p=read(target);
    for(unsigned y=0;y<24;++y) for(unsigned x=0;x<32;++x)
        check(pixel(p,x,y)==((x>=2&&x<=4&&y>=3&&y<=5)?0x7bcf:0xffff),"literal normal shade rectangle endpoints");
    padding(p);
    check(!ShadowVideoSurfaceRect(target,2,3,2,5) && !ShadowVideoSurfaceRect(target,2,3,4,3),"source rejects zero spans");
    UINT32 pitch=0; check(LockVideoSurface(target,&pitch)!=nullptr,"lock before rejected shadow");
    check(!ShadowVideoSurfaceRect(target,0,0,4,5),"shadow rejects caller-owned lock"); UnLockVideoSurface(target);
    check(read(target).bytes==p.bytes,"rejected shadow atomic");
    SGPRect clip{1,1,3,4}; SetClippingRect(&clip);
    check(ShadowVideoSurfaceRect(target,-20,-20,100,100),"surface clamp and global half-open clip"); auto q=read(target);
    for(unsigned y=0;y<24;++y) for(unsigned x=0;x<32;++x) {
        UINT16 expected=pixel(p,x,y);
        if(x>=1&&x<3&&y>=1&&y<4) expected=expected==0xffff?0x7bcf:0x39c7;
        check(pixel(q,x,y)==expected,"shadow clipped complete RGB565 allocation");
    }
    padding(q); SetClippingRect(&all);
    check(ColorFillVideoSurfaceArea(target,0,0,32,24,0xffff) && ShadowVideoSurfaceRectUsingLowPercentTable(target,2,3,4,5),"alternate table retained");
    check(pixel(read(target),4,5)==0xce59,"alternate literal intensity value");
}
void imageShadow(UINT32 target,HVOBJECT image) {
    SGPRect all{0,0,32,24}; SetClippingRect(&all);
    check(ColorFillVideoSurfaceArea(target,0,0,32,24,0xffff),"image shadow clear");
    check(ShadowVideoSurfaceImage(target,image,2,2),"source image drop-shadow strips"); auto p=read(target);
    for(unsigned y=0;y<24;++y) for(unsigned x=0;x<32;++x) {
        const bool horizontal=x>=5&&x<=10&&y>=8&&y<=11;
        const bool vertical=x>=10&&x<=13&&y>=5&&y<=8;
        check(pixel(p,x,y)==(horizontal&&vertical?0x39c7:horizontal||vertical?0x7bcf:0xffff),"literal two-strip shadow including double-shaded shared corner");
    }
    padding(p);
    UINT32 pitch=0; check(LockVideoSurface(target,&pitch)!=nullptr,"lock before image shadow rejection");
    check(!ShadowVideoSurfaceImage(target,image,2,2),"image shadow rejects caller-owned lock"); UnLockVideoSurface(target);
    check(read(target).bytes==p.bytes,"locked image shadow atomic");
    check(!ShadowVideoSurfaceImage(target,nullptr,2,2) && !ShadowVideoSurfaceImage(0,image,2,2),"invalid image and target rejected");
    check(!ShadowVideoSurfaceImage(target,image,std::numeric_limits<INT32>::max(),0),"image coordinate overflow rejected");
    check(read(target).bytes==p.bytes,"invalid image shadow no pixel writes");
    check(ColorFillVideoSurfaceArea(target,0,0,32,24,0xffff),"negative image shadow clear");
    SGPRect clip{2,1,6,5}; SetClippingRect(&clip);
    // A descriptor view borrows the actual fixture's pixels and palette, but has
    // nonzero offsets. This API intentionally uses only width/height.
    ETRLEObject first=image->pETRLEObject[0]; first.sOffsetX=19; first.sOffsetY=-11;
    SGPVObject offsetView=*image; offsetView.pETRLEObject=&first; offsetView.usNumberOfObjects=1;
    check(ShadowVideoSurfaceImage(target,&offsetView,-4,-4),"image shadow clips negative coordinates and ignores image offsets"); p=read(target);
    for(unsigned y=0;y<24;++y) for(unsigned x=0;x<32;++x) {
        const bool horizontal=x>=2&&x<=4&&y>=2&&y<5;
        const bool vertical=x>=4&&x<6&&y>=1&&y<=2;
        check(pixel(p,x,y)==(horizontal&&vertical?0x39c7:horizontal||vertical?0x7bcf:0xffff),"literal clipped shadow strips and half-open global clip");
    }
    padding(p); SetClippingRect(&all);
    check(ShadowVideoSurfaceImage(target,image,100,100) && read(target).bytes==p.bytes,"source successful offscreen image-shadow no-op");
}

void disabledOverlays(UINT32 target,INT32 image,INT32 font) {
    const INT32 noGray=UseLoadedButtonImage(image,-1,0,1,2,3); check(noGray>=0,"disabled overlay image without gray frame");
    const INT32 id=QuickCreateButton(noGray,2,2,BUTTON_TOGGLE,31,nullptr,nullptr);
    check(id>=0 && DisableButton(id),"disabled quick button for overlays");
    SGPRect all{0,0,32,24}; SetClippingRect(&all);
    auto draw=[&] { check(ColorFillVideoSurfaceArea(target,0,0,32,24,0xffff),"disabled overlay target clear"); check(DrawButton(id),"actual disabled overlay draw"); return read(target); };
    for (INT8 style : {INT8(DISABLED_STYLE_NONE),INT8(DISABLED_STYLE_HATCHED),INT8(DISABLED_STYLE_SHADED),INT8(DISABLED_STYLE_DEFAULT)}) {
        SpecifyDisabledButtonStyle(id,style); auto p=draw();
        for(unsigned y=0;y<24;++y) for(unsigned x=0;x<32;++x) {
            UINT16 expected=0xffff;
            if (x>=2&&x<10&&y>=2&&y<8) {
                expected=(x>2&&x<9&&y>2&&y<7)?0x001f:0xffff;
                if (style==DISABLED_STYLE_SHADED) expected=expected==0xffff?0x7bcf:0x000f;
                if ((style==DISABLED_STYLE_HATCHED || style==DISABLED_STYLE_DEFAULT) && (x==2||((x-2+y-2)&1)==0)) expected=0;
            }
            check(pixel(p,x,y)==expected,"literal NONE/HATCHED/SHADED/no-text DEFAULT output");
        }
        padding(p);
    }
    SpecifyDisabledButtonStyle(id,DISABLED_STYLE_HATCHED);
    check(ColorFillVideoSurfaceArea(target,0,0,32,24,0xffff),"clipped hatch clear");
    SGPRect clip{3,2,9,7}; SetClippingRect(&clip); check(DrawButton(id),"clipped hatch"); auto p=read(target);
    for(unsigned y=0;y<24;++y) for(unsigned x=0;x<32;++x) {
        UINT16 expected=0xffff;
        if (x>=3&&x<9&&y>=2&&y<7) {
            expected=y>2?0x001f:0xffff;
            if(x==3||((x-3+y-2)&1)==0) expected=0;
        }
        check(pixel(p,x,y)==expected,"hatch phase starts at clipped origin with source always-filled first column");
    }
    padding(p); SetClippingRect(&all); RemoveButton(id);
    const INT32 gray=QuickCreateButton(image,2,2,BUTTON_TOGGLE,31,nullptr,nullptr);
    check(gray>=0 && DisableButton(gray),"explicit gray disabled button"); SpecifyDisabledButtonStyle(gray,DISABLED_STYLE_SHADED);
    check(ColorFillVideoSurfaceArea(target,0,0,32,24,0xffff) && DrawButton(gray),"gray-frame precedence draw"); p=read(target);
    check(pixel(p,3,3)==0x8410 && pixel(p,3,2)==0xffff,"explicit gray image suppresses shade overlay"); RemoveButton(gray);
    wchar_t label[]=L"A";
    const INT32 text=CreateIconAndTextButton(noGray,label,font,2,1,-1,-1,TEXT_CJUSTIFIED,2,2,BUTTON_TOGGLE,31,nullptr,nullptr);
    auto* b=GetButtonPtr(text); check(b!=nullptr,"disabled text button"); b->Area.RegionBottomRightX=26; b->Area.RegionBottomRightY=16;
    check(DisableButton(text),"disable text for default shade");
    check(ColorFillVideoSurfaceArea(target,0,0,32,24,0xffff) && DrawButton(text),"text default shade draw"); p=read(target);
    check(pixel(p,14,8)==0x03c0 && pixel(p,14,9)==0x7800,"DEFAULT shades real foreground and glyph shadow after text draw");
    SpecifyDisabledButtonStyle(text,DISABLED_STYLE_NONE);
    check(ColorFillVideoSurfaceArea(target,0,0,32,24,0xffff) && DrawButton(text),"disabled text NONE draw"); p=read(target);
    check(pixel(p,14,8)==0x07e0 && pixel(p,14,9)==0xf800,"NONE leaves disabled text unshaded");
    RemoveButton(text); UnloadButtonImage(noGray);
}

void buttons(UINT32 target,INT32 font) {
    SGPRect all{0,0,32,24}; SetClippingRect(&all);
    const INT32 image=LoadButtonImage("buttons.sti",4,0,1,2,3); check(image>=0,"load authored button images");
    imageShadow(target,ButtonPictures[image].vobj);
    check(SetButtonDestBuffer(target),"overlay button destination"); disabledOverlays(target,image,font);
    const INT32 alias=UseLoadedButtonImage(image,4,0,1,2,3); check(alias>=0,"share authored object");
    wchar_t genericText[]=L"A";
    check(CreateIconAndTextButton(image,genericText,font,2,1,3,1,0,0,0,BUTTON_GENERIC,31,nullptr,nullptr)==-1,"valid image generic creation explicitly unsupported");
    wchar_t callerText[]=L"A";
    const INT32 id=CreateIconAndTextButton(image,callerText,font,2,1,3,1,TEXT_CJUSTIFIED,2,2,BUTTON_TOGGLE,31,nullptr,nullptr);
    GUI_BUTTON* b=GetButtonPtr(id); check(b && b->string!=callerText && std::wcscmp(b->string,L"A")==0,"creation owns string");
    callerText[0]=L'B'; check(b->string[0]==L'A',"caller text lifetime isolated");
    check(DISABLED_STYLE_NONE==0 && DISABLED_STYLE_DEFAULT==1 && DISABLED_STYLE_HATCHED==2 && DISABLED_STYLE_SHADED==3 && b->bDisabledStyle==DISABLED_STYLE_DEFAULT,"original disabled-style enum and creation default");
    SpecifyDisabledButtonStyle(id,DISABLED_STYLE_NONE); check(b->bDisabledStyle==DISABLED_STYLE_NONE,"valid NONE disabled style state");
    SpecifyDisabledButtonStyle(id,99); check(b->bDisabledStyle==DISABLED_STYLE_NONE,"invalid disabled style leaves prior state");
    // The original public GUI_BUTTON region defines text layout independently
    // of its bitmap descriptor. Enlarge those public logical bounds to make
    // full glyph alignment observable with the existing 8x6 authored image.
    b->Area.RegionBottomRightX=26; b->Area.RegionBottomRightY=16;
    check(SetButtonDestBuffer(target),"button destination");
    const UINT32 callerDest=surface(7,5);
    char otherFilename[]="font.sti"; const INT32 callerFont=LoadFontFile(otherFilename);
    check(callerFont>=0 && callerFont!=font,"separate caller font binding");
    check(SetFont(callerFont) && SetFontDestBuffer(callerDest,1,1,6,4,TRUE),"caller font destination");
    SetRGBFontForeground(UINT16(0x1234)); SetRGBFontShadow(UINT16(0x4321)); SetRGBFontBackground(0,0,0);
    SaveFontSettings(); const auto before=sgp::CaptureNativeFontState();
    auto draw=[&] { check(ColorFillVideoSurfaceArea(target,0,0,32,24,0x2104),"clear text target"); check(DrawButton(id),"quick bitmap plus actual font draw"); check(same(before,sgp::CaptureNativeFontState()),"font and destination restored exactly"); return read(target); };
    auto p=draw();
    check(pixel(p,14,8)==0x07e0 && pixel(p,14,9)==0xf800 && pixel(p,13,9)==0x07e0,"centered authored A glyph and shadow"); padding(p);
    b->uiFlags|=BUTTON_CLICKED_ON; p=draw();
    check(pixel(p,15,9)==0x001f && pixel(p,15,10)==0xf800,"pressed color and one-pixel text shift");
    b->Area.uiFlags|=MSYS_MOUSE_IN_AREA; SpecifyButtonHilitedTextColors(id,1,2); gfRenderHilights=FALSE; p=draw();
    check(pixel(p,15,9)==0xf800 && pixel(p,15,10)==0x07e0,"source text hover precedes down even when image hilites disabled");
    DisableButton(id); p=draw(); check(pixel(p,15,9)==0x001f,"disabled text falls back to pressed color"); EnableButton(id);
    b->uiFlags&=~UINT32(BUTTON_CLICKED_ON); b->Area.uiFlags&=~UINT32(MSYS_MOUSE_IN_AREA);
    SpecifyButtonTextJustification(id,TEXT_LJUSTIFIED); p=draw(); check(pixel(p,10,8)==0x07e0,"left justification source margin");
    SpecifyButtonTextJustification(id,TEXT_RJUSTIFIED); p=draw(); check(pixel(p,18,8)==0x07e0,"right justification source margin");
    SpecifyButtonTextJustification(id,99); check(b->bJustification==TEXT_CJUSTIFIED,"bad setter justification centers");
    SpecifyButtonTextOffsets(id,4,3,FALSE); SpecifyButtonTextSubOffsets(id,1,1,FALSE); p=draw();
    check(pixel(p,9,6)==0x07e0,"explicit plus sub offsets");
    SpecifyButtonUpTextColors(id,3,2); SpecifyButtonMultiColorFont(id,TRUE); p=draw(); check(b->fMultiColor && pixel(p,9,6)==0x001f && pixel(p,9,7)==0x07e0,"source stores multicolor flag but renders mprintf colors"); SpecifyButtonMultiColorFont(id,FALSE);
    b->uiFlags|=BUTTON_16BPPCOLOURTEXT; SpecifyButtonUpTextColors(id,0x7bef,0); p=draw();
    check(pixel(p,9,6)==0x7bef && pixel(p,9,7)==0x0001,"source direct RGB565 text and minimum shadow");
    b->uiFlags&=~UINT32(BUTTON_16BPPCOLOURTEXT);
    SpecifyButtonText(id,b->string); check(std::wcscmp(b->string,L"A")==0,"self rebind copies before free");
    wchar_t more[]=L"BA"; SpecifyButtonText(id,more); SpecifyButtonText(id,b->string+1); check(std::wcscmp(b->string,L"A")==0,"substring rebind copies before free");
    wchar_t literal[]=L"A%"; SpecifyButtonText(id,literal); check(DrawButton(id),"percent text rendered literally");
    SpecifyButtonText(id,nullptr); check(!b->string && (b->uiFlags&BUTTON_DIRTY),"text removal dirties bitmap");
    check(SetButtonCursor(id,1234) && b->Area.Cursor==1234 && !SetButtonCursor(-1,1),"cursor updates retained mouse region");
    check(SpecifyButtonImage(id,alias) && b->ImageNum==UINT32(alias),"image rebind");
    UnloadButtonImage(image); check(!ButtonPictures[image].vobj && ButtonPictures[alias].vobj,"rebind releases old reference and transfers object ownership");
    check(!SpecifyButtonImage(id,9999),"invalid image rejected");
    SpecifyFullButtonTextAttributes(id,more,font,2,1,3,1,TEXT_LJUSTIFIED); check(b->string && b->bJustification==TEXT_LJUSTIFIED,"full text attrs");
    RemoveTextFromButton(id); check(!b->string && b->bTextXOffset==-1 && b->fShiftText && b->sForeColorDown==-1,"source remove text defaults");
    SpecifyGeneralButtonTextAttributes(id,more,font,3,0); check(b->sForeColor==3 && b->string,"general text attrs");
    UINT32 pitch=0; check(LockVideoSurface(target,&pitch)!=nullptr,"lock target before draw failure"); check(!DrawButton(id),"locked target rejects text button"); UnLockVideoSurface(target);
    check(same(before,sgp::CaptureNativeFontState()),"failed draw state isolation");
    SetRGBFontForeground(UINT16(0xffff)); RestoreFontSettings(); check(same(before,sgp::CaptureNativeFontState()),"legacy saved slot survives nested button draws");
    RemoveButton(id); check(!GetButtonPtr(id),"text owning button removed"); UnloadButtonImage(alias);
    check(CreateIconAndTextButton(-1,more,font,2,1,3,1,0,0,0,0,31,nullptr,nullptr)==-1,"invalid image creation");
    UnloadFont(static_cast<UINT32>(callerFont));
    check(DeleteVideoSurfaceFromIndex(callerDest),"caller destination cleanup");
}
void cleanup() {
    ShutdownButtonSystem(); MSYS_Shutdown(); ShutdownFontManager(); ShutdownVideoSurfaceManager(); ShutdownVideoObjectManager();
    fileman::CloseAll(); vfs::Log::flushDeleteAll(); vfs::CVirtualFileSystem::shutdownVFS(); vfs::ObjectAllocator::clear();
}
}
int main(int argc,char** argv) {
    try {
        if(argc!=2) throw std::runtime_error("expected generate_button_fixtures.py fixtureRoot");
        const auto root=std::filesystem::canonical(argv[1]);
        auto directory=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string())); check(directory->init(),"fixture directory");
        auto profile=std::make_unique<vfs::CVirtualProfile>("menu-api",vfs::Path(root.u8string()),false); profile->addLocation(directory.get());
        auto* location=directory.release(); auto* mounted=profile.get(); getVFS()->getProfileStack()->pushProfile(profile.release()); check(getVFS()->addLocation(location,mounted),"fixture VFS mount");
        check(InitializeVideoObjectManager() && InitializeVideoSurfaceManager(),"actual resource managers");
        UINT16 symbols[]{L'A',L'B',L' '}; FontTranslationTable table{3,symbols}; check(InitializeFontManager(16,&table),"actual font manager");
        char filename[]="font.sti"; const INT32 font=LoadFontFile(filename); check(font>=0,"load authored font");
        MSYS_Init(); check(InitButtonSystem(),"actual button manager"); const auto target=surface(32,24);
        shadows(target); buttons(target,font); cleanup(); std::cout<<"PASS: "<<checks<<" native menu API assertions\n"; return 0;
    } catch(const std::exception& e) { cleanup(); std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<'\n'; return 1; }
}
