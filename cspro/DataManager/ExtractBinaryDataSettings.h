#pragma once


struct ExtractBinaryDataSettings
{
    enum class FilenameFormat { Filename, KeyFilename, Signature, KeySignature };

    FilenameFormat filename_format = FilenameFormat::Filename;
    bool right_trim_keys = true;
    std::string invalid_character_replacement = "_";
    std::string output_directory;

    static ExtractBinaryDataSettings CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;
};
