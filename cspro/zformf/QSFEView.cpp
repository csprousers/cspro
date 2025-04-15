#include "StdAfx.h"
#include "QSFEView.h"
#include "TableToolbarButton.h"
#include <zToolsO/Encoders.h>
#include <zToolsO/FileIO.h>
#include <zUtilO/BCMenu.h>
#include <zUtilF/ImageFileDialog.h>
#include <zHtml/SharedHtmlLocalFileServer.h>
#include <zCapiO/CapiLogicParameters.h>
#include <zCapiO/CapiQuestionManager.h>
#include <zCapiO/CapiStyle.h>
#include <zCapiO/CapiText.h>


namespace
{
    constexpr UINT TimerId = 20250414;
}


IMPLEMENT_DYNAMIC(CQSFEView, CFormView)

BEGIN_MESSAGE_MAP(CQSFEView, CFormView)

    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_WM_CONTEXTMENU()
    ON_WM_TIMER()

    ON_COMMAND(ID_VIEW_FORM, OnViewForm)
    ON_COMMAND(ID_VIEW_LOGIC, OnViewLogic)
    ON_COMMAND(ID_TOGGLE_QSF_SECOND_VIEW, OnToggleSecondView)

    ON_EN_CHANGE(IDC_HTML_EDIT, OnChangeHtmlEdit)
    ON_EN_SETFOCUS(IDC_HTML_EDIT, OnSetFocusHtmlEdit)

    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)

    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)

    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)

    ON_COMMAND(ID_EDIT_PASTE_WITHOUT_FORMATTING, OnEditPasteWithoutFormatting)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE_WITHOUT_FORMATTING, OnUpdateEditPaste)

    ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)

    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    ON_COMMAND(ID_EDIT_REDO, OnEditRedo)

    ON_CBN_SELENDOK(IDC_STYLE, OnFormatStyle)
    ON_UPDATE_COMMAND_UI(IDC_STYLE, OnUpdateFormatStyle)

    ON_COMMAND(ID_FORMAT_BOLD, OnFormatBold)
    ON_UPDATE_COMMAND_UI(ID_FORMAT_BOLD, OnUpdateFormatBold)

    ON_COMMAND(ID_FORMAT_ITALIC, OnFormatItalic)
    ON_UPDATE_COMMAND_UI(ID_FORMAT_ITALIC, OnUpdateFormatItalic)

    ON_COMMAND(ID_FORMAT_UNDERLINE, OnFormatUnderline)
    ON_UPDATE_COMMAND_UI(ID_FORMAT_UNDERLINE, OnUpdateFormatUnderline)

    ON_CBN_SELENDOK(IDC_FONTFACE, OnFormatFontFace)
    ON_UPDATE_COMMAND_UI(IDC_FONTFACE, OnUpdateFormatFontFace)

    ON_CBN_SELENDOK(IDC_FONTSIZE, OnFormatFontSize)
    ON_UPDATE_COMMAND_UI(IDC_FONTSIZE, OnUpdateFormatFontSize)

    ON_COMMAND(ID_FORMAT_COLOR, OnFormatColor)
    ON_UPDATE_COMMAND_UI(ID_FORMAT_COLOR, OnUpdateFormatColor)

    ON_COMMAND(ID_FORMAT_ALIGNLEFT, OnFormatAlignLeft)
    ON_UPDATE_COMMAND_UI(ID_FORMAT_ALIGNLEFT, OnUpdateFormatAlignLeft)

    ON_COMMAND(ID_FORMAT_ALIGNCENTER, OnFormatAlignCenter)
    ON_UPDATE_COMMAND_UI(ID_FORMAT_ALIGNCENTER, OnUpdateFormatAlignCenter)

    ON_COMMAND(ID_FORMAT_ALIGNRIGHT, OnFormatAlignRight)
    ON_UPDATE_COMMAND_UI(ID_FORMAT_ALIGNRIGHT, OnUpdateFormatAlignRight)

    ON_COMMAND(ID_FORMAT_OUTLINE_BULLET, OnEditFormatOutlineBullet)
    ON_UPDATE_COMMAND_UI(ID_FORMAT_OUTLINE_BULLET, OnUpdateEditFormatOutlineBullet)

    ON_COMMAND(ID_FORMAT_OUTLINE_NUMBER, OnEditFormatOutlineNumbering)
    ON_UPDATE_COMMAND_UI(ID_FORMAT_OUTLINE_NUMBER, OnUpdateEditFormatOutlineNumbering)

    ON_COMMAND(ID_EDIT_INSERT_IMAGE, OnEditInsertImage)
    ON_UPDATE_COMMAND_UI(ID_EDIT_INSERT_IMAGE, OnUpdateEditInsertImage)

    ON_UPDATE_COMMAND_UI(ID_INSERT_TABLE, OnUpdateInsertTable)
    ON_COMMAND(ID_INSERT_TABLE, OnInsertTable)

    ON_UPDATE_COMMAND_UI(ID_INSERT_LINK, OnUpdateInsertLink)
    ON_COMMAND(ID_INSERT_LINK, OnInsertLink)

    ON_COMMAND(ID_TEXT_DIR_RTL, OnChangeTextDirectionRightToLeft)
    ON_UPDATE_COMMAND_UI(ID_TEXT_DIR_RTL, OnUpdateChangeTextDirectionRightToLeft)

    ON_COMMAND(ID_TEXT_DIR_LTR, OnChangeTextDirectionLeftToRight)
    ON_UPDATE_COMMAND_UI(ID_TEXT_DIR_LTR, OnUpdateChangeTextDirectionLeftToRight)

    ON_COMMAND_RANGE(ID_QSF_EDITOR_EDIT_HTML_VISUAL, ID_QSF_EDITOR_EDIT_HTML_VISUAL_CODE, OnChangeEditType)
    ON_UPDATE_COMMAND_UI_RANGE(ID_QSF_EDITOR_EDIT_HTML_VISUAL, ID_QSF_EDITOR_EDIT_HTML_VISUAL_CODE, OnUpdateChangeEditType)

    ON_COMMAND_RANGE(ID_QSF_EDITOR_VIEW_QUESTION, ID_QSF_EDITOR_VIEW_HELP, OnViewQuestionHelpText)
    ON_UPDATE_COMMAND_UI_RANGE(ID_QSF_EDITOR_VIEW_QUESTION, ID_QSF_EDITOR_VIEW_HELP, OnUpdateViewQuestionHelpText)

    ON_COMMAND(IDC_EDIT_LANG, OnLanguageChanged)
    ON_CBN_SELENDOK(IDC_EDIT_LANG, OnLanguageChanged)

END_MESSAGE_MAP()


CQSFEView::CQSFEView(CFormDoc* const pFormDoc)
    :   CFormView(IDD_QSF_EDIT_VIEW),
        m_htmlEditorCtrl(std::make_unique<HtmlEditorCtrl>()),
        m_textType(CapiText::Type::Question),
        m_languageIndex(0)
{
    ASSERT(pFormDoc != nullptr);

    // get the file path for this form file's application
    Application* application;

    if( WindowsDesktopMessage::Send(UWM::Designer::GetApplication, &application, pFormDoc) == 1 )
    {
        m_applicationFilePath = application->GetApplicationFilePath();
    }

    else
    {
        ASSERT(false);
        m_applicationFilePath = TC::ToUtf8(pFormDoc->GetPathName());
    }

    SetUpFileServer();
    ASSERT(m_questionTextVirtualFileMapping != nullptr);

    m_htmlEditorCtrl->SetUrl(m_questionTextVirtualFileMapping->GetUrl());
}


CQSFEView::~CQSFEView()
{
}


void CQSFEView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_HTML_EDIT, *m_htmlEditorCtrl);
}


void CQSFEView::OnUpdate(CView* const pSender, const LPARAM lHint, CObject* /*pHint*/)
{
    if( pSender == this )
        return;

    if( lHint == Hint::CapiEditorUpdateLanguages )
        SetLanguages(GetFormDoc()->GetCapiQuestionManager()->GetLanguages());

    if( lHint == Hint::CapiEditorUpdateStyles || lHint == Hint::CapiEditorUpdateQuestionStyles )
        SetStyles(GetFormDoc()->GetCapiQuestionManager()->GetStyles());

    if( lHint == Hint::CapiEditorUpdateQuestion || lHint == Hint::CapiEditorUpdateQuestionStyles )
    {
        m_fillSyntaxCheckResults.clear();
        UpdateFillErrorDisplay();
    }

    UpdateDisplayText();
}


int CQSFEView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if( __super::OnCreate(lpCreateStruct) == -1 )
        return -1;

    CapiQuestionManager* const question_manager = GetFormDoc()->GetCapiQuestionManager();
    SetStyles(question_manager->GetStyles());

    if( !m_toolbar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC,
                            CRect(1, 1, 1, 1), AFX_IDW_TOOLBAR + 50) )
    {
        return -1;
    }

    if( !m_toolbar.LoadToolBar(IDR_QSFBAR) )
        return -1;

    m_toolbar.SetRouteCommandsViaFrame(FALSE);

    SetLanguages(question_manager->GetLanguages());

    return 0;
}


void CQSFEView::OnDestroy()
{
    StopIdleTimer();
    m_questionTextVirtualFileMapping.reset();
}


void CQSFEView::OnSize(const UINT nType, const int cx, const int cy)
{
    __super::OnSize(nType, cx, cy);

    CRect client_rect;
    GetClientRect(&client_rect);

    m_toolbar.WrapToolBar(client_rect.Width(), client_rect.Height());

    CSize toolbar_size = m_toolbar.CalcSize(FALSE);

    constexpr int ToolbarVerticalSpacing = 4;
    toolbar_size.cy += ToolbarVerticalSpacing;

    m_toolbar.SetWindowPos(nullptr, 0, 0, toolbar_size.cx, toolbar_size.cy, SWP_NOACTIVATE | SWP_NOZORDER);

    client_rect.top += toolbar_size.cy;

    if( m_htmlEditorCtrl->m_hWnd != nullptr )
    {
        m_htmlEditorCtrl->MoveWindow(client_rect);
        m_htmlEditorCtrl->SendMessage(WM_SIZE, nType, MAKELPARAM(client_rect.Width(), client_rect.Height()));
    }
}


void CQSFEView::OnContextMenu(CWnd* /*pWnd*/, const CPoint point)
{
    BCMenu popup_menu;
    popup_menu.CreatePopupMenu();

    popup_menu.AppendMenu(MF_STRING | ( m_htmlEditorCtrl->CanCut() ? MF_ENABLED : MF_GRAYED ), ID_EDIT_CUT, L"Cu&t\tCtrl+X");
    popup_menu.AppendMenu(MF_STRING | ( m_htmlEditorCtrl->CanCopy() ? MF_ENABLED : MF_GRAYED ), ID_EDIT_COPY, L"&Copy\tCtrl+C");
    popup_menu.AppendMenu(MF_STRING | ( m_htmlEditorCtrl->CanPaste() ? MF_ENABLED : MF_GRAYED ), ID_EDIT_PASTE, L"&Paste\tCtrl+V");
    popup_menu.AppendMenu(MF_STRING | ( m_htmlEditorCtrl->CanPaste() ? MF_ENABLED : MF_GRAYED ), ID_EDIT_PASTE_WITHOUT_FORMATTING, L"Paste &Without formatting\tCtrl+Shift+V");
    popup_menu.AppendMenu(MF_SEPARATOR);
    popup_menu.AppendMenu(MF_STRING | ( m_htmlEditorCtrl->GetText().empty() ? MF_GRAYED : MF_ENABLED ), ID_EDIT_SELECT_ALL, L"Select &All");
    popup_menu.AppendMenu(MF_SEPARATOR);

    CFormChildWnd* const pParentFrame = assert_cast<CFormChildWnd*>(GetParentFrame());

    popup_menu.AppendMenu(MF_STRING, ID_TOGGLE_QSF_SECOND_VIEW, pParentFrame->m_bHideSecondLang ? L"&Show Second View" :
                                                                                                  L"&Hide Second View");

    popup_menu.AppendMenu(MF_SEPARATOR);
    popup_menu.AppendMenu(MF_STRING, ID_VIEW_FORM, L"View &Form");
    popup_menu.AppendMenu(MF_STRING, ID_VIEW_LOGIC, L"View &Logic");

    popup_menu.LoadToolbar(IDR_FORM_FRAME);
    popup_menu.TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);
}


void CQSFEView::OnTimer(const UINT nIDEvent)
{
    if( nIDEvent != TimerId )
        return;

    StopIdleTimer();

    CFormDoc* const form_doc = GetFormDoc();
    CapiEditorViewModel& view_model = form_doc->GetCapiEditorViewModel();

    if( !view_model.CanHaveText() )
        return;

    CapiText text = view_model.GetText(m_languageIndex, m_textType);
    bool updated = false;

    for( const CapiFill& fill : text.GetFills() )
    {
        SharableString fill_text(fill.GetTextToEvaluate_sv());

        if( m_fillSyntaxCheckResults.find(fill_text.GetString()) == m_fillSyntaxCheckResults.end() )
        {
            CapiEditorViewModel::SyntaxCheckResult result = view_model.CheckSyntax(CapiLogicParameters::Type::Fill, fill_text);
            m_fillSyntaxCheckResults.try_emplace(fill_text.Release(), result);
            updated = true;
        }
    }

    if( updated )
        UpdateFillErrorDisplay();
}


void CQSFEView::SetLanguages(std::vector<Language> languages)
{
    ASSERT(!languages.empty());
    m_languages = std::move(languages);
    m_toolbar.SetLanguages(m_languages);
    SetLanguage(0);
}


void CQSFEView::SetLanguage(const size_t language_index)
{
    ASSERT(language_index < m_languages.size());
    m_languageIndex = language_index;
    m_toolbar.SetLanguage(m_languages[m_languageIndex]);
}


void CQSFEView::SetLanguage(const std::string_view language_label_sv)
{
    const auto& lookup = std::find_if(m_languages.cbegin(), m_languages.cend(),
                                      [&](const Language& language) { return ( language.GetLabel() == language_label_sv ); });

    if( lookup != m_languages.end() )
        SetLanguage(std::distance(m_languages.cbegin(), lookup));
}


void CQSFEView::SetStyles(const std::vector<CapiStyle>& styles)
{
    std::vector<HtmlEditorCtrl::Style> editor_styles;

    for( const CapiStyle& style : styles )
        editor_styles.emplace_back(HtmlEditorCtrl::Style{ "span", style.name, style.class_name, style.css });

    m_htmlEditorCtrl->SetStyles(editor_styles);
    m_toolbar.SetStyles(editor_styles);
}


bool CQSFEView::IsDirty() const
{
    return m_htmlEditorCtrl->IsDirty();
}


CFormDoc* CQSFEView::GetFormDoc()
{
    return assert_cast<CFormDoc*>(GetDocument());
}


void CQSFEView::SetUpFileServer()
{
    // to make relative paths in the question text work, the HTML editor must
    // appear as if it exists in the application directory; we will load the
    // editing HTML once and then issue it as a virtual file
    static SharableString editor_html;

    if( !editor_html.IsSet() )
    {
        try
        {
            const std::string editor_html_file_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::HtmlEditor), "index.html");
            editor_html = FileIO::ReadText(editor_html_file_path);
        }

        catch( const FileIO::Exception& )
        {
            editor_html = "<html><body><p>There was an error loading the HTML editor.</p></body></html>";
        }
    }

    m_fileServer = std::make_unique<SharedHtmlLocalFileServer>();

    m_questionTextVirtualFileMapping = std::make_unique<VirtualFileMapping>(
        m_fileServer->CreateVirtualHtmlFile(PortableFunctions::PathGetDirectory(m_applicationFilePath),
        [&]()
        {
            return editor_html;
        }));
}


void CQSFEView::UpdateDisplayText()
{
    CFormDoc* const form_doc = GetFormDoc();
    CapiEditorViewModel& view_model = form_doc->GetCapiEditorViewModel();

    if( view_model.CanHaveText() )
    {
        EnableWindow(TRUE);
        m_htmlEditorCtrl->EnableWindow(TRUE);

        const CapiText& capi_text = view_model.GetText(m_languageIndex, m_textType);

        if( capi_text.GetText()->empty() )
        {
            m_htmlEditorCtrl->Clear();
        }

        else
        {
            std::wstring wide_text = TC::ToWide(capi_text.GetText().GetString());

            if( wide_text != m_htmlEditorCtrl->GetText() )
                m_htmlEditorCtrl->SetText(std::move(wide_text));

            StartIdleTimer();
        }
    }

    else
    {
        EnableWindow(FALSE);
        m_htmlEditorCtrl->EnableWindow(FALSE);

        m_htmlEditorCtrl->Clear();
    }

    UpdateToolbar();
}


void CQSFEView::UpdateToolbar()
{
    CCmdUI button_cmd_ui;
    button_cmd_ui.m_nIndexMax = m_toolbar.GetCount();
    button_cmd_ui.m_pOther = &m_toolbar;

    for( UINT i = 0; i < button_cmd_ui.m_nIndexMax; ++i )
    {
        const UINT nID = m_toolbar.GetItemID(i);

        if( nID != 0 )
        {
            button_cmd_ui.m_nIndex = i;
            button_cmd_ui.m_nID = nID;
            button_cmd_ui.DoUpdate(this, FALSE);
        }
    }
}


void CQSFEView::StartIdleTimer()
{
    m_idleTimer = SetTimer(TimerId, 1000, nullptr);
}


void CQSFEView::StopIdleTimer()
{
    if( m_idleTimer.has_value() )
    {
        KillTimer(TimerId);
        m_idleTimer.reset();
    }
}


void CQSFEView::UpdateFillErrorDisplay()
{
    std::map<std::string, std::string> errors;

    for( const auto& [fill, result] : m_fillSyntaxCheckResults )
    {
        if( std::holds_alternative<CapiEditorViewModel::SyntaxCheckError>(result) )
            errors.try_emplace(fill, std::get<CapiEditorViewModel::SyntaxCheckError>(result).error_message);
    }

    m_htmlEditorCtrl->SetSyntaxErrors(errors);
}


void CQSFEView::OnViewForm()
{
    CFormChildWnd* const pParentFrame = assert_cast<CFormChildWnd*>(GetParentFrame());
    pParentFrame->OnViewForm();
}


void CQSFEView::OnViewLogic()
{
    CFormChildWnd* const pParentFrame = assert_cast<CFormChildWnd*>(GetParentFrame());
    pParentFrame->OnViewLogic();
}


void CQSFEView::OnToggleSecondView()
{
    CFormChildWnd* const pParentFrame = assert_cast<CFormChildWnd*>(GetParentFrame());
    pParentFrame->m_bHideSecondLang = !pParentFrame->m_bHideSecondLang;
    pParentFrame->DisplayActiveMode();
}


void CQSFEView::OnChangeHtmlEdit()
{
    // Text changed in editor - update it in document
    CFormDoc* const form_doc = GetFormDoc();
    CapiEditorViewModel& view_model = form_doc->GetCapiEditorViewModel();

    if( view_model.CanHaveText() )
        view_model.SetText(m_languageIndex, m_textType, TC::ToUtf8(m_htmlEditorCtrl->GetText()));

    StartIdleTimer();
}


void CQSFEView::OnSetFocusHtmlEdit()
{
    // Make this the active view - this ensures that menu selections (undo, cut, paste...)
    // will be routed to this view
    GetParentFrame()->SetActiveView(this);
    GetDocument()->UpdateAllViews(nullptr, Hint::CapiEditorUpdateStyles);
}


void CQSFEView::OnEditCopy()
{
    m_htmlEditorCtrl->Copy();
}


void CQSFEView::OnUpdateEditCopy(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(m_htmlEditorCtrl->CanCopy());
}


void CQSFEView::OnEditCut()
{
    m_htmlEditorCtrl->Cut();
}


void CQSFEView::OnUpdateEditCut(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(m_htmlEditorCtrl->CanCut());
}


void CQSFEView::OnEditPaste()
{
    m_htmlEditorCtrl->Paste(true);
}


void CQSFEView::OnEditPasteWithoutFormatting()
{
    m_htmlEditorCtrl->Paste(false);
}


void CQSFEView::OnUpdateEditPaste(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(m_htmlEditorCtrl->CanPaste());
}


void CQSFEView::OnEditSelectAll()
{
    m_htmlEditorCtrl->SelectAll();
}


void CQSFEView::OnEditUndo()
{
    m_htmlEditorCtrl->Undo();
}


void CQSFEView::OnEditRedo()
{
    m_htmlEditorCtrl->Redo();
}


void CQSFEView::OnFormatStyle()
{
    m_htmlEditorCtrl->ApplyStyle(m_toolbar.GetSelectedStyle());
    m_htmlEditorCtrl->MoveFocus();
}


void CQSFEView::OnUpdateFormatStyle(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());

    if( pCmdUI->m_pOther == &m_toolbar )
    {
        const HtmlEditorCtrl::Style* const style = m_htmlEditorCtrl->GetStyle();

        if( style != nullptr )
            m_toolbar.SetSelectedStyle(*style);
    }
}


void CQSFEView::OnFormatBold()
{
    m_htmlEditorCtrl->Bold();
}


void CQSFEView::OnUpdateFormatBold(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());

    if( pCmdUI->m_pOther == &m_toolbar )
        pCmdUI->SetCheck(m_htmlEditorCtrl->IsBold());
}


void CQSFEView::OnFormatItalic()
{
    m_htmlEditorCtrl->Italic();
}


void CQSFEView::OnUpdateFormatItalic(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());

    if( pCmdUI->m_pOther == &m_toolbar )
        pCmdUI->SetCheck(m_htmlEditorCtrl->IsItalic());
}


void CQSFEView::OnFormatUnderline()
{
    m_htmlEditorCtrl->Underline();
}


void CQSFEView::OnUpdateFormatUnderline(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());

    if( pCmdUI->m_pOther == &m_toolbar )
        pCmdUI->SetCheck(m_htmlEditorCtrl->IsUnderline());
}


void CQSFEView::OnFormatFontFace()
{
    m_htmlEditorCtrl->SetFont(m_toolbar.GetFontFace());
    m_htmlEditorCtrl->MoveFocus();
}


void CQSFEView::OnUpdateFormatFontFace(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());

    if( pCmdUI->m_pOther == &m_toolbar )
        m_toolbar.SetFontFace(UTF8_TODO::GetWide(m_htmlEditorCtrl->GetFontName()));
}


void CQSFEView::OnFormatFontSize()
{
    m_htmlEditorCtrl->SetFontSize(m_toolbar.GetFontSize());
    m_htmlEditorCtrl->MoveFocus();
}


void CQSFEView::OnUpdateFormatFontSize(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());

    if( pCmdUI->m_pOther == &m_toolbar )
        m_toolbar.SetFontSize(m_htmlEditorCtrl->GetFontSize());
}


void CQSFEView::OnFormatColor()
{
    const COLORREF color = m_toolbar.GetForeColor();
    m_htmlEditorCtrl->SetForeColor(color);
}


void CQSFEView::OnUpdateFormatColor(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());
}


void CQSFEView::OnFormatAlignLeft()
{
    m_htmlEditorCtrl->AlignLeft();
}


void CQSFEView::OnUpdateFormatAlignLeft(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());

    if( pCmdUI->m_pOther == &m_toolbar )
        pCmdUI->SetCheck(m_htmlEditorCtrl->GetTextAlignment() == HtmlEditorCtrl::TextAlign::Left);
}


void CQSFEView::OnFormatAlignCenter()
{
    m_htmlEditorCtrl->AlignCenter();
}


void CQSFEView::OnUpdateFormatAlignCenter(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());

    if( pCmdUI->m_pOther == &m_toolbar )
        pCmdUI->SetCheck(m_htmlEditorCtrl->GetTextAlignment() == HtmlEditorCtrl::TextAlign::Center);
}


void CQSFEView::OnFormatAlignRight()
{
    m_htmlEditorCtrl->AlignRight();
}


void CQSFEView::OnUpdateFormatAlignRight(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());

    if( pCmdUI->m_pOther == &m_toolbar )
        pCmdUI->SetCheck(m_htmlEditorCtrl->GetTextAlignment() == HtmlEditorCtrl::TextAlign::Right);
}


void CQSFEView::OnEditFormatOutlineBullet()
{
    m_htmlEditorCtrl->UnorderedList();
}


void CQSFEView::OnUpdateEditFormatOutlineBullet(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());

    if( pCmdUI->m_pOther == &m_toolbar )
        pCmdUI->SetCheck(m_htmlEditorCtrl->GetListStyle() == HtmlEditorCtrl::ListStyle::Unordered);
}


void CQSFEView::OnEditFormatOutlineNumbering()
{
    m_htmlEditorCtrl->OrderedList();
}


void CQSFEView::OnUpdateEditFormatOutlineNumbering(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());

    if( pCmdUI->m_pOther == &m_toolbar )
        pCmdUI->SetCheck(m_htmlEditorCtrl->GetListStyle() == HtmlEditorCtrl::ListStyle::Ordered);
}


void CQSFEView::OnEditInsertImage()
{
    ImageFileDialog dlg;

    if( dlg.DoModal() != IDOK )
        return;

    std::string image_file_path_on_disk = TC::ToUtf8(dlg.GetPathName());

    if( image_file_path_on_disk.empty() )
        return;

    if( PathGetVolume(image_file_path_on_disk) != PathGetVolume(m_applicationFilePath) )
    {
        ErrorMessage::Display(L"Images must be on the same disk volume as your CSPro application. "
                              L"Try copying the file to the folder that contains your application.");
        return;
    }

    std::string relative_path = GetRelativePath(m_applicationFilePath, image_file_path_on_disk);
    Path::MakeToForwardSlash(relative_path);
    m_htmlEditorCtrl->InsertImage(Encoders::ToUri(std::move(relative_path), false));
}


void CQSFEView::OnUpdateEditInsertImage(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());
}


void CQSFEView::OnInsertTable()
{
    const CSize dimensions = m_toolbar.GetTableDimensions();
    m_htmlEditorCtrl->InsertTable(dimensions);
}


void CQSFEView::OnUpdateInsertTable(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());
}


void CQSFEView::OnInsertLink()
{
    m_htmlEditorCtrl->ShowInsertLinkDialog();
}


void CQSFEView::OnUpdateInsertLink(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());
}


void CQSFEView::OnChangeTextDirectionRightToLeft()
{
    m_htmlEditorCtrl->RightToLeft();
}


void CQSFEView::OnUpdateChangeTextDirectionRightToLeft(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());
}


void CQSFEView::OnChangeTextDirectionLeftToRight()
{
    m_htmlEditorCtrl->LeftToRight();
}


void CQSFEView::OnUpdateChangeTextDirectionLeftToRight(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl->GetCodeViewShowing());
}



void CQSFEView::OnChangeEditType(const UINT nID)
{
    const bool edit_html_code = ( nID == ID_QSF_EDITOR_EDIT_HTML_VISUAL_CODE );

    if( edit_html_code != m_htmlEditorCtrl->GetCodeViewShowing() )
        m_htmlEditorCtrl->ToggleCodeView();
}


void CQSFEView::OnUpdateChangeEditType(CCmdUI* const pCmdUI)
{
    bool check = ( pCmdUI->m_pOther == &m_toolbar &&
                   GetFormDoc()->GetCapiEditorViewModel().CanHaveText() );

    if( check )
    {
        const bool edit_html_code = ( pCmdUI->m_nID == ID_QSF_EDITOR_EDIT_HTML_VISUAL_CODE );
        check = ( edit_html_code == m_htmlEditorCtrl->GetCodeViewShowing() );
    }

    pCmdUI->Enable();
    pCmdUI->SetCheck(check);
}


void CQSFEView::OnViewQuestionHelpText(const UINT nID)
{
    const CapiText::Type this_text_type = ( nID == ID_QSF_EDITOR_VIEW_QUESTION ) ? CapiText::Type::Question :
                                                                                   CapiText::Type::Help;

    if( m_textType != this_text_type )
    {
        m_textType = this_text_type;
        UpdateDisplayText();
    }
}


void CQSFEView::OnUpdateViewQuestionHelpText(CCmdUI* const pCmdUI)
{
    bool check = ( pCmdUI->m_pOther == &m_toolbar &&
                   GetFormDoc()->GetCapiEditorViewModel().CanHaveText() );

    if( check )
    {
        check = ( m_textType == ( ( pCmdUI->m_nID == ID_QSF_EDITOR_VIEW_QUESTION ) ? CapiText::Type::Question :
                                                                                     CapiText::Type::Help ) );
    }

    pCmdUI->Enable();
    pCmdUI->SetCheck(check);
}


void CQSFEView::OnLanguageChanged()
{
    SetLanguage(m_toolbar.GetLanguageLabel());
    UpdateDisplayText();
}
