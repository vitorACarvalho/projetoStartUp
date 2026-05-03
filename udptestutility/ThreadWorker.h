#pragma once

#include "IniFileParser.h"
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
     * @brief Destructor ensures that the worker thread has finished.
     *
     * The socket is closed and the thread is joined safely.
     */
    ~ThreadWorker();

    /**
     * @brief Waits for the internal worker thread to finish execution.
     *
     * This must be called before destroying the ThreadWorker
     * if you need to ensure that all messages were sent.
     */
    inline void Join()
    {
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
     * Timing uses steady_clock;
     */
    void RunLoop();

private:
    ConnectionConfig m_config;    /**< Full copy of configuration for thread-safe access. Not the best approach*/

    std::chrono::milliseconds m_period;     /**< Interval between packet transmissions. */
    std::chrono::steady_clock::time_point m_nextTimeToExecute{}; /**< Next scheduled time for packet sending. */

    SOCKET m_socket;              /**< UDP socket used for transmission. */
    std::thread m_threadHandler;  /**< Internal worker thread. */

    sockaddr_in m_destInfo;       /**< Pre-built destination sockaddr for fast sending. */
};