#include "DebugUi.h"

#include "../game_sa/GameLoader.h"
#include "../game_sa/GameDataFormat.h"

#include "imgui_impl_dx11.h"

#include <array>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>

namespace
{
using d11::data::eDatKeyword;

constexpr float kInertiaFixedTimeStep = 1.0f / 60.0f;
constexpr float kUiFontScale = 0.78f;
constexpr float kTopBarBaseHeight = 20.0f;
constexpr float kTopButtonBaseHeight = 15.0f;
constexpr float kTopBarBasePaddingY = 2.5f;
constexpr float kPanelBaseRounding = 8.0f;
constexpr float kPanelPosOffsetY = 6.0f;

// Значения D3DFMT как в d3d9types.h / FileLoader.cpp.
constexpr std::uint32_t kTxdD3dFmtR8G8B8 = 20;
constexpr std::uint32_t kTxdD3dFmtA8R8G8B8 = 21;
constexpr std::uint32_t kTxdD3dFmtX8R8G8B8 = 22;
constexpr std::uint32_t kTxdD3dFmt565 = 23;
constexpr std::uint32_t kTxdD3dFmt555 = 24;
constexpr std::uint32_t kTxdD3dFmt1555 = 25;
constexpr std::uint32_t kTxdD3dFmt4444 = 26;
constexpr std::uint32_t kTxdD3dFmtL8 = 50;
constexpr std::uint32_t kTxdD3dFmtA8L8 = 51;
constexpr std::uint32_t kTxdFourccDxt1 = 0x31545844u;
constexpr std::uint32_t kTxdFourccDxt3 = 0x33545844u;
constexpr std::uint32_t kTxdFourccDxt5 = 0x35545844u;

std::string TxdDictionaryDeviceRu(std::uint16_t deviceId)
{
    switch (deviceId)
    {
    case 0:
        return "не задано (0) — обычно словарь под Direct3D 9";
    case 1:
        return "Direct3D 8";
    case 2:
        return "Direct3D 9";
    default:
    {
        char b[96];
        std::snprintf(b, sizeof(b), "код устройства в словаре: %u (см. RenderWare)", static_cast<unsigned>(deviceId));
        return b;
    }
    }
}

std::string TxdPlatformNativeRu(std::uint32_t platformId)
{
    switch (platformId)
    {
    case 0:
        return "нативный растр: D3D9 (platform_id 0)";
    case 8:
        return "нативный растр: Direct3D 8";
    case 9:
        return "нативный растр: Direct3D 9";
    default:
    {
        char b[192];
        std::snprintf(
            b,
            sizeof(b),
            "нативный растр: нестандартный platform_id %u (разметка может отличаться)",
            static_cast<unsigned>(platformId));
        return b;
    }
    }
}

std::string TxdD3dFormatRu(std::uint32_t fmt)
{
    switch (fmt)
    {
    case kTxdD3dFmtR8G8B8:
        return "D3DFMT_R8G8B8 — 24 бит, 3 байта на пиксель (BGR в растре), без отдельной альфы";
    case kTxdD3dFmtA8R8G8B8:
        return "D3DFMT_A8R8G8B8 — 32 бит, полный цвет и альфа (в файле обычно BGRA)";
    case kTxdD3dFmtX8R8G8B8:
        return "D3DFMT_X8R8G8B8 — 32 бит BGRX (четвёртый байт не альфа); в редакторах часто подпись «888» / BGRA";
    case kTxdD3dFmt565:
        return "D3DFMT_R5G6B5 — 16 бит на пиксель, цвет";
    case kTxdD3dFmt555:
        return "D3DFMT_X1R5G5B5 — 16 бит, без альфы (1 бит не используется)";
    case kTxdD3dFmt1555:
        return "D3DFMT_A1R5G5B5 — 16 бит, грубая альфа (1 бит)";
    case kTxdD3dFmt4444:
        return "D3DFMT_A4R4G4B4 — 16 бит, цвет и альфа по 4 бита";
    case kTxdD3dFmtL8:
        return "D3DFMT_L8 — один канал (серый / яркость)";
    case kTxdD3dFmtA8L8:
        return "D3DFMT_A8L8 — яркость и альфа, два канала";
    case kTxdFourccDxt1:
        return "FourCC «DXT1» (BC1) — блоковое сжатие цвета, простая прозрачность";
    case kTxdFourccDxt3:
        return "FourCC «DXT3» (BC2) — сжатие с явной альфой 4 бита на пиксель";
    case kTxdFourccDxt5:
        return "FourCC «DXT5» (BC3) — сжатие цвета + интерполируемая альфа";
    default:
    {
        char b[128];
        std::snprintf(
            b,
            sizeof(b),
            "формат по коду 0x%08X — см. D3DFMT / FourCC в документации Direct3D",
            static_cast<unsigned>(fmt));
        return b;
    }
    }
}

std::string TxdMipLevelsRu(std::uint8_t mipLevelCount)
{
    const unsigned n = static_cast<unsigned>(mipLevelCount);
    if (n == 0u)
    {
        return "Число мип-уровней: 0 — для текстуры это подозрительно.";
    }
    if (n == 1u)
    {
        return "Мип-уровни: 1 — хранится только полное разрешение, цепочки уменьшенных копий (mipmap) нет.";
    }
    char b[160];
    std::snprintf(
        b,
        sizeof(b),
        "Мип-уровни: %u — одна полноразмерная текстура и ещё %u уменьшённых (всего ступеней mip-chain).",
        n,
        n - 1u);
    return b;
}

const char* TxdFilterModeRu(std::uint8_t mode)
{
    switch (mode)
    {
    case 0:
        return "не задан / по умолчанию движка";
    case 1:
        return "ближайший сосед (резко)";
    case 2:
        return "линейная интерполяция";
    case 3:
        return "выбор мип-уровня + ближайший сосед";
    case 4:
        return "выбор мип-уровня + линейная";
    case 5:
        return "линейная по экрану, ближайший мип";
    case 6:
        return "трилинейная (линейно по экрану и между мипами)";
    default:
        return nullptr;
    }
}

const char* TxdUvAddressRu(std::uint8_t mode)
{
    switch (mode)
    {
    case 0:
        return "повтор (wrap)";
    case 1:
        return "зеркало (mirror)";
    case 2:
        return "зажим (clamp)";
    case 3:
        return "цвет границы (border)";
    default:
        return nullptr;
    }
}

namespace fs = std::filesystem;

std::string MakePathKey(const std::filesystem::path& p)
{
    std::string s = p.lexically_normal().string();
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}

std::string ToLowerAsciiCopy(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string FormatCountCompactRu(std::uint64_t value)
{
    const std::uint64_t million = 1000000ull;
    const std::uint64_t thousand = 1000ull;

    if (value >= million)
    {
        const std::uint64_t mln = value / million;
        const std::uint64_t rem_th = (value % million) / thousand;
        char b[96];
        if (rem_th > 0ull)
        {
            std::snprintf(
                b,
                sizeof(b),
                "%llu млн %llu тыс (%llu)",
                static_cast<unsigned long long>(mln),
                static_cast<unsigned long long>(rem_th),
                static_cast<unsigned long long>(value));
        }
        else
        {
            std::snprintf(
                b,
                sizeof(b),
                "%llu млн (%llu)",
                static_cast<unsigned long long>(mln),
                static_cast<unsigned long long>(value));
        }
        return b;
    }

    if (value >= thousand)
    {
        const std::uint64_t th = value / thousand;
        const std::uint64_t rem = value % thousand;
        char b[96];
        if (rem > 0ull)
        {
            std::snprintf(
                b,
                sizeof(b),
                "%llu тыс %llu (%llu)",
                static_cast<unsigned long long>(th),
                static_cast<unsigned long long>(rem),
                static_cast<unsigned long long>(value));
        }
        else
        {
            std::snprintf(
                b,
                sizeof(b),
                "%llu тыс (%llu)",
                static_cast<unsigned long long>(th),
                static_cast<unsigned long long>(value));
        }
        return b;
    }

    char b[48];
    std::snprintf(b, sizeof(b), "%llu", static_cast<unsigned long long>(value));
    return b;
}

bool ContainsCaseInsensitive(const std::string& haystack, const std::string& needleLower)
{
    if (needleLower.empty())
    {
        return true;
    }
    std::string loweredHay = ToLowerAsciiCopy(haystack);
    return loweredHay.find(needleLower) != std::string::npos;
}

bool ParseExactIdQuery(const std::string& query, std::int32_t& outId)
{
    if (query.empty())
    {
        return false;
    }
    char* endPtr = nullptr;
    const long value = std::strtol(query.c_str(), &endPtr, 10);
    if (endPtr == query.c_str() || *endPtr != '\0')
    {
        return false;
    }
    if (value < static_cast<long>((std::numeric_limits<std::int32_t>::min)()) ||
        value > static_cast<long>((std::numeric_limits<std::int32_t>::max)()))
    {
        return false;
    }
    outId = static_cast<std::int32_t>(value);
    return true;
}

bool IsNearFloat(float a, float b, float eps = 0.01f)
{
    return std::fabs(a - b) <= eps;
}

std::string TryResolveIdeModelNameFromId(const GameLoader::ReadAccess& access, std::int32_t objectId)
{
    for (const auto& ideResult : access.ideResults)
    {
        for (const auto& entry : ideResult.entries)
        {
            std::string found;
            std::visit(
                [&](auto&& v)
                {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, d11::data::tIdeBaseObjectEntry> ||
                                  std::is_same_v<T, d11::data::tIdeTimedObjectEntry> ||
                                  std::is_same_v<T, d11::data::tIdeAnimatedObjectEntry>)
                    {
                        if (v.id == objectId && !v.modelName.empty())
                        {
                            found = v.modelName;
                        }
                    }
                },
                entry);
            if (!found.empty())
            {
                return found;
            }
        }
    }
    return {};
}

bool FileNameLooksLikeDff(const std::string& name)
{
    if (name.size() < 5)
    {
        return false;
    }
    std::string tail = name.substr(name.size() - 4);
    std::transform(tail.begin(), tail.end(), tail.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return tail == ".dff";
}

bool FileNameLooksLikeTxd(const std::string& name)
{
    if (name.size() < 5)
    {
        return false;
    }
    std::string tail = name.substr(name.size() - 4);
    std::transform(tail.begin(), tail.end(), tail.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return tail == ".txd";
}

const char* DatKeywordShortName(d11::data::eDatKeyword k)
{
    using d11::data::eDatKeyword;
    switch (k)
    {
    case eDatKeyword::CdImage:
        return "CDIMAGE";
    case eDatKeyword::Img:
        return "IMG";
    case eDatKeyword::ImgList:
        return "IMGLIST";
    case eDatKeyword::Water:
        return "WATER";
    case eDatKeyword::Ide:
        return "IDE";
    case eDatKeyword::ColFile:
        return "COLFILE";
    case eDatKeyword::MapZone:
        return "MAPZONE";
    case eDatKeyword::Ipl:
        return "IPL";
    case eDatKeyword::TexDiction:
        return "TEXDICTION";
    case eDatKeyword::ModelFile:
        return "MODELFILE";
    case eDatKeyword::Splash:
        return "SPLASH";
    case eDatKeyword::HierFile:
        return "HIERFILE";
    case eDatKeyword::Exit:
        return "EXIT";
    case eDatKeyword::Unknown:
    default:
        return "?";
    }
}

struct SaIdeFlagInfo
{
    std::uint32_t mask;
    const char* name;
    const char* descriptionRu;
};

void DrawSaIdeFlagsTooltip(int flags)
{
    static constexpr std::array<SaIdeFlagInfo, 15> kFlagInfo = {{
        {0x00000001u, "IS_ROAD", "участвует как дорожная поверхность (логика дорог/трафика)"},
        {0x00000004u, "DRAW_LAST", "рисуется позже непрозрачной геометрии (для корректной прозрачности)"},
        {0x00000008u, "ADDITIVE", "аддитивный блендинг: свечение, окна, световые элементы"},
        {0x00000020u, "ANIM_WORKS", "объект использует анимационное поведение секции ANIM"},
        {0x00000040u, "NO_ZBUFFER_WRITE", "не записывает глубину (может просвечивать через другие объекты)"},
        {0x00000080u, "DONT_RECEIVE_SHADOWS", "на этот объект не накладываются динамические/статические тени"},
        {0x00000200u, "IS_GLASS_TYPE_1", "стекло (тип 1), поддерживает поведение разбиваемого стекла"},
        {0x00000400u, "IS_GLASS_TYPE_2", "стекло (тип 2), альтернативный режим разбиваемого стекла"},
        {0x00000800u, "IS_GARAGE_DOOR", "распознаётся как дверь гаража с соответствующей логикой"},
        {0x00001000u, "IS_DAMAGABLE", "имеет состояния повреждения (damaged mesh / состояние модели)"},
        {0x00002000u, "IS_TREE", "дерево/растительность: применяются ветровые колебания"},
        {0x00004000u, "IS_PALM", "пальма: специальный профиль колебаний от ветра"},
        {0x00008000u, "DOES_NOT_COLLIDE_WITH_FLYER", "без коллизии с воздушным транспортом (самолёты/вертолёты)"},
        {0x00100000u, "IS_TAG", "тег/граффити, объект помечен как 'tag'"},
        {0x00200000u, "DISABLE_BACKFACE_CULLING", "двусторонний рендер: отключён backface culling"},
    }};

    const std::uint32_t raw = static_cast<std::uint32_t>(flags);
    ImGui::Text("Флаги: %d (0x%08X)", flags, raw);
    if (raw == 0u)
    {
        ImGui::TextDisabled("Флаги не установлены.");
        return;
    }

    std::uint32_t knownMask = 0u;
    bool anyKnown = false;
    for (const auto& f : kFlagInfo)
    {
        if ((raw & f.mask) != 0u)
        {
            anyKnown = true;
            knownMask |= f.mask;
            ImGui::BulletText("0x%08X  %s", f.mask, f.name);
            ImGui::TextDisabled("    %s", f.descriptionRu);
        }
    }

    if (!anyKnown)
    {
        ImGui::TextDisabled("Распознанных флагов SA нет.");
    }

    const std::uint32_t unknownMask = raw & ~knownMask;
    if (unknownMask != 0u)
    {
        ImGui::Separator();
        ImGui::Text("Неизвестные биты: 0x%08X", unknownMask);
        for (std::uint32_t bit = 0; bit < 32u; ++bit)
        {
            const std::uint32_t mask = (1u << bit);
            if ((unknownMask & mask) != 0u)
            {
                ImGui::BulletText("BIT %u (0x%08X): неизвестный/кастомный флаг", bit, mask);
            }
        }
    }
}

std::string FormatIdeDrawDistanceValue(float value)
{
    const float rounded = std::round(value);
    if (std::fabs(value - rounded) < 0.0001f)
    {
        return std::to_string(static_cast<int>(rounded));
    }

    char buffer[32] = {};
    std::snprintf(buffer, sizeof(buffer), "%.3f", value);
    std::string text(buffer);
    while (!text.empty() && text.back() == '0')
    {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '.')
    {
        text.pop_back();
    }
    return text;
}

std::string FormatIdeDrawDistanceLine(float dd0, float dd1, float dd2, std::uint8_t count)
{
    std::string result = FormatIdeDrawDistanceValue(dd0);
    if (count >= 2)
    {
        result += ", " + FormatIdeDrawDistanceValue(dd1);
    }
    if (count >= 3)
    {
        result += ", " + FormatIdeDrawDistanceValue(dd2);
    }
    return result;
}

std::string FormatIplFloatValue(double value)
{
    const double rounded = std::round(value);
    if (std::fabs(value - rounded) < 0.0000000005)
    {
        return std::to_string(static_cast<long long>(rounded));
    }

    char buffer[64] = {};
    std::snprintf(buffer, sizeof(buffer), "%.10f", value);
    std::string text(buffer);
    while (!text.empty() && text.back() == '0')
    {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '.')
    {
        text.pop_back();
    }
    return text;
}
}

void DebugUi::ApplyStyle()
{
    if (ImGui::GetCurrentContext() == nullptr)
    {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    io.Fonts->Clear();

    ImFont* uiFont = io.Fonts->AddFontFromFileTTF(
        "C:\\Windows\\Fonts\\segoeui.ttf",
        17.0f,
        nullptr,
        io.Fonts->GetGlyphRangesCyrillic());
    if (uiFont != nullptr)
    {
        io.FontDefault = uiFont;
    }

    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    style.WindowRounding = 16.0f;
    style.FrameRounding = 9.0f;
    style.GrabRounding = 8.0f;
    style.TabRounding = 8.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 0.0f;
    // Нейтральная тёмная тема (без синего), ближе к «чёрной» классической dark.
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.11f, 0.11f, 0.11f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.22f, 0.22f, 0.22f, 0.90f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.34f, 0.34f, 0.34f, 0.95f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.44f, 0.44f, 0.44f, 1.0f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.26f, 0.26f, 0.90f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.36f, 0.36f, 0.36f, 0.95f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.46f, 0.46f, 0.46f, 1.0f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.65f, 0.65f, 0.65f, 1.0f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.14f, 0.14f, 0.92f);
    style.Colors[ImGuiCol_CheckMark] = ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.72f, 0.72f, 0.72f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.18f, 0.18f, 0.95f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.32f, 0.32f, 0.32f, 0.95f);
    style.Colors[ImGuiCol_TabSelected] = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);
    style.Colors[ImGuiCol_TabSelectedOverline] = ImVec4(0.65f, 0.65f, 0.65f, 1.0f);
    style.Colors[ImGuiCol_TabDimmed] = ImVec4(0.14f, 0.14f, 0.14f, 0.92f);
    style.Colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
    style.Colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
    style.Colors[ImGuiCol_Separator] = ImVec4(0.28f, 0.28f, 0.28f, 0.50f);
    style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.42f, 0.42f, 0.42f, 0.78f);
    style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
    style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.35f, 0.35f, 0.35f, 0.35f);
    style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.50f, 0.50f, 0.50f, 0.67f);
    style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.62f, 0.62f, 0.62f, 0.95f);

    ImGui_ImplDX11_CreateDeviceObjects();
}

void DebugUi::SetHoveredCubeInfo(const HoveredCubeInfo* info)
{
    if (info)
    {
        m_hoveredCubeInfo = *info;
        return;
    }
    m_hoveredCubeInfo.reset();
}

void DebugUi::FocusTreeOnIplInstance(std::int32_t objectId, const std::string& modelName, float x, float y, float z)
{
    m_showTreeWindow = true;
    m_treeFocusPending = true;
    m_treeFocusObjectId = objectId;
    m_treeFocusPosX = x;
    m_treeFocusPosY = y;
    m_treeFocusPosZ = z;
    m_treeFocusModelLower = ToLowerAsciiCopy(modelName);
    if (m_treeSearchBuffer[0] == '\0')
    {
        std::snprintf(m_treeSearchBuffer, sizeof(m_treeSearchBuffer), "%d", objectId);
    }
}

void DebugUi::PushFrameTiming(float dtSeconds)
{
    m_lastFrameDt = dtSeconds;
    // GTA San Andreas: 1 игровая минута за 1 реальную секунду (сутки в игре ≈ 24 реальные минуты).
    constexpr float kGtaSaGameMinutesPerRealSecond = 1.0f;
    m_gtaGameMinutes += dtSeconds * kGtaSaGameMinutesPerRealSecond;
    if (m_gtaGameMinutes >= 24.0f * 60.0f)
    {
        m_gtaGameMinutes -= 24.0f * 60.0f;
    }
    const float frameMs = dtSeconds * 1000.0f;
    m_frameTimeHistoryMs[m_frameHistoryHead] = frameMs;
    m_frameHistoryHead = (m_frameHistoryHead + 1) % kFrameHistorySize;
    if (m_frameHistoryCount < kFrameHistorySize)
    {
        ++m_frameHistoryCount;
    }
}

void DebugUi::ResetFovInertiaToCamera(const d11::render::GtaStyleCamera& camera)
{
    m_fovTarget = camera.GetFovDegrees();
    m_fovVelocity = 0.0f;
}

void DebugUi::QueueFovScrollDelta(int wheelDelta, d11::render::GtaStyleCamera& camera)
{
    if (!m_fovSettings.enabled)
    {
        camera.SetFovDegrees(camera.GetFovDegrees() - static_cast<float>(wheelDelta) * 0.02f);
        return;
    }

    const float steps = static_cast<float>(wheelDelta) / static_cast<float>(WHEEL_DELTA);
    m_fovTarget -= steps * m_fovSettings.step;
    if (m_fovTarget < 1.0f)
    {
        m_fovTarget = 1.0f;
    }
    if (m_fovTarget > 160.0f)
    {
        m_fovTarget = 160.0f;
    }
    m_fovVelocity = 0.0f;
}

void DebugUi::UpdateFovInertia(d11::render::GtaStyleCamera& camera)
{
    if (!m_fovSettings.enabled)
    {
        return;
    }

    const float currentFov = camera.GetFovDegrees();
    const float fovDifference = m_fovTarget - currentFov;
    m_fovVelocity += fovDifference * m_fovSettings.spring * kInertiaFixedTimeStep;
    m_fovVelocity *= m_fovSettings.damping;
    camera.SetFovDegrees(currentFov + m_fovVelocity * kInertiaFixedTimeStep);
}

void DebugUi::ResetCameraSettingsToDefaults(d11::render::GtaStyleCamera& camera)
{
    m_cameraSettings = CameraControlSettings {};
    m_fovSettings = FovScrollSettings {};
    ResetFovInertiaToCamera(camera);
}

void DebugUi::DrawTopBar(
    float dpiScale,
    float frameMs,
    float fps,
    float topBarHeight,
    float topButtonHeight,
    float topBarPaddingY
)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float padX = 12.0f;
    const float barWidth = viewport->Size.x - padX * 2.0f;
    const float panelCornerRounding = kPanelBaseRounding / dpiScale;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + padX, viewport->Pos.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(barWidth, topBarHeight), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.62f);

    constexpr ImGuiWindowFlags kTopBarFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f / dpiScale);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, topBarPaddingY));
    ImGui::Begin("##TopOverlayBar", nullptr, kTopBarFlags);
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 minPos = ImGui::GetWindowPos();
        const ImVec2 maxPos(minPos.x + ImGui::GetWindowWidth(), minPos.y + ImGui::GetWindowHeight());
        const ImU32 barColor = ImGui::GetColorU32(ImVec4(0.07f, 0.07f, 0.07f, 0.62f));
        drawList->AddRectFilled(minPos, maxPos, barColor, panelCornerRounding, ImDrawFlags_RoundCornersBottom);
    }
    ImGui::SetWindowFontScale(kUiFontScale);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 0.0f));

    const auto openPanel = [this](UiPanel panel)
    {
        const bool isTree = (panel == UiPanel::Tree);
        const bool isCamera = (panel == UiPanel::Camera);
        const bool isPerformance = (panel == UiPanel::Performance);
        const bool isControls = (panel == UiPanel::Controls);
        const bool isScene = (panel == UiPanel::Scene);

        if ((isTree && m_showTreeWindow) || (isCamera && m_showCameraWindow) ||
            (isPerformance && m_showPerformanceWindow) || (isControls && m_showControlsWindow) ||
            (isScene && m_showSceneWindow))
        {
            m_showTreeWindow = false;
            m_showCameraWindow = false;
            m_showPerformanceWindow = false;
            m_showControlsWindow = false;
            m_showSceneWindow = false;
            return;
        }

        m_showTreeWindow = isTree;
        m_showCameraWindow = isCamera;
        m_showPerformanceWindow = isPerformance;
        m_showControlsWindow = isControls;
        m_showSceneWindow = isScene;
    };

    const auto tabButton = [this, &openPanel, topButtonHeight](const char* label, UiPanel panel)
    {
        const bool active =
            (panel == UiPanel::Tree && m_showTreeWindow) ||
            (panel == UiPanel::Camera && m_showCameraWindow) ||
            (panel == UiPanel::Performance && m_showPerformanceWindow) ||
            (panel == UiPanel::Controls && m_showControlsWindow) ||
            (panel == UiPanel::Scene && m_showSceneWindow);
        if (active)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.38f, 0.38f, 0.38f, 0.95f));
        }
        const bool clicked = ImGui::Button(label, ImVec2(0.0f, topButtonHeight));
        if (active)
        {
            ImGui::PopStyleColor();
        }
        if (clicked)
        {
            openPanel(panel);
        }
    };

    tabButton("Древо", UiPanel::Tree);
    ImGui::SameLine();
    tabButton("Камера", UiPanel::Camera);
    ImGui::SameLine();
    tabButton("Управление", UiPanel::Controls);
    ImGui::SameLine();
    tabButton("Производительность", UiPanel::Performance);
    ImGui::SameLine();
    tabButton("Сцена", UiPanel::Scene);

    (void)fps;
    (void)frameMs;
    const int totalMin = (static_cast<int>(m_gtaGameMinutes) % (24 * 60) + (24 * 60)) % (24 * 60);
    const int hh = totalMin / 60;
    const int mm = totalMin % 60;
    char timeStr[16];
    std::snprintf(timeStr, sizeof(timeStr), "%02d:%02d", hh, mm);
    const ImVec2 btnLabelSize = ImGui::CalcTextSize("Время");
    const ImVec2 timeSize = ImGui::CalcTextSize(timeStr);
    const float spacing = 8.0f;
    const float btnW = btnLabelSize.x + ImGui::GetStyle().FramePadding.x * 2.0f;
    const float totalW = btnW + spacing + timeSize.x;
    const float lineEndX = ImGui::GetWindowContentRegionMax().x;
    const float rightAlignedOffset = lineEndX - totalW - 6.0f;
    ImGui::SameLine((rightAlignedOffset > 0.0f) ? rightAlignedOffset : 0.0f);
    ImGui::Button("Время", ImVec2(0.0f, topButtonHeight));
    ImGui::SameLine(0.0f, spacing);
    ImGui::TextUnformatted(timeStr);

    ImGui::PopStyleVar();
    ImGui::End();
    ImGui::PopStyleVar(4);
}

void DebugUi::DrawTreeWindow(
    const ImVec2& panelPos,
    float panelWidth,
    float panelCornerRounding,
    float fontScale
)
{
    constexpr float kBottomMargin = 12.0f;
    constexpr float kMinPanelHeight = 180.0f;
    constexpr ImGuiWindowFlags kPanelWindowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    if (!m_showTreeWindow)
    {
        return;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    float panelHeight = (viewport->Pos.y + viewport->Size.y) - panelPos.y - kBottomMargin;
    if (panelHeight < kMinPanelHeight)
    {
        panelHeight = kMinPanelHeight;
    }

    ImGui::SetNextWindowPos(panelPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, panelCornerRounding);
    if (ImGui::Begin("Древо", &m_showTreeWindow, kPanelWindowFlags))
    {
        ImGui::SetWindowFontScale(fontScale);
        ImGui::TextWrapped(
            "Иерархия файлов игры.");
        ImGui::TextDisabled(
            "3D: двойной клик / ПКМ по IDE, IPL INST, .dff в IMG. TXD: двойной клик / ПКМ «Список текстур» — из models (включая подпапки) или из записи .txd внутри IMG.");
        ImGui::Spacing();

        if (!m_gameLoader)
        {
            ImGui::TextDisabled("GameLoader не подключён.");
        }
        else
        {
            const GameLoader::LoadState st = m_gameLoader->GetLoadState();
            const auto access = m_gameLoader->AcquireRead();
            const d11::data::tGtaDatParseSummary& sum = access.gtaDatSummary;
            const std::string& err = sum.errorMessage;

            if (st == GameLoader::LoadState::Failed)
            {
                ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.35f, 1.0f), "%s", err.empty() ? "Не удалось загрузить gta.dat." : err.c_str());
                ImGui::End();
                ImGui::PopStyleVar();
                return;
            }

            if (sum.loadOrder.empty())
            {
                ImGui::TextDisabled("Загрузка данных игры...");
                ImGui::End();
                ImGui::PopStyleVar();
                return;
            }

            bool collapseHierarchyRequested = false;
            ImGui::TextDisabled("%s", sum.sourcePath.string().c_str());
            ImGui::SameLine();
            const float collapseSymbolScale = 1.45f;
            const float collapseSymbolSize = ImGui::GetFontSize() * collapseSymbolScale;
            const float collapseBtnW = collapseSymbolSize + ImGui::GetStyle().FramePadding.x * 2.0f + 6.0f;
            const float collapseBtnH = ImGui::GetFrameHeight() + 2.0f;
            const float rightX = ImGui::GetWindowContentRegionMax().x - collapseBtnW;
            ImGui::SetCursorPosX((std::max)(ImGui::GetCursorPosX(), rightX));
            if (ImGui::Button("##collapse_tree", ImVec2(collapseBtnW, collapseBtnH)))
            {
                collapseHierarchyRequested = true;
            }
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                const ImVec2 minPos = ImGui::GetItemRectMin();
                const ImVec2 maxPos = ImGui::GetItemRectMax();
                const ImVec2 textSz = ImGui::CalcTextSize("≡");
                const ImVec2 textPos(
                    minPos.x + (maxPos.x - minPos.x - textSz.x * collapseSymbolScale) * 0.5f,
                    minPos.y + (maxPos.y - minPos.y - collapseSymbolSize) * 0.5f - 1.0f);
                dl->AddText(nullptr, collapseSymbolSize, textPos, ImGui::GetColorU32(ImGuiCol_Text), "≡");
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Свернуть древо");
            }
            ImGui::Separator();
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputTextWithHint(
                "##tree_search",
                "Поиск по дереву: слово или id",
                m_treeSearchBuffer,
                sizeof(m_treeSearchBuffer));
            const std::string searchQuery = m_treeSearchBuffer;
            const std::string searchQueryLower = ToLowerAsciiCopy(searchQuery);
            const bool hasSearchQuery = !searchQueryLower.empty();
            std::int32_t searchIdValue = 0;
            const bool hasSearchId = ParseExactIdQuery(searchQuery, searchIdValue);
            ImGui::Spacing();
            float scrollH = ImGui::GetContentRegionAvail().y;
            if (scrollH < 80.0f)
            {
                scrollH = 80.0f;
            }
            ImGui::BeginChild("##gtaDatTree", ImVec2(0.0f, scrollH), true);
            // Кол-во типов записей в gta.dat (уникальные ключевые слова в loadOrder).
            std::array<bool, 64> seenKw {};
            int kwTypes = 0;
            for (const auto& e : sum.loadOrder)
            {
                const int idx = static_cast<int>(e.keyword);
                if (idx >= 0 && idx < static_cast<int>(seenKw.size()))
                {
                    if (!seenKw[static_cast<std::size_t>(idx)])
                    {
                        seenKw[static_cast<std::size_t>(idx)] = true;
                        ++kwTypes;
                    }
                }
            }

            // (Re)build cached groupings when loadOrder changes.
            // This must run even if gta.dat node is collapsed (sibling nodes use the same cache).
            if (m_cachedLoadOrderSize != sum.loadOrder.size())
            {
                using d11::data::eDatKeyword;
                m_cachedLoadOrderSize = sum.loadOrder.size();
                m_cachedIdeEntries.clear();
                m_cachedIplEntries.clear();
                m_cachedImgEntries.clear();
                m_cachedOtherEntries.clear();
                m_cachedImgCountByAbsPath.clear();
                m_cachedImgResByAbsPath.clear();
                m_cachedImgResultsSize = 0;
                m_gtaDatImgInitNextIndex = 0;
                m_cachedIdeEntries.reserve(sum.loadOrder.size());
                m_cachedIplEntries.reserve(sum.loadOrder.size());
                m_cachedImgEntries.reserve(sum.loadOrder.size());
                m_cachedOtherEntries.reserve(sum.loadOrder.size());

                for (const auto& ce : sum.loadOrder)
                {
                    switch (ce.keyword)
                    {
                    case eDatKeyword::Ide:
                        m_cachedIdeEntries.push_back(&ce);
                        break;
                    case eDatKeyword::Ipl:
                        m_cachedIplEntries.push_back(&ce);
                        break;
                    case eDatKeyword::Img:
                    case eDatKeyword::ImgList:
                    case eDatKeyword::CdImage:
                        m_cachedImgEntries.push_back(&ce);
                        break;
                    default:
                        m_cachedOtherEntries.push_back(&ce);
                        break;
                    }
                }
            }

            if (collapseHierarchyRequested)
            {
                ImGui::SetNextItemOpen(false, ImGuiCond_Always);
            }
            if (m_treeFocusPending)
            {
                ImGui::SetNextItemOpen(true, ImGuiCond_Always);
            }
            if (ImGui::TreeNodeEx("gta.dat##root", ImGuiTreeNodeFlags_DefaultOpen, "gta.dat (%d)", kwTypes))
            {
                using d11::data::eDatKeyword;
                using d11::data::tGtaDatCatalogEntry;

                const std::size_t ideN = std::min(access.idePaths.size(), access.ideResults.size());
                if (m_cachedIdeResultsSize != ideN)
                {
                    m_cachedIdeResultsSize = ideN;
                    m_cachedIdeCountByAbsPath.clear();
                    m_cachedIdeResByAbsPath.clear();
                    m_cachedIdeCountByAbsPath.reserve(ideN);
                    m_cachedIdeResByAbsPath.reserve(ideN);
                    for (std::size_t i = 0; i < ideN; ++i)
                    {
                        const std::string key = MakePathKey(access.idePaths[i]);
                        m_cachedIdeCountByAbsPath.emplace(key, access.ideResults[i].entries.size());
                        m_cachedIdeResByAbsPath.emplace(key, &access.ideResults[i]);
                    }
                }

                const std::size_t iplN = std::min(access.iplPaths.size(), access.iplResults.size());
                if (m_cachedIplResultsSize != iplN)
                {
                    m_cachedIplResultsSize = iplN;
                    m_cachedIplCountByAbsPath.clear();
                    m_cachedIplResByAbsPath.clear();
                    m_cachedIplCountByAbsPath.reserve(iplN);
                    m_cachedIplResByAbsPath.reserve(iplN);
                    for (std::size_t i = 0; i < iplN; ++i)
                    {
                        const std::string key = MakePathKey(access.iplPaths[i]);
                        const std::size_t totalIplEntries =
                            access.iplResults[i].data.inst.size() +
                            access.iplResults[i].data.cars.size() +
                            access.iplResults[i].data.zones.size();
                        m_cachedIplCountByAbsPath.emplace(key, totalIplEntries);
                        m_cachedIplResByAbsPath.emplace(key, &access.iplResults[i]);
                    }
                }

                if (m_cachedImgResultsSize != access.imgByAbsPath.size())
                {
                    m_cachedImgResultsSize = access.imgByAbsPath.size();
                    m_cachedImgCountByAbsPath.clear();
                    m_cachedImgResByAbsPath.clear();
                    m_cachedImgCountByAbsPath.reserve(access.imgByAbsPath.size());
                    m_cachedImgResByAbsPath.reserve(access.imgByAbsPath.size());
                    for (const auto& kv : access.imgByAbsPath)
                    {
                        if (!kv.second)
                        {
                            continue;
                        }
                        const std::size_t count = kv.second->errorMessage.empty() ? kv.second->entries.size() : 0u;
                        m_cachedImgCountByAbsPath.emplace(kv.first, count);
                        m_cachedImgResByAbsPath.emplace(kv.first, kv.second);
                    }
                }

                auto resolveAbsKey = [&](const tGtaDatCatalogEntry& e) -> std::string
                {
                    const fs::path rel = fs::path(e.pathOrText);
                    const fs::path abs = (rel.is_absolute() ? rel : (m_gameLoader ? (m_gameLoader->GetGameRootPath() / rel) : rel));
                    return MakePathKey(abs);
                };

                auto resolveAbsPathString = [&](const tGtaDatCatalogEntry& e) -> std::string
                {
                    const fs::path rel = fs::path(e.pathOrText);
                    const fs::path abs = (rel.is_absolute() ? rel : (m_gameLoader ? (m_gameLoader->GetGameRootPath() / rel) : rel));
                    return abs.lexically_normal().string();
                };

                auto entryDisplayName = [](const tGtaDatCatalogEntry& e) -> std::string
                {
                    if (e.pathOrText.empty())
                    {
                        return {};
                    }
                    const fs::path path(e.pathOrText);
                    const std::string filename = path.filename().string();
                    return filename.empty() ? e.pathOrText : filename;
                };

                auto drawEntry = [](const tGtaDatCatalogEntry& e)
                {
                    const char* tag = DatKeywordShortName(e.keyword);
                    if (e.keyword == eDatKeyword::Exit)
                    {
                        ImGui::BulletText("%s", tag);
                        if (ImGui::IsItemHovered())
                        {
                            ImDrawList* dl = ImGui::GetWindowDrawList();
                            ImVec2 a = ImGui::GetItemRectMin();
                            ImVec2 b = ImGui::GetItemRectMax();
                            a.x -= 2.0f; a.y -= 1.0f;
                            b.x += 2.0f; b.y += 1.0f;
                            dl->AddRect(a, b, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.16f)), 6.0f, 0, 1.0f);
                        }
                        return;
                    }
                    if (e.skippedAsZoneIpl)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                        ImGui::BulletText("%s [zone]: %s", tag, e.pathOrText.c_str());
                        ImGui::PopStyleColor();
                        if (ImGui::IsItemHovered())
                        {
                            ImDrawList* dl = ImGui::GetWindowDrawList();
                            ImVec2 a = ImGui::GetItemRectMin();
                            ImVec2 b = ImGui::GetItemRectMax();
                            a.x -= 2.0f; a.y -= 1.0f;
                            b.x += 2.0f; b.y += 1.0f;
                            dl->AddRect(a, b, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.16f)), 6.0f, 0, 1.0f);
                        }
                        return;
                    }
                    if (e.keyword == eDatKeyword::ColFile)
                    {
                        ImGui::BulletText("%s [%d]: %s", tag, static_cast<int>(e.colFileZoneId), e.pathOrText.c_str());
                        if (ImGui::IsItemHovered())
                        {
                            ImDrawList* dl = ImGui::GetWindowDrawList();
                            ImVec2 a = ImGui::GetItemRectMin();
                            ImVec2 b = ImGui::GetItemRectMax();
                            a.x -= 2.0f; a.y -= 1.0f;
                            b.x += 2.0f; b.y += 1.0f;
                            dl->AddRect(a, b, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.16f)), 6.0f, 0, 1.0f);
                        }
                        return;
                    }
                    ImGui::BulletText("%s: %s", tag, e.pathOrText.c_str());
                    if (ImGui::IsItemHovered())
                    {
                        ImDrawList* dl = ImGui::GetWindowDrawList();
                        ImVec2 a = ImGui::GetItemRectMin();
                        ImVec2 b = ImGui::GetItemRectMax();
                        a.x -= 2.0f; a.y -= 1.0f;
                        b.x += 2.0f; b.y += 1.0f;
                        dl->AddRect(a, b, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.16f)), 6.0f, 0, 1.0f);
                    }
                };

                auto section = [&](const char* title,
                                   const char* idSuffix,
                                   const std::vector<const tGtaDatCatalogEntry*>& items,
                                   bool defaultOpen,
                                   std::size_t nestedCount)
                {
                    if (items.empty())
                    {
                        return;
                    }
                    const std::string header = std::string(title) + "##" + idSuffix;
                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
                    if (defaultOpen)
                    {
                        flags |= ImGuiTreeNodeFlags_DefaultOpen;
                    }
                    if (m_treeFocusPending && std::strcmp(title, "IPL") == 0)
                    {
                        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
                    }
                    const bool open = ImGui::TreeNodeEx(header.c_str(), flags, "%s (%d) (%d)", title, static_cast<int>(items.size()), static_cast<int>(nestedCount));

                    // IMG parsing runs asynchronously in GameLoader; UI only displays results.

                    if (open)
                    {
                        ImGui::Separator();
                        // IMPORTANT: do NOT use ImGuiListClipper here.
                        // Items can expand (TreeNode) and have variable height, which breaks the clipper and causes jumps.
                        for (const auto* e : items)
                        {
                            // IDE/IPL: дописываем количество вложенных элементов по результатам парсинга.
                            if ((e->keyword == eDatKeyword::Ide || e->keyword == eDatKeyword::Ipl) && !e->pathOrText.empty() && m_gameLoader)
                            {
                                const std::string key = resolveAbsKey(*e);
                                if (e->keyword == eDatKeyword::Ide)
                                {
                                    auto it = m_cachedIdeCountByAbsPath.find(key);
                                    if (it != m_cachedIdeCountByAbsPath.end())
                                    {
                                        const int count = static_cast<int>(it->second);
                                        const std::string absPathText = resolveAbsPathString(*e);
                                        ImGui::PushID(key.c_str());
                                        const bool nodeOpen = ImGui::TreeNodeEx("ide##file", ImGuiTreeNodeFlags_SpanAvailWidth, "IDE: %s (%d)", e->pathOrText.c_str(), count);
                                        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight))
                                        {
                                            const std::string copyName = entryDisplayName(*e);
                                            if (ImGui::MenuItem("Скопировать название"))
                                            {
                                                ImGui::SetClipboardText(copyName.c_str());
                                            }
                                            if (ImGui::MenuItem("Скопировать системный путь к файлу"))
                                            {
                                                ImGui::SetClipboardText(absPathText.c_str());
                                            }
                                            ImGui::EndPopup();
                                        }
                                        if (nodeOpen)
                                        {
                                            auto rIt = m_cachedIdeResByAbsPath.find(key);
                                            const d11::data::tIdeParseResult* res = (rIt != m_cachedIdeResByAbsPath.end()) ? rIt->second : nullptr;
                                            if (!res)
                                            {
                                                ImGui::TextDisabled("Нет результата парсинга IDE.");
                                            }
                                            else
                                            {
                                                const auto ideEntryMatchesSearchQuery = [&](const d11::data::tIdeEntry& ent) -> bool
                                                {
                                                    if (!hasSearchQuery)
                                                    {
                                                        return true;
                                                    }
                                                    const char* lab = "(unknown)";
                                                    std::visit([&](auto&& v)
                                                    {
                                                        using T = std::decay_t<decltype(v)>;
                                                        if constexpr (std::is_same_v<T, d11::data::tIdeBaseObjectEntry> ||
                                                                      std::is_same_v<T, d11::data::tIdeTimedObjectEntry> ||
                                                                      std::is_same_v<T, d11::data::tIdeAnimatedObjectEntry>)
                                                        {
                                                            lab = v.modelName.empty() ? "(no modelName)" : v.modelName.c_str();
                                                        }
                                                        else if constexpr (std::is_same_v<T, d11::data::tIdeTextureParentEntry>)
                                                        {
                                                            lab = v.textureDictionary.empty() ? "(no txd)" : v.textureDictionary.c_str();
                                                        }
                                                        else if constexpr (std::is_same_v<T, d11::data::tIdeGenericSectionEntry>)
                                                        {
                                                            lab = "(section)";
                                                        }
                                                    }, ent);
                                                    bool ideMatchesSearch = ContainsCaseInsensitive(lab ? std::string(lab) : std::string(), searchQueryLower);
                                                    std::visit([&](auto&& v)
                                                    {
                                                        using T = std::decay_t<decltype(v)>;
                                                        if constexpr (std::is_same_v<T, d11::data::tIdeBaseObjectEntry> ||
                                                                      std::is_same_v<T, d11::data::tIdeTimedObjectEntry> ||
                                                                      std::is_same_v<T, d11::data::tIdeAnimatedObjectEntry>)
                                                        {
                                                            if (ContainsCaseInsensitive(v.modelName, searchQueryLower) ||
                                                                ContainsCaseInsensitive(v.textureDictionary, searchQueryLower) ||
                                                                ContainsCaseInsensitive(std::to_string(v.id), searchQueryLower))
                                                            {
                                                                ideMatchesSearch = true;
                                                            }
                                                            if (hasSearchId && v.id == searchIdValue)
                                                            {
                                                                ideMatchesSearch = true;
                                                            }
                                                        }
                                                        else if constexpr (std::is_same_v<T, d11::data::tIdeTextureParentEntry>)
                                                        {
                                                            if (ContainsCaseInsensitive(v.textureDictionary, searchQueryLower) ||
                                                                ContainsCaseInsensitive(v.parentTextureDictionary, searchQueryLower))
                                                            {
                                                                ideMatchesSearch = true;
                                                            }
                                                        }
                                                        else if constexpr (std::is_same_v<T, d11::data::tIdeGenericSectionEntry>)
                                                        {
                                                            for (const std::string& token : v.tokens)
                                                            {
                                                                if (ContainsCaseInsensitive(token, searchQueryLower))
                                                                {
                                                                    ideMatchesSearch = true;
                                                                    break;
                                                                }
                                                            }
                                                        }
                                                    }, ent);
                                                    return ideMatchesSearch;
                                                };

                                                const int ideTotal = static_cast<int>(res->entries.size());
                                                std::vector<int> ideVisibleRows;
                                                if (hasSearchQuery)
                                                {
                                                    ideVisibleRows.reserve(static_cast<std::size_t>(ideTotal));
                                                    for (int ii = 0; ii < ideTotal; ++ii)
                                                    {
                                                        if (ideEntryMatchesSearchQuery(res->entries[static_cast<std::size_t>(ii)]))
                                                        {
                                                            ideVisibleRows.push_back(ii);
                                                        }
                                                    }
                                                }
                                                const int ideClipCount = hasSearchQuery ? static_cast<int>(ideVisibleRows.size()) : ideTotal;

                                                ImGuiListClipper clipper;
                                                clipper.Begin(ideClipCount);
                                                while (clipper.Step())
                                                {
                                                    for (int r = clipper.DisplayStart; r < clipper.DisplayEnd; ++r)
                                                    {
                                                        const int i = hasSearchQuery ? ideVisibleRows[static_cast<std::size_t>(r)] : r;
                                                        const d11::data::tIdeEntry& entry = res->entries[static_cast<std::size_t>(i)];

                                                        // Fast path: render label without allocations.
                                                        const char* labelPtr = "(unknown)";
                                                        std::visit([&](auto&& v)
                                                        {
                                                            using T = std::decay_t<decltype(v)>;
                                                            if constexpr (std::is_same_v<T, d11::data::tIdeBaseObjectEntry> ||
                                                                          std::is_same_v<T, d11::data::tIdeTimedObjectEntry> ||
                                                                          std::is_same_v<T, d11::data::tIdeAnimatedObjectEntry>)
                                                            {
                                                                labelPtr = v.modelName.empty() ? "(no modelName)" : v.modelName.c_str();
                                                            }
                                                            else if constexpr (std::is_same_v<T, d11::data::tIdeTextureParentEntry>)
                                                            {
                                                                labelPtr = v.textureDictionary.empty() ? "(no txd)" : v.textureDictionary.c_str();
                                                            }
                                                            else if constexpr (std::is_same_v<T, d11::data::tIdeGenericSectionEntry>)
                                                            {
                                                                labelPtr = "(section)";
                                                            }
                                                        }, entry);

                                                        ImGui::PushID(i);
                                                        ImGui::Bullet();
                                                        ImGui::SameLine();
                                                        if (ImGui::Selectable(
                                                                labelPtr,
                                                                false,
                                                                ImGuiSelectableFlags_AllowDoubleClick))
                                                        {
                                                            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                                                            {
                                                                std::visit(
                                                                    [&](auto&& v)
                                                                    {
                                                                        using T = std::decay_t<decltype(v)>;
                                                                        if constexpr (std::is_same_v<T, d11::data::tIdeBaseObjectEntry> ||
                                                                                      std::is_same_v<T, d11::data::tIdeTimedObjectEntry> ||
                                                                                      std::is_same_v<T, d11::data::tIdeAnimatedObjectEntry>)
                                                                        {
                                                                            if (!v.modelName.empty())
                                                                            {
                                                                                QueueModelPreviewFromIde(v.modelName);
                                                                            }
                                                                        }
                                                                    },
                                                                    entry);
                                                            }
                                                        }
                                                        ImGui::PopID();

                                                        const bool hovered = ImGui::IsItemHovered();
                                                        if (hovered)
                                                        {
                                                            ImDrawList* dl = ImGui::GetWindowDrawList();
                                                            ImVec2 a = ImGui::GetItemRectMin();
                                                            ImVec2 b = ImGui::GetItemRectMax();

                                                            // Extend highlight to full content width.
                                                            const ImVec2 winPos = ImGui::GetWindowPos();
                                                            const float contentMaxX = winPos.x + ImGui::GetWindowContentRegionMax().x;
                                                            b.x = contentMaxX;

                                                            a.x -= 6.0f;
                                                            a.y -= 1.0f;
                                                            b.x += 6.0f;
                                                            b.y += 1.0f;
                                                            dl->AddRect(a, b, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.16f)), 6.0f, 0, 1.0f);
                                                        }
                                                        if (hovered)
                                                        {
                                                            // Slow path: unpack tooltip data only on hover.
                                                            bool hasTooltip = false;
                                                            int tipId = 0;
                                                            const char* tipTxd = nullptr;
                                                            float tipDd0 = 0.0f, tipDd1 = 0.0f, tipDd2 = 0.0f;
                                                            std::uint8_t tipDdCount = 0;
                                                            int tipFlags = 0;
                                                            bool tipHasTime = false;
                                                            int tipTimeOn = 0, tipTimeOff = 0;
                                                            bool tipHasAnim = false;
                                                            const char* tipAnim = nullptr;
                                                            bool tipIsTxdp = false;
                                                            const char* tipTxdpChild = nullptr;
                                                            const char* tipTxdpParent = nullptr;
                                                            bool tipIsGeneric = false;
                                                            std::size_t tipGenericTokens = 0;

                                                            std::visit([&](auto&& v)
                                                            {
                                                                using T = std::decay_t<decltype(v)>;
                                                                if constexpr (std::is_same_v<T, d11::data::tIdeBaseObjectEntry> ||
                                                                              std::is_same_v<T, d11::data::tIdeTimedObjectEntry> ||
                                                                              std::is_same_v<T, d11::data::tIdeAnimatedObjectEntry>)
                                                                {
                                                                    hasTooltip = true;
                                                                    tipId = v.id;
                                                                    tipTxd = v.textureDictionary.empty() ? nullptr : v.textureDictionary.c_str();
                                                                    tipDd0 = v.drawDistance[0];
                                                                    tipDd1 = v.drawDistance[1];
                                                                    tipDd2 = v.drawDistance[2];
                                                                    tipDdCount = v.drawDistanceCount;
                                                                    tipFlags = v.flags;
                                                                    if constexpr (std::is_same_v<T, d11::data::tIdeTimedObjectEntry>)
                                                                    {
                                                                        tipHasTime = true;
                                                                        tipTimeOn = v.timeOnHour;
                                                                        tipTimeOff = v.timeOffHour;
                                                                    }
                                                                    if constexpr (std::is_same_v<T, d11::data::tIdeAnimatedObjectEntry>)
                                                                    {
                                                                        tipHasAnim = true;
                                                                        tipAnim = v.animationFile.empty() ? nullptr : v.animationFile.c_str();
                                                                    }
                                                                }
                                                                else if constexpr (std::is_same_v<T, d11::data::tIdeTextureParentEntry>)
                                                                {
                                                                    hasTooltip = true;
                                                                    tipIsTxdp = true;
                                                                    tipTxdpChild = v.textureDictionary.empty() ? nullptr : v.textureDictionary.c_str();
                                                                    tipTxdpParent = v.parentTextureDictionary.empty() ? nullptr : v.parentTextureDictionary.c_str();
                                                                }
                                                                else if constexpr (std::is_same_v<T, d11::data::tIdeGenericSectionEntry>)
                                                                {
                                                                    hasTooltip = true;
                                                                    tipIsGeneric = true;
                                                                    tipGenericTokens = v.tokens.size();
                                                                }
                                                            }, entry);

                                                            if (!hasTooltip)
                                                            {
                                                                continue;
                                                            }

                                                            ImGui::BeginTooltip();
                                                            if (tipIsTxdp)
                                                            {
                                                                ImGui::TextUnformatted("TXDP (связь словарей текстур):");
                                                                ImGui::Text("Дочерний TXD: %s", tipTxdpChild ? tipTxdpChild : "(пусто)");
                                                                ImGui::Text("Родительский TXD: %s", tipTxdpParent ? tipTxdpParent : "(пусто)");
                                                            }
                                                            else if (tipIsGeneric)
                                                            {
                                                                ImGui::TextUnformatted("Запись IDE (секция без подробного парсинга)");
                                                                ImGui::Text("Кол-во токенов: %d", static_cast<int>(tipGenericTokens));
                                                            }
                                                            else
                                                            {
                                                                ImGui::Text("ID (идентификатор): %d", tipId);
                                                                ImGui::Text("TXD (словарь текстур): %s", tipTxd ? tipTxd : "(пусто)");
                                                                const std::string drawDistText = FormatIdeDrawDistanceLine(tipDd0, tipDd1, tipDd2, tipDdCount);
                                                                ImGui::Text("Дальность прорисовки: %s", drawDistText.c_str());
                                                                DrawSaIdeFlagsTooltip(tipFlags);
                                                                if (tipHasTime)
                                                                {
                                                                    ImGui::Text("Время (TOBJ): %d-%d", tipTimeOn, tipTimeOff);
                                                                }
                                                                if (tipHasAnim)
                                                                {
                                                                    ImGui::Text("Анимация (ANIM): %s", tipAnim ? tipAnim : "(пусто)");
                                                                }
                                                            }
                                                            ImGui::EndTooltip();
                                                        }
                                                    }
                                                }
                                            }
                                            ImGui::TreePop();
                                        }
                                        ImGui::PopID();
                                        continue;
                                    }
                                }
                                if (e->keyword == eDatKeyword::Ipl && !e->skippedAsZoneIpl)
                                {
                                    auto it = m_cachedIplCountByAbsPath.find(key);
                                    if (it != m_cachedIplCountByAbsPath.end())
                                    {
                                        const int count = static_cast<int>(it->second);
                                        const std::string absPathText = resolveAbsPathString(*e);
                                        const auto instMatchesFocusRequest = [&](const d11::data::tIplInstEntry& inst) -> bool
                                        {
                                            if (!m_treeFocusPending || inst.objectId != m_treeFocusObjectId)
                                            {
                                                return false;
                                            }
                                            if (!m_treeFocusModelLower.empty() && !inst.modelName.empty())
                                            {
                                                if (ToLowerAsciiCopy(inst.modelName) != m_treeFocusModelLower)
                                                {
                                                    return false;
                                                }
                                            }
                                            if (!IsNearFloat(static_cast<float>(inst.position.x), m_treeFocusPosX) ||
                                                !IsNearFloat(static_cast<float>(inst.position.y), m_treeFocusPosY) ||
                                                !IsNearFloat(static_cast<float>(inst.position.z), m_treeFocusPosZ))
                                            {
                                                return false;
                                            }
                                            return true;
                                        };
                                        const auto instMatchesSearchQuery = [&](const d11::data::tIplInstEntry& inst) -> bool
                                        {
                                            if (!hasSearchQuery)
                                            {
                                                return true;
                                            }
                                            if (ContainsCaseInsensitive(inst.modelName, searchQueryLower))
                                            {
                                                return true;
                                            }
                                            if (ContainsCaseInsensitive(std::to_string(inst.objectId), searchQueryLower))
                                            {
                                                return true;
                                            }
                                            return hasSearchId && (inst.objectId == searchIdValue);
                                        };
                                        bool openThisIplNode = false;
                                        if (m_treeFocusPending)
                                        {
                                            auto rItFocus = m_cachedIplResByAbsPath.find(key);
                                            const d11::data::tIplParseResult* focusRes =
                                                (rItFocus != m_cachedIplResByAbsPath.end()) ? rItFocus->second : nullptr;
                                            if (focusRes)
                                            {
                                                for (const auto& inst : focusRes->data.inst)
                                                {
                                                    if (instMatchesFocusRequest(inst))
                                                    {
                                                        openThisIplNode = true;
                                                        break;
                                                    }
                                                }
                                            }
                                        }
                                        if (openThisIplNode)
                                        {
                                            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
                                        }
                                        ImGui::PushID(key.c_str());
                                        const bool nodeOpen = ImGui::TreeNodeEx("ipl##file", ImGuiTreeNodeFlags_SpanAvailWidth, "IPL: %s (%d)", e->pathOrText.c_str(), count);
                                        if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight))
                                        {
                                            const std::string copyName = entryDisplayName(*e);
                                            if (ImGui::MenuItem("Скопировать название"))
                                            {
                                                ImGui::SetClipboardText(copyName.c_str());
                                            }
                                            if (ImGui::MenuItem("Скопировать системный путь к файлу"))
                                            {
                                                ImGui::SetClipboardText(absPathText.c_str());
                                            }
                                            ImGui::EndPopup();
                                        }
                                        if (nodeOpen)
                                        {
                                            auto rIt = m_cachedIplResByAbsPath.find(key);
                                            const d11::data::tIplParseResult* res = (rIt != m_cachedIplResByAbsPath.end()) ? rIt->second : nullptr;
                                            if (!res)
                                            {
                                                ImGui::TextDisabled("Нет результата парсинга IPL.");
                                            }
                                            else
                                            {
                                                const bool hasInst = !res->data.inst.empty();
                                                const bool hasCars = !res->data.cars.empty();
                                                const bool hasZones = !res->data.zones.empty();

                                                if (!hasInst && !hasCars && !hasZones)
                                                {
                                                    ImGui::TextDisabled("Нет распознанных поддерживаемых секций (INST/CARS/ZONE).");
                                                }

                                                const int instSize = static_cast<int>(res->data.inst.size());
                                                int instCountLabel = instSize;
                                                if (hasSearchQuery && hasInst)
                                                {
                                                    int matchCount = 0;
                                                    for (int ii = 0; ii < instSize; ++ii)
                                                    {
                                                        if (instMatchesSearchQuery(res->data.inst[static_cast<std::size_t>(ii)]))
                                                        {
                                                            ++matchCount;
                                                        }
                                                    }
                                                    instCountLabel = matchCount;
                                                }
                                                if (m_treeFocusPending)
                                                {
                                                    ImGui::SetNextItemOpen(true, ImGuiCond_Always);
                                                }
                                                if (hasInst && ImGui::TreeNodeEx("INST##ipl_inst_section", ImGuiTreeNodeFlags_SpanAvailWidth, "INST (%d)", instCountLabel))
                                                {
                                                    std::vector<int> instVisibleIndices;
                                                    if (hasSearchQuery)
                                                    {
                                                        instVisibleIndices.reserve(static_cast<std::size_t>(instCountLabel));
                                                        for (int ii = 0; ii < instSize; ++ii)
                                                        {
                                                            if (instMatchesSearchQuery(res->data.inst[static_cast<std::size_t>(ii)]))
                                                            {
                                                                instVisibleIndices.push_back(ii);
                                                            }
                                                        }
                                                    }
                                                    const int instClipCount = hasSearchQuery ? static_cast<int>(instVisibleIndices.size()) : instSize;
                                                    ImGuiListClipper instClip;
                                                    instClip.Begin(instClipCount);
                                                    if (m_treeFocusPending)
                                                    {
                                                        if (hasSearchQuery)
                                                        {
                                                            for (int row = 0; row < static_cast<int>(instVisibleIndices.size()); ++row)
                                                            {
                                                                const int idx = instVisibleIndices[static_cast<std::size_t>(row)];
                                                                if (instMatchesFocusRequest(res->data.inst[static_cast<std::size_t>(idx)]))
                                                                {
                                                                    instClip.IncludeItemByIndex(row);
                                                                    break;
                                                                }
                                                            }
                                                        }
                                                        else
                                                        {
                                                            for (int ii = 0; ii < instSize; ++ii)
                                                            {
                                                                if (instMatchesFocusRequest(res->data.inst[static_cast<std::size_t>(ii)]))
                                                                {
                                                                    instClip.IncludeItemByIndex(ii);
                                                                    break;
                                                                }
                                                            }
                                                        }
                                                    }
                                                    while (instClip.Step())
                                                    {
                                                        for (int row = instClip.DisplayStart; row < instClip.DisplayEnd; ++row)
                                                        {
                                                            const int i = hasSearchQuery ? instVisibleIndices[static_cast<std::size_t>(row)] : row;
                                                            const auto& inst = res->data.inst[static_cast<std::size_t>(i)];
                                                            const bool isFocusedInst = instMatchesFocusRequest(inst);
                                                            const char* labelPtr = nullptr;
                                                            char idBuf[32] = {};
                                                            if (!inst.modelName.empty())
                                                            {
                                                                labelPtr = inst.modelName.c_str();
                                                            }
                                                            else
                                                            {
                                                                std::snprintf(idBuf, sizeof(idBuf), "(id %d)", inst.objectId);
                                                                labelPtr = idBuf;
                                                            }

                                                            ImGui::PushID(i);
                                                            const auto tryOpenIplPreview = [&]()
                                                            {
                                                                if (!inst.modelName.empty())
                                                                {
                                                                    QueueModelPreviewFromIde(inst.modelName);
                                                                    return;
                                                                }
                                                                const std::string resolved = TryResolveIdeModelNameFromId(access, inst.objectId);
                                                                if (!resolved.empty())
                                                                {
                                                                    QueueModelPreviewFromIde(resolved);
                                                                }
                                                            };

                                                            if (ImGui::Selectable(
                                                                    labelPtr,
                                                                    false,
                                                                    ImGuiSelectableFlags_AllowDoubleClick))
                                                            {
                                                                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                                                                {
                                                                    tryOpenIplPreview();
                                                                }
                                                            }
                                                            if (ImGui::BeginPopupContextItem("ipl_inst_ctx"))
                                                            {
                                                                if (ImGui::MenuItem("3D preview"))
                                                                {
                                                                    tryOpenIplPreview();
                                                                }
                                                                ImGui::EndPopup();
                                                            }

                                                            ImDrawList* dl = ImGui::GetWindowDrawList();
                                                            ImVec2 a = ImGui::GetItemRectMin();
                                                            ImVec2 b = ImGui::GetItemRectMax();
                                                            const ImVec2 winPos = ImGui::GetWindowPos();
                                                            const float contentMaxX = winPos.x + ImGui::GetWindowContentRegionMax().x;
                                                            b.x = contentMaxX;
                                                            a.x -= 6.0f; a.y -= 1.0f;
                                                            b.x += 6.0f; b.y += 1.0f;
                                                            if (isFocusedInst)
                                                            {
                                                                dl->AddRectFilled(a, b, ImGui::GetColorU32(ImVec4(0.92f, 0.72f, 0.15f, 0.22f)), 6.0f);
                                                                dl->AddRect(a, b, ImGui::GetColorU32(ImVec4(1.0f, 0.86f, 0.28f, 0.95f)), 6.0f, 0, 1.3f);
                                                                if (m_treeFocusPending)
                                                                {
                                                                    ImGui::SetScrollHereY(0.45f);
                                                                    m_treeFocusPending = false;
                                                                }
                                                            }

                                                            const bool hovered = ImGui::IsItemHovered();
                                                            if (hovered && !isFocusedInst)
                                                            {
                                                                dl->AddRect(a, b, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.16f)), 6.0f, 0, 1.0f);
                                                            }
                                                            if (hovered || isFocusedInst)
                                                            {
                                                                ImGui::BeginTooltip();
                                                                ImGui::Text("INST #%d", i + 1);

                                                                // Порядок как в исходном текстовом IPL (SA INST):
                                                                // id, modelName, interior, x, y, z, rx, ry, rz, rw, lod
                                                                ImGui::Text("Object ID: %d", inst.objectId);
                                                                ImGui::Text("Name (в строке): %s", inst.modelName.empty() ? "(пусто)" : inst.modelName.c_str());
                                                                ImGui::Text("Interior: %d", inst.interior);
                                                                const std::string px = FormatIplFloatValue(inst.position.x);
                                                                const std::string py = FormatIplFloatValue(inst.position.y);
                                                                const std::string pz = FormatIplFloatValue(inst.position.z);
                                                                const std::string rx = FormatIplFloatValue(inst.rotation.x);
                                                                const std::string ry = FormatIplFloatValue(inst.rotation.y);
                                                                const std::string rz = FormatIplFloatValue(inst.rotation.z);
                                                                const std::string rw = FormatIplFloatValue(inst.rotation.w);
                                                                ImGui::Text("Position X: %s", px.c_str());
                                                                ImGui::Text("Position Y: %s", py.c_str());
                                                                ImGui::Text("Position Z: %s", pz.c_str());
                                                                ImGui::Text("Rotation X: %s", rx.c_str());
                                                                ImGui::Text("Rotation Y: %s", ry.c_str());
                                                                ImGui::Text("Rotation Z: %s", rz.c_str());
                                                                ImGui::Text("Rotation W: %s", rw.c_str());
                                                                ImGui::Text("LOD index: %d", inst.lodIndex);
                                                                ImGui::TextDisabled("Name в IPL часто игнорируется игрой (поле для читаемости/тулов).");
                                                                ImGui::TextDisabled("LOD index: индекс LOD-связи (не ID модели).");
                                                                ImGui::Separator();
                                                                ImGui::TextDisabled("Double-click or RMB: 3D preview");
                                                                ImGui::EndTooltip();
                                                            }
                                                            ImGui::PopID();
                                                        }
                                                    }
                                                    ImGui::TreePop();
                                                }

                                                if (hasCars && ImGui::TreeNodeEx("CARS##ipl_cars_section", ImGuiTreeNodeFlags_SpanAvailWidth, "CARS (%d)", static_cast<int>(res->data.cars.size())))
                                                {
                                                    ImGuiListClipper clipper;
                                                    clipper.Begin(static_cast<int>(res->data.cars.size()));
                                                    while (clipper.Step())
                                                    {
                                                        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                                                        {
                                                            const auto& car = res->data.cars[static_cast<std::size_t>(i)];
                                                            ImGui::BulletText("VehicleId %d", car.vehicleId);
                                                            if (ImGui::IsItemHovered())
                                                            {
                                                                ImGui::BeginTooltip();
                                                                ImGui::Text("VehicleId: %d", car.vehicleId);
                                                                ImGui::Text("Pos: %.3f, %.3f, %.3f", car.position.x, car.position.y, car.position.z);
                                                                ImGui::Text("Angle Z: %.3f", car.angleZ);
                                                                ImGui::Text("Colors: %d, %d", car.primaryColor, car.secondaryColor);
                                                                ImGui::Text("ForceSpawn: %d", car.forceSpawn);
                                                                ImGui::Text("AlarmProb: %d", car.alarmProbability);
                                                                ImGui::Text("LockProb: %d", car.lockProbability);
                                                                ImGui::Text("Unknown: %d, %d", car.unknown1, car.unknown2);
                                                                ImGui::EndTooltip();
                                                            }
                                                        }
                                                    }
                                                    ImGui::TreePop();
                                                }

                                                if (hasZones && ImGui::TreeNodeEx("ZONE##ipl_zone_section", ImGuiTreeNodeFlags_SpanAvailWidth, "ZONE (%d)", static_cast<int>(res->data.zones.size())))
                                                {
                                                    ImGuiListClipper clipper;
                                                    clipper.Begin(static_cast<int>(res->data.zones.size()));
                                                    while (clipper.Step())
                                                    {
                                                        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                                                        {
                                                            const auto& zone = res->data.zones[static_cast<std::size_t>(i)];
                                                            ImGui::BulletText("%s", zone.name.c_str());
                                                            if (ImGui::IsItemHovered())
                                                            {
                                                                ImGui::BeginTooltip();
                                                                ImGui::Text("Name: %s", zone.name.c_str());
                                                                ImGui::Text("Type: %d", zone.type);
                                                                ImGui::Text("Level: %d", zone.level);
                                                                ImGui::Text("Min: %.3f, %.3f, %.3f", zone.minBounds.x, zone.minBounds.y, zone.minBounds.z);
                                                                ImGui::Text("Max: %.3f, %.3f, %.3f", zone.maxBounds.x, zone.maxBounds.y, zone.maxBounds.z);
                                                                ImGui::EndTooltip();
                                                            }
                                                        }
                                                    }
                                                    ImGui::TreePop();
                                                }
                                            }
                                            ImGui::TreePop();
                                        }
                                        ImGui::PopID();
                                        continue;
                                    }
                                }
                            }

                            // IMG archives referenced by gta.dat (VER2).
                            if ((e->keyword == eDatKeyword::Img || e->keyword == eDatKeyword::ImgList || e->keyword == eDatKeyword::CdImage) && !e->pathOrText.empty() && m_gameLoader)
                            {
                                const std::string absPathText = resolveAbsPathString(*e);
                                std::string ext = fs::path(absPathText).extension().string();
                                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                                if (ext == ".img")
                                {
                                    const std::string key = resolveAbsKey(*e);
                                    std::string displayPath = e->pathOrText;
                                    std::transform(displayPath.begin(), displayPath.end(), displayPath.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                                    const int count = [&]() -> int
                                    {
                                        auto it = m_cachedImgCountByAbsPath.find(key);
                                        if (it == m_cachedImgCountByAbsPath.end())
                                        {
                                            return -1;
                                        }
                                        return static_cast<int>(it->second);
                                    }();

                                    const std::string nodeId = displayPath + "##img_" + key;
                                    const bool nodeOpen = (count >= 0)
                                        ? ImGui::TreeNodeEx(nodeId.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth, "%s (%d)", displayPath.c_str(), count)
                                        : ImGui::TreeNodeEx(nodeId.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth, "%s", displayPath.c_str());

                                    if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight))
                                    {
                                        const std::string copyName = entryDisplayName(*e);
                                        if (ImGui::MenuItem("Скопировать название"))
                                        {
                                            std::string lowerName = copyName;
                                            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                                            ImGui::SetClipboardText(lowerName.c_str());
                                        }
                                        if (ImGui::MenuItem("Скопировать системный путь к файлу"))
                                        {
                                            ImGui::SetClipboardText(absPathText.c_str());
                                        }
                                        ImGui::EndPopup();
                                    }

                                    if (nodeOpen)
                                    {
                                        auto itRes = m_cachedImgResByAbsPath.find(key);
                                        const d11::data::tImgParseResult* res = (itRes != m_cachedImgResByAbsPath.end() && itRes->second) ? itRes->second.get() : nullptr;
                                        if (!res)
                                        {
                                            ImGui::TextDisabled("Нет результата парсинга IMG.");
                                        }
                                        else if (!res->errorMessage.empty())
                                        {
                                            ImGui::TextDisabled("%s", res->errorMessage.c_str());
                                        }
                                        else
                                        {
                                            const bool hasSortedOrder = !res->sortedEntryIndices.empty();
                                            const int displayCount = hasSortedOrder
                                                ? static_cast<int>(res->sortedEntryIndices.size())
                                                : static_cast<int>(res->entries.size());
                                            ImGuiListClipper clipper;
                                            clipper.Begin(displayCount);
                                            while (clipper.Step())
                                            {
                                                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                                                {
                                                    const std::size_t entryIndex = hasSortedOrder
                                                        ? res->sortedEntryIndices[static_cast<std::size_t>(i)]
                                                        : static_cast<std::size_t>(i);
                                                    const auto& entry = res->entries[entryIndex];
                                                    ImGui::PushID(i);
                                                    const bool isDff = FileNameLooksLikeDff(entry.name);
                                                    const bool isTxd = FileNameLooksLikeTxd(entry.name);
                                                    if (ImGui::Selectable(
                                                            entry.name.c_str(),
                                                            false,
                                                            ImGuiSelectableFlags_AllowDoubleClick))
                                                    {
                                                        if (isDff && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                                                        {
                                                            QueueModelPreviewFromIde(entry.name.substr(0, entry.name.size() - 4));
                                                        }
                                                        if (isTxd && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                                                        {
                                                            OpenTxdInspectorFromImgEntry(fs::path(absPathText), entry);
                                                        }
                                                    }
                                                    if (ImGui::BeginPopupContextItem("gta_dat_img_entry_ctx", ImGuiPopupFlags_MouseButtonRight))
                                                    {
                                                        if (isDff && ImGui::MenuItem("3D preview"))
                                                        {
                                                            QueueModelPreviewFromIde(entry.name.substr(0, entry.name.size() - 4));
                                                        }
                                                        if (isTxd && ImGui::MenuItem("Список текстур"))
                                                        {
                                                            OpenTxdInspectorFromImgEntry(fs::path(absPathText), entry);
                                                        }
                                                        if (ImGui::MenuItem("Скопировать название"))
                                                        {
                                                            ImGui::SetClipboardText(entry.name.c_str());
                                                        }
                                                        ImGui::EndPopup();
                                                    }

                                                    const bool hovered = ImGui::IsItemHovered();
                                                    if (hovered)
                                                    {
                                                        ImDrawList* dl = ImGui::GetWindowDrawList();
                                                        ImVec2 a = ImGui::GetItemRectMin();
                                                        ImVec2 b = ImGui::GetItemRectMax();
                                                        const ImVec2 winPos = ImGui::GetWindowPos();
                                                        const float contentMaxX = winPos.x + ImGui::GetWindowContentRegionMax().x;
                                                        b.x = contentMaxX;
                                                        a.x -= 6.0f; a.y -= 1.0f;
                                                        b.x += 6.0f; b.y += 1.0f;
                                                        dl->AddRect(a, b, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.16f)), 6.0f, 0, 1.0f);

                                                        const std::uint64_t start = static_cast<std::uint64_t>(entry.offsetBytes);
                                                        const std::uint64_t size = static_cast<std::uint64_t>(entry.streamingBytes);
                                                        const std::uint64_t end = start + size;

                                                        char sizeBuf[64] {};
                                                        if (size < 1024ull)
                                                        {
                                                            std::snprintf(sizeBuf, sizeof(sizeBuf), "%llu байт", static_cast<unsigned long long>(size));
                                                        }
                                                        else if (size < 1024ull * 1024ull)
                                                        {
                                                            const double kb = static_cast<double>(size) / 1024.0;
                                                            std::snprintf(sizeBuf, sizeof(sizeBuf), "%.2f КБ", kb);
                                                        }
                                                        else
                                                        {
                                                            const double mb = static_cast<double>(size) / (1024.0 * 1024.0);
                                                            std::snprintf(sizeBuf, sizeof(sizeBuf), "%.2f МБ", mb);
                                                        }

                                                        ImGui::BeginTooltip();
                                                        const std::string archiveStem = fs::path(absPathText).stem().string();
                                                        const std::string headerText = std::string("IMG - ") + archiveStem;
                                                        ImGui::TextUnformatted(headerText.c_str());
                                                        ImGui::Separator();
                                                        ImGui::Text("ID: %d", static_cast<int>(entryIndex + 1));
                                                        ImGui::Text("Размер: %s", sizeBuf);
                                                        ImGui::Text("Начало: %llu (0x%llX)", static_cast<unsigned long long>(start), static_cast<unsigned long long>(start));
                                                        ImGui::Text("Конец:  %llu (0x%llX)", static_cast<unsigned long long>(end), static_cast<unsigned long long>(end));
                                                        if (isDff)
                                                        {
                                                            ImGui::Separator();
                                                            ImGui::TextDisabled("Double-click or RMB: 3D preview");
                                                        }
                                                        if (isTxd)
                                                        {
                                                            ImGui::Separator();
                                                            ImGui::TextDisabled("Double-click or RMB: список текстур (TXD)");
                                                        }
                                                        ImGui::EndTooltip();
                                                    }
                                                    ImGui::PopID();
                                                }
                                            }
                                        }

                                        ImGui::TreePop();
                                    }

                                    continue;
                                }
                            }

                            drawEntry(*e);
                        }
                        ImGui::TreePop();
                    }
                    ImGui::Spacing();
                };

                // Порядок по запросу: IDE -> IPL -> IMG, далее всё остальное отдельным блоком.
                auto sumNested = [&](const std::vector<const tGtaDatCatalogEntry*>& items) -> std::size_t
                {
                    std::size_t total = 0;
                    if (!m_gameLoader)
                    {
                        return 0;
                    }
                    for (const auto* e : items)
                    {
                        if (!e || e->pathOrText.empty())
                        {
                            continue;
                        }
                        const std::string key = resolveAbsKey(*e);
                        if (e->keyword == eDatKeyword::Ide)
                        {
                            auto it = m_cachedIdeCountByAbsPath.find(key);
                            if (it != m_cachedIdeCountByAbsPath.end())
                            {
                                total += it->second;
                            }
                        }
                        else if (e->keyword == eDatKeyword::Ipl && !e->skippedAsZoneIpl)
                        {
                            auto it = m_cachedIplCountByAbsPath.find(key);
                            if (it != m_cachedIplCountByAbsPath.end())
                            {
                                total += it->second;
                            }
                        }
                    }
                    return total;
                };

                section("IDE", "section_ide", m_cachedIdeEntries, false, sumNested(m_cachedIdeEntries));
                section("IPL", "section_ipl", m_cachedIplEntries, false, sumNested(m_cachedIplEntries));
                auto sumNestedImg = [&](const std::vector<const tGtaDatCatalogEntry*>& items) -> std::size_t
                {
                    std::size_t total = 0;
                    if (!m_gameLoader)
                    {
                        return 0;
                    }
                    for (const auto* e : items)
                    {
                        if (!e || e->pathOrText.empty())
                        {
                            continue;
                        }
                        const std::string key = resolveAbsKey(*e);
                        auto it = m_cachedImgCountByAbsPath.find(key);
                        if (it != m_cachedImgCountByAbsPath.end())
                        {
                            total += it->second;
                        }
                    }
                    return total;
                };
                section("IMG", "section_img", m_cachedImgEntries, false, sumNestedImg(m_cachedImgEntries));
                section("ДРУГОЕ", "section_other", m_cachedOtherEntries, false, 0);

                ImGui::TreePop();
            }

            // Sibling node of gta.dat, not nested inside.
            const fs::path gameRoot = m_gameLoader ? m_gameLoader->GetGameRootPath() : fs::path{};
            const fs::path modelsDir = gameRoot.empty() ? fs::path{} : (gameRoot / "models");
            const std::string modelsDirKey = MakePathKey(modelsDir);

            // Refresh models IMG list from already-parsed archives (no IO on UI thread).
            // IMPORTANT: do not compare access.imgByAbsPath.size() to m_cachedModelsImgParsed.size() —
            // the loader map holds *all* game IMG archives, while the cache only keeps entries under
            // game_root/models/. That mismatch was always true and forced a full TXD directory rescan
            // every frame (recursive_directory_iterator), destroying FPS.
            if (m_gameLoader && !modelsDir.empty())
            {
                std::size_t loaderModelsImgCount = 0;
                if (!modelsDirKey.empty())
                {
                    for (const auto& kv : access.imgByAbsPath)
                    {
                        const fs::path absKeyPath = fs::path(kv.first);
                        if (MakePathKey(absKeyPath.parent_path()) == modelsDirKey)
                        {
                            ++loaderModelsImgCount;
                        }
                    }
                }
                const bool modelsImgListStale =
                    !m_modelsImgScanned ||
                    m_cachedModelsRoot != modelsDir ||
                    loaderModelsImgCount != m_cachedModelsImgPaths.size();
                if (modelsImgListStale)
                {
                    m_cachedModelsRoot = modelsDir;
                    m_modelsImgScanned = true;
                    m_cachedModelsImgPaths.clear();
                    m_cachedModelsImgParsed.clear();
                    m_modelsImgInitNextIndex = 0;

                    for (const auto& kv : access.imgByAbsPath)
                    {
                        // kv.first is already a normalized lower-case key.
                        const fs::path absKeyPath = fs::path(kv.first);
                        const std::string parentKey = MakePathKey(absKeyPath.parent_path());
                        if (!modelsDirKey.empty() && parentKey == modelsDirKey)
                        {
                            m_cachedModelsImgPaths.push_back(absKeyPath.filename().string());
                            m_cachedModelsImgParsed.emplace(kv.first, kv.second);
                        }
                    }
                    std::sort(m_cachedModelsImgPaths.begin(), m_cachedModelsImgPaths.end());

                    m_cachedModelsTxdPaths.clear();
                    std::error_code txdEc;
                    if (fs::exists(modelsDir))
                    {
                        fs::recursive_directory_iterator txdIt(
                            modelsDir,
                            fs::directory_options::skip_permission_denied,
                            txdEc);
                        const fs::recursive_directory_iterator txdEnd;
                        for (; !txdEc && txdIt != txdEnd; txdIt.increment(txdEc))
                        {
                            const fs::directory_entry& dirEnt = *txdIt;
                            if (!dirEnt.is_regular_file())
                            {
                                continue;
                            }
                            const std::string fname = dirEnt.path().filename().string();
                            if (!FileNameLooksLikeTxd(fname))
                            {
                                continue;
                            }
                            std::error_code relEc;
                            fs::path rel = fs::relative(dirEnt.path(), modelsDir, relEc);
                            if (relEc || rel.empty())
                            {
                                continue;
                            }
                            m_cachedModelsTxdPaths.push_back(rel.lexically_normal().generic_string());
                        }
                        std::sort(m_cachedModelsTxdPaths.begin(), m_cachedModelsTxdPaths.end());
                    }
                }
            }

            const int modelsImgCountLabel = m_modelsImgScanned ? static_cast<int>(m_cachedModelsImgPaths.size()) : 0;
            const int modelsTxdCountLabel = m_modelsImgScanned ? static_cast<int>(m_cachedModelsTxdPaths.size()) : 0;
            if (collapseHierarchyRequested)
            {
                ImGui::SetNextItemOpen(false, ImGuiCond_Always);
            }
            const bool modelsOpen = ImGui::TreeNodeEx(
                "models##root_models",
                ImGuiTreeNodeFlags_SpanAvailWidth,
                "models (IMG %d · TXD %d)",
                modelsImgCountLabel,
                modelsTxdCountLabel);

            if (modelsOpen)
            {
                const int imgArchivesCountLabel = m_modelsImgScanned ? static_cast<int>(m_cachedModelsImgPaths.size()) : 0;
                std::size_t totalEntriesInImg = 0;
                for (const auto& kv : m_cachedModelsImgParsed)
                {
                    if (!kv.second)
                    {
                        continue;
                    }
                    if (!kv.second->errorMessage.empty())
                    {
                        continue;
                    }
                    totalEntriesInImg += kv.second->entries.size();
                }

                if (ImGui::TreeNodeEx(
                        "IMG##models_img",
                        ImGuiTreeNodeFlags_SpanAvailWidth,
                        "IMG (%d) (%d)",
                        imgArchivesCountLabel,
                        static_cast<int>(totalEntriesInImg)))
                {
                    if (modelsDir.empty())
                    {
                        ImGui::TextDisabled("Game root не определён.");
                    }
                    else if (!fs::exists(modelsDir))
                    {
                        ImGui::TextDisabled("Папка models не найдена.");
                    }
                    else if (m_cachedModelsImgPaths.empty())
                    {
                        ImGui::TextDisabled("В папке models нет .img файлов.");
                    }
                    else
                    {
                        // IMPORTANT: do NOT use ImGuiListClipper here.
                        // Archives are TreeNodes and can expand -> variable height, which breaks the clipper and causes "missing" nodes below.
                        for (std::size_t i = 0; i < m_cachedModelsImgPaths.size(); ++i)
                        {
                            const auto& name = m_cachedModelsImgPaths[i];
                            ImGui::PushID(static_cast<int>(i));
                            // Show count in the archive label once parsed: "foo.img (1234)".
                            const fs::path absPathPreview = modelsDir / fs::path(name);
                            const std::string absKeyPreview = MakePathKey(absPathPreview);
                            const auto itCachedPreview = m_cachedModelsImgParsed.find(absKeyPreview);
                            const int cachedCount = (itCachedPreview != m_cachedModelsImgParsed.end() && itCachedPreview->second)
                                ? static_cast<int>(itCachedPreview->second->entries.size())
                                : -1;

                            const char* fmt = (cachedCount >= 0) ? "%s (%d)" : "%s";
                            const bool archiveOpen = (cachedCount >= 0)
                                ? ImGui::TreeNodeEx("##img_archive", ImGuiTreeNodeFlags_SpanAvailWidth, fmt, name.c_str(), cachedCount)
                                : ImGui::TreeNodeEx("##img_archive", ImGuiTreeNodeFlags_SpanAvailWidth, fmt, name.c_str());

                            if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonRight))
                            {
                                if (ImGui::MenuItem("Скопировать название"))
                                {
                                    ImGui::SetClipboardText(name.c_str());
                                }
                                if (ImGui::MenuItem("Скопировать системный путь к файлу"))
                                {
                                    ImGui::SetClipboardText(absKeyPreview.c_str());
                                }
                                ImGui::EndPopup();
                            }

                            if (archiveOpen)
                            {
                                const std::string absKey = absKeyPreview;
                                auto itRes = m_cachedModelsImgParsed.find(absKey);
                                if (itRes == m_cachedModelsImgParsed.end() || !itRes->second)
                                {
                                    ImGui::TextDisabled("IMG ещё не распарсен (фон).");
                                    ImGui::TreePop();
                                    ImGui::PopID();
                                    continue;
                                }

                                const auto& res = *itRes->second;
                                if (!res.errorMessage.empty())
                                {
                                    ImGui::TextDisabled("%s", res.errorMessage.c_str());
                                }
                                else
                                {
                                    const bool hasSortedOrder = !res.sortedEntryIndices.empty();
                                    const int displayCount = hasSortedOrder
                                        ? static_cast<int>(res.sortedEntryIndices.size())
                                        : static_cast<int>(res.entries.size());
                                    ImGuiListClipper inner;
                                    inner.Begin(displayCount);
                                    while (inner.Step())
                                    {
                                        for (int j = inner.DisplayStart; j < inner.DisplayEnd; ++j)
                                        {
                                            const std::size_t entryIndex = hasSortedOrder
                                                ? res.sortedEntryIndices[static_cast<std::size_t>(j)]
                                                : static_cast<std::size_t>(j);
                                            const auto& e = res.entries[entryIndex];
                                            ImGui::PushID(j);
                                            const bool isDffModels = FileNameLooksLikeDff(e.name);
                                            const bool isTxdModels = FileNameLooksLikeTxd(e.name);
                                            if (ImGui::Selectable(
                                                    e.name.c_str(),
                                                    false,
                                                    ImGuiSelectableFlags_AllowDoubleClick))
                                            {
                                                if (isDffModels && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                                                {
                                                    QueueModelPreviewFromIde(e.name.substr(0, e.name.size() - 4));
                                                }
                                                if (isTxdModels && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                                                {
                                                    OpenTxdInspectorFromImgEntry(absPathPreview, e);
                                                }
                                            }
                                            if (ImGui::BeginPopupContextItem("img_entry_ctx", ImGuiPopupFlags_MouseButtonRight))
                                            {
                                                if (isDffModels && ImGui::MenuItem("3D preview"))
                                                {
                                                    QueueModelPreviewFromIde(e.name.substr(0, e.name.size() - 4));
                                                }
                                                if (isTxdModels && ImGui::MenuItem("Список текстур"))
                                                {
                                                    OpenTxdInspectorFromImgEntry(absPathPreview, e);
                                                }
                                                if (ImGui::MenuItem("Скопировать название"))
                                                {
                                                    ImGui::SetClipboardText(e.name.c_str());
                                                }
                                                ImGui::EndPopup();
                                            }

                                            const bool itemHovered = ImGui::IsItemHovered();
                                            if (itemHovered)
                                            {
                                                ImDrawList* dl = ImGui::GetWindowDrawList();
                                                ImVec2 a = ImGui::GetItemRectMin();
                                                ImVec2 b = ImGui::GetItemRectMax();

                                                const ImVec2 winPos = ImGui::GetWindowPos();
                                                const float contentMaxX = winPos.x + ImGui::GetWindowContentRegionMax().x;
                                                b.x = contentMaxX;

                                                a.x -= 6.0f; a.y -= 1.0f;
                                                b.x += 6.0f; b.y += 1.0f;
                                                dl->AddRect(a, b, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.16f)), 6.0f, 0, 1.0f);

                                                const std::uint64_t start = static_cast<std::uint64_t>(e.offsetBytes);
                                                const std::uint64_t size = static_cast<std::uint64_t>(e.streamingBytes);
                                                const std::uint64_t end = start + size;

                                                ImGui::BeginTooltip();
                                                const std::string archiveStem = absPathPreview.stem().string();
                                                const std::string header = std::string("IMG - ") + archiveStem;
                                                ImGui::TextUnformatted(header.c_str());
                                                ImGui::Separator();
                                                ImGui::Text("ID: %d", static_cast<int>(entryIndex + 1));
                                                char sizeBuf[64] {};
                                                if (size < 1024ull)
                                                {
                                                    std::snprintf(sizeBuf, sizeof(sizeBuf), "%llu байт", static_cast<unsigned long long>(size));
                                                }
                                                else if (size < 1024ull * 1024ull)
                                                {
                                                    const double kb = static_cast<double>(size) / 1024.0;
                                                    std::snprintf(sizeBuf, sizeof(sizeBuf), "%.2f КБ", kb);
                                                }
                                                else
                                                {
                                                    const double mb = static_cast<double>(size) / (1024.0 * 1024.0);
                                                    std::snprintf(sizeBuf, sizeof(sizeBuf), "%.2f МБ", mb);
                                                }
                                                ImGui::Text("Размер: %s", sizeBuf);
                                                ImGui::Text("Начало: %llu (0x%llX)", static_cast<unsigned long long>(start), static_cast<unsigned long long>(start));
                                                ImGui::Text("Конец:  %llu (0x%llX)", static_cast<unsigned long long>(end), static_cast<unsigned long long>(end));
                                                if (isDffModels)
                                                {
                                                    ImGui::Separator();
                                                    ImGui::TextDisabled("Double-click or RMB: 3D preview");
                                                }
                                                if (isTxdModels)
                                                {
                                                    ImGui::Separator();
                                                    ImGui::TextDisabled("Double-click or RMB: список текстур (TXD)");
                                                }
                                                ImGui::EndTooltip();
                                            }
                                            ImGui::PopID();
                                        }
                                    }
                                }

                                ImGui::TreePop();
                            }

                            ImGui::PopID();
                        }
                    }
                    ImGui::TreePop();
                }

                const int txdNodeCount = m_modelsImgScanned ? static_cast<int>(m_cachedModelsTxdPaths.size()) : 0;
                if (ImGui::TreeNodeEx("TXD##models_txd", ImGuiTreeNodeFlags_SpanAvailWidth, "TXD (%d)", txdNodeCount))
                {
                    if (modelsDir.empty())
                    {
                        ImGui::TextDisabled("Game root не определён.");
                    }
                    else if (!fs::exists(modelsDir))
                    {
                        ImGui::TextDisabled("Папка models не найдена.");
                    }
                    else if (m_cachedModelsTxdPaths.empty())
                    {
                        ImGui::TextDisabled("В папке models (включая подпапки) нет .txd файлов.");
                    }
                    else
                    {
                        ImGuiListClipper txdClip;
                        txdClip.Begin(static_cast<int>(m_cachedModelsTxdPaths.size()));
                        while (txdClip.Step())
                        {
                            for (int ti = txdClip.DisplayStart; ti < txdClip.DisplayEnd; ++ti)
                            {
                                const std::string& txdName = m_cachedModelsTxdPaths[static_cast<std::size_t>(ti)];
                                ImGui::PushID(ti);
                                const fs::path txdAbs = (modelsDir / fs::path(txdName)).lexically_normal();
                                if (ImGui::Selectable(txdName.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
                                {
                                    if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                                    {
                                        OpenTxdInspectorFromFile(txdAbs, txdName);
                                    }
                                }
                                if (ImGui::BeginPopupContextItem("txd_file_ctx", ImGuiPopupFlags_MouseButtonRight))
                                {
                                    if (ImGui::MenuItem("Список текстур"))
                                    {
                                        OpenTxdInspectorFromFile(txdAbs, txdName);
                                    }
                                    if (ImGui::MenuItem("Скопировать название"))
                                    {
                                        ImGui::SetClipboardText(txdName.c_str());
                                    }
                                    if (ImGui::MenuItem("Скопировать системный путь к файлу"))
                                    {
                                        const std::string absTxd = MakePathKey(txdAbs);
                                        ImGui::SetClipboardText(absTxd.c_str());
                                    }
                                    ImGui::EndPopup();
                                }
                                ImGui::PopID();
                            }
                        }
                    }
                    ImGui::TreePop();
                }

                ImGui::TreePop();
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void DebugUi::DrawCameraWindow(
    const ImVec2& panelPos,
    float panelWidth,
    d11::render::GtaStyleCamera& camera,
    float fontScale,
    float panelCornerRounding
)
{
    constexpr ImGuiWindowFlags kPanelWindowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    if (!m_showCameraWindow)
    {
        return;
    }

    ImGui::SetNextWindowPos(panelPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, 0.0f), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, panelCornerRounding);
    if (ImGui::Begin("Настройки камеры", &m_showCameraWindow, kPanelWindowFlags))
    {
        ImGui::SetWindowFontScale(fontScale);
        ImGui::Text("Позиция: X %.2f  Y %.2f  Z %.2f", camera.GetX(), camera.GetY(), camera.GetZ());
        ImGui::Text("Yaw/Pitch: %.4f / %.4f", camera.GetYawDegrees(), camera.GetPitchDegrees());
        ImGui::Separator();
        float fov = camera.GetFovDegrees();
        if (ImGui::SliderFloat("Поле зрения (FOV)", &fov, 20.0f, 140.0f, "%.1f град"))
        {
            camera.SetFovDegrees(fov);
            ResetFovInertiaToCamera(camera);
        }
        fov = camera.GetFovDegrees();
        float mouseSensEffective = EffectiveMouseLookSensitivity(m_cameraSettings.mouseLookSensitivity, fov);
        if (ImGui::SliderFloat("Чувствительность мыши", &mouseSensEffective, 0.001f, 0.45f, "%.4f"))
        {
            const float base =
                BaseMouseLookSensitivityFromEffective(mouseSensEffective, fov);
            m_cameraSettings.mouseLookSensitivity =
                std::min(0.45f, std::max(0.03f, base));
        }
        ImGui::TextDisabled("База при FOV 90°: %.3f", m_cameraSettings.mouseLookSensitivity);
        ImGui::SliderFloat("Скорость движения", &m_cameraSettings.moveSpeed, 1.0f, 2400.0f, "%.0f");
        ImGui::SliderFloat("Множитель Shift", &m_cameraSettings.shiftSpeedMultiplier, 1.0f, 5.0f, "%.2f");
        ImGui::SliderFloat("Множитель Alt", &m_cameraSettings.altSpeedMultiplier, 0.1f, 1.0f, "%.2f");
        ImGui::SliderFloat("Сила инерции", &m_cameraSettings.movementSpringStrength, 2.0f, 60.0f, "%.2f");
        ImGui::SliderFloat("Затухание инерции", &m_cameraSettings.movementDamping, 0.50f, 0.99f, "%.3f");
        if (ImGui::Checkbox("Инертный скролл FOV", &m_fovSettings.enabled))
        {
            ResetFovInertiaToCamera(camera);
        }
        ImGui::BeginDisabled(!m_fovSettings.enabled);
        ImGui::SliderFloat("Шаг скролла FOV", &m_fovSettings.step, 0.5f, 10.0f, "%.2f");
        ImGui::SliderFloat("Пружина FOV", &m_fovSettings.spring, 2.0f, 30.0f, "%.2f");
        ImGui::SliderFloat("Затухание FOV", &m_fovSettings.damping, 0.50f, 0.99f, "%.3f");
        ImGui::EndDisabled();
        if (ImGui::Button("Сбросить параметры камеры"))
        {
            ResetCameraSettingsToDefaults(camera);
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void DebugUi::DrawPerformanceWindow(
    const ImVec2& panelPos,
    float panelWidth,
    float frameMs,
    float panelCornerRounding,
    float fontScale
)
{
    constexpr ImGuiWindowFlags kPanelWindowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    if (!m_showPerformanceWindow)
    {
        return;
    }

    ImGui::SetNextWindowPos(panelPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, 0.0f), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, panelCornerRounding);
    if (ImGui::Begin("Производительность", &m_showPerformanceWindow, kPanelWindowFlags))
    {
        ImGui::SetWindowFontScale(fontScale);
        float minMs = m_frameHistoryCount > 0 ? m_frameTimeHistoryMs[0] : 0.0f;
        float maxMs = minMs;
        float sumMs = 0.0f;
        std::array<float, kFrameHistorySize> ordered {};
        for (std::size_t i = 0; i < m_frameHistoryCount; ++i)
        {
            const std::size_t idx = (m_frameHistoryHead + kFrameHistorySize - m_frameHistoryCount + i) % kFrameHistorySize;
            const float v = m_frameTimeHistoryMs[idx];
            ordered[i] = v;
            minMs = std::min(minMs, v);
            maxMs = std::max(maxMs, v);
            sumMs += v;
        }
        const float avgMs = m_frameHistoryCount > 0 ? (sumMs / static_cast<float>(m_frameHistoryCount)) : 0.0f;
        const float avgFps = avgMs > 0.0f ? (1000.0f / avgMs) : 0.0f;
        const float p1Ms = 1000.0f / 60.0f;
        const float p0_1Ms = 1000.0f / 30.0f;

        ImGui::Text("Средний FPS: %.1f", avgFps);
        ImGui::Text("Время кадра: текущее %.2f | среднее %.2f | минимум %.2f | максимум %.2f", frameMs, avgMs, minMs, maxMs);
        ImGui::Text("Ориентиры: 60 FPS = %.2f мс | 30 FPS = %.2f мс", p1Ms, p0_1Ms);
        const float graphCeil = (maxMs + 1.0f > 50.0f) ? (maxMs + 1.0f) : 50.0f;
        ImGui::PlotLines(
            "График времени кадра (мс)",
            ordered.data(),
            static_cast<int>(m_frameHistoryCount),
            0,
            nullptr,
            0.0f,
            graphCeil,
            ImVec2(0.0f, 140.0f)
        );
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void DebugUi::DrawControlsWindow(
    const ImVec2& panelPos,
    float panelWidth,
    float panelCornerRounding,
    float fontScale
)
{
    constexpr ImGuiWindowFlags kPanelWindowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    if (!m_showControlsWindow)
    {
        return;
    }

    ImGui::SetNextWindowPos(panelPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, 0.0f), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, panelCornerRounding);
    if (ImGui::Begin("Управление", &m_showControlsWindow, kPanelWindowFlags))
    {
        ImGui::SetWindowFontScale(fontScale);
        ImGui::BulletText("Осмотр: зажать ЛКМ и двигать мышь");
        ImGui::BulletText("Движение: W / A / S / D");
        ImGui::BulletText("Вверх / вниз: Space / Ctrl");
        ImGui::BulletText("Ускорение / замедление: Shift / Alt");
        ImGui::BulletText("FOV: колесо мыши");
        ImGui::BulletText("Выход: Esc");
        ImGui::Spacing();
        ImGui::TextDisabled("Повторный клик по кнопке в чёлке закрывает окно.");
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void DebugUi::DrawSceneWindow(
    const ImVec2& panelPos,
    float panelWidth,
    float panelCornerRounding,
    float fontScale
)
{
    constexpr ImGuiWindowFlags kPanelWindowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    if (!m_showSceneWindow)
    {
        return;
    }

    // Keep the same anchor axis as other panels (Tree/Camera/Performance/Controls).
    ImGui::SetNextWindowPos(panelPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, 0.0f), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, panelCornerRounding);
    if (ImGui::Begin("Сцена", &m_showSceneWindow, kPanelWindowFlags))
    {
        ImGui::SetWindowFontScale(fontScale);

        const float sceneBtnW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
        if (ImGui::Button("Импорт сцены", ImVec2(sceneBtnW, 0.0f)))
        {
            std::printf("[SceneImport] UI: \"Импорт сцены\" clicked, request queued for next frame\n");
            std::fflush(stdout);
            m_importSceneRequested = true;
        }
        ImGui::SameLine();
        const char* rotationModeLabel = m_useQuaternionImportMode ? "Кватернионы" : "Эйлер";
        if (ImGui::Button(rotationModeLabel, ImVec2(sceneBtnW, 0.0f)))
        {
            m_useQuaternionImportMode = !m_useQuaternionImportMode;
        }
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Переключить режим поворота (R)");
        }

        ImGui::Spacing();
        ImGui::Checkbox("Кубы IPL (placeholder)", &m_showImportedFallbackCubes);
        ImGui::Checkbox("DFF в сцене", &m_showImportedDffMeshes);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Меши из IMG/models подгружаются в фоне после импорта (как в старом клиенте).");
        }
        ImGui::Checkbox("Отсечение DFF по дист. IDE (LOD)", &m_sceneDffIdeDistanceCull);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip(
                "Выключено — все DFF в кадре, как старый клиент в режиме «без LOD». Включено — дальние объекты по draw distance из IDE.");
        }
        ImGui::Spacing();
        ImGui::Checkbox("Отображать воду", &m_showSceneWater);
        ImGui::Checkbox("Грид лист", &m_showSceneGrid);
        ImGui::Checkbox("Отображать оси X Y Z", &m_showSceneAxes);
        ImGui::Checkbox("Отображать скайбокс", &m_showSceneSkybox);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Скайбокс пока не подключён — переключатель зарезервирован.");
        }

        if (m_sceneStats.has_value())
        {
            ImGui::Separator();
            const SceneStats& st = m_sceneStats.value();
            ImGui::Text("Последний импорт");
            ImGui::Text("INST (exterior): %zu", st.instCount);
            ImGui::Text("Уник. моделей: %zu", st.uniqueModelCount);
            ImGui::Text("Кубов: %zu", st.cubeInstances);
            const std::string verticesPretty = FormatCountCompactRu(st.vertices);
            const std::string trianglesPretty = FormatCountCompactRu(st.triangles);
            ImGui::Text("Вершин: %s", verticesPretty.c_str());
            ImGui::Text("Полигонов (треуг.): %s", trianglesPretty.c_str());
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void DebugUi::DrawHoveredCubeTooltip() const
{
    if (!m_hoveredCubeInfo.has_value())
    {
        return;
    }
    // Show scene tooltip only while Left Alt is held.
    if ((GetAsyncKeyState(VK_LMENU) & 0x8000) == 0)
    {
        return;
    }
    if (ImGui::GetIO().WantCaptureMouse)
    {
        return;
    }

    const HoveredCubeInfo& info = m_hoveredCubeInfo.value();
    ImGui::BeginTooltip();
    ImGui::TextUnformatted("Объект сцены");
    ImGui::Separator();
    ImGui::Text("ID: %d", info.objectId);
    ImGui::Text("Название: %s", info.modelName.empty() ? "(unknown)" : info.modelName.c_str());
    DrawSaIdeFlagsTooltip(info.flags);
    ImGui::Text("Дистанция: %.3f", info.drawDistance);
    ImGui::Text("LOD: %d", info.lodIndex);
    ImGui::Separator();
    ImGui::TextUnformatted("DFF");
    if (info.hasDffSummary)
    {
        ImGui::Text("RW версия: 0x%05X", info.dffRwVersion);
        ImGui::Text("Frames: %d", info.dffFrameCount);
        ImGui::Text("Geometries: %d", info.dffGeometryCount);
        ImGui::Text("Atomics: %d", info.dffAtomicCount);
        ImGui::Text("Materials: %d", info.dffMaterialCount);
        ImGui::Text("Textured materials: %d", info.dffTexturedMaterialCount);
        if (!info.dffTexturePreview.empty())
        {
            ImGui::TextWrapped("Textures: %s", info.dffTexturePreview.c_str());
        }
        ImGui::Text("Issues: %d", info.dffIssueCount);
        if (!info.dffArchiveName.empty())
        {
            ImGui::Text("IMG: %s", info.dffArchiveName.c_str());
        }
        if (!info.dffEntryName.empty())
        {
            ImGui::Text("Entry: %s", info.dffEntryName.c_str());
        }
    }
    else
    {
        ImGui::TextDisabled("%s", info.dffError.empty() ? "DFF summary недоступен" : info.dffError.c_str());
    }
    ImGui::EndTooltip();
}

void DebugUi::QueueModelPreviewFromIde(std::string modelName)
{
    m_modelPreviewDisplayName = modelName;
    m_modelPreviewPendingRequest = m_modelPreviewDisplayName;
    ++m_modelPreviewLoadToken;
    m_modelPreviewWindowOpen = true;
    m_modelPreviewTexValid = false;
    m_modelPreviewTexId = ImTextureID_Invalid;
    m_modelPreviewTriangleCount = 0;
    m_modelPreviewVertexCount = 0;
    m_modelPreviewStatusText.clear();
    m_modelPreviewNavYawAccum = 0.0f;
    m_modelPreviewNavPitchAccum = 0.0f;
    m_modelPreviewNavWheelAccum = 0.0f;
    m_modelPreviewNavPanXAccum = 0.0f;
    m_modelPreviewNavPanYAccum = 0.0f;
    m_modelPreviewLmbOrbitCapture = false;
    m_modelPreviewMmbPanCapture = false;
}

bool DebugUi::ConsumeModelPreviewRequest(std::string& outModelName)
{
    if (m_modelPreviewPendingRequest.empty())
    {
        return false;
    }
    outModelName = std::move(m_modelPreviewPendingRequest);
    return true;
}

void DebugUi::SetModelPreviewFrame(
    ImTextureID texId,
    bool textureValid,
    std::uint32_t triangleCount,
    std::uint32_t vertexCount,
    const char* errorOrNull)
{
    m_modelPreviewTexId = texId;
    m_modelPreviewTexValid = textureValid;
    m_modelPreviewTriangleCount = triangleCount;
    m_modelPreviewVertexCount = vertexCount;
    m_modelPreviewStatusText = errorOrNull ? errorOrNull : "";
}

void DebugUi::GetAndResetModelPreviewNav(
    float& yawDeltaDeg,
    float& pitchDeltaDeg,
    float& wheelSteps,
    float& panMouseDx,
    float& panMouseDy)
{
    yawDeltaDeg = m_modelPreviewNavYawAccum;
    pitchDeltaDeg = m_modelPreviewNavPitchAccum;
    wheelSteps = m_modelPreviewNavWheelAccum;
    panMouseDx = m_modelPreviewNavPanXAccum;
    panMouseDy = m_modelPreviewNavPanYAccum;
    m_modelPreviewNavYawAccum = 0.0f;
    m_modelPreviewNavPitchAccum = 0.0f;
    m_modelPreviewNavWheelAccum = 0.0f;
    m_modelPreviewNavPanXAccum = 0.0f;
    m_modelPreviewNavPanYAccum = 0.0f;
}

void DebugUi::DrawModelPreviewWindow()
{
    if (!m_modelPreviewWindowOpen)
    {
        return;
    }

    // Должно совпадать с Dx11RenderPipeline::kModelPreviewSize (сторона квадратного RT).
    constexpr float kPreviewPx = 600.0f;
    constexpr ImGuiWindowFlags kPreviewWinFlags =
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    const float closeBtn = ImGui::GetFrameHeight();
    const float headerH = closeBtn + ImGui::GetStyle().ItemSpacing.y + 4.0f;
    const float errExtra =
        m_modelPreviewStatusText.empty() ? 0.0f : (ImGui::GetTextLineHeightWithSpacing() + 6.0f);

    ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 center = vp->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    const float winW = kPreviewPx + ImGui::GetStyle().WindowPadding.x * 2.0f;
    const float winH = headerH + kPreviewPx + ImGui::GetStyle().WindowPadding.y * 2.0f + errExtra;
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);

    const bool wasOpen = m_modelPreviewWindowOpen;
    if (ImGui::Begin("##ide_model_preview", &m_modelPreviewWindowOpen, kPreviewWinFlags))
    {
        // Одна строка: название + счётчики слева, зона закрытия справа; крестик — линии по диагонали (ровно по центру).
        ImGui::AlignTextToFramePadding();
        const char* titleName = m_modelPreviewDisplayName.empty() ? "—" : m_modelPreviewDisplayName.c_str();
        ImGui::TextUnformatted(titleName);
        ImGui::SameLine();
        const unsigned poly = static_cast<unsigned>(m_modelPreviewTriangleCount);
        const unsigned vtx = static_cast<unsigned>(m_modelPreviewVertexCount);
        ImGui::TextDisabled("(полигонов: %u, вершин: %u)", poly, vtx);
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - closeBtn);

        const ImVec2 closeMin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##preview_close", ImVec2(closeBtn, closeBtn));
        const bool closeHover = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            m_modelPreviewWindowOpen = false;
        }
        if (closeHover)
        {
            ImGui::SetTooltip("Закрыть");
        }
        const ImVec2 closeMax = ImVec2(closeMin.x + closeBtn, closeMin.y + closeBtn);
        ImDrawList* wdl = ImGui::GetWindowDrawList();
        if (closeHover)
        {
            wdl->AddRectFilled(closeMin, closeMax, IM_COL32(220, 75, 65, 85));
        }
        const ImU32 xCol = ImGui::GetColorU32(ImGuiCol_Text);
        const float inset = closeBtn * 0.30f;
        const float th = (std::max)(1.0f, ImGui::GetFontSize() * 0.09f);
        wdl->AddLine(
            ImVec2(closeMin.x + inset, closeMin.y + inset),
            ImVec2(closeMax.x - inset, closeMax.y - inset),
            xCol,
            th);
        wdl->AddLine(
            ImVec2(closeMax.x - inset, closeMin.y + inset),
            ImVec2(closeMin.x + inset, closeMax.y - inset),
            xCol,
            th);

        ImGui::Separator();

        if (!m_modelPreviewStatusText.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.35f, 1.0f), "%s", m_modelPreviewStatusText.c_str());
        }

        const ImVec2 imgSize(kPreviewPx, kPreviewPx);
        const auto applyPreviewInteractions = [&](bool hovered)
        {
            ImGuiIO& io = ImGui::GetIO();
            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                m_modelPreviewLmbOrbitCapture = true;
            }
            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
            {
                m_modelPreviewMmbPanCapture = true;
            }
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
            {
                m_modelPreviewLmbOrbitCapture = false;
            }
            if (!ImGui::IsMouseDown(ImGuiMouseButton_Middle))
            {
                m_modelPreviewMmbPanCapture = false;
            }

            if (m_modelPreviewLmbOrbitCapture && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
            {
                m_modelPreviewNavYawAccum -= io.MouseDelta.x * 0.35f;
                m_modelPreviewNavPitchAccum += io.MouseDelta.y * 0.35f;
            }
            if (m_modelPreviewMmbPanCapture && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
            {
                m_modelPreviewNavPanXAccum -= io.MouseDelta.x;
                m_modelPreviewNavPanYAccum -= io.MouseDelta.y;
            }
            if (hovered)
            {
                m_modelPreviewNavWheelAccum += io.MouseWheel;
            }
        };

        if (m_modelPreviewTexValid && m_modelPreviewTexId != ImTextureID_Invalid)
        {
            ImGui::Image(m_modelPreviewTexId, imgSize);
            applyPreviewInteractions(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem));
        }
        else
        {
            const ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::InvisibleButton("##preview_fallback", imgSize);
            applyPreviewInteractions(ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem));
            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddRectFilled(p, ImVec2(p.x + imgSize.x, p.y + imgSize.y), IM_COL32(24, 26, 34, 255));
            dl->AddRect(p, ImVec2(p.x + imgSize.x, p.y + imgSize.y), IM_COL32(60, 64, 78, 255));
            const char* hint = m_modelPreviewStatusText.empty() ? "Ожидание кадра превью…" : "Нет изображения";
            const ImVec2 ts = ImGui::CalcTextSize(hint);
            dl->AddText(ImVec2(p.x + (imgSize.x - ts.x) * 0.5f, p.y + (imgSize.y - ts.y) * 0.5f), IM_COL32(140, 145, 160, 255), hint);
        }
    }
    ImGui::End();

    if (wasOpen && !m_modelPreviewWindowOpen)
    {
        m_modelPreviewDisplayName.clear();
        m_modelPreviewTriangleCount = 0;
        m_modelPreviewVertexCount = 0;
        m_modelPreviewStatusText.clear();
        m_modelPreviewTexId = ImTextureID_Invalid;
        m_modelPreviewTexValid = false;
        m_modelPreviewLmbOrbitCapture = false;
        m_modelPreviewMmbPanCapture = false;
    }
}

void DebugUi::OpenTxdInspectorFromFile(const fs::path& absolutePath, std::string displayName)
{
    m_txdInspectDisplayName = std::move(displayName);
    m_txdInspectParse = d11::data::ParseTxdFile(absolutePath.string());
    m_txdInspectThumbnails.clear();
    m_txdInspectGpuDirty = true;
    m_txdInspectWindowOpen = true;
}

void DebugUi::OpenTxdInspectorFromImgEntry(const fs::path& archiveAbsolute, const d11::data::tImgArchiveEntry& entry)
{
    std::string title = archiveAbsolute.filename().string();
    title += " · ";
    title += entry.name;
    m_txdInspectDisplayName = std::move(title);
    m_txdInspectParse = d11::data::ParseTxdFromImgArchiveEntry(archiveAbsolute.string(), entry);
    m_txdInspectThumbnails.clear();
    m_txdInspectGpuDirty = true;
    m_txdInspectWindowOpen = true;
}

bool DebugUi::ConsumeTxdInspectGpuDirty()
{
    if (!m_txdInspectGpuDirty)
    {
        return false;
    }
    m_txdInspectGpuDirty = false;
    return true;
}

void DebugUi::AssignTxdInspectThumbnails(std::vector<ImTextureID>&& ids)
{
    m_txdInspectThumbnails = std::move(ids);
}

void DebugUi::ClearTxdInspectGpuThumbnails()
{
    m_txdInspectThumbnails.clear();
}

ImTextureID DebugUi::GetTxdInspectThumbnail(std::size_t index) const
{
    if (index >= m_txdInspectThumbnails.size())
    {
        return ImTextureID_Invalid;
    }
    return m_txdInspectThumbnails[index];
}

void DebugUi::DrawTxdInspectWindow()
{
    if (!m_txdInspectWindowOpen)
    {
        return;
    }

    constexpr float kTxdScrollH = 560.0f;
    constexpr ImGuiWindowFlags kTxdWinFlags =
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    const float closeBtn = ImGui::GetFrameHeight();
    const float headerH = closeBtn + ImGui::GetStyle().ItemSpacing.y + 4.0f;
    const bool parseOk = m_txdInspectParse.IsOk();
    const float lineH = ImGui::GetTextLineHeightWithSpacing();
    float metaH = 0.0f;
    float bodyH = 0.0f;
    if (parseOk)
    {
        metaH = lineH * 3.0f + 8.0f;
        if (!m_txdInspectParse.issues.empty())
        {
            metaH += lineH;
        }
        bodyH = metaH + kTxdScrollH;
    }
    else
    {
        bodyH = lineH + 6.0f;
    }

    ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 center = vp->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    const float winW = 720.0f + ImGui::GetStyle().WindowPadding.x * 2.0f;
    const float winH = headerH + bodyH + ImGui::GetStyle().WindowPadding.y * 2.0f;
    ImGui::SetNextWindowSize(ImVec2(winW, winH), ImGuiCond_Always);

    const bool wasTxdOpen = m_txdInspectWindowOpen;
    if (ImGui::Begin("##txd_inspect", &m_txdInspectWindowOpen, kTxdWinFlags))
    {
        ImGui::AlignTextToFramePadding();
        const char* titleName = m_txdInspectDisplayName.empty() ? "—.txd" : m_txdInspectDisplayName.c_str();
        ImGui::TextUnformatted(titleName);
        ImGui::SameLine();
        ImGui::TextDisabled(
            "(текстур: %u)",
            static_cast<unsigned>(m_txdInspectParse.textures.size()));
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - closeBtn);

        const ImVec2 closeMin = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##txd_inspect_close", ImVec2(closeBtn, closeBtn));
        const bool closeHover = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            m_txdInspectWindowOpen = false;
        }
        if (closeHover)
        {
            ImGui::SetTooltip("Закрыть");
        }
        const ImVec2 closeMax = ImVec2(closeMin.x + closeBtn, closeMin.y + closeBtn);
        ImDrawList* wdl = ImGui::GetWindowDrawList();
        if (closeHover)
        {
            wdl->AddRectFilled(closeMin, closeMax, IM_COL32(220, 75, 65, 85));
        }
        const ImU32 xCol = ImGui::GetColorU32(ImGuiCol_Text);
        const float inset = closeBtn * 0.30f;
        const float th = (std::max)(1.0f, ImGui::GetFontSize() * 0.09f);
        wdl->AddLine(
            ImVec2(closeMin.x + inset, closeMin.y + inset),
            ImVec2(closeMax.x - inset, closeMax.y - inset),
            xCol,
            th);
        wdl->AddLine(
            ImVec2(closeMax.x - inset, closeMin.y + inset),
            ImVec2(closeMin.x + inset, closeMax.y - inset),
            xCol,
            th);

        ImGui::Separator();

        if (!parseOk)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.45f, 0.35f, 1.0f), "%s", m_txdInspectParse.errorMessage.c_str());
        }
        else
        {
            const std::string deviceRu = TxdDictionaryDeviceRu(m_txdInspectParse.dictionary.deviceId);
            ImGui::TextDisabled(
                "Версия RW: 0x%05X · словарь: объявлено текстур %u · %s",
                static_cast<unsigned>(m_txdInspectParse.rwVersion),
                static_cast<unsigned>(m_txdInspectParse.dictionary.textureCount),
                deviceRu.c_str());
            ImGui::TextDisabled(
                "Поле device_id в файле: %u (внутренний код RenderWare; в интерфейсе выше — расшифровка).",
                static_cast<unsigned>(m_txdInspectParse.dictionary.deviceId));
            if (!m_txdInspectParse.issues.empty())
            {
                ImGui::TextDisabled("Замечаний при разборе: %u", static_cast<unsigned>(m_txdInspectParse.issues.size()));
            }

            ImGui::BeginChild("##txd_scroll", ImVec2(0.0f, kTxdScrollH), ImGuiChildFlags_Borders);
            for (std::size_t ri = 0; ri < m_txdInspectParse.textures.size(); ++ri)
            {
                const auto& tex = m_txdInspectParse.textures[ri];
                ImGui::PushID(static_cast<int>(ri));

                const ImVec2 base = ImGui::GetCursorScreenPos();
                const float fullW = ImGui::GetContentRegionAvail().x;
                const float spacing = ImGui::GetStyle().ItemSpacing.x;
                const float imgColW = std::floor(fullW * (1.0f / 3.0f));
                const float textW = (std::max)(1.0f, fullW - imgColW - spacing);
                const float textX = base.x + imgColW + spacing;

                const float tw = static_cast<float>((std::max)(1u, static_cast<unsigned>(tex.width)));
                const float thPx = static_cast<float>((std::max)(1u, static_cast<unsigned>(tex.height)));

                ImGui::SetCursorScreenPos(ImVec2(textX, base.y));
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + textW);
                ImGui::BeginGroup();
                ImGui::Text("Имя: %s", tex.name.empty() ? "—" : tex.name.c_str());
                if (!tex.mask.empty())
                {
                    ImGui::TextDisabled("Маска (альфа-канал): %s", tex.mask.c_str());
                }
                ImGui::Text("Размер: %u×%u пикс., глубина %u бит", static_cast<unsigned>(tex.width), static_cast<unsigned>(tex.height), static_cast<unsigned>(tex.depth));
                const std::string platRu = TxdPlatformNativeRu(tex.platformId);
                ImGui::TextWrapped("%s", platRu.c_str());
                const std::string fmtRu = TxdD3dFormatRu(tex.d3dFormat);
                ImGui::TextWrapped("Формат хранения: %s", fmtRu.c_str());
                ImGui::TextDisabled("Код формата в файле: 0x%08X", static_cast<unsigned>(tex.d3dFormat));
                ImGui::TextWrapped("%s", TxdMipLevelsRu(tex.mipLevelCount).c_str());
                {
                    char fl[96];
                    const char* fr = TxdFilterModeRu(tex.filterMode);
                    if (fr)
                    {
                        std::snprintf(fl, sizeof(fl), "Режим фильтрации: %s (код %u)", fr, static_cast<unsigned>(tex.filterMode));
                    }
                    else
                    {
                        std::snprintf(
                            fl,
                            sizeof(fl),
                            "Режим фильтрации: неизвестный код %u",
                            static_cast<unsigned>(tex.filterMode));
                    }
                    ImGui::TextWrapped("%s", fl);
                }
                {
                    char uv[96];
                    const char* ur = TxdUvAddressRu(tex.uvAddressing);
                    if (ur)
                    {
                        std::snprintf(uv, sizeof(uv), "Адресация UV: %s (код %u)", ur, static_cast<unsigned>(tex.uvAddressing));
                    }
                    else
                    {
                        std::snprintf(
                            uv,
                            sizeof(uv),
                            "Адресация UV: неизвестный код %u",
                            static_cast<unsigned>(tex.uvAddressing));
                    }
                    ImGui::TextWrapped("%s", uv);
                }
                if (tex.platformId == 8u)
                {
                    ImGui::TextDisabled(
                        "Байт платформы D3D8 после растра: %u (тип DXT: 1=DXT1, 3=DXT3, 4/5=DXT5)",
                        static_cast<unsigned>(tex.platformPropertiesByte));
                }
                else
                {
                    ImGui::TextDisabled(
                        "Флаги платформы (байт после растра): 0x%02X",
                        static_cast<unsigned>(tex.platformPropertiesByte));
                }
                ImGui::TextDisabled(
                    "RasterFormat 0x%08X · rasterType %u",
                    static_cast<unsigned>(tex.rasterFormatFlags),
                    static_cast<unsigned>(tex.rasterType));
                if (!tex.previewNoteRu.empty())
                {
                    ImGui::TextWrapped("Превью: %s", tex.previewNoteRu.c_str());
                }
                ImGui::EndGroup();
                ImGui::PopTextWrapPos();

                const float rowH = (std::max)(ImGui::GetItemRectMax().y - base.y, 1.0f);

                const float fitScale = (std::min)(imgColW / tw, rowH / thPx);
                const float iw = tw * fitScale;
                const float ih = thPx * fitScale;
                const float ix = base.x + (imgColW - iw) * 0.5f;
                const float iy = base.y + (rowH - ih) * 0.5f;

                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddRect(
                    ImVec2(base.x, base.y),
                    ImVec2(base.x + imgColW, base.y + rowH),
                    IM_COL32(55, 60, 75, 200));

                const ImTextureID tid = GetTxdInspectThumbnail(ri);
                if (tid != ImTextureID_Invalid)
                {
                    dl->AddImage(ImTextureRef(tid), ImVec2(ix, iy), ImVec2(ix + iw, iy + ih));
                }
                else
                {
                    dl->AddRectFilled(ImVec2(ix, iy), ImVec2(ix + iw, iy + ih), IM_COL32(24, 26, 34, 255));
                    dl->AddRect(ImVec2(ix, iy), ImVec2(ix + iw, iy + ih), IM_COL32(60, 64, 78, 255));
                    const char* hint = "Нет превью";
                    const ImVec2 ts = ImGui::CalcTextSize(hint);
                    dl->AddText(
                        ImVec2(ix + (iw - ts.x) * 0.5f, iy + (ih - ts.y) * 0.5f),
                        IM_COL32(140, 145, 160, 255),
                        hint);
                }

                ImGui::SetCursorScreenPos(ImVec2(base.x, base.y + rowH + ImGui::GetStyle().ItemSpacing.y));
                ImGui::Separator();
                ImGui::PopID();
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();

    if (wasTxdOpen && !m_txdInspectWindowOpen)
    {
        m_txdInspectDisplayName.clear();
        m_txdInspectParse = {};
        m_txdInspectThumbnails.clear();
        m_txdInspectGpuDirty = false;
    }
}

void DebugUi::DrawUi(d11::render::GtaStyleCamera& camera)
{
    if (ImGui::GetCurrentContext() == nullptr)
    {
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float dpiScale = (io.DisplayFramebufferScale.y > 0.0f) ? io.DisplayFramebufferScale.y : 1.0f;
    const float topBarHeight = kTopBarBaseHeight / dpiScale;
    const float topButtonHeight = kTopButtonBaseHeight / dpiScale;
    const float topBarPaddingY = kTopBarBasePaddingY / dpiScale;
    const float panelCornerRounding = kPanelBaseRounding / dpiScale;

    const float frameMs = m_lastFrameDt * 1000.0f;
    const float fps = (frameMs > 0.0f) ? (1000.0f / frameMs) : 0.0f;
    DrawTopBar(dpiScale, frameMs, fps, topBarHeight, topButtonHeight, topBarPaddingY);

    const float padX = 12.0f;
    const ImVec2 panelPos(viewport->Pos.x + padX, viewport->Pos.y + topBarHeight + kPanelPosOffsetY);
    DrawTreeWindow(panelPos, 360.0f, panelCornerRounding, kUiFontScale);
    DrawCameraWindow(panelPos, 380.0f, camera, kUiFontScale, panelCornerRounding);
    DrawPerformanceWindow(panelPos, 430.0f, frameMs, panelCornerRounding, kUiFontScale);
    DrawControlsWindow(panelPos, 360.0f, panelCornerRounding, kUiFontScale);
    DrawSceneWindow(panelPos, 360.0f, panelCornerRounding, kUiFontScale);
    DrawModelPreviewWindow();
    DrawTxdInspectWindow();
    DrawHoveredCubeTooltip();
}
