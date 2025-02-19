#include "stdafx.h"
#include "JavaScriptProperties.h"


namespace
{
    constexpr std::string_view DefaultActionInvokerName_sv = "CS";
}


bool JavaScriptProperties::operator==(const JavaScriptProperties& rhs) const
{
    return ( m_abortOnModuleLoadError == rhs.m_abortOnModuleLoadError &&
             m_useActionInvoker == rhs.m_useActionInvoker &&
             m_actionInvokerObjectNameOverride == rhs.m_actionInvokerObjectNameOverride &&
             m_bytecodeSerialization == rhs.m_bytecodeSerialization );
}


const char* JavaScriptProperties::GetActionInvokerObjectNameOverride() const
{
    if( !m_actionInvokerObjectNameOverride.empty() )
        return m_actionInvokerObjectNameOverride.c_str();

    return nullptr;
}


std::string JavaScriptProperties::GetEvaluatedActionInvokerObjectName() const
{
    if( !m_actionInvokerObjectNameOverride.empty() )
        return m_actionInvokerObjectNameOverride;

    return std::string(DefaultActionInvokerName_sv);
}


void JavaScriptProperties::SetActionInvokerObjectNameOverride(std::string name)
{
    if( name == DefaultActionInvokerName_sv )
        name.clear();

    m_actionInvokerObjectNameOverride = std::move(name);
}



// --------------------------------------------------------------------------
// serialization
// --------------------------------------------------------------------------

CREATE_JSON_KEY(abortOnModuleLoadError)
CREATE_JSON_KEY(actionInvoker)
CREATE_JSON_KEY(bytecodeSerialization)

CREATE_ENUM_JSON_SERIALIZER(JavaScriptProperties::BytecodeSerialization,
    { JavaScriptProperties::BytecodeSerialization::ScriptAndBytecode, "scriptAndBytecode" },
    { JavaScriptProperties::BytecodeSerialization::ScriptOnly, "scriptOnly" },
    { JavaScriptProperties::BytecodeSerialization::BytecodeOnly, "bytecodeOnly" })


JavaScriptProperties JavaScriptProperties::CreateFromJson(const JsonNode& json_node)
{
    JavaScriptProperties javascript_properties;

    javascript_properties.m_abortOnModuleLoadError = json_node.GetOrDefault(JK::abortOnModuleLoadError, javascript_properties.m_abortOnModuleLoadError);

    const JsonNode& action_invoker_json_node = json_node.GetOrEmpty(JK::actionInvoker);

    if( !action_invoker_json_node.IsEmpty() )
    {
        javascript_properties.m_useActionInvoker = action_invoker_json_node.GetOrDefault(JK::enabled, javascript_properties.m_useActionInvoker);
        javascript_properties.m_actionInvokerObjectNameOverride = action_invoker_json_node.GetOrConstruct<std::string>(JK::name);
    }

    javascript_properties.m_bytecodeSerialization = json_node.GetOrDefault(JK::bytecodeSerialization, javascript_properties.m_bytecodeSerialization);

    return javascript_properties;
}


void JavaScriptProperties::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::abortOnModuleLoadError, m_abortOnModuleLoadError)
               .BeginObject(JK::actionInvoker)
                   .Write(JK::enabled, m_useActionInvoker)
                   .WriteIfNotBlank(JK::name, m_actionInvokerObjectNameOverride)
               .EndObject()
               .Write(JK::bytecodeSerialization, m_bytecodeSerialization)
               .EndObject();
}


void JavaScriptProperties::serialize(Serializer& ar)
{
    ar & m_abortOnModuleLoadError
       & m_useActionInvoker
       & m_actionInvokerObjectNameOverride;
    ar.SerializeEnum(m_bytecodeSerialization);
}
