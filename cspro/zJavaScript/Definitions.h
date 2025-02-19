#pragma once

#include <zToolsO/CSProException.h>


namespace JavaScript
{
    class Exception;
    class Executor;
    class Value;

    using Bytecode = std::vector<uint8_t>;

    enum class ModuleType { Autodetect, Global, Module };
}


// --------------------------------------------------------------------------
// Exception
// --------------------------------------------------------------------------

class JavaScript::Exception : public CSProException
{
public:
    Exception(cs::string_sz message)
        :   CSProException(message.c_str())
    {
    }

    Exception(cs::string_sz full_message, std::string base_message, std::string file_path, int line_number)
        :   CSProException(full_message.c_str()),
            m_locationDetails({ std::move(base_message), std::move(file_path), line_number })
    {
    }

    bool HasLocationDetails() const           { return m_locationDetails.has_value(); }
    const std::string& GetBaseMessage() const { ASSERT(HasLocationDetails()); return std::get<0>(*m_locationDetails); }
    const std::string& GetFilePath() const    { ASSERT(HasLocationDetails()); return std::get<1>(*m_locationDetails); }
    int GetLineNumber() const                 { ASSERT(HasLocationDetails()); return std::get<2>(*m_locationDetails); }

private:
    std::optional<std::tuple<std::string, std::string, int>> m_locationDetails;
};
