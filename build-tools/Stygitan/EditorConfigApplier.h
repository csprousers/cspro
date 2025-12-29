#pragma once

#include <Stygitan/EditorConfig.h>
#include <zToolsO/TextEncoding.h>


class EditorConfig::Applier
{
public:
    const BinaryBlock* Process(const std::string& file_path, const EditorConfig::Options& options);

private:
    static std::unique_ptr<BinaryBlock> Process(const BinaryBlock& file_data, const EditorConfig::Options& options);

    static void ProcessEndOfLine(std::string& text, EndOfLine end_of_line);

    static void ProcessTrimTrailingWhitespacePerLine(std::string& text);

    static void ProcessInsertFinalNewline(std::string& text, std::optional<EndOfLine> end_of_line);

    static std::tuple<std::string_view, std::unique_ptr<TextEncoding::Converter>> // BOM + text converter
        ProcessCharset(const TextEncoding& text_encoding, const std::optional<Charset>& charset);

private:
    struct Data
    {
        std::tuple<int64_t, int64_t> file_size_and_modified_time;
        std::unique_ptr<BinaryBlock> processed_file; // only non-null if there are changes
    };

    std::map<std::string, Data> m_data;
};
