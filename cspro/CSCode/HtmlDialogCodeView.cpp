#include "StdAfx.h"
#include "HtmlDialogCodeView.h"
#include "HtmlDialogTemplates.h"


namespace
{
    // input text, associated with saved files, will be persisted for two weeks
    constexpr const TCHAR* InputTextTableName    = _T("html_dialogs_input");
    constexpr int64_t InputTextExpirationSeconds = DateHelper::SecondsInWeek(2);
}


IMPLEMENT_DYNCREATE(HtmlDialogCodeView, CodeView)

BEGIN_MESSAGE_MAP(HtmlDialogCodeView, CodeView)
    ON_WM_DESTROY()
END_MESSAGE_MAP()


HtmlDialogCodeView::HtmlDialogCodeView()
    :   m_languageSettings(LanguageType::Json),
        m_settingsDb(CSProExecutables::Program::CSCode, InputTextTableName, InputTextExpirationSeconds, SettingsDb::KeyObfuscator::Hash)
{
}


std::variant<const CDocument*, std::wstring> HtmlDialogCodeView::GetDocumentOrTitleForBuildWnd() const
{
    return _T("HTML Dialog JSON Input");
}


void HtmlDialogCodeView::OnInitialUpdate()
{
    OnInitialUpdateWorker(GetInitialText());
}


void HtmlDialogCodeView::OnDestroy()
{
    // save the current input text for future use
    const CodeDoc& code_doc = GetCodeDoc();

    if( !code_doc.GetPathName().IsEmpty() )
        m_settingsDb.Write(code_doc.GetPathName(), GetLogicCtrl()->GetText(), true);

    __super::OnDestroy();
}


std::wstring HtmlDialogCodeView::GetInitialText()
{
    const CodeDoc& code_doc = GetCodeDoc();
    ASSERT(code_doc.GetLanguageSettings().GetLanguageType() == LanguageType::CSProHtmlDialog);

    std::optional<std::wstring> input_text;

    // 1) see if any input text has been associated with this file in a previous session
    if( !code_doc.GetPathName().IsEmpty() )
        input_text = m_settingsDb.Read<std::wstring>(code_doc.GetPathName(), false);

    // 2) if not, see if there is a dialog template with input text
    if( !input_text.has_value() )
        input_text = HtmlDialogTemplateFile().GetDefaultInputText(CS2WS(code_doc.GetPathName()));

    if( input_text.has_value() )
        return std::move(*input_text);

    // 3) otherwise create a default JSON object for the input text
    auto json_writer = Json::CreateStringWriter(JsonFormattingOptions::PrettySpacing);

    json_writer->BeginObject();

    json_writer->BeginObject(JK::inputData)
                .EndObject();

    // for the display options, default to 75% of the display size
    constexpr const TCHAR* DisplayRatioText = _T("75%");

    json_writer->BeginObject(JK::displayOptions)
                .Write(JK::width, DisplayRatioText)
                .Write(JK::height, DisplayRatioText)
                .EndObject();

    json_writer->EndObject();

    return json_writer->GetString();
}
