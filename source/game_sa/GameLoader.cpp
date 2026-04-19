#include "GameLoader.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <algorithm>
#include <utility>
#include <unordered_set>

namespace
{
std::filesystem::path GetExecutableFilePath()
{
    wchar_t buf[MAX_PATH];
    const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
    {
        return {};
    }
    return std::filesystem::path(buf);
}

std::string MakePathKey(const std::filesystem::path& p)
{
    std::string s = p.lexically_normal().string();
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return s;
}
}

GameLoader::~GameLoader()
{
    StopAsync();
}

void GameLoader::ResetUnlocked()
{
    m_loaded = false;
    m_gtaDatSummary = {};
    m_gameRoot.clear();
    m_idePaths.clear();
    m_ideResults.clear();
    m_iplPaths.clear();
    m_iplResults.clear();
    m_imgByAbsPath.clear();
    m_jobs.clear();
    m_nextJob.store(0, std::memory_order_release);
    m_imgTotalCount.store(0, std::memory_order_release);
    m_imgParsedCount.store(0, std::memory_order_release);
}

bool GameLoader::Load()
{
    StopAsync();
    {
        std::unique_lock<std::shared_mutex> lk(m_mutex);
        ResetUnlocked();
        m_state.store(LoadState::Loading, std::memory_order_release);
    }

    const std::filesystem::path exe = GetExecutableFilePath();
    if (exe.empty())
    {
        std::unique_lock<std::shared_mutex> lk(m_mutex);
        m_gtaDatSummary.errorMessage = "GetModuleFileNameW не вернул путь к exe.";
        m_state.store(LoadState::Failed, std::memory_order_release);
        return false;
    }

    {
        std::unique_lock<std::shared_mutex> lk(m_mutex);
        m_gameRoot = exe.parent_path();
    }
    const std::filesystem::path gtaDat = m_gameRoot / "data" / "gta.dat";

    {
        std::unique_lock<std::shared_mutex> lk(m_mutex);
        d11::data::LoadGtaDatCatalog(gtaDat, m_gtaDatSummary);
    }

    if (!m_gtaDatSummary.errorMessage.empty())
    {
        m_state.store(LoadState::Failed, std::memory_order_release);
        return false;
    }

    // Парсим IDE/IPL файлы, перечисленные в gta.dat.
    // Пути в gta.dat обычно относительные к корню игры.
    for (const auto& e : m_gtaDatSummary.loadOrder)
    {
        if (e.pathOrText.empty())
        {
            continue;
        }

        const std::filesystem::path rel = std::filesystem::path(e.pathOrText);
        const std::filesystem::path abs = (rel.is_absolute() ? rel : (m_gameRoot / rel));

        if (e.keyword == d11::data::eDatKeyword::Ide)
        {
            std::unique_lock<std::shared_mutex> lk(m_mutex);
            m_idePaths.push_back(abs);
            m_ideResults.push_back(d11::data::ParseIdeFile(abs.string()));
        }
        else if (e.keyword == d11::data::eDatKeyword::Ipl && !e.skippedAsZoneIpl)
        {
            std::unique_lock<std::shared_mutex> lk(m_mutex);
            m_iplPaths.push_back(abs);
            m_iplResults.push_back(d11::data::ParseIplFile(abs.string()));
        }
    }

    {
        std::unique_lock<std::shared_mutex> lk(m_mutex);
        m_loaded = true;
        m_state.store(LoadState::Loaded, std::memory_order_release);
    }
    return true;
}

void GameLoader::StartLoadAsync()
{
    StopAsync();

    m_stop.store(false, std::memory_order_release);
    m_state.store(LoadState::Loading, std::memory_order_release);

    m_loaderThread = std::thread([this]()
    {
        {
            std::unique_lock<std::shared_mutex> lk(m_mutex);
            ResetUnlocked();
        }

        const std::filesystem::path exe = GetExecutableFilePath();
        if (exe.empty())
        {
            std::unique_lock<std::shared_mutex> lk(m_mutex);
            m_gtaDatSummary.errorMessage = "GetModuleFileNameW не вернул путь к exe.";
            m_state.store(LoadState::Failed, std::memory_order_release);
            return;
        }

        {
            std::unique_lock<std::shared_mutex> lk(m_mutex);
            m_gameRoot = exe.parent_path();
        }

        const std::filesystem::path gtaDat = m_gameRoot / "data" / "gta.dat";
        d11::data::tGtaDatParseSummary localSummary;
        d11::data::LoadGtaDatCatalog(gtaDat, localSummary);

        if (!localSummary.errorMessage.empty())
        {
            std::unique_lock<std::shared_mutex> lk(m_mutex);
            m_gtaDatSummary = std::move(localSummary);
            m_state.store(LoadState::Failed, std::memory_order_release);
            return;
        }

        std::vector<Job> phase1Jobs;
        std::vector<Job> imgJobs;
        phase1Jobs.reserve(localSummary.loadOrder.size());
        imgJobs.reserve(localSummary.loadOrder.size());
        std::size_t ideCount = 0;
        std::size_t iplCount = 0;
        std::size_t imgCount = 0;

        auto pushUniqueImgJob = [&](d11::data::eDatKeyword keyword, const std::filesystem::path& abs, std::unordered_set<std::string>& seenLower)
        {
            const std::string key = MakePathKey(abs);
            if (!seenLower.insert(key).second)
            {
                return;
            }
            imgJobs.push_back(Job{keyword, abs});
            ++imgCount;
        };

        std::unordered_set<std::string> imgSeenLower;
        imgSeenLower.reserve(256);
        for (const auto& e : localSummary.loadOrder)
        {
            if (e.pathOrText.empty())
            {
                continue;
            }
            const std::filesystem::path rel = std::filesystem::path(e.pathOrText);
            const std::filesystem::path abs = (rel.is_absolute() ? rel : (m_gameRoot / rel));
            if (e.keyword == d11::data::eDatKeyword::Ide)
            {
                phase1Jobs.push_back(Job{e.keyword, abs});
                ++ideCount;
            }
            else if (e.keyword == d11::data::eDatKeyword::Ipl && !e.skippedAsZoneIpl)
            {
                phase1Jobs.push_back(Job{e.keyword, abs});
                ++iplCount;
            }
            else if (e.keyword == d11::data::eDatKeyword::Img ||
                     e.keyword == d11::data::eDatKeyword::CdImage ||
                     e.keyword == d11::data::eDatKeyword::ImgList)
            {
                std::string ext = abs.extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                if (ext == ".img")
                {
                    pushUniqueImgJob(e.keyword, abs, imgSeenLower);
                }
            }
        }

        // Also include game_root/models/*.img
        {
            const std::filesystem::path modelsDir = m_gameRoot / "models";
            std::error_code ec;
            if (std::filesystem::exists(modelsDir, ec))
            {
                for (const auto& it : std::filesystem::directory_iterator(modelsDir, ec))
                {
                    if (ec)
                    {
                        break;
                    }
                    if (!it.is_regular_file(ec))
                    {
                        continue;
                    }
                    const std::filesystem::path p = it.path();
                    std::string ext = p.extension().string();
                    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
                    if (ext == ".img")
                    {
                        pushUniqueImgJob(d11::data::eDatKeyword::Img, p, imgSeenLower);
                    }
                }
            }
        }

        {
            std::unique_lock<std::shared_mutex> lk(m_mutex);
            m_gtaDatSummary = std::move(localSummary);
            m_jobs = std::move(phase1Jobs);
            m_nextJob.store(0, std::memory_order_release);
            m_idePaths.reserve(ideCount);
            m_ideResults.reserve(ideCount);
            m_iplPaths.reserve(iplCount);
            m_iplResults.reserve(iplCount);
            m_imgByAbsPath.reserve(imgCount);
        }

        // Phase 1: IDE/IPL in parallel.
        // Note: hardware_concurrency may return 0, so clamp to 1.
        unsigned int hw = std::thread::hardware_concurrency();
        if (hw == 0u)
        {
            hw = 1u;
        }
        std::size_t workerCount = static_cast<std::size_t>(hw);
        {
            std::shared_lock<std::shared_mutex> lk(m_mutex);
            workerCount = (std::min)(workerCount, m_jobs.size());
        }
        if (workerCount == 0)
        {
            workerCount = 1;
        }

        m_workers.clear();
        m_workers.reserve(workerCount);
        for (std::size_t i = 0; i < workerCount; ++i)
        {
            m_workers.emplace_back([this]() { WorkerMain(); });
        }
        for (auto& t : m_workers)
        {
            if (t.joinable())
            {
                t.join();
            }
        }
        m_workers.clear();

        if (m_stop.load(std::memory_order_acquire))
        {
            return;
        }

        // Phase 2: IMG parsing is IO-heavy; limit concurrency to keep UI responsive.
        {
            std::unique_lock<std::shared_mutex> lk(m_mutex);
            m_jobs = std::move(imgJobs);
            m_nextJob.store(0, std::memory_order_release);
        }
        m_imgTotalCount.store(imgCount, std::memory_order_release);
        m_imgParsedCount.store(0, std::memory_order_release);
        std::size_t imgWorkerCount = 2;
        {
            std::shared_lock<std::shared_mutex> lk(m_mutex);
            imgWorkerCount = (std::min)(imgWorkerCount, m_jobs.size());
        }
        if (imgWorkerCount == 0)
        {
            imgWorkerCount = 1;
        }

        m_workers.clear();
        m_workers.reserve(imgWorkerCount);
        for (std::size_t i = 0; i < imgWorkerCount; ++i)
        {
            m_workers.emplace_back([this]() { WorkerMain(); });
        }
        for (auto& t : m_workers)
        {
            if (t.joinable())
            {
                t.join();
            }
        }
        m_workers.clear();

        if (m_stop.load(std::memory_order_acquire))
        {
            return;
        }

        {
            std::unique_lock<std::shared_mutex> lk(m_mutex);
            m_loaded = true;
        }
        m_state.store(LoadState::Loaded, std::memory_order_release);
    });
}

void GameLoader::StopAsync()
{
    m_stop.store(true, std::memory_order_release);

    // Только m_loaderThread владеет join() для m_workers. Если здесь тоже join’ить воркеры,
    // получается двойной join тем же std::thread — invalid handle / abort при закрытии.
    if (m_loaderThread.joinable())
    {
        m_loaderThread.join();
    }
    m_workers.clear();

    m_stop.store(false, std::memory_order_release);
}

void GameLoader::WorkerMain()
{
    while (!m_stop.load(std::memory_order_acquire))
    {
        const std::size_t idx = m_nextJob.fetch_add(1, std::memory_order_acq_rel);
        Job job;
        {
            std::shared_lock<std::shared_mutex> lk(m_mutex);
            if (idx >= m_jobs.size())
            {
                return;
            }
            job = m_jobs[idx];
        }

        if (job.keyword == d11::data::eDatKeyword::Ide)
        {
            d11::data::tIdeParseResult parsed = d11::data::ParseIdeFile(job.absPath.string());
            std::unique_lock<std::shared_mutex> lk(m_mutex);
            m_idePaths.push_back(job.absPath);
            m_ideResults.push_back(std::move(parsed));
        }
        else if (job.keyword == d11::data::eDatKeyword::Ipl)
        {
            d11::data::tIplParseResult parsed = d11::data::ParseIplFile(job.absPath.string());
            std::unique_lock<std::shared_mutex> lk(m_mutex);
            m_iplPaths.push_back(job.absPath);
            m_iplResults.push_back(std::move(parsed));
        }
        else if (job.keyword == d11::data::eDatKeyword::Img ||
                 job.keyword == d11::data::eDatKeyword::CdImage ||
                 job.keyword == d11::data::eDatKeyword::ImgList)
        {
            const std::string absKey = MakePathKey(job.absPath);
            {
                std::shared_lock<std::shared_mutex> lk(m_mutex);
                if (m_imgByAbsPath.find(absKey) != m_imgByAbsPath.end())
                {
                    continue;
                }
            }

            auto parsed = std::make_shared<d11::data::tImgParseResult>(d11::data::ParseImgArchiveFile(job.absPath.string()));
            std::unique_lock<std::shared_mutex> lk(m_mutex);
            m_imgByAbsPath.emplace(absKey, std::move(parsed));
            m_imgParsedCount.fetch_add(1, std::memory_order_acq_rel);
        }
    }
}

std::filesystem::path GameLoader::GetGtaDatPath() const
{
    if (m_gameRoot.empty())
    {
        return {};
    }
    return m_gameRoot / "data" / "gta.dat";
}
