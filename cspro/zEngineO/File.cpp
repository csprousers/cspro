#include "stdafx.h"
#include "File.h"
#include <zToolsO/TextEncoding.h>
#include <zUtilO/StdioFileUnicode.h>


// --------------------------------------------------------------------------
// LogicFile
// --------------------------------------------------------------------------

LogicFile::LogicFile(std::string file_name)
    :   Symbol(std::move(file_name), SymbolType::File),
        m_isUsed(false),
        m_hasGlobalVisibility(false),
        m_isWrittenTo(false),
        m_encoding(Encoding::Invalid)
{
}


LogicFile::LogicFile(const LogicFile& logic_file)
    :   Symbol(logic_file),
        m_isUsed(logic_file.m_isUsed),
        m_hasGlobalVisibility(logic_file.m_hasGlobalVisibility),
        m_isWrittenTo(logic_file.m_isWrittenTo),
        m_encoding(Encoding::Invalid)
{
}


LogicFile::~LogicFile()
{
    Close();
}


void LogicFile::CopyCompileTimeAttributes(const Symbol& symbol)
{
    const LogicFile& logic_file = assert_cast<const LogicFile&>(symbol);

    m_isUsed |= logic_file.m_isUsed;
    m_isWrittenTo |= logic_file.m_isWrittenTo;
}


std::unique_ptr<Symbol> LogicFile::CloneInInitialState() const
{
    return std::unique_ptr<LogicFile>(new LogicFile(*this));
}


bool LogicFile::Open(const bool create_new, const bool append, const bool truncate)
{
    bool success = false;

    if( !IsOpen() && !m_filePath.empty() )
    {
        UINT open_flags = CFile::modeReadWrite | CFile::shareExclusive;

        if( create_new )
        {
            open_flags |= CFile::modeCreate;

            if( !truncate )
                open_flags |= CFile::modeNoTruncate;
        }

        Encoding file_encoding = Encoding::Invalid;

        const bool file_exists = GetFileBOM(m_filePath, file_encoding);

        // if only reading, ANSI is fine, but if writing, convert to UTF-8
        if( file_exists && file_encoding == Encoding::Ansi && IsWrittenTo() )
        {
            // if can't convert to UTF-8, return false
            if( !CStdioFileUnicode::ConvertAnsiToUTF8(m_filePath) )
                return false;

            file_encoding = Encoding::Utf8;
        }

        if( !file_exists )
        {
            m_encoding = Encoding::Utf8;
        }

        else if( file_encoding == Encoding::Utf8 || file_encoding == Encoding::Ansi )
        {
            m_encoding = file_encoding;
        }

        // the file encoding could have been UTF-16LE or something else but we're not going to support it; we'll read in those files as if they were ANSI
        else
        {
            m_encoding = Encoding::Ansi;
        }

        // open the file
        if( m_file.Open(UTF8_TODO::GetWide(m_filePath).c_str(), open_flags) )
        {
            success = true;

            if( m_encoding == Encoding::Utf8 )
            {
                if( IsWrittenTo() && m_file.GetLength() < TextEncoding::Utf8Bom_sv.length() ) // write out the UTF-8 BOM
                {
                    m_file.Write(TextEncoding::Utf8Bom_sv.data(), TextEncoding::Utf8Bom_sv.length());
                }

                else // skip past the BOM
                {
                    m_file.Seek(TextEncoding::Utf8Bom_sv.length(), CFile::begin);
                }
            }

            if( append )
                m_file.SeekToEnd();
        }
    }

    return success;
}


bool LogicFile::IsOpen() const
{
#ifdef WIN_DESKTOP
    return ( m_file.m_hFile != CFile::hFileNull );
#else
    return m_file.IsOpen();
#endif
}


void LogicFile::Reset()
{
    Close();
}


bool LogicFile::Close()
{
    bool success = false;

    if( IsOpen() )
    {
        try
        {
            m_file.Close();
            success = true;
        }
        catch(...) { }
    }

    return success;
}


void LogicFile::serialize_subclass(Serializer& ar)
{
    ar & m_isWrittenTo
       & m_isUsed
       & m_hasGlobalVisibility;
}


void LogicFile::WriteValueToJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    if( m_filePath.empty() )
    {
        json_writer.WriteNull(JK::path);
    }

    else
    {
        const std::string name = PortableFunctions::PathGetFilename(m_filePath);
        const std::string extension = PortableFunctions::PathGetFileExtension(name);

        json_writer.WritePath(JK::path, m_filePath)
                   .Write(JK::name, name)
                   .Write(JK::extension, extension)
                   .WriteIfHasValue(JK::contentType, MimeType::GetTypeFromFileExtension(extension));
    }

    json_writer.EndObject();
}
