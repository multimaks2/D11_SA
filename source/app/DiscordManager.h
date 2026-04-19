#pragma once

#include <cstdint>
#include <string>

#include <discord_rpc.h>

class DiscordManager
{
public:
    DiscordManager() = default;
    ~DiscordManager();

    bool InitializeDefault();
    bool Initialize(const char* applicationId);
    void Shutdown();
    void Tick();

    void SetState(const std::string& state);
    void SetDetails(const std::string& details);
    void SetLargeText(const std::string& largeText);
    void SetSmallImageKey(const std::string& smallImageKey);
    void SetSmallImageText(const std::string& smallImageText);
    void SetStartTimestampNow();

private:
    void PublishPresence();

    static void HandleReady(const DiscordUser*);
    static void HandleDisconnected(int, const char*);
    static void HandleError(int code, const char* message);

    static DiscordManager* s_owner;

    std::string m_state;
    std::string m_details;
    std::string m_largeText;
    std::string m_smallImageKey;
    std::string m_smallImageText;

    bool m_initialized = false;
    bool m_ready = false;
    bool m_presencePublished = false;
    std::int64_t m_startTimestamp = 0;
};
