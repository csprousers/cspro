#pragma once


struct CaseTextContentCreatorSettings
{
    bool colorize_items = true;
    bool show_details_pane = true;

    static CaseTextContentCreatorSettings CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;
};
