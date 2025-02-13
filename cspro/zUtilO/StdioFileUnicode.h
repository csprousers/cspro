#pragma once

#include <zUtilO/zUtilO.h>
#include <zToolsO/Tools.h>


// GHM 20111215 added to facilitate file entry in ANSI, UTF-8, or UTF-16LE

class CLASS_DECL_ZUTILO CStdioFileUnicode : public CStdioFile
{
public:
    CStdioFileUnicode()
        :   m_encoding(Encoding::Utf8),
            m_lastPos(0)
    {
        // default output of CSPro files will be UTF-8
    }

    Encoding GetEncoding() const        { return m_encoding; }
    void SetEncoding(Encoding encoding) { m_encoding = encoding; }

    ULONGLONG FlushAndGetPosition();

    virtual ULONGLONG GetPosition() const;

#ifdef WIN_DESKTOP
    virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError = NULL);
    static FILE* CreateFileWithoutBOM(LPCTSTR lpszFileName,Encoding forcedEncoding);
#endif

    static FILE* _tfopen(LPCTSTR lpszFileName,LPCTSTR lpszOpenFlags,Encoding forcedEncoding = Encoding::Invalid,Encoding * pReturnEncoding = NULL);

    static bool ReadTextFile(NullTerminatedString filename, CString& buffer);
    static bool ConvertAnsiToUTF8(NullTerminatedString filename);
    static bool ConvertUTF8ToAnsi(NullTerminatedString filename);

    void WriteString(const TCHAR* text)
    {
        CStdioFile::WriteString(text);
    }

    void WriteString(const std::wstring& text)
    {
        CStdioFile::WriteString(text.c_str());
    }

    void WriteLine(const TCHAR* text = nullptr)
    {
        if( text != nullptr )
            CStdioFile::WriteString(text);

        CStdioFile::WriteString(_T("\n"));
    }

    void WriteLine(const std::wstring& text)
    {
        WriteLine(text.c_str());
    }

    template<typename... Args>
    void WriteFormattedString(const TCHAR* formatter, Args const&... args)
    {
        CStdioFile::WriteString(FormatText(formatter, args...));
    }

    template<typename... Args>
    void WriteFormattedLine(const TCHAR* formatter, Args const&... args)
    {
        WriteLine(FormatText(formatter, args...));
    }

private:
    Encoding m_encoding;
    int64_t m_lastPos;
};
