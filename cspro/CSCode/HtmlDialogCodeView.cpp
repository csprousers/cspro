#include "StdAfx.h"
#include "HtmlDialogCodeView.h"
#include "HtmlDialogTemplates.h"


namespace
{
    // input text, associated with saved files, will be persisted for two weeks
    constexpr const char* InputTextTableName     = "html_dialogs_input";
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


std::variant<const CDocument*, std::string> HtmlDialogCodeView::GetDocumentOrTitleForBuildWnd() const
{
    return "HTML Dialog JSON Input";
}


void HtmlDialogCodeView::OnInitialUpdate()
{
    OnInitialUpdateWorker(GetInitialText());
}


void HtmlDialogCodeView::OnDestroy()
{
    // save the current input text for future use
    const CodeDoc& code_doc = GetCodeDoc();

    if( !code_doc.GetFilePath().empty() )
        m_settingsDb.Write(code_doc.GetFilePath(), GetLogicCtrl()->GetText(), true);

    __super::OnDestroy();
}


std::string HtmlDialogCodeView::GetInitialText()
{
    const CodeDoc& code_doc = GetCodeDoc();
    ASSERT(code_doc.GetLanguageSettings().GetLanguageType() == LanguageType::CSProHtmlDialog);

    SharableString input_text;

    // 1) see if any input text has been associated with this file in a previous session
    if( !code_doc.GetFilePath().empty() )
        input_text = m_settingsDb.Read<std::string>(code_doc.GetFilePath(), false);

    // 2) if not, see if there is a dialog template with input text
    if( !input_text.IsSet() )
        input_text = HtmlDialogTemplateFile().GetDefaultInputText(code_doc.GetFilePath());

    if( input_text.IsSet() )
        return input_text.Release();

    // 3) otherwise create a default JSON object for the input text
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter(JsonFormattingOptions::PrettySpacing);

    json_writer->BeginObject();

    json_writer->BeginObject(JK::inputData)
                .EndObject();

    // for the display options, default to 75% of the display size
    constexpr const char* DisplayRatioText = "75%";

    json_writer->BeginObject(JK::displayOptions)
                .Write(JK::width, DisplayRatioText)
                .Write(JK::height, DisplayRatioText)
                .EndObject();

    json_writer->EndObject();

    return json_writer->ReleaseString();
}
