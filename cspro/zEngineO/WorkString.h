#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>


// --------------------------------------------------------------------------
// WorkString and WorkAlpha
//
// Values are stored as sharable strings.
// --------------------------------------------------------------------------

class ZENGINEO_API WorkString : public Symbol
{
protected:
    WorkString(const WorkString& work_string);

public:
    WorkString(std::string string_name);

    const std::string& GetString() const            { return *m_string; }
    const SharableString& GetSharableString() const { return m_string; }

    virtual void SetString(SharableString&& sharable_string) { m_string = std::move(sharable_string); }

    // Symbol overrides
    void CompareDeclarationAttributes(const Symbol& symbol) const override;

    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void WriteValueToJson(JsonWriter& json_writer) const override;
    void SetValueFromJson(const JsonNode& json_node) override;

    JavaScript::Value GetJavaScriptValue(JavaScript::Executor& executor) const override;
    void SetValueFromJavaScript(JavaScript::Executor& executor, const JavaScript::Value& js_value) override;

protected:
    SharableString m_string;
};


class ZENGINEO_API WorkAlpha : public WorkString
{
private:
    WorkAlpha(const WorkAlpha& work_alpha);

public:
    WorkAlpha(std::string alpha_name);

    size_t GetWideLength() const { return m_stringInResetState->length(); }
    void SetWideLength(size_t wide_length);

    // WorkString overrides
    void SetString(SharableString&& sharable_string) override;

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void serialize_subclass(Serializer& ar) override;

    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

private:
    std::shared_ptr<const std::string> m_stringInResetState;
};
