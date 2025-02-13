#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseItemPrinter.h>

class BinaryCaseItem;
class BinaryDataMetadata;
class HtmlWriter;


class ZCASEO_API CaseToHtmlConverter
{
public:
    struct CaseSpecificSettings
    {
        const std::vector<std::wstring>* case_construction_errors = nullptr;
        const TCHAR* base_url_for_binary_retrieval = nullptr;
    };

    enum class Statuses               { Show, ShowIfNotDefault, Hide };
    enum class CaseConstructionErrors { Show, Hide };
    enum class NameDisplay            { Label, Name, NameLabel };
    enum class RecordOrientation      { Horizontal, Vertical };
    enum class OccurrenceDisplay      { Number, Label };
    enum class ItemTypeDisplay        { Item, Subitem, ItemSubitem };
    enum class BlankValues            { Show, Hide };
    enum class Notes                  { Show, Hide };

    CaseToHtmlConverter();

    void SetStatuses(Statuses statuses)                                             { m_statuses = statuses; }
    void SetCaseConstructionErrors(CaseConstructionErrors case_construction_errors) { m_caseConstructionErrors = case_construction_errors; }
    void SetNameDisplay(NameDisplay name_display)                                   { m_nameDisplay = name_display; }
    void SetRecordOrientation(RecordOrientation record_orientation)                 { m_recordOrientation = record_orientation; }
    void SetOccurrenceDisplay(OccurrenceDisplay occurrence_display)                 { m_occurrenceDisplay = occurrence_display; }
    void SetItemTypeDisplay(ItemTypeDisplay item_type_display)                      { m_itemTypeDisplay = item_type_display; }
    void SetBlankValues(BlankValues blank_values)                                   { m_blankValues = blank_values; }
    void SetNotes(Notes notes)                                                      { m_notes = notes; }
    void SetCaseItemPrinterFormat(CaseItemPrinter::Format format)                   { m_caseItemPrinter.SetFormat(format); }
    void SetLanguage(std::wstring language_name)                                    { m_languageName = std::move(language_name); }
    void SetCreateBinaryDataUrls(bool create_binary_data_urls)                      { m_createBinaryDataUrls = create_binary_data_urls; }

    std::wstring ToHtml(const Case& data_case, const CaseSpecificSettings* case_specific_settings = nullptr) const;

private:
    void WriteStatuses(HtmlWriter& html_writer, const Case& data_case) const;

    void WriteCaseConstructionErrors(HtmlWriter& html_writer, const std::vector<std::wstring>& case_construction_errors) const;

    void WriteCaseLevel(HtmlWriter& html_writer, const Case& data_case, const CaseLevel& case_level,
                        const CaseSpecificSettings* case_specific_settings) const;

    void WriteCaseRecord(HtmlWriter& html_writer, const Case& data_case, const CaseRecord& case_record,
                         bool is_id_record, const CaseSpecificSettings* case_specific_settings) const;

    void WriteNotes(HtmlWriter& html_writer, const Case& data_case) const;

    void WriteBinaryCaseItem(HtmlWriter& html_writer, const BinaryCaseItem& binary_case_item,
                             const CaseItemIndex& index, const CaseSpecificSettings* case_specific_settings) const;

    std::wstring CreateBinaryDataAccessUrlBase(const BinaryDataMetadata& binary_data_metadata,
                                               const BinaryCaseItem& binary_case_item, const CaseItemIndex& index) const;

    template<typename T>
    CString GetDictionaryText(const T& t) const;

    template<typename T>
    CString GetOccurrenceLabel(const T& t, size_t occurrence) const;

private:
    Statuses m_statuses;
    CaseConstructionErrors m_caseConstructionErrors;
    NameDisplay m_nameDisplay;
    RecordOrientation m_recordOrientation;
    OccurrenceDisplay m_occurrenceDisplay;
    ItemTypeDisplay m_itemTypeDisplay;
    BlankValues m_blankValues;
    Notes m_notes;
    CaseItemPrinter m_caseItemPrinter;
    std::wstring m_languageName;
    bool m_createBinaryDataUrls;
};
