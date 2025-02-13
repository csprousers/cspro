#include "stdafx.h"
#include "ListingHelpers.h"
#include <zToolsO/base64.h>
#include <zToolsO/Utf8Convert.h>
#include <zDataO/DataRepositoryHelpers.h>


std::unique_ptr<CStdioFileUnicode> Listing::OpenListingFile(const std::wstring& filename, bool append, const TCHAR* file_type/* = _T("listing")*/)
{
    auto file = std::make_unique<CStdioFileUnicode>();
    bool open_success;

    if( append && PortableFunctions::FileIsRegular(filename) )
    {
        // make sure that the file is UTF-8
        if( Encoding encoding; GetFileBOM(filename, encoding) && encoding == Encoding::Ansi )
        {
            if( !CStdioFileUnicode::ConvertAnsiToUTF8(filename) )
                throw CSProException("Could not convert the listing file from ANSI to UTF-8.");
        }

        open_success = file->Open(filename.c_str(), CFile::modeWrite | CFile::modeNoTruncate);

        if( open_success )
            file->SeekToEnd();
    }

    else
    {
        open_success = file->Open(filename.c_str(), CFile::modeWrite | CFile::modeCreate);
    }

    if( !open_success )
        throw CSProException(_T("Could not create the %s file %s."), file_type, filename.c_str());

    return file;
}


std::wstring Listing::GetSystemDate()
{
#ifdef WASM
    return _T("WASM_TODO -- need a wcsftime substitute");

#else
    constexpr size_t TimeBufferSize = 30;
    struct tm tp = GetLocalTime();
    std::wstring system_date(TimeBufferSize, '\0');

    size_t length = wcsftime(system_date.data(), system_date.size(), _T("%b %d, %G"), &tp);
    system_date.resize(length);

    return system_date;
#endif
}


std::wstring Listing::GetSystemTime()
{
    struct tm tp = GetLocalTime();
    return FormatTextCS2WS(_T("%02d:%02d:%02d"), tp.tm_hour, tp.tm_min, tp.tm_sec);
}


std::wstring Listing::Base64EncodeIfNecessary(const std::wstring& text)
{
    // only use base-64 encoding if the text uses characters that aren't letters or numbers
    for( TCHAR ch : text )
    {
        if( !isalnum(ch) )
        {
            std::string utf8_text = UTF8Convert::WideToUTF8(text);
            std::wstring base64_encoded_text = Base64::Encode<std::wstring>(utf8_text.data(), utf8_text.length());

            // the tilde indicates that this is base64-encoded
            return _T("~") + base64_encoded_text;
        }
    }

    return text;
}


std::wstring Listing::CreateTextViewerUri(const std::wstring& filename)
{
    return SO::Concatenate(_T("cspro:///program=TextViewer&file="), Base64EncodeIfNecessary(filename));
}


std::optional<std::wstring> Listing::CreateDataUri(const ConnectionString& connection_string, const CDataDict& dictionary)
{
    if( connection_string.IsFilenamePresent() )
    {
        bool using_embedded_dictionary = DataRepositoryHelpers::DoesTypeContainEmbeddedDictionary(connection_string.GetType());

        // CSPro DB files, or other repositories when the dictionary exists, can be opened in Data Viewer
        if( using_embedded_dictionary || PortableFunctions::FileIsRegular(dictionary.GetFullFileName()) )
        {
            std::wstring data_uri = SO::Concatenate(_T("cspro:///program=DataViewer&file="), Base64EncodeIfNecessary(connection_string.ToString()));

            if( !using_embedded_dictionary )
                SO::Append(data_uri, _T("&dcf="), Base64EncodeIfNecessary(CS2WS(dictionary.GetFullFileName())));

            return data_uri;
        }

        // other repositories can be opened in Text Viewer
        else
        {
            return CreateTextViewerUri(connection_string.GetFilename());
        }
    }

    return std::nullopt;
}


std::optional<std::wstring> Listing::CreateCaseUri(const std::optional<std::wstring>& input_data_uri,
                                                   const std::optional<std::tuple<std::wstring, std::wstring>>& case_key_uuid)
{
    // construct the full link to open the case in Data or Text Viewer 
    if( input_data_uri.has_value() && case_key_uuid.has_value() )
    {
        std::wstring case_uri = SO::Concatenate(input_data_uri->c_str(),
                                                _T("&key="), Base64EncodeIfNecessary(std::get<0>(*case_key_uuid)));

        // add the UUID if it exists
        if( !std::get<1>(*case_key_uuid).empty() )
            SO::Append(case_uri, _T("&uuid="), Base64EncodeIfNecessary(std::get<1>(*case_key_uuid)));

        return case_uri;
    }

    return std::nullopt;
}
