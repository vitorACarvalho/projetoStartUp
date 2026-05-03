#pragma once

#include "IniFileParser.h"
#include <atomic>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

/**
 * @class ThreadWorker
 * @brief Dedicated thread responsible for sending UDP packets at a fixed rate.
 *
 * This worker receives a ConnectionConfig and:
 *  - Initializes a UDP socket
 *  - Calculates the sending period (from the "rate" field)
 *  - Runs a continuous loop sending the configured payload
 *  - Attempts to maintain a stable sending frequency using steady_clock
 *
 * Each ThreadWorker instance manages its own thread and socket.
 */
class ThreadWorker
{
public:

    /**
     * @brief Constructs the worker and starts the processing thread.
     *
     * The constructor makes an internal copy of ConnectionConfig.
     * It also initializes the socket and pre-calculates the sending interval.
     *
     * @param config Parsed configuration describing destination, payload, and sending rate.
     */
    ThreadWorker(const ConnectionConfig& config);

    /**
     * @brief Signals the worker thread to stop, joins it, then closes the socket.
     */
    ~ThreadWorker();

    /**
     * @brief Signals the worker to stop and waits for the thread to exit.
     *
     * The thread exits after completing its current sleep interval.
     * The destructor calls this automatically, so explicit use is optional.
     */
    inline void Join()
    {
        m_running = false;
        if (m_threadHandler.joinable())
            m_threadHandler.join();
    }

private:

    /**
     * @brief Main loop executed inside the worker thread.
     *
     * Steps:
     *  1. Wait until the next scheduled execution time
     *  2. Send the UDP payload to the destination IP/port
     *  3. Compute next execution timestamp
     *
     * Timing uses steady_clock.
     */
    void RunLoop();

private:
    ConnectionConfig m_config;         /**< Full copy of configuration for thread-safe access. */
    std::atomic<bool> m_running;       /**< Controls the run loop; set to false to stop the thread. */

    std::chrono::milliseconds m_period;     /**< Interval between packet transmissions. */
    std::chrono::steady_clock::time_point m_nextTimeToExecute{}; /**< Next scheduled time for packet sending. */

    SOCKET m_socket;              /**< UDP socket used for transmission. */
    std::thread m_threadHandler;  /**< Internal worker thread. */

    sockaddr_in m_destInfo;       /**< Pre-built destination sockaddr for fast sending. */
};