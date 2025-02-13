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
    constexpr const TCHAR* Newline = L"<br />";

    constexpr const TCHAR* Divider = L"<span class=\"c2h_divider\">&nbsp; ► &nbsp;</span>";

    constexpr const TCHAR* PartialSaveAnchor = L"PartialSave";
    constexpr const TCHAR* NoteAnchorFormatter = L"Note%p";

    // the note image is stored in the DataViewer project
    constexpr const TCHAR* NoteIcon =
        L"data:image/png;base64,"
        L"iVBORw0KGgoAAAANSUhEUgAAAA8AAAAMCAYAAAC9QufkAAAAAXNSR0IArs4c6QAAAARnQU1BAACx"
        L"jwv8YQUAAAAJcEhZcwAADsQAAA7EAZUrDhsAAABCSURBVChTtc3BCQAwCENR958qm6VYKJRCMII9"
        L"fMjBhwGAEdFuu4NJvz84t6rEThLnVpXYSeLcbzc8N/OfnWZxNwBcEpWepmQw/5cAAAAASUVORK5C"
        L"YII=";
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
        m_caseItemPrinter(CaseItemPrinter::Format::CaseTree),
        m_createBinaryDataUrls(false)
{
}


std::wstring CaseToHtmlConverter::ToHtml(const Case& data_case, const CaseSpecificSettings* case_specific_settings/* = nullptr*/) const
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


    // write the header
    HtmlStringWriter html_writer;
    html_writer.WriteDefaultHeader(data_case.GetCaseLabelOrKey(), Html::CSS::CaseView);

    // write the body
    html_writer << L"<body>";

    // write the key and case label
    html_writer << L"<p class=\"c2h_case_key\">" << data_case.GetKey();

    if( !data_case.GetCaseLabel().IsEmpty() && data_case.GetKey().Compare(data_case.GetCaseLabel()) != 0 )
        html_writer << Divider << data_case.GetCaseLabel();

    html_writer << L"</p>";

    // write the case note
    if( m_notes == Notes::Show )
    {
        const CString& case_note = data_case.GetCaseNote();

        if( !case_note.IsEmpty() )
            html_writer << L"<p class=\"c2h_case_note\">" << case_note << L"</p>";
    }

    // write the statuses
    if( m_statuses != Statuses::Hide )
        WriteStatuses(html_writer, data_case);

    // potentially write the case construction errors
    if( m_caseConstructionErrors == CaseConstructionErrors::Show &&
        case_specific_settings != nullptr && case_specific_settings->case_construction_errors != nullptr )
    {
        WriteCaseConstructionErrors(html_writer, *case_specific_settings->case_construction_errors);
    }

    // write all levels
    for( const CaseLevel* case_level : data_case.GetAllCaseLevels() )
        WriteCaseLevel(html_writer, data_case, *case_level, case_specific_settings);

    // write the notes table
    if( m_notes == Notes::Show && !data_case.GetNotes().empty() )
        WriteNotes(html_writer, data_case);

    html_writer << L"</body>"
                << L"</html>";

    return html_writer.str();
}


template<typename T>
CString CaseToHtmlConverter::GetDictionaryText(const T& t) const
{
    return ( m_nameDisplay == NameDisplay::Label ) ? t.GetLabel() :
           ( m_nameDisplay == NameDisplay::Name )  ? t.GetName() :
                                                     FormatText(L"%s: %s", t.GetName().GetString(), t.GetLabel().GetString());
}


template<typename T>
CString CaseToHtmlConverter::GetOccurrenceLabel(const T& t, size_t occurrence) const
{
    CString occurrence_label;

    if( m_occurrenceDisplay == OccurrenceDisplay::Label )
        occurrence_label = t.GetOccurrenceLabels().GetLabel(occurrence);

    if( occurrence_label.IsEmpty() )
        occurrence_label = IntToString(occurrence + 1);

    return occurrence_label;
}


void CaseToHtmlConverter::WriteStatuses(HtmlWriter& html_writer, const Case& data_case) const
{
    ASSERT(m_statuses != Statuses::Hide);

    if( m_statuses == Statuses::Show && !data_case.GetUuid().IsEmpty() )
        html_writer << L"<p class=\"c2h_status\">" << L"UUID: " << data_case.GetUuid() << L"</p>";

    if( m_statuses == Statuses::Show || data_case.GetDeleted() )
        html_writer << L"<p class=\"c2h_status\">Case is " << ( data_case.GetDeleted() ? L"" : L"not " ) << L"deleted</p>";

    if( m_statuses == Statuses::Show || data_case.GetVerified() )
        html_writer << L"<p class=\"c2h_status\">Case is " << ( data_case.GetVerified() ? L"" : L"not " ) << L"verified</p>";

    if( m_statuses == Statuses::Show || data_case.IsPartial() )
    {
        html_writer << L"<p class=\"c2h_status\">";

        if( data_case.IsPartial() )
        {
            html_writer
                << L"Case is partially saved in <b>"
                << ( ( data_case.GetPartialSaveMode() == PartialSaveMode::Add )    ? L"add" :
                     ( data_case.GetPartialSaveMode() == PartialSaveMode::Modify ) ? L"modify" :
                                                                                     L"verify" )
                << L"</b> mode";

            if( data_case.GetPartialSaveCaseItemReference() != nullptr )
            {
                const CaseItemReference& partial_save_case_item_reference = *data_case.GetPartialSaveCaseItemReference();

                html_writer
                    << L" on field "
                    << L"<a class=\"c2h_partial_save_item_link\" href=\"#" << PartialSaveAnchor << L"\">"
                    << partial_save_case_item_reference.GetName()
                    << partial_save_case_item_reference.GetItemIndexHelper().GetMinimalOccurrencesText(partial_save_case_item_reference)
                    << L"</a>";

                if( !partial_save_case_item_reference.GetLevelKey().IsEmpty() )
                    html_writer << L" on level <b>" << partial_save_case_item_reference.GetLevelKey() << L"</b>";
            }
        }

        else
        {
            html_writer << L"Case is not partially saved";
        }

        html_writer << L"</p>";
    }
}


void CaseToHtmlConverter::WriteCaseConstructionErrors(HtmlWriter& html_writer, const std::vector<std::wstring>& case_construction_errors) const
{
    if( case_construction_errors.empty() )
        return;

    html_writer << L"<p class=\"c2h_errors\">";

    for( size_t i = 0; i < case_construction_errors.size(); ++i )
    {
        if( i > 0 )
            html_writer << Newline;

        html_writer << L"⚠ " << case_construction_errors[i];
    }

    html_writer << L"</p>";
}


void CaseToHtmlConverter::WriteCaseLevel(HtmlWriter& html_writer, const Case& data_case, const CaseLevel& case_level,
                                         const CaseSpecificSettings* case_specific_settings) const
{
    // write out the level name and, if not on the root level, the level key
    html_writer << L"<p class=\"c2h_level_name\">" << GetDictionaryText(case_level.GetCaseLevelMetadata().GetDictLevel());

    if( !case_level.GetLevelKey().IsEmpty() )
        html_writer << Divider << case_level.GetLevelKey();

    html_writer << L"</p>";

    // write the IDs and then each record
    WriteCaseRecord(html_writer, data_case, case_level.GetIdCaseRecord(), true, case_specific_settings);

    for( size_t record_number = 0; record_number < case_level.GetNumberCaseRecords(); ++record_number )
    {
        const CaseRecord& case_record = case_level.GetCaseRecord(record_number);
        WriteCaseRecord(html_writer, data_case, case_record, false, case_specific_settings);
    }
}


void CaseToHtmlConverter::WriteCaseRecord(HtmlWriter& html_writer, const Case& data_case, const CaseRecord& case_record,
                                          bool is_id_record, const CaseSpecificSettings* case_specific_settings) const
{
    const CDictRecord& dictionary_record = case_record.GetCaseRecordMetadata().GetDictionaryRecord();
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
            const CaseItemReference* case_item_reference = dynamic_cast<const CaseItemReference*>(&note.GetNamedReference());

            if( case_item_reference != nullptr &&
                case_item_reference->GetCaseItem().GetDictionaryItem().GetRecord() == &dictionary_record &&
                case_item_reference->OnCaseLevel(case_record) )
            {
                record_notes.emplace_back(&note);
            }
        }
    }

    // write out the record name (if not the ID record)
    if( !is_id_record )
        html_writer << L"<p class=\"c2h_record_name\">" << GetDictionaryText(dictionary_record) << L"</p>";

    if( m_statuses == Statuses::Show && !is_id_record )
    {
        html_writer << L"<p class=\"c2h_status\">" << L"Record is " <<
            ( dictionary_record.GetRequired() ? L"" : L"not " ) << L"required, ";

        if( dictionary_record.GetMaxRecs() == 1 )
        {
            html_writer << L"singly-occurring";
        }

        else
        {
            html_writer << FormatText(L"multiply-occurring (%d of %d)", (int)case_record.GetNumberOccurrences(), (int)dictionary_record.GetMaxRecs());
        }

        html_writer << L"</p>";
    }

    // determine which items to display
    struct CaseItemWithOccurrence
    {
        const CaseItem* case_item;
        size_t occurrence;
    };

    std::vector<CaseItemWithOccurrence> case_item_with_occurrences;

    for( const CaseItem* case_item : case_record.GetCaseItems() )
    {
        const CDictItem& dictionary_item = case_item->GetDictionaryItem();

        if( ( m_itemTypeDisplay == ItemTypeDisplay::Item && dictionary_item.GetItemType() == ItemType::Subitem ) ||
            ( m_itemTypeDisplay == ItemTypeDisplay::Subitem && dictionary_item.HasSubitems() ) )
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
                case_item_with_occurrences.push_back(CaseItemWithOccurrence { case_item, occurrence });
        }
    }

    // if there are no case items to show, don't display the table
    if( case_item_with_occurrences.empty() )
    {
        html_writer << L"<p class=\"c2h_status\">No occurrences</p>";
        return;
    }

    // write out the table
    size_t number_columns = 1 + case_item_with_occurrences.size();
    size_t number_rows = 1 + case_record.GetNumberOccurrences();

    if( m_recordOrientation == RecordOrientation::Vertical )
        std::swap(number_columns, number_rows);

    html_writer << L"<table class=\"c2h_table\">";

    for( size_t row = 0; row < number_rows; ++row )
    {
        html_writer << L"<tr>";

        for( size_t column = 0; column < number_columns; ++column )
        {
            size_t record_occurrence = row - 1;
            size_t case_item_with_occurrences_index = column - 1;

            if( m_recordOrientation == RecordOrientation::Vertical )
                std::swap(record_occurrence, case_item_with_occurrences_index);

            const CaseItemWithOccurrence* case_item_with_occurrence =
                ( case_item_with_occurrences_index == SIZE_MAX ) ? nullptr :
                                                                   &case_item_with_occurrences[case_item_with_occurrences_index];

            const std::wstring row_colorizer_class = FormatTextCS2WS(L"c2h_table_r%d", record_occurrence % 2);

            if( row == 0 || column == 0 )
            {
                // write the item name
                if( ( m_recordOrientation == RecordOrientation::Horizontal && row == 0 ) ||
                    ( m_recordOrientation == RecordOrientation::Vertical && column == 0 ) )
                {
                    html_writer << L"<td class=\"c2h_table_header\">";

                    if( case_item_with_occurrence == nullptr )
                    {
                        html_writer << L"&nbsp;";
                    }

                    else
                    {
                        const CDictItem& dictionary_item = case_item_with_occurrence->case_item->GetDictionaryItem();
                        html_writer << GetDictionaryText(dictionary_item);

                        // add the item occurrence if applicable
                        if( case_item_with_occurrence->case_item->GetTotalNumberItemSubitemOccurrences() > 1 )
                            html_writer << L" (" << GetOccurrenceLabel(dictionary_item, case_item_with_occurrence->occurrence) << L")";
                    }
                }

                // write the record occurrence label
                else
                {
                    ASSERT(record_occurrence >= 0);
                    html_writer << L"<td class=\"c2h_table_header " << row_colorizer_class << L"\">";
                    html_writer << GetOccurrenceLabel(dictionary_record, record_occurrence);
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
                const bool this_is_partial_field = ( mark_current_partial_save_item_reference && case_item_reference_matches(*data_case.GetPartialSaveCaseItemReference()) );

                html_writer << L"<td class=\"" << row_colorizer_class;

                if( this_is_partial_field )
                    html_writer << L" c2h_partial_save_item_cell";

                html_writer << L"\"";

                // add a partial save anchor
                if( this_is_partial_field )
                {
                    html_writer << L" id =\"" << PartialSaveAnchor << L"\"";
                    mark_current_partial_save_item_reference = false;
                }

                html_writer << L">";

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
                        html_writer << L"<span class=\"c2h_note_icon\">"
                                    << L"<a href=\"#" << FormatText(NoteAnchorFormatter, static_cast<const void*>(field_notes.front())) << L"\">"
                                    << L"<img src=\"" << NoteIcon << L"\" title=\"";

                        for( size_t i = 0; i < field_notes.size(); ++i )
                        {
                            if( i > 0 )
                                html_writer << Encoders::ToHtmlTagValue(_T("\n")).c_str();

                            html_writer << Encoders::ToHtmlTagValue(field_notes[i]->GetContent()).c_str();
                        }

                        html_writer << L"\" /></a></span>";
                    }
                }


                // write any additional information for binary data
                if( case_item_with_occurrence->case_item->IsTypeBinary() )
                {
                    WriteBinaryCaseItem(html_writer, assert_cast<const BinaryCaseItem&>(*case_item_with_occurrence->case_item),
                        index, case_specific_settings);
                }
            }

            html_writer << L"</td>";
        }

        html_writer << L"</tr>";
    }

    html_writer << L"</table>";
}


void CaseToHtmlConverter::WriteNotes(HtmlWriter& html_writer, const Case& data_case) const
{
    const std::vector<const Note*> sorted_notes = GetSortedNotes(data_case);

    html_writer << L"<p class=\"c2h_record_name\">Notes</p>";

    html_writer << L"<table class=\"c2h_table\">";

    const bool write_level_key_column = ( data_case.GetCaseMetadata().GetDictionary().GetNumLevels() > 1 );

    // write the headers
    constexpr const TCHAR* Headers[] = { L"Level", L"Field", L"Note", L"Operator ID", L"Date/Time" };

    html_writer << L"<tr>";

    for( size_t i = ( write_level_key_column ? 0 : 1 ); i < _countof(Headers); ++i )
        html_writer << L"<td class=\"c2h_table_header\">" << Headers[i] << L"</td>";

    html_writer << L"</tr>";

    // and write each note
    for( size_t i = 0; i < sorted_notes.size(); ++i )
    {
        const Note& note = *sorted_notes[i];

        const std::wstring row_colorizer_class = FormatTextCS2WS(L"c2h_table_r%d", i % 2);

        auto add_column = [&](const CString& text, bool add_anchor = false)
        {
            html_writer << L"<td class=\"" << row_colorizer_class << L"\"";

            if( add_anchor )
                html_writer << L" id=\"" << FormatText(NoteAnchorFormatter, static_cast<const void*>(&note)) << L"\"";

            html_writer << L">" << text << L"</td>";
        };

        html_writer << L"<tr>";

        const NamedReference& named_reference = note.GetNamedReference();
        CString display_name = named_reference.GetName();
        display_name.Append(named_reference.GetMinimalOccurrencesText());

        if( write_level_key_column )
            add_column(named_reference.GetLevelKey());

        add_column(display_name, true);
        add_column(note.GetContent());
        add_column(note.GetOperatorId());
        add_column(UTF8Convert::UTF8ToWide<CString>(FormatTimestamp((double)note.GetModifiedDateTime(), "%c").c_str()));

        html_writer << L"</tr>";
    }

    html_writer << L"</table>";
}


void CaseToHtmlConverter::WriteBinaryCaseItem(HtmlWriter& html_writer, const BinaryCaseItem& binary_case_item,
                                              const CaseItemIndex& index, const CaseSpecificSettings* case_specific_settings) const
{
    const BinaryDataMetadata* binary_data_metadata = binary_case_item.GetBinaryDataMetadata_noexcept(index);

    if( binary_data_metadata == nullptr )
        return;

    // see what kind of file this is
    const std::optional<std::wstring> extension = binary_data_metadata->GetEvaluatedExtension();
    const std::optional<std::wstring> mime_type = binary_data_metadata->GetEvaluatedMimeType();

    std::optional<std::tuple<std::wstring, std::wstring>> open_and_save_urls;

    if( extension.has_value() && m_createBinaryDataUrls )
    {
        const std::wstring url_base = CreateBinaryDataAccessUrlBase(*binary_data_metadata, binary_case_item, index);
        open_and_save_urls.emplace(url_base + L"/open",
                                   url_base + L"/save");
    }

    auto start_open_url = [&]()
    {
        if( open_and_save_urls.has_value() )
            html_writer << L"<a href=\"" << std::get<0>(*open_and_save_urls).c_str() << L"\">";
    };

    auto end_open_url = [&]()
    {
        if( open_and_save_urls.has_value() )
            html_writer << L"</a>";
    };

    auto write_image_as_data_url_wrapped_in_open_url = [&](const TCHAR* class_name, const std::wstring& mime_type, const std::vector<std::byte>& data)
    {
        start_open_url();

        html_writer << L"<img class=\"" << class_name << L"\" src=\""
                    << Encoders::ToDataUrl(data, mime_type).c_str()
                    << L"\" />";

        end_open_url();
    };


    // if the MIME type is known, we can create URLs to access the data
    std::optional<std::wstring> data_access_url;

    if( mime_type.has_value() && case_specific_settings != nullptr && case_specific_settings->base_url_for_binary_retrieval != nullptr )
    {
        data_access_url = FormatTextCS2WS(_T("%s%s/%s/"),
                                          case_specific_settings->base_url_for_binary_retrieval,
                                          Encoders::ToUriComponent(index.GetSerializableText(binary_case_item)).c_str(),
                                          Encoders::ToUriComponent(*mime_type).c_str());
    }


    // if this is an image, add the image...
    if( mime_type.has_value() && MimeType::IsImageType(*mime_type) )
    {
        // ... as a URL
        if( data_access_url.has_value() )
        {
            start_open_url();

            html_writer << L"<img class=\"c2h_image_thumbnail\" "
                        << L"src=\"" << data_access_url->c_str() << L"\" />";

            end_open_url();
        }

        // ...or as a data URL
        else
        {
            const BinaryData* binary_data = binary_case_item.GetBinaryData_noexcept(index);

            if( binary_data != nullptr )
                write_image_as_data_url_wrapped_in_open_url(L"c2h_image_thumbnail", mime_type->c_str(), binary_data->GetContent());
        }
    }


    // if this is audio, add an audio control to play it
    else if( data_access_url.has_value() && mime_type.has_value() && MimeType::IsAudioType(*mime_type) )
    {
        html_writer << Newline
                    << L"<audio controls controlsList=\"nodownload\" preload=\"none\">"
                    << L"<source src=\"" << data_access_url->c_str() << L"\" />"
                    << L"</audio>";
    }


    // if not an image or audio, but if the extension is known, get an icon to represent this file type
    else if( extension.has_value() )
    {
        const std::shared_ptr<const std::vector<std::byte>> png_data = SystemIcon::GetPngForExtension(*extension);

        if( png_data != nullptr )
            write_image_as_data_url_wrapped_in_open_url(L"c2h_icon", MimeType::Type::ImagePng, *png_data);
    }


    // add links to open and save the data
    if( open_and_save_urls.has_value() )
    {
        html_writer << Newline
                    << L"<a href=\"" << std::get<0>(*open_and_save_urls).c_str() << L"\">Open</a> - "
                    << L"<a href=\"" << std::get<1>(*open_and_save_urls).c_str() << L"\">Save</a>";
    }
}


std::wstring CaseToHtmlConverter::CreateBinaryDataAccessUrlBase(const BinaryDataMetadata& binary_data_metadata,
                                                                const BinaryCaseItem& binary_case_item, const CaseItemIndex& index) const
{
    std::optional<std::wstring> suggested_filename = binary_data_metadata.GetFilename();

    // if there is no filename, use the item name with whatever extension the file was saved as
    if( !suggested_filename.has_value() )
    {
        const std::optional<std::wstring> extension = binary_data_metadata.GetEvaluatedExtension();
        ASSERT(extension.has_value());

        suggested_filename = binary_case_item.GetDictionaryItem().GetName();
        suggested_filename->append(index.GetMinimalOccurrencesText(binary_case_item));

        if( !extension->empty() )
            suggested_filename = PortableFunctions::PathAppendFileExtension(*suggested_filename, *extension);
    }

    return FormatTextCS2WS(_T("https://binary-data-access/%s/%s/"),
                           Encoders::ToUriComponent(index.GetSerializableText(binary_case_item)).c_str(),
                           Encoders::ToUriComponent(*suggested_filename).c_str());
}
