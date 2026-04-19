#pragma once

// Заготовка под освещение (в gta-reversed см. source/app/app_light.h — направленный/окружение и т.д.).
namespace d11 {
namespace app {

class AppLight {
public:
    AppLight() = default;
    ~AppLight() = default;

    AppLight(const AppLight&) = delete;
    AppLight& operator=(const AppLight&) = delete;
    AppLight(AppLight&&) = delete;
    AppLight& operator=(AppLight&&) = delete;
};

} // namespace app
} // namespace d11
