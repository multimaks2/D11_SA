#include <Windows.h>

#include <clocale>
#include <cstdio>

#include "app/Application.h"

// Подсистема Windows (/SUBSYSTEM:WINDOWS) — точка входа WinMain/wWinMain, не main().
int WINAPI wWinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPWSTR lpCmdLine,int nShowCmd)
{
//#ifdef _DEBUG
    AllocConsole();
    SetConsoleTitleW(L"D11_SA Debug Console");
    FILE* fs = nullptr;
    freopen_s(&fs, "CONIN$", "r", stdin);
    freopen_s(&fs, "CONOUT$", "w", stdout);
    freopen_s(&fs, "CONOUT$", "w", stderr);
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    setlocale(LC_ALL, "Russian");
//#endif

    app::Application app{};
    return app.Run();
}
