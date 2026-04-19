#pragma once

// Заготовка под высокоуровневую сцену (в gta-reversed см. source/game_sa/Scene.h — CScene).
namespace d11 {
namespace app {

class AppScene {
public:
    AppScene() = default;
    ~AppScene() = default;

    AppScene(const AppScene&) = delete;
    AppScene& operator=(const AppScene&) = delete;
    AppScene(AppScene&&) = delete;
    AppScene& operator=(AppScene&&) = delete;
};

} // namespace app
} // namespace d11
