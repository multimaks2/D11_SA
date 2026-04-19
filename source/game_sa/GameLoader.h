#pragma once

#include <filesystem>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

#include "GameDataFormat.h"

class GameLoader
{
public:
    enum class LoadState : std::uint8_t
    {
        Idle = 0,
        Loading,
        Loaded,
        Failed
    };

    GameLoader() = default;
    ~GameLoader();

    // Legacy synchronous load (kept for debugging/tests).
    bool Load();

    // Non-blocking load. Starts background threads and returns immediately.
    void StartLoadAsync();
    void StopAsync();

    LoadState GetLoadState() const { return m_state.load(std::memory_order_acquire); }

    bool IsLoaded() const { return m_loaded; }

    std::size_t GetImgTotalCount() const { return m_imgTotalCount.load(std::memory_order_acquire); }
    std::size_t GetImgParsedCount() const { return m_imgParsedCount.load(std::memory_order_acquire); }

    struct ReadAccess
    {
        std::shared_lock<std::shared_mutex> lock;
        const d11::data::tGtaDatParseSummary& gtaDatSummary;
        const std::vector<std::filesystem::path>& idePaths;
        const std::vector<d11::data::tIdeParseResult>& ideResults;
        const std::vector<std::filesystem::path>& iplPaths;
        const std::vector<d11::data::tIplParseResult>& iplResults;
        const std::unordered_map<std::string, std::shared_ptr<const d11::data::tImgParseResult>>& imgByAbsPath;
    };

    ReadAccess AcquireRead() const
    {
        return ReadAccess{
            std::shared_lock<std::shared_mutex>(m_mutex),
            m_gtaDatSummary,
            m_idePaths,
            m_ideResults,
            m_iplPaths,
            m_iplResults,
            m_imgByAbsPath
        };
    }

    std::filesystem::path GetGameRootPath() const { return m_gameRoot; }

    std::filesystem::path GetGtaDatPath() const;

private:
    void ResetUnlocked();
    void WorkerMain();

    struct Job
    {
        d11::data::eDatKeyword keyword {d11::data::eDatKeyword::Unknown};
        std::filesystem::path absPath;
    };

    bool m_loaded = false;
    std::filesystem::path m_gameRoot;
    d11::data::tGtaDatParseSummary m_gtaDatSummary {};

    std::vector<std::filesystem::path> m_idePaths;
    std::vector<d11::data::tIdeParseResult> m_ideResults;

    std::vector<std::filesystem::path> m_iplPaths;
    std::vector<d11::data::tIplParseResult> m_iplResults;

    // Parsed IMG archives (absolute path -> parse result).
    std::unordered_map<std::string, std::shared_ptr<const d11::data::tImgParseResult>> m_imgByAbsPath;

    mutable std::shared_mutex m_mutex;

    std::atomic<LoadState> m_state {LoadState::Idle};
    std::atomic_bool m_stop {false};
    std::atomic_size_t m_nextJob {0};
    std::vector<Job> m_jobs;
    std::thread m_loaderThread;
    std::vector<std::thread> m_workers;

    std::atomic_size_t m_imgTotalCount {0};
    std::atomic_size_t m_imgParsedCount {0};
};
