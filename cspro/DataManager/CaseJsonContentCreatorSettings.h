#pragma once


struct CaseJsonContentCreatorSettings
{
    bool json_format_compact = false;
    bool verbose = false;
    bool write_blank_values = false;
    bool write_labels = false;
    bool binary_data_urls = false;

    static CaseJsonContentCreatorSettings CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;
};
