/*
 * P710 native notice preparation: tests/native-core/image_viewer.cpp
 * New candidate test/build/documentation/metadata path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: add native SDL presentation of authored image/resource fixture output.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

/*
 * P710 native snapshot update: tests/native-core/image_viewer.cpp
 * Native Linux source/build/test adaptations captured in the accepted A9 snapshot dated 2026-11-14.
 * This file is inherited/adapted or project-added material; a new path does not establish independent authorship.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Update notice inserted during source-only package preparation: 2026-11-14.
 * This records the snapshot/preparation date, not an asserted first-edit date. Existing notices retained.
 * Exact original bytes, origins and reversible notice changes are recorded in RECONSTRUCTION-MANIFEST.json.
 */

#include "SDLPresentation.h"
#include "himage.h"
#include "video_object_core.h"
#include "etrle_blitters.h"
#include "FileManBackend.h"
#include <vfs/Core/vfs.h>
#include <vfs/Core/vfs_profile.h>
#include <vfs/Core/Location/vfs_directory_tree.h>
#include <vfs/Tools/vfs_log.h>
#include <filesystem>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
    bool managerInitialized = false;
    sgp::SDLPresentation window;
    try
    {
        if (argc != 3) throw std::runtime_error("expected fixture root and output BMP path");
        const fs::path root = fs::canonical(argv[1]);
        auto directory = std::make_unique<vfs::CReadOnlyDirectoryTree>(vfs::Path(), vfs::Path(root.u8string()));
        if (!directory->init()) throw std::runtime_error("cannot mount synthetic image directory");
        auto profile = std::make_unique<vfs::CVirtualProfile>("images", vfs::Path(root.u8string()), false);
        profile->addLocation(directory.get());
        auto* location = directory.release();
        auto* mounted = profile.get();
        getVFS()->getProfileStack()->pushProfile(profile.release());
        if (!getVFS()->addLocation(location, mounted)) throw std::runtime_error("cannot register image directory");

        constexpr int width = 40, height = 25, scale = 16;
        constexpr UINT32 pitch = width * 2 + 6;
        if (!window.Open("JA2 v1.13 native STI / ETRLE resource path", width, height, scale))
            throw std::runtime_error(std::string("native window: ") + SDL_GetError());
        if (!InitializeVideoObjectManager()) throw std::runtime_error("production object manager init failed");
        managerInitialized = true;
        VOBJECT_DESC description{};
        description.fCreateFlags = VOBJECT_CREATE_FROMFILE;
        std::strcpy(description.ImageFile, "sprite.sti");
        UINT32 spriteId = 0;
        if (!AddStandardVideoObject(&description, &spriteId))
            throw std::runtime_error("production CreateImage/CreateVideoObject failed");
        HVOBJECT sprite = nullptr;
        if (!GetVideoObject(&sprite, spriteId) || !sprite)
            throw std::runtime_error("production video-object lookup failed");
        std::vector<UINT8> pixels(static_cast<std::size_t>(pitch) * height, 0xa5);
        for (int y = 0; y < height; ++y)
            for (int x = 0; x < width; ++x)
            {
                const UINT16 color = ((x / 4 + y / 4) & 1) ? 0x3186 : 0x4210;
                std::memcpy(pixels.data() + static_cast<std::size_t>(y) * pitch + static_cast<std::size_t>(x) * 2,
                            &color, sizeof(color));
            }
        SGPRect clip{0, 0, width, height};
        SetClippingRect(&clip);
        if (!Blt8BPPDataTo16BPPBufferTransparent(reinterpret_cast<UINT16*>(pixels.data()), pitch, sprite, 5, 2, 0) ||
            !Blt8BPPDataTo16BPPBufferTransparentClip(reinterpret_cast<UINT16*>(pixels.data()), pitch, sprite, 18, 6, 1, nullptr) ||
            !Blt8BPPDataTo16BPPBufferTransparentClip(reinterpret_cast<UINT16*>(pixels.data()), pitch, sprite, -2, 15, 0, &clip))
            throw std::runtime_error("production ETRLE drawing failed");
        if (!window.Present565(pixels.data(), pixels.size(), pitch))
            throw std::runtime_error(std::string("native surface update: ") + SDL_GetError());
        if (!window.SaveWindowSurface(argv[2]))
            throw std::runtime_error(std::string("save own window surface: ") + SDL_GetError());
        std::ofstream frame(fs::path(argv[2]).replace_extension("rgb565"), std::ios::binary);
        frame.write(reinterpret_cast<const char*>(pixels.data()), static_cast<std::streamsize>(pixels.size()));
        if (!frame) throw std::runtime_error("cannot save actual RGB565 frame");
        const auto end = SDL_GetTicks() + 1500;
        while (SDL_GetTicks() < end && !window.QuitRequested()) SDL_Delay(10);
        std::cout << "PASS: object manager -> CreateImage/FileMan -> CreateVideoObject/ETRLE -> SDL3 "
                  << window.VideoDriver() << "; 2 authored frames, RGB565 with padded pitch\n";
        window.Close();
        if (!DeleteVideoObjectFromIndex(spriteId)) throw std::runtime_error("production object deletion failed");
        ShutdownVideoObjectManager();
        managerInitialized = false;
        fileman::CloseAll();
        vfs::Log::flushDeleteAll();
        vfs::CVirtualFileSystem::shutdownVFS();
        vfs::ObjectAllocator::clear();
        SDL_Quit();
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        window.Close();
        if (managerInitialized) ShutdownVideoObjectManager();
        fileman::CloseAll();
        vfs::Log::flushDeleteAll();
        vfs::CVirtualFileSystem::shutdownVFS();
        vfs::ObjectAllocator::clear();
        SDL_Quit();
        return 1;
    }
}
