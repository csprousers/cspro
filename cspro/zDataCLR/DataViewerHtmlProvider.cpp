#include "Stdafx.h"
#include "DataViewerHtmlProvider.h"
#include "Case.h"
#include "DataRepository.h"
#include "DataViewerController.h"
#include "DataViewerSettings.h"
#include <zToolsO/TimeAgoConverter.h>
#include <zToolsO/Tools.h>
#include <zToolsO/Utf8Convert.h>
#include <zHtml/HtmlWriter.h>
#include <zDataO/ISyncableDataRepository.h>
#include <zEdit2O/ScintillaColorizer.h>


CSPro::Data::DataViewerHtmlProvider::DataViewerHtmlProvider(DataViewerController^ controller, Case^ data_case)
    :   m_controller(controller),
        m_case(data_case)
{
    if( m_storageMap == nullptr )
        m_storageMap = gcnew System::Collections::Generic::Dictionary<System::String^, DataViewerHtmlProvider^>;

    static int id_counter = 0;
    m_id = System::Convert::ToString(++id_counter);

    m_storageMap->Add(m_id, this);
}


void CSPro::Data::DataViewerHtmlProvider::Initialize()
{
    // find an available port (code based on DropboxAuthorizer.cs)
    constexpr int MinPort = 50500;
    constexpr int MaxPort = 50530;

    for( int port = MinPort; port <= MaxPort; ++port )
    {
        try
        {
            System::Net::HttpListener^ http_listener = gcnew System::Net::HttpListener();

            m_baseUrl = gcnew System::String(FormatText(L"http://localhost:%d/", port));
            http_listener->Prefixes->Add(m_baseUrl);

            // ignore errors when Data Viewer is closed before a stream is fully written
            http_listener->IgnoreWriteExceptions = true;

            http_listener->Start();

            // start listening for requests and return
            http_listener->BeginGetContext(gcnew System::AsyncCallback(HttpListenerCallback), http_listener);

            return;
        }
        catch(...) { }
    }

    throw gcnew System::Exception(L"Could not initialize the Data Viewer HTML provider");
}


void CSPro::Data::DataViewerHtmlProvider::HttpListenerCallback(System::IAsyncResult^ result)
{
    System::Net::HttpListener^ http_listener = (System::Net::HttpListener^)result->AsyncState;

    System::Net::HttpListenerContext^ context = http_listener->EndGetContext(result);

    // listen for the next request
    http_listener->BeginGetContext(gcnew System::AsyncCallback(HttpListenerCallback), http_listener);

    // process the current request
    System::Net::HttpListenerRequest^ request = context->Request;
    System::Net::HttpListenerResponse^ response = context->Response;

    array<wchar_t>^ segment_split_characters = { '/' };
    array<System::String^>^ segments = request->RawUrl->Split(segment_split_characters, System::StringSplitOptions::RemoveEmptyEntries);

    bool request_processed = false;

    if( segments->Length >= 2 )
    {
        for( int i = 0; i < segments->Length; ++i )
            segments[i] = System::Uri::UnescapeDataString(segments[i]);

        DataViewerHtmlProvider^ html_provider;

        if( m_storageMap->TryGetValue(segments[0], html_provider) )
        {
            if( segments[1] == DataSummaryPath )
            {
                request_processed = html_provider->CreateDataSummaryHtml(response);
            }

            else if( segments[1] == LogicHelperPath )
            {
                request_processed = html_provider->CreateLogicHelperHtml(response);
            }

            else if( segments[1] == CaseViewPath )
            {
                request_processed = html_provider->CreateCaseViewHtml(response);
            }

            else if( segments[1] == RetrieveDataPath && segments->Length >= 4 )
            {
                request_processed = html_provider->RetrieveData(response, segments[2], segments[3]);
            }
        }
    }

    // if this was not a valid request, return no content
    if( !request_processed )
    {
        response->ContentLength64 = 0;
        response->StatusCode = static_cast<int>(System::Net::HttpStatusCode::NotFound);
        response->OutputStream->Close();
    }
}


System::Uri^ CSPro::Data::DataViewerHtmlProvider::CreateUri(System::String^ path)
{
    return gcnew System::Uri(m_baseUrl + m_id + L"/" + path);
}


void CSPro::Data::DataViewerHtmlProvider::SetResponseContent(System::Net::HttpListenerResponse^ response,
                                                             const void* data, size_t length, System::String^ mime_type)
{
    response->ContentLength64 = length;
    response->ContentType = mime_type;
    response->StatusCode = static_cast<int>(System::Net::HttpStatusCode::OK);

    System::IO::UnmanagedMemoryStream^ data_stream = gcnew System::IO::UnmanagedMemoryStream(const_cast<unsigned char*>(static_cast<const unsigned char*>(data)), length);

    // when scrolling through cases quickly in Data Viewer, the response output stream may no longer be
    // valid by the time all bytes are copied, so we catch the exception related to this
    // (System.Net.HttpListenerException: "The specified network name is no longer available")
    try
    {
        data_stream->CopyTo(response->OutputStream);
    }
    catch( System::Exception^ ) { }

    response->OutputStream->Close();
}


void CSPro::Data::DataViewerHtmlProvider::SetResponseContent(System::Net::HttpListenerResponse^ response, wstring_view html_sv)
{
    const std::string utf8_html = UTF8Convert::WideToUTF8(html_sv);
    SetResponseContent(response, utf8_html.data(), utf8_html.length(), nullptr);
}


bool CSPro::Data::DataViewerHtmlProvider::CreateDataSummaryHtml(System::Net::HttpListenerResponse^ response)
{
    ::DataRepository& repository = *m_controller->GetDataRepository()->GetNativePointer();

    HtmlStringWriter html_writer;
    html_writer.WriteDefaultHeader(repository.GetName(DataRepositoryNameType::Concise), Html::CSS::CaseView);

    html_writer << L"<body><div class=\"dv_summary_alignment\">";

    html_writer << L"<table class=\"dv_summary_table\">";

    // display the filename and type
    if( repository.GetConnectionString().IsFilenamePresent() )
        html_writer << L"<tr><td class=\"dv_summary_table_header\">File</td><td>" << repository.GetConnectionString().GetName(DataRepositoryNameType::Full) << L"</td></tr>";

    html_writer << L"<tr><td class=\"dv_summary_table_header\">Type</td><td>" << ::ToString(repository.GetRepositoryType()) << L"</td></tr>";

    constexpr const TCHAR* BlankRow = L"<tr><td colspan=\"2\"></td></tr>";
    html_writer << BlankRow;

    // display information on the number of cases
    const size_t cases = repository.GetNumberCases();
    const size_t deleted_cases = repository.GetNumberCases(CaseIterationCaseStatus::All) - cases;
    const size_t partial_cases = repository.GetNumberCases(CaseIterationCaseStatus::PartialsOnly);

    html_writer << L"<tr><td class=\"dv_summary_table_header\">Cases</td><td>" << IntToString(cases) << L"</td></tr>";

    if( deleted_cases != 0 )
        html_writer << L"<tr><td class=\"dv_summary_table_header\">Deleted Cases</td><td>" << IntToString(deleted_cases) << L"</td></tr>";

    if( partial_cases != 0 )
    {
        html_writer << L"<tr><td class=\"dv_summary_table_header\">Partial Cases</td><td>"
                    << FormatText(L"%d (%0.1f%%)", static_cast<int>(partial_cases), 100.0 * partial_cases / cases) << L"</td></tr>";
    }

    // display information on the last time the data was synced
    ISyncableDataRepository* syncable_repository = repository.GetSyncableDataRepository();

    if( syncable_repository != nullptr )
    {
        const std::vector<::SyncHistoryEntry>& history = syncable_repository->GetSyncHistory();

        if( !history.empty() )
        {
            html_writer << BlankRow;

            constexpr bool ShowOnlyLastSync = true;

            for( auto history_itr = history.crbegin(); history_itr != history.crend(); ++history_itr )
            {
                double sync_time = static_cast<double>(history_itr->getDateTime());

                CString time_ago_with_time;
                std::string formatted_time = FormatTimestamp(sync_time, "%c");
                time_ago_with_time.Format(L"%s (%s)", TimeAgoConverter::GetTimeAgo(sync_time).GetString(),
                                          UTF8Convert::UTF8ToWide(formatted_time).c_str());

                html_writer << L"<tr><td class=\"dv_summary_table_header\">Last Sync</td><td>" << time_ago_with_time.GetString()
                            << L"<br />" << history_itr->getDeviceName() << L"</td></tr>";

                if( ShowOnlyLastSync )
                    break;
            }
        }
    }

    html_writer << L"</table>";

    html_writer << L"</div></body>"
                << L"</html>";

    SetResponseContent(response, html_writer.str());

    return true;
}


bool CSPro::Data::DataViewerHtmlProvider::CreateLogicHelperHtml(System::Net::HttpListenerResponse^ response)
{
    std::wstringstream logic;

    ::DataRepository& repository = *m_controller->GetDataRepository()->GetNativePointer();
    DataViewerSettings^ settings = m_controller->GetSettings();

    const std::wstring dictionary_name = repository.GetCaseAccess()->GetDataDict().GetName();

    std::wstring connection_string = repository.GetConnectionString().ToString();
    SO::Replace(connection_string, '\\', '/');

    std::wstring case_status;

    if( settings->GetCaseIterationCaseStatus() != CaseIterationCaseStatus::NotDeletedOnly )
    {
        case_status = SO::Concatenate(L"CaseStatus.",
            ( settings->GetCaseIterationCaseStatus() == CaseIterationCaseStatus::All )          ? L"All" :
            ( settings->GetCaseIterationCaseStatus() == CaseIterationCaseStatus::PartialsOnly ) ? L"Partial" :
                                                                                                  L"Duplicate");
    }

    std::wstring dictionary_access_parameters =
        ( settings->GetCaseIterationMethod() == CaseIterationMethod::KeyOrder ) ? std::wstring() :
                                                                                  L"OrderType.Sequential";

    if( settings->GetCaseIterationOrder() != CaseIterationOrder::Ascending )
        SO::AppendWithSeparator(dictionary_access_parameters, L"Order.Descending", L", ");

    if( !case_status.empty() )
        SO::AppendWithSeparator(dictionary_access_parameters, case_status, L", ");

    if( !dictionary_access_parameters.empty() )
        dictionary_access_parameters = L"(" + dictionary_access_parameters + L")";


    if( m_case != nullptr )
    {
        ::Case& native_data_case = m_case->GetNativeReference();

        const std::wstring escaped_key_string = Encoders::ToLogicString(CS2WS(native_data_case.GetKey()));
        ASSERT(dictionary_name == Encoders::ToEscapedString(dictionary_name));

        logic << L"// ------- Case Operations -------";

        logic << L"\n\n// to load this case using the key:\n";
        logic << L"loadcase(" << dictionary_name << L", " << escaped_key_string << L");";


        if( !native_data_case.GetUuid().IsEmpty() )
        {
            ASSERT(native_data_case.GetUuid() == WS2CS(Encoders::ToEscapedString(CS2WS(native_data_case.GetUuid()))));

            logic << L"\n\n// to load this case using the UUID (necessary for loading duplicates or deleted cases):\n";
            logic << L"locate(" << dictionary_name << L", uuid, \"" << native_data_case.GetUuid().GetString() << L"\");\n";
            logic << L"retrieve(" << dictionary_name << L");";
        }


        logic << L"\n\n// to delete this case:\n";
        logic << L"delcase(" << dictionary_name << L", " << escaped_key_string << L");";


        logic << L"\n\n\n\n";
    }


    logic << L"// ------- File Operations -------";


    logic << L"\n\n// to open this file as an external dictionary:\n";
    logic << L"setfile(" << dictionary_name << L", \"" << connection_string << L"\");";


    logic << L"\n\n// to count the number of cases:\n";
    logic << L"numeric number_cases = countcases(" << dictionary_name;

    if( !case_status.empty() )
        logic << L"(" << case_status << L")";

    logic << L");";


    logic << L"\n\n// to get a list of all of the case keys:\n";
    logic << L"string list case_keys;\n";
    logic << L"keylist(" << dictionary_name << dictionary_access_parameters << L", case_keys);";


    logic << L"\n\n// to iterate through all of the cases:\n";
    logic << L"forcase " << dictionary_name << dictionary_access_parameters << L" do\n"
          << L"\t// ...\n"
             L"endfor;";

    logic << L"\n";


    // color the text
    constexpr int lexer_language = Lexers::GetLexer_LogicV8_0();
    ScintillaColorizer colorizer(lexer_language, logic.str());

    const std::wstring logic_html = colorizer.GetHtml(ScintillaColorizer::HtmlProcessorType::FullHtml);
    ASSERT(!logic_html.empty());

    SetResponseContent(response, logic_html);

    return true;
}


bool CSPro::Data::DataViewerHtmlProvider::CreateCaseViewHtml(System::Net::HttpListenerResponse^ response)
{
    ASSERT(m_case != nullptr);
    const ::Case& native_data_case = m_case->GetNativeReference();
    CaseToHtmlConverter& case_to_html_converter = m_controller->GetCaseToHtmlConverter();
    const std::map<double, std::vector<std::wstring>>& case_construction_errors_map = m_controller->GetCaseConstructionErrorsMap();

    const auto& case_construction_errors = case_construction_errors_map.find(native_data_case.GetPositionInRepository());
    ASSERT(case_construction_errors != case_construction_errors_map.cend());

    CString base_url_for_binary_retrieval = CString(m_baseUrl + m_id + L"/" + RetrieveDataPath + L"/");

    CaseToHtmlConverter::CaseSpecificSettings case_specific_settings
    {
        &case_construction_errors->second,
        base_url_for_binary_retrieval
    };

    std::wstring html = case_to_html_converter.ToHtml(native_data_case, &case_specific_settings);

    SetResponseContent(response, html);

    return true;
}


bool CSPro::Data::DataViewerHtmlProvider::RetrieveData(System::Net::HttpListenerResponse^ response,
                                                       System::String^ case_item_identifier, System::String^ mime_type)
{
    try
    {
        const std::vector<std::byte>& binary_data = m_controller->GetBinaryData(m_case, case_item_identifier);

        SetResponseContent(response, binary_data.data(), binary_data.size(), mime_type);

        return true;
    }

    catch(...)
    {
        return false;
    }
}
