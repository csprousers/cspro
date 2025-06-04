#pragma once

#include <zEngineO/zEngineO.h>
#include <zLogicO/Symbol.h>
#include <zAppO/ReportFile.h>


class ZENGINEO_API Report : public Symbol
{
private:
    Report(std::string report_name, ReportFile::Encoding report_encoding, std::string report_file_path);

public:
    Report(const ReportFile& report_file);

    static std::unique_ptr<Report> CreateReportFunctionParamter(std::string report_name);

    const std::string& GetFilePath() const { return m_filePath; }

    bool IsFunctionParameter() const { return m_filePath.empty(); }

    ReportFile::Encoding GetEncoding() const { return m_encoding; }

    void SetProgramIndex(int program_index) { m_programIndex = program_index; }
    int GetProgramIndex() const             { return m_programIndex; }

    // runtime only
    void SetReportTextBuilder(std::string* report_text_builder) { m_reportTextBuilder = report_text_builder; }
    std::string* GetReportTextBuilder()                         { return m_reportTextBuilder; }

    // Symbol overrides
    void serialize_subclass(Serializer& ar) override;

    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;
    void WriteValueToJson(JsonWriter& json_writer) const override;
    void SetValueFromJson(const JsonNode& json_node) override;

private:
    ReportFile::Encoding m_encoding;
    std::string m_filePath;
    int m_programIndex;

    // runtime only
    std::string* m_reportTextBuilder;
};
