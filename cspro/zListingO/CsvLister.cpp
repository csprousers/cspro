#include "stdafx.h"
#include "CsvLister.h"
#include <zToolsO/NumberConverter.h>


Listing::CsvLister::CsvLister(std::shared_ptr<ProcessSummary> process_summary, const std::string& file_path, const bool append, std::shared_ptr<const CaseAccess> case_access)
    :   Lister(std::move(process_summary)),
        m_csvTextCreator(DelimitedTextCreator::Type::CSV, DelimitedTextCreator::NewlineType::WriteAsCRLF),
        m_csvTextCreatorForKeys(DelimitedTextCreator::Type::CSV, DelimitedTextCreator::NewlineType::WriteAsCRLF),
        m_caseAccess(std::move(case_access))
{
    if( m_caseAccess == nullptr )
        throw CSProException("The CSV lister can only be used when working with applications that use an input dictionary.");

    const bool write_header = ( !append || !PortableFunctions::FileIsRegular(file_path) );

    const std::vector<std::vector<const CDictItem*>> id_items_by_level = m_caseAccess->GetDataDict().GetIdItemsByLevel();
    m_useLevelKey = ( id_items_by_level.size() > 1 );

    m_textFile = OpenListingFile(file_path, append);

    // quit out if not writing a header
    if( !write_header )
        return;

    m_csvTextCreator.AddTextNoNeedToDelimit("Source");

    // add a column for the level key (if applicable)
    if( m_useLevelKey )
        m_csvTextCreator.AddTextNoNeedToDelimit("Level");

    // add columns for each ID on level 1
    for( const CDictItem* dict_item : id_items_by_level.front() )
        m_csvTextCreator.AddTextNoNeedToDelimit(dict_item->GetName());

    m_csvTextCreator.AddTextNoNeedToDelimit("Type");
    m_csvTextCreator.AddTextNoNeedToDelimit("Number");
    m_csvTextCreator.AddTextNoNeedToDelimit("Text");

    m_textFile->WriteLine(m_csvTextCreator.GetSV());
}


Listing::CsvLister::~CsvLister()
{
}


void Listing::CsvLister::WriteMessages(const Messages& messages)
{
    std::optional<size_t> source_length;

    for( const Message& message : messages.messages )
    {
        if( !source_length.has_value() )
        {
            m_csvTextCreator.ResetText();

            m_csvTextCreator.AddText(messages.source);

            source_length = m_csvTextCreator.GetTextLength();
        }

        else
        {
            m_csvTextCreator.ResetBufferPosition(*source_length);
        }

        // add the keys
        if( m_useLevelKey )
            m_csvTextCreator.AddText(message.level_key);

        m_csvTextCreator.AddAlreadyDelimitedText(m_csvTextCreatorForKeys.GetSV());

        // add the type and message number
        m_csvTextCreator.AddTextNoNeedToDelimit(GetMessageTypeText(message.details));

        if( message.details.has_value() )
        {
            m_csvTextCreator.AddTextNoNeedToDelimit(IntToString(message.details->number));
        }

        else
        {
            m_csvTextCreator.AddTextNoNeedToDelimit(std::string_view());
        }

        // add the message text
        m_csvTextCreator.AddText(*message.text);

        // write the text
        m_textFile->WriteLine(m_csvTextCreator.GetSV());
    }
}


void Listing::CsvLister::ProcessCaseSource(const Case* const data_case)
{
    m_csvTextCreatorForKeys.ResetText();

    // look at the 20220810 note in Exopfile.cpp to see that this can be done in the
    // constructor if the lister is created after the CaseAccess object is initialized
    if( m_idCaseItems.empty() )
    {
        if( !m_caseAccess->IsInitialized() )
        {
            ASSERT(data_case == nullptr);
            const size_t number_level1_ids = m_caseAccess->GetDataDict().GetIdItemsByLevel().front().size();
            m_csvTextCreatorForKeys.AddAlreadyDelimitedText(SO::GetRepeatingCharacterString(',', number_level1_ids - 1));
            return;
        }

        const std::vector<CaseLevelMetadata>& case_levels = m_caseAccess->GetCaseMetadata().GetCaseLevelsMetadata();
        m_idCaseItems = case_levels.front().GetIdCaseRecordMetadata().GetCaseItems();

        for( const CaseItem* const case_item : m_idCaseItems )
        {
            if( IsNumeric(case_item->GetDataType()) )
            {
                const size_t buffer_size_for_conversion = case_item->GetDictItem().GetCompleteLen();

                if( buffer_size_for_conversion > m_numericCaseItemConversionBuffer.size() )
                    m_numericCaseItemConversionBuffer.resize(buffer_size_for_conversion);
            }
        }
    }

    if( data_case == nullptr )
        return;

    CaseItemIndex index = data_case->GetRootCaseLevel().GetIdCaseRecord().GetCaseItemIndex();

    for( const CaseItem* const case_item : m_idCaseItems )
    {
        if( IsNumeric(case_item->GetDataType()) )
        {
            const double value = assert_cast<const NumericCaseItem&>(*case_item).GetValueForOutput(index);

            const CDictItem& dict_item = case_item->GetDictItem();
            ASSERT(m_numericCaseItemConversionBuffer.size() >= dict_item.GetCompleteLen());

            NumberConverter::DoubleToText(m_numericCaseItemConversionBuffer.data(), value, dict_item.GetCompleteLen(), dict_item.GetDecimal(), false, true);

            // trim spaces, which should only be on the left
            const std::string_view converted_value_sv(m_numericCaseItemConversionBuffer.data(), dict_item.GetCompleteLen());
            ASSERT(converted_value_sv == SO::TrimRight(converted_value_sv));

            m_csvTextCreatorForKeys.AddTextNoNeedToDelimit(SO::TrimLeft(converted_value_sv));
        }

        else
        {
            ASSERT(IsString(case_item->GetDataType()));

            const std::string& value = assert_cast<const StringCaseItem&>(*case_item).GetValue(index);

            // right-trim strings
            m_csvTextCreatorForKeys.AddText(SO::TrimRight(value));
        }
    }
}
