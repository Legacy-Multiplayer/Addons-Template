#pragma once

#include "CAddonValues.h"

#include <functional>
#include <iomanip>
#include <map>
#include <sstream>

class CAddonLog
{
public:
    void Initialize(const AddonAPI* api, void* owner)
    {
        m_api = api;
        m_owner = owner;
    }
    template <typename... Args>
    void Info(const std::string& format, const Args&... args) const
    {
        Write(ADDON_LOG_INFO, format, args...);
    }
    template <typename... Args>
    void Warning(const std::string& format, const Args&... args) const
    {
        Write(ADDON_LOG_WARN, format, args...);
    }
    template <typename... Args>
    void Error(const std::string& format, const Args&... args) const
    {
        Write(ADDON_LOG_ERROR, format, args...);
    }

private:
    template <typename... Args>
    void Write(LogLevel level, const std::string& format, const Args&... args) const
    {
        if (!m_api)
        {
            return;
        }
        std::ostringstream output;
        size_t position = 0;
        const auto append = [&](const auto& value)
        {
            const auto marker = format.find("{}", position);
            if (marker == std::string::npos)
            {
                throw std::invalid_argument("Log format has too few placeholders");
            }
            output << format.substr(position, marker - position) << value;
            position = marker + 2;
        };
        (append(args), ...);
        output << format.substr(position);
        m_api->Log(m_owner, level, output.str().c_str());
    }
    const AddonAPI* m_api = nullptr;
    void* m_owner = nullptr;
};

// Resource-relative UTF-8 text operations; binary assets belong to the asset API.
class CAddonFiles
{
public:
    void Initialize(const FileSystemAPI* api)
    {
        m_api = api;
    }
    bool Exists(const std::string& path) const
    {
        return m_api && m_api->Exists(path.c_str());
    }
    std::string ReadText(const std::string& path) const
    {
        const auto handle = Open(path, "rb");
        struct Close
        {
            const FileSystemAPI* api;
            int handle;
            ~Close()
            {
                api->Close(handle);
            }
        } close{m_api, handle};
        const int size = m_api->Size(handle);
        if (size < 0 || size > 4 * 1024 * 1024)
        {
            throw std::length_error("Text file exceeds 4 MiB");
        }
        const char* text = m_api->Read(handle, static_cast<size_t>(size));
        if (!text || std::char_traits<char>::length(text) != static_cast<size_t>(size))
        {
            throw std::runtime_error("Cannot read text file: " + path);
        }
        return text;
    }
    void WriteText(const std::string& path, const std::string& text) const
    {
        if (text.size() > 4 * 1024 * 1024 || text.find('\0') != std::string::npos)
        {
            throw std::invalid_argument("Invalid text file data");
        }
        const auto handle = Open(path, "wb");
        struct Close
        {
            const FileSystemAPI* api;
            int handle;
            ~Close()
            {
                api->Close(handle);
            }
        } close{m_api, handle};
        if (m_api->Write(handle, text.c_str()) != static_cast<int>(text.size()) || !m_api->Flush(handle))
        {
            throw std::runtime_error("Cannot write text file: " + path);
        }
    }

private:
    int Open(const std::string& path, const char* mode) const
    {
        if (!m_api)
        {
            throw std::logic_error("File API unavailable");
        }
        const auto handle = m_api->Open(path.c_str(), mode);
        if (handle < 0)
        {
            throw std::runtime_error("Cannot open resource file: " + path);
        }
        return handle;
    }
    const FileSystemAPI* m_api = nullptr;
};

// Small INI-style configuration: key = "value". Load is transactional.
class CAddonConfig
{
public:
    void Initialize(CAddonFiles* files)
    {
        m_files = files;
        m_values.clear();
    }
    bool Load(const std::string& path)
    {
        if (!m_files || !m_files->Exists(path))
        {
            return false;
        }
        std::map<std::string, std::string> values;
        std::istringstream input(m_files->ReadText(path));
        std::string line;
        while (std::getline(input, line))
        {
            std::istringstream entry(line);
            entry >> std::ws;
            if (entry.eof() || entry.peek() == '#' || entry.peek() == ';')
            {
                continue;
            }
            std::string key;
            if (!std::getline(entry, key, '='))
            {
                throw std::invalid_argument("Invalid configuration line");
            }
            const auto last = key.find_last_not_of(" \t\r");
            key = last == std::string::npos ? "" : key.substr(0, last + 1);
            ValidateKey(key);
            std::string value;
            if (!(entry >> std::quoted(value)) || !(entry >> std::ws).eof() || !values.emplace(key, value).second)
            {
                throw std::invalid_argument("Invalid or duplicate configuration key: " + key);
            }
        }
        m_values.swap(values);
        return true;
    }
    void Save(const std::string& path) const
    {
        if (!m_files)
        {
            throw std::logic_error("Configuration unavailable");
        }
        std::ostringstream output;
        for (const auto& entry : m_values)
        {
            output << entry.first << " = " << std::quoted(entry.second) << '\n';
        }
        m_files->WriteText(path, output.str());
    }
    template <typename T>
    T Get(const std::string& key, const T& fallback) const
    {
        const auto found = m_values.find(key);
        if (found == m_values.end())
        {
            return fallback;
        }
        if constexpr (std::is_same_v<T, std::string>)
        {
            return found->second;
        }
        else
        {
            if constexpr (std::is_unsigned_v<T>)
            {
                const auto first = found->second.find_first_not_of(" \t");
                if (first != std::string::npos && found->second[first] == '-')
                {
                    throw std::out_of_range("Negative unsigned configuration value");
                }
            }
            T value{};
            std::istringstream input(found->second);
            if constexpr (std::is_same_v<T, bool>)
            {
                input >> std::boolalpha;
            }
            if (!(input >> value) || !(input >> std::ws).eof())
            {
                throw std::invalid_argument("Invalid configuration value: " + key);
            }
            if constexpr (std::is_floating_point_v<T>)
            {
                if (!std::isfinite(value))
                {
                    throw std::invalid_argument("Non-finite configuration value");
                }
            }
            return value;
        }
    }
    template <typename T>
    void Set(const std::string& key, const T& value)
    {
        ValidateKey(key);
        std::ostringstream output;
        output << std::boolalpha << value;
        const auto text = output.str();
        if (text.find_first_of("\r\n") != std::string::npos || text.find('\0') != std::string::npos)
        {
            throw std::invalid_argument("Configuration values must fit one line");
        }
        m_values[key] = text;
    }

private:
    static void ValidateKey(const std::string& key)
    {
        if (key.empty() || key.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") != std::string::npos)
        {
            throw std::invalid_argument("Invalid configuration key");
        }
    }
    CAddonFiles* m_files = nullptr;
    std::map<std::string, std::string> m_values;
};

class CAddonEntity
{
public:
    CAddonEntity(const AddonAPI* api, AddonEntityKind kind, int id)
        : m_api(api), m_kind(kind), m_id(id), m_generation(api ? api->GetEntityGeneration(kind, id) : 0)
    {
    }
    bool IsValid() const
    {
        return m_api && m_generation != 0 && m_api->GetEntityGeneration(m_kind, m_id) == m_generation;
    }
    int Id() const
    {
        if (!IsValid())
        {
            throw std::runtime_error("Entity no longer exists");
        }
        return m_id;
    }

private:
    const AddonAPI* m_api;
    AddonEntityKind m_kind;
    int m_id;
    uint32_t m_generation;
};

class CAddonEntities
{
public:
    explicit CAddonEntities(AddonEntityKind kind) : m_kind(kind)
    {
    }
    void Initialize(const AddonAPI* api)
    {
        m_api = api;
    }
    CAddonEntity Find(int id) const
    {
        return CAddonEntity(m_api, m_kind, id);
    }
    template <typename Function>
    void ForEach(Function function) const
    {
        if (!m_api)
        {
            return;
        }
        std::vector<int> ids(m_api->CopyEntityIds(m_kind, nullptr, 0));
        const auto count = m_api->CopyEntityIds(m_kind, ids.data(), ids.size());
        if (count > ids.size())
        {
            throw std::runtime_error("Entity enumeration must run on the server thread");
        }
        std::vector<CAddonEntity> snapshot;
        for (size_t index = 0; index < count; ++index)
        {
            snapshot.push_back(Find(ids[index]));
        }
        for (const auto& entity : snapshot)
        {
            if (entity.IsValid())
            {
                function(entity);
            }
        }
    }

private:
    AddonEntityKind m_kind;
    const AddonAPI* m_api = nullptr;
};
