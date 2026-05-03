#include "ThreadWorker.h"
#include <chrono>

// link with Ws2_32.lib
#pragma comment(lib,"Ws2_32.lib")
#include <winsock2.h>
#include <ws2tcpip.h>

ThreadWorker::ThreadWorker(const ConnectionConfig& config)
	: m_config(config)
	, m_running(false)
	, m_period(std::chrono::milliseconds(0))
	, m_nextTimeToExecute(std::chrono::steady_clock::now())
	, m_socket(INVALID_SOCKET)
	, m_destInfo({})
{
	// Validate payload length
	if (m_config.payload.empty() && m_config.payload_length == 0)
	{
		std::cout << "[ERROR] payload_length cannot be zero." << std::endl;
		return;
	}

	// If no payload was provided, generate a synthetic one automatically
	if (m_config.payload.empty())
	{
		m_config.payload.resize(m_config.payload_length);
		for (size_t i = 0; i < m_config.payload_length; i++)
			m_config.payload[i] = static_cast<uint8_t>(i & 0xFF);
	}

	// Rate validation
	if (m_config.rate == 0)
	{
		std::cout << "[ERROR] Invalid rate (cannot be zero ms)." << std::endl;
		return;
	}

	m_period = std::chrono::milliseconds(m_config.rate);

	// Init WinSock
	WSADATA wsaData{};
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "[ERROR] WSAStartup failed." << std::endl;
		return;
	}

	// Create UDP socket
	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_socket == INVALID_SOCKET)
	{
		std::cout << "[ERROR] Failed to create UDP socket." << std::endl;
		return;
	}

	int optval = 1;
	setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR,
		reinterpret_cast<const char*>(&optval), sizeof(optval));
	// Prepare destination address
	m_destInfo.sin_family = AF_INET;
	m_destInfo.sin_port = htons(m_config.destination_port);

	// Bind to a specific source address/port if configured
	if (m_config.source_port != 0 && !m_config.source_ip.empty())
	{
		sockaddr_in localAddr;
		memset(&localAddr, 0, sizeof(localAddr));
		localAddr.sin_family = AF_INET;
		localAddr.sin_addr.s_addr = INADDR_ANY;
		localAddr.sin_port = htons(m_config.source_port);

		if (inet_pton(AF_INET, m_config.source_ip.c_str(), &localAddr.sin_addr) <= 0)
		{
			std::cout << "[ERROR] Error to set the source ip " << m_config.source_ip << std::endl;
			closesocket(m_socket);
			return;
		}

		if (bind(m_socket, (struct sockaddr*)&localAddr, sizeof(localAddr)) < 0) {
			std::cout << "[ERROR] Error to bind a port " << m_config.source_port << std::endl;
			closesocket(m_socket);
			return;
		}

	}

	if (inet_pton(AF_INET, m_config.destination_ip.c_str(), &m_destInfo.sin_addr) != 1)
	{
		std::cout << "[ERROR] Invalid IP address: " << m_config.destination_ip << std::endl;
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return;
	}

	// Start the worker thread
	m_running = true;
	m_threadHandler = std::thread(&ThreadWorker::RunLoop, this);
}

void ThreadWorker::RunLoop()
{
	while (m_running)
	{
		// Send UDP packet
		int bytesSent = sendto(
			m_socket,
			reinterpret_cast<const char*>(m_config.payload.data()),
			static_cast<int>(m_config.payload_length),
			0,
			reinterpret_cast<sockaddr*>(&m_destInfo),
			sizeof(m_destInfo)
		);

		// Sleep until next execution time
		m_nextTimeToExecute = std::chrono::steady_clock::now() + m_period;
		std::this_thread::sleep_until(m_nextTimeToExecute);
	}
}

ThreadWorker::~ThreadWorker()
{
	Join();

	if (m_socket != INVALID_SOCKET)
		closesocket(m_socket);

	WSACleanup();
}