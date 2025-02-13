#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>


class ZENGINEO_API WorkString : public Symbol
{
protected:
    WorkString(const WorkString& work_string);

public:
    WorkString(std::wstring string_name);

    const std::wstring& GetString() const      { return m_string; }
    virtual void SetString(std::wstring value) { m_string = std::move(value); }

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void WriteValueToJson(JsonWriter& json_writer) const override;
    void UpdateValueFromJson(const JsonNode<wchar_t>& json_node) override;

protected:
    std::wstring m_string;
};


class ZENGINEO_API WorkAlpha : public WorkString
{
private:
    WorkAlpha(const WorkAlpha& work_alpha);

public:
    WorkAlpha(std::wstring alpha_name);

    unsigned GetLength() const { return m_length; }
    void SetLength(unsigned length);

    // WorkString overrides
    void SetString(std::wstring value) override;

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void serialize_subclass(Serializer& ar) override;

protected:
    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

private:
    unsigned m_length;
    unsigned m_numberRightSpaces;
};
