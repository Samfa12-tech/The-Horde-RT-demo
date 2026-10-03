#include "platform/windows/WindowsPlaytestScreenshot.h"

#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <utility>

namespace
{
using horde::reporting::PlaytestScreenshotPixels;
using horde::platform::windows::EncodeWindowsPlaytestScreenshot;
bool passed = true;
void Check(bool value, const char* label)
{
    if (!value) { passed = false; std::cerr << label << '\n'; }
}

bool DecodeRgb(const std::vector<std::uint8_t>& png, const PlaytestScreenshotPixels& expected)
{
    using Microsoft::WRL::ComPtr;
    ComPtr<IWICImagingFactory> factory;
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter> converter;
    HRESULT result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (SUCCEEDED(result)) result = factory->CreateStream(&stream);
    if (SUCCEEDED(result)) result = stream->InitializeFromMemory(const_cast<BYTE*>(png.data()), static_cast<DWORD>(png.size()));
    if (SUCCEEDED(result)) result = factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnDemand, &decoder);
    UINT frames = 0, width = 0, height = 0;
    if (SUCCEEDED(result)) result = decoder->GetFrameCount(&frames);
    if (SUCCEEDED(result)) result = decoder->GetFrame(0, &frame);
    if (SUCCEEDED(result)) result = frame->GetSize(&width, &height);
    if (FAILED(result) || frames != 1 || width != expected.width || height != expected.height) return false;
    if (SUCCEEDED(result)) result = factory->CreateFormatConverter(&converter);
    if (SUCCEEDED(result)) result = converter->Initialize(frame.Get(), GUID_WICPixelFormat24bppRGB,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    std::vector<std::uint8_t> rgb(static_cast<std::size_t>(width) * height * 3u);
    if (SUCCEEDED(result)) result = converter->CopyPixels(nullptr, width * 3u, static_cast<UINT>(rgb.size()), rgb.data());
    if (FAILED(result)) return false;
    for (std::size_t pixel = 0; pixel < rgb.size() / 3u; ++pixel)
        if (!std::equal(rgb.begin() + pixel * 3u, rgb.begin() + pixel * 3u + 3u,
            expected.rgba.begin() + pixel * 4u)) return false;
    return true;
}

bool DecodeRgbaFile(const std::filesystem::path& path, PlaytestScreenshotPixels& image)
{
    using Microsoft::WRL::ComPtr;
    ComPtr<IWICImagingFactory> factory;
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter> converter;
    HRESULT result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (SUCCEEDED(result)) result = factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
        WICDecodeMetadataCacheOnDemand, &decoder);
    UINT frames = 0, width = 0, height = 0;
    if (SUCCEEDED(result)) result = decoder->GetFrameCount(&frames);
    if (SUCCEEDED(result)) result = decoder->GetFrame(0, &frame);
    if (SUCCEEDED(result)) result = frame->GetSize(&width, &height);
    const auto sourcePixels = static_cast<std::uint64_t>(width) * height;
    if (FAILED(result) || frames != 1 || width == 0 || height == 0 ||
        sourcePixels > horde::reporting::kPlaytestScreenshotMaxSourcePixels) return false;
    if (SUCCEEDED(result)) result = factory->CreateFormatConverter(&converter);
    if (SUCCEEDED(result)) result = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
        WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom);
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(sourcePixels) * 4u);
    if (SUCCEEDED(result)) result = converter->CopyPixels(nullptr, width * 4u,
        static_cast<UINT>(rgba.size()), rgba.data());
    if (FAILED(result)) return false;
    image = {width, height, std::move(rgba)};
    return true;
}

int RunNativeRtFixture(const std::filesystem::path& source, const std::filesystem::path& destination)
{
    if (!source.is_absolute() || !destination.is_absolute() ||
        _wcsicmp(source.lexically_normal().c_str(), destination.lexically_normal().c_str()) == 0) return 2;

    PlaytestScreenshotPixels sourceImage;
    if (!DecodeRgbaFile(source, sourceImage)) return 3;
    PlaytestScreenshotPixels resized;
    if (!horde::reporting::ResizePlaytestScreenshotRgba(sourceImage.width, sourceImage.height,
        sourceImage.rgba, resized)) return 4;
    std::vector<std::uint8_t> png;
    if (!EncodeWindowsPlaytestScreenshot(resized, png) ||
        !horde::reporting::ValidatePlaytestScreenshot({png, resized.width, resized.height}) ||
        !DecodeRgb(png, resized)) return 5;

    std::ofstream output(destination, std::ios::binary | std::ios::trunc);
    if (!output) return 6;
    output.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    output.close();
    if (!output) return 7;
    std::cout << "Native RT fixture encoded and losslessly decoded: " << resized.width << 'x'
              << resized.height << ", " << png.size() << " bytes.\n";
    return 0;
}
}

int wmain(int argc, wchar_t** argv)
{
    const HRESULT apartment = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(apartment)) return 1;
    if (argc == 4 && std::wstring_view(argv[1]) == L"--native-rt-fixture")
    {
        const int result = RunNativeRtFixture(argv[2], argv[3]);
        CoUninitialize();
        return result;
    }
    if (argc != 1)
    {
        CoUninitialize();
        return 2;
    }
    {
        PlaytestScreenshotPixels image{3u, 2u, {255,0,0,0, 0,255,0,100, 0,0,255,255,
            12,34,56,70, 200,110,10,80, 99,77,55,90}};
        std::vector<std::uint8_t> png;
        Check(EncodeWindowsPlaytestScreenshot(image, png), "RGB fixture encodes");
        Check(horde::reporting::ValidatePlaytestScreenshot({png, image.width, image.height}),
            "actual PNG is metadata-free with valid CRC/framing");
        Check(!png.empty() && DecodeRgb(png, image), "actual WIC decoder reproduces every RGB pixel and dimensions");
        const auto original = png;
        for (std::size_t pixel = 0; pixel < image.rgba.size() / 4u; ++pixel) image.rgba[pixel * 4u + 3u] = 255u;
        Check(EncodeWindowsPlaytestScreenshot(image, png) && png == original,
            "opaque-game attachment does not encode alpha or channel-swap RGB");
        image.rgba.pop_back();
        Check(!EncodeWindowsPlaytestScreenshot(image, png) && png.empty(), "invalid byte length clears stale output");
        image = {0u, 2u, {}};
        Check(!EncodeWindowsPlaytestScreenshot(image, png), "zero dimensions reject");
        image = {769u, 432u, std::vector<std::uint8_t>(769u * 432u * 4u)};
        Check(!EncodeWindowsPlaytestScreenshot(image, png), "too-long thumbnail rejects without silent resize");
        image = {433u, 433u, std::vector<std::uint8_t>(433u * 433u * 4u)};
        Check(!EncodeWindowsPlaytestScreenshot(image, png), "too-wide short edge rejects");
        image = {768u, 432u, std::vector<std::uint8_t>(768u * 432u * 4u)};
        for (std::size_t pixel = 0; pixel < image.rgba.size() / 4u; ++pixel)
        {
            image.rgba[pixel * 4u] = static_cast<std::uint8_t>(pixel % 768u);
            image.rgba[pixel * 4u + 1u] = static_cast<std::uint8_t>(pixel / 768u);
            image.rgba[pixel * 4u + 2u] = 31u;
            image.rgba[pixel * 4u + 3u] = 255u;
        }
        Check(EncodeWindowsPlaytestScreenshot(image, png) && DecodeRgb(png, image),
            "maximum thumbnail gradient compresses and losslessly decodes");
        std::uint32_t random = 0x31415926u;
        for (auto& byte : image.rgba) { random ^= random << 13u; random ^= random >> 17u; random ^= random << 5u; byte = static_cast<std::uint8_t>(random); }
        Check(!EncodeWindowsPlaytestScreenshot(image, png) && png.empty(),
            "incompressible image stops at bounded PNG storage cap, without omission or further resize");
    }
    CoUninitialize();
    return passed ? 0 : 1;
}
