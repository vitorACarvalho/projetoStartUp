#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <functional>
#include <optional>
#include <vector>

/**
 * @struct ConnectionConfig
 * @brief Holds configuration information for a packet-sending connection.
 *
 * This structure stores payload data, destination address/port, and
 * transmission rate. It also provides helper methods for assigning values
 * from strings.
 */
struct ConnectionConfig {
    std::string destination_ip;       /**< Destination IPv4 address as string. */
    std::string source_ip;            /**< Source IPv4 address as string. */
    size_t payload_length;            /**< Size of the payload in bytes. */
    uint16_t destination_port;        /**< Destination UDP port. */
    uint16_t rate;                    /**< Sending Period in ms. */
    uint16_t source_port;             /**< Source UDP port. */
    std::vector<uint8_t> payload;      /**< Payload buffer. */

    /**
     * @brief Default constructor.
     *
     * Initializes all fields to zero or empty values.
     */
    ConnectionConfig() :
        destination_ip(""),
        source_ip(""),
        payload_length(0),
        destination_port(0),
        rate(0),
        source_port(0)
    {
    }

    /**
     * @brief Prints the contents of this configuration to stdout.
     *
     * Useful for debugging and validation.
     */
    void Print()
    {
        std::cout << "PayloadLength = " << payload_length << std::endl;
        if (!payload.empty())
        {
            std::cout << "payload = " << reinterpret_cast<const char*>(payload.data()) << std::endl;
        }
        std::cout << "destination_ip = " << destination_ip << std::endl;
        std::cout << "destination_port = " << destination_port << std::endl;
        std::cout << "rate = " << rate << std::endl;
    }

    /**
     * @brief Sets the payload length from a string value.
     * @param value String representing a numerical byte length.
     */
    void SetPayloadLength(const std::string& value)
    {
        payload_length = static_cast<size_t>(std::stoi(value));
    }

    /**
     * @brief Sets the payload data from a string.
     *
     * Copies the raw bytes of the string into the payload buffer and updates payload_length.
     *
     * @param value Raw payload string.
     */
    void SetPayload(const std::string& value)
    {
        payload.assign(value.begin(), value.end());
        payload_length = payload.size();
    }

    /**
     * @brief Sets the destination IP address.
     * @param value IP address as string.
     */
    void SetDestinationIp(const std::string& value)
    {
        destination_ip = value;
    }

    /**
     * @brief Sets the destination UDP port from a string.
     * @param value String representing the port number.
     */
    void SetDestinationPort(const std::string& value)
    {
        destination_port = static_cast<uint16_t>(std::stoi(value));
    }

    /**
     * @brief Sets the sending period from a string.
     * @param value String representing the sending period in milliseconds.
     */
    void SetRate(const std::string& value)
    {
        rate = static_cast<uint16_t>(std::stoi(value));
    }

    /**
     * @brief Sets the Source UDP port from a string.
     * @param value String representing the port number.
     */
    void SetSourcePort(const std::string& value)
    {
        source_port = static_cast<uint16_t>(std::stoi(value));
    }

    /**
    * @brief Sets the Source IP address.
    * @param value IP address as string.
    */
    void SetSourceIp(const std::string& value)
    {
        source_ip = value;
    }
};

/**
 * @class IniFileParser
 * @brief Parses INI configuration files to produce a list of ConnectionConfig objects.
 *
 * The parser supports key/value extraction, dispatch-mapping for known keys,
 * and automatic struct filling. The output is stored in a user-provided list.
 */
class IniFileParser
{
public:
    /**
     * @brief Constructs the parser and immediately attempts to read the file.
     *
     * @param filename Path to the INI file.
     * @param out Reference to a list where parsed ConnectionConfig entries will be stored.
     */
    IniFileParser(const std::string& filename, std::list<ConnectionConfig>& out);

private:
    /**
     * @brief Processes the INI file line-by-line, populating m_outputList.
     *
     * Called internally by the constructor when the file is successfully opened.
     */
    void parseFile();

    /**
     * @brief Splits a string of the form "key=value" into its components.
     *
     * @param input Raw input line.
     * @param key Output parameter for the extracted key.
     * @param value Output parameter for the extracted value.
     */
    void tokenizerString(const std::string& input, std::string& key, std::string& value);

    /**
     * @brief Trims leading/trailing whitespace and strips inline comments (text after '#').
     *
     * @param s String to trim in-place.
     */
    void trimStr(std::string& s);

    std::ifstream m_file;  /**< Input file stream used for reading the INI file. */

    /**
     * @brief Dispatch table mapping INI keys to handler functions.
     *
     * Each handler receives a pointer to a ConnectionConfig instance and a string value.
     */
    std::map<std::string, std::function<void(std::optional<ConnectionConfig>&, const std::string&)>> m_dispatch;

    std::list<ConnectionConfig>& m_outputList; /**< Reference to the output list of parsed configurations. */
};
