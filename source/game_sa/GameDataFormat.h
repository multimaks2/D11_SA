#pragma once

// Форматы данных карты GTA SA (IDE / IPL / DAT / IMG / DFF / TXD / timecyc / ZON) и POD-типы.
// Именование в духе gta-reversed: enum class e*, структуры из файлов t*, фабрики Make*.
// Реализации — в FileLoader.cpp (аналог CFileLoader.cpp в gta-reversed).

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace d11::data
{
    struct tVector3
    {
        double x {};
        double y {};
        double z {};
    };

    struct tQuaternion
    {
        double x {};
        double y {};
        double z {};
        double w {};
    };

    // IPL (Item Placement)
    struct tIplInstEntry
    {
        tVector3 position {};
        tQuaternion rotation {};
        std::string modelName;
        std::int32_t objectId {}; // Index to IDE model
        std::int32_t interior {};
        std::int32_t lodIndex {};
    };

    struct tIplCarsEntry
    {
        tVector3 position {};
        float angleZ {};
        std::int32_t vehicleId {}; // Index to IDE cars model
        std::int32_t primaryColor {};
        std::int32_t secondaryColor {};
        std::int32_t forceSpawn {};
        std::int32_t alarmProbability {};
        std::int32_t lockProbability {};
        std::int32_t unknown1 {};
        std::int32_t unknown2 {};
    };

    struct tIplZoneEntry
    {
        std::string name;
        std::int32_t type {};
        tVector3 minBounds {};
        tVector3 maxBounds {};
        std::int32_t level {};
    };

    struct tIplDataSet
    {
        std::vector<tIplInstEntry> inst;
        std::vector<tIplCarsEntry> cars;
        std::vector<tIplZoneEntry> zones;
    };

    struct tIplParseIssue
    {
        std::size_t line {};
        std::string message;
    };

    struct tIplParseResult
    {
        tIplDataSet data;
        std::vector<tIplParseIssue> issues;
    };

    // IDE (Item Definition)
    enum class eIdeSection
    {
        Objs,
        Tobj,
        Hier,
        Cars,
        Peds,
        Path,
        Fx2d,
        Weap,
        Anim,
        Txdp,
        Unknown
    };

    struct tIdeBaseObjectEntry
    {
        std::int32_t id {};
        std::string modelName;
        std::string textureDictionary;
        std::array<float, 3> drawDistance {0.0f, 0.0f, 0.0f};
        std::uint8_t drawDistanceCount {0};
        std::int32_t flags {};
    };

    struct tIdeTimedObjectEntry : tIdeBaseObjectEntry
    {
        std::int32_t timeOnHour {};
        std::int32_t timeOffHour {};
    };

    struct tIdeAnimatedObjectEntry : tIdeBaseObjectEntry
    {
        std::string animationFile;
    };

    struct tIdeTextureParentEntry
    {
        std::string textureDictionary;
        std::string parentTextureDictionary;
    };

    struct tIdeGenericSectionEntry
    {
        eIdeSection section {eIdeSection::Unknown};
        std::vector<std::string> tokens;
    };

    using tIdeEntry = std::variant<tIdeBaseObjectEntry, tIdeTimedObjectEntry, tIdeAnimatedObjectEntry, tIdeTextureParentEntry, tIdeGenericSectionEntry>;

    struct tIdeParseIssue
    {
        std::size_t line {};
        std::string message;
    };

    struct tIdeParseResult
    {
        std::vector<tIdeEntry> entries;
        std::vector<tIdeParseIssue> issues;
    };

    // DAT (gta.dat style map listing)
    enum class eDatKeyword
    {
        CdImage,
        Img,
        ImgList,
        Water,
        Ide,
        ColFile,
        MapZone,
        Ipl,
        TexDiction,
        ModelFile,
        Splash,
        HierFile,
        Exit,
        Unknown
    };

    struct tDatPathEntry
    {
        eDatKeyword keyword {eDatKeyword::Unknown};
        std::string path;
    };

    struct tDatColFileEntry
    {
        std::int32_t zoneId {};
        std::string path;
    };

    struct tDatWaterEntry
    {
        std::vector<std::string> paths;
    };

    struct tDatSplashEntry
    {
        std::string textureName;
    };

    struct tDatExitEntry
    {
    };

    using tDatEntry = std::variant<tDatPathEntry, tDatColFileEntry, tDatWaterEntry, tDatSplashEntry, tDatExitEntry>;

    struct tCatalogEntity
    {
        eDatKeyword keyword {eDatKeyword::Unknown};
        std::string pathOrText;
        virtual ~tCatalogEntity() = default;
    };

    struct tGtaDatCatalogEntry : tCatalogEntity
    {
        std::int32_t colFileZoneId {};
        bool skippedAsZoneIpl {false};
    };

    struct tGtaDatParseSummary
    {
        std::filesystem::path sourcePath;
        std::vector<tGtaDatCatalogEntry> loadOrder;
        std::string errorMessage;
    };

    void LoadGtaDatCatalog(const std::filesystem::path& gtaDatPath, tGtaDatParseSummary& outSummary);

    static constexpr std::uint32_t kImgSectorSizeBytes = 2048;

    enum class eImgArchiveVersion
    {
        Unknown = 0,
        V1 = 1,
        V2 = 2,
        V3 = 3
    };

    struct tImgArchiveEntry
    {
        std::string name;
        std::uint32_t offsetSectors {};
        std::uint32_t offsetBytes {};
        std::uint16_t streamingSectors {};
        std::uint16_t sizeInArchiveSectors {};
        std::uint32_t streamingBytes {};
        std::uint32_t sizeInArchiveBytes {};
    };

    struct tImgParseIssue
    {
        std::size_t index {};
        std::string message;
    };

    struct tImgParseResult
    {
        eImgArchiveVersion version {eImgArchiveVersion::Unknown};
        std::vector<tImgArchiveEntry> entries;
        std::vector<std::size_t> sortedEntryIndices;
        std::unordered_map<std::string, std::size_t> indexByLowerName;
        std::vector<tImgParseIssue> issues;
        std::string errorMessage;

        std::optional<std::size_t> FindEntryIndexByName(std::string_view name) const;
    };

    struct tDffChunkHeader
    {
        std::uint32_t type {};
        std::uint32_t size {};
        std::uint32_t libraryIdStamp {};
    };

    struct tDffFrame
    {
        tVector3 right {};
        tVector3 up {};
        tVector3 at {};
        tVector3 position {};
        std::int32_t parentIndex {-1};
        std::uint32_t matrixFlags {};
        std::string nodeName;
    };

    struct tDffGeometryHeader
    {
        std::uint32_t format {};
        std::int32_t triangleCount {};
        std::int32_t vertexCount {};
        std::int32_t morphTargetCount {};
        bool isNative {};
        std::uint8_t texCoordSetCount {};
    };

    struct tDffGeometry
    {
        struct tDffVertex
        {
            float x {};
            float y {};
            float z {};
            float u {};
            float v {};
        };

        struct tDffNormal
        {
            float x {};
            float y {};
            float z {};
        };

        tDffGeometryHeader header {};
        struct tDffTextureInfo
        {
            std::uint8_t filteringMode {};
            std::uint8_t uAddressMode {};
            std::uint8_t vAddressMode {};
            bool hasMipmaps {};
            std::string textureName;
            std::string maskName;
        };

        struct tDffMaterialInfo
        {
            std::uint32_t flags {};
            std::uint8_t colorR {};
            std::uint8_t colorG {};
            std::uint8_t colorB {};
            std::uint8_t colorA {};
            bool isTextured {};
            float ambient {};
            float specular {};
            float diffuse {};
            std::optional<tDffTextureInfo> texture;
        };

        std::vector<tDffMaterialInfo> materials;
        std::vector<std::int32_t> materialIndices;
        /** Индекс материала на треугольник (порядок как в triangle list). */
        std::vector<std::uint16_t> triangleMaterialIds;
        std::vector<tDffVertex> vertices;
        std::vector<tDffNormal> normals;
        std::vector<std::uint32_t> prelitColorsRGBA;
        std::vector<std::uint32_t> indices;
    };

    struct tDffAtomic
    {
        std::int32_t frameIndex {-1};
        std::int32_t geometryIndex {-1};
        std::uint32_t flags {};
        std::int32_t unused {};
    };

    struct tDffParseIssue
    {
        std::size_t offset {};
        std::string message;
    };

    struct tDffParseResult
    {
        std::uint32_t rwVersion {};
        std::uint32_t libraryIdStamp {};
        std::int32_t atomicCount {};
        std::int32_t lightCount {};
        std::int32_t cameraCount {};

        std::vector<tDffFrame> frames;
        std::vector<tDffGeometry> geometries;
        std::vector<tDffAtomic> atomics;
        std::vector<tDffParseIssue> issues;
        std::string errorMessage;

        bool IsOk() const { return errorMessage.empty(); }
        std::optional<std::size_t> FindGeometryByAtomic(std::size_t atomicIndex) const;
    };

    // TXD (RenderWare Texture Dictionary). Разметка чанков совпадает с DFF: type/size/libraryId (12 байт) + payload.
    struct tTxdChunkHeader
    {
        std::uint32_t type {};
        std::uint32_t size {};
        std::uint32_t libraryIdStamp {};
    };

    /** Struct первого подчанка Texture Dictionary: число текстур и device id (как в DragonFF / RW). */
    struct tTxdDictionaryStruct
    {
        std::uint16_t textureCount {};
        std::uint16_t deviceId {};
    };

    /** Сводка по одной записи Texture Native (D3D8/D3D9 и «device_id=0» в словаре). */
    struct tTxdTextureNativeInfo
    {
        std::string name;
        std::string mask;
        std::uint32_t platformId {};
        std::uint8_t filterMode {};
        std::uint8_t uvAddressing {};
        std::uint32_t rasterFormatFlags {};
        std::uint32_t d3dFormat {};
        std::uint16_t width {};
        std::uint16_t height {};
        std::uint8_t depth {};
        std::uint8_t mipLevelCount {};
        std::uint8_t rasterType {};
        /** Байт после растра (D3D8: тип DXT; D3D9: битовые флаги RW). */
        std::uint8_t platformPropertiesByte {};
        /** Первый мип, RGBA8 для ImGui / D3D R8G8B8A8 (пусто, если не удалось декодировать). */
        std::vector<std::uint8_t> previewRgba;
        /** Кратко по-русски: удалось ли построить превью и какой формат. */
        std::string previewNoteRu;
    };

    struct tTxdParseIssue
    {
        std::size_t offset {};
        std::string message;
    };

    struct tTxdParseResult
    {
        std::uint32_t rwVersion {};
        tTxdDictionaryStruct dictionary {};
        std::vector<tTxdTextureNativeInfo> textures;
        std::vector<tTxdParseIssue> issues;
        std::string errorMessage;

        bool IsOk() const { return errorMessage.empty(); }
    };

    struct tZonEntry
    {
        std::string name;
        std::int32_t type {};
        tVector3 minBounds {};
        tVector3 maxBounds {};
        std::int32_t level {};
        std::optional<std::string> text;
    };

    struct tTimecycEntry
    {
        std::uint8_t skyTopRgb[3] {0, 0, 0};
        std::uint8_t skyBottomRgb[3] {0, 0, 0};
    };

    struct tTimecycWeather
    {
        std::string name;
        std::array<tTimecycEntry, 8> slots {};
    };

    struct tTimecycFile
    {
        std::vector<tTimecycWeather> weathers;
        std::string errorMessage;
    };

    tIplInstEntry MakeIplInstEntry(const tVector3& position, const tQuaternion& rotation, std::int32_t objectId, std::int32_t interior, std::int32_t lodIndex);
    tIplCarsEntry MakeIplCarsEntry(const tVector3& position, float angleZ, std::int32_t vehicleId, std::int32_t primaryColor, std::int32_t secondaryColor, std::int32_t forceSpawn, std::int32_t alarmProbability, std::int32_t lockProbability, std::int32_t unknown1, std::int32_t unknown2);

    tIdeBaseObjectEntry MakeIdeBaseObjectEntry(std::int32_t id, const std::string& modelName, const std::string& textureDictionary, const std::array<float, 3>& drawDistance, std::uint8_t drawDistanceCount, std::int32_t flags);
    tIdeTimedObjectEntry MakeIdeTimedObjectEntry(std::int32_t id, const std::string& modelName, const std::string& textureDictionary, const std::array<float, 3>& drawDistance, std::uint8_t drawDistanceCount, std::int32_t flags, std::int32_t timeOnHour, std::int32_t timeOffHour);
    tIdeAnimatedObjectEntry MakeIdeAnimatedObjectEntry(std::int32_t id, const std::string& modelName, const std::string& textureDictionary, const std::array<float, 3>& drawDistance, std::uint8_t drawDistanceCount, std::int32_t flags, const std::string& animationFile);
    tIdeTextureParentEntry MakeIdeTextureParentEntry(const std::string& textureDictionary, const std::string& parentTextureDictionary);
    tIdeGenericSectionEntry MakeIdeGenericSectionEntry(eIdeSection section, std::vector<std::string> tokens);
    tIdeParseResult ParseIdeFile(const std::string& filePath);
    tIplParseResult ParseIplFile(const std::string& filePath);
    tImgParseResult ParseImgArchiveFile(const std::string& filePath);
    tDffParseResult ParseDffFile(const std::string& filePath);
    tDffParseResult ParseDffBuffer(const std::vector<std::uint8_t>& data);
    tDffParseResult ParseDffFromImgArchiveEntry(const std::string& archivePath, const tImgArchiveEntry& entry);
    tTxdParseResult ParseTxdFile(const std::string& filePath);
    tTxdParseResult ParseTxdBuffer(const std::vector<std::uint8_t>& data);
    tTxdParseResult ParseTxdFromImgArchiveEntry(const std::string& archivePath, const tImgArchiveEntry& entry);

    tDatPathEntry MakeDatPathEntry(eDatKeyword keyword, const std::string& path);
    tDatColFileEntry MakeDatColFileEntry(std::int32_t zoneId, const std::string& path);
    tDatWaterEntry MakeDatWaterEntry(std::vector<std::string> paths);
    tDatSplashEntry MakeDatSplashEntry(const std::string& textureName);
    tDatExitEntry MakeDatExitEntry();

    tZonEntry MakeZonEntry(const std::string& name, std::int32_t type, const tVector3& minBounds, const tVector3& maxBounds, std::int32_t level, const std::optional<std::string>& text = std::nullopt);

    tTimecycFile ParseTimecycDat(const std::wstring& pathUtf16);
    void SampleSkyGradient(const tTimecycFile& file, std::size_t weatherIndex, float minutesSinceMidnight, float outTopRgb[3], float outBottomRgb[3]);
}
