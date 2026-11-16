/*
 * P710 native snapshot update: sgp/NativeButtons.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: sgp/NativeButtons.cpp
 * Native Linux source/build/test adaptations captured in the accepted magazine and UI cleanup checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native adaptation, 2026-11-12/13: bounded original quick/checkbox behavior.
// Reference: 1dot13/source at3394695a387a14c78e92b158cfdcf50e4068c96b,
// sgp/Button System.cpp/.h. Keeps original field order, flags, anchor and null
// callback quirks; replaces unsafe Boolean deletion guard with depth/identity
// checks. Generic borders, icons and wrapped text use the same native
// resource/surface/font managers. Saved-background registration remains absent.
#include "native_buttons.h"
#include "native_surface.h"
#include "native_font.h"
#include "etrle_blitters.h"
#include "shading.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <cstdio>
#include <cwchar>
#include <memory>
#include <limits>
#include <new>
#include <string>

std::vector<GUI_BUTTON*> ButtonList;
std::vector<BUTTON_PICS> ButtonPictures;
UINT32 ButtonDestBuffer = FRAME_BUFFER;
BOOLEAN gfRenderHilights = TRUE;

namespace {
struct Slot {
    std::uint64_t generation = 0;
    std::uint64_t regionGeneration = 0;
    bool hidden = false;
    bool pressed = false;
    bool prepressOn = false;
};
std::array<Slot, MAX_BUTTONS> slots{};
std::array<bool, MAX_BUTTON_PICS> owned{};
std::array<bool, MAX_BUTTON_PICS> pendingUnload{};
struct GenericSkin {
    // Same order as quick-image states, then optional tiled background.
    std::array<HVOBJECT,6> images{};
    UINT16 fill=0, backgroundIndex=0;
    INT16 offsetX=0, offsetY=0;
};
std::array<GenericSkin,MAX_GENERIC_PICS> genericSkins{};
std::array<HVOBJECT,MAX_BUTTON_ICONS> genericIcons{};
bool genericValid(UINT32 id) {
    return id<genericSkins.size() && genericSkins[id].images[1] && genericSkins[id].images[3];
}
bool genericReferenced(UINT32 image) {
    for (GUI_BUTTON* b:ButtonList)
        if (b && (b->uiFlags & BUTTON_GENERIC) && b->ImageNum==image) return true;
    return false;
}
bool iconReferenced(INT16 icon) {
    for (GUI_BUTTON* b:ButtonList)
        if (b && (b->uiFlags & BUTTON_GENERIC) && b->iIconID==icon) return true;
    return false;
}
HVOBJECT loadObject(const char* filename) {
    if (!filename || !*filename || std::strlen(filename)>=SGPFILENAME_LEN) return nullptr;
    VOBJECT_DESC desc{}; desc.fCreateFlags=VOBJECT_CREATE_FROMFILE;
    std::memcpy(desc.ImageFile,filename,std::strlen(filename)+1);
    return CreateVideoObject(&desc);
}
bool validIndexed(HVOBJECT object, UINT16 index) {
    return object && object->ubBitDepth==8 && object->pETRLEObject && object->p16BPPPalette &&
        index<object->usNumberOfObjects && object->pETRLEObject[index].usWidth && object->pETRLEObject[index].usHeight;
}
bool validSkin(HVOBJECT object) {
    if (!validIndexed(object,8)) return false;
    for (UINT16 i=0;i<8;++i) if (!validIndexed(object,i)) return false;
    return true;
}
void releaseSkin(GenericSkin& skin) {
    for (auto& image:skin.images) if (image && DeleteVideoObject(image)) image=nullptr;
    if (std::none_of(skin.images.begin(),skin.images.end(),[](HVOBJECT p){return p!=nullptr;})) skin={};
}
bool initialized = false;
bool shutdownPending = false;
unsigned callbackDepth = 0;
std::uint64_t nextGeneration = 1;
struct Identity {
    INT32 id = -1;
    std::uint64_t generation = 0;
};
Identity anchor{}, previousAnchor{};
bool anchoredState = false;

GUI_BUTTON* live(INT32 id) {
    GUI_BUTTON* b = GetButtonPtr(id);
    return b && !(b->uiFlags & BUTTON_DELETION_PENDING) ? b : nullptr;
}
Identity identity(GUI_BUTTON* b) {
    // Do not dereference an untrusted or removed pointer to find its slot.
    for (INT32 id = 0; id < static_cast<INT32>(ButtonList.size()); ++id)
        if (ButtonList[static_cast<std::size_t>(id)] == b && live(id))
            return {id, slots[static_cast<std::size_t>(id)].generation};
    return {};
}
GUI_BUTTON* resolve(Identity i) {
    return i.id >= 0 && i.id < MAX_BUTTONS &&
        slots[static_cast<std::size_t>(i.id)].generation == i.generation ? live(i.id) : nullptr;
}
bool same(Identity a, Identity b) {
    return a.id >= 0 && a.id == b.id && a.generation == b.generation;
}
bool imageValid(UINT32 id) {
    return id < ButtonPictures.size() && ButtonPictures[id].vobj;
}
bool referenced(UINT32 image) {
    for (GUI_BUTTON* b : ButtonList)
        if (b && !(b->uiFlags & (BUTTON_HOT_SPOT|BUTTON_GENERIC)) && b->ImageNum == image) return true;
    return false;
}
void unload(INT32 image) {
    if (image < 0 || !imageValid(static_cast<UINT32>(image))) return;
    const auto index = static_cast<std::size_t>(image);
    if (referenced(static_cast<UINT32>(image))) {
        // Deletion is deferred inside mouse callbacks. Honor the caller's
        // matching unload after dispatch, once every referring button retires.
        // Keep the original rejection when a live button still uses the image.
        for (GUI_BUTTON* b:ButtonList)
            if (b && !(b->uiFlags & (BUTTON_HOT_SPOT|BUTTON_GENERIC|BUTTON_DELETION_PENDING)) &&
                b->ImageNum==static_cast<UINT32>(image)) return;
        pendingUnload[index]=true;
        return;
    }
    if (owned[index]) {
        // Like the original duplicate-image contract, ownership belongs to the
        // shared object, not its first state selection. Keep one owner until
        // every image slot has gone, including slots retained by live buttons.
        std::size_t successor = ButtonPictures.size();
        for (std::size_t other = 0; other < ButtonPictures.size(); ++other)
            if (other != index && ButtonPictures[other].vobj == ButtonPictures[index].vobj) {
                successor = other;
                break;
            }
        if (successor != ButtonPictures.size()) {
            owned[successor] = true;
            ButtonPictures[successor].fFlags &= ~1u; // GUI_BTN_DUPLICATE_VOBJ
        } else if (!DeleteVideoObject(ButtonPictures[index].vobj)) return;
    }
    ButtonPictures[index] = {};
    owned[index] = false;
    pendingUnload[index] = false;
}
void collect() {
    if (callbackDepth || sgp::NativeMouseDispatchActive()) return;
    for (std::size_t id = 0; id < ButtonList.size(); ++id) {
        GUI_BUTTON* b = ButtonList[id];
        if (!b || !(b->uiFlags & BUTTON_DELETION_PENDING)) continue;
        const bool autoImage = (b->uiFlags & BUTTON_SELFDELETE_IMAGE) != 0 && !(b->uiFlags & BUTTON_GENERIC);
        const UINT32 image = b->ImageNum;
        delete[] b->string;
        delete b;
        ButtonList[id] = nullptr;
        slots[id] = {};
        if (autoImage && image < MAX_BUTTON_PICS) unload(static_cast<INT32>(image));
    }
    for (INT32 image=0;image<static_cast<INT32>(ButtonPictures.size());++image)
        if (pendingUnload[static_cast<std::size_t>(image)]) unload(image);
    if (shutdownPending && std::none_of(ButtonList.begin(), ButtonList.end(), [](GUI_BUTTON* b) { return b != nullptr; })) {
        for (INT32 i = 0; i < static_cast<INT32>(ButtonPictures.size()); ++i) unload(i);
        for (auto& skin:genericSkins) releaseSkin(skin);
        for (auto& icon:genericIcons) if (icon && DeleteVideoObject(icon)) icon=nullptr;
        // DeleteVideoObject normally succeeds for owned objects. Retain a failed
        // deletion slot so a later shutdown can retry rather than leak ownership.
        bool objectsRemain = false;
        for (const auto& skin:genericSkins) for (auto image:skin.images) objectsRemain |= image!=nullptr;
        for (auto icon:genericIcons) objectsRemain |= icon!=nullptr;
        for (const auto& picture : ButtonPictures) objectsRemain |= picture.vobj != nullptr;
        if (!objectsRemain) {
            ButtonPictures.clear();
            ButtonList.clear();
            owned.fill(false);
            pendingUnload.fill(false);
            shutdownPending = false;
        }
    }
}
struct CallbackScope {
    CallbackScope() { ++callbackDepth; }
    ~CallbackScope() { --callbackDepth; collect(); }
};
GUI_BUTTON* fromRegion(MOUSE_REGION* r) {
    if (!r || !sgp::NativeMouseRegionGeneration(r)) return nullptr;
    const auto payload = MSYS_GetRegionUserData(r, 0);
    if (payload < 0 || payload >= MAX_BUTTONS) return nullptr;
    const INT32 id = static_cast<INT32>(payload);
    GUI_BUTTON* b = live(id);
    if (!b || &b->Area != r || slots[static_cast<std::size_t>(id)].hidden ||
        slots[static_cast<std::size_t>(id)].regionGeneration != sgp::NativeMouseRegionGeneration(r)) return nullptr;
    return b;
}
bool stillLive(Identity token, std::uint64_t regionGeneration) {
    GUI_BUTTON* b = resolve(token);
    return b && sgp::NativeMouseRegionGeneration(&b->Area) == regionGeneration &&
        slots[static_cast<std::size_t>(token.id)].regionGeneration == regionGeneration;
}
INT32 freeButton() {
    collect();
    if (!initialized || shutdownPending) return -1;
    for (INT32 i = 0; i < static_cast<INT32>(ButtonList.size()); ++i)
        if (!ButtonList[static_cast<std::size_t>(i)]) return i;
    return -1;
}
void setOn(GUI_BUTTON* b, bool on) {
    if (on) b->uiFlags |= BUTTON_CLICKED_ON;
    else b->uiFlags &= ~static_cast<UINT32>(BUTTON_CLICKED_ON);
    b->uiFlags |= BUTTON_DIRTY;
}
bool geometry(INT16 x, INT16 y, UINT32 width, UINT32 height) {
    return x >= 0 && y >= 0 && width <= static_cast<UINT32>(std::numeric_limits<INT16>::max() - x) &&
        height <= static_cast<UINT32>(std::numeric_limits<INT16>::max() - y);
}
INT32 install(UINT32 image, INT16 x, INT16 y, UINT32 width, UINT32 height,
              UINT32 type, INT16 priority, GUI_CALLBACK move, GUI_CALLBACK click) {
    if (!geometry(x, y, width, height) || priority < MSYS_PRIORITY_AUTO || priority > MSYS_PRIORITY_HIGHEST)
        return -1;
    const INT32 id = freeButton();
    if (id < 0 || nextGeneration == std::numeric_limits<std::uint64_t>::max()) return -1;
    GUI_BUTTON* b = new (std::nothrow) GUI_BUTTON{};
    if (!b) return -1;
    b->IDNum = id;
    b->ImageNum = image;
    b->XLoc = x;
    b->YLoc = y;
    b->ClickCallback = click;
    b->MoveCallback = move;
    b->Group = -1;
    b->bDisabledStyle=DISABLED_STYLE_DEFAULT;
    b->iIconID = -1;
    b->usIconIndex = -1;
    b->BackRect = -1;
    b->sWrappedWidth = -1;
    b->sShadowColor = b->sForeColorDown = b->sShadowColorDown = -1;
    b->sForeColorHilited = b->sShadowColorHilited = -1;
    b->bTextXOffset = b->bTextYOffset = b->bIconXOffset = b->bIconYOffset = -1;
    b->fShiftText = b->fShiftImage = TRUE;
    b->uiFlags = BUTTON_ENABLED | BUTTON_DIRTY | type |
        (move ? BUTTON_MOVE_CALLBACK : 0u) | (click ? BUTTON_CLICK_CALLBACK : 0u);
    auto& slot = slots[static_cast<std::size_t>(id)];
    slot = {};
    slot.generation = nextGeneration++;
    ButtonList[static_cast<std::size_t>(id)] = b;
    MSYS_DefineRegion(&b->Area, static_cast<UINT16>(x), static_cast<UINT16>(y),
        static_cast<UINT16>(static_cast<UINT32>(x) + width),
        static_cast<UINT16>(static_cast<UINT32>(y) + height), static_cast<INT8>(priority),
        MSYS_NO_CURSOR, QuickButtonCallbackMMove, QuickButtonCallbackMButn);
    slot.regionGeneration = sgp::NativeMouseRegionGeneration(&b->Area);
    if (!slot.regionGeneration) {
        ButtonList[static_cast<std::size_t>(id)] = nullptr;
        slot = {};
        delete b;
        return -1;
    }
    MSYS_SetRegionUserData(&b->Area, 0, id);
    SpecifyButtonSoundScheme(id,BUTTON_SOUND_SCHEME_GENERIC);
    return id;
}
INT32 addImage(HVOBJECT object, bool takeOwnership, INT32 gray, INT32 off,
               INT32 hover, INT32 on, INT32 onHover) {
    collect();
    if (!initialized || shutdownPending || !object || !object->pETRLEObject ||
        object->ubBitDepth != 8 || !object->usNumberOfObjects) return -1;
    const std::array<INT32, 5> states{gray, off, hover, on, onHover};
    UINT32 width = 0, height = 0;
    bool any = false;
    for (INT32 state : states) {
        if (state == -1) continue;
        if (state < 0 || state >= object->usNumberOfObjects) return -1;
        const ETRLEObject& frame = object->pETRLEObject[state];
        const INT32 right = static_cast<INT32>(frame.usWidth) + frame.sOffsetX;
        const INT32 bottom = static_cast<INT32>(frame.usHeight) + frame.sOffsetY;
        // Original dimensions are right/bottom extents from the button origin;
        // retain positive/negative source offsets, rejecting empty extents.
        if (!frame.usWidth || !frame.usHeight || right <= 0 || bottom <= 0 ||
            right > std::numeric_limits<INT16>::max() || bottom > std::numeric_limits<INT16>::max()) return -1;
        width = std::max(width, static_cast<UINT32>(right));
        height = std::max(height, static_cast<UINT32>(bottom));
        any = true;
    }
    if (!any) return -1;
    // Original rendering starts with frame 0 when a requested state is absent.
    // Validate that fallback too, before storing any public image slot.
    const ETRLEObject& fallback = object->pETRLEObject[0];
    if (!fallback.usWidth || !fallback.usHeight) return -1;
    for (INT32 i = 0; i < static_cast<INT32>(ButtonPictures.size()); ++i) {
        auto& picture = ButtonPictures[static_cast<std::size_t>(i)];
        if (picture.vobj) continue;
        picture = {object, gray, off, hover, on, onHover, width, height, takeOwnership ? 0u : 2u};
        owned[static_cast<std::size_t>(i)] = takeOwnership;
        return i;
    }
    return -1;
}
// Source DrawTextOnButton coordinates/colors, with isolated native font state.
// English word-wrap follows the source break/spacing rules without format-string
// interpretation or its fixed 1024-character scratch buffer.
BOOLEAN drawText(GUI_BUTTON* b, UINT16 width, UINT16 height) {
    if (!b->string) return TRUE;
    if (!IsFontLoaded(b->usFont) || b->sWrappedWidth==0 || b->sWrappedWidth < -1) return FALSE;
    SGPRect outer{}; GetClippingRect(&outer);
    const INT32 bw=b->Area.RegionBottomRightX-b->Area.RegionTopLeftX;
    const INT32 bh=b->Area.RegionBottomRightY-b->Area.RegionTopLeftY;
    const INT32 tx=b->XLoc+3, ty=b->YLoc+2;
    SGPRect clip{std::max({tx,outer.iLeft,0}),std::max({ty,outer.iTop,0}),
        std::min({b->XLoc+bw-3,outer.iRight,INT32(width)}),
        std::min({b->YLoc+bh-2,outer.iBottom,INT32(height)})};
    if (clip.iRight<=clip.iLeft || clip.iBottom<=clip.iTop) return TRUE;
    INT32 y=b->bTextYOffset==-1 ? (bh-GetFontHeight(b->usFont))/2+ty-1 : b->YLoc+b->bTextYOffset;
    INT32 x=b->XLoc+b->bTextXOffset;
    if (b->bTextXOffset==-1) {
        switch (b->bJustification) {
        case BUTTON_TEXT_LEFT: x=tx+3; break;
        case BUTTON_TEXT_RIGHT: x=clip.iRight-StringPixLength(b->string,b->usFont)-3; break;
        default: x=(bw-6-StringPixLength(b->string,b->usFont))/2+tx; break;
        }
    }
    const auto before=sgp::CaptureNativeFontState();
    struct Restore { sgp::NativeFontState s; ~Restore(){sgp::RestoreNativeFontState(s);} } restore{before};
    if (!SetFont(b->usFont) || !SetFontDestBuffer(ButtonDestBuffer,clip.iLeft,clip.iTop,clip.iRight,clip.iBottom,FALSE)) return FALSE;
    SetFontBackground(0);
    SetFontForeground(static_cast<UINT8>(b->sForeColor));
    INT16 fore=b->sForeColor;
    if (b->sShadowColor!=-1) SetFontShadow(static_cast<UINT8>(b->sShadowColor));
    // Source text hilite precedence is independent of gfRenderHilights.
    const bool hilite=(b->uiFlags & BUTTON_ENABLED) && (b->Area.uiFlags & MSYS_MOUSE_IN_AREA);
    if (hilite && b->sForeColorHilited!=-1) { fore=b->sForeColorHilited; SetFontForeground(static_cast<UINT8>(fore)); }
    else if ((b->uiFlags & BUTTON_CLICKED_ON) && b->sForeColorDown!=-1) { fore=b->sForeColorDown; SetFontForeground(static_cast<UINT8>(fore)); }
    if (hilite && b->sShadowColorHilited!=-1) SetFontShadow(static_cast<UINT8>(b->sShadowColorHilited));
    else if ((b->uiFlags & BUTTON_CLICKED_ON) && b->sShadowColorDown!=-1) SetFontShadow(static_cast<UINT8>(b->sShadowColorDown));
    else if (b->uiFlags & BUTTON_16BPPCOLOURTEXT) {
        SetRGBFontForeground(static_cast<UINT16>(fore));
        if (b->sShadowColor<1) SetRGBFontShadow(UINT16(1));
    }
    if ((b->uiFlags & BUTTON_CLICKED_ON) && b->fShiftText) { ++x; ++y; }
    if (b->sWrappedWidth!=-1) {
        // Source wrapping resets y whenever X uses automatic positioning; its
        // centered block starts at left+3+wrappedWidth/2 (not button center).
        if (b->bTextXOffset==-1) {
            y=b->YLoc+2;
            if (b->bJustification==BUTTON_TEXT_RIGHT) x=b->Area.RegionBottomRightX-3-b->sWrappedWidth;
            else if (b->bJustification==BUTTON_TEXT_CENTER) x=b->XLoc+3+b->sWrappedWidth/2;
            if (b->bJustification!=BUTTON_TEXT_LEFT && (b->uiFlags&BUTTON_CLICKED_ON) && b->fShiftText) {++x;++y;}
        }
        x+=b->bTextXSubOffSet; y+=b->bTextYSubOffSet;
        // DisplayWrappedString sets foreground/background itself. Shadow is
        // inherited from DrawTextOnButton; gap is exactly one native font row.
        SetFontForeground(static_cast<UINT8>(fore)); SetFontBackground(0);
        try {
            const std::wstring text(b->string);
            std::size_t begin=0;
            while (begin<text.size()) {
                std::size_t end=begin, lastSpace=std::wstring::npos;
                INT32 used=0;
                for (;end<text.size();++end) {
                    if (text[end]==177 || text[end]==L'\n') break;
                    CHAR16 glyph[2]{text[end],0};
                    const INT32 advance=StringPixLength(glyph,b->usFont);
                    if (text[end]==L' ') lastSpace=end;
                    if (used+advance>b->sWrappedWidth && end>begin) break;
                    used+=advance;
                }
                std::size_t next=end;
                if (end<text.size() && (text[end]==177 || text[end]==L'\n')) ++next;
                else if (end<text.size() && lastSpace!=std::wstring::npos && lastSpace>begin) {end=lastSpace; next=lastSpace+1;}
                if (end==begin && next==begin) {++end;next=end;}
                const std::wstring line=text.substr(begin,end-begin);
                INT32 lineX=x, pixels=StringPixLength(line.c_str(),b->usFont);
                if (b->bJustification==BUTTON_TEXT_RIGHT) lineX+=b->sWrappedWidth-pixels;
                else if (b->bJustification==BUTTON_TEXT_CENTER) lineX+=(b->sWrappedWidth-pixels+1)/2;
                if (sgp::DrawNativeFontText(lineX,y,line.c_str(),false)!=0) return FALSE;
                y+=GetFontHeight(b->usFont)+1;
                begin=next;
            }
            return TRUE;
        } catch (const std::bad_alloc&) {return FALSE;}
    }
    x+=b->bTextXSubOffSet; y+=b->bTextYSubOffSet;
    // The pinned source stores fMultiColor, but DrawTextOnButton always calls
    // mprintf; retain that observable behavior instead of inventing a mode.
    return sgp::DrawNativeFontText(x,y,b->string,false)==0 ? TRUE : FALSE;
}
// Clip is local to each composition stage; never replace the shared clip.
BOOLEAN drawIcon(GUI_BUTTON* b, UINT16 width, UINT16 height) {
    if (b->iIconID<0) return TRUE;
    HVOBJECT object=nullptr;
    if (b->uiFlags&BUTTON_GENERIC) {
        if (b->iIconID>=MAX_BUTTON_ICONS) return FALSE;
        object=genericIcons[static_cast<std::size_t>(b->iIconID)];
    } else if (!GetVideoObject(&object,static_cast<UINT32>(b->iIconID))) return FALSE;
    if (!validIndexed(object,static_cast<UINT16>(b->usIconIndex))) return FALSE;
    const auto& frame=object->pETRLEObject[b->usIconIndex];
    const INT32 bw=b->Area.RegionBottomRightX-b->XLoc, bh=b->Area.RegionBottomRightY-b->YLoc;
    SGPRect outer{}; GetClippingRect(&outer);
    SGPRect clip{std::max({INT32(b->XLoc)+3,outer.iLeft,0}),std::max({INT32(b->YLoc)+2,outer.iTop,0}),
        std::min({INT32(b->XLoc)+bw-3,outer.iRight,INT32(width)}),std::min({INT32(b->YLoc)+bh-2,outer.iBottom,INT32(height)})};
    if (clip.iRight<=clip.iLeft || clip.iBottom<=clip.iTop) return TRUE;
    INT32 x=b->bIconXOffset==-1 ? b->XLoc+3+(bw-6-frame.usWidth-frame.sOffsetX)/2 : b->XLoc+b->bIconXOffset;
    INT32 y=b->bIconYOffset==-1 ? b->YLoc+2+(bh-4-frame.usHeight-frame.sOffsetY)/2 : b->YLoc+b->bIconYOffset;
    if ((b->uiFlags&BUTTON_CLICKED_ON) && b->fShiftImage) {++x;++y;}
    UINT32 pitch=0; BYTE* buffer=LockVideoSurface(ButtonDestBuffer,&pitch);
    if (!buffer) return FALSE;
    const BOOLEAN result=Blt8BPPDataTo16BPPBufferTransparentClip(reinterpret_cast<UINT16*>(buffer),pitch,object,x,y,static_cast<UINT16>(b->usIconIndex),&clip);
    UnLockVideoSurface(ButtonDestBuffer); return result;
}
BOOLEAN drawGeneric(GUI_BUTTON* b, SGPRect clip) {
    const auto& skin=genericSkins[b->ImageNum];
    HVOBJECT object=skin.images[1];
    const bool hover=(b->Area.uiFlags&MSYS_MOUSE_IN_AREA) && gfRenderHilights;
    if (!(b->uiFlags&BUTTON_ENABLED)) {if (skin.images[0]) object=skin.images[0];}
    else if (b->uiFlags&BUTTON_CLICKED_ON) object=hover && skin.images[4] ? skin.images[4] : skin.images[3];
    else if (hover && skin.images[2]) object=skin.images[2];
    const INT32 bw=b->Area.RegionBottomRightX-b->XLoc, bh=b->Area.RegionBottomRightY-b->YLoc;
    UINT32 pitch=0; BYTE* buffer=LockVideoSurface(ButtonDestBuffer,&pitch);
    if (!buffer) return FALSE;
    struct Unlock {~Unlock(){UnLockVideoSurface(ButtonDestBuffer);}} unlock;
    const INT32 left=std::max(clip.iLeft,INT32(b->XLoc)), top=std::max(clip.iTop,INT32(b->YLoc));
    const INT32 right=std::min(clip.iRight,INT32(b->Area.RegionBottomRightX)), bottom=std::min(clip.iBottom,INT32(b->Area.RegionBottomRightY));
    for (INT32 y=top;y<bottom;++y) for(INT32 x=left;x<right;++x)
        std::memcpy(buffer+std::size_t(y)*pitch+std::size_t(x)*2,&skin.fill,2);
    auto blit=[&](HVOBJECT source,UINT16 index,INT32 x,INT32 y,SGPRect local) {
        return Blt8BPPDataTo16BPPBufferTransparentClip(reinterpret_cast<UINT16*>(buffer),pitch,source,x,y,index,&local)!=FALSE;
    };
    if (skin.images[5]) {
        const auto& tile=skin.images[5]->pETRLEObject[skin.backgroundIndex];
        const INT32 tw=tile.usWidth+tile.sOffsetX, th=tile.usHeight+tile.sOffsetY;
        const INT32 shift=(b->uiFlags&BUTTON_CLICKED_ON)?1:0;
        SGPRect area{std::max(left,INT32(b->XLoc)+shift),std::max(top,INT32(b->YLoc)+shift),right,bottom};
        // Same periodic phase as ImageFillVideoSurfaceArea (offset positive
        // starts one period before the origin); skip invisible repetitions.
        INT32 ox=skin.offsetX%tw; if (ox>=0) ox-=tw;
        INT32 oy=skin.offsetY%th; if (oy>=0) oy-=th;
        INT32 startX=b->XLoc+shift+ox, startY=b->YLoc+shift+oy;
        if (startX+tw<area.iLeft) startX+=((area.iLeft-startX-tw)/tw)*tw;
        if (startY+th<area.iTop) startY+=((area.iTop-startY-th)/th)*th;
        if (area.iRight>area.iLeft && area.iBottom>area.iTop)
            for(INT32 y=startY;y<area.iBottom;y+=th) for(INT32 x=startX;x<area.iRight;x+=tw)
                if(!blit(skin.images[5],skin.backgroundIndex,x,y,area)) return FALSE;
    }
    // Original 3x2 border layout: TL,T,TR,L,R,BL,B,BR,fill-pixel.
    const INT32 wide=bw/3, high=bh/2, xr=b->XLoc+bw-3, yb=b->YLoc+bh-2;
    for(INT32 q=0;q<wide;++q) {
        if(!blit(object,q?1:0,b->XLoc+q*3,b->YLoc,clip) ||
           !blit(object,q?6:5,b->XLoc+q*3,yb,clip)) return FALSE;
    }
    if(!blit(object,2,xr,b->YLoc,clip) || !blit(object,7,xr,yb,clip)) return FALSE;
    if(bh%2 && (!blit(object,3,b->XLoc,b->YLoc+(high-1)*2-1,clip) || !blit(object,4,xr,b->YLoc+(high-1)*2-1,clip))) return FALSE;
    for(INT32 q=1;q<high-1;++q)
        if(!blit(object,3,b->XLoc,b->YLoc+q*2,clip) || !blit(object,4,xr,b->YLoc+q*2,clip)) return FALSE;
    return TRUE;
}
BOOLEAN drawDisabledOverlay(GUI_BUTTON* b, INT8 style, UINT16 width, UINT16 height) {
    if (style!=DISABLED_STYLE_HATCHED && style!=DISABLED_STYLE_SHADED) return TRUE;
    SGPRect outer{}; GetClippingRect(&outer);
    const INT32 left=std::max({INT32(b->Area.RegionTopLeftX),outer.iLeft,0});
    const INT32 top=std::max({INT32(b->Area.RegionTopLeftY),outer.iTop,0});
    const INT32 right=std::min({INT32(b->Area.RegionBottomRightX),outer.iRight,INT32(width)});
    const INT32 bottom=std::min({INT32(b->Area.RegionBottomRightY),outer.iBottom,INT32(height)});
    if (right<=left || bottom<=top) return TRUE;
    UINT32 pitch=0; BYTE* buffer=LockVideoSurface(ButtonDestBuffer,&pitch);
    if (!buffer) return FALSE;
    for (INT32 y=top; y<bottom; ++y) for (INT32 x=left; x<right; ++x) {
        BYTE* at=buffer+std::size_t(y)*pitch+std::size_t(x)*2;
        UINT16 color=0;
        if (style==DISABLED_STYLE_SHADED) {
            std::memcpy(&color,at,2); color=ShadeTable[color];
        } else {
            // Original PixelateRect asm resets EBX to zero at each new row,
            // then merges its column with EDX after the first pixel. Preserve
            // its always-filled first column and clipped-origin hatch phase.
            if (x!=left && ((x-left+y-top)&1)) continue;
        }
        std::memcpy(at,&color,2);
    }
    UnLockVideoSurface(ButtonDestBuffer);
    return TRUE;
}

std::unique_ptr<CHAR16[]> copyText(const CHAR16* text) {
    if (!text || !*text) return {};
    const auto length=std::wcslen(text);
    if (length==std::numeric_limits<std::size_t>::max()) throw std::bad_alloc();
    std::unique_ptr<CHAR16[]> result(new CHAR16[length+1]);
    std::copy_n(text,length+1,result.get());
    return result;
}

}

BOOLEAN InitButtonSystem() {
    collect();
    if (initialized) return TRUE;
    if (callbackDepth || sgp::NativeMouseDispatchActive() || shutdownPending) return FALSE;
    try {
        ButtonList.assign(MAX_BUTTONS, nullptr);
        ButtonPictures.assign(MAX_BUTTON_PICS, {});
    } catch (const std::bad_alloc&) {
        ButtonList.clear();
        ButtonPictures.clear();
        return FALSE;
    }
    slots.fill({});
    owned.fill(false);
    anchor = previousAnchor = {};
    ButtonDestBuffer = FRAME_BUFFER;
    gfRenderHilights = TRUE;
    initialized = true;
    return TRUE;
}
void ShutdownButtonSystem() {
    sgp::CancelNativeButtonInput();
    initialized = false;
    shutdownPending = true;
    for (INT32 id = 0; id < static_cast<INT32>(ButtonList.size()); ++id) RemoveButton(id);
    collect();
}
INT16 LoadGenericButtonImages(const char* gray,const char* off,const char* hover,const char* on,
    const char* onHover,const char* background,INT16 index,INT16 offsetX,INT16 offsetY) {
    collect();
    if (!initialized || shutdownPending || !off || !on || index<0) return -1;
    INT16 slot=-1;
    for (INT16 i=0;i<MAX_GENERIC_PICS;++i)
        if (std::none_of(genericSkins[i].images.begin(),genericSkins[i].images.end(),[](HVOBJECT image){return image!=nullptr;})) {slot=i;break;}
    if (slot<0) return -1;
    GenericSkin skin;
    const std::array<const char*,6> names{gray,off,hover,on,onHover,background};
    for(std::size_t i=0;i<names.size();++i) {
        if (!names[i]) continue;
        skin.images[i]=loadObject(names[i]);
        if (!skin.images[i] || (i<5 ? !validSkin(skin.images[i]) : !validIndexed(skin.images[i],static_cast<UINT16>(index)))) {
            releaseSkin(skin); return -1;
        }
    }
    if (skin.images[5]) {
        const auto& tile=skin.images[5]->pETRLEObject[index];
        if (INT32(tile.usWidth)+tile.sOffsetX<=0 || INT32(tile.usHeight)+tile.sOffsetY<=0) {releaseSkin(skin);return -1;}
    }
    UINT8 pixel=0;
    if (!GetETRLEPixelValue(&pixel,skin.images[1],8,0,0)) {releaseSkin(skin);return -1;}
    skin.fill=skin.images[1]->p16BPPPalette[pixel]; skin.backgroundIndex=static_cast<UINT16>(index);
    skin.offsetX=offsetX; skin.offsetY=offsetY;
    genericSkins[slot]=skin; return slot;
}
BOOLEAN UnloadGenericButtonImage(INT16 image) {
    collect();
    if (image<0 || image>=MAX_GENERIC_PICS || !genericValid(static_cast<UINT32>(image)) || genericReferenced(static_cast<UINT32>(image))) return FALSE;
    releaseSkin(genericSkins[image]);
    return !genericValid(static_cast<UINT32>(image));
}
INT16 LoadGenericButtonIcon(const char* filename) {
    collect(); if (!initialized || shutdownPending) return -1;
    for(INT16 i=0;i<MAX_BUTTON_ICONS;++i) if(!genericIcons[i]) {
        HVOBJECT object=loadObject(filename);
        if(!validIndexed(object,0)) {if(object) DeleteVideoObject(object);return -1;}
        genericIcons[i]=object; return i;
    }
    return -1;
}
BOOLEAN UnloadGenericButtonIcon(INT16 icon) {
    collect();
    if(icon<0 || icon>=MAX_BUTTON_ICONS || !genericIcons[icon] || iconReferenced(icon)) return FALSE;
    if(!DeleteVideoObject(genericIcons[icon])) return FALSE;
    genericIcons[icon]=nullptr; return TRUE;
}
INT32 CreateTextButton(STR16 text,UINT32 font,INT16 fore,INT16 shadow,INT16 image,
    INT16 x,INT16 y,INT16 width,INT16 height,INT32 type,INT16 priority,GUI_CALLBACK move,GUI_CALLBACK click) {
    constexpr UINT32 allowed=BUTTON_TYPE_MASK|BUTTON_NEWTOGGLE;
    if (image < -1 || image>=MAX_GENERIC_PICS || font>65535 || (static_cast<UINT32>(type)&~allowed)) return -1;
    const UINT32 selected=image<0?0u:static_cast<UINT32>(image);
    if (!genericValid(selected)) {
        // Init stays usable for resource-free quick buttons. Default generic
        // files are admitted only when needed, and failure is observable.
        if (selected!=0 || !initialized || shutdownPending ||
            LoadGenericButtonImages(nullptr,DEFAULT_GENERIC_BUTTON_OFF,nullptr,DEFAULT_GENERIC_BUTTON_ON,nullptr,nullptr,0,0,0)!=0) return -1;
        genericSkins[0].images[2]=loadObject(DEFAULT_GENERIC_BUTTON_OFF_HI);
        genericSkins[0].images[4]=loadObject(DEFAULT_GENERIC_BUTTON_ON_HI);
        for(auto state:{2u,4u}) if(genericSkins[0].images[state] && !validSkin(genericSkins[0].images[state])) {
            DeleteVideoObject(genericSkins[0].images[state]); genericSkins[0].images[state]=nullptr;
        }
    }
    std::unique_ptr<CHAR16[]> ownedText;
    try {ownedText=copyText(text);} catch(const std::bad_alloc&) {return -1;}
    const INT32 id=install(selected,x,y,static_cast<UINT32>(std::max<INT16>(4,width)),static_cast<UINT32>(std::max<INT16>(3,height)),
        static_cast<UINT32>(type)|BUTTON_GENERIC,priority,move,click);
    if(GUI_BUTTON* b=live(id)) {
        b->string=ownedText.release();b->usFont=static_cast<UINT16>(font);b->sForeColor=fore;b->sShadowColor=shadow;
    }
    return id;
}
INT32 CreateIconButton(INT16 icon,INT16 frame,INT16 image,INT16 x,INT16 y,INT16 width,INT16 height,
    INT32 type,INT16 priority,GUI_CALLBACK move,GUI_CALLBACK click) {
    if(icon<0 || icon>=MAX_BUTTON_ICONS || frame<0 || !validIndexed(genericIcons[icon],static_cast<UINT16>(frame))) return -1;
    const INT32 id=CreateTextButton(nullptr,0,0,-1,image,x,y,width,height,type,priority,move,click);
    if(GUI_BUTTON* b=live(id)) {b->iIconID=icon;b->usIconIndex=frame;}
    return id;
}
BOOLEAN SpecifyButtonIcon(INT32 id,INT32 icon,UINT16 frame,INT8 x,INT8 y,BOOLEAN /* shift */) {
    GUI_BUTTON* b=live(id); if(!b) return FALSE;
    if(icon==-1) {b->iIconID=-1;b->usIconIndex=-1;b->uiFlags|=BUTTON_DIRTY;return FALSE;}
    HVOBJECT object=nullptr;
    if(b->uiFlags&BUTTON_GENERIC) {if(icon<0 || icon>=MAX_BUTTON_ICONS) return FALSE;object=genericIcons[icon];}
    else if(icon<0 || !GetVideoObject(&object,static_cast<UINT32>(icon))) return FALSE;
    if(frame>static_cast<UINT16>(std::numeric_limits<INT16>::max()) || !validIndexed(object,frame)) return FALSE;
    b->iIconID=icon;b->usIconIndex=static_cast<INT16>(frame);b->bIconXOffset=x;b->bIconYOffset=y;
    // Pinned source ignores fShiftImage and always sets TRUE.
    b->fShiftImage=TRUE;b->uiFlags|=BUTTON_DIRTY;return TRUE;
}
void SpecifyButtonTextWrappedWidth(INT32 id,INT16 width) {
    if(GUI_BUTTON* b=live(id);b && (width==-1 || width>0)) {b->sWrappedWidth=width;b->uiFlags|=BUTTON_DIRTY;}
}
void SetBtnHelpEndCallback(INT32 id,MOUSE_HELPTEXT_DONE_CALLBACK callback) {
    if(GUI_BUTTON* b=live(id)) SetRegionHelpEndCallback(&b->Area,callback);
}
INT32 UseVObjAsButtonImage(HVOBJECT object, INT32 gray, INT32 off, INT32 hover, INT32 on, INT32 onHover) {
    return addImage(object, false, gray, off, hover, on, onHover);
}
INT32 UseLoadedButtonImage(INT32 image, INT32 gray, INT32 off, INT32 hover, INT32 on, INT32 onHover) {
    collect();
    if (image < 0 || !imageValid(static_cast<UINT32>(image))) return -1;
    const auto index = static_cast<std::size_t>(image);
    // Original UseLoadedButtonImage explicitly excludes external objects.
    if (ButtonPictures[index].fFlags & 2u) return -1; // GUI_BTN_EXTERNAL_VOBJ
    const INT32 alias = addImage(ButtonPictures[index].vobj, false, gray, off, hover, on, onHover);
    if (alias >= 0) ButtonPictures[static_cast<std::size_t>(alias)].fFlags = 1u;
    return alias;
}
UINT16 GetWidthOfButtonPic(UINT16 image, INT32 frame) {
    if (!imageValid(image) || frame < 0 || frame >= ButtonPictures[image].vobj->usNumberOfObjects) return 0;
    return ButtonPictures[image].vobj->pETRLEObject[frame].usWidth;
}
UINT16 GetHeightOfButtonPic(UINT16 image, INT32 frame) {
    if (!imageValid(image) || frame < 0 || frame >= ButtonPictures[image].vobj->usNumberOfObjects) return 0;
    return ButtonPictures[image].vobj->pETRLEObject[frame].usHeight;
}
INT32 LoadButtonImage(const char* filename, INT32 gray, INT32 off, INT32 hover, INT32 on, INT32 onHover) {
    if (!initialized || !filename || !*filename || std::strlen(filename) >= SGPFILENAME_LEN) return -1;
    VOBJECT_DESC desc{};
    desc.fCreateFlags = VOBJECT_CREATE_FROMFILE;
    std::memcpy(desc.ImageFile, filename, std::strlen(filename) + 1);
    HVOBJECT object = CreateVideoObject(&desc);
    if (!object) return -1;
    const INT32 image = addImage(object, true, gray, off, hover, on, onHover);
    if (image < 0) DeleteVideoObject(object);
    return image;
}
void UnloadButtonImage(INT32 image) { collect(); unload(image); }
INT32 QuickCreateButton(UINT32 image, INT16 x, INT16 y, INT32 type, INT16 priority, GUI_CALLBACK move, GUI_CALLBACK click) {
    const UINT32 flags = static_cast<UINT32>(type);
    constexpr UINT32 allowed = BUTTON_TYPE_MASK | BUTTON_NEWTOGGLE;
    if ((flags & ~allowed) || !imageValid(image)) {
        std::fprintf(stderr, "Native quick button rejected: image=%u x=%d y=%d flags=%u priority=%d images=%zu\n",
            image, x, y, flags, priority, static_cast<std::size_t>(std::count_if(ButtonPictures.begin(), ButtonPictures.end(),
                [](const BUTTON_PICS& p) { return p.vobj != nullptr; })));
        return -1;
    }
    const INT32 id = install(image, x, y, ButtonPictures[image].MaxWidth, ButtonPictures[image].MaxHeight,
                   flags, priority, move, click);
    if (id < 0) std::fprintf(stderr, "Native quick button allocation failed: image=%u x=%d y=%d width=%u height=%u priority=%d buttons=%zu\n",
        image, x, y, ButtonPictures[image].MaxWidth, ButtonPictures[image].MaxHeight, priority,
        static_cast<std::size_t>(std::count_if(ButtonList.begin(), ButtonList.end(), [](GUI_BUTTON* b) { return b != nullptr; })));
    return id;
}
INT32 CreateHotSpot(INT16 x, INT16 y, INT16 width, INT16 height, INT16 priority, GUI_CALLBACK move, GUI_CALLBACK click) {
    if (width < 0 || height < 0) return -1;
    return install(std::numeric_limits<UINT32>::max(), x, y, static_cast<UINT32>(width),
                   static_cast<UINT32>(height), BUTTON_HOT_SPOT, priority, move, click);
}
INT32 CreateCheckBoxButton(INT16 x, INT16 y, const char* filename, INT16 priority, GUI_CALLBACK click) {
    const INT32 image = LoadButtonImage(filename, -1, 0, 1, 2, 3);
    if (image < 0) return -1;
    const INT32 id = QuickCreateButton(static_cast<UINT32>(image), x, y, BUTTON_CHECKBOX, priority, nullptr, click);
    if (GUI_BUTTON* b = live(id)) b->uiFlags |= BUTTON_SELFDELETE_IMAGE;
    else UnloadButtonImage(image);
    return id;
}
void SetButtonFastHelpText(INT32 id, STR16 text) {
    if (GUI_BUTTON* b = live(id)) SetRegionFastHelpText(&b->Area, text);
}
void AllowDisabledButtonFastHelp(INT32 id, BOOLEAN /* allow */) {
    // Original Button System.cpp ignores fAllow and always enables this flag.
    // Route through MSYS so its actual selection/renderer ownership is retained.
    if (GUI_BUTTON* b = live(id)) MSYS_AllowDisabledRegionFastHelp(&b->Area, TRUE);
}
INT32 CreateIconAndTextButton(INT32 image, const STR16 text, UINT32 font,
    INT16 fore, INT16 shadow, INT16 down, INT16 downShadow, INT8 justification,
    INT16 x, INT16 y, INT32 type, INT16 priority, GUI_CALLBACK move, GUI_CALLBACK click) {
    constexpr UINT32 allowed=BUTTON_TYPE_MASK|BUTTON_NEWTOGGLE|BUTTON_16BPPCOLOURTEXT;
    if (image<0 || !imageValid(static_cast<UINT32>(image)) || (static_cast<UINT32>(type)&~allowed) || font>65535) return -1;
    std::unique_ptr<CHAR16[]> ownedText;
    try { ownedText=copyText(text); } catch (const std::bad_alloc&) { return -1; }
    const auto& pic=ButtonPictures[static_cast<std::size_t>(image)];
    const INT32 id=install(static_cast<UINT32>(image),x,y,pic.MaxWidth,pic.MaxHeight,static_cast<UINT32>(type),priority,move,click);
    if (GUI_BUTTON* b=live(id)) {
        b->string=ownedText.release(); b->usFont=static_cast<UINT16>(font);
        b->sForeColor=fore; b->sShadowColor=shadow; b->sForeColorDown=down; b->sShadowColorDown=downShadow;
        b->bJustification=justification;
    }
    return id;
}
BOOLEAN SetButtonCursor(INT32 id, UINT16 cursor) {
    if (GUI_BUTTON* b=live(id)) { MSYS_ChangeRegionCursor(&b->Area,cursor); return TRUE; }
    return FALSE;
}
void SpecifyDisabledButtonStyle(INT32 id, INT8 style) {
    if (GUI_BUTTON* b=live(id); b && style>=DISABLED_STYLE_NONE && style<=DISABLED_STYLE_SHADED) {
        b->bDisabledStyle=style; b->uiFlags|=BUTTON_DIRTY;
    }
}

void SpecifyButtonText(INT32 id, STR16 text) {
    if (GUI_BUTTON* b=live(id)) {
        // Copy first: self/sub-string rebinding and allocation failure are safe.
        std::unique_ptr<CHAR16[]> replacement;
        try { replacement=copyText(text); } catch (const std::bad_alloc&) { return; }
        delete[] b->string; b->string=replacement.release(); b->uiFlags|=BUTTON_DIRTY;
    }
}
void RemoveTextFromButton(INT32 id) {
    if (GUI_BUTTON* b=live(id)) {
        SpecifyButtonText(id,nullptr); b->usFont=0; b->sForeColor=0;
        b->sWrappedWidth=-1; b->sShadowColor=b->sForeColorDown=b->sShadowColorDown=-1;
        b->sForeColorHilited=b->sShadowColorHilited=-1; b->bJustification=BUTTON_TEXT_CENTER;
        b->bTextXOffset=b->bTextYOffset=-1; b->bTextXSubOffSet=b->bTextYSubOffSet=0; b->fShiftText=TRUE;
    }
}
void SpecifyButtonFont(INT32 id, UINT32 font) { if (GUI_BUTTON* b=live(id); b && font<=65535) { b->usFont=static_cast<UINT16>(font); b->uiFlags|=BUTTON_DIRTY; } }
void SpecifyButtonMultiColorFont(INT32 id, BOOLEAN multi) { if (GUI_BUTTON* b=live(id)) { b->fMultiColor=multi; b->uiFlags|=BUTTON_DIRTY; } }
void SpecifyButtonUpTextColors(INT32 id, INT16 f, INT16 s) { if (GUI_BUTTON* b=live(id)) { b->sForeColor=f; b->sShadowColor=s; b->uiFlags|=BUTTON_DIRTY; } }
void SpecifyButtonDownTextColors(INT32 id, INT16 f, INT16 s) { if (GUI_BUTTON* b=live(id)) { b->sForeColorDown=f; b->sShadowColorDown=s; b->uiFlags|=BUTTON_DIRTY; } }
void SpecifyButtonHilitedTextColors(INT32 id, INT16 f, INT16 s) { if (GUI_BUTTON* b=live(id)) { b->sForeColorHilited=f; b->sShadowColorHilited=s; b->uiFlags|=BUTTON_DIRTY; } }
void SpecifyButtonTextJustification(INT32 id, INT8 j) { if (GUI_BUTTON* b=live(id)) { b->bJustification=(j<BUTTON_TEXT_LEFT || j>BUTTON_TEXT_RIGHT)?BUTTON_TEXT_CENTER:j; b->uiFlags|=BUTTON_DIRTY; } }
void SpecifyButtonTextOffsets(INT32 id, INT8 x, INT8 y, BOOLEAN shift) { if (GUI_BUTTON* b=live(id)) { b->bTextXOffset=x; b->bTextYOffset=y; b->fShiftText=shift; b->uiFlags|=BUTTON_DIRTY; } }
void SpecifyButtonTextSubOffsets(INT32 id, INT8 x, INT8 y, BOOLEAN shift) { if (GUI_BUTTON* b=live(id)) { b->bTextXSubOffSet=x; b->bTextYSubOffSet=y; b->fShiftText=shift; b->uiFlags|=BUTTON_DIRTY; } }
void SpecifyFullButtonTextAttributes(INT32 id, STR16 text, INT32 font, INT16 f, INT16 s, INT16 df, INT16 ds, INT8 j) {
    SpecifyButtonText(id,text); SpecifyButtonFont(id,static_cast<UINT32>(font));
    SpecifyButtonUpTextColors(id,f,s); SpecifyButtonDownTextColors(id,df,ds); SpecifyButtonTextJustification(id,j);
}
void SpecifyGeneralButtonTextAttributes(INT32 id, STR16 text, INT32 font, INT16 f, INT16 s) {
    SpecifyButtonText(id,text); SpecifyButtonFont(id,static_cast<UINT32>(font)); SpecifyButtonUpTextColors(id,f,s);
}
BOOLEAN SpecifyButtonImage(GUI_BUTTON* candidate, UINT32 image) {
    GUI_BUTTON* b=resolve(identity(candidate));
    if (!b || (b->uiFlags & (BUTTON_HOT_SPOT|BUTTON_GENERIC)) || !imageValid(image)) return FALSE;
    const UINT32 old=b->ImageNum;
    b->ImageNum=image; b->uiFlags|=BUTTON_DIRTY;
    // Keep the original fixed region; rebinding does not resize quick buttons.
    if ((b->uiFlags & BUTTON_SELFDELETE_IMAGE) && old!=image) {
        b->uiFlags&=~static_cast<UINT32>(BUTTON_SELFDELETE_IMAGE); unload(static_cast<INT32>(old));
    }
    return TRUE;
}
BOOLEAN SpecifyButtonImage(INT32 id, UINT32 image) { return SpecifyButtonImage(live(id),image); }

void RemoveButton(INT32 id) {
    GUI_BUTTON* b = live(id);
    if (!b) return;
    const Identity token = identity(b);
    if (same(anchor, token)) anchor = {};
    if (same(previousAnchor, token)) previousAnchor = {};
    b->uiFlags |= BUTTON_DELETION_PENDING;
    MSYS_RemoveRegion(&b->Area);
    collect();
}
BOOLEAN EnableButton(INT32 id) {
    GUI_BUTTON* b = live(id);
    if (!b) return FALSE;
    const BOOLEAN was = (b->uiFlags & BUTTON_ENABLED) ? TRUE : FALSE;
    b->uiFlags |= BUTTON_ENABLED | BUTTON_DIRTY;
    return was;
}
BOOLEAN DisableButton(INT32 id) {
    GUI_BUTTON* b = live(id);
    if (!b) return FALSE;
    const BOOLEAN was = (b->uiFlags & BUTTON_ENABLED) ? TRUE : FALSE;
    const Identity token = identity(b);
    if (same(anchor, token)) { setOn(b, anchoredState); anchor = {}; }
    if (same(previousAnchor, token)) previousAnchor = {};
    auto& slot = slots[static_cast<std::size_t>(id)];
    if (slot.pressed) setOn(b, slot.prepressOn);
    slot.pressed = false;
    if (b->ubToggleButtonActivated) setOn(b, b->ubToggleButtonOldState != 0);
    b->ubToggleButtonActivated = FALSE;
    b->uiFlags = (b->uiFlags & ~static_cast<UINT32>(BUTTON_ENABLED)) | BUTTON_DIRTY;
    // Intentionally leave MSYS enabled: disabled buttons still occlude controls.
    return was;
}
void HideButton(INT32 id) {
    GUI_BUTTON* b = live(id);
    if (!b) return;
    const Identity token = identity(b);
    if (same(anchor, token)) { setOn(b, anchoredState); anchor = {}; }
    if (same(previousAnchor, token)) previousAnchor = {};
    auto& slot = slots[static_cast<std::size_t>(id)];
    if (slot.pressed) setOn(b, slot.prepressOn);
    slot.pressed = false;
    b->ubToggleButtonActivated = FALSE;
    slot.hidden = true;
    MSYS_DisableRegion(&b->Area);
}
void ShowButton(INT32 id) {
    if (GUI_BUTTON* b = live(id)) {
        slots[static_cast<std::size_t>(id)].hidden = false;
        b->uiFlags |= BUTTON_DIRTY;
        MSYS_EnableRegion(&b->Area);
    }
}
void SetButtonPosition(INT32 id, INT16 x, INT16 y) {
    GUI_BUTTON* b = live(id);
    if (!b) return;
    const UINT32 width = static_cast<UINT32>(b->Area.RegionBottomRightX - b->Area.RegionTopLeftX);
    const UINT32 height = static_cast<UINT32>(b->Area.RegionBottomRightY - b->Area.RegionTopLeftY);
    if (!geometry(x, y, width, height)) return;
    MSYS_MoveMouseRegionTo(&b->Area, x, y);
    b->XLoc = x; b->YLoc = y; b->uiFlags |= BUTTON_DIRTY;
}
BOOLEAN SetButtonDestBuffer(UINT32 destination) {
    UINT16 width = 0, height = 0;
    UINT8 depth = 0;
    if (!GetVideoSurfaceDescription(destination, &width, &height, &depth) || depth != 16) return FALSE;
    ButtonDestBuffer = destination;
    return TRUE;
}
BOOLEAN DrawButton(INT32 id) {
    collect();
    GUI_BUTTON* b = live(id);
    if (!b || slots[static_cast<std::size_t>(id)].hidden) return FALSE;
    if (b->uiFlags & BUTTON_HOT_SPOT) return TRUE;
    if ((b->uiFlags&BUTTON_GENERIC) ? !genericValid(b->ImageNum) : !imageValid(b->ImageNum)) return FALSE;
    if (b->uiFlags&BUTTON_GENERIC) {
        UINT16 width=0,height=0;UINT8 depth=0;
        if(!GetVideoSurfaceDescription(ButtonDestBuffer,&width,&height,&depth) || depth!=16) return FALSE;
        SGPRect clip{};GetClippingRect(&clip);
        if(clip.iRight<clip.iLeft || clip.iBottom<clip.iTop) return FALSE;
        clip.iLeft=std::clamp(clip.iLeft,0,INT32(width));clip.iTop=std::clamp(clip.iTop,0,INT32(height));
        clip.iRight=std::clamp(clip.iRight,clip.iLeft,INT32(width));clip.iBottom=std::clamp(clip.iBottom,clip.iTop,INT32(height));
        BOOLEAN result=drawGeneric(b,clip);
        if(result) result=drawIcon(b,width,height);
        if(result) result=drawText(b,width,height);
        if(result && !(b->uiFlags&BUTTON_ENABLED) && !genericSkins[b->ImageNum].images[0]) {
            INT8 style=b->bDisabledStyle;
            if(style==DISABLED_STYLE_DEFAULT) style=b->string?DISABLED_STYLE_SHADED:DISABLED_STYLE_HATCHED;
            result=drawDisabledOverlay(b,style,width,height);
        }
        if(result) {b->uiOldFlags=b->uiFlags;b->uiFlags&=~static_cast<UINT32>(BUTTON_DIRTY);}
        return result;
    }
    const BUTTON_PICS& image = ButtonPictures[b->ImageNum];
    INT32 frame = 0;
    if (b->uiFlags & BUTTON_ENABLED) {
        bool hilite = (b->Area.uiFlags & MSYS_MOUSE_IN_AREA) && gfRenderHilights;
        if (b->uiFlags & BUTTON_CHECKBOX) hilite = hilite && (b->Area.ButtonState & MSYS_LEFT_BUTTON);
        if (b->uiFlags & BUTTON_CLICKED_ON) {
            if (hilite && image.OnHilite != -1) frame = image.OnHilite;
            else if (image.OnNormal != -1) frame = image.OnNormal;
        } else {
            if (hilite && image.OffHilite != -1) frame = image.OffHilite;
            else if (image.OffNormal != -1) frame = image.OffNormal;
        }
    } else if (image.Grayed != -1) frame = image.Grayed;
    else if (b->uiFlags & BUTTON_CHECKBOX) frame = (b->uiFlags & BUTTON_CLICKED_ON) ? image.OnHilite : image.OffHilite;
    else frame = image.OffNormal;
    if (frame < 0 || frame >= image.vobj->usNumberOfObjects) return FALSE;
    // The accepted native layer implements bounded object-to-buffer kernels,
    // not the legacy surface-ID BltVideoObject overload. Lock the actual target
    // surface and intersect a local clip; never alter the shared clipping state.
    UINT16 width = 0, height = 0;
    UINT8 depth = 0;
    if (!GetVideoSurfaceDescription(ButtonDestBuffer, &width, &height, &depth) || depth != 16) return FALSE;
    SGPRect clip{};
    GetClippingRect(&clip);
    if (clip.iRight < clip.iLeft || clip.iBottom < clip.iTop) return FALSE;
    clip.iLeft = std::clamp(clip.iLeft, 0, static_cast<INT32>(width));
    clip.iTop = std::clamp(clip.iTop, 0, static_cast<INT32>(height));
    clip.iRight = std::clamp(clip.iRight, clip.iLeft, static_cast<INT32>(width));
    clip.iBottom = std::clamp(clip.iBottom, clip.iTop, static_cast<INT32>(height));
    UINT32 pitch = 0;
    BYTE* buffer = LockVideoSurface(ButtonDestBuffer, &pitch);
    if (!buffer) return FALSE; // Existing caller lock remains owned by caller.
    struct Unlock {
        UINT32 id;
        ~Unlock() { if (id) UnLockVideoSurface(id); }
    } unlock{ButtonDestBuffer};
    if (static_cast<std::uint64_t>(width) * sizeof(UINT16) > pitch) return FALSE;
    BOOLEAN result = Blt8BPPDataTo16BPPBufferTransparentClip(
        reinterpret_cast<UINT16*>(buffer), pitch, image.vobj, b->XLoc, b->YLoc,
        static_cast<UINT16>(frame), &clip);
    UnLockVideoSurface(unlock.id); unlock.id=0;
    if (result) result=drawIcon(b,width,height);
    if (result) result=drawText(b,width,height);
    if (result && !(b->uiFlags & BUTTON_ENABLED) && image.Grayed==-1) {
        INT8 style=b->bDisabledStyle;
        if (style==DISABLED_STYLE_DEFAULT)
            style=(b->uiFlags & BUTTON_CHECKBOX) || !b->string ? DISABLED_STYLE_HATCHED : DISABLED_STYLE_SHADED;
        result=drawDisabledOverlay(b,style,width,height);
    }
    if (result) { b->uiOldFlags = b->uiFlags; b->uiFlags &= ~static_cast<UINT32>(BUTTON_DIRTY); }
    return result;
}
namespace {
void drawForcedCheckbox(INT32 id, bool pressed) {
    GUI_BUTTON* b = live(id);
    if (!b) return;
    // The source temporarily overrides gfLeftButtonState and leaves hover set.
    // Native drawing uses the event-replayed region state instead of the final
    // polling state, so override only that input and restore every held bit.
    const UINT16 held = b->Area.ButtonState;
    b->Area.uiFlags |= MSYS_MOUSE_IN_AREA;
    if (pressed) b->Area.ButtonState |= MSYS_LEFT_BUTTON;
    else b->Area.ButtonState &= static_cast<UINT16>(~MSYS_LEFT_BUTTON);
    DrawButton(id);
    b->Area.ButtonState = held;
}
}
void DrawCheckBoxButtonOn(INT32 id) { drawForcedCheckbox(id, true); }
void DrawCheckBoxButtonOff(INT32 id) { drawForcedCheckbox(id, false); }
void RenderButtons() {
    sgp::RestoreNativeFastHelpBackground();
    collect();
    for (INT32 i = 0; i < static_cast<INT32>(ButtonList.size()); ++i) {
        GUI_BUTTON* b = live(i);
        if (!b) continue;
        if (b->uiFlags & BUTTON_FORCE_UNDIRTY) {
            b->uiFlags &= ~static_cast<UINT32>(BUTTON_FORCE_UNDIRTY | BUTTON_DIRTY);
            continue;
        }
        if (((b->uiFlags ^ b->uiOldFlags) & (BUTTON_CLICKED_ON | BUTTON_ENABLED)) ||
            (b->uiFlags & BUTTON_DIRTY)) DrawButton(i);
    }
}
void MarkAButtonDirty(INT32 id) { if (GUI_BUTTON* b = live(id)) b->uiFlags |= BUTTON_DIRTY; }
void MarkButtonsDirty() { for (INT32 i = 0; i < static_cast<INT32>(ButtonList.size()); ++i) MarkAButtonDirty(i); }
void UnMarkButtonDirty(INT32 id) { if (GUI_BUTTON* b = live(id)) b->uiFlags &= ~static_cast<UINT32>(BUTTON_DIRTY); }
void UnmarkButtonsDirty() { for (INT32 i = 0; i < static_cast<INT32>(ButtonList.size()); ++i) UnMarkButtonDirty(i); }
void ForceButtonUnDirty(INT32 id) { if (GUI_BUTTON* b = live(id)) b->uiFlags |= BUTTON_FORCE_UNDIRTY; }
void MSYS_SetBtnUserData(INT32 id, INT32 index, NativeUIPayload data) {
    if (GUI_BUTTON* b = live(id); b && index >= 0 && index < 4) b->UserData[index] = data;
}
NativeUIPayload MSYS_GetBtnUserData(GUI_BUTTON* candidate, INT32 index) {
    GUI_BUTTON* b = resolve(identity(candidate));
    return b && index >= 0 && index < 4 ? b->UserData[index] : 0;
}
BOOLEAN GetButtonArea(INT32 id, SGPRect* rect) {
    GUI_BUTTON* b = live(id);
    if (!b || !rect) return FALSE;
    *rect = {b->Area.RegionTopLeftX, b->Area.RegionTopLeftY, b->Area.RegionBottomRightX, b->Area.RegionBottomRightY};
    return TRUE;
}
INT32 GetButtonWidth(INT32 id) { GUI_BUTTON* b = live(id); return b ? b->Area.RegionBottomRightX - b->Area.RegionTopLeftX : 0; }
INT32 GetButtonHeight(INT32 id) { GUI_BUTTON* b = live(id); return b ? b->Area.RegionBottomRightY - b->Area.RegionTopLeftY : 0; }
INT32 GetButtonX(INT32 id) { GUI_BUTTON* b = live(id); return b ? b->XLoc : 0; }
INT32 GetButtonY(INT32 id) { GUI_BUTTON* b = live(id); return b ? b->YLoc : 0; }

void QuickButtonCallbackMMove(MOUSE_REGION* region, INT32 reason) {
    CallbackScope scope;
    GUI_BUTTON* b = fromRegion(region);
    if (!b) return;
    const Identity token = identity(b);
    const auto generation = sgp::NativeMouseRegionGeneration(region);
    if ((b->uiFlags & BUTTON_ENABLED) && (reason & (MSYS_CALLBACK_REASON_LOST_MOUSE | MSYS_CALLBACK_REASON_GAIN_MOUSE)))
        b->uiFlags |= BUTTON_DIRTY;
    // Sound services may invoke client code; guard identity before touching b.
    if (b->ubSoundSchemeID && (reason&(MSYS_CALLBACK_REASON_GAIN_MOUSE|MSYS_CALLBACK_REASON_LOST_MOUSE)) && !resolve(anchor)) {
        const bool gain=(reason&MSYS_CALLBACK_REASON_GAIN_MOUSE)!=0;
        const bool enabled=(b->uiFlags&BUTTON_ENABLED)!=0;
        PlayButtonSound(b->IDNum,enabled ? (gain?BUTTON_SOUND_MOVED_ONTO:BUTTON_SOUND_MOVED_OFF_OF) :
            (gain?BUTTON_SOUND_DISABLED_MOVED_ONTO:BUTTON_SOUND_DISABLED_MOVED_OFF_OF));
        if(!stillLive(token,generation)) return;
    }
    // Original NEWTOGGLE LOST restoration precedes the supplied move callback,
    // even on the disabled path. It does not re-arm on re-entry.
    if ((b->uiFlags & BUTTON_NEWTOGGLE) && (reason & MSYS_CALLBACK_REASON_LOST_MOUSE) && b->ubToggleButtonActivated) {
        b->ubToggleButtonActivated = FALSE;
        setOn(b, b->ubToggleButtonOldState != 0);
    }
    if (!(b->uiFlags & BUTTON_ENABLED)) return;
    if ((b->uiFlags & BUTTON_MOVE_CALLBACK) && b->MoveCallback) {
        const GUI_CALLBACK callback = b->MoveCallback;
        callback(b, reason);
        if (!stillLive(token, generation)) return;
    }
}
void QuickButtonCallbackMButn(MOUSE_REGION* region, INT32 reason) {
    CallbackScope scope;
    GUI_BUTTON* b = fromRegion(region);
    if (!b) return;
    const Identity token = identity(b);
    const auto regionGeneration = sgp::NativeMouseRegionGeneration(region);
    const bool enabled = (b->uiFlags & BUTTON_ENABLED) != 0;
    const bool before = (b->uiFlags & BUTTON_CLICKED_ON) != 0;
    bool soundBefore=before;
    auto& slot = slots[static_cast<std::size_t>(token.id)];
    if (enabled && (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN)) {
        if (!slot.pressed) { slot.prepressOn = before; slot.pressed = true; }
    }
    if (enabled && (b->uiFlags & BUTTON_NEWTOGGLE)) {
        if ((reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) && !b->ubToggleButtonActivated) {
            b->ubToggleButtonOldState = before ? TRUE : FALSE;
            setOn(b, !before);
            b->ubToggleButtonActivated = TRUE;
        } else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) b->ubToggleButtonActivated = FALSE;
    }
    if (enabled && b->MoveCallback == DEFAULT_MOVE_CALLBACK) {
        if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) {
            anchor = token; anchoredState = before; setOn(b, true);
        } else if ((reason & MSYS_CALLBACK_REASON_LBUTTON_UP) && (b->uiFlags & BUTTON_NO_TOGGLE)) setOn(b, false);
    } else if (enabled && (b->uiFlags & BUTTON_CHECKBOX)) {
        if (reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) { anchor = token; anchoredState = before; soundBefore=(b->uiFlags&BUTTON_CLICKED_ON)==0; }
        else if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) {
            // Safe native extension: a stray UP cannot toggle a checkbox.
            if (!same(previousAnchor, token)) return;
            setOn(b, !before); soundBefore=before;
        }
    }
    if (!enabled) {
        if(b->ubSoundSchemeID && (reason&(MSYS_CALLBACK_REASON_LBUTTON_DWN|MSYS_CALLBACK_REASON_RBUTTON_DWN))) {
            PlayButtonSound(b->IDNum,BUTTON_SOUND_DISABLED_CLICK);
            if(!stillLive(token,regionGeneration)) return;
        }
        if ((b->uiFlags & BUTTON_ALLOW_DISABLED_CALLBACK) &&
            (b->uiFlags & BUTTON_CLICK_CALLBACK) && b->ClickCallback) {
            // memcpy preserves the original unsigned high bit on every C++17
            // implementation, avoiding out-of-range unsigned-to-signed casts.
            const UINT32 bits = static_cast<UINT32>(reason) | BUTTON_DISABLED_CALLBACK;
            INT32 disabledReason = 0;
            static_assert(sizeof(disabledReason) == sizeof(bits));
            std::memcpy(&disabledReason, &bits, sizeof(bits));
            const GUI_CALLBACK callback = b->ClickCallback;
            callback(b, disabledReason);
            if (!stillLive(token, regionGeneration)) return;
        }
        return;
    }
    if ((b->uiFlags & BUTTON_CLICK_CALLBACK) && b->ClickCallback) {
        if (!(reason & MSYS_CALLBACK_REASON_LBUTTON_UP) || b->MoveCallback != DEFAULT_MOVE_CALLBACK || same(previousAnchor, token)) {
            const GUI_CALLBACK callback = b->ClickCallback;
            callback(b, reason);
            if (!stillLive(token, regionGeneration)) return;
        }
    } else if ((reason & MSYS_CALLBACK_REASON_LBUTTON_DWN) && !(b->uiFlags & BUTTON_IGNORE_CLICKS)) {
        // Deliberately after NEWTOGGLE/anchor/checkbox processing, as original.
        b->uiFlags ^= BUTTON_CLICKED_ON;
    }
    // Preserve source checkbox transition sounds and ordinary press/release
    // sounds; callbacks have already run and may have changed the scheme.
    const bool after=(b->uiFlags&BUTTON_CHECKBOX) ? (b->uiFlags&BUTTON_CLICKED_ON)!=0 : !(reason&MSYS_CALLBACK_REASON_LBUTTON_UP);
    if(b->ubSoundSchemeID && (b->uiFlags&BUTTON_ENABLED)) {
        if ((reason&MSYS_CALLBACK_REASON_LBUTTON_UP) && (soundBefore && !after)) {
            PlayButtonSound(b->IDNum,BUTTON_SOUND_CLICKED_OFF);
            if(!stillLive(token,regionGeneration)) return;
        } else if ((reason&MSYS_CALLBACK_REASON_LBUTTON_DWN) && (!soundBefore && after)) {
            PlayButtonSound(b->IDNum,BUTTON_SOUND_CLICKED_ON);
            if(!stillLive(token,regionGeneration)) return;
        }
    }
    b->uiFlags |= BUTTON_DIRTY;
    if (reason & MSYS_CALLBACK_REASON_LBUTTON_UP) slot.pressed = false;
}
void BtnGenericMouseMoveButtonCallback(GUI_BUTTON* candidate, INT32 reason) {
    CallbackScope scope;
    const Identity token = identity(candidate);
    GUI_BUTTON* b = resolve(token);
    if (!b || !same(token, anchor)) return;
    if (reason & MSYS_CALLBACK_REASON_LOST_MOUSE) {
        if (!anchoredState) {
            setOn(b, false);
            if (b->ubSoundSchemeID) PlayButtonSound(b->IDNum, BUTTON_SOUND_CLICKED_OFF);
            // Playback may remove the button, shutdown UI or replace its region.
            // The enclosing move dispatcher revalidates before further access.
        }
    } else if (reason & MSYS_CALLBACK_REASON_GAIN_MOUSE) {
        setOn(b, true);
        if (b->ubSoundSchemeID) PlayButtonSound(b->IDNum, BUTTON_SOUND_CLICKED_ON);
    }
}
void ReleaseAnchorMode() {
    previousAnchor = {};
    if (GUI_BUTTON* b = resolve(anchor)) {
        if (MSYS_CurrentMX < b->Area.RegionTopLeftX || MSYS_CurrentMX > b->Area.RegionBottomRightX ||
            MSYS_CurrentMY < b->Area.RegionTopLeftY || MSYS_CurrentMY > b->Area.RegionBottomRightY)
            setOn(b, anchoredState);
        previousAnchor = anchor;
    }
    anchor = {};
    for (auto& slot : slots) slot.pressed = false;
}
namespace sgp {
void CancelNativeButtonInput() {
    // No callbacks, no activation; restore state preceding a not-yet-released
    // press, including null callback and NEWTOGGLE combinations.
    for (INT32 id = 0; id < static_cast<INT32>(ButtonList.size()); ++id) {
        GUI_BUTTON* b = live(id);
        if (!b) continue;
        auto& slot = slots[static_cast<std::size_t>(id)];
        if (slot.pressed) setOn(b, slot.prepressOn);
        else if (b->ubToggleButtonActivated) setOn(b, b->ubToggleButtonOldState != 0);
        slot.pressed = false;
        b->ubToggleButtonActivated = FALSE;
        // MSYS clears hover immediately after this bridge. Force its new
        // non-hover image to render even when no press state changed.
        b->uiFlags |= BUTTON_DIRTY;
    }
    anchor = previousAnchor = {};
}
void FinishNativeButtonDispatch() { collect(); }
}
