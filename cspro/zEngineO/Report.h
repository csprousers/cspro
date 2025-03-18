#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>
#include <zAppO/ReportFile.h>


class ZENGINEO_API Report : public Symbol
{
public:
    Report(std::string report_name, ReportFile::EscapeType report_escape_type, std::string report_file_path);
    Report(const ReportFile& report_file);

    const std::string& GetFilePath() const { return m_filePath; }

    bool IsFunctionParameter() const { return m_filePath.empty(); }

    bool IsTypeHtml() const          { return ( m_type == Type::Html ); }
    bool IsTypeMarkdown() const      { return ( m_type == Type::Markdown ); }
    bool IsTypeHtmlOrDerived() const { return ( m_type != Type::None ); }

    ReportFile::EscapeType GetEscapeType() const { return m_escapeType; }

    void SetProgramIndex(int program_index) { m_programIndex = program_index; }
    int GetProgramIndex() const             { return m_programIndex; }

    // runtime only
    void SetReportTextBuilder(std::string* report_text_builder) { m_reportTextBuilder = report_text_builder; }
    std::string* GetReportTextBuilder()                         { return m_reportTextBuilder; }

    // Symbol overrides
    void serialize_subclass(Serializer& ar) override;

protected:
    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

private:
    enum class Type { None, Html, Markdown };

    Type m_type;
    ReportFile::EscapeType m_escapeType;
    std::string m_filePath;
    int m_programIndex;

    // runtime only
    std::string* m_reportTextBuilder;
};
