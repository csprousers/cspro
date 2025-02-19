#pragma once

#include <zToolsO/CSProException.h>

namespace ActionInvoker { class Exception; }


class ActionInvoker::Exception : public CSProException
{
public:
    // if not blank, cause must be specified as valid JSON
    Exception(cs::string_sz message, std::string cause, std::optional<std::string> name);
    Exception(const JsonNode& json_node, bool must_use_object_format);

    const std::string& GetName() const  { return m_name; }
    const std::string& GetCause() const { return m_cause; }

private:
    std::string m_name;
    std::string m_cause;
};
