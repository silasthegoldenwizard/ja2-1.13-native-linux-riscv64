/*
 * P710 native snapshot update: tests/native-core/native_runtime_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "../../platform/linux/NativeRuntime.h"
#include "NativeFontTranslation.h"
#include "MemMan.h"
#include "FileMan.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Tools/vfs_log.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs=std::filesystem;
namespace {
unsigned checks=0;
void check(bool value,const char* message) { ++checks; if(!value) throw std::runtime_error(message); }
template<class F> bool rejects(F f) { try { f(); return false; } catch(const std::exception&) { return true; } }
void put(const fs::path& file,const std::string& text) { std::ofstream out(file,std::ios::binary); out<<text; if(!out) throw std::runtime_error("fixture write failed"); }
std::string bytes(const fs::path& file) { std::ifstream in(file,std::ios::binary); if(!in) throw std::runtime_error("fixture read failed"); return {std::istreambuf_iterator<char>(in),{}}; }
void reset() { fileman::CloseAll(); vfs::Log::flushDeleteAll(); vfs::CVirtualFileSystem::shutdownVFS(); vfs::ObjectAllocator::clear(); }
void profile(const fs::path& root) {
    fs::create_directories(root/"profile/Maps/Temp"); fs::create_directory(root/"profile/Temp"); fs::create_directory(root/"profile/ShadeTables"); fs::create_directory(root/"assets");
    p710::NativeOptions options;
    options.profile=root/"profile"; options.settings=options.profile; options.temp=options.profile/"Maps/Temp"; options.assets={root/"assets"};
    const auto cwd=fs::current_path();
    p710::ValidateNativeProfile(options);
    check(fs::current_path()==cwd,"preflight leaves cwd unchanged");
    auto bad=options; bad.profile="relative";
    check(rejects([&]{p710::ValidateNativeProfile(bad);}),"reject relative profile before init");
    bad=options; bad.assets={options.profile};
    check(rejects([&]{p710::ValidateNativeProfile(bad);}),"reject equal asset/profile roots");
    bad=options; bad.assets={root};
    check(rejects([&]{p710::ValidateNativeProfile(bad);}),"reject asset ancestor of writable root");
    bad=options; bad.assets={options.temp};
    check(rejects([&]{p710::ValidateNativeProfile(bad);}),"reject asset inside writable root");
    bad=options; bad.settings=root/"assets";
    check(rejects([&]{p710::ValidateNativeProfile(bad);}),"reject settings outside writable profile");
    bad=options; bad.temp=root/"assets";
    check(rejects([&]{p710::ValidateNativeProfile(bad);}),"reject temp outside isolated topology");
    fs::create_directory_symlink(root/"assets",options.profile/"escape");
    check(rejects([&]{p710::ValidateNativeProfile(options);}),"reject profile symlink before delete/write paths exist");
    fs::remove(options.profile/"escape");
    put(root/"assets/immutable.dat","asset bytes");
    fs::create_hard_link(root/"assets/immutable.dat",options.profile/"alias.dat");
    check(rejects([&]{p710::ValidateNativeProfile(options);}),"reject writable hardlink to asset");
    fs::remove(options.profile/"alias.dat");
    // Counterexample to normalizing before inspecting symlink components:
    // base/link/../profile lexically names base/profile, but resolves through
    // link to target/profile. Both exist and otherwise have valid topology.
    fs::create_directories(root/"lexical-base/profile");
    fs::create_directories(root/"symlink-target/sub");
    fs::create_directories(root/"symlink-target/profile/Maps/Temp");
    fs::create_directory(root/"symlink-target/profile/Temp");
    fs::create_directory(root/"symlink-target/profile/ShadeTables");
    fs::create_directory_symlink(root/"symlink-target/sub",root/"lexical-base/link");
    bad=options;
    bad.profile=root/"lexical-base/link/../profile"; bad.settings=bad.profile; bad.temp=bad.profile/"Maps/Temp";
    check(bad.profile.lexically_normal()==root/"lexical-base/profile" && fs::canonical(bad.profile)==root/"symlink-target/profile",
        "generated canceled-symlink counterexample resolves through a different ancestor");
    check(rejects([&]{p710::ValidateNativeProfile(bad);}),"reject symlink before later parent component can cancel it lexically");
    fs::create_directory(root/"ordinary-child");
    auto ordinary=options;
    ordinary.profile=root/"ordinary-child/../profile"; ordinary.settings=ordinary.profile; ordinary.temp=ordinary.profile/"Maps/Temp";
    p710::ValidateNativeProfile(ordinary);
    check(fs::canonical(ordinary.profile)==options.profile,"ordinary existing directory/../profile remains valid");
    const char* argv[]={"game","--profile",nullptr,"--settings",nullptr,"--temp",nullptr,"--assets",nullptr,"--check-profile","--LANGUAGE=GERMAN","/NOSOUND"};
    const auto p=options.profile.string(),t=options.temp.string(),a=(root/"assets").string();
    argv[2]=p.c_str(); argv[4]=p.c_str(); argv[6]=t.c_str(); argv[8]=a.c_str();
    auto parsed=p710::ParseNativeOptions(12,argv);
    check(parsed.profile==options.profile && parsed.checkProfile && parsed.noSound && parsed.overrides.size()==1,"parse roots, dry mode and original option overrides");
    const char* help[]={"game","--help"}; check(p710::ParseNativeOptions(2,help).help,"help does not require game roots");
    const char* missing[]={"game","--profile"}; check(rejects([&]{p710::ParseNativeOptions(2,missing);}),"reject incomplete root argument");
    const char* full[]={"game","/FULLSCREEN"}; check(rejects([&]{p710::ParseNativeOptions(2,full);}),"unsupported fullscreen fails explicitly");
    const auto config=options.profile/"vfs_config.ini";
    put(config,"[vfs_config]\nPROFILES = assets,user\n[PROFILE_assets]\nNAME = assets\nPROFILE_ROOT = "+a+"\nLOCATIONS = assets\n[LOC_assets]\nTYPE = DIRECTORY\nPATH = \n[PROFILE_user]\nNAME = isolated_user\nWRITE = true\nPROFILE_ROOT = /must/never/be/used\n");
    put(root/"assets/Temp-policy.txt","read only remains");
    p710::InitializeNativeVFS(options,{config});
    check(FileExists("immutable.dat"),"actual original VFS sees readonly asset");
    check(!FileDelete("immutable.dat"),"actual FileMan cannot delete readonly asset");
    char file[]="Temp/generated.tmp";
    HWFILE handle=FileOpen(file,FILE_ACCESS_WRITE|FILE_OPEN_ALWAYS,FALSE);
    check(handle!=0,"actual FileMan creates isolated temp through original write profile");
    UINT32 count=0; char value[]="native profile";
    check(FileWrite(handle,value,14,&count) && count==14,"actual FileMan writes exact isolated temp bytes"); FileClose(handle);
    check(bytes(options.profile/"Temp/generated.tmp")=="native profile","host temp resides in isolated profile");
    check(bytes(root/"assets/immutable.dat")=="asset bytes","readonly asset bytes survive write/delete tests");
    check(EraseDirectory("Temp"),"original temp cleanup API removes writable temp");
    check(!fs::exists(options.profile/"Temp/generated.tmp"),"cleanup reaches only generated writable temp");
    reset();
    const auto before=bytes(config);
    put(config,"[vfs_config]\nPROFILES = evil\n[PROFILE_evil]\nNAME = evil\nPROFILE_ROOT = "+root.string()+"\nLOCATIONS = escape\n[LOC_escape]\nTYPE = DIRECTORY\nPATH = \n");
    check(rejects([&]{p710::InitializeNativeVFS(options,{config});}),"config root outside explicit asset roots fails before mount");
    reset(); put(config,before);
    check(fs::current_path()==cwd,"VFS setup/checks never change process cwd");
}
void framePolicy() {
    check(p710::ContinueNativeWindowedFrame(true,false,false,true),"focused original windowed frames continue");
    check(p710::ContinueNativeWindowedFrame(true,false,false,false),"focus loss retains original windowed frame continuation with advancing clocks");
    for(bool focused:{false,true}) {
        check(!p710::ContinueNativeWindowedFrame(false,false,false,focused),"program stop ends frames regardless of focus");
        check(!p710::ContinueNativeWindowedFrame(true,true,false,focused),"SDL quit ends frames regardless of focus");
        check(!p710::ContinueNativeWindowedFrame(true,false,true,focused),"input window termination ends frames regardless of focus");
    }
}
void translation() {
    check(InitializeMemoryManager()==TRUE,"real native memory manager");
    auto* table=CreateEnglishTransTable();
    check(table && table->usNumberOfSymbols==255,"original translation count retained");
    check(table->DynamicArrayOf16BitValues[0]=='A' && table->DynamicArrayOf16BitValues[25]=='Z' && table->DynamicArrayOf16BitValues[26]=='a',"literal original alphabet glyph positions");
    check(table->DynamicArrayOf16BitValues[52]=='0' && table->DynamicArrayOf16BitValues[92]==' ',"literal original digits and space glyph positions");
    check(table->DynamicArrayOf16BitValues[93]==196 && table->DynamicArrayOf16BitValues[100]==1040,"literal German and Cyrillic positions");
    check(table->DynamicArrayOf16BitValues[254]==0,"unused original allocation tail is deterministic");
    check(InitializeFontManager(16,table)==TRUE,"native manager accepts genuine table");
    DestroyNativeFontTranslationTable(table);
    check(GetIndex(L'A')==0 && GetIndex(L'\u00c4')==93 && GetIndex(L'\u0410')==100,"manager copy survives explicit factory free");
    ShutdownFontManager(); ShutdownMemoryManager();
}
}
int main(int argc,char** argv) {
    fs::path root;
    try {
        if(argc!=2) throw std::runtime_error("pass a fresh generated workspace fixture directory");
        root=fs::absolute(argv[1]);
        if(fs::exists(root)) throw std::runtime_error("fixture root must be fresh (no existing paths removed)");
        fs::create_directories(root);
        profile(root); framePolicy(); translation();
        std::cout<<checks<<" native runtime/profile/translation checks passed\n";
        fs::remove_all(root); return 0;
    } catch(const std::exception& error) { std::cerr<<"FAIL: "<<error.what()<<'\n'; reset(); return 1; }
}
