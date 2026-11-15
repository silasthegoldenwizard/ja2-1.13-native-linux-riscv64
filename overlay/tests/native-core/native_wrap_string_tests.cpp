/*
 * P710 native snapshot update: tests/native-core/native_wrap_string_tests.cpp
 * Native Linux source/build/test adaptations captured in the tested experimental source snapshot dated 2026-11-15.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-15.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "Utilities.h"
#include "Font.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <array>
#include <cwchar>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
namespace {
unsigned checks=0;
void check(bool b,const char* why) {++checks;if(!b)throw std::runtime_error(why);}
template<std::size_t N,std::size_t M> BOOLEAN wrap(CHAR16 (&a)[N],CHAR16 (&b)[M],UINT16 width,INT32 font) {return WrapString(a,N,b,M,width,font);}
void cleanup() {
    ShutdownFontManager();ShutdownVideoSurfaceManager();ShutdownVideoObjectManager();
    fileman::CloseAll();vfs::Log::flushDeleteAll();vfs::CVirtualFileSystem::shutdownVFS();vfs::ObjectAllocator::clear();
}
void run() {
    UINT16 symbols[]{65,66,32};FontTranslationTable table{3,symbols};
    check(InitializeVideoObjectManager()&&InitializeVideoSurfaceManager(),"real rendering managers");
    check(InitializeFontManager(16,&table),"real bitmap font manager");
    char file[]="font.sti";const INT32 font=LoadFontFile(file);check(font>=0,"generated bitmap font loaded through VFS");
    auto* object=GetFontObject(font);
    check(object&&GetWidth(object,GetIndex(L'A'))==4&&GetWidth(object,GetIndex(L'B'))==2&&GetWidth(object,GetIndex(L' '))==2,"production glyph widths include original A offset");
    CHAR16 words[6]=L"AA BB",rest[3]={L'X',L'Y',0};
    check(wrap(words,rest,10,font),"word-boundary split fits exact remainder capacity");
    check(!std::wcscmp(words,L"AA")&&!std::wcscmp(rest,L"BB"),"literal word split results");
    CHAR16 overflowSpace[6]=L"AA BB",other[3]{};
    check(wrap(overflowSpace,other,8,font)&&!std::wcscmp(overflowSpace,L"AA")&&!std::wcscmp(other,L"BB"),"overflowing space itself is chosen");
    CHAR16 noSpace[5]=L"AABB",tail[4]{};
    check(wrap(noSpace,tail,8,font),"no-space split fits exact hyphen and terminator capacity");
    check(!std::wcscmp(noSpace,L"AA")&&!std::wcscmp(tail,L"-BB"),"legacy hyphen prefixes remainder, not first line");
    // Production translation maps the unknown '-' to glyph0 (A, width4).
    // The second call must not return the identical '-ABB' forever.
    check(GetWidth(object,GetIndex(L'-'))==4,"actual fallback hyphen width");
    CHAR16 narrow[5]=L"AABB",continuation[5]{};
    check(wrap(narrow,continuation,4,font)&&!std::wcscmp(narrow,L"A")&&!std::wcscmp(continuation,L"-ABB"),"first narrow-line hyphen split makes progress");
    CHAR16 continuedTail[5]=L"XYZW";
    check(!wrap(continuation,continuedTail,4,font)&&!std::wcscmp(continuation,L"-ABB")&&!std::wcscmp(continuedTail,L"XYZW"),"continuation would repeat identical remainder: reject unchanged");
    CHAR16 underflow[5]=L"AAAA",underflowTail[4]{};
    check(wrap(underflow,underflowTail,8,font)&&!std::wcscmp(underflow,L"AA")&&!std::wcscmp(underflowTail,L"-AA"),"backwards no-space scan stops at zero without reading before array");
    CHAR16 exact[3]=L"AB",unchanged[4]=L"XYZ";
    check(!wrap(exact,unchanged,6,font)&&!std::wcscmp(exact,L"AB")&&!std::wcscmp(unchanged,L"XYZ"),"equal width needs no split and preserves both arrays");
    CHAR16 tiny[2]=L"A",tinyTail[4]=L"XYZ";
    check(!wrap(tiny,tinyTail,0,font)&&!std::wcscmp(tiny,L"A")&&!std::wcscmp(tinyTail,L"XYZ"),"first character wider than line declines nonprogressing empty split");
    CHAR16 leading[5]=L" AAB",leadingTail[4]{};
    check(wrap(leading,leadingTail,2,font)&&!std::wcscmp(leading,L"")&&!std::wcscmp(leadingTail,L"AAB"),"leading space can be consumed safely");
    CHAR16 shortInput[5]=L"AABB",shortTail[3]=L"XY";
    const auto inputBefore=std::array<CHAR16,5>{L'A',L'A',L'B',L'B',0};
    const auto tailBefore=std::array<CHAR16,3>{L'X',L'Y',0};
    check(!wrap(shortInput,shortTail,8,font)&&!std::memcmp(shortInput,inputBefore.data(),sizeof(shortInput))&&!std::memcmp(shortTail,tailBefore.data(),sizeof(shortTail)),"one character short rejects before input truncation or suffix writes");
    CHAR16 unterminated[4]{L'A',L'A',L'A',L'A'};CHAR16 untouched[4]=L"XYZ";
    check(!wrap(unterminated,untouched,8,font)&&unterminated[3]==L'A'&&!std::wcscmp(untouched,L"XYZ"),"bounded unterminated input does not scan beyond its real array");
    CHAR16 alias[9]=L"AA BB";const auto aliasBefore=std::array<CHAR16,9>{L'A',L'A',L' ',L'B',L'B',0,0,0,0};
    check(!WrapString(alias,9,alias+2,7,8,font)&&!std::memcmp(alias,aliasBefore.data(),sizeof(alias)),"partially overlapping arrays rejected before mutation");
    check(!WrapString(shortInput,5,untouched,0,8,font)&&!std::memcmp(shortInput,inputBefore.data(),sizeof(shortInput))&&!std::wcscmp(untouched,L"XYZ"),"zero output capacity rejects unchanged");
    check(!WrapString(nullptr,5,untouched,4,8,font)&&!std::wcscmp(untouched,L"XYZ"),"null input rejects unchanged");
    check(!WrapString(shortInput,5,nullptr,4,8,font)&&!std::memcmp(shortInput,inputBefore.data(),sizeof(shortInput)),"null output rejects unchanged");
    check(!WrapString(shortInput,5,untouched,4,8,-1)&&!std::memcmp(shortInput,inputBefore.data(),sizeof(shortInput))&&!std::wcscmp(untouched,L"XYZ"),"invalid production font rejects unchanged");
    // The same actual capacities now passed by the two original engine callers.
    CHAR16 assignment[300]{},assignmentTail[300]{};
    for(unsigned i=0;i<299;++i)assignment[i]=L'A';
    check(wrap(assignment,assignmentTail,250,font)&&std::wcslen(assignment)==62&&std::wcslen(assignmentTail)==238&&assignmentTail[0]==L'-',"300-character facility arrays use actual capacities");
    CHAR16 item[100]{},itemTail[100]{};for(unsigned i=0;i<99;++i)item[i]=L'A';
    check(wrap(item,itemTail,8,font)&&std::wcslen(item)==2&&std::wcslen(itemTail)==98&&itemTail[0]==L'-',"100-character inventory arrays use actual capacities");
    UnloadFont(static_cast<UINT32>(font));
}
}
int main(int argc,char** argv) {
    try {
        check(argc==2,"generated rendering fixture root argument");
        const auto root=std::filesystem::canonical(argv[1]);
        auto directory=std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(),vfs::Path(root.u8string()));
        check(directory->init(),"actual fixture directory");
        auto profile=std::make_unique<vfs::CVirtualProfile>("wrap-string",vfs::Path(root.u8string()),false);
        profile->addLocation(directory.get());auto* location=directory.release();auto* mounted=profile.get();
        getVFS()->getProfileStack()->pushProfile(profile.release());check(getVFS()->addLocation(location,mounted),"real VFS fixture mount");
        run();cleanup();std::cout<<checks<<" original WrapString checks passed\n";return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<'\n';cleanup();return 1;}
}
