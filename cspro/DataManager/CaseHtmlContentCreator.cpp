#include "StdAfx.h"
#include "CaseHtmlContentCreator.h"
#include "CaseHtmlContentCreatorSettings.h"
#include "ViewOptionsHelper.h"
#include <zUtilO/TemporaryFile.h>
#include <zCaseO/BinaryCaseItem.h>


template<> constexpr CaseToHtmlConverter::NameDisplay FirstInEnum<CaseToHtmlConverter::NameDisplay>() { return CaseToHtmlConverter::NameDisplay::Label; }
template<> constexpr CaseToHtmlConverter::NameDisplay LastInEnum<CaseToHtmlConverter::NameDisplay>()  { return CaseToHtmlConverter::NameDisplay::NameLabel; }

template<> constexpr CaseItemPrinter::Format FirstInEnum<CaseItemPrinter::Format>() { return CaseItemPrinter::Format::Label; }
template<> constexpr CaseItemPrinter::Format LastInEnum<CaseItemPrinter::Format>()  { return CaseItemPrinter::Format::CaseTree; }


CREATE_JSON_KEY(blankValues)
CREATE_JSON_KEY(dictionaryNames)
CREATE_JSON_KEY(errors)
CREATE_JSON_KEY(recordOrientation)
CREATE_JSON_KEY(statuses)
CREATE_JSON_KEY(valueDisplay)

CREATE_ENUM_JSON_SERIALIZER(CaseToHtmlConverter::Statuses,
    { CaseToHtmlConverter::Statuses::Show,             "show" },
    { CaseToHtmlConverter::Statuses::ShowIfNotDefault, "showIfNotDefault" },
    { CaseToHtmlConverter::Statuses::Hide,             "hide" })

CREATE_ENUM_JSON_SERIALIZER(CaseToHtmlConverter::CaseConstructionErrors,
    { CaseToHtmlConverter::CaseConstructionErrors::Show, "show" },
    { CaseToHtmlConverter::CaseConstructionErrors::Hide, "hide" })

CREATE_ENUM_JSON_SERIALIZER(CaseToHtmlConverter::NameDisplay,
    { CaseToHtmlConverter::NameDisplay::Label,     "label" },
    { CaseToHtmlConverter::NameDisplay::Name,      "name" },
    { CaseToHtmlConverter::NameDisplay::NameLabel, "labelAndName" })

CREATE_ENUM_JSON_SERIALIZER(CaseToHtmlConverter::RecordOrientation ,
    { CaseToHtmlConverter::RecordOrientation::Horizontal, "horizontal" },
    { CaseToHtmlConverter::RecordOrientation::Vertical,   "vertical" })

CREATE_ENUM_JSON_SERIALIZER(CaseToHtmlConverter::OccurrenceDisplay ,
    { CaseToHtmlConverter::OccurrenceDisplay::Number, "number" },
    { CaseToHtmlConverter::OccurrenceDisplay::Label,  "label" })

CREATE_ENUM_JSON_SERIALIZER(CaseToHtmlConverter::ItemTypeDisplay ,
    { CaseToHtmlConverter::ItemTypeDisplay::Item,        "item" },
    { CaseToHtmlConverter::ItemTypeDisplay::Subitem,     "subitem" },
    { CaseToHtmlConverter::ItemTypeDisplay::ItemSubitem, "itemAndSubitem" })

CREATE_ENUM_JSON_SERIALIZER(CaseToHtmlConverter::BlankValues,
    { CaseToHtmlConverter::BlankValues::Show, "show" },
    { CaseToHtmlConverter::BlankValues::Hide, "hide" })

CREATE_ENUM_JSON_SERIALIZER(CaseItemPrinter::Format,
    { CaseItemPrinter::Format::Label,     "label" },
    { CaseItemPrinter::Format::Code,      "code" },
    { CaseItemPrinter::Format::LabelCode, "labelAndCode" },
    { CaseItemPrinter::Format::CodeLabel, "codeAndLabel" },
    { CaseItemPrinter::Format::CaseTree,  "caseTree" })


// --------------------------------------------------------------------------
// CaseHtmlContentCreatorSettings
// --------------------------------------------------------------------------

CaseHtmlContentCreatorSettings::CaseHtmlContentCreatorSettings()
    :   m_creator(nullptr),
        m_caseItemPrinterFormat(m_caseItemPrinter.GetFormat())
{
}


const std::vector<std::string>* CaseHtmlContentCreatorSettings::GetCaseConstructionErrors() const
{
    ASSERT(m_creator != nullptr && m_creator->m_dataCase != nullptr);
    return assert_cast<const StringVectorCaseConstructionReporter*>(m_creator->m_dataCase->GetCaseConstructionReporter())->GetErrors();
}


bool CaseHtmlContentCreatorSettings::EmbedResources() const
{
    ASSERT(m_creator != nullptr);
    return m_creator->m_embedResources;
}


std::string CaseHtmlContentCreatorSettings::CreateFileUrl(const std::string& file_path)
{
    SharedHtmlLocalFileServer& file_server = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetSharedHtmlLocalFileServer();
    return file_server.CreateFileUrl(file_path);
}


bool CaseHtmlContentCreatorSettings::AddBinaryDataOpenAndSaveUrls() const
{
    return !EmbedResources();
}


std::tuple<std::string, std::string> CaseHtmlContentCreatorSettings::CreateBinaryDataOpenAndSaveUrls(const BinaryCaseItem& binary_case_item, const CaseItemIndex& index,
                                                                                                     const std::string& suggested_filename)
{
    ASSERT(AddBinaryDataOpenAndSaveUrls());

    const std::string base_url = SO::Concatenate("javascript:window.chrome.webview.postMessage({"
                                                 "\"key\":", Encoders::ToJsonString(index.GetSerializableText(binary_case_item)),
                                                 ",\"filename\":", Encoders::ToJsonString(suggested_filename),
                                                 ",\"action\":\"");

    return { base_url + "open\"});",
             base_url + "save\"});" };
}


std::string CaseHtmlContentCreatorSettings::CreateBinaryDataUrl(const BinaryCaseItem& binary_case_item, const CaseItemIndex& index,
                                                                const std::string& mime_type, const std::string& suggested_filename)
{
    ASSERT(m_creator != nullptr);

    if( m_creator->m_embedResources )
        return std::string();

    return m_creator->CreateBinaryDataHandler(index.GetSerializableText(binary_case_item), mime_type, suggested_filename);
}


bool CaseHtmlContentCreatorSettings::CaseHtmlContentCreatorSettings::ProcessViewOptionsMenu(const std::variant<UINT, CCmdUI*>& data)
{
    switch( ViewOptionsHelper::GetCommandId(data) )
    {
        case ID_VIEW_OPTIONS_HTML_DICTIONARY_LABELS:             return ViewOptionsHelper::HandleCheck(data, m_nameDisplay, NameDisplay::Label);
        case ID_VIEW_OPTIONS_HTML_DICTIONARY_NAMES:              return ViewOptionsHelper::HandleCheck(data, m_nameDisplay, NameDisplay::Name);
        case ID_VIEW_OPTIONS_HTML_DICTIONARY_NAMES_AND_LABELS:   return ViewOptionsHelper::HandleCheck(data, m_nameDisplay, NameDisplay::NameLabel);

        case ID_VIEW_OPTIONS_HTML_DICTIONARY_ITEMS_AND_SUBITEMS: return ViewOptionsHelper::HandleCheck(data, m_itemTypeDisplay, ItemTypeDisplay::ItemSubitem);
        case ID_VIEW_OPTIONS_HTML_DICTIONARY_ITEMS:              return ViewOptionsHelper::HandleCheck(data, m_itemTypeDisplay, ItemTypeDisplay::Item);
        case ID_VIEW_OPTIONS_HTML_DICTIONARY_SUBITEMS:           return ViewOptionsHelper::HandleCheck(data, m_itemTypeDisplay, ItemTypeDisplay::Subitem);

        case ID_VIEW_OPTIONS_HTML_RECORD_HORIZONTAL:             return ViewOptionsHelper::HandleCheck(data, m_recordOrientation, RecordOrientation::Horizontal);
        case ID_VIEW_OPTIONS_HTML_RECORD_VERTICAL:               return ViewOptionsHelper::HandleCheck(data, m_recordOrientation, RecordOrientation::Vertical);

        case ID_VIEW_OPTIONS_HTML_RECORD_OCCURRENCE_LABELS:      return ViewOptionsHelper::HandleCheck(data, m_occurrenceDisplay, OccurrenceDisplay::Label);
        case ID_VIEW_OPTIONS_HTML_RECORD_OCCURRENCE_NUMBERS:     return ViewOptionsHelper::HandleCheck(data, m_occurrenceDisplay, OccurrenceDisplay::Number);

        case ID_VIEW_OPTIONS_HTML_VALUE_CASE_TREE:               return ViewOptionsHelper::HandleCheck(data, m_caseItemPrinterFormat, CaseItemPrinter::Format::CaseTree);
        case ID_VIEW_OPTIONS_HTML_VALUE_LABELS:                  return ViewOptionsHelper::HandleCheck(data, m_caseItemPrinterFormat, CaseItemPrinter::Format::Label);
        case ID_VIEW_OPTIONS_HTML_VALUE_CODES:                   return ViewOptionsHelper::HandleCheck(data, m_caseItemPrinterFormat, CaseItemPrinter::Format::Code);
        case ID_VIEW_OPTIONS_HTML_VALUE_LABELS_AND_CODES:        return ViewOptionsHelper::HandleCheck(data, m_caseItemPrinterFormat, CaseItemPrinter::Format::LabelCode);
        case ID_VIEW_OPTIONS_HTML_VALUE_CODES_AND_LABELS:        return ViewOptionsHelper::HandleCheck(data, m_caseItemPrinterFormat, CaseItemPrinter::Format::CodeLabel);

        case ID_VIEW_OPTIONS_HTML_VALUE_SHOW_BLANKS:             return ViewOptionsHelper::HandleCheck(data, m_blankValues, BlankValues::Show);
        case ID_VIEW_OPTIONS_HTML_VALUE_HIDE_BLANKS:             return ViewOptionsHelper::HandleCheck(data, m_blankValues, BlankValues::Hide);

        case ID_VIEW_OPTIONS_HTML_CASE_DEFAULT_DETAILS:          return ViewOptionsHelper::HandleCheck(data, m_statuses, Statuses::ShowIfNotDefault);
        case ID_VIEW_OPTIONS_HTML_CASE_ALL_DETAILS:              return ViewOptionsHelper::HandleCheck(data, m_statuses, Statuses::Show);

        case ID_VIEW_OPTIONS_HTML_CASE_SHOW_ERRORS:              return ViewOptionsHelper::HandleCheck(data, m_caseConstructionErrors, CaseConstructionErrors::Show);
        case ID_VIEW_OPTIONS_HTML_CASE_HIDE_ERRORS:              return ViewOptionsHelper::HandleCheck(data, m_caseConstructionErrors, CaseConstructionErrors::Hide);

        case ID_TOGGLE_DICTIONARY_DISPLAY:                       return ViewOptionsHelper::HandleAccelerator(data, m_nameDisplay);
        case ID_TOGGLE_VALUE_DISPLAY:                            return ViewOptionsHelper::HandleAccelerator(data, m_caseItemPrinterFormat);

        default:                                                 return ViewOptionsHelper::HandleUnknownCommand(data);
    }
}


CaseHtmlContentCreatorSettings CaseHtmlContentCreatorSettings::CreateFromJson(const JsonNode& json_node)
{
    CaseHtmlContentCreatorSettings settings;

    settings.m_statuses = json_node.Get<Statuses>(JK::statuses);
    settings.m_caseConstructionErrors = json_node.Get<CaseConstructionErrors>(JK::errors);
    settings.m_nameDisplay = json_node.Get<NameDisplay>(JK::dictionaryNames);
    settings.m_recordOrientation = json_node.Get<RecordOrientation>(JK::recordOrientation);
    settings.m_occurrenceDisplay = json_node.Get<OccurrenceDisplay>(JK::occurrences);
    settings.m_itemTypeDisplay = json_node.Get<ItemTypeDisplay>(JK::itemDisplay);
    settings.m_blankValues = json_node.Get<BlankValues>(JK::blankValues);
    settings.m_caseItemPrinterFormat = json_node.Get<CaseItemPrinter::Format>(JK::valueDisplay);

    return settings;
}


void CaseHtmlContentCreatorSettings::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::statuses, m_statuses)
               .Write(JK::errors, m_caseConstructionErrors)
               .Write(JK::dictionaryNames, m_nameDisplay)
               .Write(JK::recordOrientation, m_recordOrientation)
               .Write(JK::occurrences, m_occurrenceDisplay)
               .Write(JK::itemDisplay, m_itemTypeDisplay)
               .Write(JK::blankValues, m_blankValues)
               .Write(JK::valueDisplay, m_caseItemPrinterFormat)
               .EndObject();
}



// --------------------------------------------------------------------------
// CaseHtmlContentCreator
// --------------------------------------------------------------------------

CaseHtmlContentCreator::CaseHtmlContentCreator(CaseHoldingDoc& case_holding_doc)
    :   CaseContentCreatorBase(case_holding_doc),
        m_caseHtmlContentCreatorSettings(case_holding_doc.GetSettings<CaseHtmlContentCreatorSettings>()),
        m_embedResources(false),
        m_lastProcessedDataCasePositionInRepository(-1)
{
}


const wchar_t* CaseHtmlContentCreator::GetSaveTitle() const
{
    return L"Save Case HTML";
}


UINT CaseHtmlContentCreator::GetViewOptionsMenuResourceId() const
{
    return IDR_VIEW_CASE_HTML;
}


bool CaseHtmlContentCreator::ProcessViewOptionsMenu(const std::variant<UINT, CCmdUI*> data)
{
    return m_caseHtmlContentCreatorSettings->ProcessViewOptionsMenu(data);
}


void CaseHtmlContentCreator::ProcessWebViewMessage(const JsonNode& json_node)
{
    const bool open = ( json_node.GetFromStringOptions(JK::action, { "open", "save" }) == 0 );
    const std::string suggested_filename = json_node.Get<std::string>(JK::filename);
    const std::shared_ptr<const std::vector<std::byte>> binary_data = GetBinaryData(json_node.Get<std::string_view>(JK::key));

    if( !m_caseHoldingDoc.DictionaryAllowsExport(open ? "opening binary data" : "saving binary data") )
        return;

    if( open )
    {
        std::string temporary_file_path = GetUniqueTempFilePath(suggested_filename);
        FileIO::Write(temporary_file_path, *binary_data);

        // make the file read-only
        SetFileAttributes(TC::ToWide(temporary_file_path).c_str(), FILE_ATTRIBUTE_READONLY);

        OpenFileInAssociatedApplication(temporary_file_path);

        TemporaryFile::RegisterFileForDeletion(std::move(temporary_file_path));
    }

    else
    {
        SaveFileDlg save_file_dlg(0, PortableFunctions::PathGetFileExtension(suggested_filename), suggested_filename);

        if( !suggested_filename.empty() )
            save_file_dlg.SetTitle(FormatText("Save Binary Data (%s) As", suggested_filename.c_str()));

        if( save_file_dlg.DoModal() == IDOK )
            FileIO::Write(save_file_dlg.GetFilePath(), *binary_data);
    }
}


SharableString CaseHtmlContentCreator::GetHtmlContentWorker(const bool embed_resources/* = false*/)
{
    ASSERT(m_dataCase != nullptr);

    // clear any binary data handlers that may have existed for other cases
    if( m_lastProcessedDataCasePositionInRepository != m_dataCase->GetPositionInRepository() )
    {
        m_lastProcessedDataCasePositionInRepository = m_dataCase->GetPositionInRepository();
        m_binaryDataVirtualFileMappingHandlers.clear();
    }

    m_embedResources = embed_resources;
    m_caseHtmlContentCreatorSettings->SetCaseHtmlContentCreator(*this);

    return m_caseHtmlContentCreatorSettings->ToHtml(*m_dataCase);
}


std::shared_ptr<const std::vector<std::byte>> CaseHtmlContentCreator::GetBinaryData(const std::string_view access_key_sv)
{
    if( m_dataCase != nullptr )
    {
        const CaseItem* case_item;
        std::unique_ptr<CaseItemIndex> index;
        std::tie(case_item, index) = CaseItemIndex::FromSerializableText(*m_dataCase, access_key_sv);

        if( case_item != nullptr )
        {
            const BinaryCaseItem& binary_case_item = assert_cast<const BinaryCaseItem&>(*case_item);
            return binary_case_item.GetBinaryDataAccessor(*index).GetBinaryData().GetSharedContent();
        }
    }

    throw CSProException("The binary data for key '%s' could not be accessed.",
                         ( m_dataCase != nullptr ) ? m_dataCase->GetKey().c_str() : "");
}


const std::string& CaseHtmlContentCreator::CreateBinaryDataHandler(const std::string& access_key, const std::string& mime_type, const std::string& suggested_filename)
{
    const auto& lookup = m_binaryDataVirtualFileMappingHandlers.find(access_key);

    if( lookup != m_binaryDataVirtualFileMappingHandlers.cend() )
        return lookup->second->GetUrl();

    VirtualFileMappingHandler& virtual_file_mapping_handler =
        *m_binaryDataVirtualFileMappingHandlers.try_emplace(access_key, std::make_unique<CallbackVirtualFileMappingHandler>(
        [this, this_access_key = access_key, this_mime_type = mime_type]
        (VirtualFileMappingResponse& response)
        {
            try
            {
                response.SetContent(GetBinaryData(this_access_key), this_mime_type);
                return true;
            }

            catch( const CSProException& exception )
            {
                ErrorMessage::PostMessageForDisplay(exception);
                return false;
            }
        })).first->second;

    SharedHtmlLocalFileServer& file_server = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetSharedHtmlLocalFileServer();
    file_server.CreateVirtualFile(virtual_file_mapping_handler, suggested_filename);

    return virtual_file_mapping_handler.GetUrl();
}
