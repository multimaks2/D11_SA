#pragma once

// «Конвейер» кадра: очередь экземпляров для будущей отрисовки мешей (симуляция не тянет D3D).
// Сетка и оси GTA (Z вверх) сейчас рисуются в Dx11RenderPipeline; конвейер можно снова подключить к пайплайну при появлении мешей.

#include <DirectXMath.h>
#include <cstdint>
#include <vector>

namespace d11::render {

// Идентификатор сетки, которую знает бэкенд (пока одна встроенная).
enum class eBuiltInMesh : std::uint8_t
{
    UnitCube = 0
};

struct tRenderMeshInstance
{
    eBuiltInMesh mesh {eBuiltInMesh::UnitCube};
    DirectX::XMFLOAT4X4 world {};
    DirectX::XMFLOAT4 color {1.0f, 1.0f, 1.0f, 1.0f};
};

class RenderConveyor {
public:
    void BeginFrame() { m_items.clear(); }

    void Submit(const tRenderMeshInstance& item) { m_items.push_back(item); }

    [[nodiscard]] const std::vector<tRenderMeshInstance>& Items() const { return m_items; }
    [[nodiscard]] std::size_t Count() const { return m_items.size(); }

private:
    std::vector<tRenderMeshInstance> m_items;
};

} // namespace d11::render
