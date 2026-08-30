#include "scene/assets/AssetManifest.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <limits>
#include <string_view>
#include <unordered_set>

namespace horde::scene::assets
{

namespace
{

constexpr std::size_t kMaximumManifestBytes = 1024u * 1024u;
constexpr std::size_t kMaximumJsonDepth = 32u;
constexpr std::size_t kMaximumArrayElements = 4096u;

bool ReadFile(const std::filesystem::path& path,
              std::string& text,
              std::string& diagnostic)
{
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input)
    {
        diagnostic = "Could not read asset manifest: " + path.string();
        return false;
    }
    const std::streampos end = input.tellg();
    if (end < std::streampos{0})
    {
        diagnostic = "Could not read asset manifest: " + path.string();
        return false;
    }
    const auto bytes = static_cast<std::uintmax_t>(end);
    if (bytes > kMaximumManifestBytes)
    {
        diagnostic = "Asset manifest exceeds the bounded JSON size limit.";
        return false;
    }
    text.resize(static_cast<std::size_t>(bytes));
    input.seekg(0, std::ios::beg);
    if (!text.empty())
    {
        input.read(text.data(), static_cast<std::streamsize>(text.size()));
        if (input.gcount() != static_cast<std::streamsize>(text.size()))
        {
            diagnostic = "Could not read asset manifest: " + path.string();
            return false;
        }
    }
    return true;
}

class JsonReader
{
public:
    explicit JsonReader(std::string_view text) : text_(text) {}

    template <typename MemberHandler>
    bool Object(MemberHandler&& handler)
    {
        if (!Enter('{')) return false;
        std::unordered_set<std::string> keys;
        SkipWhitespace();
        if (Consume('}')) return Leave();
        for (;;)
        {
            std::string key;
            if (!String(key) || !Consume(':')) return FailInvalid();
            if (!keys.insert(key).second)
            {
                error_ = "Asset manifest JSON contains duplicate key '" + key + "'.";
                return false;
            }
            if (!handler(key)) return false;
            SkipWhitespace();
            if (Consume('}')) return Leave();
            if (!Consume(',')) return FailInvalid();
        }
    }

    template <typename ElementHandler>
    bool Array(ElementHandler&& handler)
    {
        if (!Enter('[')) return false;
        SkipWhitespace();
        if (Consume(']')) return Leave();
        std::size_t index = 0u;
        for (;;)
        {
            if (index >= kMaximumArrayElements)
            {
                error_ = "Asset manifest JSON array exceeds the bounded element limit.";
                return false;
            }
            if (!handler(index++)) return false;
            SkipWhitespace();
            if (Consume(']')) return Leave();
            if (!Consume(',')) return FailInvalid();
        }
    }

    bool String(std::string& value)
    {
        SkipWhitespace();
        if (offset_ >= text_.size() || text_[offset_++] != '"') return FailInvalid();
        value.clear();
        while (offset_ < text_.size())
        {
            const unsigned char character = static_cast<unsigned char>(text_[offset_++]);
            if (character == '"') return true;
            if (character < 0x20u) return FailInvalid();
            if (character != '\\')
            {
                value.push_back(static_cast<char>(character));
                continue;
            }
            if (offset_ >= text_.size()) return FailInvalid();
            const char escape = text_[offset_++];
            switch (escape)
            {
            case '"': value.push_back('"'); break;
            case '\\': value.push_back('\\'); break;
            case '/': value.push_back('/'); break;
            case 'b': value.push_back('\b'); break;
            case 'f': value.push_back('\f'); break;
            case 'n': value.push_back('\n'); break;
            case 'r': value.push_back('\r'); break;
            case 't': value.push_back('\t'); break;
            case 'u':
            {
                std::uint32_t codePoint = 0u;
                if (!Hex4(codePoint)) return FailInvalid();
                if (codePoint >= 0xd800u && codePoint <= 0xdbffu)
                {
                    if (offset_ + 2u > text_.size() || text_[offset_] != '\\' ||
                        text_[offset_ + 1u] != 'u') return FailInvalid();
                    offset_ += 2u;
                    std::uint32_t low = 0u;
                    if (!Hex4(low) || low < 0xdc00u || low > 0xdfffu) return FailInvalid();
                    codePoint = 0x10000u + ((codePoint - 0xd800u) << 10u) + (low - 0xdc00u);
                }
                else if (codePoint >= 0xdc00u && codePoint <= 0xdfffu)
                {
                    return FailInvalid();
                }
                AppendUtf8(codePoint, value);
                break;
            }
            default: return FailInvalid();
            }
        }
        return FailInvalid();
    }

    bool Number(double& value)
    {
        SkipWhitespace();
        const std::size_t start = offset_;
        if (Peek('-')) ++offset_;
        if (Peek('0'))
        {
            ++offset_;
            if (offset_ < text_.size() && IsDigit(text_[offset_])) return FailInvalid();
        }
        else
        {
            if (offset_ >= text_.size() || text_[offset_] < '1' || text_[offset_] > '9')
                return FailInvalid();
            while (offset_ < text_.size() && IsDigit(text_[offset_])) ++offset_;
        }
        if (Peek('.'))
        {
            ++offset_;
            if (offset_ >= text_.size() || !IsDigit(text_[offset_])) return FailInvalid();
            while (offset_ < text_.size() && IsDigit(text_[offset_])) ++offset_;
        }
        if (Peek('e') || Peek('E'))
        {
            ++offset_;
            if (Peek('+') || Peek('-')) ++offset_;
            if (offset_ >= text_.size() || !IsDigit(text_[offset_])) return FailInvalid();
            while (offset_ < text_.size() && IsDigit(text_[offset_])) ++offset_;
        }
        try
        {
            value = std::stod(std::string(text_.substr(start, offset_ - start)));
            return true;
        }
        catch (...)
        {
            return FailInvalid();
        }
    }

    bool Unsigned(std::uint32_t& value)
    {
        double number = 0.0;
        if (!Number(number) || !std::isfinite(number) || number < 0.0 ||
            number > static_cast<double>(std::numeric_limits<std::uint32_t>::max()) ||
            std::floor(number) != number)
        {
            return FailInvalid();
        }
        value = static_cast<std::uint32_t>(number);
        return true;
    }

    bool Boolean(bool& value)
    {
        SkipWhitespace();
        if (Match("true"))
        {
            value = true;
            return true;
        }
        if (Match("false"))
        {
            value = false;
            return true;
        }
        return FailInvalid();
    }

    bool Finish()
    {
        SkipWhitespace();
        if (offset_ != text_.size())
        {
            error_ = "Asset manifest JSON has trailing content.";
            return false;
        }
        return true;
    }

    bool RejectField(std::string_view scope, const std::string& key)
    {
        error_ = "Asset manifest contains unsupported " + std::string(scope) +
                 " field '" + key + "'.";
        return false;
    }

    const std::string& Error() const { return error_; }

private:
    static bool IsDigit(char character) { return character >= '0' && character <= '9'; }

    void SkipWhitespace()
    {
        while (offset_ < text_.size() &&
               (text_[offset_] == ' ' || text_[offset_] == '\t' ||
                text_[offset_] == '\r' || text_[offset_] == '\n'))
        {
            ++offset_;
        }
    }

    bool Peek(char character) const
    {
        return offset_ < text_.size() && text_[offset_] == character;
    }

    bool Consume(char character)
    {
        SkipWhitespace();
        if (!Peek(character)) return false;
        ++offset_;
        return true;
    }

    bool Match(std::string_view value)
    {
        SkipWhitespace();
        if (text_.substr(offset_, value.size()) != value) return false;
        offset_ += value.size();
        return true;
    }

    bool Enter(char character)
    {
        if (!Consume(character)) return FailInvalid();
        if (++depth_ > kMaximumJsonDepth)
        {
            error_ = "Asset manifest JSON exceeds the bounded nesting limit.";
            return false;
        }
        return true;
    }

    bool Leave()
    {
        --depth_;
        return true;
    }

    bool FailInvalid()
    {
        if (error_.empty()) error_ = "Asset manifest JSON is invalid.";
        return false;
    }

    static int HexDigit(char character)
    {
        if (character >= '0' && character <= '9') return character - '0';
        if (character >= 'a' && character <= 'f') return character - 'a' + 10;
        if (character >= 'A' && character <= 'F') return character - 'A' + 10;
        return -1;
    }

    bool Hex4(std::uint32_t& value)
    {
        if (offset_ + 4u > text_.size()) return false;
        value = 0u;
        for (std::size_t i = 0u; i < 4u; ++i)
        {
            const int digit = HexDigit(text_[offset_++]);
            if (digit < 0) return false;
            value = (value << 4u) | static_cast<std::uint32_t>(digit);
        }
        return true;
    }

    static void AppendUtf8(std::uint32_t codePoint, std::string& output)
    {
        if (codePoint <= 0x7fu) output.push_back(static_cast<char>(codePoint));
        else if (codePoint <= 0x7ffu)
        {
            output.push_back(static_cast<char>(0xc0u | (codePoint >> 6u)));
            output.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
        }
        else if (codePoint <= 0xffffu)
        {
            output.push_back(static_cast<char>(0xe0u | (codePoint >> 12u)));
            output.push_back(static_cast<char>(0x80u | ((codePoint >> 6u) & 0x3fu)));
            output.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
        }
        else
        {
            output.push_back(static_cast<char>(0xf0u | (codePoint >> 18u)));
            output.push_back(static_cast<char>(0x80u | ((codePoint >> 12u) & 0x3fu)));
            output.push_back(static_cast<char>(0x80u | ((codePoint >> 6u) & 0x3fu)));
            output.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
        }
    }

    std::string_view text_;
    std::size_t offset_ = 0u;
    std::size_t depth_ = 0u;
    std::string error_;
};

bool ParseCoordinateSystem(JsonReader& reader, AssetManifest& manifest)
{
    bool hasUp = false;
    bool hasForward = false;
    const bool parsed = reader.Object([&](const std::string& key) {
        if (key == "up")
        {
            hasUp = reader.String(manifest.upAxis);
            return hasUp;
        }
        if (key == "forward")
        {
            hasForward = reader.String(manifest.forwardAxis);
            return hasForward;
        }
        return reader.RejectField("coordinateSystem", key);
    });
    return parsed && hasUp && hasForward;
}

bool ParseBudgets(JsonReader& reader, AssetManifest& manifest)
{
    std::array<bool, 5u> present{};
    const bool parsed = reader.Object([&](const std::string& key) {
        if (key == "maxVertices")
        {
            present[0] = reader.Unsigned(manifest.budgets.maxVertices);
            return present[0];
        }
        if (key == "maxIndices")
        {
            present[1] = reader.Unsigned(manifest.budgets.maxIndices);
            return present[1];
        }
        if (key == "maxPrimitives")
        {
            present[2] = reader.Unsigned(manifest.budgets.maxPrimitives);
            return present[2];
        }
        if (key == "maxMaterials")
        {
            present[3] = reader.Unsigned(manifest.budgets.maxMaterials);
            return present[3];
        }
        if (key == "maxTextureLayersPerKind")
        {
            present[4] = reader.Unsigned(manifest.budgets.maxTextureLayersPerKind);
            return present[4];
        }
        return reader.RejectField("budgets", key);
    });
    return parsed && std::all_of(present.begin(), present.end(), [](bool value) { return value; });
}

bool ParseLods(JsonReader& reader, AssetManifest& manifest)
{
    return reader.Array([&](std::size_t) {
        AssetLodBudget lod;
        bool hasName = false;
        bool hasTriangles = false;
        const bool parsed = reader.Object([&](const std::string& key) {
            if (key == "name")
            {
                hasName = reader.String(lod.name);
                return hasName;
            }
            if (key == "maxTriangles")
            {
                hasTriangles = reader.Unsigned(lod.maxTriangles);
                return hasTriangles;
            }
            return reader.RejectField("LOD", key);
        });
        if (!parsed || !hasName || !hasTriangles) return false;
        manifest.lods.push_back(std::move(lod));
        return true;
    });
}

bool ParseRequiredSockets(JsonReader& reader, AssetManifest& manifest)
{
    return reader.Array([&](std::size_t) {
        std::string socket;
        if (!reader.String(socket)) return false;
        manifest.requiredSockets.push_back(std::move(socket));
        return true;
    });
}

bool ParseTextureProfile(JsonReader& reader, AssetManifest& manifest)
{
    bool hasAndroid = false;
    bool hasWindows = false;
    bool hasMipmapped = false;
    const bool parsed = reader.Object([&](const std::string& key) {
        if (key == "android")
        {
            hasAndroid = reader.String(manifest.textureProfile.androidEncoding);
            return hasAndroid;
        }
        if (key == "windows")
        {
            hasWindows = reader.String(manifest.textureProfile.windowsEncoding);
            return hasWindows;
        }
        if (key == "mipmapped")
        {
            hasMipmapped = reader.Boolean(manifest.textureProfile.mipmapped);
            return hasMipmapped;
        }
        if (key == "resolution")
        {
            std::uint32_t ignored = 0u;
            return reader.Unsigned(ignored) && ignored > 0u;
        }
        return reader.RejectField("runtimeTextureProfile", key);
    });
    return parsed && hasAndroid && hasWindows && hasMipmapped;
}

bool ParseFloat3(JsonReader& reader, std::array<float, 3u>& values)
{
    std::size_t count = 0u;
    const bool parsed = reader.Array([&](std::size_t) {
        if (count >= values.size()) return false;
        double value = 0.0;
        if (!reader.Number(value) || !std::isfinite(value) ||
            value < 0.0 || value > 1.0) return false;
        values[count++] = static_cast<float>(value);
        return true;
    });
    return parsed && count == values.size();
}

bool ParseOptionalFloat(JsonReader& reader, float& destination, bool& present,
                        double minimum, double maximum)
{
    double value = 0.0;
    if (!reader.Number(value) || !std::isfinite(value) ||
        value < minimum || value > maximum) return false;
    destination = static_cast<float>(value);
    present = true;
    return true;
}

bool ParseMaterialOverrides(JsonReader& reader, AssetManifest& manifest)
{
    return reader.Array([&](std::size_t) {
        MaterialOverride overrideValue;
        bool hasMaterial = false;
        const bool parsed = reader.Object([&](const std::string& key) {
            if (key == "material")
            {
                hasMaterial = reader.String(overrideValue.material);
                return hasMaterial;
            }
            if (key == "emissiveStrength")
                return ParseOptionalFloat(reader, overrideValue.emissiveStrength,
                    overrideValue.hasEmissiveStrength, 0.0,
                    std::numeric_limits<double>::max());
            if (key == "transmissionFactor")
                return ParseOptionalFloat(reader, overrideValue.transmissionFactor,
                    overrideValue.hasTransmissionFactor, 0.0, 1.0);
            if (key == "ior")
                return ParseOptionalFloat(reader, overrideValue.ior,
                    overrideValue.hasIor, 1.0, 4.0);
            if (key == "thicknessFactor")
                return ParseOptionalFloat(reader, overrideValue.thicknessFactor,
                    overrideValue.hasThicknessFactor, 0.0,
                    std::numeric_limits<double>::max());
            if (key == "attenuationDistance")
                return ParseOptionalFloat(reader, overrideValue.attenuationDistance,
                    overrideValue.hasAttenuationDistance,
                    std::numeric_limits<double>::min(),
                    std::numeric_limits<double>::max());
            if (key == "roughnessFactor")
                return ParseOptionalFloat(reader, overrideValue.roughnessFactor,
                    overrideValue.hasRoughnessFactor, 0.0, 1.0);
            if (key == "attenuationColor")
            {
                overrideValue.hasAttenuationColor = ParseFloat3(
                    reader, overrideValue.attenuationColor);
                return overrideValue.hasAttenuationColor;
            }
            if (key == "thinWall")
            {
                overrideValue.hasThinWall = reader.Boolean(overrideValue.thinWall);
                return overrideValue.hasThinWall;
            }
            return reader.RejectField("materialOverride", key);
        });
        const bool hasOverride = overrideValue.hasEmissiveStrength ||
            overrideValue.hasTransmissionFactor || overrideValue.hasIor ||
            overrideValue.hasThicknessFactor || overrideValue.hasAttenuationDistance ||
            overrideValue.hasAttenuationColor || overrideValue.hasRoughnessFactor ||
            overrideValue.hasThinWall;
        if (!parsed || !hasMaterial || !hasOverride) return false;
        manifest.materialOverrides.push_back(std::move(overrideValue));
        return true;
    });
}

bool ParseManifest(std::string_view text, AssetManifest& manifest, std::string& diagnostic)
{
    JsonReader reader(text);
    bool hasSchema = false;
    bool hasAsset = false;
    bool hasMetresPerUnit = false;
    bool hasCoordinateSystem = false;
    bool hasBudgets = false;
    bool hasLods = false;
    bool hasRequiredSockets = false;
    bool hasTextureProfile = false;
    bool hasMaterialOverrides = false;
    double metresPerUnit = 0.0;

    const bool parsed = reader.Object([&](const std::string& key) {
        if (key == "schema")
        {
            hasSchema = reader.Unsigned(manifest.schema);
            return hasSchema;
        }
        if (key == "asset")
        {
            hasAsset = reader.String(manifest.assetName);
            return hasAsset;
        }
        if (key == "metresPerUnit")
        {
            hasMetresPerUnit = reader.Number(metresPerUnit);
            return hasMetresPerUnit;
        }
        if (key == "coordinateSystem")
        {
            hasCoordinateSystem = ParseCoordinateSystem(reader, manifest);
            return hasCoordinateSystem;
        }
        if (key == "budgets")
        {
            hasBudgets = ParseBudgets(reader, manifest);
            return hasBudgets;
        }
        if (key == "lods")
        {
            hasLods = ParseLods(reader, manifest);
            return hasLods;
        }
        if (key == "requiredSockets")
        {
            hasRequiredSockets = ParseRequiredSockets(reader, manifest);
            return hasRequiredSockets;
        }
        if (key == "runtimeTextureProfile")
        {
            hasTextureProfile = ParseTextureProfile(reader, manifest);
            return hasTextureProfile;
        }
        if (key == "materialOverrides")
        {
            hasMaterialOverrides = ParseMaterialOverrides(reader, manifest);
            return hasMaterialOverrides;
        }
        if (key == "distribution" || key == "licenceStatus")
        {
            std::string ignored;
            return reader.String(ignored);
        }
        return reader.RejectField("root", key);
    });

    if (!parsed || !reader.Finish())
    {
        diagnostic = reader.Error().empty()
            ? "Asset manifest JSON is invalid."
            : reader.Error();
        return false;
    }
    if (!hasSchema || !hasAsset || !hasMetresPerUnit || !hasCoordinateSystem ||
        !hasBudgets || !hasLods || !hasRequiredSockets || !hasTextureProfile ||
        !hasMaterialOverrides)
    {
        diagnostic = "Asset manifest is missing required schema-1 fields.";
        return false;
    }
    manifest.metresPerUnit = static_cast<float>(metresPerUnit);
    return true;
}

} // namespace

bool AssetManifest::Load(const std::filesystem::path& path,
                         AssetManifest& manifest,
                         std::string& diagnostic)
{
    manifest = {};
    std::string text;
    if (!ReadFile(path, text, diagnostic)) return false;
    if (!ParseManifest(text, manifest, diagnostic)) return false;

    if (manifest.schema != 1u)
    {
        diagnostic = "Asset manifest schema must be 1.";
        return false;
    }
    if (!std::isfinite(manifest.metresPerUnit) || manifest.metresPerUnit <= 0.0f)
    {
        diagnostic = "Asset manifest metresPerUnit must be finite and greater than zero.";
        return false;
    }
    if (manifest.upAxis != "+Y" || manifest.forwardAxis != "+Z")
    {
        diagnostic = "Asset manifest coordinateSystem must be +Y up and +Z forward.";
        return false;
    }
    if (manifest.budgets.maxVertices == 0u || manifest.budgets.maxIndices == 0u ||
        manifest.budgets.maxPrimitives == 0u || manifest.budgets.maxMaterials == 0u ||
        manifest.budgets.maxTextureLayersPerKind == 0u)
    {
        diagnostic = "Asset manifest budgets must all be greater than zero.";
        return false;
    }
    if (manifest.lods.empty())
    {
        diagnostic = "Asset manifest must define at least one LOD budget.";
        return false;
    }
    for (const AssetLodBudget& lod : manifest.lods)
    {
        if (lod.maxTriangles == 0u)
        {
            diagnostic = "Asset manifest LOD '" + lod.name +
                         "' maxTriangles must be greater than zero.";
            return false;
        }
    }
    if (manifest.textureProfile.androidEncoding != "astc" ||
        manifest.textureProfile.windowsEncoding != "rgba8" ||
        !manifest.textureProfile.mipmapped)
    {
        diagnostic = "Asset manifest runtimeTextureProfile must be Android ASTC, Windows RGBA8, and mipmapped.";
        return false;
    }
    diagnostic.clear();
    return true;
}

} // namespace horde::scene::assets
