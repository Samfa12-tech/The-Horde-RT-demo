#include "platform/windows/WindowsPlaytestScreenshot.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <wrl/implements.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>

namespace horde::platform::windows
{
namespace
{
using Microsoft::WRL::ComPtr;
constexpr auto kCap = reporting::kPlaytestScreenshotMaxBytes;

class BoundedPngStream final : public Microsoft::WRL::RuntimeClass<
    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IStream>
{
public:
    BoundedPngStream() { bytes.reserve(kCap); }
    ~BoundedPngStream() { if (!bytes.empty()) SecureZeroMemory(bytes.data(), bytes.size()); }
    HRESULT STDMETHODCALLTYPE Read(void* output, ULONG count, ULONG* read) override
    {
        if (read) *read = 0;
        if (count != 0 && !output) return STG_E_INVALIDPOINTER;
        const auto amount = std::min<std::size_t>(count, position < bytes.size() ? bytes.size() - position : 0);
        if (amount != 0) std::memcpy(output, bytes.data() + position, amount);
        position += amount;
        if (read) *read = static_cast<ULONG>(amount);
        return amount == count ? S_OK : S_FALSE;
    }
    HRESULT STDMETHODCALLTYPE Write(const void* input, ULONG count, ULONG* written) override
    {
        if (written) *written = 0;
        if (count != 0 && !input) return STG_E_INVALIDPOINTER;
        if (count > kCap - position) return STG_E_MEDIUMFULL;
        bytes.resize(std::max(bytes.size(), position + count));
        if (count != 0) std::memcpy(bytes.data() + position, input, count);
        position += count;
        if (written) *written = count;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER offset, DWORD origin, ULARGE_INTEGER* result) override
    {
        std::int64_t base = 0;
        if (origin == STREAM_SEEK_CUR) base = static_cast<std::int64_t>(position);
        else if (origin == STREAM_SEEK_END) base = static_cast<std::int64_t>(bytes.size());
        else if (origin != STREAM_SEEK_SET) return STG_E_INVALIDFUNCTION;
        if (offset.QuadPart < -base || offset.QuadPart > static_cast<std::int64_t>(kCap) - base)
            return STG_E_INVALIDFUNCTION;
        position = static_cast<std::size_t>(base + offset.QuadPart);
        if (result) result->QuadPart = position;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER size) override
    {
        if (size.QuadPart > kCap) return STG_E_MEDIUMFULL;
        bytes.resize(static_cast<std::size_t>(size.QuadPart));
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE CopyTo(IStream*, ULARGE_INTEGER, ULARGE_INTEGER*, ULARGE_INTEGER*) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE Commit(DWORD) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE Revert() override { return STG_E_INVALIDFUNCTION; }
    HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) override { return STG_E_INVALIDFUNCTION; }
    HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER, ULARGE_INTEGER, DWORD) override { return STG_E_INVALIDFUNCTION; }
    HRESULT STDMETHODCALLTYPE Stat(STATSTG* result, DWORD) override
    {
        if (!result) return STG_E_INVALIDPOINTER;
        *result = {};
        result->type = STGTY_STREAM;
        result->cbSize.QuadPart = bytes.size();
        result->grfMode = STGM_READWRITE;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Clone(IStream**) override { return E_NOTIMPL; }
    std::vector<std::uint8_t> bytes;
private:
    std::size_t position = 0;
};

std::uint32_t ReadBigEndian(const std::uint8_t* data) noexcept
{
    return (std::uint32_t{data[0]} << 24u) | (std::uint32_t{data[1]} << 16u) |
        (std::uint32_t{data[2]} << 8u) | data[3];
}

bool KeepOnlyImageChunks(const std::vector<std::uint8_t>& encoded, std::vector<std::uint8_t>& output)
{
    constexpr std::array<std::uint8_t, 8> signature{137, 80, 78, 71, 13, 10, 26, 10};
    if (encoded.size() < 8u || !std::equal(signature.begin(), signature.end(), encoded.begin())) return false;
    output.assign(signature.begin(), signature.end());
    std::size_t offset = 8u;
    bool ended = false;
    while (encoded.size() - offset >= 12u)
    {
        const auto length = ReadBigEndian(encoded.data() + offset);
        if (length > encoded.size() - offset - 12u) return false;
        const auto* type = encoded.data() + offset + 4u;
        const bool keep = std::memcmp(type, "IHDR", 4u) == 0 || std::memcmp(type, "IDAT", 4u) == 0 ||
            std::memcmp(type, "IEND", 4u) == 0;
        if (keep) output.insert(output.end(), encoded.begin() + offset, encoded.begin() + offset + length + 12u);
        else if ((type[0] & 0x20u) == 0u) return false; // unknown critical chunk
        offset += length + 12u;
        if (std::memcmp(type, "IEND", 4u) == 0) { ended = true; break; }
    }
    return ended && offset == encoded.size();
}

bool Encode(const reporting::PlaytestScreenshotPixels& pixels, std::vector<std::uint8_t>& png)
{
    const auto width = pixels.width, height = pixels.height;
    if (width == 0u || height == 0u ||
        std::max(width, height) > reporting::kPlaytestScreenshotCaptureLongEdge ||
        std::min(width, height) > reporting::kPlaytestScreenshotCaptureShortEdge ||
        std::uint64_t{width} * height * 4u != pixels.rgba.size()) return false;
    std::vector<std::uint8_t> rgb(static_cast<std::size_t>(width) * height * 3u);
    // WIC's PNG encoder accepts 24bppBGR, not 24bppRGB. The resulting PNG is
    // standard RGB8; decoder tests enforce normal red/blue channel order.
    for (std::size_t pixel = 0; pixel < rgb.size() / 3u; ++pixel)
    {
        rgb[pixel * 3u] = pixels.rgba[pixel * 4u + 2u];
        rgb[pixel * 3u + 1u] = pixels.rgba[pixel * 4u + 1u];
        rgb[pixel * 3u + 2u] = pixels.rgba[pixel * 4u];
    }
    ComPtr<IWICImagingFactory> factory;
    ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> properties;
    auto stream = Microsoft::WRL::Make<BoundedPngStream>();
    if (!stream) return false;
    HRESULT result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (SUCCEEDED(result)) result = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
    if (SUCCEEDED(result)) result = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
    if (SUCCEEDED(result)) result = encoder->CreateNewFrame(&frame, &properties);
    PROPBAG2 filter{};
    filter.pstrName = const_cast<wchar_t*>(L"FilterOption");
    VARIANT option{};
    option.vt = VT_UI1;
    option.bVal = WICPngFilterSub; // same lossless row prediction as Android
    if (SUCCEEDED(result)) result = properties->Write(1u, &filter, &option);
    if (SUCCEEDED(result)) result = frame->Initialize(properties.Get());
    if (SUCCEEDED(result)) result = frame->SetSize(width, height);
    WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
    if (SUCCEEDED(result)) result = frame->SetPixelFormat(&format);
    if (SUCCEEDED(result) && !IsEqualGUID(format, GUID_WICPixelFormat24bppBGR)) return false;
    if (SUCCEEDED(result)) result = frame->WritePixels(height, width * 3u, static_cast<UINT>(rgb.size()), rgb.data());
    if (SUCCEEDED(result)) result = frame->Commit();
    if (SUCCEEDED(result)) result = encoder->Commit();
    if (FAILED(result) || !KeepOnlyImageChunks(stream->bytes, png)) return false;
    return reporting::ValidatePlaytestScreenshot({png, width, height});
}
}

bool EncodeWindowsPlaytestScreenshot(const reporting::PlaytestScreenshotPixels& pixels,
    std::vector<std::uint8_t>& png) noexcept
{
    png.clear();
    const HRESULT initialised = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initialised) && initialised != RPC_E_CHANGED_MODE) return false;
    bool success = false;
    try { success = Encode(pixels, png); } catch (...) { success = false; }
    if (SUCCEEDED(initialised)) CoUninitialize();
    if (!success) { if (!png.empty()) SecureZeroMemory(png.data(), png.size()); png.clear(); }
    return success;
}
}
