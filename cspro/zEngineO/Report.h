#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>


class ZENGINEO_API Report : public Symbol
{
public:
    Report(std::wstring report_name, std::wstring report_filename);

    const std::wstring& GetFilename() const { return m_filename; }

    bool IsFunctionParameter() const { return m_filename.empty(); }

    bool IsHtmlType() const { return m_isHtmlType; }

    void SetProgramIndex(int program_index) { m_programIndex = program_index; }
    int GetProgramIndex() const             { return m_programIndex; }

    // runtime only
    void SetReportTextBuilder(std::wstring* report_text_builder) { m_reportTextBuilder = report_text_builder; }
    std::wstring* GetReportTextBuilder()                         { return m_reportTextBuilder; }

    // Symbol overrides
    void serialize_subclass(Serializer& ar) override;

protected:
    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

private:
    std::wstring m_filename;
    bool m_isHtmlType;
    int m_programIndex;

    // runtime only
    std::wstring* m_reportTextBuilder;
};
