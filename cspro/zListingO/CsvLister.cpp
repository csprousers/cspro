#include "stdafx.h"
#include "CsvLister.h"
#include <zToolsO/Encoders.h>
#include <zToolsO/NumberConverter.h>


namespace
{
    size_t GetMaximumDelimitedLength(const CDictItem& dict_item)
    {
        if( IsNumeric(dict_item) )
        {
            return dict_item.GetCompleteLen();
        }

        // assume every character is "
        else if( IsString(dict_item) )
        {
            return 2 + dict_item.GetLen() * 2;
        }

        else
        {
            throw ProgrammingErrorException();
        }
    }
}


Listing::CsvLister::CsvLister(std::shared_ptr<ProcessSummary> process_summary, const std::wstring& filename, bool append, std::shared_ptr<const CaseAccess> case_access)
    :   Lister(std::move(process_summary)),
        m_caseAccess(std::move(case_access)),
        m_useLevelKey(false)
{
    if( m_caseAccess == nullptr )
        throw CSProException("The CSV lister can only be used when working with applications that use an input dictionary.");

    bool write_header = ( !append || !PortableFunctions::FileIsRegular(filename) );

    m_file = OpenListingFile(filename, append);

    std::wstring header = _T("Source");

    // get the dictionary ID items
    std::vector<std::vector<const CDictItem*>> id_items_by_level = m_caseAccess->GetDataDict().GetIdItemsByLevel();

    if( id_items_by_level.size() > 1 )
    {
        m_useLevelKey = true;
        header.append(_T(",Level"));
    }

    // the length must support at least all of the commas and the null terminator
    size_t maximum_columns_length = id_items_by_level.front().size() + 1;

    for( const CDictItem* dict_item : id_items_by_level.front() )
    {
        maximum_columns_length += GetMaximumDelimitedLength(*dict_item);
        header.append(_T(",")).append(dict_item->GetName());
    }

    m_keyColumnsText = std::make_unique<TCHAR[]>(maximum_columns_length);
    ASSERT(m_keyColumnsText[0] == 0);

    header.append(_T(",Type,Number,Text"));

    if( write_header )
        m_file->WriteLine(header);
}


Listing::CsvLister::~CsvLister()
{
    if( m_file != nullptr )
        m_file->Close();
}


void Listing::CsvLister::WriteMessages(const Messages& messages)
{
    static constexpr const TCHAR* TypeNames[] =
    {
        _T("Abort"),
        _T("Error"),
        _T("Warning"),
        _T("User")
    };

    std::wstring delimited_source = Encoders::ToCsv(messages.source);

    for( const Message& message : messages.messages )
    {
        const TCHAR* type_name;

        if( message.details.has_value() )
        {
            size_t index = static_cast<size_t>(message.details->type);
            ASSERT(index < _countof(TypeNames));
            type_name = TypeNames[index];
        }

        else
        {
            type_name = _T("Write");
        }

        m_file->WriteFormattedLine(_T("%s%s%s,%s,%s,%s,%s"),
            delimited_source.c_str(),
            m_useLevelKey ? _T(",") : _T(""),
            m_useLevelKey ? message.level_key.c_str() : _T(""),
            m_keyColumnsText.get(),
            type_name,
            message.details.has_value() ? IntToString(message.details->number).GetString() : _T(""),
            Encoders::ToCsv(message.text).c_str());
    }
}


void Listing::CsvLister::ProcessCaseSource(const Case* data_case)
{
    TCHAR* key_column_itr = m_keyColumnsText.get();

    // look at the 20220810 note in Exopfile.cpp to see that this can be done in the 
    // constructor if the lister is created after the CaseAccess object is initialized
    if( m_idCaseItems.empty() )
    {
        if( !m_caseAccess->IsInitialized() )
        {
            ASSERT(data_case == nullptr);
            _tcscpy(key_column_itr, SO::GetRepeatingCharacterString(',', m_caseAccess->GetDataDict().GetIdItemsByLevel().front().size() - 1));
            return;
        }

        const auto& case_levels = m_caseAccess->GetCaseMetadata().GetCaseLevelsMetadata();
        m_idCaseItems = case_levels.front()->GetIdCaseRecordMetadata()->GetCaseItems();
    }

    std::optional<CaseItemIndex> index;

    if( data_case != nullptr )
        index.emplace(data_case->GetRootCaseLevel().GetIdCaseRecord().GetCaseItemIndex());

    for( const CaseItem* case_item : m_idCaseItems )
    {
        if( index.has_value() )
        {
            if( case_item->IsTypeNumeric() )
            {
                double value = assert_cast<const NumericCaseItem&>(*case_item).GetValueForOutput(*index);

                const CDictItem& dict_item = case_item->GetDictionaryItem();
                NumberConverter::DoubleToText(value, key_column_itr, dict_item.GetCompleteLen(), dict_item.GetDecimal(), false, true);

                // count and then left-trim spaces
                size_t number_spaces = 0;

                for( ; number_spaces < dict_item.GetCompleteLen(); ++number_spaces )
                {
                    if( !std::iswspace(key_column_itr[number_spaces]) )
                        break;
                }

                if( number_spaces == 0 )
                {
                    key_column_itr += dict_item.GetCompleteLen();
                }

                else
                {
                    for( size_t i = number_spaces; i < dict_item.GetCompleteLen(); ++i, ++key_column_itr )
                        *key_column_itr = key_column_itr[number_spaces];
                }                    
            }

            else
            {
                ASSERT(case_item->IsTypeString());

                // right-trim spaces and then delimit the string
                wstring_view value_sv = SO::TrimRight(assert_cast<const StringCaseItem&>(*case_item).GetValueSV(*index));
                std::wstring delimited_value = Encoders::ToCsv(value_sv);

                _tcscpy(key_column_itr, delimited_value.c_str());

                key_column_itr += delimited_value.length();
            }
        }

        *(key_column_itr++) = ',';
    }

    // terminate the string where the last comma was
    *( key_column_itr - 1 ) = 0;
}
