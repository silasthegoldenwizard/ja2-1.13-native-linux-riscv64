/*
 * P710 native snapshot update: tests/native-core/native_ini_tests.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

// Original CIniReader + actual bundled VFS, with authored read-only profiles.
// Fixtures and any fallback/parser diagnostics stay in this test's unique /tmp root.
#include "INIReader.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace {
unsigned checks = 0;
void check(bool value, const char* label) {
    ++checks;
    if (!value) throw std::runtime_error(label);
}
void clearErrors() { while (!iniErrorMessages.empty()) iniErrorMessages.pop(); }
struct Temporary {
    fs::path previous = fs::current_path();
    fs::path root;
    Temporary() {
        char pattern[] = "/tmp/p710-native-ini-XXXXXX";
        const char* made = ::mkdtemp(pattern);
        if (!made) throw std::runtime_error("unique INI temporary directory");
        root = made;
        fs::current_path(root); // Isolate the parser's legacy physical-file fallback.
    }
    ~Temporary() {
        clearErrors();
        vfs::Log::flushDeleteAll();
        vfs::CVirtualFileSystem::shutdownVFS();
        std::error_code error;
        fs::current_path(previous, error);
        fs::remove_all(root, error);
    }
};
void mount(const fs::path& root, const char* name) {
    auto directory = std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(), vfs::Path(root.u8string()));
    check(directory->init(), "initialize real read-only directory");
    auto profile = std::make_unique<vfs::CVirtualProfile>(name, vfs::Path(root.u8string()), false);
    profile->addLocation(directory.get());
    auto* location = directory.release();
    auto* mounted = profile.get();
    getVFS()->getProfileStack()->pushProfile(profile.release());
    check(getVFS()->addLocation(location, mounted), "mount real read-only profile");
}
using Fixtures = std::map<fs::path, std::string>;
void author(const Fixtures& fixtures) {
    for (const auto& fixture : fixtures) {
        fs::create_directories(fixture.first.parent_path());
        std::ofstream output(fixture.first, std::ios::binary);
        output << fixture.second;
        if (!output) throw std::runtime_error("author owned INI fixture");
    }
}
void verifyPreserved(const Fixtures& fixtures) {
    for (const auto& fixture : fixtures) {
        std::ifstream input(fixture.first, std::ios::binary);
        check(input.good(), "authored file remains readable");
        const std::string bytes{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        check(bytes == fixture.second, "read-only fixture bytes preserved");
    }
}
std::string string(CIniReader& reader, const char* section, const char* key, const char* fallback = "absent") {
    char buffer[512];
    reader.ReadString(section, key, fallback, buffer, sizeof(buffer));
    return buffer;
}
void scalarAndErrors() {
    CIniReader reader("Values.ini");
    check(reader.Is_CIniReader_File_Found(), "ordinary constructor initializes found flag");
    check(reader.ReadInteger("Numbers", "signed", 0) == -12, "signed integer");
    check(reader.ReadInteger("numbers", "SIGNED", 0) == -12, "actual VFS case-insensitive section/key");
    check(reader.ReadInteger("Numbers", "prefix", 0) == 18, "existing numeric-prefix parse preserved");
    check(reader.ReadInteger("Numbers", "missing", 7) == 7 &&
          reader.ReadInteger("Numbers", "bad", 9) == 9, "missing/malformed integer default");
    check(reader.ReadDouble("Numbers", "real", 0, -10, 10) == 1.25 &&
          reader.ReadFloat("Numbers", "real", 0, -10, 10) == 1.25f, "real conversions");
    check(reader.ReadDouble("Numbers", "bad", 2.5, 0, 5) == 2.5 &&
          reader.ReadFloat("Numbers", "missing", 3.5f, 0, 5) == 3.5f, "missing/malformed real default");
    check(reader.ReadUINT32("Numbers", "u32", 0, 0, UINT32{0xffffffff}) == UINT32{0xffffffff}, "full UINT32 value");
    check(reader.ReadUINT16("Numbers", "u16", 0, 0, 65535) == 65535 &&
          reader.ReadUINT8("Numbers", "u8", 0, 0, 255) == 255, "unsigned frontend widths");
    check(reader.ReadUINT32("Numbers", "bad", 22, 0, 100) == 22, "malformed unsigned default");
    check(iniErrorMessages.empty(), "valid and numeric default reads do not queue errors");
    check(reader.ReadInteger("Numbers", "signed", 0, -5, 10) == -5, "integer lower clamp");
    check(iniErrorMessages.size() == 1 && iniErrorMessages.top() ==
          "The value [Numbers][signed] = \"-12\" in file [Values.ini] is outside the valid range [-5 , 10].  -5 will be used.",
          "original range diagnostic bytes");
    check(reader.ReadInteger("Numbers", "high", 0, 0, 10) == 10, "integer upper clamp");
    check(reader.ReadDouble("Numbers", "lowReal", 0, -2, 2) == -2 &&
          reader.ReadFloat("Numbers", "highReal", 0, -2, 2) == 2, "double lower/float upper clamps");
    check(reader.ReadDouble("Numbers", "highReal", 0, -2, 2) == 2 &&
          reader.ReadFloat("Numbers", "lowReal", 0, -2, 2) == -2, "double upper/float lower clamps");
    check(reader.ReadUINT8("Numbers", "u8", 0, 0, 200) == 200 &&
          reader.ReadUINT16("Numbers", "small", 0, 10, 100) == 10 &&
          reader.ReadUINT32("Numbers", "u32", 0, 0, 1000) == 1000, "unsigned range clamps");
    check(reader.ReadInteger("Numbers", "missing", 99, 0, 10) == 10, "default values pass through original range check");
    check(iniErrorMessages.size() == 10, "one queued diagnostic per clamp");
    clearErrors();
    check(reader.ReadBoolean("Flags", "true", false) && !reader.ReadBoolean("Flags", "false", true), "boolean case-insensitive literals");
    check(!reader.ReadBoolean("Flags", "numeric", false), "numeric boolean remains invalid");
    check(iniErrorMessages.size() == 1 && iniErrorMessages.top() ==
          "The value [Flags][numeric] = \"1\" in file [Values.ini] is neither TRUE nor FALSE.  The value FALSE will be used.",
          "original boolean error diagnostic");
    check(reader.ReadBoolean("Flags", "missing", true, false) && iniErrorMessages.size() == 1,
          "suppressed missing boolean error uses default");
    check(reader.ReadBoolean("Flags", "bad", true) && iniErrorMessages.size() == 2, "malformed boolean queues error");
    clearErrors();
}
void stringsAndArrays() {
    CIniReader reader("Values.ini", TRUE);
    check(string(reader, "Text", "utf8") == u8"Grüße 🌍", "UTF-8 content via real VFS parser");
    check(string(reader, "Text", "missing", "fallback") == "fallback" && string(reader, "Text", "empty").empty(),
          "missing string default and explicitly empty string");
    check(string(reader, "Text", "literal") == "100% %s%n", "literal format tokens remain data");
    char bounded[6] = {'L', '?', '?', '?', '?', 'R'};
    reader.ReadString("Text", "word", "", bounded + 1, 4);
    check(std::string(bounded + 1) == "abc" && bounded[0] == 'L' && bounded[5] == 'R', "bounded string truncation and canaries");
    char single = 'Q';
    reader.ReadString("Text", "word", "", &single, 1);
    check(single == 0, "one-byte destination terminates");
    single = 'Q';
    reader.ReadString("Text", "word", "", &single, 0);
    reader.ReadString("Text", "word", "", nullptr, 0);
    reader.ReadString("Text", "word", "", nullptr, 4);
    check(single == 'Q', "zero-capacity/null destination is a safe no-op");
    std::unique_ptr<char[]> legacy(reader.ReadString("Text", "long", ""));
    check(std::strlen(legacy.get()) == 254 && std::string(legacy.get()) == std::string(254, 'x'), "legacy allocating string bound and caller ownership");
    std::vector<FLOAT> floats{99};
    std::vector<INT32> integers{99};
    reader.ReadFloatArray("Arrays", "float", floats);
    reader.ReadINT32Array("Arrays", "int", integers);
    check(floats == std::vector<FLOAT>{1.5f, -2, 3.25f} && integers == std::vector<INT32>{1, -2, 3}, "array whitespace and replacement");
    reader.ReadINT32Array("Arrays", "partial", integers);
    check(integers == std::vector<INT32>{4, 0} && iniErrorMessages.size() == 1,
          "existing malformed array retains parsed prefix then appends zero");
    check(iniErrorMessages.top() == "There was an error reading array [Arrays][partial] in file [Values.ini]. Defaulting to [0].",
          "original malformed array diagnostic");
    reader.ReadFloatArray("Arrays", "trailing", floats);
    check(floats == std::vector<FLOAT>{2, 0}, "trailing comma preserves prefix plus zero");
    reader.ReadINT32Array("Arrays", "missing", integers);
    check(integers == std::vector<INT32>{0} && iniErrorMessages.size() == 3, "missing array defaults to zero and queues error");
    reader.Clear();
    check(reader.ReadInteger("Numbers", "signed", 7) == 7 && reader.Is_CIniReader_File_Found(), "Clear removes values and retains original detection state");
    clearErrors();
}
void precedence() {
    CIniReader plain("Shared.ini");
    check(plain.ReadInteger("Scope", "common", 0) == 4, "highest profile override beats highest normal file");
    check(plain.ReadInteger("Scope", "baseOnly", -1) == -1 && plain.ReadInteger("Scope", "lowerOverrideOnly", -1) == -1,
          "unregistered file and override each use only selected highest profile");
    CIniReader forced("Shared.ini", FALSE);
    check(forced.ReadInteger("Scope", "common", 0) == 2 && forced.ReadInteger("Scope", "topOnly", 0) == 8,
          "boolean constructor retains original no-override contract");
    CIniReader::RegisterFileForMerging("SHARED.INI");
    CIniReader merged("shared.ini");
    check(merged.Is_CIniReader_File_Found() && merged.ReadInteger("Scope", "common", 0) == 4 &&
          merged.ReadInteger("Scope", "baseOnly", 0) == 9 && merged.ReadInteger("Scope", "topOnly", 0) == 8,
          "registered merge reads bottom to top then applies selected override");
    check(string(merged, "Scope", "list") == "base, mod" && merged.ReadInteger("Scope", "lowerOverrideOnly", -1) == -1,
          "VFS += merge operator preserved and override itself remains unmerged");
    CIniReader mergedForced("Shared.ini", TRUE);
    check(mergedForced.ReadInteger("Scope", "common", 0) == 2 && mergedForced.ReadInteger("Scope", "baseOnly", 0) == 9,
          "registered boolean constructor merges without applying override");
    CIniReader missing("Missing.ini"), missingForced("Missing.ini", FALSE);
    check(!missing.Is_CIniReader_File_Found() && !missingForced.Is_CIniReader_File_Found() &&
          missing.ReadInteger("Scope", "common", 11) == 11, "missing unregistered file detection and default");
    CIniReader::RegisterFileForMerging("RegisteredMissing.ini");
    CIniReader registeredMissing("RegisteredMissing.ini");
    check(registeredMissing.Is_CIniReader_File_Found(), "retain original registered-merge success flag for absent profile files");
    CIniReader overrideOnly("OverrideOnly.ini");
    check(!overrideOnly.Is_CIniReader_File_Found() && overrideOnly.ReadInteger("Scope", "common", 0) == 6,
          "override-only values do not change base-file detection");
}
void extensionPaths(const std::string& longName) {
    for (const char* name : {"Config.dir/multi.part.ini", "Config.dir\\multi.part.ini", "Config.dir/extensionless", "Config.dir/trailing."}) {
        CIniReader reader(name);
        check(reader.ReadInteger("Scope", "common", 0) == 7, "override replaces final filename extension with either VFS separator");
    }
    CIniReader longPath(longName.c_str());
    check(longPath.Is_CIniReader_File_Found() && longPath.ReadInteger("Scope", "common", 0) == 7,
          "native override handles paths longer than original diagnostic buffer");
    clearErrors();
    check(longPath.ReadInteger("Scope", "common", 0, 0, 1) == 1, "long-path clamp");
    check(iniErrorMessages.top().find("in file [" + longName.substr(0, 259) + "]") != std::string::npos,
          "retain original259-byte diagnostic filename truncation");
    clearErrors();
}
} // namespace
int main() {
    try {
        Temporary temporary;
        const auto base = temporary.root / "base", mod = temporary.root / "mod";
        const std::string longName = std::string(100, 'a') + "/" + std::string(100, 'b') + "/" + std::string(80, 'c') + ".ini";
        Fixtures fixtures{
            {base / "Values.ini", std::string(
                "; authored fixture\r\n# comment\r\n! comment\r\n[Numbers]\r\n"
                "signed = -12\r\nhigh = 42\r\nprefix = 18tail\r\nbad = invalid\r\n"
                "real = 1.25\r\nlowReal = -8.5\r\nhighReal = 8.5\r\nu32 = 4294967295\r\nu16 = 65535\r\nu8 = 255\r\nsmall = 3\r\n"
                "[Flags]\r\ntrue = TrUe\r\nfalse = FALSE\r\nnumeric = 1\r\nbad = maybe\r\n"
                "[Text]\r\nutf8 = ") + u8"Grüße 🌍" + "\r\nword = abcdef\r\nempty = \r\nliteral = 100% %s%n\r\nlong = " + std::string(300, 'x') +
                "\r\n[Arrays]\r\nfloat = 1.5, -2, 3.25\r\nint = 1, -2, 3\r\npartial = 4, invalid, 6\r\ntrailing = 2,\r\n"},
            {base / "Shared.ini", "[Scope]\ncommon = 1\nbaseOnly = 9\nlist = base\n"},
            {mod / "Shared.ini", "[Scope]\ncommon = 2\ntopOnly = 8\nlist += mod\n"},
            {base / "Shared.Override", "[Scope]\ncommon = 3\nlowerOverrideOnly = 5\n"},
            {mod / "Shared.Override", "[Scope]\ncommon = 4\n"},
            {base / "OverrideOnly.Override", "[Scope]\ncommon = 6\n"},
            {base / "Config.dir/multi.part.ini", "[Scope]\ncommon = 1\n"},
            {base / "Config.dir/multi.part.Override", "[Scope]\ncommon = 7\n"},
            {base / "Config.dir/extensionless", "[Scope]\ncommon = 1\n"},
            {base / "Config.dir/extensionless.Override", "[Scope]\ncommon = 7\n"},
            {base / "Config.dir/trailing.", "[Scope]\ncommon = 1\n"},
            {base / "Config.dir/trailing.Override", "[Scope]\ncommon = 7\n"},
            {base / longName, "[Scope]\ncommon = 1\n"},
            {base / (longName.substr(0, longName.size() - 4) + ".Override"), "[Scope]\ncommon = 7\n"}
        };
        author(fixtures);
        mount(base, "base"); mount(mod, "mod");
        scalarAndErrors(); stringsAndArrays(); precedence(); extensionPaths(longName);
        verifyPreserved(fixtures);
        std::cout << "original INIReader: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "original INIReader failed after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
