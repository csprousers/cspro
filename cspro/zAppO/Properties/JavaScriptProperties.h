#pragma once

#include <zAppO/zAppO.h>


class ZAPPO_API JavaScriptProperties
{
public:
    enum class BytecodeSerialization { ScriptAndBytecode, ScriptOnly, BytecodeOnly };
    static constexpr BytecodeSerialization DefaultBytecodeSerialization = BytecodeSerialization::ScriptAndBytecode;

    bool operator==(const JavaScriptProperties& rhs) const;
    bool operator!=(const JavaScriptProperties& rhs) const { return !( *this == rhs ); }

    bool GetAbortOnModuleLoadError() const     { return m_abortOnModuleLoadError; }
    void SetAbortOnModuleLoadError(bool abort) { m_abortOnModuleLoadError = abort; }

    bool GetUseActionInvoker() const   { return m_useActionInvoker; }
    void SetUseActionInvoker(bool use) { m_useActionInvoker = use; }

    const char* GetActionInvokerObjectNameOverride() const;
    std::string GetEvaluatedActionInvokerObjectName() const;
    void SetActionInvokerObjectNameOverride(std::string name);

    BytecodeSerialization GetBytecodeSerialization() const             { return m_bytecodeSerialization; }
    void SetBytecodeSerialization(BytecodeSerialization serialization) { m_bytecodeSerialization = serialization; }


    // serialization
    // --------------------------------------------------------------------------
    static JavaScriptProperties CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);


private:
    bool m_abortOnModuleLoadError = true;
    bool m_useActionInvoker = true;
    std::string m_actionInvokerObjectNameOverride;
    BytecodeSerialization m_bytecodeSerialization = DefaultBytecodeSerialization;
};
