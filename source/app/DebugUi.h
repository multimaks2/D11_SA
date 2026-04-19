#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
// Some Windows headers define `data` as a macro; that breaks qualified names like d11::data::t*.
#if defined(data)
#undef data
#endif

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "imgui.h"

#include "GameDataFormat.h"
#include "render/GtaStyleCamera.h"

class GameLoader;

inline float EffectiveMouseLookSensitivity(float baseSensitivity, float fovDegrees)
{
    constexpr float kMinFov = 1.0f;
    constexpr float kRefFov = 90.0f;
    constexpr float kMaxFov = 160.0f;
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kMinScale = 0.005f; // keep finite at very low FOV

    const float f = std::fmin(std::fmax(fovDegrees, kMinFov), kMaxFov);
    const float radHalf = (f * 0.5f) * (kPi / 180.0f);
    const float refRadHalf = (kRefFov * 0.5f) * (kPi / 180.0f);
    float scale = std::tan(radHalf) / std::tan(refRadHalf);
    if (scale < kMinScale)
    {
        scale = kMinScale;
    }
    return baseSensitivity * scale;
}

inline float BaseMouseLookSensitivityFromEffective(float effectiveSensitivity, float fovDegrees)
{
    constexpr float kMinFov = 1.0f;
    constexpr float kRefFov = 90.0f;
    constexpr float kMaxFov = 160.0f;
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kMinScale = 0.005f;

    const float f = std::fmin(std::fmax(fovDegrees, kMinFov), kMaxFov);
    const float radHalf = (f * 0.5f) * (kPi / 180.0f);
    const float refRadHalf = (kRefFov * 0.5f) * (kPi / 180.0f);
    float scale = std::tan(radHalf) / std::tan(refRadHalf);
    if (scale < kMinScale)
    {
        scale = kMinScale;
    }
    return effectiveSensitivity / scale;
}

class DebugUi
{
public:
    struct SceneStats
    {
        std::size_t instCount = 0;
        std::size_t uniqueModelCount = 0;
        std::size_t cubeInstances = 0;
        std::size_t dffMeshInstances = 0;
        std::size_t dffModelsParsed = 0;
        std::size_t dffModelsQueued = 0;
        std::uint64_t vertices = 0;
        std::uint64_t triangles = 0;
    };
    struct CameraControlSettings
    {
        float mouseLookSensitivity = 0.12f;
        float moveSpeed = 800.0f;
        float shiftSpeedMultiplier = 1.25f;
        float altSpeedMultiplier = 0.1f;
        float movementSpringStrength = 20.0f;
        float movementDamping = 0.85f;
    };

    struct FovScrollSettings
    {
        bool enabled = true;
        float step = 4.0f;
        float spring = 30.0f;
        float damping = 0.80f;
    };

    DebugUi() = default;
    ~DebugUi() = default;

    void ApplyStyle();

    void PushFrameTiming(float dtSeconds);
    void QueueFovScrollDelta(int wheelDelta, d11::render::GtaStyleCamera& camera);
    void UpdateFovInertia(d11::render::GtaStyleCamera& camera);
    void ResetFovInertiaToCamera(const d11::render::GtaStyleCamera& camera);

    const CameraControlSettings& GetCameraControlSettings() const { return m_cameraSettings; }
    const FovScrollSettings& GetFovScrollSettings() const { return m_fovSettings; }

    /** Минуты с полуночи (0..1440), дробные — темп как в GTA SA: 1 игровая минута за 1 реальную секунду. */
    float GetGtaGameMinutes() const { return m_gtaGameMinutes; }

    void SetGameLoader(const GameLoader* loader) { m_gameLoader = loader; }
    struct HoveredCubeInfo
    {
        std::int32_t objectId {};
        std::string modelName;
        std::int32_t flags {};
        float drawDistance {};
        std::int32_t lodIndex {};
        bool hasDffSummary = false;
        std::uint32_t dffRwVersion {};
        std::int32_t dffFrameCount {};
        std::int32_t dffGeometryCount {};
        std::int32_t dffAtomicCount {};
        std::int32_t dffIssueCount {};
        std::int32_t dffMaterialCount {};
        std::int32_t dffTexturedMaterialCount {};
        std::string dffTexturePreview;
        std::string dffArchiveName;
        std::string dffEntryName;
        std::string dffError;
    };
    void SetHoveredCubeInfo(const HoveredCubeInfo* info);
    void SetSceneStats(const SceneStats& stats) { m_sceneStats = stats; }
    void FocusTreeOnIplInstance(std::int32_t objectId, const std::string& modelName, float x, float y, float z);

    void DrawUi(d11::render::GtaStyleCamera& camera);
    bool ConsumeImportSceneRequest()
    {
        const bool requested = m_importSceneRequested;
        m_importSceneRequested = false;
        return requested;
    }
    bool IsImportQuaternionMode() const { return m_useQuaternionImportMode; }
    void ToggleImportRotationMode() { m_useQuaternionImportMode = !m_useQuaternionImportMode; }
    bool IsDffMeshesVisible() const { return m_showImportedDffMeshes; }
    bool IsFallbackCubesVisible() const { return m_showImportedFallbackCubes; }
    /** Отсечение DFF по draw distance IDE (как включённый SA LOD в старом клиенте). По умолчанию выкл. — все объекты. */
    bool IsSceneDffIdeDistanceCull() const { return m_sceneDffIdeDistanceCull; }

    bool IsSceneGridVisible() const { return m_showSceneGrid; }
    bool IsSceneAxesVisible() const { return m_showSceneAxes; }
    bool IsSceneSkyboxVisible() const { return m_showSceneSkybox; }

    void QueueModelPreviewFromIde(std::string modelName);
    bool ConsumeModelPreviewRequest(std::string& outModelName);
    /** Вызывается из Dx11RenderPipeline: текстура превью, валидность, число граней (треугольников), вершин в VB, ошибка. */
    void SetModelPreviewFrame(
        ImTextureID texId,
        bool textureValid,
        std::uint32_t triangleCount,
        std::uint32_t vertexCount,
        const char* errorOrNull);
    [[nodiscard]] const std::string& GetModelPreviewDisplayName() const { return m_modelPreviewDisplayName; }
    [[nodiscard]] std::uint32_t GetModelPreviewTriangleCount() const { return m_modelPreviewTriangleCount; }
    [[nodiscard]] std::uint32_t GetModelPreviewVertexCount() const { return m_modelPreviewVertexCount; }
    /** Увеличивается при каждом новом запросе превью — пайплайн пересобирает меш. */
    [[nodiscard]] std::uint32_t GetModelPreviewLoadToken() const { return m_modelPreviewLoadToken; }
    bool IsModelPreviewWindowOpen() const { return m_modelPreviewWindowOpen; }
    void GetAndResetModelPreviewNav(
        float& yawDeltaDeg,
        float& pitchDeltaDeg,
        float& wheelSteps,
        float& panMouseDx,
        float& panMouseDy);

    [[nodiscard]] bool IsTxdInspectWindowOpen() const { return m_txdInspectWindowOpen; }
    /** После открытия/смены TXD — один раз true, затем сброс (см. Dx11RenderPipeline::UpdateTxdInspectThumbnails). */
    [[nodiscard]] bool ConsumeTxdInspectGpuDirty();
    void AssignTxdInspectThumbnails(std::vector<ImTextureID>&& ids);
    void ClearTxdInspectGpuThumbnails();
    [[nodiscard]] const d11::data::tTxdParseResult& GetTxdInspectParse() const { return m_txdInspectParse; }
    [[nodiscard]] ImTextureID GetTxdInspectThumbnail(std::size_t index) const;

private:
    enum class UiPanel : std::uint8_t
    {
        Tree = 0,
        Camera,
        Performance,
        Controls,
        Scene
    };

    void DrawTopBar(float dpiScale, float frameMs, float fps, float topBarHeight, float topButtonHeight, float topBarPaddingY);
    void DrawTreeWindow(const ImVec2& panelPos, float panelWidth, float panelCornerRounding, float fontScale);
    void DrawCameraWindow(const ImVec2& panelPos, float panelWidth, d11::render::GtaStyleCamera& camera, float fontScale, float panelCornerRounding);
    void DrawPerformanceWindow(const ImVec2& panelPos, float panelWidth, float frameMs, float panelCornerRounding, float fontScale);
    void DrawControlsWindow(const ImVec2& panelPos, float panelWidth, float panelCornerRounding, float fontScale);
    void DrawSceneWindow(const ImVec2& panelPos, float panelWidth, float panelCornerRounding, float fontScale);
    void DrawHoveredCubeTooltip() const;
    void DrawModelPreviewWindow();
    /** Окно того же размера, что превью DFF: список текстур из .txd (двойной клик в дереве models/TXD). */
    void DrawTxdInspectWindow();
    void OpenTxdInspectorFromFile(const std::filesystem::path& absolutePath, std::string displayName);
    void OpenTxdInspectorFromImgEntry(const std::filesystem::path& archiveAbsolute, const d11::data::tImgArchiveEntry& entry);
    void ResetCameraSettingsToDefaults(d11::render::GtaStyleCamera& camera);

    bool m_showTreeWindow = true;
    bool m_showCameraWindow = false;
    bool m_showPerformanceWindow = false;
    bool m_showControlsWindow = false;
    bool m_showSceneWindow = false;
    bool m_importSceneRequested = false;
    bool m_useQuaternionImportMode = true;
    bool m_showImportedDffMeshes = true;
    bool m_sceneDffIdeDistanceCull = false;
    bool m_showImportedFallbackCubes = true;
    bool m_showSceneWater = true;
    bool m_showSceneGrid = true;
    bool m_showSceneAxes = true;
    bool m_showSceneSkybox = false;

    std::string m_modelPreviewPendingRequest;
    std::string m_modelPreviewDisplayName;
    std::uint32_t m_modelPreviewLoadToken = 0;
    bool m_modelPreviewWindowOpen = false;
    ImTextureID m_modelPreviewTexId {};
    bool m_modelPreviewTexValid = false;
    std::uint32_t m_modelPreviewTriangleCount = 0;
    std::uint32_t m_modelPreviewVertexCount = 0;
    std::string m_modelPreviewStatusText;
    float m_modelPreviewNavYawAccum = 0.0f;
    float m_modelPreviewNavPitchAccum = 0.0f;
    float m_modelPreviewNavWheelAccum = 0.0f;
    float m_modelPreviewNavPanXAccum = 0.0f;
    float m_modelPreviewNavPanYAccum = 0.0f;
    bool m_modelPreviewLmbOrbitCapture = false;
    bool m_modelPreviewMmbPanCapture = false;

    bool m_txdInspectWindowOpen = false;
    std::string m_txdInspectDisplayName;
    d11::data::tTxdParseResult m_txdInspectParse {};
    bool m_txdInspectGpuDirty = false;
    std::vector<ImTextureID> m_txdInspectThumbnails;

    CameraControlSettings m_cameraSettings {};
    FovScrollSettings m_fovSettings {};

    float m_fovTarget = 90.0f;
    float m_fovVelocity = 0.0f;

    static constexpr std::size_t kFrameHistorySize = 240;
    std::array<float, kFrameHistorySize> m_frameTimeHistoryMs {};
    std::size_t m_frameHistoryCount = 0;
    std::size_t m_frameHistoryHead = 0;
    float m_lastFrameDt = 0.0f;

    float m_gtaGameMinutes = 0.0f;

    const GameLoader* m_gameLoader = nullptr;

    // UI caches for the gta.dat tree to avoid heavy per-frame work.
    std::size_t m_cachedLoadOrderSize = 0;
    std::vector<const d11::data::tGtaDatCatalogEntry*> m_cachedIdeEntries;
    std::vector<const d11::data::tGtaDatCatalogEntry*> m_cachedIplEntries;
    std::vector<const d11::data::tGtaDatCatalogEntry*> m_cachedImgEntries;
    std::vector<const d11::data::tGtaDatCatalogEntry*> m_cachedOtherEntries;

    std::size_t m_cachedIdeResultsSize = 0;
    std::size_t m_cachedIplResultsSize = 0;
    std::unordered_map<std::string, std::size_t> m_cachedIdeCountByAbsPath;
    std::unordered_map<std::string, std::size_t> m_cachedIplCountByAbsPath;
    std::unordered_map<std::string, const d11::data::tIdeParseResult*> m_cachedIdeResByAbsPath;
    std::unordered_map<std::string, const d11::data::tIplParseResult*> m_cachedIplResByAbsPath;

    // gta.dat IMG parsing cache (IMG/CdImage/ImgList that point to .img archives).
    std::size_t m_cachedImgResultsSize = 0;
    std::unordered_map<std::string, std::size_t> m_cachedImgCountByAbsPath;
    std::unordered_map<std::string, std::shared_ptr<const d11::data::tImgParseResult>> m_cachedImgResByAbsPath;
    std::size_t m_gtaDatImgInitNextIndex = 0;

    // models/IMG cache (scanned from game_root/models/*.img, case-insensitive).
    std::filesystem::path m_cachedModelsRoot;
    bool m_modelsImgScanned = false;
    std::vector<std::string> m_cachedModelsImgPaths;
    /** Относительные пути .txd под game_root/models (рекурсивно, с подпапками). */
    std::vector<std::string> m_cachedModelsTxdPaths;
    std::size_t m_modelsImgInitNextIndex = 0;

    // Parsed IMG archives cache: absPath -> result.
    std::unordered_map<std::string, std::shared_ptr<const d11::data::tImgParseResult>> m_cachedModelsImgParsed;
    std::optional<HoveredCubeInfo> m_hoveredCubeInfo;
    std::optional<SceneStats> m_sceneStats;
    char m_treeSearchBuffer[128] {};
    bool m_treeFocusPending = false;
    std::int32_t m_treeFocusObjectId = 0;
    float m_treeFocusPosX = 0.0f;
    float m_treeFocusPosY = 0.0f;
    float m_treeFocusPosZ = 0.0f;
    std::string m_treeFocusModelLower;
};
