#include "stdafx.h"
#include "ErrorLister.h"


Listing::ErrorLister::ErrorLister(const PFF& pff)
    :   m_applicationErrorsFilePath(UTF8_TODO::GetUtf8(pff.GetApplicationErrorsFilename())),
        m_applicationFilePath(UTF8_TODO::GetUtf8(pff.GetAppFName())),
        m_applicationType(UTF8_TODO::GetUtf8(pff.GetAppTypeString())),
        m_hasErrors(false)
{
    SetupEnvironmentToCreateFile(m_applicationErrorsFilePath);

    // delete the file if it exists (because the presence of the file means that there were errors)
    PortableFunctions::FileDelete(m_applicationErrorsFilePath);
}


Listing::ErrorLister::~ErrorLister()
{
}


void Listing::ErrorLister::EnsureFileExists()
{
    // if the file is not yet open, create it
    if( m_textFile == nullptr )
    {
        m_textFile = OpenListingFile(m_applicationErrorsFilePath, false);

        // write the header
        m_textFile->WriteFormattedLine("%-15s %s", "Application", m_applicationFilePath.c_str());
        m_textFile->WriteFormattedLine("%-15s %s", "Type", m_applicationType.c_str());

        m_textFile->WriteFormattedLine("%-15s %s", "Date", DateTime::LocalDateString().c_str());
        m_textFile->WriteFormattedLine("%-15s %s", "Time", DateTime::LocalTimeString().c_str());

        m_textFile->WriteLine();
        m_textFile->WriteLine("CSPro Error Summary");
    }
}


void Listing::ErrorLister::Write(const Logic::ParserMessage& parser_message)
{
    EnsureFileExists();

    const char* type_text;

    if( parser_message.type == Logic::ParserMessage::Type::Error )
    {
        type_text = "ERROR";
        m_hasErrors = true;
    }

    else if( parser_message.type == Logic::ParserMessage::Type::Warning )
    {
        type_text = "WARNING";
    }

    else
    {
        ASSERT(false);
        return;
    }

    std::string error_source = !parser_message.compilation_unit_name.empty() ? PortableFunctions::PathGetFilename(parser_message.compilation_unit_name) :
                                                                               parser_message.proc_name;

    // space out errors when the source changes
    if( error_source != m_lastErrorSource )
    {
        m_textFile->WriteLine();
        m_lastErrorSource = std::move(error_source);
    }

    m_textFile->WriteFormattedLine("%s(%s, %d): %s", type_text, m_lastErrorSource.c_str(),
                                   static_cast<int>(parser_message.line_number), parser_message.message_text.c_str());
}


void Listing::ErrorLister::Write(const std::string_view message_text_sv)
{
    EnsureFileExists();

    m_textFile->WriteLine();
    m_textFile->WriteLine(message_text_sv);
}
