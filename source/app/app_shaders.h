#pragma once

// Заготовка под слой HLSL / компиляцию и биндинг шейдеров (DX11; в gta-reversed см. пайплайн RenderWare / D3D9).
namespace d11 {
namespace app {

class AppShaders {
public:
    AppShaders() = default;
    ~AppShaders() = default;

    AppShaders(const AppShaders&) = delete;
    AppShaders& operator=(const AppShaders&) = delete;
    AppShaders(AppShaders&&) = delete;
    AppShaders& operator=(AppShaders&&) = delete;
};

} // namespace app
} // namespace d11
