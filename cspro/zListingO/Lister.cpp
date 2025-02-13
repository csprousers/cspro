#include "stdafx.h"
#include "Lister.h"
#include "CsvLister.h"
#include "DataFileLister.h"
#include "ExcelLister.h"
#include "HtmlLister.h"
#include "JsonLister.h"
#include "NullLister.h"
#include "TextLister.h"
#include <zUtilO/Viewers.h>
#include <zDataO/DataRepositoryHelpers.h>


namespace
{
    Listing::ListingType GetListingType(const std::wstring& listing_filename)
    {
        if( listing_filename.empty() )
            return Listing::ListingType::Null;

        auto matches = [extension = PortableFunctions::PathGetFileExtension(listing_filename)](const auto& test_extension)
        {
            return SO::EqualsNoCase(extension, test_extension);
        };

        return matches(FileExtensions::CSV)                   ? Listing::ListingType::Csv :
               matches(FileExtensions::Data::CSProDB)         ? Listing::ListingType::DataFile :
               matches(FileExtensions::Data::TextDataDefault) ? Listing::ListingType::DataFile :
               matches(FileExtensions::Excel)                 ? Listing::ListingType::Excel :
               matches(FileExtensions::HTML)                  ? Listing::ListingType::Html :
               matches(FileExtensions::HTM)                   ? Listing::ListingType::Html :
               matches(FileExtensions::Json)                  ? Listing::ListingType::Json :
                                                                Listing::ListingType::Text;
    }
}


std::unique_ptr<Listing::Lister> Listing::Lister::Create(std::shared_ptr<ProcessSummary> process_summary, const PFF& pff,
                                                         bool append, std::shared_ptr<const CaseAccess> case_access)
{
    std::wstring listing_filename;

    if( pff.GetApplication() == nullptr || pff.GetApplication()->GetCreateListingFile() )
    {
        listing_filename = pff.GetListingFName();

        // the listing filename typically isn't specified in the PFF for entry applications;
        // in these cases, we will create the listing filename based on the data file's name,
        // or the application filename is there is no data file
        if( SO::IsWhitespace(listing_filename) && pff.GetAppType() == APPTYPE::ENTRY_TYPE )
        {
            if( pff.GetSingleInputDataConnectionString().IsFilenamePresent() )
            {
                listing_filename = pff.GetSingleInputDataConnectionString().GetFilename() + FileExtensions::WithDot::Listing;
            }

            else
            {
                listing_filename = PortableFunctions::PathRemoveFileExtension(pff.GetAppFName()) + FileExtensions::WithDot::Listing;
            }
        }
    }

    ListingType listing_type = GetListingType(listing_filename);

    // if no filename is specified, use the null lister
    if( listing_type == ListingType::Null )
    {
        return std::make_unique<NullLister>(process_summary);
    }

    else
    {
        SetupEnvironmentToCreateFile(listing_filename);

        if( listing_type == ListingType::Text )
        {
            return std::make_unique<TextLister>(std::move(process_summary), std::move(listing_filename), append, pff);
        }

        else if( listing_type == ListingType::Csv )
        {
            return std::make_unique<CsvLister>(std::move(process_summary), std::move(listing_filename), append, std::move(case_access));
        }

        else if( listing_type == ListingType::DataFile )
        {
            return std::make_unique<DataFileLister>(std::move(process_summary), std::move(listing_filename), append, std::move(case_access));
        }

        else if( listing_type == ListingType::Excel )
        {
            return std::make_unique<ExcelLister>(std::move(process_summary), std::move(listing_filename), std::move(case_access));
        }

        else if( listing_type == ListingType::Html )
        {
            return std::make_unique<HtmlLister>(std::move(process_summary), std::move(listing_filename), append, pff);
        }

        else if( listing_type == ListingType::Json )
        {
            return std::make_unique<JsonLister>(std::move(process_summary), std::move(listing_filename), std::move(case_access));
        }
    }

    throw ProgrammingErrorException();
}


Listing::Lister::Lister(std::shared_ptr<ProcessSummary> process_summary)
    :   m_processSummary(std::move(process_summary)),
        m_currentCaseToBeProcessed(nullptr),
        m_currentCasePositionInRepository(std::numeric_limits<double>::lowest()),
        m_updateProcessSummaryWithMessageNumbers(false)
{
    ASSERT(m_processSummary != nullptr);
}


Listing::Lister::~Lister()
{
    ASSERT(m_messages.messages.empty());
}


void Listing::Lister::WriteAndResetCachedMessages()
{
    if( !m_messages.messages.empty() )
    {
        WriteMessages(m_messages);
        m_messages.messages.clear();
    }
}


void Listing::Lister::UpdateCaseSourceDetails(const ConnectionString& connection_string, const CDataDict& dictionary)
{
    WriteAndResetCachedMessages();

    ProcessCaseSourceDetails(connection_string, dictionary);
}


void Listing::Lister::SetMessageSource(std::wstring message_source)
{
    WriteAndResetCachedMessages();

    ProcessCaseSource(nullptr);

    m_messages.source = std::move(message_source);
    m_currentCaseToBeProcessed = nullptr;
    m_currentCasePositionInRepository = std::numeric_limits<double>::lowest();
    m_currentCaseLevelKey.clear();
}


void Listing::Lister::SetMessageSource(const Case& data_case, std::wstring level_key/* = std::wstring()*/)
{
    // in case the position in repository is not properly updated, also use the case key
    // as a check as to whether the case has changed
    bool case_changed = ( m_currentCasePositionInRepository != data_case.GetPositionInRepository() ||
                          m_messages.source != CS2WS(data_case.GetKey()) );

    if( case_changed || ( !IssueMultipleLevelMessagesTogether() && level_key != m_currentCaseLevelKey ) )
    {
        WriteAndResetCachedMessages();

        if( case_changed )
        {
            m_messages.source = data_case.GetKey();
            m_currentCaseToBeProcessed = &data_case;
            m_currentCasePositionInRepository = data_case.GetPositionInRepository();
        }
    }

    m_currentCaseLevelKey = std::move(level_key);
}


void Listing::Lister::Write(MessageType message_type, int message_number, std::wstring message_text)
{
    // if this message is from a new case, process it
    if( m_currentCaseToBeProcessed != nullptr )
    {
        ProcessCaseSource(m_currentCaseToBeProcessed);
        m_currentCaseToBeProcessed = nullptr;
    }

    // update the message counters
    m_processSummary->IncrementMessageCounter(message_type);

    if( m_updateProcessSummaryWithMessageNumbers && message_type != MessageType::User )
    {
        auto& counts_map = m_processSummary->GetSystemMessagesCountMap();
        auto number_previously_issued_lookup = counts_map.find(message_number);

        if( number_previously_issued_lookup != counts_map.cend() )
        {
            number_previously_issued_lookup->second = number_previously_issued_lookup->second + 1;
        }

        else
        {
            counts_map.try_emplace(message_number, 1);
        }
    }

    // cache the message
    m_messages.messages.emplace_back(Message { m_currentCaseLevelKey, MessageDetails{ message_type, message_number }, std::move(message_text) });
}


void Listing::Lister::WriteLineForWriteFile(std::wstring text)
{
    m_messages.messages.emplace_back(Message { m_currentCaseLevelKey, std::nullopt, std::move(text) });
}


void Listing::Lister::Finalize(const PFF& pff, const std::vector<std::vector<MessageSummary>>& message_summary_sets)
{
    WriteAndResetCachedMessages();

    for( const auto& message_summary_set : message_summary_sets )
    {
        if( !message_summary_set.empty() )
            WriteMessageSummaries(message_summary_set);
    };

    if( PortableFunctions::FileIsRegular(pff.GetApplicationErrorsFilename()) )
        WriteWarningAboutApplicationErrors(CS2WS(pff.GetApplicationErrorsFilename()));

    UpdateProcessSummary();

    WriteFooter();
}


std::optional<std::tuple<Listing::ListingType, void*>> Listing::Lister::GetFrequencyPrinterFeatures()
{
    std::optional<std::tuple<bool, ListingType, void*>> frequency_printer_details = GetFrequencyPrinter();

    if( !frequency_printer_details.has_value() )
        return std::nullopt;

    if( std::get<0>(*frequency_printer_details) )
        WriteAndResetCachedMessages();

    return std::make_tuple(std::get<1>(*frequency_printer_details), std::get<2>(*frequency_printer_details));
}


std::tuple<size_t, size_t, size_t, size_t> Listing::Lister::CountMessages() const
{
    size_t num_errors = 0;
    size_t num_warnings = 0;
    size_t num_user_messages = 0;
    size_t num_total_messages = 0;

    for( const Message& message : m_messages.messages )
    {
        if( message.details.has_value() )
        {
            ++num_total_messages;

            if( message.details->type == MessageType::Error )
            {
                ++num_errors;
            }

            else if( message.details->type == MessageType::Warning )
            {
                ++num_warnings;
            }

            else if( message.details->type == MessageType::User )
            {
                ++num_user_messages;
            }
        }
    }

    return { num_errors, num_warnings, num_user_messages, num_total_messages };
}


void Listing::Lister::View(NullTerminatedString listing_filename)
{
    if( !PortableFunctions::FileIsRegular(listing_filename) )
        return;

#ifdef WIN_DESKTOP
    ListingType listing_type = GetListingType(listing_filename);

    bool view_in_text_viewer = ( ( listing_type == ListingType::Text ) ||
                                 ( listing_type == ListingType::DataFile && !DataRepositoryHelpers::IsTypeSQLiteOrDerived(ConnectionString(listing_filename).GetType()) ) );

    if( view_in_text_viewer )
    {
        ViewFileInTextViewer(listing_filename);
    }

    else
#endif
    {
        Viewer().ViewFile(listing_filename);
    }
}
