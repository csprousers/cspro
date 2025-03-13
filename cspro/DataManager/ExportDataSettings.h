#pragma once


struct ExportDataSettings
{
    std::set<DataRepositoryType> export_formats;
    std::string base_file_path;
    bool one_file_per_record;

    static ExportDataSettings CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;
};
