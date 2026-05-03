#include <iostream>
#include <list>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <filesystem>
#include "ConfigurationManager.h"
#include "ThreadWorker.h"

static const std::string DEFAULT_CONFIG_FILE = "./Config.ini";

static std::atomic<bool>       g_stop{false};
static std::condition_variable g_cv;
static std::mutex              g_mutex;

static BOOL WINAPI ConsoleHandler(DWORD signal)
{
    if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT)
    {
        g_stop = true;
        g_cv.notify_one();
        return TRUE;
    }
    return FALSE;
}

int main(int argc, char** argv)
{
    std::string filename;
    if (argc > 1)
    {
        filename = argv[1];
    }
    else
    {
        filename = DEFAULT_CONFIG_FILE;
        std::cout << "No configuration file provided, using default: " << filename << std::endl;
        if (!std::filesystem::exists(filename))
        {
            std::cout << "Default configuration file not found!" << std::endl;
            return -1;
        }
    }

    bool initialized = ConfigurationManager::Instance().Initialize(filename);
    if (!initialized)
    {
        std::cout << "Error on loading configuration!" << std::endl;
        return -1;
    }

    const std::list<ConnectionConfig>& configs = ConfigurationManager::Instance().GetConfiguration();

    std::vector<std::shared_ptr<ThreadWorker>> threads;
    for (const ConnectionConfig& config : configs)
        threads.push_back(std::make_shared<ThreadWorker>(config));

    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    std::cout << "Running. Press Ctrl+C to stop." << std::endl;

    {
        std::unique_lock<std::mutex> lock(g_mutex);
        g_cv.wait(lock, [] { return g_stop.load(); });
    }

    std::cout << "Stopping..." << std::endl;
    for (auto& t : threads)
        t->Join();

    return 0;
}
