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
    enum class Statuses               { Show, ShowIfNotDefault, Hide };
    enum class CaseConstructionErrors { Show, Hide };
    enum class NameDisplay            { Label, Name, NameLabel };
    enum class RecordOrientation      { Horizontal, Vertical };
    enum class OccurrenceDisplay      { Number, Label };
    enum class ItemTypeDisplay        { Item, Subitem, ItemSubitem };
    enum class BlankValues            { Show, Hide };
    enum class Notes                  { Show, Hide };

    CaseToHtmlConverter();
    virtual ~CaseToHtmlConverter() { }

    void SetStatuses(Statuses statuses)                                             { m_statuses = statuses; }
    void SetNameDisplay(NameDisplay name_display)                                   { m_nameDisplay = name_display; }
    void SetRecordOrientation(RecordOrientation record_orientation)                 { m_recordOrientation = record_orientation; }
    void SetOccurrenceDisplay(OccurrenceDisplay occurrence_display)                 { m_occurrenceDisplay = occurrence_display; }
    void SetItemTypeDisplay(ItemTypeDisplay item_type_display)                      { m_itemTypeDisplay = item_type_display; }
    void SetBlankValues(BlankValues blank_values)                                   { m_blankValues = blank_values; }
    void SetNotes(Notes notes)                                                      { m_notes = notes; }
    void SetCaseItemPrinterFormat(CaseItemPrinter::Format format)                   { m_caseItemPrinter.SetFormat(format); }
    void SetLanguage(std::string language_name)                                     { m_languageName = std::move(language_name); }

    std::string ToHtml(const Case& data_case);

protected:
    virtual const std::vector<std::string>* GetCaseConstructionErrors() const;

    // If EmbedResources returns false, the CSS and note image will be added as URLs created using CreateFileUrl.
    virtual bool EmbedResources() const;
    virtual std::string CreateFileUrl(const std::string& file_path);

    virtual bool AddBinaryDataOpenAndSaveUrls() const;
    virtual std::tuple<std::string, std::string> CreateBinaryDataOpenAndSaveUrls(const BinaryCaseItem& binary_case_item, const CaseItemIndex& index,
                                                                                 const std::string& suggested_filename);

    // If CreateBinaryDataUrl returns a blank string, the binary data will be written as a data URL.
    virtual std::string CreateBinaryDataUrl(const BinaryCaseItem& binary_case_item, const CaseItemIndex& index,
                                            const std::string& mime_type, const std::string& suggested_filename);

private:
    void WriteStatuses(HtmlWriter& html_writer, const Case& data_case) const;

    static std::string GetCaseConstructionErrorsHtml(const std::vector<std::string>& case_construction_errors);

    void WriteCaseLevel(HtmlWriter& html_writer, const Case& data_case, const CaseLevel& case_level);

    void WriteCaseRecord(HtmlWriter& html_writer, const Case& data_case, const CaseRecord& case_record, bool is_id_record);

    void WriteNotes(HtmlWriter& html_writer, const Case& data_case) const;

    void WriteBinaryCaseItem(HtmlWriter& html_writer, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index);

    template<typename T>
    std::string GetDictionaryText(const T& t) const;

    template<typename T>
    std::string GetOccurrenceLabel(const T& t, size_t occurrence) const;

protected:
    Statuses m_statuses;
    CaseConstructionErrors m_caseConstructionErrors;
    NameDisplay m_nameDisplay;
    RecordOrientation m_recordOrientation;
    OccurrenceDisplay m_occurrenceDisplay;
    ItemTypeDisplay m_itemTypeDisplay;
    BlankValues m_blankValues;
    Notes m_notes;
    CaseItemPrinter m_caseItemPrinter;
    std::string m_languageName;
};
