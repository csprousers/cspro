#include "stdafx.h"
#include "ErrorLister.h"


Listing::ErrorLister::ErrorLister(const PFF& pff)
    :   m_listingFilename(pff.GetApplicationErrorsFilename()),
        m_applicationFilename(CS2WS(pff.GetAppFName())),
        m_applicationType(CS2WS(pff.GetAppTypeString())),
        m_hasErrors(false)
{
    SetupEnvironmentToCreateFile(m_listingFilename);

    // delete the file if it exists (because the presence of the file means that there were errors)
    PortableFunctions::FileDelete(m_listingFilename);
}


Listing::ErrorLister::~ErrorLister()
{
    if( m_file != nullptr )
        m_file->Close();
}


void Listing::ErrorLister::EnsureFileExists()
{
    // if the file is not yet open, create it
    if( m_file == nullptr )
    {
        m_file = OpenListingFile(m_listingFilename, false);

        // write the header
        m_file->WriteFormattedLine(_T("%-15s %s"), _T("Application"), m_applicationFilename.c_str());
        m_file->WriteFormattedLine(_T("%-15s %s"), _T("Type"), m_applicationType.c_str());

        m_file->WriteFormattedLine(_T("%-15s %s"), _T("Date"), GetSystemDate().c_str());
        m_file->WriteFormattedLine(_T("%-15s %s"), _T("Time"), GetSystemTime().c_str());

        m_file->WriteLine(_T("\nCSPro Error Summary"));
    }
}


void Listing::ErrorLister::Write(const Logic::ParserMessage& parser_message)
{
    EnsureFileExists();

    const TCHAR* type_text = nullptr;

    if( parser_message.type == Logic::ParserMessage::Type::Error )
    {
        type_text = _T("ERROR");
        m_hasErrors = true;
    }

    else if( parser_message.type == Logic::ParserMessage::Type::Warning )
    {
        type_text = _T("WARNING");
    }

    ASSERT(type_text != nullptr);

    std::wstring error_source = !parser_message.compilation_unit_name.empty() ? PortableFunctions::PathGetFilename(parser_message.compilation_unit_name) :
                                                                                parser_message.proc_name;

    // space out errors when the source changes
    if( error_source != m_lastErrorSource )
    {
        m_file->WriteLine();
        m_lastErrorSource = std::move(error_source);
    }

    m_file->WriteFormattedLine(_T("%s(%s, %d): %s"), type_text, m_lastErrorSource.c_str(),
                               (int)parser_message.line_number, parser_message.message_text.c_str());
}


void Listing::ErrorLister::Write(NullTerminatedString message_text)
{
    EnsureFileExists();
    m_file->WriteLine();
    m_file->WriteLine(message_text);
}
