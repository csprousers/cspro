#include "StdAfx.h"
#include "QuestionnaireView.h"
#include "DictionaryBasedDoc.h"
#include <zJson/Json.h>
#include <zAppO/Application.h>
#include <zFormatterO/QuestionnaireViewer.h>
#include <zAction/AccessToken.h>
#include <zAction/WebController.h>


// --------------------------------------------------------------------------
// DesignerQuestionnaireViewer
// --------------------------------------------------------------------------

class DesignerQuestionnaireViewer : public QuestionnaireViewer
{
public:
    DesignerQuestionnaireViewer(DictionaryBasedDoc& document);

    // QuestionnaireViewer overrides
    std::wstring GetDictionaryName() override;
    std::wstring GetCurrentLanguageName() override;
    bool ShowLanguageBar() override;
    std::wstring GetDirectoryForUrl() override;

private:
    DictionaryBasedDoc& m_document;
};


DesignerQuestionnaireViewer::DesignerQuestionnaireViewer(DictionaryBasedDoc& document)
    :   m_document(document)
{
}


std::wstring DesignerQuestionnaireViewer::GetDictionaryName()
{
    const CDataDict* dictionary = m_document.GetSharedDictionary().get();

    if( dictionary != nullptr )
        return CS2WS(dictionary->GetName());

    return ReturnProgrammingError(std::wstring());        
}


std::wstring DesignerQuestionnaireViewer::GetCurrentLanguageName()
{
    std::wstring language_name;
    WindowsDesktopMessage::Send(UWM::Designer::GetCurrentLanguageName, &m_document, &language_name);
    return language_name;
}


bool DesignerQuestionnaireViewer::ShowLanguageBar()
{
    // there is no reason to the show the language bar since the Designer already has one
    return false;
}


std::wstring DesignerQuestionnaireViewer::GetDirectoryForUrl()
{
    Application* application;

    if( WindowsDesktopMessage::Send(UWM::Designer::GetApplication, &application, &m_document) == 1 )
        return PortableFunctions::PathGetDirectory(CS2WS(application->GetApplicationFilename()));

    return std::wstring();
}



// --------------------------------------------------------------------------
// QuestionnaireView
// --------------------------------------------------------------------------

IMPLEMENT_DYNAMIC(QuestionnaireView, CFormView)


BEGIN_MESSAGE_MAP(QuestionnaireView, CFormView)
    ON_WM_SIZE()
    // VQ_TODO focusChange is not handled at the moment ... ON_MESSAGE(UWM::Designer::TreeSelectionChanged, OnTreeSelectionChanged)
END_MESSAGE_MAP()


QuestionnaireView::QuestionnaireView(DictionaryBasedDoc& document)
    :   CFormView(IDD_QUESTIONNAIRE_VIEW),
        m_questionnaireViewer(std::make_unique<DesignerQuestionnaireViewer>(document))
{
    // set up the Action Invoker to serve the questionnaire JSON
    ActionInvoker::WebController& web_controller = m_htmlViewCtrl.RegisterCSProHostObject();
    web_controller.GetCaller().AddAccessTokenOverride(std::wstring(ActionInvoker::AccessToken::QuestionnaireView_Index_sv));

    web_controller.GetListener().SetOnGetInputDataCallback([&]() { return m_questionnaireViewer->GetInputData(); });

    m_htmlViewCtrl.NavigateTo(m_questionnaireViewer->GetUrl());
}


QuestionnaireView::~QuestionnaireView()
{
}


BOOL QuestionnaireView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
    // VQ_TODO ... this can probably be optimized to create a Create that takes no parameters other than the
    // m_pCurrentDoc + m_pCurrentFrame values of CCreateContext, and maybe nID
    return __super::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}


void QuestionnaireView::DoDataExchange(CDataExchange* pDX)
{
	__super::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_QUESTIONNAIRE_HTML, m_htmlViewCtrl);
}


void QuestionnaireView::OnSize(UINT nType, int cx, int cy)
{
    __super::OnSize(nType, cx, cy);

    // resize the HTML control so that it fills up the client window
    if( m_htmlViewCtrl.m_hWnd != nullptr )
    {
        CRect client_rect;
        GetClientRect(&client_rect);
        m_htmlViewCtrl.MoveWindow(client_rect);
    }
}


template<typename CF>
void QuestionnaireView::SendMessageToWebView2(const TCHAR* action, CF callback_function)
{
    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    json_writer->Write(JK::action, action);
    callback_function(*json_writer);

    json_writer->EndObject();

    const std::wstring function_call = SO::Concatenate(_T("onMessage(\""),
                                                       Encoders::ToEscapedString(json_writer->GetString()),
                                                       _T("\");"));
    m_htmlViewCtrl.ExecuteScript(function_call,
        [&](const std::wstring& result)
        {
            // if the message was handled, the onMessage function should return true;
            // otherwise the page will be refreshed
            try
            {
                if( Json::Parse(result).Get<bool>() )
                    return;
            }
            catch( const JsonParseException& ) { }

            m_htmlViewCtrl.Reload();
        });
}


void QuestionnaireView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
    __super::OnUpdate(pSender, lHint, pHint);

    if( lHint == Hint::UseQuestionTextChanged )
    {
        RefreshContent(true);
    }

    else if( lHint == Hint::LanguageChanged )
    {
        SendMessageToWebView2(_T("languageChange"),
            [&](JsonWriter& json_writer)
            {
                json_writer.Write(JK::language, m_questionnaireViewer->GetCurrentLanguageName());
            });
    }
}


void QuestionnaireView::RefreshContent(const bool refresh_inputs)
{
    // if necessary, set up the questionnaire viewer again
    if( refresh_inputs )
    {
        // with the refactoring to have the questionnaire viewer call CS.Application.getQuestionnaireContent,
        // this is no longer necessary, but we will keep this here in case we bring back a model where the
        // inputs are being set using non-Action Invoker methods
        m_questionnaireViewer->ResetInputs();
    }

    SendMessageToWebView2(_T("refreshContent"),
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::name, m_questionnaireViewer->GetDictionaryName());
        });
}


LRESULT QuestionnaireView::OnTreeSelectionChanged(WPARAM wParam, LPARAM /*lParam*/)
{
    const TCHAR* name = reinterpret_cast<const TCHAR*>(wParam);
    ASSERT(name != nullptr);

    SendMessageToWebView2(_T("focusChange"),
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(_T("focus"), name);
        });

    return 1;
}
