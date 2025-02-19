#include "stdafx.h"
#include "CaseToHtmlConverter.h"
#include "BinaryCaseItem.h"
#include "CaseItemReference.h"
#include "NoteSorter.h"
#include <zUtilO/Interapp.h>
#include <zUtilO/MimeType.h>
#include <zHtml/HtmlWriter.h>
#include <zUtilF/SystemIcon.h>


namespace
{
    constexpr std::string_view Divider_sv = u8"<span class=\"c2h_divider\">&nbsp; ► &nbsp;</span>";

    constexpr std::string_view PartialSaveAnchor_sv = "PartialSave";
    constexpr const char* NoteAnchorFormatter = "Note%p";

    // the note icon is also in the html/images directory
    constexpr std::string_view NoteIcon_sv =
        "data:image/png;base64,"
        "iVBORw0KGgoAAAANSUhEUgAAAA8AAAAMCAYAAAC9QufkAAAAAXNSR0IArs4c6QAAAARnQU1BAACx"
        "jwv8YQUAAAAJcEhZcwAADsQAAA7EAZUrDhsAAABCSURBVChTtc3BCQAwCENR958qm6VYKJRCMII9"
        "fMjBhwGAEdFuu4NJvz84t6rEThLnVpXYSeLcbzc8N/OfnWZxNwBcEpWepmQw/5cAAAAASUVORK5C"
        "YII=";
}


CaseToHtmlConverter::CaseToHtmlConverter()
    :   m_statuses(Statuses::ShowIfNotDefault),
        m_caseConstructionErrors(CaseConstructionErrors::Show),
        m_nameDisplay(NameDisplay::Label),
        m_recordOrientation(RecordOrientation::Horizontal),
        m_occurrenceDisplay(OccurrenceDisplay::Label),
        m_itemTypeDisplay(ItemTypeDisplay::ItemSubitem),
        m_blankValues(BlankValues::Show),
        m_notes(Notes::Show),
        m_caseItemPrinter(CaseItemPrinter::Format::CaseTree)
{
}


const std::vector<std::string>* CaseToHtmlConverter::GetCaseConstructionErrors() const
{
    return nullptr;
}


bool CaseToHtmlConverter::EmbedResources() const
{
    return true;
}


std::string CaseToHtmlConverter::CreateFileUrl(const std::string& file_path)
{
    return ReturnProgrammingError(Encoders::ToFileUrl(file_path));
}


bool CaseToHtmlConverter::AddBinaryDataOpenAndSaveUrls() const
{
    return false;
}


std::tuple<std::string, std::string> CaseToHtmlConverter::CreateBinaryDataOpenAndSaveUrls(const BinaryCaseItem& /*binary_case_item*/, const CaseItemIndex& /*index*/,
                                                                                          const std::string& /*suggested_filename*/)
{
    return ReturnProgrammingError(std::make_tuple(std::string(), std::string()));
}


std::string CaseToHtmlConverter::CreateBinaryDataUrl(const BinaryCaseItem& /*binary_case_item*/, const CaseItemIndex& /*index*/,
                                                     const std::string& /*mime_type*/, const std::string& /*suggested_filename*/)
{
    return std::string();
}


std::string CaseToHtmlConverter::ToHtml(const Case& data_case)
{
    // set the proper dictionary language
    if( !m_languageName.empty() )
    {
        const CDataDict& dictionary = data_case.GetCaseMetadata().GetDictionary();
        const std::optional<size_t> language_index = dictionary.IsLanguageDefined(m_languageName);

        if( language_index.has_value() && dictionary.GetCurrentLanguageIndex() != *language_index )
            dictionary.SetCurrentLanguage(*language_index);

        // if changed, the language could be set back to the initial language, but for now, we'll keep
        // the dictionary in the new language
    }


    HtmlStringWriter html_writer;

    // write the header
    html_writer.WriteRaw(HtmlWriter::DefaultHeader_sv);
    html_writer << "<title>" << data_case.GetCaseLabelOrKey() << "</title>\n";

    if( EmbedResources() )
    {
        html_writer << "<style>\n";
        html_writer.WriteRaw(Html::GetCSS(Html::CSS::CaseView));
        html_writer << "</style>";
    }

    else
    {
        html_writer << "<link rel=\"stylesheet\" href=\"";
        html_writer.WriteRaw(CreateFileUrl(Html::GetCSSFilePath(Html::CSS::CaseView)));
        html_writer << "\"";
    }

    html_writer << "\n</head>\n";

    // write the body
    html_writer << "<body>\n";

    // write the key and case label
    html_writer << "<p class=\"c2h_case_key\">" << data_case.GetKey();

    if( !data_case.GetCaseLabel().empty() && data_case.GetKey() != data_case.GetCaseLabel() )
    {
        html_writer.WriteRaw(Divider_sv);
        html_writer << data_case.GetCaseLabel();
    }

    html_writer << "</p>\n";

    // write the case note
    if( m_notes == Notes::Show )
    {
        const std::string& case_note = data_case.GetCaseNote();

        if( !case_note.empty() )
            html_writer << "<p class=\"c2h_case_note\">" << case_note << "</p>\n";
    }

    // write the statuses
    if( m_statuses != Statuses::Hide )
        WriteStatuses(html_writer, data_case);

    // potentially write the case construction errors
    std::optional<std::tuple<size_t, size_t, size_t>> case_construction_errors_initial_count_start_position_errors_length;

    if( m_caseConstructionErrors == CaseConstructionErrors::Show )
    {
        const std::vector<std::string>* const case_construction_errors = GetCaseConstructionErrors();

        case_construction_errors_initial_count_start_position_errors_length.emplace(( case_construction_errors != nullptr ) ? case_construction_errors->size() : 0,
                                                                                    html_writer.length(),
                                                                                    0);

        if( std::get<0>(*case_construction_errors_initial_count_start_position_errors_length) != 0 )
        {
            const std::string errors_html = GetCaseConstructionErrorsHtml(*case_construction_errors);
            std::get<2>(*case_construction_errors_initial_count_start_position_errors_length) = errors_html.length();
            html_writer.WriteRaw(errors_html);
        }
    }

    // write all levels
    for( const CaseLevel* const case_level : data_case.GetAllCaseLevels() )
        WriteCaseLevel(html_writer, data_case, *case_level);

    // write the notes table
    if( m_notes == Notes::Show && !data_case.GetNotes().empty() )
        WriteNotes(html_writer, data_case);

    html_writer << "</body>\n"
                << "</html>\n";

    std::string html = html_writer.str();

    // if any additional case construction errors occurred while writing the HTML,
    // (e.g., because binary data could not be read), rewrite the errors
    if( case_construction_errors_initial_count_start_position_errors_length.has_value() )
    {
        const std::vector<std::string>* const case_construction_errors = GetCaseConstructionErrors();

        if( case_construction_errors != nullptr &&
            case_construction_errors->size() != std::get<0>(*case_construction_errors_initial_count_start_position_errors_length) )
        {
            html.replace(std::get<1>(*case_construction_errors_initial_count_start_position_errors_length),
                         std::get<2>(*case_construction_errors_initial_count_start_position_errors_length),
                         GetCaseConstructionErrorsHtml(*case_construction_errors));
        }
    }

    return html;
}


template<typename T>
std::string CaseToHtmlConverter::GetDictionaryText(const T& t) const
{
    return ( m_nameDisplay == NameDisplay::Label ) ? UTF8_TODO::GetUtf8(t.GetLabel()) :
           ( m_nameDisplay == NameDisplay::Name )  ? t.GetName() :
                                                     SO::CreateColonSeparatedString(t.GetName(), UTF8_TODO::GetUtf8(t.GetLabel()));
}


template<typename T>
std::string CaseToHtmlConverter::GetOccurrenceLabel(const T& t, const size_t occurrence) const
{
    std::string occurrence_label;

    if( m_occurrenceDisplay == OccurrenceDisplay::Label )
        occurrence_label = UTF8_TODO::GetUtf8(t.GetOccurrenceLabels().GetLabel(occurrence));

    if( occurrence_label.empty() )
        occurrence_label = IntToString(occurrence + 1);

    return occurrence_label;
}


void CaseToHtmlConverter::WriteStatuses(HtmlWriter& html_writer, const Case& data_case) const
{
    ASSERT(m_statuses != Statuses::Hide);

    if( m_statuses == Statuses::Show && !data_case.GetUuid().empty() )
        html_writer << "<p class=\"c2h_status\">" << "UUID: " << data_case.GetUuid() << "</p>\n";

    if( m_statuses == Statuses::Show || data_case.GetDeleted() )
        html_writer << "<p class=\"c2h_status\">Case is " << ( data_case.GetDeleted() ? "" : "not " ) << "deleted</p>\n";

    if( m_statuses == Statuses::Show || data_case.GetVerified() )
        html_writer << "<p class=\"c2h_status\">Case is " << ( data_case.GetVerified() ? "" : "not " ) << "verified</p>\n";

    if( m_statuses == Statuses::Show || data_case.IsPartial() )
    {
        html_writer << "<p class=\"c2h_status\">";

        if( data_case.IsPartial() )
        {
            html_writer << "Case is partially saved in <b>"
                        << ( ( data_case.GetPartialSaveMode() == PartialSaveMode::Add )    ? "add" :
                             ( data_case.GetPartialSaveMode() == PartialSaveMode::Modify ) ? "modify" :
                                                                                             "verify" )
                        << "</b> mode";

            if( data_case.GetPartialSaveCaseItemReference() != nullptr )
            {
                const CaseItemReference& partial_save_case_item_reference = *data_case.GetPartialSaveCaseItemReference();

                html_writer << " on field "
                            << "<a class=\"c2h_partial_save_item_link\" href=\"#";
                html_writer.WriteRaw(PartialSaveAnchor_sv);
                html_writer << "\">"
                            << partial_save_case_item_reference.GetName()
                            << partial_save_case_item_reference.GetItemIndexHelper().GetMinimalOccurrencesText(partial_save_case_item_reference)
                            << "</a>";

                if( !partial_save_case_item_reference.GetLevelKey().empty() )
                {
                    html_writer << " on level <b>"
                                << partial_save_case_item_reference.GetLevelKey()
                                << "</b>";
                }
            }
        }

        else
        {
            html_writer << "Case is not partially saved";
        }

        html_writer << "</p>\n";
    }
}


std::string CaseToHtmlConverter::GetCaseConstructionErrorsHtml(const std::vector<std::string>& case_construction_errors)
{
    ASSERT(!case_construction_errors.empty());

    HtmlStringWriter html_writer;

    html_writer << "<p class=\"c2h_errors\">";

    for( size_t i = 0; i < case_construction_errors.size(); ++i )
    {
        if( i > 0 )
            html_writer.WriteNewline();

        html_writer << u8"\n⚠ " << case_construction_errors[i];
    }

    html_writer << "\n</p>\n";

    return html_writer.str();
}


void CaseToHtmlConverter::WriteCaseLevel(HtmlWriter& html_writer, const Case& data_case, const CaseLevel& case_level)
{
    // write out the level name and, if not on the root level, the level key
    html_writer << "<p class=\"c2h_level_name\">" << GetDictionaryText(case_level.GetCaseLevelMetadata().GetDictLevel());

    if( !case_level.GetLevelKey().IsEmpty() )
    {
        html_writer.WriteRaw(Divider_sv);
        html_writer << UTF8_TODO::GetUtf8(case_level.GetLevelKey());
    }

    html_writer << "</p>\n";

    // write the IDs and then each record
    WriteCaseRecord(html_writer, data_case, case_level.GetIdCaseRecord(), true);

    for( size_t record_number = 0; record_number < case_level.GetNumberCaseRecords(); ++record_number )
    {
        const CaseRecord& case_record = case_level.GetCaseRecord(record_number);
        WriteCaseRecord(html_writer, data_case, case_record, false);
    }
}


void CaseToHtmlConverter::WriteCaseRecord(HtmlWriter& html_writer, const Case& data_case, const CaseRecord& case_record, const bool is_id_record)
{
    const CDictRecord& dict_record = case_record.GetCaseRecordMetadata().GetDictRecord();
    CaseItemIndex index = case_record.GetCaseItemIndex();

    bool mark_current_partial_save_item_reference = ( m_statuses != Statuses::Hide &&
                                                      data_case.GetPartialSaveCaseItemReference() != nullptr &&
                                                      data_case.GetPartialSaveCaseItemReference()->OnCaseLevel(case_record) );

    // get the notes on this record
    std::vector<const Note*> record_notes;

    if( m_notes == Notes::Show )
    {
        for( const Note& note : data_case.GetNotes() )
        {
            const CaseItemReference* const case_item_reference = dynamic_cast<const CaseItemReference*>(&note.GetNamedReference());

            if( case_item_reference != nullptr &&
                case_item_reference->GetCaseItem().GetDictItem().GetRecord() == &dict_record &&
                case_item_reference->OnCaseLevel(case_record) )
            {
                record_notes.emplace_back(&note);
            }
        }
    }

    // write out the record name (if not the ID record)
    if( !is_id_record )
    {
        html_writer << "<p class=\"c2h_record_name\">"
                    << GetDictionaryText(dict_record)
                    << "</p>\n";
    }

    if( m_statuses == Statuses::Show && !is_id_record )
    {
        html_writer << "<p class=\"c2h_status\">" << "Record is "
                    << ( dict_record.GetRequired() ? "" : "not " )
                    << "required, ";

        if( dict_record.GetMaxRecs() == 1 )
        {
            html_writer << "singly-occurring";
        }

        else
        {
            html_writer << FormatText("multiply-occurring (%d of %d)", static_cast<int>(case_record.GetNumberOccurrences()),
                                                                       static_cast<int>(dict_record.GetMaxRecs()));
        }

        html_writer << "</p>\n";
    }

    // determine which items to display
    struct CaseItemWithOccurrence
    {
        const CaseItem* case_item;
        size_t occurrence;
    };

    std::vector<CaseItemWithOccurrence> case_item_with_occurrences;

    for( const CaseItem* const const case_item : case_record.GetCaseItems() )
    {
        const CDictItem& dict_item = case_item->GetDictItem();

        if( ( m_itemTypeDisplay == ItemTypeDisplay::Item && dict_item.GetItemType() == ItemType::Subitem ) ||
            ( m_itemTypeDisplay == ItemTypeDisplay::Subitem && dict_item.HasSubitems() ) )
        {
            continue;
        }

        for( size_t occurrence = 0; occurrence < case_item->GetTotalNumberItemSubitemOccurrences(); ++occurrence )
        {
            bool use_this_occurrence = ( m_blankValues == BlankValues::Show );

            // if hiding blank values, check if data exists for this occurrence
            if( !use_this_occurrence && case_record.HasOccurrences() )
            {
                index.SetItemSubitemOccurrence(*case_item, occurrence);

                for( index.ResetRecordOccurrence(); !use_this_occurrence && index.GetRecordOccurrence() < case_record.GetNumberOccurrences(); index.IncrementRecordOccurrence() )
                    use_this_occurrence = !case_item->IsBlank(index);
            }

            if( use_this_occurrence )
                case_item_with_occurrences.emplace_back(CaseItemWithOccurrence { case_item, occurrence });
        }
    }

    // if there are no case items to show, don't display the table
    if( case_item_with_occurrences.empty() )
    {
        html_writer << "<p class=\"c2h_status\">No occurrences</p>\n";
        return;
    }

    // write out the table
    size_t number_columns = 1 + case_item_with_occurrences.size();
    size_t number_rows = 1 + case_record.GetNumberOccurrences();

    if( m_recordOrientation == RecordOrientation::Vertical )
        std::swap(number_columns, number_rows);

    html_writer << "<table class=\"c2h_table\">\n";

    for( size_t row = 0; row < number_rows; ++row )
    {
        html_writer << "<tr>";

        for( size_t column = 0; column < number_columns; ++column )
        {
            size_t record_occurrence = row - 1;
            size_t case_item_with_occurrences_index = column - 1;

            if( m_recordOrientation == RecordOrientation::Vertical )
                std::swap(record_occurrence, case_item_with_occurrences_index);

            const CaseItemWithOccurrence* const case_item_with_occurrence =
                ( case_item_with_occurrences_index == SIZE_MAX ) ? nullptr :
                                                                   &case_item_with_occurrences[case_item_with_occurrences_index];

            const std::string row_colorizer_class = FormatText("c2h_table_r%d", static_cast<int>(record_occurrence % 2));

            if( row == 0 || column == 0 )
            {
                // write the item name
                if( ( m_recordOrientation == RecordOrientation::Horizontal && row == 0 ) ||
                    ( m_recordOrientation == RecordOrientation::Vertical && column == 0 ) )
                {
                    html_writer << "<td class=\"c2h_table_header\">";

                    if( case_item_with_occurrence == nullptr )
                    {
                        html_writer << "&nbsp;";
                    }

                    else
                    {
                        const CDictItem& dict_item = case_item_with_occurrence->case_item->GetDictItem();
                        html_writer << GetDictionaryText(dict_item);

                        // add the item occurrence if applicable
                        if( case_item_with_occurrence->case_item->GetTotalNumberItemSubitemOccurrences() > 1 )
                            html_writer << " (" << GetOccurrenceLabel(dict_item, case_item_with_occurrence->occurrence) << ")";
                    }
                }

                // write the record occurrence label
                else
                {
                    ASSERT(record_occurrence >= 0);
                    html_writer << "<td class=\"c2h_table_header ";
                    html_writer.WriteRaw(row_colorizer_class);
                    html_writer << "\">"
                                << GetOccurrenceLabel(dict_record, record_occurrence);
                }
            }

            // write the data cell
            else
            {
                index.SetRecordOccurrence(record_occurrence);
                index.SetItemSubitemOccurrence(*case_item_with_occurrence->case_item, case_item_with_occurrence->occurrence);

                auto case_item_reference_matches = [&](const CaseItemReference& case_item_reference) -> bool
                {
                    return case_item_reference.Equals(*case_item_with_occurrence->case_item, index);
                };

                // shade the cell if it is the location of the partial save
                const bool this_is_partial_field = ( mark_current_partial_save_item_reference &&
                                                     case_item_reference_matches(*data_case.GetPartialSaveCaseItemReference()) );

                html_writer << "<td class=\"";
                html_writer.WriteRaw(row_colorizer_class);

                if( this_is_partial_field )
                    html_writer << " c2h_partial_save_item_cell";

                html_writer << "\"";

                // add a partial save anchor
                if( this_is_partial_field )
                {
                    html_writer << " id =\"";
                    html_writer.WriteRaw(PartialSaveAnchor_sv);
                    html_writer << "\"";
                    mark_current_partial_save_item_reference = false;
                }

                html_writer << ">";

                // add the data
                html_writer << m_caseItemPrinter.GetText(*case_item_with_occurrence->case_item, index);


                // add any notes associated with the field
                if( !record_notes.empty() )
                {
                    // get the notes and sort them by date
                    std::vector<const Note*> field_notes;

                    for( auto record_note_itr = record_notes.cbegin(); record_note_itr != record_notes.cend(); )
                    {
                        if( case_item_reference_matches(assert_cast<const CaseItemReference&>((*record_note_itr)->GetNamedReference())) )
                        {
                            field_notes.emplace_back(*record_note_itr);
                            record_note_itr = record_notes.erase(record_note_itr);
                        }

                        else
                        {
                            ++record_note_itr;
                        }
                    }

                    if( !field_notes.empty() )
                    {
                        std::sort(field_notes.begin(), field_notes.end(),
                                  [](const Note* note1, const Note* note2) { return ( note1->GetModifiedDateTime() < note2->GetModifiedDateTime() ); });

                        // the note anchor will use the pointer address in the name
                        html_writer << "<span class=\"c2h_note_icon\">"
                                    << "<a href=\"#";
                        html_writer.WriteTagValue(FormatText(NoteAnchorFormatter, static_cast<const void*>(field_notes.front())));
                        html_writer << "\">"
                                       "<img src=\"";

                        if( EmbedResources() )
                        {
                            html_writer.WriteRaw(NoteIcon_sv);
                        }

                        else
                        {
                            static const std::string note_icon_file_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::Images),
                                                                                         "case-note.png");
                            html_writer.WriteRaw(CreateFileUrl(note_icon_file_path));
                        }

                        html_writer << "\" title=\"";

                        for( size_t i = 0; i < field_notes.size(); ++i )
                        {
                            if( i > 0 )
                                html_writer.WriteTagValue("\n");

                            html_writer.WriteTagValue(field_notes[i]->GetContent());
                        }

                        html_writer << "\" /></a></span>";
                    }
                }


                // write any additional information for binary data
                if( IsBinary(case_item_with_occurrence->case_item->GetDataType()) )
                    WriteBinaryCaseItem(html_writer, assert_cast<const BinaryCaseItem&>(*case_item_with_occurrence->case_item), index);
            }

            html_writer << "</td>";
        }

        html_writer << "</tr>\n";
    }

    html_writer << "</table>\n";
}


void CaseToHtmlConverter::WriteNotes(HtmlWriter& html_writer, const Case& data_case) const
{
    const std::vector<const Note*> sorted_notes = GetSortedNotes(data_case);

    html_writer << "<p class=\"c2h_record_name\">Notes</p>\n";

    html_writer << "<table class=\"c2h_table\">\n";

    const bool write_level_key_column = ( data_case.GetCaseMetadata().GetDictionary().GetNumLevels() > 1 );

    // write the headers
    constexpr const char* Headers[] = { "Level", "Field", "Note", "Operator ID", "Date/Time" };

    html_writer << "<tr>";

    for( size_t i = ( write_level_key_column ? 0 : 1 ); i < _countof(Headers); ++i )
        html_writer << "<td class=\"c2h_table_header\">" << Headers[i] << "</td>";

    html_writer << "</tr>\n";

    // and write each note
    for( size_t i = 0; i < sorted_notes.size(); ++i )
    {
        const Note& note = *sorted_notes[i];

        const std::string row_colorizer_class = FormatText("c2h_table_r%d", static_cast<int>(i % 2));

        auto add_column = [&](const std::string& text, const bool add_anchor = false)
        {
            html_writer << "<td class=\"";
            html_writer.WriteRaw(row_colorizer_class);
            html_writer << "\"";

            if( add_anchor )
            {
                html_writer << " id=\"";
                html_writer.WriteTagValue(FormatText(NoteAnchorFormatter, static_cast<const void*>(&note)));
                html_writer << "\"";
            }

            html_writer << ">" << text << "</td>";
        };

        html_writer << "<tr>";

        const NamedReference& named_reference = note.GetNamedReference();
        const std::string display_name = named_reference.GetName() + named_reference.GetMinimalOccurrencesText();

        if( write_level_key_column )
            add_column(named_reference.GetLevelKey());

        add_column(display_name, true);
        add_column(note.GetContent());
        add_column(note.GetOperatorId());
        add_column(FormatTimestamp(static_cast<double>(note.GetModifiedDateTime())));

        html_writer << "</tr>\n";
    }

    html_writer << "</table>\n";
}


void CaseToHtmlConverter::WriteBinaryCaseItem(HtmlWriter& html_writer, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
{
    const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);

    if( !binary_data_accessor.IsDefined() )
        return;

    const BinaryDataMetadata& binary_data_metadata = binary_data_accessor.GetBinaryDataMetadata();

    // see what kind of file this is
    const std::string suggested_filename = binary_case_item.GetSuggestedFilename(index);
    const std::string mime_type = binary_data_metadata.GetEvaluatedMimeType(MimeType::Type::Unknown);

    std::optional<std::tuple<std::string, std::string>> open_and_save_urls;

    if( AddBinaryDataOpenAndSaveUrls() )
        open_and_save_urls = CreateBinaryDataOpenAndSaveUrls(binary_case_item, index, suggested_filename);

    const std::string data_access_url = CreateBinaryDataUrl(binary_case_item, index, mime_type, suggested_filename);

    auto start_open_url = [&]()
    {
        if( open_and_save_urls.has_value() )
        {
            html_writer << "<a href=\"";
            html_writer.WriteTagValue(std::get<0>(*open_and_save_urls));
            html_writer << "\">";
        }
    };

    auto end_open_url = [&]()
    {
        if( open_and_save_urls.has_value() )
            html_writer << "</a>";
    };

    auto write_image_as_data_url_wrapped_in_open_url = [&](const char* const class_name, const std::string& image_mime_type,
                                                           const std::vector<std::byte>& data)
    {
        start_open_url();

        html_writer << "<img class=\"" << class_name << "\" src=\"";
        html_writer.WriteRaw(Encoders::ToDataUrl(data, image_mime_type));
        html_writer << "\" />";

        end_open_url();
    };

    // if this is an image, add the image...
    if( MimeType::IsImageType(mime_type) )
    {
        // ... as a URL
        if( !data_access_url.empty() )
        {
            start_open_url();

            html_writer << "<img class=\"c2h_image_thumbnail\" src=\"";
            html_writer.WriteTagValue(data_access_url);
            html_writer << "\" />";

            end_open_url();
        }

        // ...or as a data URL
        else
        {
            const BinaryData* const binary_data = binary_case_item.GetBinaryData_noexcept(index);

            if( binary_data != nullptr )
                write_image_as_data_url_wrapped_in_open_url("c2h_image_thumbnail", mime_type, binary_data->GetContent());
        }
    }


    // if this is audio, add an audio control to play it
    else if( !data_access_url.empty() && MimeType::IsAudioType(mime_type) )
    {
        html_writer.WriteNewline();
        html_writer << "<audio controls controlsList=\"nodownload\" preload=\"none\">"
                       "<source src=\"";
        html_writer.WriteTagValue(data_access_url);
        html_writer << "\" />"
                       "</audio>";
    }


    // if not an image or audio, but if the extension is known, get an icon to represent this file type
    else
    {
        const std::optional<std::string> extension = binary_data_metadata.GetEvaluatedExtension();

        if( extension.has_value() )
        {
            const std::shared_ptr<const std::vector<std::byte>> png_data = SystemIcon::GetPngForExtension(*extension);

            if( png_data != nullptr )
                write_image_as_data_url_wrapped_in_open_url("c2h_icon", MimeType::Type::ImagePng, *png_data);
        }
    }


    // add links to open and save the data
    if( open_and_save_urls.has_value() )
    {
        html_writer.WriteNewline();
        html_writer << "<a href=\"";
        html_writer.WriteTagValue(std::get<0>(*open_and_save_urls));
        html_writer << "\">Open</a> - "
                       "<a href=\"";
        html_writer.WriteTagValue(std::get<1>(*open_and_save_urls));
        html_writer << "\">Save</a>";
    }
}
