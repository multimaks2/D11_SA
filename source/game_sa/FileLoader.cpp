#include "GameDataFormat.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <locale>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace d11::data
{
namespace detail_gta_dat
{
namespace
{
std::string Trim(std::string value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
    if (begin == value.end())
    {
        return {};
    }
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
    return std::string(begin, end);
}

std::string ToUpperAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return value;
}

std::vector<std::string> SplitWhitespace(const std::string& line)
{
    std::vector<std::string> result;
    std::stringstream stream(line);
    std::string token;
    while (stream >> token)
    {
        result.push_back(token);
    }
    return result;
}

std::string JoinTokensFrom(const std::vector<std::string>& tokens, std::size_t startIndex)
{
    if (startIndex >= tokens.size())
    {
        return {};
    }
    std::string out = tokens[startIndex];
    for (std::size_t i = startIndex + 1; i < tokens.size(); ++i)
    {
        out += ' ';
        out += tokens[i];
    }
    return out;
}

bool PathLooksLikeZoneIpl(const std::string& pathUtf8)
{
    if (pathUtf8.size() < 4)
    {
        return false;
    }
    const std::string upper = ToUpperAscii(pathUtf8);
    return upper.compare(upper.size() - 4, 4, ".ZON") == 0;
}

eDatKeyword KeywordFromToken(const std::string& upperKw)
{
    if (upperKw == "CDIMAGE")
    {
        return eDatKeyword::CdImage;
    }
    if (upperKw == "IMG")
    {
        return eDatKeyword::Img;
    }
    if (upperKw == "IMGLIST")
    {
        return eDatKeyword::ImgList;
    }
    if (upperKw == "WATER")
    {
        return eDatKeyword::Water;
    }
    if (upperKw == "IDE")
    {
        return eDatKeyword::Ide;
    }
    if (upperKw == "COLFILE")
    {
        return eDatKeyword::ColFile;
    }
    if (upperKw == "MAPZONE")
    {
        return eDatKeyword::MapZone;
    }
    if (upperKw == "IPL")
    {
        return eDatKeyword::Ipl;
    }
    if (upperKw == "TEXDICTION")
    {
        return eDatKeyword::TexDiction;
    }
    if (upperKw == "MODELFILE")
    {
        return eDatKeyword::ModelFile;
    }
    if (upperKw == "SPLASH")
    {
        return eDatKeyword::Splash;
    }
    if (upperKw == "HIERFILE")
    {
        return eDatKeyword::HierFile;
    }
    if (upperKw == "EXIT")
    {
        return eDatKeyword::Exit;
    }
    return eDatKeyword::Unknown;
}
}
static void LoadGtaDatCatalogImpl(const std::filesystem::path& gtaDatPath, tGtaDatParseSummary& outSummary)
{
    outSummary.sourcePath = gtaDatPath;
    outSummary.loadOrder.clear();
    outSummary.errorMessage.clear();

    std::ifstream file(gtaDatPath);
    if (!file)
    {
        outSummary.errorMessage = "Не удалось открыть gta.dat: " + gtaDatPath.string();
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        const auto hashPos = line.find('#');
        if (hashPos != std::string::npos)
        {
            line = line.substr(0, hashPos);
        }
        line = Trim(line);
        if (line.empty())
        {
            continue;
        }

        const std::vector<std::string> tokens = SplitWhitespace(line);
        if (tokens.empty())
        {
            continue;
        }

        const std::string kwUpper = ToUpperAscii(tokens[0]);
        const eDatKeyword kw = KeywordFromToken(kwUpper);

        tGtaDatCatalogEntry entry;
        entry.keyword = kw;

        if (kw == eDatKeyword::Exit)
        {
            outSummary.loadOrder.push_back(entry);
            continue;
        }

        if (kw == eDatKeyword::Unknown)
        {
            continue;
        }

        if (kw == eDatKeyword::ColFile)
        {
            if (tokens.size() < 3)
            {
                continue;
            }
            try
            {
                entry.colFileZoneId = std::stoi(tokens[1]);
            }
            catch (...)
            {
                continue;
            }
            entry.pathOrText = JoinTokensFrom(tokens, 2);
            outSummary.loadOrder.push_back(entry);
            continue;
        }

        if (kw == eDatKeyword::Water)
        {
            std::string rest = line;
            const std::size_t afterKw = line.find_first_not_of(" \t", tokens[0].size());
            if (afterKw != std::string::npos)
            {
                rest = Trim(line.substr(tokens[0].size()));
            }
            else
            {
                rest.clear();
            }
            entry.pathOrText = rest;
            outSummary.loadOrder.push_back(entry);
            continue;
        }

        if (kw == eDatKeyword::Splash)
        {
            if (tokens.size() >= 2)
            {
                entry.pathOrText = JoinTokensFrom(tokens, 1);
            }
            outSummary.loadOrder.push_back(entry);
            continue;
        }

        if (tokens.size() < 2)
        {
            continue;
        }

        entry.pathOrText = JoinTokensFrom(tokens, 1);

        if (kw == eDatKeyword::Ipl || kw == eDatKeyword::MapZone)
        {
            if (PathLooksLikeZoneIpl(entry.pathOrText))
            {
                entry.skippedAsZoneIpl = true;
            }
        }

        outSummary.loadOrder.push_back(entry);
    }
}
}

void LoadGtaDatCatalog(const std::filesystem::path& gtaDatPath, tGtaDatParseSummary& outSummary)
{
    detail_gta_dat::LoadGtaDatCatalogImpl(gtaDatPath, outSummary);
}

    tDatPathEntry MakeDatPathEntry(eDatKeyword keyword, const std::string& path)
    {
        return tDatPathEntry{
            keyword,
            path
        };
    }

    tDatColFileEntry MakeDatColFileEntry(std::int32_t zoneId, const std::string& path)
    {
        return tDatColFileEntry{
            zoneId,
            path
        };
    }

    tDatWaterEntry MakeDatWaterEntry(std::vector<std::string> paths)
    {
        return tDatWaterEntry{
            std::move(paths)
        };
    }

    tDatSplashEntry MakeDatSplashEntry(const std::string& textureName)
    {
        return tDatSplashEntry{
            textureName
        };
    }

    tDatExitEntry MakeDatExitEntry()
    {
        return tDatExitEntry{
        };
    }

    tIdeBaseObjectEntry MakeIdeBaseObjectEntry(std::int32_t id, const std::string& modelName, const std::string& textureDictionary, const std::array<float, 3>& drawDistance, std::uint8_t drawDistanceCount, std::int32_t flags)
    {
        return tIdeBaseObjectEntry{
            id,
            modelName,
            textureDictionary,
            drawDistance,
            drawDistanceCount,
            flags
        };
    }

    tIdeTimedObjectEntry MakeIdeTimedObjectEntry(std::int32_t id, const std::string& modelName, const std::string& textureDictionary, const std::array<float, 3>& drawDistance, std::uint8_t drawDistanceCount, std::int32_t flags, std::int32_t timeOnHour, std::int32_t timeOffHour)
    {
        tIdeTimedObjectEntry entry;
        entry.id = id;
        entry.modelName = modelName;
        entry.textureDictionary = textureDictionary;
        entry.drawDistance = drawDistance;
        entry.drawDistanceCount = drawDistanceCount;
        entry.flags = flags;
        entry.timeOnHour = timeOnHour;
        entry.timeOffHour = timeOffHour;
        return entry;
    }

    tIdeAnimatedObjectEntry MakeIdeAnimatedObjectEntry(std::int32_t id, const std::string& modelName, const std::string& textureDictionary, const std::array<float, 3>& drawDistance, std::uint8_t drawDistanceCount, std::int32_t flags, const std::string& animationFile)
    {
        tIdeAnimatedObjectEntry entry;
        entry.id = id;
        entry.modelName = modelName;
        entry.textureDictionary = textureDictionary;
        entry.drawDistance = drawDistance;
        entry.drawDistanceCount = drawDistanceCount;
        entry.flags = flags;
        entry.animationFile = animationFile;
        return entry;
    }

    tIdeTextureParentEntry MakeIdeTextureParentEntry(const std::string& textureDictionary, const std::string& parentTextureDictionary)
    {
        return tIdeTextureParentEntry{
            textureDictionary,
            parentTextureDictionary
        };
    }

    tIdeGenericSectionEntry MakeIdeGenericSectionEntry(eIdeSection section, std::vector<std::string> tokens)
    {
        return tIdeGenericSectionEntry{
            section,
            std::move(tokens)
        };
    }

    tIplInstEntry MakeIplInstEntry(const tVector3& position, const tQuaternion& rotation, std::int32_t objectId, std::int32_t interior, std::int32_t lodIndex)
    {
        return tIplInstEntry{
            position,
            rotation,
            {},
            objectId,
            interior,
            lodIndex
        };
    }

    tIplCarsEntry MakeIplCarsEntry(const tVector3& position, float angleZ, std::int32_t vehicleId, std::int32_t primaryColor, std::int32_t secondaryColor, std::int32_t forceSpawn, std::int32_t alarmProbability, std::int32_t lockProbability, std::int32_t unknown1, std::int32_t unknown2)
    {
        return tIplCarsEntry{
            position,
            angleZ,
            vehicleId,
            primaryColor,
            secondaryColor,
            forceSpawn,
            alarmProbability,
            lockProbability,
            unknown1,
            unknown2
        };
    }

    tZonEntry MakeZonEntry(const std::string& name, std::int32_t type, const tVector3& minBounds, const tVector3& maxBounds, std::int32_t level, const std::optional<std::string>& text)
    {
        return tZonEntry{
            name,
            type,
            minBounds,
            maxBounds,
            level,
            text
        };
    }

namespace detail_ide
{
namespace
{
        std::string Trim(const std::string& value)
        {
            const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
            if (begin == value.end())
            {
                return {};
            }

            const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
            return std::string(begin, end);
        }

        std::string ToLower(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return value;
        }

        std::vector<std::string> SplitByComma(const std::string& line)
        {
            std::vector<std::string> result;
            std::stringstream stream(line);
            std::string token;
            while (std::getline(stream, token, ','))
            {
                result.push_back(Trim(token));
            }
            return result;
        }

        std::vector<std::string> SplitByWhitespace(const std::string& line)
        {
            std::vector<std::string> result;
            std::stringstream stream(line);
            std::string token;
            while (stream >> token)
            {
                result.push_back(token);
            }
            return result;
        }

        std::vector<std::string> SplitTokens(const std::string& line)
        {
            if (line.find(',') != std::string::npos)
            {
                return SplitByComma(line);
            }

            return SplitByWhitespace(line);
        }

        std::optional<std::int32_t> ParseInt(const std::string& token)
        {
            try
            {
                std::size_t consumed = 0;
                const int value = std::stoi(token, &consumed, 10);
                if (consumed != token.size())
                {
                    return std::nullopt;
                }
                return value;
            }
            catch (...)
            {
                return std::nullopt;
            }
        }

        std::optional<float> ParseFloat(const std::string& token)
        {
            std::istringstream stream(token);
            stream.imbue(std::locale::classic());

            float value = 0.0f;
            stream >> std::noskipws >> value;
            if (stream.fail() || !stream.eof())
            {
                return std::nullopt;
            }
            return value;
        }

        bool IsIntegerToken(const std::string& token)
        {
            return ParseInt(token).has_value();
        }

        eIdeSection ParseSectionType(const std::string& token)
        {
            const std::string section = ToLower(token);
            if (section == "objs")
            {
                return eIdeSection::Objs;
            }
            if (section == "tobj")
            {
                return eIdeSection::Tobj;
            }
            if (section == "hier")
            {
                return eIdeSection::Hier;
            }
            if (section == "cars")
            {
                return eIdeSection::Cars;
            }
            if (section == "peds")
            {
                return eIdeSection::Peds;
            }
            if (section == "path")
            {
                return eIdeSection::Path;
            }
            if (section == "2dfx")
            {
                return eIdeSection::Fx2d;
            }
            if (section == "weap")
            {
                return eIdeSection::Weap;
            }
            if (section == "anim")
            {
                return eIdeSection::Anim;
            }
            if (section == "txdp")
            {
                return eIdeSection::Txdp;
            }
            return eIdeSection::Unknown;
        }

        struct ParsedDrawDistance
        {
            std::array<float, 3> values {0.0f, 0.0f, 0.0f};
            std::uint8_t count {0};
        };

        ParsedDrawDistance ParseDrawDistanceTokens(const std::vector<std::string>& middleTokens)
        {
            ParsedDrawDistance drawDistance;
            if (middleTokens.empty())
            {
                return drawDistance;
            }

            std::size_t offset = 0;
            if (middleTokens.size() >= 2 && IsIntegerToken(middleTokens.front()))
            {
                offset = 1;
            }

            std::size_t drawIndex = 0;
            for (std::size_t index = offset; index < middleTokens.size() && drawIndex < drawDistance.values.size(); ++index)
            {
                if (const auto parsed = ParseFloat(middleTokens[index]); parsed.has_value())
                {
                    drawDistance.values[drawIndex] = *parsed;
                    ++drawIndex;
                }
            }
            drawDistance.count = static_cast<std::uint8_t>(drawIndex);

            return drawDistance;
        }

        std::optional<tIdeBaseObjectEntry> ParseBaseObjectLine(const std::vector<std::string>& tokens)
        {
            // GTA SA OBJS обычно: id, model, txd, drawDist, flags (5 колонок).
            // Иногда drawDist может быть 2-3 числами, поэтому колонок может быть больше.
            if (tokens.size() < 5)
            {
                return std::nullopt;
            }

            const auto id = ParseInt(tokens[0]);
            const auto flags = ParseInt(tokens.back());
            if (!id.has_value() || !flags.has_value())
            {
                return std::nullopt;
            }

            std::vector<std::string> middleTokens(tokens.begin() + 3, tokens.end() - 1);
            const auto drawDistance = ParseDrawDistanceTokens(middleTokens);

            return MakeIdeBaseObjectEntry(*id, tokens[1], tokens[2], drawDistance.values, drawDistance.count, *flags);
        }
    
}
static tIdeParseResult ParseIdeFileImpl(const std::string& filePath)
    {
        tIdeParseResult result;

        std::ifstream file(filePath);
        if (!file.is_open())
        {
            result.issues.push_back({0, "Could not open IDE file: " + filePath});
            return result;
        }

        eIdeSection currentSection = eIdeSection::Unknown;
        std::string rawLine;
        std::size_t lineNumber = 0;

        while (std::getline(file, rawLine))
        {
            ++lineNumber;

            const std::size_t commentPos = rawLine.find_first_of("#;");
            const std::string line = Trim(rawLine.substr(0, commentPos));
            if (line.empty())
            {
                continue;
            }

            const std::string lowered = ToLower(line);
            if (lowered == "end")
            {
                currentSection = eIdeSection::Unknown;
                continue;
            }

            const eIdeSection possibleSection = ParseSectionType(line);
            if (possibleSection != eIdeSection::Unknown)
            {
                currentSection = possibleSection;
                continue;
            }

            const auto tokens = SplitTokens(line);
            if (tokens.empty())
            {
                continue;
            }

            if (currentSection == eIdeSection::Unknown)
            {
                result.issues.push_back({lineNumber, "Data line outside of a known IDE section"});
                continue;
            }

            switch (currentSection)
            {
            case eIdeSection::Objs:
            {
                const auto parsed = ParseBaseObjectLine(tokens);
                if (!parsed.has_value())
                {
                    result.issues.push_back({lineNumber, "Failed to parse OBJS entry"});
                    break;
                }
                result.entries.push_back(*parsed);
                break;
            }
            case eIdeSection::Tobj:
            {
                if (tokens.size() < 8)
                {
                    result.issues.push_back({lineNumber, "Failed to parse TOBJ entry: not enough columns"});
                    break;
                }

                const auto id = ParseInt(tokens[0]);
                const auto flags = ParseInt(tokens[tokens.size() - 3]);
                const auto timeOn = ParseInt(tokens[tokens.size() - 2]);
                const auto timeOff = ParseInt(tokens[tokens.size() - 1]);
                if (!id.has_value() || !flags.has_value() || !timeOn.has_value() || !timeOff.has_value())
                {
                    result.issues.push_back({lineNumber, "Failed to parse TOBJ entry: invalid numeric values"});
                    break;
                }

                std::vector<std::string> middleTokens(tokens.begin() + 3, tokens.end() - 3);
                const auto drawDistance = ParseDrawDistanceTokens(middleTokens);
                result.entries.push_back(MakeIdeTimedObjectEntry(*id, tokens[1], tokens[2], drawDistance.values, drawDistance.count, *flags, *timeOn, *timeOff));
                break;
            }
            case eIdeSection::Anim:
            {
                if (tokens.size() < 7)
                {
                    result.issues.push_back({lineNumber, "Failed to parse ANIM entry: not enough columns"});
                    break;
                }

                const auto id = ParseInt(tokens[0]);
                const auto flags = ParseInt(tokens.back());
                if (!id.has_value() || !flags.has_value())
                {
                    result.issues.push_back({lineNumber, "Failed to parse ANIM entry: invalid numeric values"});
                    break;
                }

                std::vector<std::string> middleTokens(tokens.begin() + 4, tokens.end() - 1);
                const auto drawDistance = ParseDrawDistanceTokens(middleTokens);
                result.entries.push_back(MakeIdeAnimatedObjectEntry(*id, tokens[1], tokens[2], drawDistance.values, drawDistance.count, *flags, tokens[3]));
                break;
            }
            case eIdeSection::Txdp:
            {
                if (tokens.size() < 2)
                {
                    result.issues.push_back({lineNumber, "Failed to parse TXDP entry"});
                    break;
                }
                result.entries.push_back(MakeIdeTextureParentEntry(tokens[0], tokens[1]));
                break;
            }
            case eIdeSection::Hier:
            case eIdeSection::Cars:
            case eIdeSection::Peds:
            case eIdeSection::Path:
            case eIdeSection::Fx2d:
            case eIdeSection::Weap:
            case eIdeSection::Unknown:
            default:
                result.entries.push_back(MakeIdeGenericSectionEntry(currentSection, tokens));
                break;
            }
        }

        return result;
    }
}

tIdeParseResult ParseIdeFile(const std::string& filePath)
{
    return detail_ide::ParseIdeFileImpl(filePath);
}

namespace detail_ipl
{
namespace
{
enum class eIplSection
{
    Unknown,
    Inst,
    Cars,
    Zone
};

std::string Trim(const std::string& value)
{
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
    if (begin == value.end())
    {
        return {};
    }

    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
    return std::string(begin, end);
}

std::string ToLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::optional<std::int32_t> ParseInt(const std::string& token)
{
    try
    {
        std::size_t consumed = 0;
        const int value = std::stoi(token, &consumed, 10);
        if (consumed != token.size())
        {
            return std::nullopt;
        }
        return value;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

std::optional<double> ParseDouble(const std::string& token)
{
    std::istringstream stream(token);
    stream.imbue(std::locale::classic());

    double value = 0.0;
    stream >> std::noskipws >> value;
    if (stream.fail() || !stream.eof())
    {
        return std::nullopt;
    }
    return value;
}

std::vector<std::string> SplitByComma(const std::string& line)
{
    std::vector<std::string> result;
    std::stringstream stream(line);
    std::string token;
    while (std::getline(stream, token, ','))
    {
        result.push_back(Trim(token));
    }
    return result;
}

std::vector<std::string> SplitByWhitespace(const std::string& line)
{
    std::vector<std::string> result;
    std::stringstream stream(line);
    std::string token;
    while (stream >> token)
    {
        result.push_back(token);
    }
    return result;
}

std::vector<std::string> SplitTokens(const std::string& line)
{
    if (line.find(',') != std::string::npos)
    {
        return SplitByComma(line);
    }
    return SplitByWhitespace(line);
}

eIplSection ParseSectionType(const std::string& token)
{
    const std::string lowered = ToLower(token);
    if (lowered == "inst")
    {
        return eIplSection::Inst;
    }
    if (lowered == "cars")
    {
        return eIplSection::Cars;
    }
    if (lowered == "zone")
    {
        return eIplSection::Zone;
    }
    return eIplSection::Unknown;
}

const char* SectionName(eIplSection section)
{
    switch (section)
    {
    case eIplSection::Inst: return "INST";
    case eIplSection::Cars: return "CARS";
    case eIplSection::Zone: return "ZONE";
    case eIplSection::Unknown:
    default: return "UNKNOWN";
    }
}

bool ParseInstTokens(const std::vector<std::string>& t, tIplInstEntry& out)
{
    // SA INST:
    // id, modelName, interior, x, y, z, rx, ry, rz, rw, lod
    if (t.size() < 11)
    {
        return false;
    }

    const auto id = ParseInt(t[0]);
    const auto interior = ParseInt(t[2]);
    const auto x = ParseDouble(t[3]);
    const auto y = ParseDouble(t[4]);
    const auto z = ParseDouble(t[5]);
    const auto rx = ParseDouble(t[6]);
    const auto ry = ParseDouble(t[7]);
    const auto rz = ParseDouble(t[8]);
    const auto rw = ParseDouble(t[9]);
    const auto lod = ParseInt(t[10]);
    if (!id || !interior || !x || !y || !z || !rx || !ry || !rz || !rw || !lod)
    {
        return false;
    }

    out = MakeIplInstEntry(
        tVector3{*x, *y, *z},
        tQuaternion{*rx, *ry, *rz, *rw},
        *id,
        *interior,
        *lod
    );
    out.modelName = t[1];
    return true;
}

bool ParseCarsTokens(const std::vector<std::string>& t, tIplCarsEntry& out)
{
    // SA CARS:
    // x, y, z, angleZ, vehicleId, primary, secondary, forceSpawn, alarm, lock, unknown1, unknown2
    if (t.size() < 12)
    {
        return false;
    }

    const auto x = ParseDouble(t[0]);
    const auto y = ParseDouble(t[1]);
    const auto z = ParseDouble(t[2]);
    const auto angleZ = ParseDouble(t[3]);
    const auto vehicleId = ParseInt(t[4]);
    const auto primaryColor = ParseInt(t[5]);
    const auto secondaryColor = ParseInt(t[6]);
    const auto forceSpawn = ParseInt(t[7]);
    const auto alarmProbability = ParseInt(t[8]);
    const auto lockProbability = ParseInt(t[9]);
    const auto unknown1 = ParseInt(t[10]);
    const auto unknown2 = ParseInt(t[11]);
    if (!x || !y || !z || !angleZ || !vehicleId || !primaryColor || !secondaryColor ||
        !forceSpawn || !alarmProbability || !lockProbability || !unknown1 || !unknown2)
    {
        return false;
    }

    out = MakeIplCarsEntry(
        tVector3{*x, *y, *z},
        static_cast<float>(*angleZ),
        *vehicleId,
        *primaryColor,
        *secondaryColor,
        *forceSpawn,
        *alarmProbability,
        *lockProbability,
        *unknown1,
        *unknown2
    );
    return true;
}

bool ParseZoneTokens(const std::vector<std::string>& t, tIplZoneEntry& out)
{
    // SA ZONE:
    // name, type, x1, y1, z1, x2, y2, z2, level
    if (t.size() < 9)
    {
        return false;
    }

    const auto type = ParseInt(t[1]);
    const auto x1 = ParseDouble(t[2]);
    const auto y1 = ParseDouble(t[3]);
    const auto z1 = ParseDouble(t[4]);
    const auto x2 = ParseDouble(t[5]);
    const auto y2 = ParseDouble(t[6]);
    const auto z2 = ParseDouble(t[7]);
    const auto level = ParseInt(t[8]);
    if (!type || !x1 || !y1 || !z1 || !x2 || !y2 || !z2 || !level)
    {
        return false;
    }

    out = tIplZoneEntry{
        t[0],
        *type,
        tVector3{*x1, *y1, *z1},
        tVector3{*x2, *y2, *z2},
        *level
    };
    return true;
}
}
// namespace

static tIplParseResult ParseIplFileImpl(const std::string& filePath)
{
    tIplParseResult result;

    std::ifstream file(filePath);
    if (!file.is_open())
    {
        result.issues.push_back({0, "Could not open IPL file: " + filePath});
        return result;
    }

    eIplSection currentSection = eIplSection::Unknown;
    std::string rawLine;
    std::size_t lineNumber = 0;

    while (std::getline(file, rawLine))
    {
        ++lineNumber;

        const std::size_t commentPos = rawLine.find_first_of("#;");
        const std::string line = Trim(rawLine.substr(0, commentPos));
        if (line.empty())
        {
            continue;
        }

        const std::string lowered = ToLower(line);
        if (lowered == "end")
        {
            currentSection = eIplSection::Unknown;
            continue;
        }

        const eIplSection possibleSection = ParseSectionType(line);
        if (possibleSection != eIplSection::Unknown)
        {
            currentSection = possibleSection;
            continue;
        }

        if (currentSection == eIplSection::Unknown)
        {
            result.issues.push_back({lineNumber, "Data line outside of a known IPL section"});
            continue;
        }

        const auto tokens = SplitTokens(line);
        if (tokens.empty())
        {
            continue;
        }

        switch (currentSection)
        {
        case eIplSection::Inst:
        {
            tIplInstEntry inst;
            if (!ParseInstTokens(tokens, inst))
            {
                result.issues.push_back({lineNumber, "Failed to parse INST entry"});
                break;
            }
            result.data.inst.push_back(inst);
            break;
        }
        case eIplSection::Cars:
        {
            tIplCarsEntry cars;
            if (!ParseCarsTokens(tokens, cars))
            {
                result.issues.push_back({lineNumber, "Failed to parse CARS entry"});
                break;
            }
            result.data.cars.push_back(cars);
            break;
        }
        case eIplSection::Zone:
        {
            tIplZoneEntry zone;
            if (!ParseZoneTokens(tokens, zone))
            {
                result.issues.push_back({lineNumber, "Failed to parse ZONE entry"});
                break;
            }
            result.data.zones.push_back(zone);
            break;
        }
        case eIplSection::Unknown:
        default:
            result.issues.push_back({lineNumber, std::string("Unhandled IPL section: ") + SectionName(currentSection)});
            break;
        }
    }

    return result;
}
}

tIplParseResult ParseIplFile(const std::string& filePath)
{
    return detail_ipl::ParseIplFileImpl(filePath);
}

namespace detail_img
{
static std::string ImgToLowerAscii(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });
            return value;
        }

namespace
{
std::string TrimNullTerminatedName(const char* buffer, std::size_t maxLen)
        {
            std::size_t len = 0;
            while (len < maxLen && buffer[len] != '\0')
            {
                ++len;
            }

            while (len > 0)
            {
                const unsigned char ch = static_cast<unsigned char>(buffer[len - 1]);
                if (std::isspace(ch) == 0)
                {
                    break;
                }
                --len;
            }

            return std::string(buffer, buffer + len);
        }

        bool ReadExact(std::ifstream& stream, void* dst, std::size_t size)
        {
            stream.read(static_cast<char*>(dst), static_cast<std::streamsize>(size));
            return stream.good();
        }

        std::uint64_t TellU64(std::ifstream& stream)
        {
            return static_cast<std::uint64_t>(stream.tellg());
        }
    
}

    static tImgParseResult ParseImgArchiveFileImpl(const std::string& filePath)
    {
        tImgParseResult out;

        std::ifstream f(filePath, std::ios::binary);
        if (!f.is_open())
        {
            out.errorMessage = "Не удалось открыть файл";
            return out;
        }

        f.seekg(0, std::ios::end);
        const std::uint64_t fileSizeBytes = TellU64(f);
        f.seekg(0, std::ios::beg);

        char ver[4] {};
        if (!ReadExact(f, ver, sizeof(ver)))
        {
            out.errorMessage = "Не удалось прочитать заголовок";
            return out;
        }

        if (std::memcmp(ver, "VER2", 4) != 0)
        {
            out.errorMessage = "Неизвестная версия IMG (ожидалась VER2)";
            out.version = eImgArchiveVersion::Unknown;
            return out;
        }

        out.version = eImgArchiveVersion::V2;

        std::uint32_t entryCount = 0;
        if (!ReadExact(f, &entryCount, sizeof(entryCount)))
        {
            out.errorMessage = "Не удалось прочитать количество записей";
            return out;
        }

        out.entries.reserve(entryCount);
        out.indexByLowerName.reserve(entryCount);

        // IMG v2 directory entry (32 bytes):
        // 4 bytes DWORD offset (sectors)
        // 2 bytes WORD  streaming size (sectors)  (often used as "size")
        // 2 bytes WORD  size in archive (sectors) (often 0)
        // 24 bytes char[24] name (null-terminated)
        struct RawDirEntryV2
        {
            std::uint32_t offsetSectors;
            std::uint16_t streamingSectors;
            std::uint16_t sizeInArchiveSectors;
            char name[24];
        };

        for (std::uint32_t i = 0; i < entryCount; ++i)
        {
            RawDirEntryV2 raw {};
            if (!ReadExact(f, &raw, sizeof(raw)))
            {
                out.issues.push_back({static_cast<std::size_t>(i), "Не удалось прочитать directory entry"});
                break;
            }

            tImgArchiveEntry e;
            e.name = TrimNullTerminatedName(raw.name, sizeof(raw.name));
            e.offsetSectors = raw.offsetSectors;
            e.streamingSectors = raw.streamingSectors;
            e.sizeInArchiveSectors = raw.sizeInArchiveSectors;

            if (!e.name.empty())
            {
                const std::string lower = ImgToLowerAscii(e.name);
                const auto [it, inserted] = out.indexByLowerName.emplace(lower, out.entries.size());
                if (!inserted)
                {
                    out.issues.push_back({static_cast<std::size_t>(i), "Дубликат имени в IMG: " + e.name});
                }
            }
            else
            {
                out.issues.push_back({static_cast<std::size_t>(i), "Пустое имя у записи"});
            }

            const std::uint32_t effectiveSectors = (e.streamingSectors != 0) ? e.streamingSectors : e.sizeInArchiveSectors;
            if (effectiveSectors == 0)
            {
                out.issues.push_back({static_cast<std::size_t>(i), "Нулевой размер (sectors) у записи"});
            }

            const std::uint64_t offsetBytes64 = static_cast<std::uint64_t>(e.offsetSectors) * kImgSectorSizeBytes;
            const std::uint64_t streamingBytes64 = static_cast<std::uint64_t>(effectiveSectors) * kImgSectorSizeBytes;
            const std::uint64_t sizeInArchiveBytes64 = static_cast<std::uint64_t>(e.sizeInArchiveSectors) * kImgSectorSizeBytes;

            e.offsetBytes = static_cast<std::uint32_t>(std::min<std::uint64_t>(offsetBytes64, 0xFFFFFFFFull));
            e.streamingBytes = static_cast<std::uint32_t>(std::min<std::uint64_t>(streamingBytes64, 0xFFFFFFFFull));
            e.sizeInArchiveBytes = static_cast<std::uint32_t>(std::min<std::uint64_t>(sizeInArchiveBytes64, 0xFFFFFFFFull));

            if (offsetBytes64 > fileSizeBytes)
            {
                out.issues.push_back({static_cast<std::size_t>(i), "Offset выходит за пределы файла"});
            }
            else if (streamingBytes64 != 0 && (offsetBytes64 + streamingBytes64) > fileSizeBytes)
            {
                out.issues.push_back({static_cast<std::size_t>(i), "Размер выходит за пределы файла"});
            }

            out.entries.push_back(std::move(e));
        }

        // UI-friendly deterministic order:
        // beer_girla.dff, beer_girla.txd, beer_playa.dff, beer_playa.txd, ...
        out.sortedEntryIndices.resize(out.entries.size());
        std::iota(out.sortedEntryIndices.begin(), out.sortedEntryIndices.end(), std::size_t{0});
        std::sort(out.sortedEntryIndices.begin(), out.sortedEntryIndices.end(), [&](std::size_t lhs, std::size_t rhs)
        {
            const std::string aName = ImgToLowerAscii(out.entries[lhs].name);
            const std::string bName = ImgToLowerAscii(out.entries[rhs].name);

            const std::filesystem::path aPath(aName);
            const std::filesystem::path bPath(bName);

            const std::string aStem = aPath.stem().string();
            const std::string bStem = bPath.stem().string();
            if (aStem != bStem)
            {
                return aStem < bStem;
            }

            const std::string aExt = aPath.extension().string();
            const std::string bExt = bPath.extension().string();
            if (aExt != bExt)
            {
                return aExt < bExt;
            }

            return aName < bName;
        });

        return out;
    }
}

std::optional<std::size_t> tImgParseResult::FindEntryIndexByName(std::string_view name) const
{
    std::string key(name.begin(), name.end());
    key = detail_img::ImgToLowerAscii(std::move(key));
    const auto it = indexByLowerName.find(key);
    if (it == indexByLowerName.end())
    {
        return std::nullopt;
    }
    return it->second;
}

tImgParseResult ParseImgArchiveFile(const std::string& filePath)
{
    return detail_img::ParseImgArchiveFileImpl(filePath);
}

namespace detail_dff
{
    namespace
    {
        constexpr std::uint32_t kRwIdStruct = 0x00000001u;
        constexpr std::uint32_t kRwIdString = 0x00000002u;
        constexpr std::uint32_t kRwIdExtension = 0x00000003u;
        constexpr std::uint32_t kRwIdTexture = 0x00000006u;
        constexpr std::uint32_t kRwIdMaterial = 0x00000007u;
        constexpr std::uint32_t kRwIdMaterialList = 0x00000008u;
        constexpr std::uint32_t kRwIdFrameList = 0x0000000Eu;
        constexpr std::uint32_t kRwIdGeometry = 0x0000000Fu;
        constexpr std::uint32_t kRwIdClump = 0x00000010u;
        constexpr std::uint32_t kRwIdAtomic = 0x00000014u;
        /** UV Animation Dictionary (DragonFF / cpp-rwtools), часто идёт перед Clump в SA DFF. */
        constexpr std::uint32_t kRwIdUvAnimDict = 0x0000002Bu;
        constexpr std::uint32_t kRwIdGeometryList = 0x0000001Au;
        constexpr std::uint32_t kRwIdNodeName = 0x0253F2FEu; // Rockstar frame node-name plugin

        class BufferReader
        {
        public:
            BufferReader(const std::vector<std::uint8_t>& data, std::size_t begin, std::size_t end)
                : m_data(&data)
                , m_begin(begin)
                , m_end(end)
                , m_cursor(begin)
            {
            }

            std::size_t Tell() const { return m_cursor; }
            std::size_t Remaining() const { return (m_cursor <= m_end) ? (m_end - m_cursor) : 0; }
            const std::vector<std::uint8_t>& Data() const { return *m_data; }

            bool Skip(std::size_t count)
            {
                if (count > Remaining())
                {
                    return false;
                }
                m_cursor += count;
                return true;
            }

            bool ReadU32(std::uint32_t& out)
            {
                if (Remaining() < 4)
                {
                    return false;
                }
                const std::uint8_t* ptr = m_data->data() + m_cursor;
                out = static_cast<std::uint32_t>(ptr[0]) |
                    (static_cast<std::uint32_t>(ptr[1]) << 8) |
                    (static_cast<std::uint32_t>(ptr[2]) << 16) |
                    (static_cast<std::uint32_t>(ptr[3]) << 24);
                m_cursor += 4;
                return true;
            }

            bool ReadI32(std::int32_t& out)
            {
                std::uint32_t value = 0;
                if (!ReadU32(value))
                {
                    return false;
                }
                out = static_cast<std::int32_t>(value);
                return true;
            }

            bool ReadF32(float& out)
            {
                std::uint32_t raw = 0;
                if (!ReadU32(raw))
                {
                    return false;
                }
                std::memcpy(&out, &raw, sizeof(out));
                return true;
            }

            bool ReadU8(std::uint8_t& out)
            {
                if (Remaining() < 1)
                {
                    return false;
                }
                out = m_data->data()[m_cursor];
                ++m_cursor;
                return true;
            }

            bool ReadU16(std::uint16_t& out)
            {
                if (Remaining() < 2)
                {
                    return false;
                }
                const std::uint8_t* ptr = m_data->data() + m_cursor;
                out = static_cast<std::uint16_t>(ptr[0]) |
                    static_cast<std::uint16_t>(ptr[1] << 8);
                m_cursor += 2;
                return true;
            }

            bool Slice(std::size_t size, BufferReader& out)
            {
                if (size > Remaining())
                {
                    return false;
                }
                out = BufferReader(*m_data, m_cursor, m_cursor + size);
                m_cursor += size;
                return true;
            }

            bool ReadCString(std::string& out, std::size_t byteCount)
            {
                if (byteCount > Remaining())
                {
                    return false;
                }
                out.assign(reinterpret_cast<const char*>(m_data->data() + m_cursor), byteCount);
                const std::size_t termPos = out.find('\0');
                if (termPos != std::string::npos)
                {
                    out.resize(termPos);
                }
                while (!out.empty() && static_cast<unsigned char>(out.back()) <= 0x20u)
                {
                    out.pop_back();
                }
                m_cursor += byteCount;
                return true;
            }

        private:
            const std::vector<std::uint8_t>* m_data;
            std::size_t m_begin {};
            std::size_t m_end {};
            std::size_t m_cursor {};
        };

        void AddIssue(tDffParseResult& out, std::size_t offset, const std::string& message)
        {
            out.issues.push_back(tDffParseIssue {offset, message});
        }

        bool ReadChunk(BufferReader& reader, tDffChunkHeader& header, BufferReader& payload)
        {
            if (!reader.ReadU32(header.type) || !reader.ReadU32(header.size) || !reader.ReadU32(header.libraryIdStamp))
            {
                return false;
            }
            return reader.Slice(static_cast<std::size_t>(header.size), payload);
        }

        std::uint32_t DecodeRwVersion(std::uint32_t libIdStamp)
        {
            if (libIdStamp == 0u)
            {
                return 0u;
            }
            if ((libIdStamp & 0xFFFF0000u) != 0u)
            {
                return ((libIdStamp >> 16) & 0x0003FFFFu) + 0x30000u;
            }
            return libIdStamp;
        }

        bool ParseNodeNameFromExtension(BufferReader& extensionReader, std::string& nodeName)
        {
            while (extensionReader.Remaining() >= 12)
            {
                tDffChunkHeader childHeader {};
                BufferReader childPayload(extensionReader.Data(), 0, 0);
                if (!ReadChunk(extensionReader, childHeader, childPayload))
                {
                    return false;
                }
                if (childHeader.type == kRwIdNodeName)
                {
                    std::string parsedName;
                    if (!childPayload.ReadCString(parsedName, childPayload.Remaining()))
                    {
                        return false;
                    }
                    nodeName = std::move(parsedName);
                }
            }
            return true;
        }

        bool ParseFrameListChunk(BufferReader& frameListReader, tDffParseResult& out)
        {
            tDffChunkHeader structHeader {};
            BufferReader structPayload(frameListReader.Data(), 0, 0);
            if (!ReadChunk(frameListReader, structHeader, structPayload))
            {
                out.errorMessage = "Frame List: не удалось прочитать Struct";
                return false;
            }
            if (structHeader.type != kRwIdStruct)
            {
                out.errorMessage = "Frame List: ожидался Struct";
                return false;
            }

            std::uint32_t frameCount = 0;
            if (!structPayload.ReadU32(frameCount))
            {
                out.errorMessage = "Frame List: не удалось прочитать frame_count";
                return false;
            }
            out.frames.clear();
            out.frames.reserve(frameCount);

            for (std::uint32_t i = 0; i < frameCount; ++i)
            {
                tDffFrame frame {};
                float f = 0.0f;
                if (!structPayload.ReadF32(f))
                {
                    out.errorMessage = "Frame List: повреждён массив frame_data";
                    return false;
                }
                frame.right.x = f;
                if (!structPayload.ReadF32(f))
                {
                    out.errorMessage = "Frame List: повреждён массив frame_data";
                    return false;
                }
                frame.right.y = f;
                if (!structPayload.ReadF32(f))
                {
                    out.errorMessage = "Frame List: повреждён массив frame_data";
                    return false;
                }
                frame.right.z = f;

                if (!structPayload.ReadF32(f))
                {
                    out.errorMessage = "Frame List: повреждён массив frame_data";
                    return false;
                }
                frame.up.x = f;
                if (!structPayload.ReadF32(f))
                {
                    out.errorMessage = "Frame List: повреждён массив frame_data";
                    return false;
                }
                frame.up.y = f;
                if (!structPayload.ReadF32(f))
                {
                    out.errorMessage = "Frame List: повреждён массив frame_data";
                    return false;
                }
                frame.up.z = f;

                if (!structPayload.ReadF32(f))
                {
                    out.errorMessage = "Frame List: повреждён массив frame_data";
                    return false;
                }
                frame.at.x = f;
                if (!structPayload.ReadF32(f))
                {
                    out.errorMessage = "Frame List: повреждён массив frame_data";
                    return false;
                }
                frame.at.y = f;
                if (!structPayload.ReadF32(f))
                {
                    out.errorMessage = "Frame List: повреждён массив frame_data";
                    return false;
                }
                frame.at.z = f;

                if (!structPayload.ReadF32(f))
                {
                    out.errorMessage = "Frame List: повреждён массив frame_data";
                    return false;
                }
                frame.position.x = f;
                if (!structPayload.ReadF32(f))
                {
                    out.errorMessage = "Frame List: повреждён массив frame_data";
                    return false;
                }
                frame.position.y = f;
                if (!structPayload.ReadF32(f))
                {
                    out.errorMessage = "Frame List: повреждён массив frame_data";
                    return false;
                }
                frame.position.z = f;

                if (!structPayload.ReadI32(frame.parentIndex) || !structPayload.ReadU32(frame.matrixFlags))
                {
                    out.errorMessage = "Frame List: повреждён массив frame_data";
                    return false;
                }

                out.frames.push_back(std::move(frame));
            }

            std::size_t extensionFrameIndex = 0;
            while (frameListReader.Remaining() >= 12)
            {
                tDffChunkHeader childHeader {};
                BufferReader childPayload(frameListReader.Data(), 0, 0);
                const std::size_t headerOffset = frameListReader.Tell();
                if (!ReadChunk(frameListReader, childHeader, childPayload))
                {
                    AddIssue(out, headerOffset, "Frame List: не удалось прочитать дочерний chunk");
                    break;
                }

                if (childHeader.type == kRwIdExtension)
                {
                    std::string nodeName;
                    if (!ParseNodeNameFromExtension(childPayload, nodeName))
                    {
                        AddIssue(out, headerOffset, "Frame List: повреждён Extension chunk");
                        continue;
                    }
                    if (extensionFrameIndex < out.frames.size() && !nodeName.empty())
                    {
                        out.frames[extensionFrameIndex].nodeName = std::move(nodeName);
                    }
                    ++extensionFrameIndex;
                }
            }

            return true;
        }

        bool ParseGeometryChunk(BufferReader& geometryReader, tDffParseResult& out)
        {
            auto ParseStringChunk = [&](BufferReader& stringReader, std::string& value) -> bool
            {
                tDffChunkHeader header {};
                BufferReader payload(stringReader.Data(), 0, 0);
                if (!ReadChunk(stringReader, header, payload))
                {
                    return false;
                }
                if (header.type != kRwIdString)
                {
                    return false;
                }
                return payload.ReadCString(value, payload.Remaining());
            };

            auto ParseTextureChunk = [&](BufferReader& textureReader, tDffGeometry::tDffTextureInfo& outTexture) -> bool
            {
                tDffChunkHeader structHeader {};
                BufferReader structPayload(textureReader.Data(), 0, 0);
                if (!ReadChunk(textureReader, structHeader, structPayload) || structHeader.type != kRwIdStruct)
                {
                    return false;
                }

                std::uint32_t sampler = 0;
                if (!structPayload.ReadU32(sampler))
                {
                    return false;
                }
                outTexture.filteringMode = static_cast<std::uint8_t>(sampler & 0xFFu);
                outTexture.uAddressMode = static_cast<std::uint8_t>((sampler >> 8) & 0x0Fu);
                outTexture.vAddressMode = static_cast<std::uint8_t>((sampler >> 12) & 0x0Fu);
                outTexture.hasMipmaps = ((sampler >> 16) & 0x01u) != 0u;

                if (!ParseStringChunk(textureReader, outTexture.textureName))
                {
                    return false;
                }
                if (!ParseStringChunk(textureReader, outTexture.maskName))
                {
                    return false;
                }

                while (textureReader.Remaining() >= 12)
                {
                    tDffChunkHeader childHeader {};
                    BufferReader childPayload(textureReader.Data(), 0, 0);
                    if (!ReadChunk(textureReader, childHeader, childPayload))
                    {
                        return false;
                    }
                    if (childHeader.type == kRwIdExtension)
                    {
                        continue;
                    }
                }
                return true;
            };

            auto ParseMaterialChunk = [&](BufferReader& materialReader, tDffGeometry::tDffMaterialInfo& material) -> bool
            {
                tDffChunkHeader structHeader {};
                BufferReader structPayload(materialReader.Data(), 0, 0);
                if (!ReadChunk(materialReader, structHeader, structPayload) || structHeader.type != kRwIdStruct)
                {
                    return false;
                }

                std::uint8_t r = 0, g = 0, b = 0, a = 0;
                std::int32_t unused = 0;
                std::int32_t isTextured = 0;
                if (!structPayload.ReadU32(material.flags) ||
                    !structPayload.ReadU8(r) || !structPayload.ReadU8(g) || !structPayload.ReadU8(b) || !structPayload.ReadU8(a) ||
                    !structPayload.ReadI32(unused) ||
                    !structPayload.ReadI32(isTextured))
                {
                    return false;
                }
                material.colorR = r;
                material.colorG = g;
                material.colorB = b;
                material.colorA = a;
                material.isTextured = (isTextured != 0);

                if (out.rwVersion > 0x30400u && structPayload.Remaining() >= 12)
                {
                    if (!structPayload.ReadF32(material.ambient) ||
                        !structPayload.ReadF32(material.specular) ||
                        !structPayload.ReadF32(material.diffuse))
                    {
                        return false;
                    }
                }

                while (materialReader.Remaining() >= 12)
                {
                    tDffChunkHeader childHeader {};
                    BufferReader childPayload(materialReader.Data(), 0, 0);
                    if (!ReadChunk(materialReader, childHeader, childPayload))
                    {
                        return false;
                    }

                    if (childHeader.type == kRwIdTexture)
                    {
                        tDffGeometry::tDffTextureInfo texture {};
                        if (!ParseTextureChunk(childPayload, texture))
                        {
                            AddIssue(out, materialReader.Tell(), "Material: не удалось прочитать Texture chunk");
                            continue;
                        }
                        material.texture = std::move(texture);
                    }
                }
                return true;
            };

            auto ParseMaterialListChunk = [&](BufferReader& materialListReader, tDffGeometry& geometry) -> bool
            {
                tDffChunkHeader structHeader {};
                BufferReader structPayload(materialListReader.Data(), 0, 0);
                if (!ReadChunk(materialListReader, structHeader, structPayload) || structHeader.type != kRwIdStruct)
                {
                    return false;
                }

                std::int32_t materialCount = 0;
                if (!structPayload.ReadI32(materialCount) || materialCount < 0)
                {
                    return false;
                }

                geometry.materialIndices.clear();
                geometry.materials.clear();
                geometry.materialIndices.reserve(static_cast<std::size_t>(materialCount));
                geometry.materials.resize(static_cast<std::size_t>(materialCount));

                for (std::int32_t i = 0; i < materialCount; ++i)
                {
                    std::int32_t materialIndex = -1;
                    if (!structPayload.ReadI32(materialIndex))
                    {
                        return false;
                    }
                    geometry.materialIndices.push_back(materialIndex);
                }

                for (std::int32_t i = 0; i < materialCount; ++i)
                {
                    const std::int32_t idx = geometry.materialIndices[static_cast<std::size_t>(i)];
                    if (idx == -1)
                    {
                        tDffChunkHeader materialHeader {};
                        BufferReader materialPayload(materialListReader.Data(), 0, 0);
                        if (!ReadChunk(materialListReader, materialHeader, materialPayload))
                        {
                            AddIssue(out, materialListReader.Tell(), "Material List: недостаточно Material chunks");
                            break;
                        }
                        if (materialHeader.type != kRwIdMaterial)
                        {
                            AddIssue(out, materialListReader.Tell(), "Material List: ожидался Material chunk");
                            return false;
                        }
                        if (!ParseMaterialChunk(materialPayload, geometry.materials[static_cast<std::size_t>(i)]))
                        {
                            AddIssue(out, materialListReader.Tell(), "Material List: ошибка парсинга Material");
                        }
                        continue;
                    }

                    if (idx < 0 || idx >= i)
                    {
                        AddIssue(out, materialListReader.Tell(), "Material List: некорректный material instance index");
                        continue;
                    }
                    geometry.materials[static_cast<std::size_t>(i)] = geometry.materials[static_cast<std::size_t>(idx)];
                }

                return true;
            };

            tDffChunkHeader structHeader {};
            BufferReader structPayload(geometryReader.Data(), 0, 0);
            if (!ReadChunk(geometryReader, structHeader, structPayload))
            {
                out.errorMessage = "Geometry: не удалось прочитать Struct";
                return false;
            }
            if (structHeader.type != kRwIdStruct)
            {
                out.errorMessage = "Geometry: ожидался Struct";
                return false;
            }
            if (structPayload.Remaining() < 16)
            {
                out.errorMessage = "Geometry: слишком короткий Struct";
                return false;
            }

            tDffGeometry geometry {};
            if (!structPayload.ReadU32(geometry.header.format) ||
                !structPayload.ReadI32(geometry.header.triangleCount) ||
                !structPayload.ReadI32(geometry.header.vertexCount) ||
                !structPayload.ReadI32(geometry.header.morphTargetCount))
            {
                out.errorMessage = "Geometry: не удалось прочитать заголовок геометрии";
                return false;
            }

            geometry.header.isNative = (geometry.header.format & 0x01000000u) != 0u;
            geometry.header.texCoordSetCount = static_cast<std::uint8_t>((geometry.header.format >> 16) & 0xFFu);
            if (geometry.header.texCoordSetCount > 8)
            {
                AddIssue(out, structPayload.Tell(), "Geometry: число UV-наборов > 8");
            }
            if (geometry.header.vertexCount < 0 || geometry.header.triangleCount < 0 || geometry.header.morphTargetCount < 0)
            {
                AddIssue(out, structPayload.Tell(), "Geometry: отрицательные счётчики в Struct");
            }
            if (geometry.header.isNative)
            {
                AddIssue(out, structPayload.Tell(), "Geometry: native-геометрия пока не декодируется (поддержка только summary)");
            }
            else
            {
                const bool hasPrelit = (geometry.header.format & 0x00000008u) != 0u;
                const bool hasTextured = (geometry.header.format & 0x00000004u) != 0u;
                const bool hasTextured2 = (geometry.header.format & 0x00000080u) != 0u;
                std::size_t uvSetCount = static_cast<std::size_t>(geometry.header.texCoordSetCount);
                if (uvSetCount == 0)
                {
                    uvSetCount = hasTextured2 ? 2u : (hasTextured ? 1u : 0u);
                }

                const std::size_t vertexCount = static_cast<std::size_t>(geometry.header.vertexCount);
                const std::size_t triangleCount = static_cast<std::size_t>(geometry.header.triangleCount);
                const std::size_t morphTargetCount = static_cast<std::size_t>(geometry.header.morphTargetCount);

                if (hasPrelit)
                {
                    const std::size_t colorBytes = vertexCount * 4u;
                    if (!structPayload.Skip(colorBytes))
                    {
                        out.errorMessage = "Geometry: повреждён prelit-массив";
                        return false;
                    }
                }

                geometry.vertices.assign(vertexCount, {});
                if (uvSetCount > 0)
                {
                    for (std::size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
                    {
                        float u = 0.0f;
                        float v = 0.0f;
                        if (!structPayload.ReadF32(u) || !structPayload.ReadF32(v))
                        {
                            out.errorMessage = "Geometry: повреждён UV-массив";
                            return false;
                        }
                        geometry.vertices[vertexIndex].u = u;
                        geometry.vertices[vertexIndex].v = v;
                    }
                    const std::size_t uvBytesPerSet = vertexCount * sizeof(float) * 2u;
                    for (std::size_t uvSet = 1; uvSet < uvSetCount; ++uvSet)
                    {
                        if (!structPayload.Skip(uvBytesPerSet))
                        {
                            out.errorMessage = "Geometry: повреждён UV-массив (доп. набор)";
                            return false;
                        }
                    }
                }
                else
                {
                    const std::size_t uvBytesPerSet = vertexCount * sizeof(float) * 2u;
                    for (std::size_t uvSet = 0; uvSet < uvSetCount; ++uvSet)
                    {
                        if (!structPayload.Skip(uvBytesPerSet))
                        {
                            out.errorMessage = "Geometry: повреждён UV-массив";
                            return false;
                        }
                    }
                }

                geometry.triangleMaterialIds.clear();
                geometry.triangleMaterialIds.reserve(triangleCount);
                geometry.indices.clear();
                geometry.indices.reserve(triangleCount * 3u);
                for (std::size_t tri = 0; tri < triangleCount; ++tri)
                {
                    std::uint16_t v2 = 0;
                    std::uint16_t v1 = 0;
                    std::uint16_t materialId = 0;
                    std::uint16_t v3 = 0;
                    if (!structPayload.ReadU16(v2) ||
                        !structPayload.ReadU16(v1) ||
                        !structPayload.ReadU16(materialId) ||
                        !structPayload.ReadU16(v3))
                    {
                        out.errorMessage = "Geometry: повреждён triangle-массив";
                        return false;
                    }
                    geometry.triangleMaterialIds.push_back(materialId);
                    geometry.indices.push_back(static_cast<std::uint32_t>(v1));
                    geometry.indices.push_back(static_cast<std::uint32_t>(v2));
                    geometry.indices.push_back(static_cast<std::uint32_t>(v3));
                }

                bool capturedVertices = false;
                for (std::size_t morph = 0; morph < morphTargetCount; ++morph)
                {
                    float sphereX = 0.0f;
                    float sphereY = 0.0f;
                    float sphereZ = 0.0f;
                    float sphereRadius = 0.0f;
                    std::uint32_t hasVertices = 0;
                    std::uint32_t hasNormals = 0;
                    if (!structPayload.ReadF32(sphereX) ||
                        !structPayload.ReadF32(sphereY) ||
                        !structPayload.ReadF32(sphereZ) ||
                        !structPayload.ReadF32(sphereRadius) ||
                        !structPayload.ReadU32(hasVertices) ||
                        !structPayload.ReadU32(hasNormals))
                    {
                        out.errorMessage = "Geometry: повреждён morph target header";
                        return false;
                    }
                    (void)sphereX;
                    (void)sphereY;
                    (void)sphereZ;
                    (void)sphereRadius;

                    if (hasVertices != 0u)
                    {
                        if (!capturedVertices)
                        {
                            if (geometry.vertices.size() != vertexCount)
                            {
                                geometry.vertices.assign(vertexCount, {});
                            }
                            for (std::size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
                            {
                                if (!structPayload.ReadF32(geometry.vertices[vertexIndex].x) ||
                                    !structPayload.ReadF32(geometry.vertices[vertexIndex].y) ||
                                    !structPayload.ReadF32(geometry.vertices[vertexIndex].z))
                                {
                                    out.errorMessage = "Geometry: повреждён vertex-массив";
                                    return false;
                                }
                            }
                            capturedVertices = true;
                        }
                        else
                        {
                            const std::size_t verticesBytes = vertexCount * sizeof(float) * 3u;
                            if (!structPayload.Skip(verticesBytes))
                            {
                                out.errorMessage = "Geometry: повреждён vertex-массив";
                                return false;
                            }
                        }
                    }

                    if (hasNormals != 0u)
                    {
                        const std::size_t normalsBytes = vertexCount * sizeof(float) * 3u;
                        if (!structPayload.Skip(normalsBytes))
                        {
                            out.errorMessage = "Geometry: повреждён normal-массив";
                            return false;
                        }
                    }
                }

                if (geometry.vertices.empty() || geometry.indices.empty())
                {
                    AddIssue(out, structPayload.Tell(), "Geometry: отсутствуют вершины или треугольники");
                }
            }

            bool materialListParsed = false;
            while (geometryReader.Remaining() >= 12)
            {
                tDffChunkHeader childHeader {};
                BufferReader childPayload(geometryReader.Data(), 0, 0);
                const std::size_t childOffset = geometryReader.Tell();
                if (!ReadChunk(geometryReader, childHeader, childPayload))
                {
                    AddIssue(out, childOffset, "Geometry: не удалось прочитать дочерний chunk");
                    break;
                }

                if (childHeader.type == kRwIdMaterialList)
                {
                    if (!ParseMaterialListChunk(childPayload, geometry))
                    {
                        AddIssue(out, childOffset, "Geometry: ошибка Material List");
                    }
                    else
                    {
                        materialListParsed = true;
                    }
                    continue;
                }
            }

            if (!materialListParsed)
            {
                AddIssue(out, geometryReader.Tell(), "Geometry: Material List не найден");
            }

            out.geometries.push_back(std::move(geometry));
            return true;
        }

        bool ParseGeometryListChunk(BufferReader& geometryListReader, tDffParseResult& out)
        {
            tDffChunkHeader structHeader {};
            BufferReader structPayload(geometryListReader.Data(), 0, 0);
            if (!ReadChunk(geometryListReader, structHeader, structPayload))
            {
                out.errorMessage = "Geometry List: не удалось прочитать Struct";
                return false;
            }
            if (structHeader.type != kRwIdStruct)
            {
                out.errorMessage = "Geometry List: ожидался Struct";
                return false;
            }

            std::uint32_t geometryCount = 0;
            if (!structPayload.ReadU32(geometryCount))
            {
                out.errorMessage = "Geometry List: не удалось прочитать geometry_count";
                return false;
            }
            out.geometries.clear();
            out.geometries.reserve(geometryCount);

            for (std::uint32_t i = 0; i < geometryCount; ++i)
            {
                tDffChunkHeader geomHeader {};
                BufferReader geomPayload(geometryListReader.Data(), 0, 0);
                const std::size_t geomOffset = geometryListReader.Tell();
                if (!ReadChunk(geometryListReader, geomHeader, geomPayload))
                {
                    out.errorMessage = "Geometry List: не удалось прочитать Geometry chunk";
                    return false;
                }
                if (geomHeader.type != kRwIdGeometry)
                {
                    AddIssue(out, geomOffset, "Geometry List: ожидался chunk типа Geometry");
                    out.errorMessage = "Geometry List: нарушена структура";
                    return false;
                }
                if (!ParseGeometryChunk(geomPayload, out))
                {
                    return false;
                }
            }
            return true;
        }

        bool ParseAtomicChunk(BufferReader& atomicReader, tDffParseResult& out)
        {
            tDffChunkHeader structHeader {};
            BufferReader structPayload(atomicReader.Data(), 0, 0);
            if (!ReadChunk(atomicReader, structHeader, structPayload))
            {
                out.errorMessage = "Atomic: не удалось прочитать Struct";
                return false;
            }
            if (structHeader.type != kRwIdStruct)
            {
                out.errorMessage = "Atomic: ожидался Struct";
                return false;
            }
            if (structPayload.Remaining() < 12)
            {
                out.errorMessage = "Atomic: слишком короткий Struct";
                return false;
            }

            tDffAtomic atomic {};
            if (!structPayload.ReadI32(atomic.frameIndex) ||
                !structPayload.ReadI32(atomic.geometryIndex) ||
                !structPayload.ReadU32(atomic.flags))
            {
                out.errorMessage = "Atomic: не удалось прочитать поля frame/geometry/flags";
                return false;
            }
            if (structPayload.Remaining() >= 4)
            {
                if (!structPayload.ReadI32(atomic.unused))
                {
                    out.errorMessage = "Atomic: не удалось прочитать reserved-поле";
                    return false;
                }
            }

            out.atomics.push_back(atomic);
            return true;
        }

        bool ParseClumpChunk(BufferReader& clumpReader, tDffParseResult& out)
        {
            tDffChunkHeader clumpStructHeader {};
            BufferReader clumpStructPayload(clumpReader.Data(), 0, 0);
            if (!ReadChunk(clumpReader, clumpStructHeader, clumpStructPayload))
            {
                out.errorMessage = "Clump: не удалось прочитать Struct";
                return false;
            }
            if (clumpStructHeader.type != kRwIdStruct)
            {
                out.errorMessage = "Clump: ожидался Struct";
                return false;
            }

            out.atomicCount = 0;
            out.lightCount = 0;
            out.cameraCount = 0;
            if (!clumpStructPayload.ReadI32(out.atomicCount))
            {
                out.errorMessage = "Clump: не удалось прочитать atomic count";
                return false;
            }
            if (clumpStructPayload.Remaining() >= 4)
            {
                if (!clumpStructPayload.ReadI32(out.lightCount))
                {
                    out.errorMessage = "Clump: не удалось прочитать light count";
                    return false;
                }
            }
            if (clumpStructPayload.Remaining() >= 4)
            {
                if (!clumpStructPayload.ReadI32(out.cameraCount))
                {
                    out.errorMessage = "Clump: не удалось прочитать camera count";
                    return false;
                }
            }

            tDffChunkHeader frameListHeader {};
            BufferReader frameListPayload(clumpReader.Data(), 0, 0);
            if (!ReadChunk(clumpReader, frameListHeader, frameListPayload))
            {
                out.errorMessage = "Clump: не найден Frame List";
                return false;
            }
            if (frameListHeader.type != kRwIdFrameList)
            {
                out.errorMessage = "Clump: ожидался Frame List";
                return false;
            }
            if (!ParseFrameListChunk(frameListPayload, out))
            {
                return false;
            }

            tDffChunkHeader geometryListHeader {};
            BufferReader geometryListPayload(clumpReader.Data(), 0, 0);
            if (!ReadChunk(clumpReader, geometryListHeader, geometryListPayload))
            {
                out.errorMessage = "Clump: не найден Geometry List";
                return false;
            }
            if (geometryListHeader.type != kRwIdGeometryList)
            {
                out.errorMessage = "Clump: Geometry List отсутствует (старый DFF < 0x30400 не поддерживается в этом этапе)";
                return false;
            }
            if (!ParseGeometryListChunk(geometryListPayload, out))
            {
                return false;
            }

            out.atomics.clear();
            if (out.atomicCount < 0)
            {
                AddIssue(out, clumpReader.Tell(), "Clump: отрицательный atomic count");
                out.atomicCount = 0;
            }
            out.atomics.reserve(static_cast<std::size_t>(out.atomicCount));
            for (std::int32_t i = 0; i < out.atomicCount; ++i)
            {
                tDffChunkHeader atomicHeader {};
                BufferReader atomicPayload(clumpReader.Data(), 0, 0);
                const std::size_t atomicOffset = clumpReader.Tell();
                if (!ReadChunk(clumpReader, atomicHeader, atomicPayload))
                {
                    out.errorMessage = "Clump: не удалось прочитать Atomic chunk";
                    return false;
                }
                if (atomicHeader.type != kRwIdAtomic)
                {
                    AddIssue(out, atomicOffset, "Clump: ожидался chunk типа Atomic");
                    out.errorMessage = "Clump: нарушена структура атомиков";
                    return false;
                }
                if (!ParseAtomicChunk(atomicPayload, out))
                {
                    return false;
                }
            }

            return true;
        }

        void ValidateClumpLinks(tDffParseResult& out)
        {
            for (std::size_t i = 0; i < out.atomics.size(); ++i)
            {
                const tDffAtomic& atomic = out.atomics[i];
                if (atomic.frameIndex < 0 || static_cast<std::size_t>(atomic.frameIndex) >= out.frames.size())
                {
                    AddIssue(out, i, "Atomic: frameIndex вне диапазона Frame List");
                }
                if (atomic.geometryIndex < 0 || static_cast<std::size_t>(atomic.geometryIndex) >= out.geometries.size())
                {
                    AddIssue(out, i, "Atomic: geometryIndex вне диапазона Geometry List");
                }
            }
        }

        /**
         * Корневой Atomic (без Clump): как в DragonFF load_memory — внутри Atomic может лежать Geometry.
         */
        bool ParseRootAtomicDff(BufferReader atomicBody, const tDffChunkHeader& atomicHeader, tDffParseResult& out)
        {
            out.libraryIdStamp = atomicHeader.libraryIdStamp;
            out.rwVersion = DecodeRwVersion(atomicHeader.libraryIdStamp);
            out.frames.clear();
            out.geometries.clear();
            out.atomics.clear();
            out.atomicCount = 0;
            out.lightCount = 0;
            out.cameraCount = 0;

            while (atomicBody.Remaining() >= 12)
            {
                tDffChunkHeader childHeader {};
                BufferReader childPayload(atomicBody.Data(), 0, 0);
                if (!ReadChunk(atomicBody, childHeader, childPayload))
                {
                    out.errorMessage = "Atomic(root): не удалось прочитать дочерний chunk";
                    return false;
                }
                if (childHeader.type == kRwIdStruct)
                {
                    tDffAtomic atomic {};
                    if (childPayload.Remaining() < 12)
                    {
                        out.errorMessage = "Atomic(root): слишком короткий Struct";
                        return false;
                    }
                    if (!childPayload.ReadI32(atomic.frameIndex) ||
                        !childPayload.ReadI32(atomic.geometryIndex) ||
                        !childPayload.ReadU32(atomic.flags))
                    {
                        out.errorMessage = "Atomic(root): не удалось прочитать поля frame/geometry/flags";
                        return false;
                    }
                    if (childPayload.Remaining() >= 4)
                    {
                        if (!childPayload.ReadI32(atomic.unused))
                        {
                            out.errorMessage = "Atomic(root): не удалось прочитать reserved-поле";
                            return false;
                        }
                    }
                    out.atomics.push_back(atomic);
                }
                else if (childHeader.type == kRwIdGeometry)
                {
                    if (!ParseGeometryChunk(childPayload, out))
                    {
                        return false;
                    }
                }
                // Extension и прочие плагины — тело уже пропущено ReadChunk-ом для atomicBody
            }

            if (out.geometries.empty())
            {
                out.errorMessage = "Atomic(root): не найдена Geometry";
                return false;
            }

            if (out.frames.empty())
            {
                tDffFrame dummyFrame {};
                dummyFrame.parentIndex = -1;
                out.frames.push_back(dummyFrame);
            }

            if (out.atomics.empty())
            {
                tDffAtomic synthetic {};
                synthetic.frameIndex = 0;
                synthetic.geometryIndex = 0;
                synthetic.flags = 0;
                synthetic.unused = 0;
                out.atomics.push_back(synthetic);
            }

            out.atomicCount = static_cast<std::int32_t>(out.atomics.size());
            return true;
        }
    }

    static tDffParseResult ParseDffBufferImpl(const std::vector<std::uint8_t>& data)
    {
        tDffParseResult out;
        if (data.empty())
        {
            out.errorMessage = "DFF буфер пуст";
            return out;
        }

        BufferReader fileReader(data, 0, data.size());
        while (fileReader.Remaining() >= 12)
        {
            tDffChunkHeader header {};
            BufferReader payload(data, 0, 0);
            if (!ReadChunk(fileReader, header, payload))
            {
                out.errorMessage = "Не удалось прочитать chunk DFF";
                return out;
            }

            if (header.type == kRwIdClump)
            {
                out.libraryIdStamp = header.libraryIdStamp;
                out.rwVersion = DecodeRwVersion(header.libraryIdStamp);
                if (!ParseClumpChunk(payload, out))
                {
                    return out;
                }
                ValidateClumpLinks(out);
                return out;
            }

            if (header.type == kRwIdAtomic)
            {
                if (!ParseRootAtomicDff(payload, header, out))
                {
                    return out;
                }
                ValidateClumpLinks(out);
                return out;
            }

            if (header.type == kRwIdUvAnimDict)
            {
                continue;
            }

            // Неизвестный верхнеуровневый chunk — тело уже пропущено; ищем дальше Clump/Atomic.
        }

        out.errorMessage = "В DFF не найден поддерживаемый корень (Clump или Atomic)";
        return out;
    }

    static tDffParseResult ParseDffFileImpl(const std::string& filePath)
    {
        tDffParseResult out;
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open())
        {
            out.errorMessage = "Не удалось открыть DFF файл";
            return out;
        }
        file.seekg(0, std::ios::end);
        const std::streamoff size = file.tellg();
        if (size <= 0)
        {
            out.errorMessage = "DFF файл пуст";
            return out;
        }
        file.seekg(0, std::ios::beg);

        std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
        file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
        if (!file.good() && !file.eof())
        {
            out.errorMessage = "Не удалось прочитать DFF файл";
            return out;
        }
        return ParseDffBufferImpl(data);
    }

    static tDffParseResult ParseDffFromImgArchiveEntryImpl(const std::string& archivePath, const tImgArchiveEntry& entry)
    {
        tDffParseResult out;
        const std::uint32_t bytesToRead = (entry.streamingBytes != 0u) ? entry.streamingBytes : entry.sizeInArchiveBytes;
        if (bytesToRead == 0u)
        {
            out.errorMessage = "IMG entry имеет нулевой размер";
            return out;
        }

        std::ifstream archive(archivePath, std::ios::binary);
        if (!archive.is_open())
        {
            out.errorMessage = "Не удалось открыть IMG архив";
            return out;
        }
        archive.seekg(0, std::ios::end);
        const std::uint64_t archiveSize = static_cast<std::uint64_t>(archive.tellg());
        const std::uint64_t offset = static_cast<std::uint64_t>(entry.offsetBytes);
        const std::uint64_t endOffset = offset + static_cast<std::uint64_t>(bytesToRead);
        if (endOffset > archiveSize)
        {
            out.errorMessage = "IMG entry выходит за пределы архива";
            return out;
        }
        archive.seekg(static_cast<std::streamoff>(offset), std::ios::beg);

        std::vector<std::uint8_t> data(bytesToRead);
        archive.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
        if (!archive.good() && !archive.eof())
        {
            out.errorMessage = "Не удалось прочитать IMG entry";
            return out;
        }
        return ParseDffBufferImpl(data);
    }
}

std::optional<std::size_t> tDffParseResult::FindGeometryByAtomic(std::size_t atomicIndex) const
{
    if (atomicIndex >= atomics.size())
    {
        return std::nullopt;
    }
    const std::int32_t geoIdx = atomics[atomicIndex].geometryIndex;
    if (geoIdx < 0 || static_cast<std::size_t>(geoIdx) >= geometries.size())
    {
        return std::nullopt;
    }
    return static_cast<std::size_t>(geoIdx);
}

tDffParseResult ParseDffBuffer(const std::vector<std::uint8_t>& data)
{
    return detail_dff::ParseDffBufferImpl(data);
}

tDffParseResult ParseDffFile(const std::string& filePath)
{
    return detail_dff::ParseDffFileImpl(filePath);
}

tDffParseResult ParseDffFromImgArchiveEntry(const std::string& archivePath, const tImgArchiveEntry& entry)
{
    return detail_dff::ParseDffFromImgArchiveEntryImpl(archivePath, entry);
}

namespace detail_txd
{
namespace
{
constexpr std::uint32_t kRwIdStruct = 0x00000001u;
constexpr std::uint32_t kRwIdExtension = 0x00000003u;
constexpr std::uint32_t kRwIdTextureNative = 0x00000015u;
constexpr std::uint32_t kRwIdTextureDictionary = 0x00000016u;
constexpr std::uint32_t kRwIdPiTextureDictionary = 0x00000023u;

const std::vector<std::uint8_t>& TxdScratchEmptyBuffer()
{
    static const std::vector<std::uint8_t> kEmpty{};
    return kEmpty;
}

std::uint32_t DecodeRwVersionTxd(std::uint32_t libIdStamp)
{
    if (libIdStamp == 0u)
    {
        return 0u;
    }
    if ((libIdStamp & 0xFFFF0000u) != 0u)
    {
        return ((libIdStamp >> 16) & 0x0003FFFFu) + 0x30000u;
    }
    return libIdStamp;
}

class TxdBufferReader
{
public:
    TxdBufferReader()
        : m_data(&TxdScratchEmptyBuffer())
        , m_end(0)
        , m_cursor(0)
    {
    }

    TxdBufferReader(const std::vector<std::uint8_t>& data, std::size_t begin, std::size_t end)
        : m_data(&data)
        , m_end(end)
        , m_cursor(begin)
    {
    }

    std::size_t Tell() const { return m_cursor; }
    std::size_t Remaining() const { return (m_cursor <= m_end) ? (m_end - m_cursor) : 0; }

    bool ReadU8(std::uint8_t& out)
    {
        if (Remaining() < 1)
        {
            return false;
        }
        out = m_data->data()[m_cursor];
        ++m_cursor;
        return true;
    }

    bool ReadU16(std::uint16_t& out)
    {
        if (Remaining() < 2)
        {
            return false;
        }
        const std::uint8_t* ptr = m_data->data() + m_cursor;
        out = static_cast<std::uint16_t>(ptr[0]) | static_cast<std::uint16_t>(static_cast<std::uint16_t>(ptr[1]) << 8);
        m_cursor += 2;
        return true;
    }

    bool ReadU32(std::uint32_t& out)
    {
        if (Remaining() < 4)
        {
            return false;
        }
        const std::uint8_t* ptr = m_data->data() + m_cursor;
        out = static_cast<std::uint32_t>(ptr[0]) |
            (static_cast<std::uint32_t>(ptr[1]) << 8) |
            (static_cast<std::uint32_t>(ptr[2]) << 16) |
            (static_cast<std::uint32_t>(ptr[3]) << 24);
        m_cursor += 4;
        return true;
    }

    bool Slice(std::size_t size, TxdBufferReader& out)
    {
        if (size > Remaining())
        {
            return false;
        }
        out = TxdBufferReader(*m_data, m_cursor, m_cursor + size);
        m_cursor += size;
        return true;
    }

private:
    const std::vector<std::uint8_t>* m_data {};
    std::size_t m_end {};
    std::size_t m_cursor {};
};

bool ReadChunkTxd(TxdBufferReader& reader, tTxdChunkHeader& header, TxdBufferReader& payload)
{
    if (!reader.ReadU32(header.type) || !reader.ReadU32(header.size) || !reader.ReadU32(header.libraryIdStamp))
    {
        return false;
    }
    return reader.Slice(static_cast<std::size_t>(header.size), payload);
}

std::string RwPackedString32(const std::uint8_t* p)
{
    std::size_t len = 0;
    while (len < 32)
    {
        const unsigned char c = static_cast<unsigned char>(p[len]);
        if (c < 32 || c > 126)
        {
            break;
        }
        ++len;
    }
    return std::string(reinterpret_cast<const char*>(p), len);
}

// Значения как в d3d9types.h (GTA SA / RW используют те же коды).
constexpr std::uint32_t kD3dFmtR8G8B8 = 20; // D3DFMT_R8G8B8 — 3 байта на пиксель (BGR в растре)
constexpr std::uint32_t kD3dFmtA8R8G8B8 = 21;    // D3DFMT_A8R8G8B8
constexpr std::uint32_t kD3dFmtX8R8G8B8 = 22;     // D3DFMT_X8R8G8B8 — 4 байта (BGRX); часто помечают как «888» в редакторах
constexpr std::uint32_t kD3dFmt565 = 23;
constexpr std::uint32_t kD3dFmt555 = 24;
constexpr std::uint32_t kD3dFmt1555 = 25;
constexpr std::uint32_t kD3dFmt4444 = 26;
constexpr std::uint32_t kD3dFmtL8 = 50;
constexpr std::uint32_t kD3dFmtA8L8 = 51;
constexpr std::uint32_t kFourccDxt1 = 0x31545844u;
constexpr std::uint32_t kFourccDxt3 = 0x33545844u;
constexpr std::uint32_t kFourccDxt5 = 0x35545844u;

void ExpandRgb565(std::uint16_t c, std::uint8_t& r, std::uint8_t& g, std::uint8_t& b)
{
    r = static_cast<std::uint8_t>(((c >> 11) & 0x1Fu) * 255 / 31);
    g = static_cast<std::uint8_t>(((c >> 5) & 0x3Fu) * 255 / 63);
    b = static_cast<std::uint8_t>((c & 0x1Fu) * 255 / 31);
}

bool DecodeBc1Block(std::uint32_t bits, std::uint16_t c0, std::uint16_t c1, std::uint8_t outRgba[4 * 4])
{
    std::uint8_t r0{}, g0{}, b0{}, r1{}, g1{}, b1{};
    ExpandRgb565(c0, r0, g0, b0);
    ExpandRgb565(c1, r1, g1, b1);
    std::uint8_t pr[4]{}, pg[4]{}, pb[4]{}, pa[4]{};
    pr[0] = r0;
    pg[0] = g0;
    pb[0] = b0;
    pa[0] = 255;
    pr[1] = r1;
    pg[1] = g1;
    pb[1] = b1;
    pa[1] = 255;
    if (c0 > c1)
    {
        pr[2] = static_cast<std::uint8_t>((2 * r0 + r1) / 3);
        pg[2] = static_cast<std::uint8_t>((2 * g0 + g1) / 3);
        pb[2] = static_cast<std::uint8_t>((2 * b0 + b1) / 3);
        pa[2] = 255;
        pr[3] = static_cast<std::uint8_t>((r0 + 2 * r1) / 3);
        pg[3] = static_cast<std::uint8_t>((g0 + 2 * g1) / 3);
        pb[3] = static_cast<std::uint8_t>((b0 + 2 * b1) / 3);
        pa[3] = 255;
    }
    else
    {
        pr[2] = static_cast<std::uint8_t>((r0 + r1) / 2);
        pg[2] = static_cast<std::uint8_t>((g0 + g1) / 2);
        pb[2] = static_cast<std::uint8_t>((b0 + b1) / 2);
        pa[2] = 255;
        pr[3] = 0;
        pg[3] = 0;
        pb[3] = 0;
        pa[3] = 0;
    }
    std::uint32_t bbits = bits;
    for (int py = 0; py < 4; ++py)
    {
        for (int px = 0; px < 4; ++px)
        {
            const std::uint32_t code = bbits & 3u;
            bbits >>= 2;
            const int o = (py * 4 + px) * 4;
            outRgba[o + 0] = pr[code];
            outRgba[o + 1] = pg[code];
            outRgba[o + 2] = pb[code];
            outRgba[o + 3] = pa[code];
        }
    }
    return true;
}

bool DecodeBc1ToRgba(const std::uint8_t* src, std::size_t srcLen, std::uint32_t w, std::uint32_t h, std::vector<std::uint8_t>& rgba)
{
    const std::uint32_t bw = (w + 3u) / 4u;
    const std::uint32_t bh = (h + 3u) / 4u;
    const std::size_t need = static_cast<std::size_t>(bw) * static_cast<std::size_t>(bh) * 8u;
    if (srcLen < need)
    {
        return false;
    }
    rgba.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u, 0);
    std::size_t off = 0;
    std::uint8_t block[4 * 4 * 4]{};
    for (std::uint32_t by = 0; by < bh; ++by)
    {
        for (std::uint32_t bx = 0; bx < bw; ++bx)
        {
            const std::uint16_t c0 = static_cast<std::uint16_t>(src[off] | (src[off + 1] << 8));
            off += 2;
            const std::uint16_t c1 = static_cast<std::uint16_t>(src[off] | (src[off + 1] << 8));
            off += 2;
            const std::uint32_t cbits = static_cast<std::uint32_t>(src[off] | (src[off + 1] << 8) | (src[off + 2] << 16) | (src[off + 3] << 24));
            off += 4;
            DecodeBc1Block(cbits, c0, c1, block);
            for (int py = 0; py < 4; ++py)
            {
                for (int px = 0; px < 4; ++px)
                {
                    const std::uint32_t gx = bx * 4u + static_cast<std::uint32_t>(px);
                    const std::uint32_t gy = by * 4u + static_cast<std::uint32_t>(py);
                    if (gx >= w || gy >= h)
                    {
                        continue;
                    }
                    const int si = (py * 4 + px) * 4;
                    const std::size_t di = (static_cast<std::size_t>(gy) * w + gx) * 4u;
                    rgba[di + 0] = block[si + 0];
                    rgba[di + 1] = block[si + 1];
                    rgba[di + 2] = block[si + 2];
                    rgba[di + 3] = block[si + 3];
                }
            }
        }
    }
    return true;
}

bool DecodeBc2ToRgba(const std::uint8_t* src, std::size_t srcLen, std::uint32_t w, std::uint32_t h, std::vector<std::uint8_t>& rgba)
{
    const std::uint32_t bw = (w + 3u) / 4u;
    const std::uint32_t bh = (h + 3u) / 4u;
    const std::size_t need = static_cast<std::size_t>(bw) * static_cast<std::size_t>(bh) * 16u;
    if (srcLen < need)
    {
        return false;
    }
    rgba.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u, 0);
    std::size_t off = 0;
    std::uint8_t a4[16]{};
    std::uint8_t block[4 * 4 * 4]{};
    for (std::uint32_t by = 0; by < bh; ++by)
    {
        for (std::uint32_t bx = 0; bx < bw; ++bx)
        {
            for (int i = 0; i < 8; ++i)
            {
                const std::uint8_t b = src[off++];
                a4[i * 2] = static_cast<std::uint8_t>((b & 0x0Fu) * 17);
                a4[i * 2 + 1] = static_cast<std::uint8_t>(((b >> 4) & 0x0Fu) * 17);
            }
            const std::uint16_t c0 = static_cast<std::uint16_t>(src[off] | (src[off + 1] << 8));
            off += 2;
            const std::uint16_t c1 = static_cast<std::uint16_t>(src[off] | (src[off + 1] << 8));
            off += 2;
            const std::uint32_t bits = static_cast<std::uint32_t>(src[off] | (src[off + 1] << 8) | (src[off + 2] << 16) | (src[off + 3] << 24));
            off += 4;
            DecodeBc1Block(bits, c0, c1, block);
            for (int py = 0; py < 4; ++py)
            {
                for (int px = 0; px < 4; ++px)
                {
                    const std::uint32_t gx = bx * 4u + static_cast<std::uint32_t>(px);
                    const std::uint32_t gy = by * 4u + static_cast<std::uint32_t>(py);
                    if (gx >= w || gy >= h)
                    {
                        continue;
                    }
                    const int si = (py * 4 + px) * 4;
                    const std::size_t di = (static_cast<std::size_t>(gy) * w + gx) * 4u;
                    rgba[di + 0] = block[si + 0];
                    rgba[di + 1] = block[si + 1];
                    rgba[di + 2] = block[si + 2];
                    rgba[di + 3] = a4[py * 4 + px];
                }
            }
        }
    }
    return true;
}

bool DecodeBc3ToRgba(const std::uint8_t* src, std::size_t srcLen, std::uint32_t w, std::uint32_t h, std::vector<std::uint8_t>& rgba)
{
    const std::uint32_t bw = (w + 3u) / 4u;
    const std::uint32_t bh = (h + 3u) / 4u;
    const std::size_t need = static_cast<std::size_t>(bw) * static_cast<std::size_t>(bh) * 16u;
    if (srcLen < need)
    {
        return false;
    }
    rgba.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u, 0);
    std::size_t off = 0;
    std::uint8_t block[4 * 4 * 4]{};
    for (std::uint32_t by = 0; by < bh; ++by)
    {
        for (std::uint32_t bx = 0; bx < bw; ++bx)
        {
            const std::uint8_t a0 = src[off];
            const std::uint8_t a1 = src[off + 1];
            off += 2;
            std::uint64_t abits = static_cast<std::uint64_t>(src[off]) | (static_cast<std::uint64_t>(src[off + 1]) << 8) |
                (static_cast<std::uint64_t>(src[off + 2]) << 16) | (static_cast<std::uint64_t>(src[off + 3]) << 24) |
                (static_cast<std::uint64_t>(src[off + 4]) << 32) | (static_cast<std::uint64_t>(src[off + 5]) << 40);
            off += 6;
            std::uint8_t alphas[8]{};
            alphas[0] = a0;
            alphas[1] = a1;
            if (a0 > a1)
            {
                for (int i = 0; i < 6; ++i)
                {
                    alphas[2 + i] = static_cast<std::uint8_t>(((6 - i) * a0 + (1 + i) * a1) / 7);
                }
            }
            else
            {
                for (int i = 0; i < 4; ++i)
                {
                    alphas[2 + i] = static_cast<std::uint8_t>(((4 - i) * a0 + (1 + i) * a1) / 5);
                }
                alphas[6] = 0;
                alphas[7] = 255;
            }
            const std::uint16_t c0 = static_cast<std::uint16_t>(src[off] | (src[off + 1] << 8));
            off += 2;
            const std::uint16_t c1 = static_cast<std::uint16_t>(src[off] | (src[off + 1] << 8));
            off += 2;
            const std::uint32_t bits = static_cast<std::uint32_t>(src[off] | (src[off + 1] << 8) | (src[off + 2] << 16) | (src[off + 3] << 24));
            off += 4;
            DecodeBc1Block(bits, c0, c1, block);
            for (int py = 0; py < 4; ++py)
            {
                for (int px = 0; px < 4; ++px)
                {
                    const std::uint32_t gx = bx * 4u + static_cast<std::uint32_t>(px);
                    const std::uint32_t gy = by * 4u + static_cast<std::uint32_t>(py);
                    if (gx >= w || gy >= h)
                    {
                        continue;
                    }
                    const int pix = py * 4 + px;
                    const std::uint64_t aidx = (abits >> (pix * 3)) & 7u;
                    const int si = pix * 4;
                    const std::size_t di = (static_cast<std::size_t>(gy) * w + gx) * 4u;
                    rgba[di + 0] = block[si + 0];
                    rgba[di + 1] = block[si + 1];
                    rgba[di + 2] = block[si + 2];
                    rgba[di + 3] = alphas[static_cast<std::size_t>(aidx)];
                }
            }
        }
    }
    return true;
}

void BgraToRgbaInPlace(std::vector<std::uint8_t>& bgra)
{
    for (std::size_t i = 0; i + 3 < bgra.size(); i += 4)
    {
        const std::uint8_t t = bgra[i + 0];
        bgra[i + 0] = bgra[i + 2];
        bgra[i + 2] = t;
    }
}

bool TxdTryBuildPreview(tTxdTextureNativeInfo& te, const std::vector<std::uint8_t>& pal, const std::vector<std::vector<std::uint8_t>>& mips)
{
    te.previewRgba.clear();
    te.previewNoteRu.clear();
    if (te.width == 0 || te.height == 0)
    {
        te.previewNoteRu = "Некорректный размер текстуры.";
        return false;
    }
    if (static_cast<std::uint32_t>(te.width) > 2048u || static_cast<std::uint32_t>(te.height) > 2048u)
    {
        te.previewNoteRu = "Слишком большое разрешение для превью.";
        return false;
    }
    if (mips.empty() || mips[0].empty())
    {
        te.previewNoteRu = "Нет данных первого мип-уровня.";
        return false;
    }
    const std::uint8_t* pix = mips[0].data();
    const std::size_t psz = mips[0].size();
    const std::uint32_t w = te.width;
    const std::uint32_t h = te.height;
    const std::uint32_t palType = (te.rasterFormatFlags >> 13) & 3u;

    if (palType == 1u && te.depth == 8 && pal.size() >= 1024)
    {
        te.previewRgba.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u);
        if (psz < static_cast<std::size_t>(w) * static_cast<std::size_t>(h))
        {
            te.previewNoteRu = "Обрезаны индексы палитры.";
            return false;
        }
        for (std::uint32_t y = 0; y < h; ++y)
        {
            for (std::uint32_t x = 0; x < w; ++x)
            {
                const std::uint8_t idx = pix[static_cast<std::size_t>(y) * w + x];
                const std::uint8_t* p = pal.data() + static_cast<std::size_t>(idx) * 4u;
                std::uint8_t* d = te.previewRgba.data() + (static_cast<std::size_t>(y) * w + x) * 4u;
                d[0] = p[2];
                d[1] = p[1];
                d[2] = p[0];
                d[3] = p[3];
            }
        }
        te.previewNoteRu = "Палитра 8 бит (256 цветов), отображение как RGBA.";
        return true;
    }

    if (te.platformId == 9u || te.platformId == 0u)
    {
        switch (te.d3dFormat)
        {
        case kD3dFmtA8R8G8B8:
            if (psz < static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u)
            {
                te.previewNoteRu = "Недостаточно данных для BGRA 8888.";
                return false;
            }
            te.previewRgba.assign(pix, pix + static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u);
            BgraToRgbaInPlace(te.previewRgba);
            te.previewNoteRu = "Несжатый BGRA 8 бит на канал (D3DFMT_A8R8G8B8).";
            return true;
        case kD3dFmtX8R8G8B8:
            if (psz < static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u)
            {
                te.previewNoteRu = "Недостаточно данных для BGRX (X8R8G8B8).";
                return false;
            }
            te.previewRgba.assign(pix, pix + static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u);
            BgraToRgbaInPlace(te.previewRgba);
            for (std::size_t i = 3; i < te.previewRgba.size(); i += 4)
            {
                te.previewRgba[i] = 255;
            }
            te.previewNoteRu = "D3DFMT_X8R8G8B8 — 32 бит BGRX в файле (как «888» в Magic.TXD); альфа для превью = непрозрачно.";
            return true;
        case kD3dFmtR8G8B8:
            if (psz < static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 3u)
            {
                te.previewNoteRu = "Недостаточно данных для BGR 888 (24 бит).";
                return false;
            }
            te.previewRgba.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u);
            for (std::size_t i = 0, j = 0; i < psz && j < te.previewRgba.size(); i += 3, j += 4)
            {
                te.previewRgba[j + 0] = pix[i + 2];
                te.previewRgba[j + 1] = pix[i + 1];
                te.previewRgba[j + 2] = pix[i + 0];
                te.previewRgba[j + 3] = 255;
            }
            te.previewNoteRu = "D3DFMT_R8G8B8 — 24 бит BGR, без отдельной альфы.";
            return true;
        case kD3dFmt565:
            if (psz < static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 2u)
            {
                te.previewNoteRu = "Недостаточно данных для BGR 565.";
                return false;
            }
            te.previewRgba.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u);
            for (std::size_t i = 0; i < static_cast<std::size_t>(w) * static_cast<std::size_t>(h); ++i)
            {
                const std::uint16_t v = static_cast<std::uint16_t>(pix[i * 2] | (pix[i * 2 + 1] << 8));
                std::uint8_t r, g, b;
                ExpandRgb565(v, r, g, b);
                te.previewRgba[i * 4 + 0] = r;
                te.previewRgba[i * 4 + 1] = g;
                te.previewRgba[i * 4 + 2] = b;
                te.previewRgba[i * 4 + 3] = 255;
            }
            te.previewNoteRu = "Несжатый 16 бит RGB (5-6-5).";
            return true;
        case kD3dFmtL8:
            if (psz < static_cast<std::size_t>(w) * static_cast<std::size_t>(h))
            {
                te.previewNoteRu = "Недостаточно данных для L8.";
                return false;
            }
            te.previewRgba.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u);
            for (std::size_t i = 0; i < static_cast<std::size_t>(w) * static_cast<std::size_t>(h); ++i)
            {
                const std::uint8_t v = pix[i];
                te.previewRgba[i * 4 + 0] = v;
                te.previewRgba[i * 4 + 1] = v;
                te.previewRgba[i * 4 + 2] = v;
                te.previewRgba[i * 4 + 3] = 255;
            }
            te.previewNoteRu = "Один канал — яркость (серый).";
            return true;
        case kD3dFmtA8L8:
            if (psz < static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 2u)
            {
                te.previewNoteRu = "Недостаточно данных для A8L8.";
                return false;
            }
            te.previewRgba.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u);
            for (std::size_t i = 0; i < static_cast<std::size_t>(w) * static_cast<std::size_t>(h); ++i)
            {
                const std::uint8_t ll = pix[i * 2];
                const std::uint8_t aa = pix[i * 2 + 1];
                te.previewRgba[i * 4 + 0] = ll;
                te.previewRgba[i * 4 + 1] = ll;
                te.previewRgba[i * 4 + 2] = ll;
                te.previewRgba[i * 4 + 3] = aa;
            }
            te.previewNoteRu = "Два канала: яркость и альфа.";
            return true;
        case kFourccDxt1:
            if (DecodeBc1ToRgba(pix, psz, w, h, te.previewRgba))
            {
                te.previewNoteRu = "Сжатие BC1 / DXT1 (блоки 4×4).";
                return true;
            }
            te.previewNoteRu = "Ошибка разбора BC1 / DXT1.";
            return false;
        case kFourccDxt3:
            if (DecodeBc2ToRgba(pix, psz, w, h, te.previewRgba))
            {
                te.previewNoteRu = "Сжатие BC2 / DXT3 (явная альфа 4 бит).";
                return true;
            }
            te.previewNoteRu = "Ошибка разбора BC2 / DXT3.";
            return false;
        case kFourccDxt5:
            if (DecodeBc3ToRgba(pix, psz, w, h, te.previewRgba))
            {
                te.previewNoteRu = "Сжатие BC3 / DXT5 (интерполяция альфы).";
                return true;
            }
            te.previewNoteRu = "Ошибка разбора BC3 / DXT5.";
            return false;
        default:
        {
            char nb[192];
            std::snprintf(
                nb,
                sizeof(nb),
                "Формат Direct3D не поддержан для превью (код 0x%08X).",
                static_cast<unsigned>(te.d3dFormat));
            te.previewNoteRu = nb;
            return false;
        }
        }
    }

    if (te.platformId == 8u)
    {
        const unsigned dxt = te.platformPropertiesByte;
        if (dxt >= 1u && dxt <= 5u)
        {
            if (dxt == 1u && DecodeBc1ToRgba(pix, psz, w, h, te.previewRgba))
            {
                te.previewNoteRu = "D3D8: сжатие DXT1 / BC1.";
                return true;
            }
            if (dxt == 3u && DecodeBc2ToRgba(pix, psz, w, h, te.previewRgba))
            {
                te.previewNoteRu = "D3D8: сжатие DXT3 / BC2.";
                return true;
            }
            if ((dxt == 4u || dxt == 5u) && DecodeBc3ToRgba(pix, psz, w, h, te.previewRgba))
            {
                te.previewNoteRu = "D3D8: сжатие DXT5 / BC3.";
                return true;
            }
        }
        te.previewNoteRu = "D3D8: не удалось декодировать (ожидался DXT или нестандартный растр).";
        return false;
    }

    te.previewNoteRu = "Платформа не D3D8/D3D9 — превью не строится.";
    return false;
}

bool ParseTextureNativeStructPayload(TxdBufferReader& s, tTxdTextureNativeInfo& te, tTxdParseResult& out)
{
    constexpr std::size_t kMinForD3d = 72 + 15 + 1;
    if (s.Remaining() < kMinForD3d)
    {
        return false;
    }
    std::uint32_t platform = 0;
    std::uint8_t f0 = 0;
    std::uint8_t f1 = 0;
    std::uint16_t padU16 = 0;
    if (!s.ReadU32(platform) || !s.ReadU8(f0) || !s.ReadU8(f1) || !s.ReadU16(padU16))
    {
        return false;
    }
    (void)padU16;
    std::uint8_t nameBuf[32]{};
    std::uint8_t maskBuf[32]{};
    for (int i = 0; i < 32; ++i)
    {
        if (!s.ReadU8(nameBuf[i]))
        {
            return false;
        }
    }
    for (int i = 0; i < 32; ++i)
    {
        if (!s.ReadU8(maskBuf[i]))
        {
            return false;
        }
    }
    te.platformId = platform;
    te.filterMode = f0;
    te.uvAddressing = f1;
    te.name = RwPackedString32(nameBuf);
    te.mask = RwPackedString32(maskBuf);

    std::uint32_t rasterFmt = 0;
    std::uint32_t d3dFmt = 0;
    std::uint16_t w = 0;
    std::uint16_t h = 0;
    std::uint8_t d = 0;
    std::uint8_t nl = 0;
    std::uint8_t rt = 0;
    std::uint8_t platByte = 0;
    if (!s.ReadU32(rasterFmt) || !s.ReadU32(d3dFmt) || !s.ReadU16(w) || !s.ReadU16(h))
    {
        return false;
    }
    if (!s.ReadU8(d) || !s.ReadU8(nl) || !s.ReadU8(rt) || !s.ReadU8(platByte))
    {
        return false;
    }
    te.platformPropertiesByte = platByte;
    te.rasterFormatFlags = rasterFmt;
    te.d3dFormat = d3dFmt;
    te.width = w;
    te.height = h;
    te.depth = d;
    te.mipLevelCount = nl;
    te.rasterType = rt;

    if (platform != 8 && platform != 9 && platform != 0)
    {
        out.issues.push_back(tTxdParseIssue {s.Tell(), "Нативная текстура: platform_id не D3D8/D3D9, разметка может отличаться"});
    }

    const std::uint32_t palType = (rasterFmt >> 13) & 3u;
    std::size_t palSize = 0;
    if (palType == 1u)
    {
        palSize = 1024;
    }
    else if (palType == 2u)
    {
        palSize = (d == 4) ? 64u : 128u;
    }
    if (s.Remaining() < palSize)
    {
        return false;
    }
    std::vector<std::uint8_t> palette(palSize);
    for (std::size_t i = 0; i < palSize; ++i)
    {
        if (!s.ReadU8(palette[i]))
        {
            return false;
        }
    }

    std::vector<std::vector<std::uint8_t>> mips;
    mips.reserve(nl);
    for (std::uint32_t li = 0; li < static_cast<std::uint32_t>(nl); ++li)
    {
        std::uint32_t blockSz = 0;
        if (!s.ReadU32(blockSz))
        {
            return false;
        }
        if (s.Remaining() < static_cast<std::size_t>(blockSz))
        {
            return false;
        }
        std::vector<std::uint8_t> level(blockSz);
        for (std::uint32_t b = 0; b < blockSz; ++b)
        {
            if (!s.ReadU8(level[b]))
            {
                return false;
            }
        }
        mips.push_back(std::move(level));
    }

    TxdTryBuildPreview(te, palette, mips);
    return true;
}

void ParseTextureNativeChunkPayload(TxdBufferReader& nativeBody, tTxdParseResult& out)
{
    while (nativeBody.Remaining() >= 12)
    {
        tTxdChunkHeader ch {};
        TxdBufferReader pl {};
        if (!ReadChunkTxd(nativeBody, ch, pl))
        {
            out.issues.push_back(tTxdParseIssue {nativeBody.Tell(), "Texture Native: не удалось прочитать подчанк"});
            break;
        }
        if (ch.type == kRwIdStruct)
        {
            tTxdTextureNativeInfo te {};
            if (ParseTextureNativeStructPayload(pl, te, out))
            {
                out.textures.push_back(std::move(te));
            }
            else
            {
                out.issues.push_back(tTxdParseIssue {pl.Tell(), "Texture Native Struct: ошибка разбора (размер/формат)"});
            }
        }
        else if (ch.type == kRwIdExtension)
        {
            continue;
        }
        else
        {
            char buf[64];
            std::snprintf(
                buf,
                sizeof(buf),
                "Texture Native: неожиданный подчанк 0x%08X",
                static_cast<unsigned int>(ch.type));
            out.issues.push_back(tTxdParseIssue {nativeBody.Tell(), buf});
        }
    }
}

bool ParseTextureDictionaryPayload(TxdBufferReader& body, tTxdParseResult& out)
{
    bool sawDictStruct = false;
    std::uint16_t expectTextures = 0;
    std::uint16_t gotTextures = 0;

    while (body.Remaining() >= 12)
    {
        tTxdChunkHeader ch {};
        TxdBufferReader pl {};
        const std::size_t chunkStart = body.Tell();
        if (!ReadChunkTxd(body, ch, pl))
        {
            out.issues.push_back(tTxdParseIssue {chunkStart, "TXD: обрыв заголовка чанка"});
            return false;
        }
        if (ch.type == kRwIdStruct)
        {
            std::uint16_t tc = 0;
            std::uint16_t di = 0;
            if (!pl.ReadU16(tc) || !pl.ReadU16(di))
            {
                out.errorMessage = "Texture Dictionary: не удалось прочитать Struct (textureCount/deviceId)";
                return false;
            }
            out.dictionary.textureCount = tc;
            out.dictionary.deviceId = di;
            sawDictStruct = true;
            expectTextures = tc;
        }
        else if (ch.type == kRwIdTextureNative)
        {
            ParseTextureNativeChunkPayload(pl, out);
            ++gotTextures;
        }
        else if (ch.type == kRwIdExtension)
        {
            continue;
        }
        else
        {
            char buf[72];
            std::snprintf(buf, sizeof(buf), "TXD: неизвестный подчанк 0x%08X", static_cast<unsigned int>(ch.type));
            out.issues.push_back(tTxdParseIssue {chunkStart, buf});
        }
    }

    if (!sawDictStruct)
    {
        out.errorMessage = "Texture Dictionary: не найден Struct с числом текстур";
        return false;
    }
    if (expectTextures != gotTextures)
    {
        char buf[128];
        std::snprintf(
            buf,
            sizeof(buf),
            "Число Texture Native (%u) не совпадает с объявленным (%u)",
            static_cast<unsigned>(gotTextures),
            static_cast<unsigned>(expectTextures));
        out.issues.push_back(tTxdParseIssue {body.Tell(), buf});
    }
    return true;
}
} // namespace

static tTxdParseResult ParseTxdBufferImpl(const std::vector<std::uint8_t>& data)
{
    tTxdParseResult out {};
    TxdBufferReader root(data, 0, data.size());
    tTxdChunkHeader header {};
    TxdBufferReader payload {};
    if (data.size() < 12)
    {
        out.errorMessage = "TXD: файл слишком мал";
        return out;
    }
    if (!ReadChunkTxd(root, header, payload))
    {
        out.errorMessage = "TXD: не удалось прочитать корневой чанк";
        return out;
    }
    out.rwVersion = DecodeRwVersionTxd(header.libraryIdStamp);

    if (header.type == kRwIdPiTextureDictionary)
    {
        out.errorMessage = "PI Texture Dictionary (0x23) пока не поддержан";
        return out;
    }
    if (header.type != kRwIdTextureDictionary)
    {
        out.errorMessage = "TXD: ожидался Texture Dictionary (0x16)";
        return out;
    }

    if (!ParseTextureDictionaryPayload(payload, out) && out.errorMessage.empty())
    {
        out.errorMessage = "TXD: ошибка разбора словаря";
    }
    return out;
}

static tTxdParseResult ParseTxdFileImpl(const std::string& filePath)
{
    tTxdParseResult out {};
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        out.errorMessage = "TXD: не удалось открыть файл";
        return out;
    }
    const std::streamoff sz = file.tellg();
    if (sz <= 0)
    {
        out.errorMessage = (sz < 0) ? "TXD: не удалось определить размер файла" : "TXD: файл пуст";
        return out;
    }
    if (static_cast<std::uint64_t>(sz) > 128ull * 1024ull * 1024ull)
    {
        out.errorMessage = "TXD: файл слишком большой";
        return out;
    }
    std::vector<std::uint8_t> data(static_cast<std::size_t>(sz));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!file && !file.eof())
    {
        out.errorMessage = "TXD: ошибка чтения";
        return out;
    }
    return ParseTxdBufferImpl(data);
}
} // namespace detail_txd

tTxdParseResult ParseTxdBuffer(const std::vector<std::uint8_t>& data)
{
    return detail_txd::ParseTxdBufferImpl(data);
}

tTxdParseResult ParseTxdFile(const std::string& filePath)
{
    return detail_txd::ParseTxdFileImpl(filePath);
}

tTxdParseResult ParseTxdFromImgArchiveEntry(const std::string& archivePath, const tImgArchiveEntry& entry)
{
    tTxdParseResult out;
    const std::uint32_t bytesToRead = (entry.streamingBytes != 0u) ? entry.streamingBytes : entry.sizeInArchiveBytes;
    if (bytesToRead == 0u)
    {
        out.errorMessage = "IMG: запись TXD имеет нулевой размер";
        return out;
    }

    std::ifstream archive(archivePath, std::ios::binary);
    if (!archive.is_open())
    {
        out.errorMessage = "IMG: не удалось открыть архив";
        return out;
    }
    archive.seekg(0, std::ios::end);
    const std::uint64_t archiveSize = static_cast<std::uint64_t>(archive.tellg());
    const std::uint64_t offset = static_cast<std::uint64_t>(entry.offsetBytes);
    const std::uint64_t endOffset = offset + static_cast<std::uint64_t>(bytesToRead);
    if (endOffset > archiveSize)
    {
        out.errorMessage = "IMG: запись TXD выходит за пределы архива";
        return out;
    }
    archive.seekg(static_cast<std::streamoff>(offset), std::ios::beg);

    std::vector<std::uint8_t> data(bytesToRead);
    archive.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!archive.good() && !archive.eof())
    {
        out.errorMessage = "IMG: не удалось прочитать TXD из архива";
        return out;
    }
    return ParseTxdBuffer(data);
}

namespace detail_timecyc
{
namespace
{
constexpr float kTimeSlotMinutes[8] = {
    0.0f,      // Midnight
    5.0f * 60.0f,
    6.0f * 60.0f,
    7.0f * 60.0f,
    12.0f * 60.0f,
    19.0f * 60.0f,
    20.0f * 60.0f,
    22.0f * 60.0f,
};

// PC timecyc: после Amb (3), Amb_Obj (3), Dir (3) идут Sky top (3), Sky bottom (3).
constexpr int kMinTokensPerDataLine = 15;
constexpr int kSkyTopToken0 = 9;
constexpr int kSkyBottomToken0 = 12;

std::string TrimAscii(std::string s)
{
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
    {
        s.erase(s.begin());
    }
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t'))
    {
        s.pop_back();
    }
    return s;
}

bool IsWeatherHeaderLine(const std::string& line, std::string& outName)
{
    const std::string t = TrimAscii(line);
    if (t.size() < 4 || t[0] != '/')
    {
        return false;
    }
    std::size_t slashes = 0;
    while (slashes < t.size() && t[slashes] == '/')
    {
        ++slashes;
    }
    // "//Midnight" = 2 слэша — это не имя погоды, а подпись слота. Блоки: //////////// NAME
    if (slashes < 4)
    {
        return false;
    }
    std::size_t p = slashes;
    while (p < t.size() && (t[p] == ' ' || t[p] == '\t'))
    {
        ++p;
    }
    if (p >= t.size())
    {
        return false;
    }
    outName = TrimAscii(t.substr(p));
    return !outName.empty();
}

std::vector<std::string> SplitWhitespace(const std::string& line)
{
    std::vector<std::string> out;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok)
    {
        out.push_back(std::move(tok));
    }
    return out;
}

bool ParseRgbToken(const std::string& tok, int& v)
{
    try
    {
        std::size_t n = 0;
        const long x = std::stol(tok, &n, 10);
        if (n != tok.size())
        {
            return false;
        }
        if (x < 0 || x > 255)
        {
            return false;
        }
        v = static_cast<int>(x);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ParseDataLineToEntry(const std::vector<std::string>& tok, tTimecycEntry& e, std::string& err)
{
    if (tok.size() < static_cast<std::size_t>(kMinTokensPerDataLine))
    {
        err = "Слишком мало чисел в строке timecyc.";
        return false;
    }
    for (int c = 0; c < 3; ++c)
    {
        int r = 0;
        if (!ParseRgbToken(tok[static_cast<std::size_t>(kSkyTopToken0 + c)], r))
        {
            err = "Sky top: ожидался байт 0..255.";
            return false;
        }
        e.skyTopRgb[c] = static_cast<std::uint8_t>(r);
    }
    for (int c = 0; c < 3; ++c)
    {
        int r = 0;
        if (!ParseRgbToken(tok[static_cast<std::size_t>(kSkyBottomToken0 + c)], r))
        {
            err = "Sky bottom: ожидался байт 0..255.";
            return false;
        }
        e.skyBottomRgb[c] = static_cast<std::uint8_t>(r);
    }
    return true;
}

void LerpRgb(
    const std::uint8_t a[3],
    const std::uint8_t b[3],
    float u,
    float out[3]
)
{
    const float t = std::clamp(u, 0.0f, 1.0f);
    for (int i = 0; i < 3; ++i)
    {
        out[i] = (static_cast<float>(a[i]) + (static_cast<float>(b[i]) - static_cast<float>(a[i])) * t) / 255.0f;
    }
}
}
// namespace

static tTimecycFile ParseTimecycDatImpl(const std::wstring& pathUtf16)
{
    tTimecycFile out {};
    std::ifstream f(pathUtf16);
    if (!f)
    {
        out.errorMessage = "Не удалось открыть timecyc.dat";
        return out;
    }

    tTimecycWeather* current = nullptr;
    int slotFill = 0;

    std::string line;
    while (std::getline(f, line))
    {
        const std::string trimmed = TrimAscii(line);
        if (trimmed.empty())
        {
            continue;
        }

        std::string weatherName;
        if (IsWeatherHeaderLine(trimmed, weatherName))
        {
            if (current != nullptr && slotFill < 8 && !out.weathers.empty())
            {
                out.weathers.pop_back();
            }
            out.weathers.push_back(tTimecycWeather {});
            out.weathers.back().name = std::move(weatherName);
            current = &out.weathers.back();
            slotFill = 0;
            continue;
        }

        if (trimmed[0] == '/' || trimmed[0] == ';')
        {
            continue;
        }

        if (current == nullptr)
        {
            continue;
        }

        if (slotFill >= 8)
        {
            continue;
        }

        const std::vector<std::string> tok = SplitWhitespace(trimmed);
        tTimecycEntry ent {};
        std::string perr;
        if (!ParseDataLineToEntry(tok, ent, perr))
        {
            continue;
        }

        current->slots[static_cast<std::size_t>(slotFill)] = ent;
        ++slotFill;
    }

    if (current != nullptr && slotFill < 8 && !out.weathers.empty())
    {
        out.weathers.pop_back();
    }

    if (out.weathers.empty())
    {
        out.errorMessage = "В файле не найдено ни одного блока погоды (//////////// NAME).";
    }

    return out;
}

static void SampleSkyGradientImpl(
    const tTimecycFile& file,
    std::size_t weatherIndex,
    float minutesSinceMidnight,
    float outTopRgb[3],
    float outBottomRgb[3])
{
    for (int i = 0; i < 3; ++i)
    {
        outTopRgb[i] = 0.08f;
        outBottomRgb[i] = 0.12f;
    }

    if (file.weathers.empty() || weatherIndex >= file.weathers.size())
    {
        return;
    }

    const auto& w = file.weathers[weatherIndex];
    float T = std::fmod(minutesSinceMidnight, 1440.0f);
    if (T < 0.0f)
    {
        T += 1440.0f;
    }

    for (int seg = 0; seg < 8; ++seg)
    {
        const int i = seg;
        const int j = (seg + 1) % 8;
        const float ta = kTimeSlotMinutes[i];
        float tb = kTimeSlotMinutes[j];
        if (j == 0)
        {
            tb = 1440.0f;
        }

        if (T >= ta && T < tb)
        {
            const float u = (tb > ta) ? ((T - ta) / (tb - ta)) : 0.0f;
            LerpRgb(w.slots[static_cast<std::size_t>(i)].skyTopRgb, w.slots[static_cast<std::size_t>(j)].skyTopRgb, u, outTopRgb);
            LerpRgb(
                w.slots[static_cast<std::size_t>(i)].skyBottomRgb,
                w.slots[static_cast<std::size_t>(j)].skyBottomRgb,
                u,
                outBottomRgb
            );
            return;
        }
    }

    // T == 1440 или из-за погрешности — полночь
    LerpRgb(w.slots[7].skyTopRgb, w.slots[0].skyTopRgb, 1.0f, outTopRgb);
    LerpRgb(w.slots[7].skyBottomRgb, w.slots[0].skyBottomRgb, 1.0f, outBottomRgb);
}
}

tTimecycFile ParseTimecycDat(const std::wstring& pathUtf16)
{
    return detail_timecyc::ParseTimecycDatImpl(pathUtf16);
}

void SampleSkyGradient(const tTimecycFile& file, std::size_t weatherIndex, float minutesSinceMidnight, float outTopRgb[3], float outBottomRgb[3])
{
    detail_timecyc::SampleSkyGradientImpl(file, weatherIndex, minutesSinceMidnight, outTopRgb, outBottomRgb);
}

}
