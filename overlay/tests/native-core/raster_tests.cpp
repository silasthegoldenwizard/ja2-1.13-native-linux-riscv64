/*
 * P710 native notice preparation: tests/native-core/raster_tests.cpp
 * New candidate test/build/documentation/metadata path.
 * Native adaptation recorded in the 2026-11-12 checkpoint.
 * Exact first-edit/creation date is not established; this is a checkpoint-record date.
 * Scope: add independent guarded raster fixtures and expected-buffer comparisons.
 * Engine baseline: 1dot13/source@3394695a387a14c78e92b158cfdcf50e4068c96b.
 * Notice inserted: 2026-11-12.
 * Existing source bytes and original notices retained; no additional rights are granted.
 * This record does not establish license applicability, compliance or distribution permission.
 */

#include "line.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

static_assert(sizeof(INT32) == 4 && std::numeric_limits<INT32>::is_signed);
static_assert(sizeof(UINT8) == 1 && sizeof(INT8) == 1 && sizeof(INT16) == 2);
static_assert(sizeof(BOOLEAN) == 1 && sizeof(FLOAT) == 4);
static_assert(sizeof(SGPRect) == 16);
static_assert(offsetof(SGPRect, iLeft) == 0 && offsetof(SGPRect, iTop) == 4);
static_assert(offsetof(SGPRect, iRight) == 8 && offsetof(SGPRect, iBottom) == 12);

namespace
{
constexpr int width = 5;
constexpr int height = 5;
constexpr int pitch = 14; // Ten pixel bytes and four distinct padding bytes.
constexpr std::size_t guard = 32;
constexpr INT16 color = 0x1234;
using Point = std::pair<int, int>;

struct Frame
{
    std::size_t start;
    std::vector<UINT8> bytes;

    explicit Frame(std::size_t byteOffset = 0)
        : start(guard + byteOffset), bytes(start + pitch * height + guard, 0xd3)
    {
        for (int y = 0; y < height; ++y)
        {
            const auto row = start + static_cast<std::size_t>(y * pitch);
            std::fill_n(bytes.begin() + static_cast<std::ptrdiff_t>(row), width * 2, 0);
            std::fill_n(bytes.begin() + static_cast<std::ptrdiff_t>(row + width * 2),
                        pitch - width * 2, 0xa5);
        }
    }

    UINT8* data() { return bytes.data() + start; }

    // Only explicit expected coordinates or initial pixel values use this helper.
    // It does not calculate lines, clipping, borders or raster tie breaking.
    void pixel(int x, int y, std::uint16_t value)
    {
        if (x < 0 || x >= width || y < 0 || y >= height)
            throw std::runtime_error("Fixture coordinate outside the image");
        const auto position = start + static_cast<std::size_t>(y * pitch + x * 2);
        bytes.at(position) = static_cast<UINT8>(value & 0xffu);
        bytes.at(position + 1) = static_cast<UINT8>(value >> 8);
    }
};

struct Suite
{
    int passed = 0;
    int failed = 0;
    std::size_t recordedBytes = 0;
    std::ofstream actualFile;
    std::ofstream expectedFile;
    std::ofstream index;

    explicit Suite(const std::filesystem::path& output)
        : actualFile(output / "actual-fixtures.bin", std::ios::binary),
          expectedFile(output / "expected-fixtures.bin", std::ios::binary),
          index(output / "fixtures.tsv")
    {
        if (!actualFile || !expectedFile || !index)
            throw std::runtime_error("Cannot open fixture output files");
        index << "case\tfile_offset\tbytes\tresult\n";
    }

    void check(const std::string& name, const Frame& actual, const Frame& expected)
    {
        const bool same = actual.bytes == expected.bytes;
        if (same) ++passed;
        else
        {
            ++failed;
            const auto difference = std::mismatch(actual.bytes.begin(), actual.bytes.end(),
                                                   expected.bytes.begin());
            std::cerr << "FAIL " << name << " at guarded byte "
                      << std::distance(actual.bytes.begin(), difference.first)
                      << ": actual=" << static_cast<unsigned>(*difference.first)
                      << " expected=" << static_cast<unsigned>(*difference.second) << '\n';
        }
        std::cout << (same ? "PASS " : "FAIL ") << name << '\n';
        actualFile.write(reinterpret_cast<const char*>(actual.bytes.data()),
                         static_cast<std::streamsize>(actual.bytes.size()));
        expectedFile.write(reinterpret_cast<const char*>(expected.bytes.data()),
                           static_cast<std::streamsize>(expected.bytes.size()));
        index << name << '\t' << recordedBytes << '\t' << actual.bytes.size()
              << '\t' << (same ? "PASS" : "FAIL") << '\n';
        recordedBytes += actual.bytes.size();
    }
};

void expectPoints(Frame& expected, std::initializer_list<Point> points,
                  std::uint16_t value = 0x1234)
{
    for (const auto& [x, y] : points) expected.pixel(x, y, value);
}

void lineFixture(Suite& suite, const std::string& name, int x0, int y0, int x1, int y1,
                 std::initializer_list<Point> points)
{
    // Fixed hand-authored pixel sets exercise both API directions and both
    // clipped/unclipped paths. No expected set is derived from raster output.
    for (INT32 clipped : {0, 1})
    {
        for (bool reverse : {false, true})
        {
            Frame actual;
            Frame expected;
            SetClippingRegionAndImageWidth(pitch, 0, 0, width, height);
            if (reverse) LineDraw(clipped, x1, y1, x0, y0, color, actual.data());
            else LineDraw(clipped, x0, y0, x1, y1, color, actual.data());
            expectPoints(expected, points);
            suite.check(name + (clipped ? ".clip" : ".direct")
                        + (reverse ? ".reverse" : ".forward"), actual, expected);
        }
    }
}

void runLines(Suite& suite)
{
    lineFixture(suite, "horizontal", 0, 2, 4, 2,
                {{0,2}, {1,2}, {2,2}, {3,2}, {4,2}});
    lineFixture(suite, "vertical", 2, 0, 2, 4,
                {{2,0}, {2,1}, {2,2}, {2,3}, {2,4}});
    lineFixture(suite, "diagonal-positive", 0, 0, 4, 4,
                {{0,0}, {1,1}, {2,2}, {3,3}, {4,4}});
    lineFixture(suite, "diagonal-negative", 4, 0, 0, 4,
                {{4,0}, {3,1}, {2,2}, {1,3}, {0,4}});
    lineFixture(suite, "single-point", 2, 2, 2, 2, {{2,2}});
    lineFixture(suite, "shallow-east", 0, 1, 4, 3,
                {{0,1}, {1,2}, {2,2}, {3,3}, {4,3}});
    lineFixture(suite, "steep-east", 1, 0, 3, 4,
                {{1,0}, {2,1}, {2,2}, {3,3}, {3,4}});
    lineFixture(suite, "steep-west", 3, 0, 1, 4,
                {{3,0}, {2,1}, {2,2}, {1,3}, {1,4}});
    lineFixture(suite, "shallow-west", 4, 1, 0, 3,
                {{4,1}, {3,2}, {2,2}, {1,3}, {0,3}});
    lineFixture(suite, "shallow-fractional", 0, 0, 3, 2,
                {{0,0}, {1,1}, {2,1}, {3,2}});
    lineFixture(suite, "steep-fractional", 0, 0, 2, 3,
                {{0,0}, {1,1}, {1,2}, {2,3}});
    lineFixture(suite, "long-horizontal-runs", 0, 0, 4, 1,
                {{0,0}, {1,0}, {2,1}, {3,1}, {4,1}});
    lineFixture(suite, "long-vertical-runs", 0, 0, 1, 4,
                {{0,0}, {0,1}, {1,2}, {1,3}, {1,4}});
}

void runClipping(Suite& suite)
{
    struct Case
    {
        const char* name;
        int x0, y0, x1, y1;
        std::initializer_list<Point> pixels;
    };
    const Case cases[] = {
        {"clip-horizontal", -2,2,6,2, {{1,2},{2,2},{3,2}}},
        {"clip-vertical", 2,-2,2,6, {{2,1},{2,2},{2,3}}},
        {"clip-diagonal", -2,-2,6,6, {{1,1},{2,2},{3,3}}},
        {"clip-antidiagonal", -2,6,6,-2, {{1,3},{2,2},{3,1}}},
        {"outside-horizontal", -2,0,6,0, {}},
        {"outside-vertical", 4,-2,4,6, {}},
        {"outside-diagonal", -5,3,-1,7, {}},
        {"inside-point", 1,3,1,3, {{1,3}}},
        {"outside-point", 0,0,0,0, {}}
    };
    for (const auto& test : cases)
    {
        Frame actual;
        Frame expected;
        SetClippingRegionAndImageWidth(pitch, 1, 1, 3, 3);
        LineDraw(1, test.x0, test.y0, test.x1, test.y1, color, actual.data());
        expectPoints(expected, test.pixels);
        suite.check(test.name, actual, expected);
    }
}

void runPixels(Suite& suite)
{
    {
        Frame actual, expected;
        SetClippingRegionAndImageWidth(pitch, 0, 0, width, height);
        PixelDraw(0, 2, 3, color, actual.data());
        expected.pixel(2, 3, 0x1234);
        suite.check("pixel-little-endian-padded-pitch", actual, expected);
    }
    {
        Frame actual, expected;
        SetClippingRegionAndImageWidth(pitch, 0, 0, width, height);
        actual.pixel(2, 3, 0x1200);
        PixelAlterColour(0, 2, 3, 0x0034, actual.data());
        expected.pixel(2, 3, 0x1234);
        suite.check("alter-colour-preserves-existing-bits", actual, expected);
    }
    {
        Frame actual, expected;
        SetClippingRegionAndImageWidth(pitch, 0, 0, width, height);
        PixelDraw(0, 2, 3, -1, actual.data());
        expected.pixel(2, 3, 0xffff);
        suite.check("negative-colour-bit-pattern", actual, expected);
    }
    {
        Frame actual, expected;
        SetClippingRegionAndImageWidth(pitch, 1, 1, 3, 3);
        PixelDraw(1, 1, 1, color, actual.data());
        PixelDraw(1, 3, 1, color, actual.data());
        PixelDraw(1, 1, 3, color, actual.data());
        PixelDraw(1, 3, 3, color, actual.data());
        PixelDraw(1, -1, 2, color, actual.data());
        PixelDraw(1, 4, 3, color, actual.data());
        PixelDraw(1, 2, 0, color, actual.data());
        PixelAlterColour(1, 2, 4, color, actual.data());
        expectPoints(expected, {{1,1},{3,1},{1,3},{3,3}});
        suite.check("pixel-inclusive-boundaries-and-outside-rejection", actual, expected);
    }
    {
        Frame actual(1), expected(1);
        SetClippingRegionAndImageWidth(pitch, 0, 0, width, height);
        LineDraw(1, 0, 0, 4, 4, color, actual.data());
        PixelAlterColour(1, 2, 2, 0x0040, actual.data());
        expectPoints(expected, {{0,0},{1,1},{3,3},{4,4}});
        expected.pixel(2, 2, 0x1274);
        suite.check("offset-one-byte-storage", actual, expected);
    }
}

void runRectangles(Suite& suite)
{
    for (bool useStruct : {false, true})
    {
        Frame actual, expected;
        SetClippingRegionAndImageWidth(pitch, 0, 0, width, height);
        if (useStruct) RectangleDraw(1, SGPRect{1,1,3,3}, color, actual.data());
        else RectangleDraw(1, 1, 1, 3, 3, color, actual.data());
        expectPoints(expected, {{1,1},{2,1},{3,1},{1,2},{3,2},{1,3},{2,3},{3,3}});
        suite.check(useStruct ? "rectangle-struct" : "rectangle-coordinates", actual, expected);
    }
    {
        Frame actual, expected;
        SetClippingRegionAndImageWidth(pitch, 0, 0, width, height);
        RectangleDraw(1, SGPRect{-1,1,3,3}, color, actual.data());
        expectPoints(expected, {{0,1},{1,1},{2,1},{3,1},{3,2},{0,3},{1,3},{2,3},{3,3}});
        suite.check("rectangle-signed-coordinates-clipped", actual, expected);
    }
    {
        Frame actual, expected;
        SetClippingRegionAndImageWidth(pitch, 0, 0, width, height);
        RectangleDraw(1, 2, 2, 2, 2, color, actual.data());
        expected.pixel(2, 2, 0x1234);
        suite.check("rectangle-single-point", actual, expected);
    }
}

void preview(Suite& suite, const std::filesystem::path& output)
{
    Frame actual, expected;
    SetClippingRegionAndImageWidth(pitch, 0, 0, width, height);
    RectangleDraw(1, 0, 0, 4, 4, 0x001f, actual.data());
    LineDraw(1, 0, 0, 4, 4, 0x7800, actual.data());
    LineDraw(1, 4, 0, 0, 4, 0x07e0, actual.data());
    PixelAlterColour(1, 2, 2, 0x7800, actual.data());
    const std::array<std::uint16_t, 25> pixels = {
        0x7800,0x001f,0x001f,0x001f,0x07e0,
        0x001f,0x7800,0x0000,0x07e0,0x001f,
        0x001f,0x0000,0x7fe0,0x0000,0x001f,
        0x001f,0x07e0,0x0000,0x7800,0x001f,
        0x07e0,0x001f,0x001f,0x001f,0x7800
    };
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            expected.pixel(x, y, pixels.at(static_cast<std::size_t>(y * width + x)));
    suite.check("composite-preview", actual, expected);

    std::ofstream ppm(output / "raster-preview.ppm", std::ios::binary);
    ppm << "P6\n5 5\n255\n";
    std::ofstream raw(output / "raster-preview.rgb565", std::ios::binary);
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const auto position = actual.start + static_cast<std::size_t>(y * pitch + x * 2);
            const auto value = static_cast<unsigned>(actual.bytes[position])
                             | (static_cast<unsigned>(actual.bytes[position + 1]) << 8);
            const std::array<char,3> rgb = {
                static_cast<char>(((value >> 11) & 31u) * 255u / 31u),
                static_cast<char>(((value >> 5) & 63u) * 255u / 63u),
                static_cast<char>((value & 31u) * 255u / 31u)
            };
            ppm.write(rgb.data(), 3);
            raw.write(reinterpret_cast<const char*>(&actual.bytes[position]), 2);
        }
    }
    if (!ppm || !raw) throw std::runtime_error("Cannot write preview output");
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        if (argc != 3 || std::string(argv[1]) != "--output")
            throw std::runtime_error("Usage: p710_native_raster_tests --output DIRECTORY");
        const std::filesystem::path output(argv[2]);
        std::filesystem::create_directories(output);
        Suite suite(output);
        runLines(suite);
        runClipping(suite);
        runPixels(suite);
        runRectangles(suite);
        preview(suite, output);
        std::ofstream summary(output / "results.json");
        summary << "{\"passed\":" << suite.passed << ",\"failed\":" << suite.failed
                << ",\"width\":5,\"height\":5,\"pitch_bytes\":14,\"guard_bytes\":32}\n";
        if (!summary || !suite.actualFile || !suite.expectedFile || !suite.index)
            throw std::runtime_error("Cannot write test evidence");
        std::cout << "Raster fixtures: " << suite.passed << " passed, " << suite.failed
                  << " failed; every complete guarded/padded byte buffer compared\n";
        return suite.failed == 0 ? 0 : 1;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 2;
    }
}
