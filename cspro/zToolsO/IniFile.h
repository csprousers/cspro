#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/TextFile.h>


// IniFileReader and IniFileWriter can be used in places where
// CSpecFile was previously used to read and write from INI-style files;
// the classes' methods throw exceptions on errors

class IniFileBase : protected FileIO::TextFile
{
protected:
    static constexpr std::string_view AttributeValueSeparatorText_sv = "=";
    static constexpr char AttributeValueSeparatorChar                = AttributeValueSeparatorText_sv.front();
    static constexpr char ContinuationLineIndicator                  = '&';

public:
    static constexpr const char* VersionKey = "Version";

protected:
    IniFileBase() { }

public:
    // calls TextFile::SetProperties
    template<typename T>
    void SetProperties(T&& property_retriever_or_object) { TextFile::SetProperties(std::forward<T>(property_retriever_or_object)); }

    // calls TextFile::Close
    void Close() { TextFile::Close(); }
};



// --------------------------------------------------------------------------
// IniFileReader
// --------------------------------------------------------------------------

class CLASS_DECL_ZTOOLSO IniFileReader : public IniFileBase
{
public:
    // calls TextFile::OpenForTextReading
    void Open(InterfaceString file_path, int share_flag = INT_MIN) { TextFile::OpenForTextReading(std::move(file_path), share_flag); }

    // reads a line, splitting it into an attribute and value using the attribute/value separator,
    // returning true if a line was read;
    // if trim is true, then both the attribute and value will be trimmed
    bool ReadLine(std::string& attribute, std::string& value, bool trim = true);

    // returns the line number of the last-returned read line;
    // the number will not be 100% accurate when unreading lines that were continued on multiple lines
    size_t GetLineNumber() const { return m_linesRead - m_unreadLines.size(); }

    // sets the attribute and value as the next line to be read
    void UnreadLine(std::string attribute, std::string value);

private:
    size_t m_linesRead = 0;
    std::vector<std::tuple<std::string, std::string>> m_unreadLines;
};



// --------------------------------------------------------------------------
// IniFileWriter
// --------------------------------------------------------------------------

class CLASS_DECL_ZTOOLSO IniFileWriter : public IniFileBase
{
public:
    // calls TextFile::OpenForTextWritingCreate
    void Open(InterfaceString file_path, int share_flag = INT_MIN) { TextFile::OpenForTextWritingCreate(std::move(file_path), share_flag); }

    // writes the version number
    IniFileWriter& WriteVersion();

    // writes a blank line
    IniFileWriter& WriteLine();

    // writes the line
    template<typename T>
    IniFileWriter& WriteLine(T&& text);

    // writes the attribute=value line
    template<typename AT, typename VT>
    IniFileWriter& WriteLine(AT&& attribute, const VT& value);

private:
    template<typename VT>
    static auto GetValueAsString(const VT& value);
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline IniFileWriter& IniFileWriter::WriteLine()
{
    TextFile::WriteLine();
    return *this;
}


template<typename T>
IniFileWriter& IniFileWriter::WriteLine(T&& text)
{
    TextFile::WriteLine(std::forward<T>(text));
    return *this;
}


template<typename AT, typename VT>
IniFileWriter& IniFileWriter::WriteLine(AT&& attribute, const VT& value)
{
    WriteLine(SO::Concatenate(std::forward<AT>(attribute),
                              AttributeValueSeparatorText_sv,
                              GetValueAsString(value)));
    return *this;
}


template<typename VT>
auto IniFileWriter::GetValueAsString(const VT& value)
{
    if constexpr(std::is_same_v<VT, double>)
    {
        return FormatText("%f", value);
    }

    else if constexpr(std::is_same_v<VT, int32_t> ||
                      std::is_same_v<VT, uint32_t> ||
                      std::is_same_v<VT, int64_t> ||
                      std::is_same_v<VT, uint64_t>)
    {
        return IntToString(value);
    }

    else
    {
        return std::string_view(value);
    }
}
