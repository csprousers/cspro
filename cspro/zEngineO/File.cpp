#include "stdafx.h"
#include "File.h"
#include <zUtilO/StdioFileUnicode.h>


// --------------------------------------------------------------------------
// LogicFile
// --------------------------------------------------------------------------

LogicFile::LogicFile(std::string file_name)
    :   Symbol(std::move(file_name), SymbolType::File),
        m_isUsed(false),
        m_hasGlobalVisibility(false),
        m_isWrittenTo(false)
{
}


LogicFile::LogicFile(const LogicFile& logic_file)
    :   Symbol(logic_file),
        m_isUsed(logic_file.m_isUsed),
        m_hasGlobalVisibility(logic_file.m_hasGlobalVisibility),
        m_isWrittenTo(logic_file.m_isWrittenTo)
{
}


LogicFile::~LogicFile() noexcept
{
    Close_noexcept();
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


void LogicFile::Open(bool create, bool append, const bool create_if_not_exist)
{
    // | create | append |
    // -------------------
    // | true   | false  | create a new file
    // | false  | true   | open an existing file, at the end, or create one if create_if_not_exist is true
    // | false  | false  | open an existing file, or create one if create_if_not_exist is true
    ASSERT(( create != append ) || ( !create && !append ));

    if( m_textFile != nullptr )
        return;

    if( m_filePath.empty() )
        throw CSProException("You must specify the path of a file to open.");

    ASSERT(!m_lastOperationWasWriting.has_value());

    std::optional<TextEncoding> text_encoding;
    static_assert(TextEncoding::DefaultEncoding == TextEncoding::Type::Utf8Bom);

    // file exists
    if( PortableFunctions::FileIsRegular(m_filePath) )
    {
        text_encoding = TextEncoding::ReadFileBom(m_filePath, TextEncoding::Type::Ansi);

        // if only reading, ANSI is fine, but if writing, convert to UTF-8
        if( IsWrittenTo() && text_encoding->GetType() == TextEncoding::Type::Ansi )
        {
            if( !CStdioFileUnicode::ConvertAnsiToUTF8(m_filePath) )
                throw CSProException("Could not convert the file from ANSI to UTF-8: " + m_filePath);

            text_encoding.reset();
        }
    }

    // file does not exist
    else
    {
        if( create_if_not_exist )
        {
            create = true;
            append = false;
        }

        if( !create )
            throw FileIO::Exception::FileNotFound(m_filePath);

        // create the directory for the file when necessary
        FileIO::CreateDirectoriesForFile(m_filePath);
    }

    auto text_file = std::make_unique<FileIO::TextFile>();
    text_file->SetWriteNewlineAsCRLF(false);

    if( text_encoding.has_value() )
        text_file->SetTextEncoding(*text_encoding);

    text_file->OpenForTextReadingAndWriting(m_filePath, create, append
#ifdef WIN32
        , _SH_DENYRW // share_flag
#endif
    );

    m_textFile = std::move(text_file);
}


void LogicFile::Reset()
{
    Close_noexcept();
}


void LogicFile::Close()
{
    m_textFile.reset();
    m_lastOperationWasWriting.reset();
}


void LogicFile::Close_noexcept() noexcept
{
    try
    {
        Close();
    }
    catch(...) { ASSERT(false); }
}


void LogicFile::StartOperation(const bool writing)
{
    ASSERT(IsOpen());

    if( m_lastOperationWasWriting == writing )
        return;

    if( m_lastOperationWasWriting.has_value() )
        m_textFile->Seek(0, SEEK_CUR);

    m_lastOperationWasWriting = writing;
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
