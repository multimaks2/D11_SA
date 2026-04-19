#include "DiscordManager.h"

#include <cstdio>
#include <ctime>

DiscordManager* DiscordManager::s_owner = nullptr;

namespace
{
constexpr const char* kDefaultDiscordApplicationId = "1494061761391886356";
constexpr const char* kDefaultDiscordState = "В главной сцене";
constexpr const char* kDefaultDiscordDetails = "Рендер: DirectX 11";
constexpr const char* kDefaultDiscordLargeImageText = "D11_SA";
constexpr const char* kDefaultDiscordSmallImageKey = "t_radar_cj_bc_1";
constexpr const char* kDefaultDiscordSmallImageText = "CJ ожидает создания игры";
}

DiscordManager::~DiscordManager()
{
    Shutdown();
}

bool DiscordManager::InitializeDefault()
{
    SetState(kDefaultDiscordState);
    SetDetails(kDefaultDiscordDetails);
    SetLargeText(kDefaultDiscordLargeImageText);
    SetSmallImageKey(kDefaultDiscordSmallImageKey);
    SetSmallImageText(kDefaultDiscordSmallImageText);
    SetStartTimestampNow();
    return Initialize(kDefaultDiscordApplicationId);
}

bool DiscordManager::Initialize(const char* applicationId)
{
    if (m_initialized)
    {
        return true;
    }

    DiscordEventHandlers handlers {};
    handlers.ready = HandleReady;
    handlers.disconnected = HandleDisconnected;
    handlers.errored = HandleError;

    Discord_Initialize(applicationId, &handlers, 1, nullptr);
    s_owner = this;
    m_initialized = true;
    m_ready = false;
    m_presencePublished = false;
    std::printf("[Discord] Инициализация запрошена\n");
    std::fflush(stdout);
    return true;
}

void DiscordManager::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    Discord_ClearPresence();
    Discord_Shutdown();
    m_initialized = false;
    m_ready = false;
    m_presencePublished = false;
    if (s_owner == this)
    {
        s_owner = nullptr;
    }
}

void DiscordManager::Tick()
{
    if (!m_initialized)
    {
        return;
    }

#ifdef DISCORD_DISABLE_IO_THREAD
    Discord_UpdateConnection();
#endif
    Discord_RunCallbacks();
    if (m_ready && !m_presencePublished)
    {
        PublishPresence();
    }
}

void DiscordManager::SetState(const std::string& state)
{
    m_state = state;
    m_presencePublished = false;
}

void DiscordManager::SetDetails(const std::string& details)
{
    m_details = details;
    m_presencePublished = false;
}

void DiscordManager::SetLargeText(const std::string& largeText)
{
    m_largeText = largeText;
    m_presencePublished = false;
}

void DiscordManager::SetSmallImageKey(const std::string& smallImageKey)
{
    m_smallImageKey = smallImageKey;
    m_presencePublished = false;
}

void DiscordManager::SetSmallImageText(const std::string& smallImageText)
{
    m_smallImageText = smallImageText;
    m_presencePublished = false;
}

void DiscordManager::SetStartTimestampNow()
{
    m_startTimestamp = static_cast<std::int64_t>(std::time(nullptr));
    m_presencePublished = false;
}

void DiscordManager::PublishPresence()
{
    DiscordRichPresence presence {};
    presence.state = m_state.empty() ? nullptr : m_state.c_str();
    presence.details = m_details.empty() ? nullptr : m_details.c_str();
    presence.largeImageText = m_largeText.empty() ? nullptr : m_largeText.c_str();
    presence.smallImageKey = m_smallImageKey.empty() ? nullptr : m_smallImageKey.c_str();
    presence.smallImageText = m_smallImageText.empty() ? nullptr : m_smallImageText.c_str();
    presence.startTimestamp = m_startTimestamp;
    Discord_UpdatePresence(&presence);
    m_presencePublished = true;
    std::printf("[Discord] Активность опубликована\n");
    std::fflush(stdout);
}

void DiscordManager::HandleReady(const DiscordUser*)
{
    if (!s_owner)
    {
        return;
    }

    s_owner->m_ready = true;
    std::printf("[Discord] Клиент готов (Ready)\n");
    std::fflush(stdout);
}

void DiscordManager::HandleDisconnected(int, const char*)
{
    if (!s_owner)
    {
        return;
    }

    s_owner->m_ready = false;
    s_owner->m_presencePublished = false;
}

void DiscordManager::HandleError(int code, const char* message)
{
    std::printf("[Discord] Ошибка #%d: %s\n", code, message ? message : "unknown");
    std::fflush(stdout);
}
