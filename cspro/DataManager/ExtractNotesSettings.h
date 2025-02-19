#pragma once


struct ExtractNotesSettings
{
    enum class OutputType { CSPro, CSV, Excel };

    OutputType output_type = OutputType::CSPro;
    ConnectionString notes_connection_string;
    std::string notes_dictionary_file_path;

    static ExtractNotesSettings CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;
};
