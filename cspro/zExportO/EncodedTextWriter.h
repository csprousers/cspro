#pragma once


class EncodedTextWriter
{
public:
    EncodedTextWriter(DataRepositoryType type, const CaseAccess& case_access, const std::wstring& filename, const ConnectionString& connection_string);
    ~EncodedTextWriter();

    void WriteLine(wstring_view line_sv = wstring_view());

    template<typename... Args>
    void WriteFormattedLine(const TCHAR* formatter, Args const&... args)
    {
        WriteLine(FormatText(formatter, args...));
    }

private:
    FILE* m_file;
    bool m_ansi;
    std::vector<char> m_outputBuffer;
};
