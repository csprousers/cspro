#include "stdafx.h"
#include "ExcelLister.h"
#include <zToolsO/NumberConverter.h>
#include <zToolsO/VariantVisitOverload.h>
#include <zExcelO/ExcelWriter.h>
#include <zDictO/Rules.h>


Listing::ExcelLister::ExcelLister(std::shared_ptr<ProcessSummary> process_summary, const std::string& file_path, std::shared_ptr<const CaseAccess> case_access)
    :   Lister(std::move(process_summary)),
        m_row(0),
        m_caseAccess(std::move(case_access)),
        m_useLevelKey(false)
{
    if( m_caseAccess == nullptr )
        throw CSProException("The Excel lister can only be used when working with applications that use an input dictionary.");

    m_excelWriter = std::make_unique<ExcelWriter>();
    m_excelWriter->CreateWorkbook(file_path);
    m_excelWriter->AddAndSetCurrentWorksheet("CSPro Listing");

    // get the dictionary ID items
    std::vector<std::vector<const CDictItem*>> id_items_by_level = m_caseAccess->GetDataDict().GetIdItemsByLevel();
    m_keyValues.resize(id_items_by_level.front().size());

    // write the header
    constexpr unsigned MaxSourceLength = 16;
    constexpr unsigned MaxTypeLength = 7;
    constexpr unsigned MaxNumberLength = 8;
    constexpr unsigned MaxTextLength = 80;

    lxw_format* header_format = m_excelWriter->GetFormat(ExcelWriter::Format::Bold);

    uint16_t column = 0;
    std::vector<double> column_widths;

    auto add_column = [&](const cs::string_sz text)
    {
        m_excelWriter->Write(m_row, column, text, header_format);
        column_widths.emplace_back(ExcelWriter::GetWidthForText(ExcelWriter::TextType::Characters, SO::WideLength(text)));
        ++column;
    };

    auto adjust_column_width = [&](const double width)
    {
        column_widths.back() = std::max(column_widths.back(), width);
    };

    auto get_column_width_for_item = [](const CDictItem& dict_item)
    {
        const ExcelWriter::TextType text_type = ( dict_item.GetContentType() == ContentType::Numeric ) ? ExcelWriter::TextType::Numbers :
                                                                                                         ExcelWriter::TextType::Characters;
        return ExcelWriter::GetWidthForText(text_type, dict_item.GetCompleteLen());
    };

    auto calculate_column_width_for_ids = [&](const std::vector<const CDictItem*>& dict_items)
    {
        double width = 0;

        for( const CDictItem* dict_item : dict_items )
        {
            ASSERT(DictionaryRules::CanBeIdItem(*dict_item));
            width += get_column_width_for_item(*dict_item);
        }

        return width;
    };

    add_column("Source");
    adjust_column_width(ExcelWriter::GetWidthForText(ExcelWriter::TextType::Characters, MaxSourceLength));
    adjust_column_width(calculate_column_width_for_ids(id_items_by_level.front()));

    // add the level key
    if( id_items_by_level.size() > 1 )
    {
        m_useLevelKey = true;
        add_column("Level");

        double level_key_width = 0;

        for( size_t level = 1; level < id_items_by_level.size(); ++level )
            level_key_width += calculate_column_width_for_ids(id_items_by_level[level]);

        adjust_column_width(level_key_width);
    }

    // add the primary level items
    for( const CDictItem* const dict_item : id_items_by_level.front() )
    {
        add_column(dict_item->GetName());
        adjust_column_width(get_column_width_for_item(*dict_item));
    }

    add_column("Type");
    adjust_column_width(ExcelWriter::GetWidthForText(ExcelWriter::TextType::Characters, MaxTypeLength));

    add_column("Number");
    adjust_column_width(ExcelWriter::GetWidthForText(ExcelWriter::TextType::Numbers, MaxNumberLength));

    add_column("Text");
    adjust_column_width(ExcelWriter::GetWidthForText(ExcelWriter::TextType::Characters, MaxTextLength));

    ++m_row;

    // set the column widths and set a froze pane on the header row
    for( uint16_t c = 0; c < column_widths.size(); ++c )
        m_excelWriter->SetColumnWidth(c, column_widths[c]);

    m_excelWriter->FreezeTopRow();
}


Listing::ExcelLister::~ExcelLister()
{
    if( m_excelWriter != nullptr )
        m_excelWriter->Close();
}


void Listing::ExcelLister::WriteMessages(const Messages& messages)
{
    for( const Message& message : messages.messages )
    {
        uint16_t column = 0;

        // make the source key a link (if possible)
        lxw_format* link_format = nullptr;
        const std::optional<std::string> case_uri = CreateCaseUri(m_inputDataUri, m_caseKeyUuid);

        if( case_uri.has_value() )
        {
            m_excelWriter->WriteUrl(m_row, column, *case_uri);
            link_format = m_excelWriter->GetFormat(ExcelWriter::Format::Underline);
        }

        m_excelWriter->Write(m_row, column, messages.source, link_format);

        ++column;

        if( m_useLevelKey )
        {
            m_excelWriter->Write(m_row, column, message.level_key);
            ++column;
        }

        for( const KeyValueType& key_value : m_keyValues )
        {
            std::visit(
                overload
                {
                    [&](const std::monostate&)       { m_excelWriter->Write(m_row, column, SO::Empty_string); },
                    [&](const SharableString& value) { m_excelWriter->Write(m_row, column, *value); },
                    [&](const auto& value)           { m_excelWriter->Write(m_row, column, value); }

                }, key_value);

            ++column;
        }

        m_excelWriter->Write(m_row, column, GetMessageTypeText(message.details));
        ++column;

        if( message.details.has_value() )
            m_excelWriter->Write(m_row, column, message.details->number);

        ++column;

        m_excelWriter->Write(m_row, column, *message.text);
        ++column;

        ++m_row;
    }
}


void Listing::ExcelLister::ProcessCaseSourceDetails(const ConnectionString& connection_string, const CDataDict& dictionary)
{
    m_inputDataUri = CreateDataUri(connection_string, dictionary);
}


void Listing::ExcelLister::ProcessCaseSource(const Case* const data_case)
{
    if( data_case == nullptr )
    {
        m_caseKeyUuid.reset();
        std::fill(m_keyValues.begin(), m_keyValues.end(), std::monostate());
    }

    else
    {
        // look at the 20220810 note in Exopfile.cpp to see that this can be done in the
        // constructor if the lister is created after the CaseAccess object is initialized
        if( m_idCaseItems.empty() )
        {
            ASSERT(m_caseAccess->IsInitialized());
            const std::vector<CaseLevelMetadata>& case_levels = m_caseAccess->GetCaseMetadata().GetCaseLevelsMetadata();
            m_idCaseItems = case_levels.front().GetIdCaseRecordMetadata().GetCaseItems();
            ASSERT(m_idCaseItems.size() == m_keyValues.size());
        }

        m_caseKeyUuid.emplace(data_case->GetKey(), data_case->GetUuid());

        const CaseItemIndex index = data_case->GetRootCaseLevel().GetIdCaseRecord().GetCaseItemIndex();
        auto key_value_itr = m_keyValues.begin();

        for( const CaseItem* const case_item : m_idCaseItems )
        {
            if( IsNumeric(case_item->GetDataType()) )
            {
                const double value = assert_cast<const NumericCaseItem&>(*case_item).GetValueForOutput(index);

                *key_value_itr = ( !IsSpecial(value) ) ? KeyValueType(value) :
                                 ( value == NOTAPPL )  ? KeyValueType(std::monostate()) :
                                                         KeyValueType(SpecialValues::ValueToString(value));
            }

            else
            {
                ASSERT(IsString(case_item->GetDataType()));
                SharableString value = assert_cast<const StringCaseItem&>(*case_item).GetSharableString(index);
                value.MakeTrimRight();
                *key_value_itr = std::move(value);
            }

            ++key_value_itr;
        }
    }
}
