#pragma once


namespace EditorConfig
{
    class Evaluator;
    struct Options;
    struct OptionStrings;

    enum class Indent { Space, Tab };

    enum class EndOfLine { LF, CR, CRLF };

    enum class Charset { Latin1, Utf8, Utf8Bom, Utf16BE, Utf16LE };
}


struct EditorConfig::OptionStrings
{
    static const char* Indent[2];
    static const char* EndOfLine[3];
    static const char* Charset[5];
};


struct EditorConfig::Options
{
    // standard EditorConfig options
    std::optional<Indent> indent_style;
    std::optional<int> indent_size;
    std::optional<EndOfLine> end_of_line;
    std::optional<Charset> charset;
    std::optional<bool> trim_trailing_whitespace;
    std::optional<bool> insert_final_newline;

    // Stygitan options
    std::optional<bool> stygitan_trim_final_newlines;

    // methods
    bool operator<(const Options& rhs) const noexcept;

    bool IsDefined() const noexcept;

    std::string GetShortDescription() const noexcept;
    std::string GetLongDescription() const noexcept;
};


class EditorConfig::Evaluator
{
public:
    ~Evaluator() noexcept;

    // Returns the evaluated EditorConfig options for the file, throwing an exception on error.
    // If there are no .editorconfig files for this file, the file is evaluated using
    // CSPro's default .editorconfig file.
    Options Parse(cs::string_view_sz file_path_sv) { return Parse(file_path_sv, false); }

private:
    const std::string& GetTempDirectoryForDefaultProcessing();

    Options Parse(cs::string_view_sz file_path_sv, bool using_default_editorconfig);

private:
    std::string m_tempDirectoryForDefaultProcessing;
};
