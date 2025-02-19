#include "stdafx.h"
#include "Exception.h"


ActionInvoker::Exception::Exception(const cs::string_sz message, std::string cause, std::optional<std::string> name)
    :   CSProException(message.c_str()),
        m_name(name.has_value() ? std::move(*name) : "ActionInvokerError"),
        m_cause(std::move(cause))
{
}


ActionInvoker::Exception::Exception(const JsonNode& json_node, const bool must_use_object_format)
    :   Exception(( !must_use_object_format && json_node.IsString() ) ? Exception(json_node.Get<std::string>(),
                                                                                  std::string(),
                                                                                  std::nullopt) :
                                                                        Exception(json_node.Get<std::string>(JK::message),
                                                                                  json_node.Contains(JK::cause) ? json_node.Get(JK::cause).GetNodeAsString() : std::string(),
                                                                                  json_node.GetOptional<std::string>(JK::name)))
{
}
