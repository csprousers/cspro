#pragma once

#include <zToolsO/CSProException.h>


namespace JavaScript
{
    // --------------------------------------------------------------------------
    // Exception
    // --------------------------------------------------------------------------
    class Exception : public CSProException
    {
    public:
        Exception(const char* message)
            :   CSProException(message)
        {
        }

        Exception(const char* full_message, std::string base_message, std::wstring filename, int line_number)
            :   CSProException(full_message),
                m_locationDetails({ std::move(base_message), std::move(filename), line_number })
        {
        }

        bool HasLocationDetails() const           { return m_locationDetails.has_value(); }
        const std::string& GetBaseMessage() const { ASSERT(HasLocationDetails()); return std::get<0>(*m_locationDetails); }
        const std::wstring& GetFilename() const   { ASSERT(HasLocationDetails()); return std::get<1>(*m_locationDetails); }
        int GetLineNumber() const                 { ASSERT(HasLocationDetails()); return std::get<2>(*m_locationDetails); }

    private:
        std::optional<std::tuple<std::string, std::wstring, int>> m_locationDetails;
    };



    // --------------------------------------------------------------------------
    // ByteCode
    // --------------------------------------------------------------------------
    using ByteCode = std::vector<uint8_t>;



    // --------------------------------------------------------------------------
    // ModuleType
    // --------------------------------------------------------------------------
    enum class ModuleType { Autodetect, Global, Module };
}
