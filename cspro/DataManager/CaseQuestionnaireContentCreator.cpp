#include "StdAfx.h"
#include "CaseQuestionnaireContentCreator.h"
#include "CaseQuestionnaireContentCreatorSettings.h"
#include "QuestionnaireContentDlg.h"
#include "ViewOptionsHelper.h"
#include <zUtilO/ApplicationLoadException.h>
#include <zHtml/VirtualFileMapping.h>
#include <zFormO/FormFile.h>
#include <zCapiO/CapiQuestionManager.h>
#include <zFormatterO/QuestionnaireViewer.h>


CREATE_JSON_KEY(contentUrl)


// --------------------------------------------------------------------------
// CaseQuestionnaireContentCreatorSettings
// --------------------------------------------------------------------------

void CaseQuestionnaireContentCreatorSettings::SetFormFile(std::string file_path, std::shared_ptr<const CDEFormFile> form_file)
{
    m_formFilePath = std::move(file_path);
    m_formFile = std::move(form_file);

    ASSERT(m_formFilePath.empty() == ( m_formFile == nullptr ));
}


void CaseQuestionnaireContentCreatorSettings::SetCapiQuestionManager(std::string file_path, std::shared_ptr<const CapiQuestionManager> capi_question_manager)
{
    m_questionTextFilePath = std::move(file_path);
    m_capiQuestionManager = std::move(capi_question_manager);

    ASSERT(m_questionTextFilePath.empty() == ( m_capiQuestionManager == nullptr ));
}


CaseQuestionnaireContentCreatorSettings CaseQuestionnaireContentCreatorSettings::CreateFromJson(const JsonNode& json_node)
{
    CaseQuestionnaireContentCreatorSettings settings;

    if( json_node.Contains(JK::forms) )
        settings.m_formFilePath = json_node.GetAbsolutePath(JK::forms);

    if( json_node.Contains(JK::questionText) )
        settings.m_questionTextFilePath = json_node.GetAbsolutePath(JK::questionText);

    return settings;
}


void CaseQuestionnaireContentCreatorSettings::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject();

    if( !m_formFilePath.empty() )
        json_writer.WritePath(JK::forms, m_formFilePath);

    if( !m_questionTextFilePath.empty() )
        json_writer.WritePath(JK::questionText, m_questionTextFilePath);

    json_writer.EndObject();
}



// --------------------------------------------------------------------------
// CaseQuestionnaireContentCreator::ContentVirtualFileMappingHandler
// --------------------------------------------------------------------------

class CaseQuestionnaireContentCreator::ContentVirtualFileMappingHandler : public VirtualFileMappingHandler
{
public:
    CaseQuestionnaireContentCreator::ContentVirtualFileMappingHandler(std::shared_ptr<const std::string> questionnaire_content)
        :   m_questionnaireContent(std::move(questionnaire_content))
    {
        ASSERT(m_questionnaireContent != nullptr);
    }

    bool ServeContent(VirtualFileMappingResponse& response) override
    {
        try
        {
            response.SetContent(*m_questionnaireContent, MimeType::Type::Json);
            return true;
        }
        catch(...) { return ReturnProgrammingError(false); }
    }

private:
    std::shared_ptr<const std::string> m_questionnaireContent;
};



// --------------------------------------------------------------------------
// CaseQuestionnaireContentCreator
// --------------------------------------------------------------------------

CaseQuestionnaireContentCreator::CaseQuestionnaireContentCreator(CaseHoldingDoc& case_holding_doc)
    :   CaseContentCreatorBase(case_holding_doc),
        m_settings(case_holding_doc.GetSettings<CaseQuestionnaireContentCreatorSettings>()),
        m_quesionnaireViewHtml(QuestionnaireViewer::GetQuestionnaireViewHtml()),
        m_questionnaireContentCreator(std::make_unique<QuestionnaireContentCreator>()),
        m_questionnaireContent(std::make_unique<std::string>()),
        m_contentVirtualFileMappingHandler(std::make_unique<ContentVirtualFileMappingHandler>(m_questionnaireContent))
{
    m_questionnaireContentCreator->SetDictionary(m_caseHoldingDoc.GetSharedDictionary());
    m_questionnaireContentCreator->SetBypassDictionaryMatchesCheck();

    LinkAssociatedQuestionnaireContent();

    SharedHtmlLocalFileServer& file_server = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetSharedHtmlLocalFileServer();
    file_server.CreateVirtualFile(*m_contentVirtualFileMappingHandler);
}


SharableString CaseQuestionnaireContentCreator::GetActionInvokerInputData()
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::language, m_caseHoldingDoc.GetDictionary().GetCurrentLanguage().GetName())
                .Write(JK::showLanguageBar, false)
                .Write(JK::contentUrl, m_contentVirtualFileMappingHandler->GetUrl())
                .EndObject();

    return json_writer->ReleaseSharableString();
}


UINT CaseQuestionnaireContentCreator::GetViewOptionsMenuResourceId() const
{
    return IDR_VIEW_CASE_QUESTIONNAIRE;
}


bool CaseQuestionnaireContentCreator::ProcessViewOptionsMenu(const std::variant<UINT, CCmdUI*> data)
{
    switch( ViewOptionsHelper::GetCommandId(data) )
    {
        case ID_VIEW_OPTIONS_QUESTIONNAIRE_MODIFY_CONTENT: return ViewOptionsHelper::RouteCommand(data, *this, &CaseQuestionnaireContentCreator::ModifyAssociatedQuestionnaireContent);
        default:                                           return ViewOptionsHelper::HandleUnknownCommand(data);
    }
}


std::unique_ptr<const CDEFormFile> CaseQuestionnaireContentCreator::LoadFormFile(const std::string& file_path, std::shared_ptr<const CDataDict> dictionary)
{
    ASSERT(dictionary != nullptr);

    auto form_file = std::make_unique<CDEFormFile>();

    if( !form_file->Open(file_path, true) )
        throw ApplicationFileLoadException(file_path, "form");

    const std::unique_ptr<const CDataDict> form_file_dictionary = CDataDict::InstantiateAndOpen(form_file->GetDictionaryFilename(), true);

    if( form_file_dictionary->GetName() != dictionary->GetName() )
    {
        throw CSProException("The form file uses the dictionary '%s', not the dictionary associated with this data: '%s'",
                             form_file_dictionary->GetName().c_str(),
                             dictionary->GetName().c_str());
    }

    // use the dictionary object associated with the case, rather than form_file_dictionary,
    // which is only used to make sure the dictionary name matched
    form_file->SetDictionary(std::move(dictionary));
    form_file->UpdatePointers();

    return form_file;
}


std::unique_ptr<const CapiQuestionManager> CaseQuestionnaireContentCreator::LoadCapiQuestionManager(const std::string& file_path)
{
    auto capi_question_manager = std::make_unique<CapiQuestionManager>();

    capi_question_manager->Load(file_path);

    return capi_question_manager;
}


void CaseQuestionnaireContentCreator::LinkAssociatedQuestionnaireContent()
{
    auto post_exception = [&](const CSProException& exception)
    {
        ErrorMessage::PostMessageForDisplay(FormatText("There were errors loading the questionnaire content associated with the dictionary '%s':\n\n%s",
                                                       m_caseHoldingDoc.GetDictionary().GetName().c_str(),
                                                       exception.what()));
    };

    if( m_settings->GetFormFile() == nullptr && !m_settings->GetFormFilePath().empty() )
    {
        try
        {
            m_settings->SetFormFile(m_settings->GetFormFilePath(),
                                    LoadFormFile(m_settings->GetFormFilePath(), m_caseHoldingDoc.GetSharedDictionary()));
        }

        catch( const CSProException& exception )
        {
            m_settings->SetFormFile(std::string(), nullptr);
            post_exception(exception);
        }
    }

    if( m_settings->GetCapiQuestionManager() == nullptr && !m_settings->GetQuestionTextFilePath().empty() )
    {
        try
        {
            m_settings->SetCapiQuestionManager(m_settings->GetQuestionTextFilePath(),
                                               LoadCapiQuestionManager(m_settings->GetQuestionTextFilePath()));
        }

        catch( const CSProException& exception )
        {
            m_settings->SetCapiQuestionManager(std::string(), nullptr);
            post_exception(exception);
        }
    }

    SyncQuestionnaireContentCreatorWithAssociatedQuestionnaireContent();
}


bool CaseQuestionnaireContentCreator::ModifyAssociatedQuestionnaireContent()
{
    QuestionnaireContentDlg dlg(m_caseHoldingDoc, *m_settings);

    if( dlg.DoModal() != IDOK )
        return false;

    *m_settings = dlg.GetSettings();
    SyncQuestionnaireContentCreatorWithAssociatedQuestionnaireContent();

    return true;
}


void CaseQuestionnaireContentCreator::SyncQuestionnaireContentCreatorWithAssociatedQuestionnaireContent()
{
    m_questionnaireContentCreator->SetFormFile(m_settings->GetFormFile());
    m_questionnaireContentCreator->SetCapiQuestionManager(m_settings->GetCapiQuestionManager());

    // force the recreation of the content server in case the question text path changed
    m_htmlContentServer.reset();
}


void CaseQuestionnaireContentCreator::GetUrlWorker()
{
    UpdateCurrentCase();
    m_questionnaireContentCreator->SetCase(m_dataCase);

    *m_questionnaireContent = m_questionnaireContentCreator->GetContent();

    if( !m_htmlContentServer.has_value() )
    {
        SharedHtmlLocalFileServer& file_server = assert_cast<CMainFrame*>(AfxGetMainWnd())->GetSharedHtmlLocalFileServer();

        // if question text is specified, serve the content using a virtual HTML file located in that directory
        // so that relative paths such as images embedded in question text will display properly
        if( !m_settings->GetQuestionTextFilePath().empty() )
        {
            const std::string directory_for_url = PortableFunctions::PathGetDirectory(m_settings->GetQuestionTextFilePath());
            m_htmlContentServer.emplace(file_server.CreateVirtualHtmlFile(directory_for_url, [&]() { return m_quesionnaireViewHtml; }));
        }

        else
        {
            m_htmlContentServer = std::make_unique<TextVirtualFileMappingHandler>(m_quesionnaireViewHtml, MimeType::Type::Html);
            file_server.CreateVirtualFile(*std::get<std::unique_ptr<VirtualFileMappingHandler>>(*m_htmlContentServer));
        }
    }
}
