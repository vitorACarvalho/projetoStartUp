#include "IniFileParser.h"
#include <iostream>
#include <fstream>
#include <string>

IniFileParser::IniFileParser(const std::string& filename, std::list<ConnectionConfig>& out) :
    m_file(filename),
    m_outputList(out)
{
    // Create a mapping between INI keys and struct-member setter functions.
    // Each key maps to a lambda that modifies the current ConnectionConfig object.
    m_dispatch["[Connection]"] = [&](std::optional<ConnectionConfig>& current, const std::string&)
    {
        if (current.has_value())
        {
            m_outputList.push_back(std::move(*current));
        }
        current.emplace(); // constructs a fresh default ConnectionConfig in-place
    };

    m_dispatch["payload_length"] = [](std::optional<ConnectionConfig>& current, const std::string& value)
    {
        current->SetPayloadLength(value); 
    };

    m_dispatch["payload"] = [](std::optional<ConnectionConfig>& current, const std::string& value)
    {
        current->SetPayload(value);
    };
    m_dispatch["destination_ip"] = [](std::optional<ConnectionConfig>& current, const std::string& value)
    { 
        current->SetDestinationIp(value);
    };
    m_dispatch["destination_port"] = [](std::optional<ConnectionConfig>& current, const std::string& value)
    {
        current->SetDestinationPort(value);
    };
    m_dispatch["sending_period"] = [](std::optional<ConnectionConfig>& current, const std::string& value)
    {
        current->SetRate(value);
    };
    m_dispatch["source_port"] = [](std::optional<ConnectionConfig>& current, const std::string& value)
    {
        current->SetSourcePort(value);
    };
    m_dispatch["source_ip"] = [](std::optional<ConnectionConfig>& current, const std::string& value)
    {
        current->SetSourceIp(value); 
    };

    // Ensure output list starts empty
        
    m_outputList.clear();

    if (m_file.is_open())
    {
        parseFile();
        m_file.close();
    }
    else
    {
        std::cout << "Failed to open file: " << filename
            << " (error flags=" << m_file.rdstate() << ")\n";
    }
}

void IniFileParser::parseFile()
{
    std::string line;
    std::optional<ConnectionConfig> connection; 

    while (std::getline(m_file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::string key, value;

        if (line == "[Connection]")
            key = line;
        else
            tokenizerString(line, key, value);

        auto handler = m_dispatch.find(key);
        if (handler == m_dispatch.end())
        {
            if (!key.empty())
                std::cout << "Unknown parameter in INI file: " << key << std::endl;
        }
        else
        {
            handler->second(connection, value);
        }
    }

    if (connection.has_value())
    {
        m_outputList.push_back(std::move(*connection));
    }
}

void IniFileParser::trimStr(std::string& s)
{
    // Remove inline comments
    auto pos = s.find('#');
    if (pos != std::string::npos)
        s = s.substr(0, pos);

    // Trim left side
    s.erase(s.begin(),
        std::find_if(s.begin(), s.end(), [](char c) { return !std::isspace(c); }));

    // Trim right side
    s.erase(
        std::find_if(s.rbegin(), s.rend(), [](char c) { return !std::isspace(c); }).base(),
        s.end()
    );
}
void IniFileParser::tokenizerString(const std::string& input, std::string& key, std::string& value)
{
    // Helper 
    auto pos = input.find('=');
    if (pos == std::string::npos)
    {
        // Invalid key/value format (line without '=')
        key.clear();
        value.clear();
        return;
    }

    key = input.substr(0, pos);
    value = input.substr(pos + 1);

    // Remove extra spaces and inline comments
    trimStr(key);
    trimStr(value);
}