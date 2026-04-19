#include "Dx11RenderPipeline.h"

#include "../app/DebugUi.h"
#include "../game_sa/GameLoader.h"

#include <d3dcompiler.h>

#include <DirectXMath.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include <deque>
#include <mutex>
#include <thread>

namespace d11::render {
namespace {

// Сетка и оси XYZ в одной горизонтальной плоскости z = 0 (карта GTA: XY, вверх Z).
constexpr float kGridSize = 3000.0f;
constexpr float kGridStep = 15.0f;
constexpr float kGridPlaneZ = 0.0f;
constexpr float kAxesOriginZ = 0.0f;
// Половина толщины линии осей в пикселях (шейдер разворачивает ленту к камере — с гридом по визуальной толщине).
constexpr float kAxisRibbonHalfPx = 0.75f;

std::uint64_t NowSteadyUs()
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

struct AxesCB
{
    DirectX::XMFLOAT4X4 mvp{};
    DirectX::XMFLOAT3 eye{};
    float ribbonHalfPx{};
    float viewportW{};
    float viewportH{};
    float tanHalfFovY{};
    float pad0{};
};
static_assert(sizeof(AxesCB) == 96, "AxesCB must match HLSL cbuffer");
static_assert(sizeof(AxesCB) % 16 == 0, "cbuffer size");

// Оси: X красный, Y зелёный, Z синий (см. комментарий в AxesRenderer.cpp старого проекта).
constexpr float kAxisLength = 500.0f * 1.25f;

constexpr char kGridVs[] = R"(
cbuffer GridCB : register(b0)
{
    row_major float4x4 g_mvp;
};
struct VsIn
{
    float3 pos : POSITION;
};
float4 main(VsIn vin) : SV_Position
{
    return mul(float4(vin.pos, 1.0f), g_mvp);
}
)";

constexpr char kGridPs[] = R"(
float4 main() : SV_Target
{
    return float4(0.55f, 0.55f, 0.58f, 0.72f);
}
)";

constexpr char kAxesVs[] = R"(
cbuffer AxesCB : register(b0)
{
    row_major float4x4 g_mvp;
    float3 g_eye;
    float g_ribbonHalfPx;
    float g_viewportW;
    float g_viewportH;
    float g_tanHalfFovY;
    float g_pad0;
};
struct VsIn
{
    float3 pos : POSITION;
    float4 col : COLOR;
    float2 axisSide : TEXCOORD0;
};
struct VsOut
{
    float4 pos : SV_Position;
    float4 col : COLOR;
};
float3 AxisDir(float id)
{
    if (id < 0.5f)
    {
        return float3(1.0f, 0.0f, 0.0f);
    }
    if (id < 1.5f)
    {
        return float3(0.0f, 1.0f, 0.0f);
    }
    return float3(0.0f, 0.0f, 1.0f);
}
VsOut main(VsIn vin)
{
    const float3 lineDir = AxisDir(vin.axisSide.x);
    const float side = vin.axisSide.y;
    float3 world = vin.pos;
    const float3 vEye = g_eye - world;
    float dist = length(vEye);
    float3 viewDir = dist > 1e-5f ? (vEye / dist) : float3(0.0f, 0.0f, 1.0f);
    // Лента только в плоскости z=0 для X/Y. Нельзя делать cross(lineDir,viewDir) и обнулять z:
    // при взгляде почти горизонтально в XY cross даёт только Z и обнуляется — ось ломается.
    float3 binorm;
    const float ax = vin.axisSide.x;
    if (ax < 0.5f)
    {
        binorm = float3(0.0f, 1.0f, 0.0f);
    }
    else if (ax < 1.5f)
    {
        binorm = float3(-1.0f, 0.0f, 0.0f);
    }
    else
    {
        float2 vxy = viewDir.xy;
        const float len2 = dot(vxy, vxy);
        if (len2 < 1e-10f)
        {
            vxy = float2(1.0f, 0.0f);
        }
        else
        {
            vxy *= rsqrt(len2);
        }
        binorm = float3(-vxy.y, vxy.x, 0.0f);
    }
    if (dot(binorm, viewDir) > 0.0f)
    {
        binorm = -binorm;
    }
    const float vh = max(g_viewportH, 1.0f);
    const float halfW = g_ribbonHalfPx * dist * (2.0f * g_tanHalfFovY) / vh;
    world += side * halfW * binorm;
    VsOut o;
    o.pos = mul(float4(world, 1.0f), g_mvp);
    o.col = vin.col;
    return o;
}
)";

constexpr char kAxesPs[] = R"(
struct VsOut
{
    float4 pos : SV_Position;
    float4 col : COLOR;
};
float4 main(VsOut pin) : SV_Target
{
    return pin.col;
}
)";

constexpr char kPreviewVs[] = R"(
cbuffer PreviewCB : register(b0)
{
    row_major float4x4 g_mvp;
    row_major float4x4 g_world;
};
struct VsIn
{
    float3 pos : POSITION;
    float3 nrm : NORMAL;
    float2 uv : TEXCOORD0;
};
struct VsOut
{
    float4 pos : SV_Position;
    float3 wn : NORMAL;
    float2 uv : TEXCOORD0;
};
VsOut main(VsIn vin)
{
    VsOut o;
    o.pos = mul(float4(vin.pos, 1.0f), g_mvp);
    o.wn = mul(float4(vin.nrm, 0.0f), g_world).xyz;
    o.uv = vin.uv;
    return o;
}
)";

constexpr char kPreviewPs[] = R"(
Texture2D g_tex : register(t0);
SamplerState g_samp : register(s0);
struct VsOut
{
    float4 pos : SV_Position;
    float3 wn : NORMAL;
    float2 uv : TEXCOORD0;
};
float4 main(VsOut pin) : SV_Target
{
    float3 n = normalize(pin.wn);
    float3 L = normalize(float3(0.35f, 0.55f, 0.78f));
    float d = saturate(dot(n, L));
    float3 amb = float3(0.11f, 0.12f, 0.15f);
    float3 diff = float3(0.52f, 0.54f, 0.58f) * d;
    float3 base = g_tex.Sample(g_samp, pin.uv).rgb;
    return float4(base * (amb + diff), 1.0f);
}
)";

constexpr char kSceneInstVs[] = R"(
cbuffer SceneCB : register(b0)
{
    row_major float4x4 g_viewProj;
};
struct VsIn
{
    float3 pos : POSITION;
    float3 nrm : NORMAL;
};
struct InstanceGpu
{
    float4 row0;
    float4 row1;
    float4 row2;
    float4 row3;
    float4 color;
    float4 extra;
};
StructuredBuffer<InstanceGpu> g_inst : register(t0);
struct VsOut
{
    float4 pos : SV_Position;
    float3 wn : NORMAL;
    float4 color : COLOR;
    /**1 = рисовать инстанс, 0 = скрыт (clip только в PS — во VS недопустим). */
    float draw : TEXCOORD0;
};
VsOut main(VsIn vin, uint iid : SV_InstanceID)
{
    InstanceGpu I = g_inst[iid];
    row_major float4x4 world = float4x4(I.row0, I.row1, I.row2, I.row3);
    float4 wp = mul(float4(vin.pos, 1.0f), world);
    VsOut o;
    o.pos = mul(wp, g_viewProj);
    o.wn = mul(float4(vin.nrm, 0.0f), world).xyz;
    o.color = I.color;
    o.draw = I.extra.x;
    return o;
}
)";

constexpr char kSceneInstPs[] = R"(
struct VsOut
{
    float4 pos : SV_Position;
    float3 wn : NORMAL;
    float4 color : COLOR;
    float draw : TEXCOORD0;
};
float4 main(VsOut pin) : SV_Target
{
    clip(pin.draw - 0.5f);
    return pin.color;
}
)";

constexpr char kSceneDffMeshVs[] = R"(
cbuffer SceneDffCB : register(b0)
{
    row_major float4x4 g_mvp;
    float4 g_color;
    float4 g_pad;
};
struct VsIn
{
    float3 pos : POSITION;
};
struct VsOut
{
    float4 pos : SV_Position;
    float4 col : COLOR;
};
VsOut main(VsIn vin)
{
    VsOut o;
    o.pos = mul(float4(vin.pos, 1.0f), g_mvp);
    o.col = g_color;
    return o;
}
)";

/** Как в d11_sa_old_code Engine/DffMeshRenderer: цвет инстанса без затемнения (там PS — return pin.col). */
constexpr char kSceneDffMeshPs[] = R"(
struct VsOut
{
    float4 pos : SV_Position;
    float4 col : COLOR;
};
float4 main(VsOut pin) : SV_Target
{
    return pin.col;
}
)";

struct PreviewVertex
{
    float px, py, pz;
    float nx, ny, nz;
    float u, v;
};

struct PreviewCB
{
    DirectX::XMFLOAT4X4 mvp{};
    DirectX::XMFLOAT4X4 world{};
};
static_assert(sizeof(PreviewCB) == 128, "PreviewCB size");

std::string ToLowerAscii(std::string s)
{
    for (char& c : s)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
}

bool ImgEntryStemIsDff(const std::string& entryName, std::string_view stemLower)
{
    namespace fs = std::filesystem;
    const fs::path p(entryName);
    const std::string stem = ToLowerAscii(p.stem().string());
    const std::string ext = ToLowerAscii(p.extension().string());
    return ext == ".dff" && stem == stemLower;
}

std::string MakePathKeyNorm(const std::filesystem::path& p)
{
    std::string s = p.lexically_normal().string();
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}

struct SceneVertexInst
{
    DirectX::XMFLOAT3 pos {};
    DirectX::XMFLOAT3 nrm {};
};

struct SceneDffMeshCB
{
    DirectX::XMFLOAT4X4 mvp {};
    DirectX::XMFLOAT4 color {};
    DirectX::XMFLOAT4 pad {};
};
static_assert(sizeof(SceneDffMeshCB) == 96, "SceneDffMeshCB size");

struct IdeSceneMeta
{
    std::string modelName;
    std::string textureDictionary;
    float drawDistance[3] {};
    std::uint8_t drawDistanceCount = 0;
    std::int32_t flags = 0;
};

static void CollectIdeMapForScene(const GameLoader::ReadAccess& access, std::unordered_map<std::int32_t, IdeSceneMeta>& out)
{
    for (const auto& ide : access.ideResults)
    {
        for (const auto& entry : ide.entries)
        {
            std::visit(
                [&out](const auto& v)
                {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (
                        std::is_same_v<T, d11::data::tIdeBaseObjectEntry> || std::is_same_v<T, d11::data::tIdeTimedObjectEntry> ||
                        std::is_same_v<T, d11::data::tIdeAnimatedObjectEntry>)
                    {
                        IdeSceneMeta m {};
                        m.modelName = v.modelName;
                        m.textureDictionary = v.textureDictionary;
                        m.drawDistance[0] = v.drawDistance[0];
                        m.drawDistance[1] = v.drawDistance[1];
                        m.drawDistance[2] = v.drawDistance[2];
                        m.drawDistanceCount = v.drawDistanceCount;
                        m.flags = v.flags;
                        out.emplace(v.id, std::move(m));
                    }
                },
                entry);
        }
    }
}

static float MaxIdeDrawDistance(const IdeSceneMeta* meta)
{
    if (meta == nullptr)
    {
        return 300.0f;
    }
    float mx = 0.0f;
    if (meta->drawDistanceCount >= 1)
    {
        mx = (std::max)(mx, meta->drawDistance[0]);
    }
    if (meta->drawDistanceCount >= 2)
    {
        mx = (std::max)(mx, meta->drawDistance[1]);
    }
    if (meta->drawDistanceCount >= 3)
    {
        mx = (std::max)(mx, meta->drawDistance[2]);
    }
    if (mx <= 0.0f)
    {
        mx = 300.0f;
    }
    return mx;
}

static DirectX::XMFLOAT4 ColorFromObjectId(std::int32_t objectId)
{
    const std::uint32_t u = static_cast<std::uint32_t>(objectId) * 2654435761u;
    auto ch = [](std::uint32_t x, int sh) -> float
    {
        return (static_cast<float>((x >> sh) & 255u) / 255.0f) * 0.55f + 0.22f;
    };
    return DirectX::XMFLOAT4 {ch(u, 0), ch(u, 8), ch(u, 16), 1.0f};
}

static std::string InstModelStemLower(const d11::data::tIplInstEntry& inst, const IdeSceneMeta* ideMeta)
{
    namespace fs = std::filesystem;
    if (ideMeta != nullptr && !ideMeta->modelName.empty())
    {
        return ToLowerAscii(fs::path(ideMeta->modelName).stem().string());
    }
    if (!inst.modelName.empty())
    {
        return ToLowerAscii(fs::path(inst.modelName).stem().string());
    }
    return {};
}

static void AppendUnitCubeMesh(std::vector<SceneVertexInst>& verts, std::vector<std::uint16_t>& indices)
{
    using namespace DirectX;
    auto face = [&](XMFLOAT3 n, XMFLOAT3 v0, XMFLOAT3 v1, XMFLOAT3 v2, XMFLOAT3 v3)
    {
        const std::uint16_t base = static_cast<std::uint16_t>(verts.size());
        verts.push_back({v0, n});
        verts.push_back({v1, n});
        verts.push_back({v2, n});
        verts.push_back({v3, n});
        indices.push_back(static_cast<std::uint16_t>(base + 0));
        indices.push_back(static_cast<std::uint16_t>(base + 1));
        indices.push_back(static_cast<std::uint16_t>(base + 2));
        indices.push_back(static_cast<std::uint16_t>(base + 0));
        indices.push_back(static_cast<std::uint16_t>(base + 2));
        indices.push_back(static_cast<std::uint16_t>(base + 3));
    };
    face({1, 0, 0}, {1, -1, -1}, {1, 1, -1}, {1, 1, 1}, {1, -1, 1});
    face({-1, 0, 0}, {-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1}, {-1, -1, -1});
    face({0, 1, 0}, {-1, 1, -1}, {1, 1, -1}, {1, 1, 1}, {-1, 1, 1});
    face({0, -1, 0}, {-1, -1, 1}, {1, -1, 1}, {1, -1, -1}, {-1, -1, -1});
    face({0, 0, 1}, {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1});
    face({0, 0, -1}, {-1, 1, -1}, {1, 1, -1}, {1, -1, -1}, {-1, -1, -1});
}

static std::string ModelStemToDffEntryName(std::string_view modelStemLower)
{
    std::string name(modelStemLower.begin(), modelStemLower.end());
    name = ToLowerAscii(std::move(name));
    if (name.size() < 4 || name.compare(name.size() - 4, 4, ".dff") != 0)
    {
        name += ".dff";
    }
    return name;
}

static void MergeDffToSceneMesh(const d11::data::tDffParseResult& dff, std::vector<DirectX::XMFLOAT3>& verts, std::vector<std::uint32_t>& indices)
{
    verts.clear();
    indices.clear();
    std::unordered_set<std::int32_t> usedGeom;
    for (const auto& atomic : dff.atomics)
    {
        if (atomic.geometryIndex < 0 || atomic.geometryIndex >= static_cast<std::int32_t>(dff.geometries.size()))
        {
            continue;
        }
        if (!usedGeom.insert(atomic.geometryIndex).second)
        {
            continue;
        }
        const d11::data::tDffGeometry& g = dff.geometries[static_cast<std::size_t>(atomic.geometryIndex)];
        if (g.vertices.empty() || g.indices.empty())
        {
            continue;
        }
        const std::uint32_t base = static_cast<std::uint32_t>(verts.size());
        for (const auto& v : g.vertices)
        {
            verts.push_back(DirectX::XMFLOAT3 {v.x, v.y, v.z});
        }
        for (const std::uint32_t ix : g.indices)
        {
            if (ix >= g.vertices.size())
            {
                continue;
            }
            indices.push_back(base + ix);
        }
    }
    if (!verts.empty())
    {
        return;
    }
    for (const d11::data::tDffGeometry& g : dff.geometries)
    {
        if (g.vertices.empty() || g.indices.empty())
        {
            continue;
        }
        const std::uint32_t base = static_cast<std::uint32_t>(verts.size());
        for (const auto& v : g.vertices)
        {
            verts.push_back(DirectX::XMFLOAT3 {v.x, v.y, v.z});
        }
        for (const std::uint32_t ix : g.indices)
        {
            if (ix >= g.vertices.size())
            {
                continue;
            }
            indices.push_back(base + ix);
        }
    }
}

struct DffPreviewSource
{
    d11::data::tDffParseResult dff {};
    bool fromImg = false;
    std::string imgArchiveKey;
    d11::data::tImgArchiveEntry imgEntry {};
};

bool TryLoadDffForPreviewSource(const GameLoader& loader, const std::string& modelStemRaw, DffPreviewSource& out, std::string& err)
{
    out = {};
    const std::string stemLower = ToLowerAscii(modelStemRaw);
    const auto root = loader.GetGameRootPath();
    if (root.empty())
    {
        err = "Корень игры не задан (loader).";
        return false;
    }
    namespace fs = std::filesystem;
    const fs::path loose = root / "models" / (stemLower + ".dff");
    if (fs::exists(loose))
    {
        out.dff = d11::data::ParseDffFile(loose.string());
        if (!out.dff.IsOk())
        {
            err = out.dff.errorMessage.empty() ? "Ошибка разбора DFF" : out.dff.errorMessage;
            return false;
        }
        return true;
    }

    const auto access = loader.AcquireRead();
    for (const auto& kv : access.imgByAbsPath)
    {
        if (!kv.second || !kv.second->errorMessage.empty())
        {
            continue;
        }
        for (const auto& ent : kv.second->entries)
        {
            if (!ImgEntryStemIsDff(ent.name, stemLower))
            {
                continue;
            }
            out.dff = d11::data::ParseDffFromImgArchiveEntry(kv.first, ent);
            if (out.dff.IsOk() && !out.dff.geometries.empty())
            {
                out.fromImg = true;
                out.imgArchiveKey = kv.first;
                out.imgEntry = ent;
                return true;
            }
            err = out.dff.errorMessage.empty() ? "Не удалось разобрать DFF из IMG" : out.dff.errorMessage;
        }
    }
    err = "Файл не найден: models\\" + stemLower + ".dff и нет подходящей записи в IMG.";
    return false;
}

std::string NormalizeIdeTxdStem(std::string s)
{
    s = ToLowerAscii(std::move(s));
    if (s.size() > 4)
    {
        if (s.compare(s.size() - 4, 4, ".txd") == 0)
        {
            s.resize(s.size() - 4);
        }
    }
    return s;
}

std::string LookupIdeTextureDictionaryStem(const GameLoader::ReadAccess& access, const std::string& modelStemLower)
{
    for (const auto& ide : access.ideResults)
    {
        for (const auto& entry : ide.entries)
        {
            std::string dict;
            std::visit(
                [&](auto&& v)
                {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, d11::data::tIdeBaseObjectEntry> ||
                                  std::is_same_v<T, d11::data::tIdeTimedObjectEntry> ||
                                  std::is_same_v<T, d11::data::tIdeAnimatedObjectEntry>)
                    {
                        if (ToLowerAscii(v.modelName) == modelStemLower && !v.textureDictionary.empty())
                        {
                            dict = NormalizeIdeTxdStem(v.textureDictionary);
                        }
                    }
                },
                entry);
            if (!dict.empty())
            {
                return dict;
            }
        }
    }
    return {};
}

std::string ResolveTxdStemWithTxdp(const GameLoader::ReadAccess& access, const std::string& txdStemLower)
{
    std::string cur = txdStemLower;
    for (const auto& ide : access.ideResults)
    {
        for (const auto& entry : ide.entries)
        {
            const auto* txdp = std::get_if<d11::data::tIdeTextureParentEntry>(&entry);
            if (!txdp || txdp->textureDictionary.empty() || txdp->parentTextureDictionary.empty())
            {
                continue;
            }
            if (NormalizeIdeTxdStem(txdp->textureDictionary) == cur)
            {
                return NormalizeIdeTxdStem(txdp->parentTextureDictionary);
            }
        }
    }
    return cur;
}

bool ImgEntryNameEqualsLower(const std::string& entryName, const std::string& wantLower)
{
    std::string n = entryName;
    std::transform(n.begin(), n.end(), n.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return n == wantLower;
}

bool FindTxdInImgPreferKey(
    const GameLoader::ReadAccess& access,
    const std::string& preferImgKeyLower,
    const std::string& txdFileNameLower,
    std::string& outArchiveKey,
    d11::data::tImgArchiveEntry& outEntry)
{
    if (!preferImgKeyLower.empty())
    {
        auto it = access.imgByAbsPath.find(preferImgKeyLower);
        if (it != access.imgByAbsPath.end() && it->second && it->second->errorMessage.empty())
        {
            for (const auto& e : it->second->entries)
            {
                if (ImgEntryNameEqualsLower(e.name, txdFileNameLower))
                {
                    outArchiveKey = it->first;
                    outEntry = e;
                    return true;
                }
            }
        }
    }
    for (const auto& kv : access.imgByAbsPath)
    {
        if (!kv.second || !kv.second->errorMessage.empty())
        {
            continue;
        }
        for (const auto& e : kv.second->entries)
        {
            if (ImgEntryNameEqualsLower(e.name, txdFileNameLower))
            {
                outArchiveKey = kv.first;
                outEntry = e;
                return true;
            }
        }
    }
    return false;
}

const d11::data::tTxdTextureNativeInfo* FindTxdNativeByNameLower(const d11::data::tTxdParseResult& txd, const std::string& texLower)
{
    for (const auto& t : txd.textures)
    {
        std::string n = t.name;
        std::transform(n.begin(), n.end(), n.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        if (n == texLower)
        {
            return &t;
        }
    }
    return nullptr;
}

bool CreateImmutableRgbaTextureSrv(
    ID3D11Device* device,
    UINT width,
    UINT height,
    const std::uint8_t* rgba,
    ID3D11ShaderResourceView** outSrv)
{
    if (!device || !rgba || width == 0 || height == 0 || !outSrv)
    {
        return false;
    }
    *outSrv = nullptr;

    D3D11_TEXTURE2D_DESC td{};
    td.Width = width;
    td.Height = height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA srd{};
    srd.pSysMem = rgba;
    srd.SysMemPitch = UINT(width * 4u);

    ID3D11Texture2D* tex2d = nullptr;
    if (FAILED(device->CreateTexture2D(&td, &srd, &tex2d)) || !tex2d)
    {
        return false;
    }
    const HRESULT hr = device->CreateShaderResourceView(tex2d, nullptr, outSrv);
    tex2d->Release();
    return SUCCEEDED(hr) && *outSrv != nullptr;
}

struct PreviewDrawRange
{
    ID3D11ShaderResourceView* srv = nullptr;
    UINT start_vertex = 0;
    UINT vertex_count = 0;
};

bool BuildTexturedPreviewGeometry(
    const GameLoader& loader,
    const DffPreviewSource& src,
    const std::string& modelStemForIdeLower,
    ID3D11Device* device,
    ID3D11ShaderResourceView* whiteSrv,
    std::unordered_map<std::string, d11::data::tTxdParseResult>& txdParseCache,
    std::unordered_map<std::string, ID3D11ShaderResourceView*>& texSrvCache,
    std::vector<PreviewVertex>& verts,
    std::vector<PreviewDrawRange>& batches,
    DirectX::XMFLOAT3& outCenter,
    float& outRadius,
    std::uint32_t& outTriCount,
    std::string& err)
{
    using namespace DirectX;
    err.clear();
    verts.clear();
    batches.clear();
    outTriCount = 0;

    if (!device || !whiteSrv)
    {
        err = "Нет устройства D3D или белой текстуры превью.";
        return false;
    }

    namespace fs = std::filesystem;
    const auto access = loader.AcquireRead();
    std::string ideTxdStem = LookupIdeTextureDictionaryStem(access, modelStemForIdeLower);
    if (ideTxdStem.empty())
    {
        ideTxdStem = modelStemForIdeLower;
    }
    ideTxdStem = ResolveTxdStemWithTxdp(access, ideTxdStem);

    std::string txdFileLower = ideTxdStem + ".txd";
    std::transform(txdFileLower.begin(), txdFileLower.end(), txdFileLower.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    std::string txdParseKey;
    bool txdIsImg = false;
    std::string txdImgArchKey;
    d11::data::tImgArchiveEntry txdImgEnt {};
    std::string txdLoosePath;

    const std::string preferImg = src.fromImg ? src.imgArchiveKey : std::string {};
    if (FindTxdInImgPreferKey(access, preferImg, txdFileLower, txdImgArchKey, txdImgEnt))
    {
        std::string entLower = txdImgEnt.name;
        std::transform(entLower.begin(), entLower.end(), entLower.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        txdParseKey = txdImgArchKey + "!" + entLower;
        txdIsImg = true;
    }
    else
    {
        const fs::path root = loader.GetGameRootPath();
        if (!root.empty())
        {
            const fs::path loose = root / "models" / (ideTxdStem + ".txd");
            if (fs::exists(loose))
            {
                txdLoosePath = loose.lexically_normal().string();
                txdParseKey = MakePathKeyNorm(loose);
            }
        }
    }

    auto getOrParseTxd = [&]() -> d11::data::tTxdParseResult* {
        if (txdParseKey.empty())
        {
            return nullptr;
        }
        auto it = txdParseCache.find(txdParseKey);
        if (it != txdParseCache.end())
        {
            return &it->second;
        }
        d11::data::tTxdParseResult parsed;
        if (txdIsImg)
        {
            parsed = d11::data::ParseTxdFromImgArchiveEntry(txdImgArchKey, txdImgEnt);
        }
        else
        {
            parsed = d11::data::ParseTxdFile(txdLoosePath);
        }
        auto ins = txdParseCache.emplace(txdParseKey, std::move(parsed));
        return &ins.first->second;
    };

    auto resolveSrv = [&](const std::string& texNameLower) -> ID3D11ShaderResourceView* {
        if (texNameLower.empty() || txdParseKey.empty())
        {
            return whiteSrv;
        }
        const std::string srvKey = txdParseKey + "|" + texNameLower;
        auto itS = texSrvCache.find(srvKey);
        if (itS != texSrvCache.end())
        {
            return itS->second;
        }
        d11::data::tTxdParseResult* txd = getOrParseTxd();
        if (!txd || !txd->IsOk())
        {
            texSrvCache[srvKey] = whiteSrv;
            return whiteSrv;
        }
        const d11::data::tTxdTextureNativeInfo* nat = FindTxdNativeByNameLower(*txd, texNameLower);
        if (!nat || nat->width == 0 || nat->height == 0 || nat->previewRgba.empty())
        {
            texSrvCache[srvKey] = whiteSrv;
            return whiteSrv;
        }
        const UINT w = static_cast<UINT>(nat->width);
        const UINT h = static_cast<UINT>(nat->height);
        if (nat->previewRgba.size() != static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u)
        {
            texSrvCache[srvKey] = whiteSrv;
            return whiteSrv;
        }
        ID3D11ShaderResourceView* srv = nullptr;
        if (!CreateImmutableRgbaTextureSrv(device, w, h, nat->previewRgba.data(), &srv) || !srv)
        {
            texSrvCache[srvKey] = whiteSrv;
            return whiteSrv;
        }
        texSrvCache[srvKey] = srv;
        return srv;
    };

    ID3D11ShaderResourceView* curSrv = whiteSrv;
    std::size_t batchStart = 0;

    auto flushBatch = [&](ID3D11ShaderResourceView* srv, std::size_t start, std::size_t endExclusive) {
        if (endExclusive <= start)
        {
            return;
        }
        PreviewDrawRange r {};
        r.srv = srv ? srv : whiteSrv;
        r.start_vertex = static_cast<UINT>(start);
        r.vertex_count = static_cast<UINT>(endExclusive - start);
        batches.push_back(r);
    };

    for (const auto& g : src.dff.geometries)
    {
        if (g.vertices.empty() || g.indices.empty() || (g.indices.size() % 3) != 0)
        {
            continue;
        }
        const auto& V = g.vertices;
        const std::size_t triCount = g.indices.size() / 3u;
        for (std::size_t triIdx = 0; triIdx < triCount; ++triIdx)
        {
            std::uint16_t mid = 0;
            if (!g.triangleMaterialIds.empty() && triIdx < g.triangleMaterialIds.size())
            {
                mid = g.triangleMaterialIds[triIdx];
            }
            std::string texLower;
            if (mid < g.materials.size())
            {
                const auto& mat = g.materials[mid];
                if (mat.isTextured && mat.texture.has_value() && !mat.texture->textureName.empty())
                {
                    texLower = mat.texture->textureName;
                    std::transform(texLower.begin(), texLower.end(), texLower.begin(), [](unsigned char ch) {
                        return static_cast<char>(std::tolower(ch));
                    });
                }
            }

            ID3D11ShaderResourceView* triSrv = resolveSrv(texLower);
            if (triSrv != curSrv)
            {
                flushBatch(curSrv, batchStart, verts.size());
                batchStart = verts.size();
                curSrv = triSrv;
            }

            const UINT ia = g.indices[triIdx * 3u];
            const UINT ib = g.indices[triIdx * 3u + 1u];
            const UINT ic = g.indices[triIdx * 3u + 2u];
            if (ia >= V.size() || ib >= V.size() || ic >= V.size())
            {
                continue;
            }
            const XMVECTOR p0 = XMVectorSet(V[ia].x, V[ia].y, V[ia].z, 1.0f);
            const XMVECTOR p1 = XMVectorSet(V[ib].x, V[ib].y, V[ib].z, 1.0f);
            const XMVECTOR p2 = XMVectorSet(V[ic].x, V[ic].y, V[ic].z, 1.0f);
            const XMVECTOR e1 = XMVectorSubtract(p1, p0);
            const XMVECTOR e2 = XMVectorSubtract(p2, p0);
            XMVECTOR cn = XMVector3Normalize(XMVector3Cross(e1, e2));
            XMFLOAT3 fn{};
            XMStoreFloat3(&fn, cn);

            auto pushVtx = [&](UINT idx) {
                const auto& v = V[idx];
                verts.push_back(PreviewVertex {v.x, v.y, v.z, fn.x, fn.y, fn.z, v.u, v.v});
            };
            pushVtx(ia);
            pushVtx(ib);
            pushVtx(ic);
            ++outTriCount;
        }
    }

    flushBatch(curSrv, batchStart, verts.size());

    if (verts.empty())
    {
        err = "В DFF нет треугольников для отображения.";
        return false;
    }

    XMVECTOR mn = XMVectorReplicate(1e30f);
    XMVECTOR mx = XMVectorReplicate(-1e30f);
    for (const auto& v : verts)
    {
        const XMVECTOR p = XMVectorSet(v.px, v.py, v.pz, 0.0f);
        mn = XMVectorMin(mn, p);
        mx = XMVectorMax(mx, p);
    }
    XMVECTOR centerVec = XMVectorScale(XMVectorAdd(mn, mx), 0.5f);
    XMVECTOR ext = XMVectorSubtract(mx, mn);
    const float radius = XMVectorGetX(XMVector3Length(ext)) * 0.5f;
    XMStoreFloat3(&outCenter, centerVec);
    outRadius = (std::max)(radius, 0.01f);
    return true;
}

void BuildPreviewMesh(
    const d11::data::tDffParseResult& dff,
    std::vector<PreviewVertex>& verts,
    DirectX::XMFLOAT3& outCenter,
    float& outRadius,
    std::uint32_t& outTriCount,
    std::string& err)
{
    using namespace DirectX;
    verts.clear();
    outTriCount = 0;
    for (const auto& g : dff.geometries)
    {
        if (g.vertices.empty() || g.indices.empty() || (g.indices.size() % 3) != 0)
        {
            continue;
        }
        const auto& V = g.vertices;
        for (size_t i = 0; i + 2 < g.indices.size(); i += 3)
        {
            const UINT a = g.indices[i];
            const UINT b = g.indices[i + 1];
            const UINT c = g.indices[i + 2];
            if (a >= V.size() || b >= V.size() || c >= V.size())
            {
                continue;
            }
            const XMVECTOR p0 = XMVectorSet(V[a].x, V[a].y, V[a].z, 1.0f);
            const XMVECTOR p1 = XMVectorSet(V[b].x, V[b].y, V[b].z, 1.0f);
            const XMVECTOR p2 = XMVectorSet(V[c].x, V[c].y, V[c].z, 1.0f);
            const XMVECTOR e1 = XMVectorSubtract(p1, p0);
            const XMVECTOR e2 = XMVectorSubtract(p2, p0);
            XMVECTOR cn = XMVector3Normalize(XMVector3Cross(e1, e2));
            XMFLOAT3 fn{};
            XMStoreFloat3(&fn, cn);
            auto pushVtx = [&](UINT idx) {
                const auto& v = V[idx];
                verts.push_back(PreviewVertex {v.x, v.y, v.z, fn.x, fn.y, fn.z, v.u, v.v});
            };
            pushVtx(a);
            pushVtx(b);
            pushVtx(c);
            ++outTriCount;
        }
    }
    if (verts.empty())
    {
        err = "В DFF нет треугольников для отображения.";
        return;
    }
    XMVECTOR mn = XMVectorReplicate(1e30f);
    XMVECTOR mx = XMVectorReplicate(-1e30f);
    for (const auto& v : verts)
    {
        const XMVECTOR p = XMVectorSet(v.px, v.py, v.pz, 0.0f);
        mn = XMVectorMin(mn, p);
        mx = XMVectorMax(mx, p);
    }
    const XMVECTOR ctr = XMVectorScale(XMVectorAdd(mn, mx), 0.5f);
    XMStoreFloat3(&outCenter, ctr);
    float r2 = 0.0f;
    for (const auto& v : verts)
    {
        const XMVECTOR p = XMVectorSet(v.px, v.py, v.pz, 0.0f);
        const float d = XMVectorGetX(XMVector3LengthSq(XMVectorSubtract(p, ctr)));
        r2 = (std::max)(r2, d);
    }
    outRadius = std::sqrt(r2);
    if (outRadius < 1e-4f)
    {
        outRadius = 1.0f;
    }
}

} // namespace

static ImTextureID SrvToImTextureId(ID3D11ShaderResourceView* srv)
{
    return static_cast<ImTextureID>(reinterpret_cast<std::uintptr_t>(srv));
}

Dx11RenderPipeline::~Dx11RenderPipeline()
{
    Shutdown();
}

bool Dx11RenderPipeline::compile_blob(const char* src, const char* entry, const char* target, ID3DBlob** out_blob, UINT compile_flags)
{
    ID3DBlob* errors = nullptr;
    const HRESULT hr = D3DCompile(
        src,
        std::strlen(src),
        nullptr,
        nullptr,
        nullptr,
        entry,
        target,
        compile_flags,
        0,
        out_blob,
        &errors
    );
    if (FAILED(hr))
    {
        if (errors != nullptr && errors->GetBufferPointer() != nullptr)
        {
            std::printf(
                "[SceneImport] D3DCompile failed entry=%s target=%s hr=0x%08lx\n%s\n",
                entry,
                target,
                static_cast<unsigned long>(hr),
                static_cast<const char*>(errors->GetBufferPointer()));
            std::fflush(stdout);
        }
        else
        {
            std::printf(
                "[SceneImport] D3DCompile failed entry=%s target=%s hr=0x%08lx (no error blob)\n",
                entry,
                target,
                static_cast<unsigned long>(hr));
            std::fflush(stdout);
        }
        if (errors != nullptr)
        {
            errors->Release();
        }
        if (out_blob != nullptr && *out_blob != nullptr)
        {
            (*out_blob)->Release();
            *out_blob = nullptr;
        }
        return false;
    }
    if (errors != nullptr)
    {
        errors->Release();
    }
    return true;
}

bool Dx11RenderPipeline::create_depth(ID3D11Device* device, std::uint32_t width, std::uint32_t height)
{
    if (m_depth_dsv)
    {
        m_depth_dsv->Release();
        m_depth_dsv = nullptr;
    }
    if (m_depth_tex)
    {
        m_depth_tex->Release();
        m_depth_tex = nullptr;
    }

    D3D11_TEXTURE2D_DESC td{};
    td.Width = width;
    td.Height = height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    td.SampleDesc.Count = m_msaa_samples;
    td.SampleDesc.Quality = 0;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    if (FAILED(device->CreateTexture2D(&td, nullptr, &m_depth_tex)))
    {
        return false;
    }
    return SUCCEEDED(device->CreateDepthStencilView(m_depth_tex, nullptr, &m_depth_dsv));
}

bool Dx11RenderPipeline::create_grid_and_axes(ID3D11Device* device)
{
    if (m_vb_grid)
    {
        m_vb_grid->Release();
        m_vb_grid = nullptr;
    }
    if (m_vb_axes)
    {
        m_vb_axes->Release();
        m_vb_axes = nullptr;
    }

    std::vector<float> grid_verts;
    grid_verts.reserve(8000);
    for (float x = -kGridSize; x <= kGridSize + 0.1f; x += kGridStep)
    {
        grid_verts.push_back(x);
        grid_verts.push_back(-kGridSize);
        grid_verts.push_back(kGridPlaneZ);
        grid_verts.push_back(x);
        grid_verts.push_back(kGridSize);
        grid_verts.push_back(kGridPlaneZ);
    }
    for (float y = -kGridSize; y <= kGridSize + 0.1f; y += kGridStep)
    {
        grid_verts.push_back(-kGridSize);
        grid_verts.push_back(y);
        grid_verts.push_back(kGridPlaneZ);
        grid_verts.push_back(kGridSize);
        grid_verts.push_back(y);
        grid_verts.push_back(kGridPlaneZ);
    }

    m_grid_vertex_count = static_cast<std::uint32_t>(grid_verts.size() / 3);

    D3D11_BUFFER_DESC vbd{};
    vbd.ByteWidth = static_cast<UINT>(grid_verts.size() * sizeof(float));
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vsd{};
    vsd.pSysMem = grid_verts.data();
    if (FAILED(device->CreateBuffer(&vbd, &vsd, &m_vb_grid)))
    {
        return false;
    }

    const float L = kAxisLength;
    const float z0 = kAxesOriginZ;

    auto push_axis_vert = [](std::vector<float>& o, float px, float py, float pz, float cr, float cg, float cb, float axisId, float side)
    {
        o.insert(o.end(), {px, py, pz, cr, cg, cb, 1.0f, axisId, side});
    };

    auto push_axis_segment = [&push_axis_vert](std::vector<float>& o,
        float ax,
        float ay,
        float az,
        float bx,
        float by,
        float bz,
        float cr,
        float cg,
        float cb,
        float axisId)
    {
        push_axis_vert(o, ax, ay, az, cr, cg, cb, axisId, -1.0f);
        push_axis_vert(o, ax, ay, az, cr, cg, cb, axisId, 1.0f);
        push_axis_vert(o, bx, by, bz, cr, cg, cb, axisId, -1.0f);
        push_axis_vert(o, ax, ay, az, cr, cg, cb, axisId, 1.0f);
        push_axis_vert(o, bx, by, bz, cr, cg, cb, axisId, 1.0f);
        push_axis_vert(o, bx, by, bz, cr, cg, cb, axisId, -1.0f);
    };

    std::vector<float> axes_verts;
    axes_verts.reserve(9u * 6u * 3u);

    push_axis_segment(axes_verts, 0.0f, 0.0f, z0, L, 0.0f, z0, 1.0f, 0.0f, 0.0f, 0.0f);
    push_axis_segment(axes_verts, 0.0f, 0.0f, z0, 0.0f, L, z0, 0.0f, 1.0f, 0.0f, 1.0f);
    push_axis_segment(axes_verts, 0.0f, 0.0f, z0, 0.0f, 0.0f, z0 + L, 0.0f, 0.0f, 1.0f, 2.0f);

    m_axes_vertex_count = static_cast<std::uint32_t>(axes_verts.size() / 9);

    vbd.ByteWidth = static_cast<UINT>(axes_verts.size() * sizeof(float));
    vsd.pSysMem = axes_verts.data();
    return SUCCEEDED(device->CreateBuffer(&vbd, &vsd, &m_vb_axes));
}

bool Dx11RenderPipeline::create_line_pipeline(ID3D11Device* device)
{
    ID3DBlob* blob_grid_vs = nullptr;
    ID3DBlob* blob_grid_ps = nullptr;
    ID3DBlob* blob_axes_vs = nullptr;
    ID3DBlob* blob_axes_ps = nullptr;

    if (!compile_blob(kGridVs, "main", "vs_5_0", &blob_grid_vs) || !compile_blob(kGridPs, "main", "ps_5_0", &blob_grid_ps) ||
        !compile_blob(kAxesVs, "main", "vs_5_0", &blob_axes_vs) || !compile_blob(kAxesPs, "main", "ps_5_0", &blob_axes_ps))
    {
        if (blob_grid_vs)
        {
            blob_grid_vs->Release();
        }
        if (blob_grid_ps)
        {
            blob_grid_ps->Release();
        }
        if (blob_axes_vs)
        {
            blob_axes_vs->Release();
        }
        if (blob_axes_ps)
        {
            blob_axes_ps->Release();
        }
        return false;
    }

    const D3D11_INPUT_ELEMENT_DESC grid_layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    HRESULT hr = device->CreateInputLayout(
        grid_layout,
        static_cast<UINT>(std::size(grid_layout)),
        blob_grid_vs->GetBufferPointer(),
        blob_grid_vs->GetBufferSize(),
        &m_il_grid
    );
    if (FAILED(hr))
    {
        blob_grid_vs->Release();
        blob_grid_ps->Release();
        blob_axes_vs->Release();
        blob_axes_ps->Release();
        return false;
    }

    hr = device->CreateVertexShader(blob_grid_vs->GetBufferPointer(), blob_grid_vs->GetBufferSize(), nullptr, &m_vs_grid);
    if (FAILED(hr))
    {
        blob_grid_vs->Release();
        blob_grid_ps->Release();
        blob_axes_vs->Release();
        blob_axes_ps->Release();
        return false;
    }

    hr = device->CreatePixelShader(blob_grid_ps->GetBufferPointer(), blob_grid_ps->GetBufferSize(), nullptr, &m_ps_grid);
    if (FAILED(hr))
    {
        blob_grid_vs->Release();
        blob_grid_ps->Release();
        blob_axes_vs->Release();
        blob_axes_ps->Release();
        return false;
    }

    const D3D11_INPUT_ELEMENT_DESC axes_layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    hr = device->CreateInputLayout(
        axes_layout,
        static_cast<UINT>(std::size(axes_layout)),
        blob_axes_vs->GetBufferPointer(),
        blob_axes_vs->GetBufferSize(),
        &m_il_axes
    );
    if (FAILED(hr))
    {
        blob_grid_vs->Release();
        blob_grid_ps->Release();
        blob_axes_vs->Release();
        blob_axes_ps->Release();
        return false;
    }

    hr = device->CreateVertexShader(blob_axes_vs->GetBufferPointer(), blob_axes_vs->GetBufferSize(), nullptr, &m_vs_axes);
    if (FAILED(hr))
    {
        blob_grid_vs->Release();
        blob_grid_ps->Release();
        blob_axes_vs->Release();
        blob_axes_ps->Release();
        return false;
    }

    hr = device->CreatePixelShader(blob_axes_ps->GetBufferPointer(), blob_axes_ps->GetBufferSize(), nullptr, &m_ps_axes);

    blob_grid_vs->Release();
    blob_grid_ps->Release();
    blob_axes_vs->Release();
    blob_axes_ps->Release();

    if (FAILED(hr))
    {
        return false;
    }

    D3D11_BUFFER_DESC cbd{};
    cbd.ByteWidth = sizeof(DirectX::XMFLOAT4X4);
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device->CreateBuffer(&cbd, nullptr, &m_cb_mvp)))
    {
        return false;
    }

    cbd.ByteWidth = sizeof(AxesCB);
    if (FAILED(device->CreateBuffer(&cbd, nullptr, &m_cb_axes)))
    {
        return false;
    }

    D3D11_DEPTH_STENCIL_DESC dsd{};
    dsd.DepthEnable = TRUE;
    dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsd.DepthFunc = D3D11_COMPARISON_LESS;
    if (FAILED(device->CreateDepthStencilState(&dsd, &m_depth_state)))
    {
        return false;
    }

    D3D11_BLEND_DESC blend_alpha{};
    blend_alpha.RenderTarget[0].BlendEnable = TRUE;
    blend_alpha.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blend_alpha.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blend_alpha.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blend_alpha.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blend_alpha.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blend_alpha.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blend_alpha.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(device->CreateBlendState(&blend_alpha, &m_blend_alpha)))
    {
        return false;
    }

    D3D11_BLEND_DESC blend_opaque{};
    blend_opaque.RenderTarget[0].BlendEnable = FALSE;
    blend_opaque.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (FAILED(device->CreateBlendState(&blend_opaque, &m_blend_opaque)))
    {
        return false;
    }

    D3D11_RASTERIZER_DESC rs{};
    rs.FillMode = D3D11_FILL_SOLID;
    rs.CullMode = D3D11_CULL_NONE;
    rs.DepthClipEnable = TRUE;
    rs.MultisampleEnable = (m_msaa_samples > 1u) ? TRUE : FALSE;
    if (FAILED(device->CreateRasterizerState(&rs, &m_rs_lines)))
    {
        return false;
    }

    D3D11_RASTERIZER_DESC rs_axes{};
    rs_axes.FillMode = D3D11_FILL_SOLID;
    rs_axes.CullMode = D3D11_CULL_NONE;
    rs_axes.DepthClipEnable = TRUE;
    rs_axes.MultisampleEnable = (m_msaa_samples > 1u) ? TRUE : FALSE;
    rs_axes.DepthBias = -4;
    rs_axes.DepthBiasClamp = 0.0f;
    rs_axes.SlopeScaledDepthBias = -0.35f;
    if (FAILED(device->CreateRasterizerState(&rs_axes, &m_rs_axes)))
    {
        m_rs_lines->Release();
        m_rs_lines = nullptr;
        return false;
    }
    return true;
}

void Dx11RenderPipeline::map_mvp(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& mvp) const
{
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(context->Map(m_cb_mvp, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        std::memcpy(mapped.pData, &mvp, sizeof(mvp));
        context->Unmap(m_cb_mvp, 0);
    }
}

void Dx11RenderPipeline::map_axes_cb(
    ID3D11DeviceContext* context,
    const GtaStyleCamera& camera,
    float aspect,
    std::uint32_t viewport_w,
    std::uint32_t viewport_h) const
{
    if (m_cb_axes == nullptr)
    {
        return;
    }
    AxesCB cb{};
    DirectX::XMStoreFloat4x4(&cb.mvp, camera.WorldViewProjection(aspect));
    cb.eye = DirectX::XMFLOAT3 {camera.GetX(), camera.GetY(), camera.GetZ()};
    cb.ribbonHalfPx = kAxisRibbonHalfPx;
    cb.viewportW = static_cast<float>((viewport_w > 0u) ? viewport_w : 1u);
    cb.viewportH = static_cast<float>((viewport_h > 0u) ? viewport_h : 1u);
    const float fov_rad = DirectX::XMConvertToRadians(camera.GetFovDegrees());
    cb.tanHalfFovY = std::tan(fov_rad * 0.5f);
    cb.pad0 = 0.0f;

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(context->Map(m_cb_axes, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        std::memcpy(mapped.pData, &cb, sizeof(cb));
        context->Unmap(m_cb_axes, 0);
    }
}

bool Dx11RenderPipeline::Initialize(ID3D11Device* device, std::uint32_t width, std::uint32_t height, std::uint32_t msaa_sample_count)
{
    Shutdown();
    m_device = device;
    m_width = width;
    m_height = height;
    m_msaa_samples = (msaa_sample_count == 0u) ? 1u : msaa_sample_count;

    if (!create_depth(device, width, height) || !create_grid_and_axes(device) || !create_line_pipeline(device))
    {
        Shutdown();
        return false;
    }
    return true;
}

void Dx11RenderPipeline::release_model_preview_texture_resources()
{
    for (auto& kv : m_preview_texture_srv_by_key)
    {
        if (kv.second != nullptr && kv.second != m_preview_white_texture_srv)
        {
            kv.second->Release();
        }
    }
    m_preview_texture_srv_by_key.clear();
    m_preview_txd_parse_by_key.clear();
    if (m_preview_white_texture_srv != nullptr)
    {
        m_preview_white_texture_srv->Release();
        m_preview_white_texture_srv = nullptr;
    }
    if (m_preview_sampler != nullptr)
    {
        m_preview_sampler->Release();
        m_preview_sampler = nullptr;
    }
    m_preview_draw_batches.clear();
}

bool Dx11RenderPipeline::ensure_model_preview_sampler_and_white_tex(ID3D11Device* device)
{
    if (m_preview_sampler != nullptr && m_preview_white_texture_srv != nullptr)
    {
        return true;
    }
    if (m_preview_sampler == nullptr)
    {
        D3D11_SAMPLER_DESC sd{};
        sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        sd.MaxAnisotropy = 1;
        sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
        sd.MinLOD = 0;
        sd.MaxLOD = D3D11_FLOAT32_MAX;
        if (FAILED(device->CreateSamplerState(&sd, &m_preview_sampler)) || m_preview_sampler == nullptr)
        {
            return false;
        }
    }
    if (m_preview_white_texture_srv == nullptr)
    {
        static const std::uint8_t px[4] = {255, 255, 255, 255};
        if (!CreateImmutableRgbaTextureSrv(device, 1, 1, px, &m_preview_white_texture_srv) || m_preview_white_texture_srv == nullptr)
        {
            return false;
        }
    }
    return true;
}

void Dx11RenderPipeline::release_model_preview()
{
    m_preview_draw_batches.clear();
    if (m_preview_srv)
    {
        m_preview_srv->Release();
        m_preview_srv = nullptr;
    }
    if (m_preview_rtv)
    {
        m_preview_rtv->Release();
        m_preview_rtv = nullptr;
    }
    if (m_preview_color)
    {
        m_preview_color->Release();
        m_preview_color = nullptr;
    }
    if (m_preview_dsv)
    {
        m_preview_dsv->Release();
        m_preview_dsv = nullptr;
    }
    if (m_preview_depth)
    {
        m_preview_depth->Release();
        m_preview_depth = nullptr;
    }
    if (m_preview_vb)
    {
        m_preview_vb->Release();
        m_preview_vb = nullptr;
    }
    m_preview_vertex_count = 0;
    m_preview_tri_count = 0;
    m_preview_mesh_name.clear();
    m_preview_load_token = 0;
}

void Dx11RenderPipeline::release_model_preview_gpu_all()
{
    release_model_preview_texture_resources();
    release_model_preview();
    if (m_rs_preview)
    {
        m_rs_preview->Release();
        m_rs_preview = nullptr;
    }
    if (m_il_preview)
    {
        m_il_preview->Release();
        m_il_preview = nullptr;
    }
    if (m_vs_preview)
    {
        m_vs_preview->Release();
        m_vs_preview = nullptr;
    }
    if (m_ps_preview)
    {
        m_ps_preview->Release();
        m_ps_preview = nullptr;
    }
    if (m_cb_preview)
    {
        m_cb_preview->Release();
        m_cb_preview = nullptr;
    }
}

void Dx11RenderPipeline::release_txd_inspect_thumbnails()
{
    for (ID3D11ShaderResourceView* srv : m_txd_inspect_srvs)
    {
        if (srv != nullptr)
        {
            srv->Release();
        }
    }
    m_txd_inspect_srvs.clear();
}

void Dx11RenderPipeline::UpdateTxdInspectThumbnails(DebugUi& debug_ui)
{
    if (m_device == nullptr)
    {
        return;
    }

    if (!debug_ui.IsTxdInspectWindowOpen())
    {
        release_txd_inspect_thumbnails();
        debug_ui.ClearTxdInspectGpuThumbnails();
        return;
    }

    if (!debug_ui.ConsumeTxdInspectGpuDirty())
    {
        return;
    }

    release_txd_inspect_thumbnails();

    const d11::data::tTxdParseResult& parse = debug_ui.GetTxdInspectParse();
    std::vector<ImTextureID> ids;
    ids.reserve(parse.textures.size());
    m_txd_inspect_srvs.reserve(parse.textures.size());

    for (const d11::data::tTxdTextureNativeInfo& tex : parse.textures)
    {
        if (tex.width == 0 || tex.height == 0 || tex.previewRgba.empty())
        {
            ids.push_back(ImTextureID_Invalid);
            m_txd_inspect_srvs.push_back(nullptr);
            continue;
        }
        const UINT w = static_cast<UINT>(tex.width);
        const UINT h = static_cast<UINT>(tex.height);
        const std::size_t expected = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4u;
        if (tex.previewRgba.size() != expected)
        {
            ids.push_back(ImTextureID_Invalid);
            m_txd_inspect_srvs.push_back(nullptr);
            continue;
        }

        D3D11_TEXTURE2D_DESC td{};
        td.Width = w;
        td.Height = h;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_IMMUTABLE;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA srd{};
        srd.pSysMem = tex.previewRgba.data();
        srd.SysMemPitch = UINT(w * 4u);

        ID3D11Texture2D* tex2d = nullptr;
        HRESULT hr = m_device->CreateTexture2D(&td, &srd, &tex2d);
        if (FAILED(hr) || tex2d == nullptr)
        {
            ids.push_back(ImTextureID_Invalid);
            m_txd_inspect_srvs.push_back(nullptr);
            continue;
        }

        ID3D11ShaderResourceView* srv = nullptr;
        hr = m_device->CreateShaderResourceView(tex2d, nullptr, &srv);
        tex2d->Release();
        if (FAILED(hr) || srv == nullptr)
        {
            ids.push_back(ImTextureID_Invalid);
            m_txd_inspect_srvs.push_back(nullptr);
            continue;
        }

        m_txd_inspect_srvs.push_back(srv);
        ids.push_back(SrvToImTextureId(srv));
    }

    debug_ui.AssignTxdInspectThumbnails(std::move(ids));
}

bool Dx11RenderPipeline::ensure_model_preview_targets(ID3D11Device* device)
{
    if (m_preview_color != nullptr)
    {
        D3D11_TEXTURE2D_DESC cur{};
        m_preview_color->GetDesc(&cur);
        if (cur.Width != kModelPreviewSize || cur.Height != kModelPreviewSize)
        {
            if (m_preview_srv)
            {
                m_preview_srv->Release();
                m_preview_srv = nullptr;
            }
            if (m_preview_rtv)
            {
                m_preview_rtv->Release();
                m_preview_rtv = nullptr;
            }
            m_preview_color->Release();
            m_preview_color = nullptr;
            if (m_preview_dsv)
            {
                m_preview_dsv->Release();
                m_preview_dsv = nullptr;
            }
            if (m_preview_depth)
            {
                m_preview_depth->Release();
                m_preview_depth = nullptr;
            }
        }
    }

    if (m_preview_srv != nullptr && m_preview_rtv != nullptr && m_preview_dsv != nullptr)
    {
        return true;
    }

    if (m_preview_srv)
    {
        m_preview_srv->Release();
        m_preview_srv = nullptr;
    }
    if (m_preview_rtv)
    {
        m_preview_rtv->Release();
        m_preview_rtv = nullptr;
    }
    if (m_preview_color)
    {
        m_preview_color->Release();
        m_preview_color = nullptr;
    }
    if (m_preview_dsv)
    {
        m_preview_dsv->Release();
        m_preview_dsv = nullptr;
    }
    if (m_preview_depth)
    {
        m_preview_depth->Release();
        m_preview_depth = nullptr;
    }

    D3D11_TEXTURE2D_DESC td{};
    td.Width = kModelPreviewSize;
    td.Height = kModelPreviewSize;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    if (FAILED(device->CreateTexture2D(&td, nullptr, &m_preview_color)))
    {
        return false;
    }
    if (FAILED(device->CreateRenderTargetView(m_preview_color, nullptr, &m_preview_rtv)))
    {
        return false;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
    srvd.Format = td.Format;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvd.Texture2D.MipLevels = 1;
    if (FAILED(device->CreateShaderResourceView(m_preview_color, &srvd, &m_preview_srv)))
    {
        return false;
    }

    td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    if (FAILED(device->CreateTexture2D(&td, nullptr, &m_preview_depth)))
    {
        return false;
    }
    return SUCCEEDED(device->CreateDepthStencilView(m_preview_depth, nullptr, &m_preview_dsv));
}

bool Dx11RenderPipeline::ensure_model_preview_shaders(ID3D11Device* device)
{
    if (m_vs_preview != nullptr && m_ps_preview != nullptr && m_il_preview != nullptr && m_cb_preview != nullptr &&
        m_rs_preview != nullptr)
    {
        return true;
    }

    if (m_rs_preview)
    {
        m_rs_preview->Release();
        m_rs_preview = nullptr;
    }
    if (m_il_preview)
    {
        m_il_preview->Release();
        m_il_preview = nullptr;
    }
    if (m_vs_preview)
    {
        m_vs_preview->Release();
        m_vs_preview = nullptr;
    }
    if (m_ps_preview)
    {
        m_ps_preview->Release();
        m_ps_preview = nullptr;
    }
    if (m_cb_preview)
    {
        m_cb_preview->Release();
        m_cb_preview = nullptr;
    }

    ID3DBlob* blobVs = nullptr;
    ID3DBlob* blobPs = nullptr;
    if (!compile_blob(kPreviewVs, "main", "vs_5_0", &blobVs) || !compile_blob(kPreviewPs, "main", "ps_5_0", &blobPs))
    {
        if (blobVs)
        {
            blobVs->Release();
        }
        if (blobPs)
        {
            blobPs->Release();
        }
        return false;
    }

    const D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    HRESULT hr = device->CreateInputLayout(
        layout,
        static_cast<UINT>(std::size(layout)),
        blobVs->GetBufferPointer(),
        blobVs->GetBufferSize(),
        &m_il_preview);
    if (FAILED(hr))
    {
        blobVs->Release();
        blobPs->Release();
        return false;
    }
    hr = device->CreateVertexShader(blobVs->GetBufferPointer(), blobVs->GetBufferSize(), nullptr, &m_vs_preview);
    if (FAILED(hr))
    {
        blobVs->Release();
        blobPs->Release();
        return false;
    }
    hr = device->CreatePixelShader(blobPs->GetBufferPointer(), blobPs->GetBufferSize(), nullptr, &m_ps_preview);
    blobVs->Release();
    blobPs->Release();
    if (FAILED(hr))
    {
        return false;
    }

    D3D11_BUFFER_DESC cbd{};
    cbd.ByteWidth = sizeof(PreviewCB);
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device->CreateBuffer(&cbd, nullptr, &m_cb_preview)))
    {
        return false;
    }

    if (!ensure_model_preview_sampler_and_white_tex(device))
    {
        return false;
    }

    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    return SUCCEEDED(device->CreateRasterizerState(&rd, &m_rs_preview));
}

bool Dx11RenderPipeline::rebuild_model_preview_mesh(const GameLoader& loader, const std::string& modelStem, std::string& err)
{
    err.clear();
    DffPreviewSource src {};
    if (!TryLoadDffForPreviewSource(loader, modelStem, src, err))
    {
        return false;
    }
    std::vector<PreviewVertex> verts;
    std::vector<PreviewDrawRange> tmpBatches;
    DirectX::XMFLOAT3 center {};
    float radius = 1.0f;
    std::uint32_t triCount = 0;
    std::string texErr;
    const std::string stemLower = ToLowerAscii(modelStem);
    bool texturedOk = false;
    {
        std::lock_guard<std::mutex> lock(m_preview_texture_cache_mutex);
        if (!ensure_model_preview_sampler_and_white_tex(m_device))
        {
            err = "Не удалось создать сэмплер/текстуру превью.";
            return false;
        }
        texturedOk = BuildTexturedPreviewGeometry(
            loader,
            src,
            stemLower,
            m_device,
            m_preview_white_texture_srv,
            m_preview_txd_parse_by_key,
            m_preview_texture_srv_by_key,
            verts,
            tmpBatches,
            center,
            radius,
            triCount,
            texErr);
    }

    if (!texturedOk || verts.empty())
    {
        verts.clear();
        tmpBatches.clear();
        BuildPreviewMesh(src.dff, verts, center, radius, triCount, err);
        if (!err.empty())
        {
            return false;
        }
        tmpBatches.push_back(PreviewDrawRange {m_preview_white_texture_srv, 0u, static_cast<UINT>(verts.size())});
    }

    m_preview_draw_batches.clear();
    m_preview_draw_batches.reserve(tmpBatches.size());
    for (const auto& t : tmpBatches)
    {
        ModelPreviewDrawBatch b {};
        b.texture_srv = t.srv;
        b.start_vertex = t.start_vertex;
        b.vertex_count = t.vertex_count;
        m_preview_draw_batches.push_back(b);
    }

    if (m_preview_vb)
    {
        m_preview_vb->Release();
        m_preview_vb = nullptr;
    }
    m_preview_vertex_count = static_cast<UINT>(verts.size());
    m_preview_tri_count = triCount;
    m_preview_center = center;
    m_preview_radius = radius;

    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = static_cast<UINT>(sizeof(PreviewVertex) * verts.size());
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    const D3D11_SUBRESOURCE_DATA sd {verts.data(), 0, 0};
    return SUCCEEDED(m_device->CreateBuffer(&bd, &sd, &m_preview_vb));
}

void Dx11RenderPipeline::render_model_preview_internal(ID3D11DeviceContext* context)
{
    using namespace DirectX;

    ID3D11RenderTargetView* prevRtv = nullptr;
    ID3D11DepthStencilView* prevDsv = nullptr;
    context->OMGetRenderTargets(1, &prevRtv, &prevDsv);

    const float clear[4] = {0.07f, 0.075f, 0.1f, 1.0f};
    context->ClearRenderTargetView(m_preview_rtv, clear);
    context->ClearDepthStencilView(m_preview_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    context->OMSetRenderTargets(1, &m_preview_rtv, m_preview_dsv);

    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<FLOAT>(kModelPreviewSize);
    vp.Height = static_cast<FLOAT>(kModelPreviewSize);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    context->RSSetViewports(1, &vp);

    const float yr = XMConvertToRadians(m_preview_yawDeg);
    const float pr = XMConvertToRadians(m_preview_pitchDeg);
    const float cpr = std::cos(pr);
    const float dist = (std::max)(m_preview_radius * 2.35f * m_preview_dist_scale, 0.25f);
    const float ax = std::cos(yr) * cpr * dist;
    const float ay = std::sin(yr) * cpr * dist;
    const float az = std::sin(pr) * dist;
    const XMVECTOR at = XMLoadFloat3(&m_preview_center);
    const XMVECTOR eye = XMVectorAdd(at, XMVectorSet(ax, ay, az, 0.0f));
    const XMVECTOR up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
    const XMMATRIX view = XMMatrixLookAtRH(eye, at, up);
    const float zFar = (std::max)(m_preview_radius * 48.0f, 80.0f);
    const XMMATRIX proj = XMMatrixPerspectiveFovRH(XMConvertToRadians(48.0f), 1.0f, 0.05f, zFar);
    const XMMATRIX mvp = XMMatrixMultiply(view, proj);
    const XMMATRIX world = XMMatrixIdentity();

    PreviewCB cb{};
    XMStoreFloat4x4(&cb.mvp, mvp);
    XMStoreFloat4x4(&cb.world, world);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(context->Map(m_cb_preview, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        std::memcpy(mapped.pData, &cb, sizeof(cb));
        context->Unmap(m_cb_preview, 0);
    }

    const float blend_factor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    const UINT sample_mask = 0xffffffffu;

    context->OMSetDepthStencilState(m_depth_state, 0);
    context->OMSetBlendState(m_blend_opaque, blend_factor, sample_mask);
    context->RSSetState(m_rs_preview);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride = sizeof(PreviewVertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, &m_preview_vb, &stride, &offset);
    context->IASetInputLayout(m_il_preview);
    context->VSSetShader(m_vs_preview, nullptr, 0);
    context->PSSetShader(m_ps_preview, nullptr, 0);
    ID3D11Buffer* cbb = m_cb_preview;
    context->VSSetConstantBuffers(0, 1, &cbb);
    context->PSSetConstantBuffers(0, 1, &cbb);
    ID3D11SamplerState* samp = m_preview_sampler;
    if (samp != nullptr)
    {
        context->PSSetSamplers(0, 1, &samp);
    }
    ID3D11ShaderResourceView* fallbackSrv = m_preview_white_texture_srv;
    if (m_preview_draw_batches.empty())
    {
        if (fallbackSrv != nullptr)
        {
            context->PSSetShaderResources(0, 1, &fallbackSrv);
        }
        context->Draw(m_preview_vertex_count, 0);
    }
    else
    {
        for (const ModelPreviewDrawBatch& b : m_preview_draw_batches)
        {
            ID3D11ShaderResourceView* srv = b.texture_srv != nullptr ? b.texture_srv : fallbackSrv;
            if (srv != nullptr)
            {
                context->PSSetShaderResources(0, 1, &srv);
            }
            context->Draw(b.vertex_count, b.start_vertex);
        }
    }

    ID3D11ShaderResourceView* nullSrv = nullptr;
    context->PSSetShaderResources(0, 1, &nullSrv);

    context->RSSetState(nullptr);
    context->OMSetBlendState(nullptr, nullptr, sample_mask);

    context->OMSetRenderTargets(1, &prevRtv, prevDsv);
    if (prevRtv)
    {
        prevRtv->Release();
    }
    if (prevDsv)
    {
        prevDsv->Release();
    }
}

void Dx11RenderPipeline::UpdateModelPreview(
    ID3D11DeviceContext* context,
    DebugUi& debug_ui,
    const GameLoader& game_loader,
    float orbitYawDeltaDeg,
    float orbitPitchDeltaDeg,
    float zoomWheelSteps,
    float panMouseDx,
    float panMouseDy)
{
    if (!context || !m_device)
    {
        return;
    }

    if (!debug_ui.IsModelPreviewWindowOpen() || debug_ui.GetModelPreviewDisplayName().empty())
    {
        release_model_preview();
        debug_ui.SetModelPreviewFrame(ImTextureID_Invalid, false, 0, 0, nullptr);
        return;
    }

    if (!ensure_model_preview_shaders(m_device) || !ensure_model_preview_targets(m_device))
    {
        debug_ui.SetModelPreviewFrame(ImTextureID_Invalid, false, 0, 0, "Не удалось создать ресурсы превью (Direct3D 11).");
        return;
    }

    const std::string& modelName = debug_ui.GetModelPreviewDisplayName();
    const std::uint32_t token = debug_ui.GetModelPreviewLoadToken();
    if (m_preview_mesh_name != modelName || m_preview_load_token != token)
    {
        m_preview_mesh_name = modelName;
        m_preview_load_token = token;
        std::string err;
        if (!rebuild_model_preview_mesh(game_loader, modelName, err))
        {
            if (m_preview_vb)
            {
                m_preview_vb->Release();
                m_preview_vb = nullptr;
            }
            m_preview_vertex_count = 0;
            m_preview_tri_count = 0;
            debug_ui.SetModelPreviewFrame(SrvToImTextureId(m_preview_srv), false, 0, 0, err.c_str());
            return;
        }
        m_preview_yawDeg = 42.0f;
        m_preview_pitchDeg = 20.0f;
        m_preview_dist_scale = 1.0f;
    }

    if ((std::fabs(panMouseDx) > 1e-6f || std::fabs(panMouseDy) > 1e-6f) && m_preview_radius > 1e-6f)
    {
        using namespace DirectX;
        const float yr = XMConvertToRadians(m_preview_yawDeg);
        const float pr = XMConvertToRadians(m_preview_pitchDeg);
        const float cpr = std::cos(pr);
        const XMVECTOR zaxis = XMVector3Normalize(XMVectorSet(
            -std::cos(yr) * cpr,
            -std::sin(yr) * cpr,
            -std::sin(pr),
            0.0f));
        const XMVECTOR upDir = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
        XMVECTOR xaxis = XMVector3Cross(zaxis, upDir);
        const float xLenSq = XMVectorGetX(XMVector3LengthSq(xaxis));
        if (xLenSq < 1e-10f)
        {
            xaxis = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
        }
        else
        {
            xaxis = XMVector3Normalize(xaxis);
        }
        const XMVECTOR yaxis = XMVector3Normalize(XMVector3Cross(xaxis, zaxis));
        const float panScale = m_preview_radius * 0.0014f;
        XMVECTOR at = XMLoadFloat3(&m_preview_center);
        at = XMVectorAdd(at, XMVectorScale(xaxis, panMouseDx * panScale));
        at = XMVectorSubtract(at, XMVectorScale(yaxis, panMouseDy * panScale));
        XMStoreFloat3(&m_preview_center, at);
    }

    m_preview_yawDeg += orbitYawDeltaDeg * 0.45f;
    m_preview_pitchDeg += orbitPitchDeltaDeg * 0.45f;
    m_preview_pitchDeg = (std::clamp)(m_preview_pitchDeg, -80.0f, 80.0f);
    m_preview_dist_scale *= std::exp(-zoomWheelSteps * 0.14f);
    m_preview_dist_scale = (std::clamp)(m_preview_dist_scale, 0.22f, 7.5f);

    if (m_preview_vertex_count == 0 || !m_preview_vb)
    {
        debug_ui.SetModelPreviewFrame(SrvToImTextureId(m_preview_srv), false, 0, 0, "Нет геометрии для превью.");
        return;
    }

    render_model_preview_internal(context);
    debug_ui.SetModelPreviewFrame(
        SrvToImTextureId(m_preview_srv),
        true,
        m_preview_tri_count,
        static_cast<std::uint32_t>(m_preview_vertex_count),
        nullptr);
}

void Dx11RenderPipeline::release_imported_scene_gpu()
{
    m_scene_instance_count = 0;
    m_scene_index_count = 0;
    if (m_srv_scene_instances != nullptr)
    {
        m_srv_scene_instances->Release();
        m_srv_scene_instances = nullptr;
    }
    if (m_buf_scene_instances != nullptr)
    {
        m_buf_scene_instances->Release();
        m_buf_scene_instances = nullptr;
    }
    if (m_ib_scene_cube != nullptr)
    {
        m_ib_scene_cube->Release();
        m_ib_scene_cube = nullptr;
    }
    if (m_vb_scene_cube != nullptr)
    {
        m_vb_scene_cube->Release();
        m_vb_scene_cube = nullptr;
    }
    if (m_cb_scene != nullptr)
    {
        m_cb_scene->Release();
        m_cb_scene = nullptr;
    }
    if (m_il_scene_inst != nullptr)
    {
        m_il_scene_inst->Release();
        m_il_scene_inst = nullptr;
    }
    if (m_vs_scene_inst != nullptr)
    {
        m_vs_scene_inst->Release();
        m_vs_scene_inst = nullptr;
    }
    if (m_ps_scene_inst != nullptr)
    {
        m_ps_scene_inst->Release();
        m_ps_scene_inst = nullptr;
    }
    if (m_rs_scene_solid != nullptr)
    {
        m_rs_scene_solid->Release();
        m_rs_scene_solid = nullptr;
    }
    m_scene_placements.clear();
    m_scene_placement_indices_by_model.clear();
    m_scene_cube_instances_cpu.clear();
    m_scene_cube_instances_dirty = false;
    m_scene_dff_draw_list.clear();
}

void Dx11RenderPipeline::release_imported_scene_instance_data()
{
    m_scene_instance_count = 0;
    if (m_srv_scene_instances != nullptr)
    {
        m_srv_scene_instances->Release();
        m_srv_scene_instances = nullptr;
    }
    if (m_buf_scene_instances != nullptr)
    {
        m_buf_scene_instances->Release();
        m_buf_scene_instances = nullptr;
    }
}

void Dx11RenderPipeline::release_scene_cube_instancing_pipeline_only()
{
    m_scene_index_count = 0;
    if (m_ib_scene_cube != nullptr)
    {
        m_ib_scene_cube->Release();
        m_ib_scene_cube = nullptr;
    }
    if (m_vb_scene_cube != nullptr)
    {
        m_vb_scene_cube->Release();
        m_vb_scene_cube = nullptr;
    }
    if (m_cb_scene != nullptr)
    {
        m_cb_scene->Release();
        m_cb_scene = nullptr;
    }
    if (m_il_scene_inst != nullptr)
    {
        m_il_scene_inst->Release();
        m_il_scene_inst = nullptr;
    }
    if (m_vs_scene_inst != nullptr)
    {
        m_vs_scene_inst->Release();
        m_vs_scene_inst = nullptr;
    }
    if (m_ps_scene_inst != nullptr)
    {
        m_ps_scene_inst->Release();
        m_ps_scene_inst = nullptr;
    }
    if (m_rs_scene_solid != nullptr)
    {
        m_rs_scene_solid->Release();
        m_rs_scene_solid = nullptr;
    }
}

bool Dx11RenderPipeline::ensure_imported_scene_pipeline(ID3D11Device* device)
{
    if (device == nullptr)
    {
        return false;
    }
    if (m_vs_scene_inst != nullptr && m_ps_scene_inst != nullptr && m_il_scene_inst != nullptr && m_vb_scene_cube != nullptr &&
        m_ib_scene_cube != nullptr && m_cb_scene != nullptr && m_rs_scene_solid != nullptr && m_scene_index_count > 0)
    {
        return true;
    }

    release_scene_cube_instancing_pipeline_only();

    ID3DBlob* blob_vs = nullptr;
    ID3DBlob* blob_ps = nullptr;
    if (!compile_blob(kSceneInstVs, "main", "vs_5_0", &blob_vs) || !compile_blob(kSceneInstPs, "main", "ps_5_0", &blob_ps))
    {
        if (blob_vs != nullptr)
        {
            blob_vs->Release();
        }
        if (blob_ps != nullptr)
        {
            blob_ps->Release();
        }
        return false;
    }

    const D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    HRESULT hr = device->CreateInputLayout(
        layout,
        static_cast<UINT>(std::size(layout)),
        blob_vs->GetBufferPointer(),
        blob_vs->GetBufferSize(),
        &m_il_scene_inst);
    if (FAILED(hr))
    {
        blob_vs->Release();
        blob_ps->Release();
        return false;
    }

    hr = device->CreateVertexShader(blob_vs->GetBufferPointer(), blob_vs->GetBufferSize(), nullptr, &m_vs_scene_inst);
    if (FAILED(hr))
    {
        blob_vs->Release();
        blob_ps->Release();
        return false;
    }

    hr = device->CreatePixelShader(blob_ps->GetBufferPointer(), blob_ps->GetBufferSize(), nullptr, &m_ps_scene_inst);
    blob_vs->Release();
    blob_ps->Release();
    if (FAILED(hr))
    {
        return false;
    }

    D3D11_BUFFER_DESC cbd {};
    cbd.ByteWidth = sizeof(DirectX::XMFLOAT4X4);
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device->CreateBuffer(&cbd, nullptr, &m_cb_scene)) || m_cb_scene == nullptr)
    {
        return false;
    }

    std::vector<SceneVertexInst> cube_verts;
    std::vector<std::uint16_t> cube_idx;
    AppendUnitCubeMesh(cube_verts, cube_idx);
    m_scene_index_count = static_cast<std::uint32_t>(cube_idx.size());

    D3D11_BUFFER_DESC vbd {};
    vbd.ByteWidth = static_cast<UINT>(cube_verts.size() * sizeof(SceneVertexInst));
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vsd {};
    vsd.pSysMem = cube_verts.data();
    if (FAILED(device->CreateBuffer(&vbd, &vsd, &m_vb_scene_cube)) || m_vb_scene_cube == nullptr)
    {
        return false;
    }

    D3D11_BUFFER_DESC ibd {};
    ibd.ByteWidth = static_cast<UINT>(cube_idx.size() * sizeof(std::uint16_t));
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    D3D11_SUBRESOURCE_DATA isd {};
    isd.pSysMem = cube_idx.data();
    if (FAILED(device->CreateBuffer(&ibd, &isd, &m_ib_scene_cube)) || m_ib_scene_cube == nullptr)
    {
        return false;
    }

    D3D11_RASTERIZER_DESC rd {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.FrontCounterClockwise = FALSE;
    rd.DepthClipEnable = TRUE;
    rd.MultisampleEnable = (m_msaa_samples > 1u) ? TRUE : FALSE;
    if (FAILED(device->CreateRasterizerState(&rd, &m_rs_scene_solid)) || m_rs_scene_solid == nullptr)
    {
        return false;
    }

    return true;
}

void Dx11RenderPipeline::map_scene_viewproj(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& viewProj)
{
    if (context == nullptr || m_cb_scene == nullptr)
    {
        return;
    }
    D3D11_MAPPED_SUBRESOURCE mapped {};
    if (SUCCEEDED(context->Map(m_cb_scene, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        std::memcpy(mapped.pData, &viewProj, sizeof(viewProj));
        context->Unmap(m_cb_scene, 0);
    }
}

void Dx11RenderPipeline::draw_imported_scene(
    ID3D11DeviceContext* context,
    const GtaStyleCamera& camera,
    float aspect,
    std::uint32_t viewport_width,
    std::uint32_t viewport_height)
{
    (void)viewport_width;
    (void)viewport_height;
    if (m_scene_cube_instances_dirty && context != nullptr && m_buf_scene_instances != nullptr && !m_scene_cube_instances_cpu.empty() &&
        m_scene_cube_instances_cpu.size() == m_scene_instance_count)
    {
        const UINT byteWidth = static_cast<UINT>(sizeof(SceneInstanceGpu) * m_scene_cube_instances_cpu.size());
        context->UpdateSubresource(m_buf_scene_instances, 0, nullptr, m_scene_cube_instances_cpu.data(), byteWidth, byteWidth);
        m_scene_cube_instances_dirty = false;
    }
    if (!m_draw_imported_scene_cubes || m_scene_instance_count == 0 || context == nullptr || m_vb_scene_cube == nullptr ||
        m_ib_scene_cube == nullptr || m_buf_scene_instances == nullptr || m_srv_scene_instances == nullptr ||
        m_vs_scene_inst == nullptr || m_ps_scene_inst == nullptr || m_il_scene_inst == nullptr || m_cb_scene == nullptr ||
        m_rs_scene_solid == nullptr)
    {
        return;
    }

    DirectX::XMFLOAT4X4 vp {};
    DirectX::XMStoreFloat4x4(&vp, camera.WorldViewProjection(aspect));
    map_scene_viewproj(context, vp);

    const float blend_factor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    const UINT sample_mask = 0xffffffffu;
    context->OMSetBlendState(m_blend_opaque, blend_factor, sample_mask);
    context->OMSetDepthStencilState(m_depth_state, 0);
    context->RSSetState(m_rs_scene_solid);

    const UINT stride = sizeof(SceneVertexInst);
    UINT vb_offset = 0;
    context->IASetVertexBuffers(0, 1, &m_vb_scene_cube, &stride, &vb_offset);
    context->IASetIndexBuffer(m_ib_scene_cube, DXGI_FORMAT_R16_UINT, 0);
    context->IASetInputLayout(m_il_scene_inst);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->VSSetShader(m_vs_scene_inst, nullptr, 0);
    context->PSSetShader(m_ps_scene_inst, nullptr, 0);
    ID3D11Buffer* cb = m_cb_scene;
    context->VSSetConstantBuffers(0, 1, &cb);
    ID3D11ShaderResourceView* srv = m_srv_scene_instances;
    context->VSSetShaderResources(0, 1, &srv);

    context->DrawIndexedInstanced(m_scene_index_count, m_scene_instance_count, 0, 0, 0);

    ID3D11ShaderResourceView* null_srv = nullptr;
    context->VSSetShaderResources(0, 1, &null_srv);
}

void Dx11RenderPipeline::stop_scene_dff_worker()
{
    m_scene_dff_gen.fetch_add(1u, std::memory_order_acq_rel);
    if (m_scene_dff_worker.joinable())
    {
        m_scene_dff_worker.join();
    }
    if (m_scene_texturing_worker.joinable())
    {
        m_scene_texturing_worker.join();
    }
}

void Dx11RenderPipeline::release_imported_scene_dff_gpu()
{
    for (SceneDffMeshTemplateGpu& t : m_scene_dff_templates)
    {
        if (t.vertexBuffer != nullptr)
        {
            t.vertexBuffer->Release();
            t.vertexBuffer = nullptr;
        }
        if (t.indexBuffer != nullptr)
        {
            t.indexBuffer->Release();
            t.indexBuffer = nullptr;
        }
        t.indexCount = 0;
    }
    m_scene_dff_templates.clear();
    m_scene_dff_template_by_stem.clear();
    m_scene_dff_draw_list.clear();
    m_scene_dff_draw_list_sort_dirty = false;
    m_scene_stat_dff_vertex_total = 0;
    m_scene_stat_dff_index_total = 0;
    {
        std::lock_guard<std::mutex> lock(m_scene_dff_mutex);
        m_scene_dff_parsed_by_stem.clear();
        m_scene_dff_completed_models.clear();
        m_scene_dff_texturing_queue.clear();
        m_scene_dff_textured_ready.clear();
    }
    m_scene_tex_timing_logs_printed = 0;
    if (m_rs_scene_dff != nullptr)
    {
        m_rs_scene_dff->Release();
        m_rs_scene_dff = nullptr;
    }
    if (m_il_scene_dff != nullptr)
    {
        m_il_scene_dff->Release();
        m_il_scene_dff = nullptr;
    }
    if (m_vs_scene_dff != nullptr)
    {
        m_vs_scene_dff->Release();
        m_vs_scene_dff = nullptr;
    }
    if (m_ps_scene_dff != nullptr)
    {
        m_ps_scene_dff->Release();
        m_ps_scene_dff = nullptr;
    }
    if (m_cb_scene_dff != nullptr)
    {
        m_cb_scene_dff->Release();
        m_cb_scene_dff = nullptr;
    }
}

bool Dx11RenderPipeline::ensure_scene_dff_mesh_pipeline(ID3D11Device* device)
{
    if (device == nullptr)
    {
        return false;
    }
    if (m_vs_scene_dff != nullptr && m_ps_scene_dff != nullptr && m_il_scene_dff != nullptr && m_cb_scene_dff != nullptr &&
        m_rs_scene_dff != nullptr)
    {
        return true;
    }

    if (m_rs_scene_dff != nullptr)
    {
        m_rs_scene_dff->Release();
        m_rs_scene_dff = nullptr;
    }
    if (m_il_scene_dff != nullptr)
    {
        m_il_scene_dff->Release();
        m_il_scene_dff = nullptr;
    }
    if (m_vs_scene_dff != nullptr)
    {
        m_vs_scene_dff->Release();
        m_vs_scene_dff = nullptr;
    }
    if (m_ps_scene_dff != nullptr)
    {
        m_ps_scene_dff->Release();
        m_ps_scene_dff = nullptr;
    }
    if (m_cb_scene_dff != nullptr)
    {
        m_cb_scene_dff->Release();
        m_cb_scene_dff = nullptr;
    }

    ID3DBlob* blob_vs = nullptr;
    ID3DBlob* blob_ps = nullptr;
    constexpr UINT kDffMeshCompileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
    if (!compile_blob(kSceneDffMeshVs, "main", "vs_5_0", &blob_vs, kDffMeshCompileFlags) ||
        !compile_blob(kSceneDffMeshPs, "main", "ps_5_0", &blob_ps, kDffMeshCompileFlags))
    {
        if (blob_vs != nullptr)
        {
            blob_vs->Release();
        }
        if (blob_ps != nullptr)
        {
            blob_ps->Release();
        }
        return false;
    }

    const D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    HRESULT hr = device->CreateInputLayout(
        layout,
        static_cast<UINT>(std::size(layout)),
        blob_vs->GetBufferPointer(),
        blob_vs->GetBufferSize(),
        &m_il_scene_dff);
    if (FAILED(hr))
    {
        blob_vs->Release();
        blob_ps->Release();
        return false;
    }

    hr = device->CreateVertexShader(blob_vs->GetBufferPointer(), blob_vs->GetBufferSize(), nullptr, &m_vs_scene_dff);
    if (FAILED(hr))
    {
        blob_vs->Release();
        blob_ps->Release();
        return false;
    }

    hr = device->CreatePixelShader(blob_ps->GetBufferPointer(), blob_ps->GetBufferSize(), nullptr, &m_ps_scene_dff);
    blob_vs->Release();
    blob_ps->Release();
    if (FAILED(hr))
    {
        return false;
    }

    D3D11_BUFFER_DESC cbd {};
    cbd.ByteWidth = sizeof(SceneDffMeshCB);
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device->CreateBuffer(&cbd, nullptr, &m_cb_scene_dff)) || m_cb_scene_dff == nullptr)
    {
        return false;
    }

    D3D11_RASTERIZER_DESC rd {};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    rd.MultisampleEnable = (m_msaa_samples > 1u) ? TRUE : FALSE;
    if (FAILED(device->CreateRasterizerState(&rd, &m_rs_scene_dff)) || m_rs_scene_dff == nullptr)
    {
        return false;
    }

    return true;
}

void Dx11RenderPipeline::map_scene_dff_mvp(ID3D11DeviceContext* context, const DirectX::XMFLOAT4X4& mvp, const DirectX::XMFLOAT4& color)
{
    if (context == nullptr || m_cb_scene_dff == nullptr)
    {
        return;
    }
    SceneDffMeshCB cb {};
    cb.mvp = mvp;
    cb.color = color;
    cb.pad = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
    D3D11_MAPPED_SUBRESOURCE mapped {};
    if (SUCCEEDED(context->Map(m_cb_scene_dff, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
    {
        std::memcpy(mapped.pData, &cb, sizeof(cb));
        context->Unmap(m_cb_scene_dff, 0);
    }
}

void Dx11RenderPipeline::draw_imported_scene_dff_meshes(
    ID3D11DeviceContext* context,
    const GtaStyleCamera& camera,
    float aspect)
{
    if (!m_draw_imported_scene_dff || m_scene_dff_draw_list.empty() || context == nullptr || m_device == nullptr)
    {
        return;
    }

    bool any_textured = false;
    bool any_indexed = false;
    for (const SceneDffDrawInstance& draw : m_scene_dff_draw_list)
    {
        if (draw.templateIndex >= m_scene_dff_templates.size())
        {
            continue;
        }
        const SceneDffMeshTemplateGpu& tmpl = m_scene_dff_templates[draw.templateIndex];
        if (tmpl.textured && tmpl.vertexBuffer != nullptr && !tmpl.texturedBatches.empty())
        {
            any_textured = true;
        }
        else if (tmpl.vertexBuffer != nullptr && tmpl.indexBuffer != nullptr && tmpl.indexCount > 0)
        {
            any_indexed = true;
        }
    }
    if (!any_textured && !any_indexed)
    {
        return;
    }

    if (any_indexed && !ensure_scene_dff_mesh_pipeline(m_device))
    {
        return;
    }
    if (any_textured && !ensure_model_preview_shaders(m_device))
    {
        return;
    }

    using namespace DirectX;
    const XMMATRIX view_proj = camera.WorldViewProjection(aspect);

    if (m_scene_dff_draw_list_sort_dirty)
    {
        std::sort(
            m_scene_dff_draw_list.begin(),
            m_scene_dff_draw_list.end(),
            [](const SceneDffDrawInstance& a, const SceneDffDrawInstance& b) {
                return a.templateIndex < b.templateIndex;
            });
        m_scene_dff_draw_list_sort_dirty = false;
    }

    const float blend_factor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    const UINT sample_mask = 0xffffffffu;
    context->OMSetBlendState(m_blend_opaque, blend_factor, sample_mask);
    context->OMSetDepthStencilState(m_depth_state, 0);

    ID3D11SamplerState* preview_samp = m_preview_sampler;

    std::size_t indexed_bound_template = static_cast<std::size_t>(-1);

    for (const SceneDffDrawInstance& draw : m_scene_dff_draw_list)
    {
        if (draw.templateIndex >= m_scene_dff_templates.size())
        {
            continue;
        }
        const SceneDffMeshTemplateGpu& tmpl = m_scene_dff_templates[draw.templateIndex];

        if (m_scene_dff_ide_distance_cull)
        {
            const float draw_r = draw.drawDistanceMax * m_scene_draw_distance_scale;
            if (draw_r > 0.0f)
            {
                const float dx = draw.position.x - camera.GetX();
                const float dy = draw.position.y - camera.GetY();
                const float dz = draw.position.z - camera.GetZ();
                const float dist_sq = dx * dx + dy * dy + dz * dz;
                if (dist_sq > draw_r * draw_r)
                {
                    continue;
                }
            }
        }

        XMMATRIX rot = XMMatrixIdentity();
        if (m_scene_import_use_quaternion)
        {
            XMVECTOR q = XMLoadFloat4(&draw.rotation);
            const float q_len_sq = XMVectorGetX(XMQuaternionLengthSq(q));
            if (q_len_sq > 1.0e-8f)
            {
                q = XMQuaternionNormalize(q);
                q = XMQuaternionConjugate(q);
                rot = XMMatrixRotationQuaternion(q);
            }
        }
        else
        {
            rot = XMMatrixRotationRollPitchYaw(draw.rotation.x, draw.rotation.y, draw.rotation.z);
        }

        const XMMATRIX trn = XMMatrixTranslation(draw.position.x, draw.position.y, draw.position.z);
        const XMMATRIX world = XMMatrixMultiply(rot, trn);
        const XMMATRIX mvp = XMMatrixMultiply(world, view_proj);

        if (tmpl.textured && tmpl.vertexBuffer != nullptr && !tmpl.texturedBatches.empty())
        {
            indexed_bound_template = static_cast<std::size_t>(-1);
            context->RSSetState(m_rs_preview);
            context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context->IASetInputLayout(m_il_preview);
            context->VSSetShader(m_vs_preview, nullptr, 0);
            context->PSSetShader(m_ps_preview, nullptr, 0);
            ID3D11Buffer* cbb = m_cb_preview;
            context->VSSetConstantBuffers(0, 1, &cbb);
            context->PSSetConstantBuffers(0, 1, &cbb);

            PreviewCB cb {};
            XMStoreFloat4x4(&cb.mvp, mvp);
            XMStoreFloat4x4(&cb.world, world);

            D3D11_MAPPED_SUBRESOURCE mapped {};
            if (SUCCEEDED(context->Map(m_cb_preview, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
            {
                std::memcpy(mapped.pData, &cb, sizeof(cb));
                context->Unmap(m_cb_preview, 0);
            }

            const UINT stride = sizeof(PreviewVertex);
            UINT vb_offset = 0u;
            ID3D11Buffer* vb = tmpl.vertexBuffer;
            context->IASetVertexBuffers(0, 1, &vb, &stride, &vb_offset);

            if (preview_samp != nullptr)
            {
                context->PSSetSamplers(0, 1, &preview_samp);
            }

            ID3D11ShaderResourceView* fallback_srv = m_preview_white_texture_srv;
            for (const ModelPreviewDrawBatch& b : tmpl.texturedBatches)
            {
                ID3D11ShaderResourceView* srv = b.texture_srv != nullptr ? b.texture_srv : fallback_srv;
                if (srv != nullptr)
                {
                    context->PSSetShaderResources(0, 1, &srv);
                }
                context->Draw(b.vertex_count, b.start_vertex);
            }

            ID3D11ShaderResourceView* null_srv = nullptr;
            context->PSSetShaderResources(0, 1, &null_srv);
        }
        else if (tmpl.vertexBuffer != nullptr && tmpl.indexBuffer != nullptr && tmpl.indexCount > 0)
        {
            if (indexed_bound_template != draw.templateIndex)
            {
                indexed_bound_template = draw.templateIndex;
                context->RSSetState(m_rs_scene_dff);
                context->IASetInputLayout(m_il_scene_dff);
                context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                context->VSSetShader(m_vs_scene_dff, nullptr, 0);
                context->PSSetShader(m_ps_scene_dff, nullptr, 0);
                ID3D11Buffer* cbuf = m_cb_scene_dff;
                context->VSSetConstantBuffers(0, 1, &cbuf);
                context->PSSetConstantBuffers(0, 1, &cbuf);
                constexpr UINT stride = sizeof(float) * 3u;
                constexpr UINT vb_offset = 0u;
                ID3D11Buffer* vb = tmpl.vertexBuffer;
                context->IASetVertexBuffers(0, 1, &vb, &stride, &vb_offset);
                context->IASetIndexBuffer(tmpl.indexBuffer, DXGI_FORMAT_R32_UINT, 0);
            }

            XMFLOAT4X4 mvp_store {};
            XMStoreFloat4x4(&mvp_store, mvp);
            map_scene_dff_mvp(context, mvp_store, draw.color);
            context->DrawIndexed(tmpl.indexCount, 0, 0);
        }
    }
}

void Dx11RenderPipeline::refresh_debug_scene_stats(DebugUi& debug_ui)
{
    DebugUi::SceneStats s {};
    s.instCount = m_scene_stat_inst_total;
    s.uniqueModelCount = m_scene_stat_unique_models;
    s.cubeInstances =
        (m_scene_instance_count != 0u) ? static_cast<std::size_t>(m_scene_instance_count) : m_scene_cube_instances_cpu.size();
    s.dffMeshInstances = m_scene_dff_draw_list.size();
    s.dffModelsParsed = m_scene_dff_template_by_stem.size();
    s.dffModelsQueued = m_scene_stat_unique_models;
    s.vertices = m_scene_stat_dff_vertex_total;
    s.triangles = m_scene_stat_dff_index_total / 3u;
    debug_ui.SetSceneStats(s);
}

void Dx11RenderPipeline::scene_dff_worker_main(
    std::uint64_t gen,
    std::filesystem::path gameRoot,
    std::vector<std::pair<std::string, std::shared_ptr<const d11::data::tImgParseResult>>> archives,
    std::vector<std::string> modelsSorted)
{
    namespace fs = std::filesystem;
    std::size_t worker_idx = 0;
    for (const std::string& stem : modelsSorted)
    {
        if (m_scene_dff_gen.load(std::memory_order_acquire) != gen)
        {
            return;
        }

        if ((worker_idx++ & 15u) == 0u)
        {
            std::this_thread::yield();
        }

        SceneAsyncDffMeshData data {};
        const std::string dff_entry = ModelStemToDffEntryName(stem);

        if (!gameRoot.empty())
        {
            const fs::path loose = gameRoot / "models" / dff_entry;
            if (fs::exists(loose))
            {
                d11::data::tDffParseResult dff = d11::data::ParseDffFile(loose.string());
                if (dff.IsOk())
                {
                    MergeDffToSceneMesh(dff, data.vertices, data.indices);
                }
            }
        }

        if (m_scene_dff_gen.load(std::memory_order_acquire) != gen)
        {
            return;
        }

        if (data.vertices.empty())
        {
            for (const auto& kv : archives)
            {
                if (m_scene_dff_gen.load(std::memory_order_acquire) != gen)
                {
                    return;
                }
                if (!kv.second || !kv.second->errorMessage.empty())
                {
                    continue;
                }
                const auto idx_opt = kv.second->FindEntryIndexByName(dff_entry);
                if (!idx_opt || *idx_opt >= kv.second->entries.size())
                {
                    continue;
                }
                d11::data::tDffParseResult dff =
                    d11::data::ParseDffFromImgArchiveEntry(kv.first, kv.second->entries[*idx_opt]);
                if (!dff.IsOk())
                {
                    continue;
                }
                MergeDffToSceneMesh(dff, data.vertices, data.indices);
                if (!data.vertices.empty())
                {
                    break;
                }
            }
        }

        data.hasRenderableGeometry = !data.vertices.empty() && !data.indices.empty();

        std::lock_guard<std::mutex> lock(m_scene_dff_mutex);
        if (m_scene_dff_gen.load(std::memory_order_acquire) != gen)
        {
            return;
        }
        m_scene_dff_parsed_by_stem[stem] = std::move(data);
        m_scene_dff_completed_models.push_back(stem);
    }
}

void Dx11RenderPipeline::scene_texturing_worker_main(std::uint64_t gen, const GameLoader* loader)
{
    if (loader == nullptr)
    {
        return;
    }

    while (m_scene_dff_gen.load(std::memory_order_acquire) == gen)
    {
        SceneAsyncTexturingRequest req {};
        {
            std::lock_guard<std::mutex> lock(m_scene_dff_mutex);
            if (!m_scene_dff_texturing_queue.empty())
            {
                req = std::move(m_scene_dff_texturing_queue.front());
                m_scene_dff_texturing_queue.pop_front();
            }
        }

        if (req.modelLower.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        const std::string& model_lower = req.modelLower;
        const std::uint64_t tex_worker_t0_us = NowSteadyUs();

        DffPreviewSource preview_src {};
        std::string preview_load_err;
        if (!TryLoadDffForPreviewSource(*loader, model_lower, preview_src, preview_load_err))
        {
            continue;
        }

        std::vector<PreviewVertex> tex_verts;
        std::vector<PreviewDrawRange> tex_batches;
        DirectX::XMFLOAT3 tex_center {};
        float tex_radius = 1.0f;
        std::uint32_t tex_tri_count = 0;
        std::string tex_err;

        const std::uint64_t tex_build_t0_us = NowSteadyUs();
        bool textured_ok = false;
        {
            std::lock_guard<std::mutex> lock(m_preview_texture_cache_mutex);
            if (m_preview_white_texture_srv != nullptr)
            {
                textured_ok = BuildTexturedPreviewGeometry(
                    *loader,
                    preview_src,
                    model_lower,
                    m_device,
                    m_preview_white_texture_srv,
                    m_preview_txd_parse_by_key,
                    m_preview_texture_srv_by_key,
                    tex_verts,
                    tex_batches,
                    tex_center,
                    tex_radius,
                    tex_tri_count,
                    tex_err);
            }
        }
        const std::uint64_t tex_build_us = NowSteadyUs() - tex_build_t0_us;

        if (!textured_ok || tex_verts.empty())
        {
            continue;
        }

        SceneAsyncTexturedData ready {};
        ready.modelLower = model_lower;
        ready.vertices.reserve(tex_verts.size());
        for (const PreviewVertex& v : tex_verts)
        {
            SceneAsyncTexturedData::Vertex out {};
            out.px = v.px;
            out.py = v.py;
            out.pz = v.pz;
            out.nx = v.nx;
            out.ny = v.ny;
            out.nz = v.nz;
            out.u = v.u;
            out.v = v.v;
            ready.vertices.push_back(out);
        }
        ready.batches.reserve(tex_batches.size());
        for (const PreviewDrawRange& r : tex_batches)
        {
            ModelPreviewDrawBatch b {};
            b.texture_srv = r.srv;
            b.start_vertex = r.start_vertex;
            b.vertex_count = r.vertex_count;
            ready.batches.push_back(b);
        }
        ready.triCount = tex_tri_count;
        ready.requestUs = req.requestUs;
        ready.buildUs = tex_build_us;
        ready.workerTotalUs = NowSteadyUs() - tex_worker_t0_us;

        {
            std::lock_guard<std::mutex> lock(m_scene_dff_mutex);
            if (m_scene_dff_gen.load(std::memory_order_acquire) != gen)
            {
                return;
            }
            m_scene_dff_textured_ready.push_back(std::move(ready));
        }
    }
}

void Dx11RenderPipeline::PumpImportedSceneDffUploads(DebugUi& debug_ui, const GameLoader& loader)
{
    (void)loader;
    if (m_device == nullptr)
    {
        return;
    }

    /** Быстрый этап: как можно быстрее завести в сцену геометрию (VB+IB). */
    constexpr int kMaxModelsPerFrame = 32;
    int processed = 0;
    bool pump_added_dff_draws = false;
    while (processed < kMaxModelsPerFrame)
    {
        std::string model_lower;
        {
            std::lock_guard<std::mutex> lock(m_scene_dff_mutex);
            if (m_scene_dff_completed_models.empty())
            {
                break;
            }
            model_lower = std::move(m_scene_dff_completed_models.front());
            m_scene_dff_completed_models.pop_front();
        }

        SceneAsyncDffMeshData model_data {};
        {
            std::lock_guard<std::mutex> lock(m_scene_dff_mutex);
            const auto it = m_scene_dff_parsed_by_stem.find(model_lower);
            if (it == m_scene_dff_parsed_by_stem.end())
            {
                ++processed;
                continue;
            }
            model_data = std::move(it->second);
            m_scene_dff_parsed_by_stem.erase(it);
        }

        if (!model_data.hasRenderableGeometry)
        {
            ++processed;
            continue;
        }

        std::size_t template_index = 0;
        const auto tmpl_it = m_scene_dff_template_by_stem.find(model_lower);
        if (tmpl_it == m_scene_dff_template_by_stem.end())
        {
            SceneDffMeshTemplateGpu tmpl {};
            {
                std::vector<float> interleaved;
                interleaved.reserve(model_data.vertices.size() * 3u);
                for (const DirectX::XMFLOAT3& v : model_data.vertices)
                {
                    interleaved.push_back(v.x);
                    interleaved.push_back(v.y);
                    interleaved.push_back(v.z);
                }

                D3D11_BUFFER_DESC vbd {};
                vbd.ByteWidth = static_cast<UINT>(interleaved.size() * sizeof(float));
                vbd.Usage = D3D11_USAGE_IMMUTABLE;
                vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
                D3D11_SUBRESOURCE_DATA vsd {};
                vsd.pSysMem = interleaved.data();
                HRESULT hr = m_device->CreateBuffer(&vbd, &vsd, &tmpl.vertexBuffer);
                if (FAILED(hr) || tmpl.vertexBuffer == nullptr)
                {
                    ++processed;
                    continue;
                }

                D3D11_BUFFER_DESC ibd {};
                ibd.ByteWidth = static_cast<UINT>(model_data.indices.size() * sizeof(std::uint32_t));
                ibd.Usage = D3D11_USAGE_IMMUTABLE;
                ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
                D3D11_SUBRESOURCE_DATA isd {};
                isd.pSysMem = model_data.indices.data();
                hr = m_device->CreateBuffer(&ibd, &isd, &tmpl.indexBuffer);
                if (FAILED(hr) || tmpl.indexBuffer == nullptr)
                {
                    tmpl.vertexBuffer->Release();
                    tmpl.vertexBuffer = nullptr;
                    ++processed;
                    continue;
                }

                tmpl.indexCount = static_cast<std::uint32_t>(model_data.indices.size());
                m_scene_stat_dff_vertex_total += model_data.vertices.size();
                m_scene_stat_dff_index_total += model_data.indices.size();
            }

            template_index = m_scene_dff_templates.size();
            m_scene_dff_templates.push_back(tmpl);
            m_scene_dff_template_by_stem.emplace(model_lower, template_index);
            {
                std::lock_guard<std::mutex> lock(m_scene_dff_mutex);
                SceneAsyncTexturingRequest req {};
                req.modelLower = model_lower;
                req.requestUs = NowSteadyUs();
                m_scene_dff_texturing_queue.push_back(std::move(req));
            }
        }
        else
        {
            template_index = tmpl_it->second;
        }

        const auto meta_it = m_scene_placement_indices_by_model.find(model_lower);
        if (meta_it != m_scene_placement_indices_by_model.end())
        {
            for (const std::size_t pidx : meta_it->second)
            {
                if (pidx >= m_scene_placements.size())
                {
                    continue;
                }
                const SceneImportPlacement& pl = m_scene_placements[pidx];
                SceneDffDrawInstance draw {};
                draw.templateIndex = template_index;
                draw.position = pl.position;
                draw.rotation = pl.rotation;
                draw.color = pl.color;
                draw.drawDistanceMax = pl.drawDistanceMax;
                m_scene_dff_draw_list.push_back(draw);
                pump_added_dff_draws = true;

                if (pl.cubeInstanceIndex < m_scene_cube_instances_cpu.size())
                {
                    m_scene_cube_instances_cpu[pl.cubeInstanceIndex].extra = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
                }
            }
        }

        m_scene_cube_instances_dirty = true;
        ++processed;
    }

    constexpr int kMaxReadyApplyPerFrame = 1;
    int applied = 0;
    while (applied < kMaxReadyApplyPerFrame)
    {
        SceneAsyncTexturedData ready {};
        {
            std::lock_guard<std::mutex> lock(m_scene_dff_mutex);
            if (m_scene_dff_textured_ready.empty())
            {
                break;
            }
            ready = std::move(m_scene_dff_textured_ready.front());
            m_scene_dff_textured_ready.pop_front();
        }

        const auto idx_it = m_scene_dff_template_by_stem.find(ready.modelLower);
        if (idx_it == m_scene_dff_template_by_stem.end() || idx_it->second >= m_scene_dff_templates.size())
        {
            continue;
        }

        SceneDffMeshTemplateGpu& tmpl = m_scene_dff_templates[idx_it->second];
        if (tmpl.textured || ready.vertices.empty())
        {
            continue;
        }

        ID3D11Buffer* textured_vb = nullptr;
        const std::uint64_t upload_t0_us = NowSteadyUs();
        D3D11_BUFFER_DESC bd {};
        bd.ByteWidth = static_cast<UINT>(sizeof(SceneAsyncTexturedData::Vertex) * ready.vertices.size());
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        const D3D11_SUBRESOURCE_DATA sd {ready.vertices.data(), 0, 0};
        const HRESULT hr_vb = m_device->CreateBuffer(&bd, &sd, &textured_vb);
        if (FAILED(hr_vb) || textured_vb == nullptr)
        {
            continue;
        }

        if (tmpl.vertexBuffer != nullptr)
        {
            tmpl.vertexBuffer->Release();
        }
        if (tmpl.indexBuffer != nullptr)
        {
            tmpl.indexBuffer->Release();
            tmpl.indexBuffer = nullptr;
        }

        tmpl.vertexBuffer = textured_vb;
        tmpl.indexCount = 0;
        tmpl.textured = true;
        tmpl.texturedVertexCount = static_cast<UINT>(ready.vertices.size());
        tmpl.texturedBatches = std::move(ready.batches);

        if (m_scene_tex_timing_logs_printed < 20u)
        {
            const std::uint64_t now_us = NowSteadyUs();
            const std::uint64_t total_us = (ready.requestUs != 0u && now_us >= ready.requestUs) ? (now_us - ready.requestUs) : 0u;
            const std::uint64_t upload_us = now_us - upload_t0_us;
            const std::uint64_t queue_wait_us =
                (total_us > ready.workerTotalUs + upload_us) ? (total_us - ready.workerTotalUs - upload_us) : 0u;
            std::printf(
                "[SceneImport][TexPipeline] #%u model=%s total=%llu us queue=%llu us build=%llu us worker=%llu us upload=%llu us tris=%u\n",
                static_cast<unsigned>(m_scene_tex_timing_logs_printed + 1u),
                ready.modelLower.c_str(),
                static_cast<unsigned long long>(total_us),
                static_cast<unsigned long long>(queue_wait_us),
                static_cast<unsigned long long>(ready.buildUs),
                static_cast<unsigned long long>(ready.workerTotalUs),
                static_cast<unsigned long long>(upload_us),
                static_cast<unsigned>(ready.triCount));
            std::fflush(stdout);
            ++m_scene_tex_timing_logs_printed;
        }

        ++applied;
    }

    if (pump_added_dff_draws)
    {
        m_scene_dff_draw_list_sort_dirty = true;
    }

    refresh_debug_scene_stats(debug_ui);
}

void Dx11RenderPipeline::ImportSceneFromLoader(const GameLoader& loader, DebugUi& debug_ui, bool useQuaternionRotation)
{
    stop_scene_dff_worker();
    release_imported_scene_dff_gpu();
    release_imported_scene_instance_data();

    m_scene_placements.clear();
    m_scene_placement_indices_by_model.clear();
    m_scene_cube_instances_cpu.clear();
    m_scene_cube_instances_dirty = false;
    m_scene_import_use_quaternion = useQuaternionRotation;

    DebugUi::SceneStats stats {};

    if (m_device == nullptr)
    {
        debug_ui.SetSceneStats(stats);
        return;
    }
    const GameLoader::LoadState load_st = loader.GetLoadState();
    if (load_st != GameLoader::LoadState::Loaded)
    {
        debug_ui.SetSceneStats(stats);
        return;
    }

    if (!ensure_imported_scene_pipeline(m_device))
    {
        debug_ui.SetSceneStats(stats);
        return;
    }

    constexpr std::size_t kMaxInstances = 400000u;
    const auto access = loader.AcquireRead();

    std::unordered_map<std::int32_t, IdeSceneMeta> ide_by_id;
    CollectIdeMapForScene(access, ide_by_id);

    std::vector<SceneInstanceGpu> instances;
    instances.reserve(32768);
    std::unordered_set<std::string> uniq_models;

    std::size_t inst_total = 0;

    using namespace DirectX;
    constexpr float kPlaceholderHalfExtent = 2.5f;

    bool cap_hit = false;
    for (const auto& ipl : access.iplResults)
    {
        for (const auto& inst : ipl.data.inst)
        {
            ++inst_total;

            const IdeSceneMeta* meta = nullptr;
            const auto ide_it = ide_by_id.find(inst.objectId);
            if (ide_it != ide_by_id.end())
            {
                meta = &ide_it->second;
            }
            const float ide_draw_dist = MaxIdeDrawDistance(meta);

            const std::string stem = InstModelStemLower(inst, meta);
            if (!stem.empty())
            {
                uniq_models.insert(stem);
            }

            XMMATRIX rot_m = XMMatrixIdentity();
            if (useQuaternionRotation)
            {
                XMVECTOR q = XMVectorSet(
                    static_cast<float>(inst.rotation.x),
                    static_cast<float>(inst.rotation.y),
                    static_cast<float>(inst.rotation.z),
                    static_cast<float>(inst.rotation.w));
                const float len = XMVectorGetX(XMVector4Length(q));
                if (len > 1.0e-6f)
                {
                    q = XMVectorScale(q, 1.0f / len);
                    rot_m = XMMatrixRotationQuaternion(q);
                }
            }
            else
            {
                rot_m = XMMatrixRotationRollPitchYaw(
                    static_cast<float>(inst.rotation.x),
                    static_cast<float>(inst.rotation.y),
                    static_cast<float>(inst.rotation.z));
            }

            const XMMATRIX scale_m = XMMatrixScaling(kPlaceholderHalfExtent, kPlaceholderHalfExtent, kPlaceholderHalfExtent);
            const XMMATRIX trans_m = XMMatrixTranslation(
                static_cast<float>(inst.position.x),
                static_cast<float>(inst.position.y),
                static_cast<float>(inst.position.z));
            const XMMATRIX world = XMMatrixMultiply(XMMatrixMultiply(scale_m, rot_m), trans_m);

            DirectX::XMFLOAT4X4 wstore {};
            XMStoreFloat4x4(&wstore, world);
            SceneInstanceGpu gpu {};
            gpu.row0 = DirectX::XMFLOAT4(wstore._11, wstore._12, wstore._13, wstore._14);
            gpu.row1 = DirectX::XMFLOAT4(wstore._21, wstore._22, wstore._23, wstore._24);
            gpu.row2 = DirectX::XMFLOAT4(wstore._31, wstore._32, wstore._33, wstore._34);
            gpu.row3 = DirectX::XMFLOAT4(wstore._41, wstore._42, wstore._43, wstore._44);
            gpu.color = ColorFromObjectId(inst.objectId);
            gpu.extra = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);

            const std::size_t cube_slot = instances.size();
            instances.push_back(gpu);

            if (!stem.empty())
            {
                SceneImportPlacement placement {};
                placement.modelStemLower = stem;
                placement.position = DirectX::XMFLOAT3(
                    static_cast<float>(inst.position.x),
                    static_cast<float>(inst.position.y),
                    static_cast<float>(inst.position.z));
                placement.rotation = DirectX::XMFLOAT4(
                    static_cast<float>(inst.rotation.x),
                    static_cast<float>(inst.rotation.y),
                    static_cast<float>(inst.rotation.z),
                    static_cast<float>(inst.rotation.w));
                placement.color = gpu.color;
                placement.cubeInstanceIndex = cube_slot;
                placement.objectId = inst.objectId;
                placement.drawDistanceMax = ide_draw_dist;
                m_scene_placement_indices_by_model[stem].push_back(m_scene_placements.size());
                m_scene_placements.push_back(std::move(placement));
            }

            if (instances.size() >= kMaxInstances)
            {
                cap_hit = true;
                break;
            }
        }
        if (cap_hit)
        {
            break;
        }
    }

    m_scene_stat_inst_total = inst_total;
    m_scene_stat_unique_models = uniq_models.size();

    stats.instCount = inst_total;
    stats.uniqueModelCount = uniq_models.size();
    stats.cubeInstances = instances.size();
    stats.dffModelsQueued = uniq_models.size();

    if (instances.empty())
    {
        debug_ui.SetSceneStats(stats);
        return;
    }

    m_scene_cube_instances_cpu = instances;

    D3D11_BUFFER_DESC bd {};
    bd.ByteWidth = static_cast<UINT>(sizeof(SceneInstanceGpu) * instances.size());
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bd.StructureByteStride = sizeof(SceneInstanceGpu);

    D3D11_SUBRESOURCE_DATA srd {};
    srd.pSysMem = instances.data();

    HRESULT hr = m_device->CreateBuffer(&bd, &srd, &m_buf_scene_instances);
    if (FAILED(hr) || m_buf_scene_instances == nullptr)
    {
        debug_ui.SetSceneStats(stats);
        return;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvd {};
    srvd.Format = DXGI_FORMAT_UNKNOWN;
    srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvd.Buffer.FirstElement = 0;
    srvd.Buffer.NumElements = static_cast<UINT>(instances.size());

    hr = m_device->CreateShaderResourceView(m_buf_scene_instances, &srvd, &m_srv_scene_instances);
    if (FAILED(hr) || m_srv_scene_instances == nullptr)
    {
        m_buf_scene_instances->Release();
        m_buf_scene_instances = nullptr;
        debug_ui.SetSceneStats(stats);
        return;
    }

    m_scene_instance_count = static_cast<std::uint32_t>(instances.size());

    std::vector<std::string> models_sorted(uniq_models.begin(), uniq_models.end());
    std::sort(models_sorted.begin(), models_sorted.end());

    std::vector<std::pair<std::string, std::shared_ptr<const d11::data::tImgParseResult>>> arch_snap;
    arch_snap.reserve(access.imgByAbsPath.size());
    for (const auto& kv : access.imgByAbsPath)
    {
        arch_snap.push_back(kv);
    }

    const std::filesystem::path game_root = loader.GetGameRootPath();
    const GameLoader* loader_ptr = &loader;
    {
        std::lock_guard<std::mutex> lock(m_preview_texture_cache_mutex);
        ensure_model_preview_sampler_and_white_tex(m_device);
    }
    const std::uint64_t gen = m_scene_dff_gen.fetch_add(1u, std::memory_order_acq_rel) + 1u;
    try
    {
        m_scene_dff_worker = std::thread(
            [this,
             gen,
             game_root,
             arch_snap = std::move(arch_snap),
             models_sorted = std::move(models_sorted)]() mutable
            {
                scene_dff_worker_main(gen, game_root, std::move(arch_snap), std::move(models_sorted));
            });
        m_scene_texturing_worker = std::thread([this, gen, loader_ptr]() {
            scene_texturing_worker_main(gen, loader_ptr);
        });
    }
    catch (...)
    {
        // std::thread can throw; сцена останется на кубах
    }

    refresh_debug_scene_stats(debug_ui);
}

void Dx11RenderPipeline::Shutdown()
{
    stop_scene_dff_worker();
    release_txd_inspect_thumbnails();
    release_model_preview_gpu_all();
    release_imported_scene_dff_gpu();
    release_imported_scene_gpu();
    if (m_rs_lines)
    {
        m_rs_lines->Release();
        m_rs_lines = nullptr;
    }
    if (m_rs_axes)
    {
        m_rs_axes->Release();
        m_rs_axes = nullptr;
    }
    if (m_blend_opaque)
    {
        m_blend_opaque->Release();
        m_blend_opaque = nullptr;
    }
    if (m_blend_alpha)
    {
        m_blend_alpha->Release();
        m_blend_alpha = nullptr;
    }
    if (m_cb_mvp)
    {
        m_cb_mvp->Release();
        m_cb_mvp = nullptr;
    }
    if (m_cb_axes)
    {
        m_cb_axes->Release();
        m_cb_axes = nullptr;
    }
    if (m_vs_grid)
    {
        m_vs_grid->Release();
        m_vs_grid = nullptr;
    }
    if (m_ps_grid)
    {
        m_ps_grid->Release();
        m_ps_grid = nullptr;
    }
    if (m_il_grid)
    {
        m_il_grid->Release();
        m_il_grid = nullptr;
    }
    if (m_vb_grid)
    {
        m_vb_grid->Release();
        m_vb_grid = nullptr;
    }
    m_grid_vertex_count = 0;

    if (m_vs_axes)
    {
        m_vs_axes->Release();
        m_vs_axes = nullptr;
    }
    if (m_ps_axes)
    {
        m_ps_axes->Release();
        m_ps_axes = nullptr;
    }
    if (m_il_axes)
    {
        m_il_axes->Release();
        m_il_axes = nullptr;
    }
    if (m_vb_axes)
    {
        m_vb_axes->Release();
        m_vb_axes = nullptr;
    }
    m_axes_vertex_count = 0;

    if (m_depth_state)
    {
        m_depth_state->Release();
        m_depth_state = nullptr;
    }
    if (m_depth_dsv)
    {
        m_depth_dsv->Release();
        m_depth_dsv = nullptr;
    }
    if (m_depth_tex)
    {
        m_depth_tex->Release();
        m_depth_tex = nullptr;
    }
    m_device = nullptr;
    m_width = 0;
    m_height = 0;
    m_msaa_samples = 1;
    m_draw_grid = true;
    m_draw_axes = true;
}

void Dx11RenderPipeline::Resize(ID3D11Device* device, std::uint32_t width, std::uint32_t height)
{
    if (width == 0 || height == 0 || device == nullptr)
    {
        return;
    }
    m_width = width;
    m_height = height;
    create_depth(device, width, height);
}

void Dx11RenderPipeline::DrawFrame(
    ID3D11DeviceContext* context,
    ID3D11RenderTargetView* rtv,
    const GtaStyleCamera& camera,
    std::uint32_t viewport_width,
    std::uint32_t viewport_height)
{
    if (!context || !rtv || !m_depth_dsv || !m_cb_mvp || !m_cb_axes || !m_blend_alpha || !m_blend_opaque || !m_rs_lines ||
        !m_rs_axes)
    {
        return;
    }

    const float aspect = (viewport_height > 0) ? (static_cast<float>(viewport_width) / static_cast<float>(viewport_height)) : 1.0f;
    DirectX::XMFLOAT4X4 mvp_store{};
    DirectX::XMStoreFloat4x4(&mvp_store, camera.WorldViewProjection(aspect));

    const float clear_color[4] = {0.02f, 0.02f, 0.05f, 1.0f};
    context->ClearRenderTargetView(rtv, clear_color);
    context->ClearDepthStencilView(m_depth_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    context->OMSetDepthStencilState(m_depth_state, 0);

    ID3D11RenderTargetView* rtvs[] = {rtv};
    context->OMSetRenderTargets(1, rtvs, m_depth_dsv);

    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<FLOAT>(viewport_width);
    vp.Height = static_cast<FLOAT>(viewport_height);
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    context->RSSetViewports(1, &vp);

    const float blend_factor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    const UINT sample_mask = 0xffffffffu;

    context->OMSetDepthStencilState(m_depth_state, 0);
    map_mvp(context, mvp_store);
    ID3D11Buffer* cb = m_cb_mvp;
    context->VSSetConstantBuffers(0, 1, &cb);

    context->RSSetState(m_rs_lines);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    if (m_draw_grid && m_vb_grid && m_grid_vertex_count > 0 && m_vs_grid && m_ps_grid && m_il_grid)
    {
        context->OMSetBlendState(m_blend_alpha, blend_factor, sample_mask);
        const UINT stride = sizeof(float) * 3;
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &m_vb_grid, &stride, &offset);
        context->IASetInputLayout(m_il_grid);
        context->VSSetShader(m_vs_grid, nullptr, 0);
        context->PSSetShader(m_ps_grid, nullptr, 0);
        context->Draw(m_grid_vertex_count, 0);
    }

    if (m_draw_axes && m_vb_axes && m_axes_vertex_count > 0 && m_vs_axes && m_ps_axes && m_il_axes)
    {
        map_axes_cb(context, camera, aspect, viewport_width, viewport_height);
        context->OMSetBlendState(m_blend_opaque, blend_factor, sample_mask);
        context->RSSetState(m_rs_axes);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        const UINT stride = sizeof(float) * 9;
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, &m_vb_axes, &stride, &offset);
        context->IASetInputLayout(m_il_axes);
        context->VSSetShader(m_vs_axes, nullptr, 0);
        context->PSSetShader(m_ps_axes, nullptr, 0);
        ID3D11Buffer* cb_axes = m_cb_axes;
        context->VSSetConstantBuffers(0, 1, &cb_axes);
        context->Draw(m_axes_vertex_count, 0);
    }

    /** Порядок как в d11_sa_old_code SceneRenderer::RenderFrame: после сетки/осей — сначала DFF, затем IPL-кубы
     *  (меши сцены, затем заглушки поверх по глубине/порядку). Sky/Water в старом клиенте — до сетки; здесь пока нет. */
    draw_imported_scene_dff_meshes(context, camera, aspect);
    draw_imported_scene(context, camera, aspect, viewport_width, viewport_height);

    context->RSSetState(nullptr);
    context->OMSetBlendState(nullptr, nullptr, sample_mask);
}

} // namespace d11::render
