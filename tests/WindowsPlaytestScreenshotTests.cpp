#include "platform/windows/WindowsPlaytestScreenshot.h"

#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <algorithm>
#include <iostream>

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
}

int main()
{
    const HRESULT apartment = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(apartment)) return 1;
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
