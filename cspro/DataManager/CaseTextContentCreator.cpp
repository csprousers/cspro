#include "StdAfx.h"
#include "CaseTextContentCreator.h"
#include "CaseTextContentCreatorSettings.h"
#include "ViewOptionsHelper.h"
#include <zToolsO/NewlineSubstitutor.h>
#include <zDictO/DictionaryIterator.h>
#include <zDictO/ValueProcessor.h>
#include <zCaseO/CaseItemPrinter.h>
#include <zCaseO/TextToCaseConverter.h>


CREATE_JSON_KEY(colorizeItems)
CREATE_JSON_KEY(showDetailsPane)


// --------------------------------------------------------------------------
// CaseTextContentCreatorSettings
// --------------------------------------------------------------------------

CaseTextContentCreatorSettings CaseTextContentCreatorSettings::CreateFromJson(const JsonNode& json_node)
{
    return CaseTextContentCreatorSettings
    {
        json_node.Get<bool>(JK::colorizeItems),
        json_node.Get<bool>(JK::showDetailsPane),
    };
}


void CaseTextContentCreatorSettings::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::colorizeItems, colorize_items)
               .Write(JK::showDetailsPane, show_details_pane)
               .EndObject();
}



// --------------------------------------------------------------------------
// CaseTextContentCreator::Formatter declaration
// --------------------------------------------------------------------------

struct CaseTextContentCreator::Formatter
{
    struct StartLen
    {
        size_t start; // start is 0-based
        size_t len;
    };

    struct Entity
    {
        const DictNamedBase* dict_element; // null for record type
        bool is_id_item;
        StartLen start_len;
        std::shared_ptr<const ValueProcessor> value_processor;
    };

    struct ParsedEntity
    {
        std::string text;
        const Entity* entity; // null for line breaks
        const char* class_type; // null if not applicable
    };

    std::string html_part1_pre_title;
    std::string html_part2_pre_case;
    std::string html_part3_pre_case_details;
    std::string html_part4_end;

    std::variant<const CDictRecord*, StartLen> single_dict_record_or_record_type_start_len;
    std::map<std::string, const CDictRecord*, std::less<>> record_type_map;

    std::map<const CDictRecord*, std::vector<Entity>> entity_map;

    void ReadHtmlTemplate();

    void SetUpRecordTypeMap(const CDataDict& dictionary);

    static void AddEntities(std::vector<Entity>& entities, const CDictRecord& dict_record, bool is_id_record);

    const CDictRecord* GetDictRecordForLine(std::string_view line_sv) const;

    std::vector<ParsedEntity> ParseCaseText(const std::string& case_text) const;

    std::tuple<std::string, std::string> CreateCaseHtml(const CDataDict& dictionary, const CaseTextContentCreatorSettings& settings,
                                                        const std::vector<ParsedEntity>& parsed_entities) const;
};



// --------------------------------------------------------------------------
// CaseTextContentCreator
// --------------------------------------------------------------------------

CaseTextContentCreator::CaseTextContentCreator(CaseHoldingDoc& case_holding_doc)
    :   CaseContentCreatorBase(case_holding_doc),
        m_caseTextContentCreatorSettings(case_holding_doc.GetSettings<CaseTextContentCreatorSettings>()),
        m_textToCaseConverter(std::make_unique<TextToCaseConverter>(case_holding_doc.GetCaseAccess().GetCaseMetadata()))
{
}


CaseTextContentCreator::~CaseTextContentCreator()
{
}


const wchar_t* CaseTextContentCreator::GetSaveTitle() const
{
    return L"Save Case Text";
}


std::vector<const char*> CaseTextContentCreator::GetSaveFormats() const
{
    return { FileExtensions::Text,
             FileExtensions::HTML };
}


UINT CaseTextContentCreator::GetViewOptionsMenuResourceId() const
{
    return IDR_VIEW_CASE_TEXT;
}


bool CaseTextContentCreator::ProcessViewOptionsMenu(const std::variant<UINT, CCmdUI*> data)
{
    switch( ViewOptionsHelper::GetCommandId(data) )
    {
        case ID_VIEW_OPTIONS_TEXT_COLORIZE:     return ViewOptionsHelper::HandleBooleanCheck(data, m_caseTextContentCreatorSettings->colorize_items);
        case ID_VIEW_OPTIONS_TEXT_DETAILS_PANE: return ViewOptionsHelper::HandleBooleanCheck(data, m_caseTextContentCreatorSettings->show_details_pane);
        default:                                return ViewOptionsHelper::HandleUnknownCommand(data);
    }
}


std::string CaseTextContentCreator::GetCaseText() const
{
    ASSERT(m_dataCase != nullptr);
    return m_textToCaseConverter->CaseToTextUtf8(*m_dataCase);
}


SharableString CaseTextContentCreator::GetTextContentWorker()
{
    return GetCaseText();
}


SharableString CaseTextContentCreator::GetHtmlContentWorker(bool /*embed_resources = false*/)
{
    const std::string title = "Case " + m_dataCase->GetSingleLineKey();
    const std::string case_text = GetCaseText();

    // the plain text view is easy to construct
    if( !m_caseTextContentCreatorSettings->colorize_items &&
        !m_caseTextContentCreatorSettings->show_details_pane )
    {
        return Encoders::ToPreformattedTextHtml(title, case_text);
    }

    // otherwise we need to build a HTML page using the template
    if( m_formatter == nullptr )
    {
        m_formatter = std::make_unique<Formatter>();
        m_formatter->ReadHtmlTemplate();
        m_formatter->SetUpRecordTypeMap(m_caseHoldingDoc.GetDictionary());
    }

    // parse the case text into each entity and create the case HTML
    const std::vector<Formatter::ParsedEntity> parsed_entities = m_formatter->ParseCaseText(case_text);
    const auto [case_html, script] = m_formatter->CreateCaseHtml(m_caseHoldingDoc.GetDictionary(), *m_caseTextContentCreatorSettings, parsed_entities);

    return SO::Concatenate(m_formatter->html_part1_pre_title,
                           Encoders::ToHtml(title),
                           m_formatter->html_part2_pre_case,
                           case_html,
                           m_formatter->html_part3_pre_case_details,
                           script,
                           m_formatter->html_part4_end);
}



// --------------------------------------------------------------------------
// CaseTextContentCreator::Formatter definitions
// --------------------------------------------------------------------------

void CaseTextContentCreator::Formatter::ReadHtmlTemplate()
{
    const std::string html = FileIO::ReadText(Path::Combine(Html::GetDirectory(Html::Subdirectory::Visualizations),
                                                            "text-case.html"));

    size_t current_offset = 0;

    auto find_section = [&](const std::string_view text_sv)
    {
        const size_t pos = html.find(text_sv, current_offset);

        if( pos == std::string::npos )
            throw CSProException(SO::Concatenate("The HTML template is missing the section: ", text_sv));

        std::string section_text = html.substr(current_offset, pos - current_offset);

        current_offset = pos + text_sv.length();

        return section_text;
    };

    html_part1_pre_title = find_section("~~TITLE~~");
    html_part2_pre_case = find_section("~~CASE~~");
    html_part3_pre_case_details = find_section("// ~~CASE_DETAILS~~"),
    html_part4_end = html.substr(current_offset);
}


void CaseTextContentCreator::Formatter::SetUpRecordTypeMap(const CDataDict& dictionary)
{
    std::vector<Entity> id_item_and_record_type_entities;

    // add the record type information
    if( dictionary.GetRecTypeLen() == 0 )
    {
        ASSERT(dictionary.GetNumLevels() == 1 &&
               dictionary.GetLevel(0).GetNumRecords() == 1);

        single_dict_record_or_record_type_start_len = dictionary.GetLevel(0).GetRecord(0);
    }

    else
    {
        single_dict_record_or_record_type_start_len = StartLen { static_cast<size_t>(dictionary.GetRecTypeStart()) - 1,
                                                                 static_cast<size_t>(dictionary.GetRecTypeLen()) };
        id_item_and_record_type_entities.emplace_back(Entity { nullptr, false, std::get<StartLen>(single_dict_record_or_record_type_start_len) });
    }

    // map the records
    DictionaryIterator::Foreach<CDictRecord>(dictionary,
        [&](const CDictRecord& dict_record)
        {
            if( dict_record.IsIdRecord() )
            {
                AddEntities(id_item_and_record_type_entities, dict_record, true);
            }

            else
            {
                // add this record's record type...
                if( std::holds_alternative<StartLen>(single_dict_record_or_record_type_start_len) )
                {
                    ASSERT(dictionary.GetRecTypeLen() == SO::WideLength(dict_record.GetRecTypeVal()));
                    record_type_map.try_emplace(dict_record.GetRecTypeVal(), &dict_record);
                }

                // ...and items
                std::vector<Entity>& entities = entity_map.try_emplace(&dict_record, id_item_and_record_type_entities).first->second;
                ASSERT(entities.size() == id_item_and_record_type_entities.size());

                AddEntities(entities, dict_record, false);
            }
        });
}


const CDictRecord* CaseTextContentCreator::Formatter::GetDictRecordForLine(const std::string_view line_sv) const
{
    if( std::holds_alternative<const CDictRecord*>(single_dict_record_or_record_type_start_len) )
    {
        return std::get<const CDictRecord*>(single_dict_record_or_record_type_start_len);
    }

    else
    {
        const StartLen& record_type_start_len = std::get<StartLen>(single_dict_record_or_record_type_start_len);
        const size_t record_type_offset = SO::WideGetOffset(line_sv, record_type_start_len.start);

        if( record_type_offset != std::string_view::npos )
        {
            const std::string_view record_type_sv = SO::WideSubstring(line_sv.substr(record_type_offset), 0, record_type_start_len.len);
            const auto& lookup = record_type_map.find(record_type_sv);

            if( lookup != record_type_map.cend() )
                return lookup->second;
        }

        throw ProgrammingErrorException();
    }
}


void CaseTextContentCreator::Formatter::AddEntities(std::vector<Entity>& entities, const CDictRecord& dict_record, const bool is_id_record)
{
    // add the items
    for( int i = 0; i < dict_record.GetNumItems(); ++i )
    {
        const CDictItem* const dict_item = dict_record.GetItem(i);

        for( size_t occurrence = 0; occurrence < dict_item->GetItemSubitemOccurs(); ++occurrence )
        {
            entities.emplace_back(Entity { dict_item,
                                           is_id_record,
                                           StartLen { static_cast<size_t>(dict_item->GetStart() + ( occurrence * dict_item->GetLen() ) - 1),
                                                      static_cast<size_t>(dict_item->GetLen()) },
                                           ValueProcessor::CreateValueProcessor(*dict_item) });
        }
    }

    // sort by the start position, item/subitem status, and then name
    if( !is_id_record )
    {
        std::sort(entities.begin(), entities.end(),
            [&](const Entity& entity1, const Entity& entity2)
            {
                if( entity1.start_len.start != entity2.start_len.start )
                    return ( entity1.start_len.start < entity2.start_len.start );

                // sort record types first
                if( entity1.dict_element == nullptr || entity2.dict_element == nullptr )
                    return ( entity1.dict_element == nullptr );

                // sort items before subitems
                if( entity1.dict_element->GetElementType() == DictElementType::Item &&
                    entity2.dict_element->GetElementType() == DictElementType::Item &&
                    assert_cast<const CDictItem*>(entity1.dict_element)->IsSubitem() != assert_cast<const CDictItem*>(entity2.dict_element)->IsSubitem() )
                {
                    return !assert_cast<const CDictItem*>(entity1.dict_element)->IsSubitem();
                }

                return ( entity1.dict_element->GetName() < entity2.dict_element->GetName() );
            });
    }
}


std::vector<CaseTextContentCreator::Formatter::ParsedEntity> CaseTextContentCreator::Formatter::ParseCaseText(const std::string& case_text) const
{
    std::vector<ParsedEntity> parsed_entities;

    SO::ForeachLine(case_text, false,
        [&](std::string_view line_sv)
        {
            // add a line break when the case has multiple lines
            if( !parsed_entities.empty() )
                parsed_entities.emplace_back(ParsedEntity { "\n", nullptr, nullptr });

            // determine what record we are using
            const CDictRecord* const dict_record = GetDictRecordForLine(line_sv);
            const auto& entities_lookup = entity_map.find(dict_record);
            const std::vector<Entity>& line_entities = ( entities_lookup != entity_map.cend() ) ? entities_lookup->second :
                                                                                                  throw ProgrammingErrorException();

            // parse the case line
            int id_counter = 0;
            int item_counter = 0;
            size_t wide_pos = 0;

            while( !line_sv.empty() )
            {
                auto entity_lookup = std::find_if(line_entities.cbegin(), line_entities.cend(),
                                                  [&](const Entity& entity) { return ( wide_pos == entity.start_len.start ); });

                while( entity_lookup != line_entities.cend() &&
                       wide_pos == entity_lookup->start_len.start )
                {
                    const char* const class_type =
                        ( entity_lookup->dict_element == nullptr ) ? "recordType" :
                        ( entity_lookup->is_id_item )              ? ( ( ( id_counter++ % 2 ) == 0 ) ? "id0" : "id1" ) :
                                                                     ( ( ( item_counter++ % 2 ) == 0 ) ? "item0" : "item1" );

                    parsed_entities.emplace_back(ParsedEntity { std::string(SO::WideSubstring(line_sv, 0, entity_lookup->start_len.len)),
                                                                &(*entity_lookup),
                                                                class_type });

                    ++entity_lookup;
                }

                line_sv.remove_prefix(TC::Utf8BytesFromFirstByte(line_sv.front()));
                ++wide_pos;
            }
        });

    return parsed_entities;
}


std::tuple<std::string, std::string> CaseTextContentCreator::Formatter::CreateCaseHtml(const CDataDict& dictionary,
                                                                                       const CaseTextContentCreatorSettings& settings,
                                                                                       const std::vector<ParsedEntity>& parsed_entities) const
{
    std::string html;
    std::string script = "const details = ";
    std::unique_ptr<std::vector<std::vector<const ParsedEntity*>>> details_pane_data;

    if( settings.show_details_pane )
    {
        details_pane_data = std::make_unique<std::vector<std::vector<const ParsedEntity*>>>();
    }

    else
    {
        script.append("undefined;");
    }

    size_t next_item_start_pos = 0;

    for( const ParsedEntity& parsed_entity : parsed_entities )
    {
        // on newlines, add spacing to added to match the template's indentation
        if( parsed_entity.text == "\n" )
        {
            html.append("<br>\n        ");
            next_item_start_pos = 0;
            continue;
        }

        ASSERT(parsed_entity.entity != nullptr &&
               parsed_entity.class_type != nullptr);

        // subitems will not be output directly, but we will show their data in the details pane
        if( parsed_entity.entity->dict_element != nullptr &&
            assert_cast<const CDictItem*>(parsed_entity.entity->dict_element)->IsSubitem() )
        {
            if( details_pane_data != nullptr )
            {
                ASSERT(!details_pane_data->empty());
                details_pane_data->back().emplace_back(&parsed_entity);
            }

            continue;
        }

        // add blank spaces (e.g., for a first level record's undefined second level IDs)
        const size_t blank_spaces = parsed_entity.entity->start_len.start - next_item_start_pos;
        ASSERT(blank_spaces <= parsed_entity.entity->start_len.start);

        if( blank_spaces != 0 )
            html.append(Encoders::ToHtml(SO::GetRepeatingCharacterString(' ', blank_spaces)));

        // create links when using the details pane
        if( details_pane_data != nullptr )
        {
            details_pane_data->emplace_back().emplace_back(&parsed_entity);
            const std::string link_text = IntToString(details_pane_data->size());

            html.append("<a id=\"cs")
                .append(link_text)
                .append("\">");
        }

        // wrap in a class when colorizing
        if( settings.colorize_items )
        {
            html.append("<span class=\"")
                .append(parsed_entity.class_type)
                .append("\">");
        }

        html.append(Encoders::ToHtml(parsed_entity.text));

        if( settings.colorize_items )
            html.append("</span>");

        if( details_pane_data != nullptr )
            html.append("</a>");

        next_item_start_pos = parsed_entity.entity->start_len.start + parsed_entity.entity->start_len.len;
    }

    // build the script for the details pane
    if( details_pane_data != nullptr )
    {
        const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();
        json_writer->BeginArray();

        // add information about the dictionary at index 0
        json_writer->BeginObject()
                    .BeginArray(JK::metadata)
                    .Write("Type").Write("Dictionary")
                    .Write("Name").Write(dictionary.GetName())
                    .Write("Label").Write(dictionary.GetLabel())
                    .EndArray()
                    .EndObject();

        // add information about each parsed entity
        for( const std::vector<const ParsedEntity*>& these_parsed_entities : *details_pane_data )
        {
            json_writer->BeginObject()
                        .BeginArray(JK::metadata);

            bool first_parsed_entity = true;

            for( const ParsedEntity* const parsed_entity : these_parsed_entities )
            {
                ASSERT(parsed_entity->entity != nullptr);
                const CDictItem* const dict_item = assert_nullable_cast<const CDictItem*>(parsed_entity->entity->dict_element);

                // separate parsed entities with blank rows
                if( first_parsed_entity )
                {
                    first_parsed_entity = false;
                }

                else
                {
                    json_writer->Write("").Write("");
                }

                json_writer->Write("Type");

                if( dict_item == nullptr )
                {
                    const auto& record_lookup = record_type_map.find(parsed_entity->text);
                    json_writer->Write(FormatText("Record Type (Record %s)",
                                                  ( record_lookup != record_type_map.cend() ) ? record_lookup->second->GetName().c_str() :
                                                                                                ReturnProgrammingError("")));
                }

                else if( parsed_entity->entity->is_id_item )
                {
                    const DictLevel* const dict_level = dict_item->GetLevel();
                    json_writer->Write(FormatText("ID Item (Level %d: %s)",
                                                  static_cast<int>(dict_item->GetLevel()->GetLevelNumber() + 1),
                                                  dict_level->GetName().c_str()));
                }

                else
                {
                    json_writer->Write(FormatText("%s (Record %s)",
                                                  dict_item->IsSubitem() ? "Subitem" : "Item",
                                                  dict_item->GetRecord()->GetName().c_str()));
                }

                if( dict_item != nullptr )
                {
                    json_writer->Write("Name").Write(dict_item->GetName())
                                .Write("Label").Write(dict_item->GetLabel());
                }

                json_writer->Write("Start Position").Write(parsed_entity->entity->start_len.start + 1)
                            .Write("Length").Write(parsed_entity->entity->start_len.len)
                            .Write("Value (Text)").Write(parsed_entity->text);

                if( dict_item != nullptr )
                {
                    std::set<std::string, std::less<>> written_values;

                    auto write_value = [&](const cs::string_sz description, std::string_view value_sv)
                    {
                        if( IsString(dict_item->GetDataType()) )
                            value_sv = SO::TrimRightSpace(value_sv);

                        // only write identical values once
                        if( written_values.find(value_sv) == written_values.cend() )
                        {
                            written_values.emplace(value_sv);
                            json_writer->Write(description).Write(value_sv);
                        }
                    };

                    ASSERT(parsed_entity->entity->value_processor != nullptr);

                    const std::variant<double, std::string> value =
                        IsNumeric(dict_item->GetDataType())
                        ? std::variant<double, std::string>(parsed_entity->entity->value_processor->GetNumericFromInput(parsed_entity->text))
                        : std::variant<double, std::string>(parsed_entity->text);

                    // write numeric values properly formatted (e.g., with decimal marks)
                    if( std::holds_alternative<double>(value) )
                    {
                        write_value("Value (Numeric)", CaseItemPrinter::FormatNumber(*dict_item, std::get<double>(value), false));
                    }

                    // write alpha values that would appear differently due to newline escaping
                    else if( IsString(dict_item->GetDataType()) )
                    {
                        const std::string formatted_value = NewlineSubstitutor::UnicodeNLToNewline(parsed_entity->text);

                        if( formatted_value != parsed_entity->text )
                            write_value("Value (Alpha)", formatted_value);
                    }

                    // write labels from the value set
                    if( dict_item->HasValueSets() )
                    {
                        for( const DictValueSet& dict_value_set : dict_item->GetValueSets() )
                        {
                            const std::shared_ptr<const ValueProcessor> value_processor =
                                ValueProcessor::CreateValueProcessor(*dict_item, &dict_value_set);

                            const DictValue* const dict_value = std::visit(
                                [&](const auto& this_value) { return value_processor->GetDictValue(this_value); },
                                value
                            );

                            if( dict_value != nullptr )
                            {
                                write_value(FormatText("Value (Label: %s)", dict_value_set.GetName().c_str()),
                                            UTF8_TODO::GetUtf8(dict_value->GetLabel()));
                            }
                        }
                    }
                }
            }

            json_writer->EndArray()
                        .EndObject();
        }

        json_writer->EndArray();

        script.append(json_writer->ReleaseString())
              .push_back(';');
    }

    return { std::move(html), std::move(script) };
}
