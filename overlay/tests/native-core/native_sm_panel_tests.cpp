/*
 * P710 native snapshot update: tests/native-core/native_sm_panel_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted magazine and UI cleanup checkpoint dated 2026-11-16.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-16.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Exercise the original inventory-panel lifecycle with authored images only.
#include "Button System.h"
#include "Interface.h"
#include "Interface Panels.h"
#include "Interface Items.h"
#include "Items.h"
#include "vsurface.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>

extern INT32 iSMPanelButtons[NUM_SM_BUTTONS];
extern INT32 giSMStealthImages, giSMStealthButton;
extern INT32 giSMZipperImages, giSMZipperButton;
extern INT32 giSMDropPackImages, giSMDropPackButton;
extern UINT32 guiCurrentItemDescriptionScreen;
extern BOOLEAN gfInKeyRingPopup;
extern OBJECTTYPE* gpItemDescObject;
extern INT32 giItemDescAmmoButtonImages, giItemDescAmmoButton;
extern INT32 giMapInvDescButtonImage, giMapInvDescButton;

namespace {
unsigned checks=0;
void check(bool ok,const char* why) {
    ++checks;
    if (!ok) throw std::runtime_error(why);
}
void cleanup() {
    ShutdownButtonSystem(); MSYS_Shutdown();
    ShutdownVideoSurfaceManager(); ShutdownVideoObjectManager(); fileman::CloseAll();
    vfs::Log::flushDeleteAll(); vfs::CVirtualFileSystem::shutdownVFS();
    vfs::ObjectAllocator::clear();
}
void dynamicButton(INT32& image,INT32& button) {
    image=LoadButtonImage("buttons.sti",4,0,1,2,3);
    button=QuickCreateButton(static_cast<UINT32>(image),2,3,0,31,nullptr,nullptr);
    check(image>=0 && button>=0,"dynamic inventory control");
}
}

int main(int argc,char** argv) {
    try {
        if (argc!=2) throw std::runtime_error("expected authored fixture directory");
        const auto root=std::filesystem::canonical(argv[1]);
        auto dir=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
        check(dir->init(),"fixture directory");
        auto profile=std::make_unique<vfs::CVirtualProfile>("sm-panel",vfs::Path(root.u8string()),false);
        profile->addLocation(dir.get()); auto* location=dir.release(); auto* mounted=profile.get();
        getVFS()->getProfileStack()->pushProfile(profile.release());
        check(getVFS()->addLocation(location,mounted),"mount authored panel images");
        check(InitializeVideoObjectManager() && InitializeVideoSurfaceManager(),"resource managers");
        MSYS_Init(); check(InitButtonSystem(),"button manager");
        for (int cycle=0;cycle<MAX_BUTTON_PICS+1;++cycle) {
            gsCurInterfacePanel=SM_PANEL;
            guiCurrentItemDescriptionScreen=GAME_SCREEN;
            gfInKeyRingPopup=FALSE;
            check(CreateSMPanelButtons(),"all production inventory buttons create");
            for (INT32 id:iSMPanelButtons) check(GetButtonPtr(id)!=nullptr,"inventory button is live");
            dynamicButton(giSMStealthImages,giSMStealthButton);
            dynamicButton(giSMZipperImages,giSMZipperButton);
            dynamicButton(giSMDropPackImages,giSMDropPackButton);
            // These states caused RenderBackpackButtons to skip teardown.
            if (cycle%3==0) gsCurInterfacePanel=TEAM_PANEL;
            if (cycle%3==1) guiCurrentItemDescriptionScreen=MAP_SCREEN;
            if (cycle%3==2) gfInKeyRingPopup=TRUE;
            RemoveSMPanelButtons();
            check(std::none_of(ButtonList.begin(),ButtonList.end(),[](GUI_BUTTON* b){return b!=nullptr;}),
                  "panel teardown removes every control in each context");
            check(std::none_of(ButtonPictures.begin(),ButtonPictures.end(),[](const BUTTON_PICS& p){return p.vobj!=nullptr;}),
                  "panel teardown releases every image");
            for (INT32 id:iSMPanelButtons) check(id==-1,"retired inventory IDs reset");
            RemoveSMPanelButtons();
        }

        // The strategic item-description close path must retire its controls
        // before unloading their images, even outside a button callback.
        gsCurInterfacePanel=TEAM_PANEL; gfInKeyRingPopup=FALSE;
        guiCurrentItemDescriptionScreen=MAP_SCREEN;
        OBJECTTYPE described;
        described.usItem=401; described.ubNumberOfObjects=1;
        Item[401].usItemClass=IC_AMMO;
        gpItemDescObject=&described;
        dynamicButton(giMapInvDescButtonImage,giMapInvDescButton);
        dynamicButton(giItemDescAmmoButtonImages,giItemDescAmmoButton);
        gfInItemDescBox=TRUE;
        DeleteItemDescriptionBox();
        check(!gfInItemDescBox && !gpItemDescObject,"description closure clears its state");
        check(std::none_of(ButtonList.begin(),ButtonList.end(),[](GUI_BUTTON* b){return b!=nullptr;}),
              "description closure removes its controls");
        check(std::none_of(ButtonPictures.begin(),ButtonPictures.end(),[](const BUTTON_PICS& p){return p.vobj!=nullptr;}),
              "description closure releases its images");
        cleanup();
        std::cout<<"PASS: "<<checks<<" production inventory-panel lifecycle assertions\n";
        return 0;
    } catch (const std::exception& e) {
        cleanup(); std::cerr<<"FAIL after "<<checks<<" assertions: "<<e.what()<<'\n'; return 1;
    }
}
