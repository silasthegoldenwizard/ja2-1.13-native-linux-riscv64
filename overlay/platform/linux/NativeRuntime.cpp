/*
 * P710 native snapshot update: platform/linux/NativeRuntime.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

/*
 * P710 native snapshot update: platform/linux/NativeRuntime.cpp
 * Native Linux source/build/test adaptations captured in the November 20-21 save-load and crash-report checkpoint.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-21.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Native outer entry for the genuine v1.13 engine. Windows sgp.cpp remains intact.
#include "NativeRuntime.h"
#include "native_crash_report.h"
#include "types.h"
#include "sgp.h"
#include "FileMan.h"
#include "MemMan.h"
#include "input.h"
#include "Font.h"
#include "NativeFontTranslation.h"
#include "random.h"
#include "gameloop.h"
#include "soundman.h"
#include "Button System.h"
#include "mousesystem.h"
#include "JA2 Splash.h"
#include "Timer Control.h"
#include "GameSettings.h"
#include "GameVersion.h"
#include "INIReader.h"
#include "Intro.h"
#include "Music Control.h"
#include "Text.h"
#include "ExportStrings.h"
#include "ImportStrings.h"
#include "structure.h"
#include "lua_state.h"
#include "language.hpp"
#include <vfs/Aspects/vfs_settings.h>
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_init.h>
#include <vfs/Tools/vfs_log.h>
#include <algorithm>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <exception>
#include <iostream>
#include <list>
#include <stdexcept>
#include <system_error>

namespace fs = std::filesystem;
BOOLEAN gfApplicationActive=FALSE;
BOOLEAN gfProgramIsRunning=FALSE;
BOOLEAN gfGameInitialized=FALSE;
BOOLEAN gfDontUseDDBlits=FALSE;
BOOLEAN gfIgnoreMessages=FALSE;
CHAR8 gzCommandLine[100]{};
CHAR8 gzErrorMsg[2048]{};
UINT32 guiMouseWheelMsg=0;
int iWindowedMode=1;
bool s_bExportStrings=false;
bool g_bUseXML_Structures=false;
extern bool g_bUseXML_Strings, g_bUsePngItemImages;
extern int iScreenMode;
extern INT32 bScreenModeCmdLine;

namespace {
p710::NativeOptions activeOptions;
std::vector<fs::path> activeConfigs;
vfs::Path forcedStructure;
struct Services {
    bool debug=false, memory=false, file=false, video=false, objects=false;
    bool input=false, vfs=false, clock=false, fonts=false, sound=false, gameClock=false;
    bool music=false, gameAttempted=false, shuttingDown=false;
} services;
[[noreturn]] void fail(const std::string& text) { throw std::runtime_error(text); }
bool within(const fs::path& child,const fs::path& root) {
    auto a=child.begin(),b=root.begin();
    for (;b!=root.end();++a,++b) if(a==child.end() || *a!=*b) return false;
    return true;
}
fs::path absoluteDirectory(const fs::path& input) {
    if (!input.is_absolute()) fail("Every native root must be an explicit absolute directory: "+input.string());
    fs::path walk;
    // Inspect the exact traversal used by canonical(), before a later ".."
    // could lexically cancel a symlink component. Ordinary directory/.. paths
    // remain valid; a symlink is rejected when it is first encountered.
    for (const auto& part: input) {
        walk/=part;
        if (fs::is_symlink(fs::symlink_status(walk))) fail("Symlink root component is unsupported: "+walk.string());
    }
    const auto result=fs::canonical(input);
    if (!fs::is_directory(result) || result==result.root_path()) fail("Native root must be a non-root directory: "+input.string());
    return result;
}
void validateTree(const fs::path& root,bool writable) {
    for (const auto& entry:fs::recursive_directory_iterator(root)) {
        const auto status=entry.symlink_status();
        if (fs::is_symlink(status)) fail("Symlinks in native roots are unsupported: "+entry.path().string());
        if (!fs::is_regular_file(status) && !fs::is_directory(status)) fail("Native roots support regular files/directories only: "+entry.path().string());
        if (writable && fs::is_regular_file(status) && fs::hard_link_count(entry.path())!=1)
            fail("Writable profile cannot contain multiply-linked files: "+entry.path().string());
    }
}
fs::path profileFile(const p710::NativeOptions& options,const fs::path& value) {
    const auto file=fs::canonical(value.is_absolute()?value:options.settings/value);
    if (!within(file,fs::canonical(options.profile)) || !fs::is_regular_file(file)) fail("Configuration must be an existing file in the isolated profile: "+file.string());
    return file;
}
void virtualPath(const vfs::Path& value) {
    const std::string name=vfs::String::as_utf8(value());
    if (name.empty()) return;
    if (name.front()=='/' || name.front()=='\\' || name.find(':')!=std::string::npos) fail("Absolute virtual VFS path is unsupported");
    std::string portable=name;
    std::replace(portable.begin(),portable.end(),'\\','/');
    for(const auto& part:fs::path(portable)) if(part=="..") fail("Parent traversal in virtual VFS path is unsupported");
}
void addWritable(vfs_init::VfsConfig& config,const fs::path& root,const vfs::String& name) {
    auto* profile=new vfs_init::Profile;
    config.addProfile(profile,true);
    profile->m_name=name; profile->m_root=vfs::Path(root.u8string()); profile->m_writable=true;
    auto* location=new vfs_init::Location;
    profile->addLocation(location,true); location->m_type="DIRECTORY";
}
void require(BOOLEAN result,const char* stage) { if(result!=TRUE) fail(std::string("Original/native initialization failed: ")+stage); }
void settingsList(vfs::PropertyContainer& props,const wchar_t* key,std::list<vfs::String>& list) {
    list.clear(); props.getStringListProperty(L"Ja2 Settings",key,list,L"");
}
}

bool p710::ContinueNativeWindowedFrame(bool running,bool quitRequested,bool inputTerminated,bool /*inputFocused*/) noexcept {
    // Original windowed sgp.cpp keeps the application active on focus loss.
    // NativeInput owns inactive-input suppression; clocks and game frames must
    // continue together, with no new pause/focus policy at this outer boundary.
    return running && !quitRequested && !inputTerminated;
}

bool p710::NativeScaledGameDimensions(int displayWidth,int displayHeight,int uiScale,
                                      int& logicalWidth,int& logicalHeight) noexcept {
    if (displayWidth<=0 || displayHeight<=0 || uiScale<1 || uiScale>4) return false;
    std::int64_t w=std::max(1280,displayWidth/uiScale),h=std::max(720,displayHeight/uiScale);
    if (w*displayHeight<h*displayWidth) w=(h*displayWidth+displayHeight-1)/displayHeight;
    else h=(w*displayHeight+displayWidth-1)/displayWidth;
    // Original centered panel offsets assume an even difference from the base.
    w=(w+1)&~std::int64_t(1);h=(h+1)&~std::int64_t(1);
    if (w>32768 || h>32768) return false;
    logicalWidth=static_cast<int>(w);logicalHeight=static_cast<int>(h);return true;
}

const char* p710::NativeUsage() {
    return "Usage: p710_native_game --profile DIR --temp DIR/Maps/Temp --settings DIR --assets DIR [--assets DIR ...] [--check-profile] [--KEY=VALUE] [/NOSOUND] [/WINDOW|/FULLSCREEN]\n"
           "All roots must already exist, be absolute and contain no symlinks. Settings must equal profile; temp must equal profile/Maps/Temp. Assets are separate read-only VFS roots.\n"
           "Place Ja2.ini, Language.ini and original VFS INI files in the isolated profile. Existing game installations are outside this entry's authorization.\n"
           "--help and --check-profile never initialize original game services or write settings.\n"
           "/FULLSCREEN uses borderless desktop mode. --NATIVE_DISPLAY=upper selects the upper monitor; --NATIVE_UI_SCALE=2 enlarges the UI (range1-4; fullscreen default2).\n";
}
p710::NativeOptions p710::ParseNativeOptions(int argc,const char* const* argv) {
    NativeOptions out;
    for(int i=1;i<argc;++i) {
        if(!argv[i]) fail("Null command-line argument");
        std::string arg=argv[i];
        if(arg=="--help" || arg=="-h") { out.help=true; continue; }
        if(arg=="--check-profile") { out.checkProfile=true; continue; }
        if(arg=="--profile" || arg=="--temp" || arg=="--settings" || arg=="--assets") {
            if(++i>=argc || !argv[i] || !*argv[i]) fail("Missing root for "+arg);
            if(arg=="--assets") out.assets.emplace_back(argv[i]);
            else {
                auto& target=arg=="--profile"?out.profile:arg=="--temp"?out.temp:out.settings;
                if(!target.empty()) fail("Duplicate root option "+arg);
                target=argv[i];
            }
            continue;
        }
        if(arg=="/NOSOUND") { out.noSound=true; }
        else if(arg=="/WINDOW") out.overrides.emplace_back("SCREEN_MODE_WINDOWED","1");
        else if(arg=="/FULLSCREEN") out.overrides.emplace_back("SCREEN_MODE_WINDOWED","0");
        else {
            std::size_t start=arg.rfind("--",0)==0?2:arg.rfind("-",0)==0?1:arg.rfind("/",0)==0?1:0;
            auto equal=arg.find_first_of("=:",start);
            if(!start || equal==std::string::npos || equal==start || equal+1==arg.size()) fail("Unknown argument (settings use --KEY=VALUE): "+arg);
            out.overrides.emplace_back(arg.substr(start,equal-start),arg.substr(equal+1));
        }
        if(!out.gameCommandLine.empty()) out.gameCommandLine+=' ';
        out.gameCommandLine+=arg;
    }
    return out;
}
void p710::ValidateNativeProfile(const NativeOptions& options) {
    const auto profile=absoluteDirectory(options.profile);
    const auto settings=absoluteDirectory(options.settings);
    const auto temp=absoluteDirectory(options.temp);
    if(settings!=profile || temp!=profile/"Maps"/"Temp") fail("Settings must be profile root and temp must be profile/Maps/Temp");
    if(options.assets.empty()) fail("At least one explicitly read-only asset root is required");
    validateTree(profile,true);
    for(const auto& item:options.assets) {
        const auto assets=absoluteDirectory(item);
        if(within(assets,profile)||within(profile,assets)) fail("Asset/profile overlap is forbidden");
        validateTree(assets,false);
    }
    for(const auto* name:{"Temp","ShadeTables"})
        if(!fs::is_directory(profile/name)) fail(std::string("Create the isolated writable subdirectory before startup: ")+name);
}

void p710::InitializeNativeVFS(const NativeOptions& options,const std::vector<fs::path>& files) {
    ValidateNativeProfile(options);
    if(files.empty()) fail("Original VFS configuration list is empty");
    vfs::PropertyContainer props;
    for(const auto& file:files) if(!props.initFromIniFile(vfs::Path(profileFile(options,file).u8string()))) fail("Cannot read original VFS configuration");
    std::list<vfs::String> profiles;
    props.getStringListProperty(L"vfs_config",L"PROFILES",profiles,L"");
    if(profiles.empty()) fail("Original VFS configuration declares no profiles");
    vfs_init::VfsConfig config;
    bool writable=false;
    for(const auto& id:profiles) {
        const vfs::String section=vfs::String("PROFILE_")+id;
        const auto name=props.getStringProperty(section,L"NAME",id);
        if(props.getBoolProperty(section,L"WRITE",false)) {
            if(writable) fail("Original VFS configuration declares multiple writable profiles");
            writable=true; addWritable(config,fs::canonical(options.profile),name); continue;
        }
        auto root=fs::path(props.getStringProperty(section,L"PROFILE_ROOT",L"").utf8());
        if(root.empty()) fail("Readonly original VFS profile needs an explicit root");
        if(root.is_relative()) root=options.settings/root;
        root=absoluteDirectory(root);
        bool allowed=false;
        for(const auto& assets:options.assets) allowed|=within(root,fs::canonical(assets));
        if(!allowed) fail("Original VFS asset profile is outside explicit asset roots: "+root.string());
        auto* profile=new vfs_init::Profile; config.addProfile(profile,true);
        profile->m_name=name; profile->m_root=vfs::Path(root.u8string());
        std::list<vfs::String> locations;
        props.getStringListProperty(section,L"LOCATIONS",locations,L"");
        if(locations.empty()) fail("Original readonly VFS profile has no locations");
        for(const auto& id:locations) {
            const vfs::String section=vfs::String("LOC_")+id;
            auto* location=new vfs_init::Location; profile->addLocation(location,true);
            location->m_path=props.getStringProperty(section,L"PATH",L"");
            location->m_vfs_path=props.getStringProperty(section,L"VFS_PATH",L"");
            location->m_mount_point=props.getStringProperty(section,L"MOUNT_POINT",L"");
            location->m_type=props.getStringProperty(section,L"TYPE",L"");
            location->m_optional=props.getBoolProperty(section,L"OPTIONAL",false);
            virtualPath(location->m_path); virtualPath(location->m_vfs_path); virtualPath(location->m_mount_point);
            if(!vfs::StrCmp::Equal(location->m_type,L"DIRECTORY") && !vfs::StrCmp::Equal(location->m_type,L"LIBRARY")) fail("Unsupported original VFS location type");
            if(!location->m_path.empty()) {
                std::string relative=vfs::String::as_utf8(location->m_path()); std::replace(relative.begin(),relative.end(),'\\','/');
                const auto target=fs::weakly_canonical(root/relative);
                if(!within(target,root)) fail("Original VFS location escapes its readonly root");
            }
        }
    }
    if(!writable) addWritable(config,fs::canonical(options.profile),"P710_NATIVE_PROFILE");
    if(!vfs_init::initVirtualFileSystem(config)) fail("Original VFS initialization failed");
    for(const char* name:{"Temp","Maps/Temp","ShadeTables"}) getVFS()->getVirtualLocation(vfs::Path(name),true)->setIsExclusive(true);

}

void GetRuntimeSettings() {
    vfs::PropertyContainer props;
    if(!props.initFromIniFile(vfs::Path(profileFile(activeOptions,GAME_INI_FILE).u8string()))) fail("Cannot read isolated Ja2.ini");
    for(const auto& item:activeOptions.overrides) props.setStringProperty("Ja2 Settings",item.first,item.second);
    // The native entry has no crash-upload or deliberate crash backend. Never
    // silently invoke a configured network action through startup configuration.
    if(!props.getStringProperty("Ja2 Settings",L"CRASH_TELEMETRY_URL").empty() || !props.getStringProperty("Ja2 Settings",L"CRASH_TEST").empty())
        fail("Native crash telemetry/test options are unsupported");
    const auto locale=props.getStringProperty("Ja2 Settings",L"LOCALE");
    if(!locale.empty() && !std::setlocale(LC_ALL,locale.utf8().c_str())) fail("Configured locale is unavailable");
    vfs::PropertyContainer languages;
    const auto languageFile=activeOptions.settings/std::string(LANGUAGE_INI_FILE);
    if(fs::exists(languageFile) && !languages.initFromIniFile(vfs::Path(profileFile(activeOptions,languageFile).u8string()))) fail("Cannot read isolated Language.ini");
    // Retain original precedence: Language.ini overrides Ja2.ini/command line.
    auto language=props.getStringProperty("Ja2 Settings",L"LANGUAGE");
    const auto languageIni=languages.getStringProperty("Language",L"LANGUAGE");
    if(!languageIni.empty()) language=languageIni;
    SetLanguageFromName(language.utf8()); BindLanguageStrings(g_lang);
    vfs::Settings::setUseUnicode(!props.getBoolProperty("Ja2 Settings",L"VFS_NO_UNICODE",false));
    const auto indirect=props.getStringProperty("Ja2 Settings",L"VFS_CONFIG");
    if(!indirect.empty()) {
        vfs::PropertyContainer config;
        if(!config.initFromIniFile(vfs::Path(profileFile(activeOptions,indirect.utf8()).u8string()))) fail("Cannot read VFS_CONFIG");
        const auto files=config.getStringProperty("vfs_config",L"VFS_CONFIG_INI");
        if(!files.empty()) props.setStringProperty("Ja2 Settings",L"VFS_CONFIG_INI",files);
    }
    std::list<vfs::String> list;
    settingsList(props,L"VFS_CONFIG_INI",list);
    if(list.empty()) list.emplace_back("vfs_config.ini");
    activeConfigs.clear();
    for(const auto& file:list) activeConfigs.push_back(profileFile(activeOptions,file.utf8()));
    for(const auto* key:{L"MERGE_INI_FILES",L"MERGE_INI_FILES_UB"}) {
        settingsList(props,key,list);
        for(const auto& file:list) { virtualPath(vfs::Path(file)); CIniReader::RegisterFileForMerging(file); }
    }
    g_bUsePngItemImages=props.getBoolProperty("Ja2 Settings",L"USE_PNG_ITEM_IMAGES",false);
    g_bUseXML_Structures=props.getBoolProperty("Ja2 Settings",L"USE_XML_STRUCTURES",false);
    g_bUseXML_Strings=props.getBoolProperty("Ja2 Settings",L"USE_XML_STRINGS",false);
    s_bExportStrings=props.getBoolProperty("Ja2 Settings",L"EXPORT_STRINGS",false);
    forcedStructure=props.getStringProperty("Ja2 Settings",L"FORCE_LOAD_JSD_XML_FILE",L"");
    virtualPath(forcedStructure);
    iResolution=static_cast<int>(props.getIntProperty("Ja2 Settings",L"SCREEN_RESOLUTION",-1));
    static constexpr int sizes[][2]={{640,480},{960,540},{800,600},{1024,600},{1280,720},{1024,768},{1280,768},{1360,768},{1366,768},{1280,800},{1440,900},{1600,900},{1280,960},{1440,960},{1770,1000},{1280,1024},{1360,1024},{1600,1024},{1440,1050},{1680,1050},{1920,1080},{1600,1200},{1920,1200},{2560,1440},{2560,1600}};
    const auto requestedMode=props.getIntProperty("Ja2 Settings",L"SCREEN_MODE_WINDOWED",1);
    if (requestedMode!=0 && requestedMode!=1) fail("Native display mode must be windowed1 or borderless0");
    sgp::NativePresentationOptions presentation;
    presentation.borderless=requestedMode==0;
    const auto target=props.getStringProperty("Ja2 Settings",L"NATIVE_DISPLAY",L"primary").utf8();
    if (target=="upper") presentation.display=sgp::NativeDisplayTarget::Upper;
    else if (target!="primary") fail("NATIVE_DISPLAY must be primary or upper");
    const auto uiScale=props.getIntProperty("Ja2 Settings",L"NATIVE_UI_SCALE",presentation.borderless?2:1);
    if (uiScale<1 || uiScale>4) fail("NATIVE_UI_SCALE must be between1 and4");
    int width,height;
    if (presentation.borderless) {
        SDL_Rect bounds{};
        if (!sgp::QueryNativeDisplayBounds(presentation.display,bounds) ||
            !p710::NativeScaledGameDimensions(bounds.w,bounds.h,static_cast<int>(uiScale),width,height))
            fail("Cannot derive native layout for the selected display");
        iResolution=width>=1024 && height>=768?_1024x768:width>=1280 && height>=720?_1280x720:
                    width>=800 && height>=600?_800x600:_640x480;
    } else if(iResolution==_CustomRes) {
        width=std::max(640,static_cast<int>(props.getIntProperty("Ja2 Settings",L"CUSTOM_SCREEN_RESOLUTION_X",640)));
        height=std::max(480,static_cast<int>(props.getIntProperty("Ja2 Settings",L"CUSTOM_SCREEN_RESOLUTION_Y",480)));
        iResolution=width<800 || height<600?_640x480:width<1024 || height<768?_800x600:_1024x768;
    } else {
        if(iResolution<0 || iResolution>=_CustomRes) iResolution=_800x600;
        width=sizes[iResolution][0]; height=sizes[iResolution][1];
    }
    if(width>32768 || height>32768) fail("Resolution exceeds native input geometry");
    // Window decorations are outside SDL's logical framebuffer. The original
    // Win32 maximize subtraction (16x70) must not shrink native game geometry.
    SCREEN_WIDTH=static_cast<UINT16>(width); SCREEN_HEIGHT=static_cast<UINT16>(height);
    iScreenWidthOffset=(width-640)/2; iScreenHeightOffset=(height-480)/2;
    const int baseWidth=iResolution<_800x600?640:iResolution<_1024x768?800:1024;
    const int baseHeight=iResolution<_800x600?480:iResolution<_1024x768?600:768;
    xResOffset=(width-baseWidth)/2; yResOffset=(height-baseHeight)/2;
    xResSize=width-2*xResOffset; yResSize=height-2*yResOffset;
    // SDL desktop fullscreen retains the original windowed engine/frame policy.
    // Presentation alone owns fullscreen state and monitor bounds.
    iWindowedMode=1;
    iScreenMode=iWindowedMode; bScreenModeCmdLine=TRUE;
    iPlayIntro=static_cast<int>(props.getIntProperty("Ja2 Settings",L"PLAY_INTRO",iPlayIntro));
    iUseWinFonts=static_cast<int>(props.getIntProperty("Ja2 Settings",L"USE_WINFONTS",iUseWinFonts));
    fTooltipScaleFactor=static_cast<float>(props.getFloatProperty("Ja2 Settings",L"TOOLTIP_SCALE_FACTOR",100))/100;
    if(!std::isfinite(fTooltipScaleFactor)) fail("Invalid tooltip scale");
    fTooltipScaleFactor=std::max(1.0f,fTooltipScaleFactor);
    iDisableMouseScrolling=static_cast<int>(props.getIntProperty("Ja2 Settings",L"DISABLE_MOUSE_SCROLLING",iDisableMouseScrolling));
    SetHiSpeedClockMode(TRUE);
    if(!sgp::ConfigureNativeVideo(SCREEN_WIDTH,SCREEN_HEIGHT,16,
        presentation.borderless?1:static_cast<UINT32>(uiScale),APPLICATION_NAME,presentation)) fail("Native video configuration rejected");
}

BOOLEAN InitializeStandardGamingPlatform(void* instance,int show) {
    if(instance || show) fail("Native platform accepts no Windows instance/show callback");
    p710::ValidateNativeProfile(activeOptions); // before Debug/FileMan/game writes
    services.vfs=true; // PropertyContainer uses the VFS singleton even before mounts
    GetRuntimeSettings();
    require(InitializeDebugManager()?TRUE:FALSE,"debug manager"); services.debug=true;
    require(InitializeMemoryManager(),"memory manager"); services.memory=true;
    require(InitializeFileManager(nullptr),"file manager"); services.file=true;
    // Native input attaches to an existing SDL-owned window; Win32 used hooks
    // before creating its window. Keep all original game-facing APIs otherwise.
    require(InitializeVideoObjectManager(),"video object manager"); services.objects=true;
    require(InitializeVideoManager(nullptr,0,nullptr),"video manager"); services.video=true;
    require(InitializeInputManager(),"input manager"); services.input=true;
    services.vfs=true; // rollback even when original VFS init throws partway
    p710::InitializeNativeVFS(activeOptions,activeConfigs);
    // Match the original language-selected save-directory isolation policy.
    for(auto index:{MSG_SAVEDIRECTORY,MSG_MPSAVEDIRECTORY}) {
        if(!pMessageStrings[index] || std::wcslen(pMessageStrings[index])<3) fail("Original language save directory is unavailable");
        getVFS()->getVirtualLocation(vfs::Path(pMessageStrings[index]+3),true)->setIsExclusive(true);
    }
    if(!forcedStructure.empty()) {
        auto file=vfs::String::as_utf8(forcedStructure());
        if(!LoadStructureFile(file.data())) fail("Original forced structure load failed");
    }
    if(g_bUseXML_Strings) { if(s_bExportStrings) Loc::ExportStrings(); Loc::ImportStrings(); }
    InitJA2SplashScreen();
    require(InitializeClockManager(),"SGP clock"); services.clock=true;
    auto* table=CreateEnglishTransTable();
    if(!table) fail("Original font translation allocation failed");
    const auto fontResult=InitializeFontManager(16,table);
    DestroyNativeFontTranslationTable(table); // native manager owns a deep copy
    require(fontResult,"font manager"); services.fonts=true;
    if(activeOptions.noSound) SoundEnableSound(FALSE);
    require(InitializeSoundManager(),"sound manager"); services.sound=true;
    services.music=true;
    InitializeMusicLists();
    require(InitializeJA2Clock(),"JA2 gameplay clock"); services.gameClock=true;
    services.gameAttempted=true;
    // ERROR_SCREEN is3, not TRUE. Never promote that failure into success.
    require(InitializeGame(),"genuine game initialization");
    gfGameInitialized=TRUE;
    return TRUE;
}

void ShutdownStandardGamingPlatform() {
    if(services.shuttingDown) return;
    services.shuttingDown=true;
    gfProgramIsRunning=FALSE; gfApplicationActive=FALSE;
    std::exception_ptr firstFailure;
    auto attempt=[&](auto action) { try { action(); } catch(...) { if(!firstFailure) firstFailure=std::current_exception(); } };
    attempt([] { ClearTimerNotifyCallbacks(); });
    if(services.gameClock) { services.gameClock=false; attempt([] { ShutdownJA2Clock(); }); }
    if(services.sound) attempt([] { SoundServiceStreams(); });
    // Original ShutdownGame assumes all screen/game initialization completed.
    // Partial failure is reported and tears down platform managers only; it must
    // never execute a synthetic successful-game shutdown/save operation.
    if(gfGameInitialized) { gfGameInitialized=FALSE; attempt([] { ShutdownGame(); }); }
    if(services.gameAttempted) {
        services.gameAttempted=false;
        attempt([] { LuaState::CLOSE_ALL(); });
        attempt([] { ShutdownButtonSystem(); }); attempt([] { MSYS_Shutdown(); });
    }
    if(services.sound) { services.sound=false; attempt([] { ShutdownSoundManager(); }); }
    if(services.music) { services.music=false; attempt([] { ShutdownMusicLists(); }); }
    if(services.fonts) { services.fonts=false; attempt([] { ShutdownFontManager(); }); }
    if(services.clock) { services.clock=false; attempt([] { ShutdownClockManager(); }); }
    if(services.input) { services.input=false; attempt([] { ShutdownInputManager(); }); }
    if(services.video) { services.video=false; attempt([] { ShutdownVideoManager(); }); }
    attempt([] { ShutdownVideoSurfaceManager(); });
    if(services.objects) { services.objects=false; attempt([] { ShutdownVideoObjectManager(); }); }
    if(services.file) { services.file=false; attempt([] { ShutdownFileManager(); }); }
    if(services.vfs) {
        services.vfs=false;
        attempt([] { vfs::Log::flushDeleteAll(); });
        attempt([] { vfs::CVirtualFileSystem::shutdownVFS(); });
        attempt([] { vfs::ObjectAllocator::clear(); });
    }
    if(services.memory) { services.memory=false; attempt([] { ShutdownMemoryManager(); }); }
    if(services.debug) { services.debug=false; attempt([] { ShutdownDebugManager(); }); }
    if(firstFailure) std::rethrow_exception(firstFailure);
}
void CreateStandardGamingPlatform(void*) {
    // The original window-created hook starts the gameplay clock. Native outer
    // init starts it directly, before original InitializeGame uses timer options.
    if(!services.gameClock) { require(InitializeJA2Clock(),"JA2 gameplay clock"); services.gameClock=true; }
}
void SGPExit() { ShutdownStandardGamingPlatform(); }
void ShutdownWithErrorBox(CHAR8* message) {
    std::snprintf(gzErrorMsg,sizeof(gzErrorMsg),"%s",message?message:"Unknown engine error");
    gfIgnoreMessages=TRUE; gfProgramIsRunning=FALSE;
    fail(gzErrorMsg);
}
int p710::RunNativeMain(int argc,const char* const* argv) {
    fs::path oldDirectory;
    bool changedDirectory=false, initializationAttempted=false, crashReportingReady=false;
    int result=0;
    try {
        auto options=ParseNativeOptions(argc,argv);
        if(options.help) { std::cout<<NativeUsage(); return 0; }
        ValidateNativeProfile(options);
        if(options.checkProfile) { std::cout<<"Native profile topology valid; game initialization was not invoked.\n"; return 0; }
        // Fail profile-local missing configuration before changing process state.
        (void)profileFile(options,GAME_INI_FILE);
        oldDirectory=fs::current_path();
        fs::current_path(fs::canonical(options.settings)); changedDirectory=true;
        sgp::InstallNativeCrashReporter(czVersionString); crashReportingReady=true;
        activeOptions=std::move(options); services={};
        std::snprintf(gzCommandLine,sizeof(gzCommandLine),"%s",activeOptions.gameCommandLine.c_str());
        InitializeRandom();
        initializationAttempted=true;
        require(InitializeStandardGamingPlatform(nullptr,0),"standard gaming platform");
        if(g_lang==i18n::Lang::en) SetIntroType(INTRO_SPLASH);
        gfApplicationActive=TRUE; gfProgramIsRunning=TRUE;
        while(gfProgramIsRunning) {
            sgp::PumpNativeVideoEvents();
            if(!ContinueNativeWindowedFrame(gfProgramIsRunning,sgp::NativeVideoQuitRequested(),
                sgp::NativeInputTerminated(),sgp::NativeInputFocused())) { gfProgramIsRunning=FALSE; break; }
            gfApplicationActive=TRUE;
            PumpJA2Clock(); SoundServiceStreams();
            GameLoop();
            if(gzErrorMsg[0]) fail(gzErrorMsg);
            SDL_Delay(1);
        }
    } catch(const std::exception& error) {
        if(crashReportingReady) sgp::WriteNativeRuntimeFailure("native startup/runtime failure",error.what());
        std::cerr<<"Native startup/runtime failure: "<<error.what()<<'\n'; result=1;
    } catch(const char* error) {
        if(crashReportingReady) sgp::WriteNativeRuntimeFailure("native engine failure",error?error:"unknown");
        std::cerr<<"Native engine failure: "<<(error?error:"unknown")<<'\n'; result=1;
    } catch(...) {
        if(crashReportingReady) sgp::WriteNativeRuntimeFailure("native engine failure","unknown exception");
        std::cerr<<"Native engine failure: unknown exception\n"; result=1;
    }
    if(initializationAttempted) {
        try { ShutdownStandardGamingPlatform(); }
        catch(const std::exception& error) {
            if(crashReportingReady) sgp::WriteNativeRuntimeFailure("native shutdown failure",error.what());
            std::cerr<<"Native shutdown failure: "<<error.what()<<'\n'; result=1;
        }
        catch(...) {
            if(crashReportingReady) sgp::WriteNativeRuntimeFailure("native shutdown failure","unknown exception");
            std::cerr<<"Native shutdown failure\n"; result=1;
        }
    }
    if(changedDirectory) { std::error_code error; fs::current_path(oldDirectory,error); if(error) { std::cerr<<"Cannot restore original directory\n"; result=1; } }
    return result;
}
