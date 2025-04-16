#include "StdAfx.h"
#include "QSFEView.h"
#include "TableToolbarButton.h"
#include <zToolsO/Encoders.h>
#include <zUtilO/BCMenu.h>
#include <zUtilF/ImageFileDialog.h>
#include <zHtml/InsertLinkDlg.h>
#include <zCapiO/CapiQuestionManager.h>


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

    ON_EN_SETFOCUS(IDC_HTML_EDIT, OnSetFocusEditor)
    ON_EN_CHANGE(IDC_HTML_EDIT, OnChangeHtmlEditor)

    ON_EN_SETFOCUS(IDC_QSF_LOGIC_CONTROL, OnSetFocusEditor)
    ON_EN_CHANGE(IDC_QSF_LOGIC_CONTROL, OnChangeTextEditor)

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
    ON_UPDATE_COMMAND_UI(ID_FORMAT_COLOR, OnUpdateIsActiveEditorAcceptingVisualStyles)

    ON_COMMAND_RANGE(ID_FORMAT_ALIGN_LEFT, ID_FORMAT_ALIGN_RIGHT, OnFormatAlign)
    ON_UPDATE_COMMAND_UI_RANGE(ID_FORMAT_ALIGN_LEFT, ID_FORMAT_ALIGN_RIGHT, OnUpdateFormatAlign)

    ON_COMMAND(ID_FORMAT_OUTLINE_BULLET, OnEditFormatOutlineBullet)
    ON_UPDATE_COMMAND_UI(ID_FORMAT_OUTLINE_BULLET, OnUpdateEditFormatOutlineBullet)

    ON_COMMAND(ID_FORMAT_OUTLINE_NUMBER, OnEditFormatOutlineNumbering)
    ON_UPDATE_COMMAND_UI(ID_FORMAT_OUTLINE_NUMBER, OnUpdateEditFormatOutlineNumbering)

    ON_COMMAND(ID_EDIT_INSERT_IMAGE, OnEditInsertImage)
    ON_UPDATE_COMMAND_UI(ID_EDIT_INSERT_IMAGE, OnUpdateIsActiveEditorAcceptingVisualStyles)

    ON_COMMAND(ID_INSERT_TABLE, OnInsertTable)
    ON_UPDATE_COMMAND_UI(ID_INSERT_TABLE, OnUpdateIsActiveEditorAcceptingVisualStyles)

    ON_COMMAND(ID_INSERT_LINK, OnInsertLink)
    ON_UPDATE_COMMAND_UI(ID_INSERT_LINK, OnUpdateIsActiveEditorAcceptingVisualStyles)

    ON_COMMAND(ID_TEXT_DIR_RTL, OnChangeTextDirectionRightToLeft)
    ON_UPDATE_COMMAND_UI(ID_TEXT_DIR_RTL, OnUpdateIsActiveEditorVisualHtml)

    ON_COMMAND(ID_TEXT_DIR_LTR, OnChangeTextDirectionLeftToRight)
    ON_UPDATE_COMMAND_UI(ID_TEXT_DIR_LTR, OnUpdateIsActiveEditorVisualHtml)

    ON_COMMAND_RANGE(ID_QSF_EDITOR_EDIT_HTML_VISUAL, ID_QSF_EDITOR_EDIT_TEXT_MARKDOWN, OnChangeEditorType)
    ON_UPDATE_COMMAND_UI_RANGE(ID_QSF_EDITOR_EDIT_HTML_VISUAL, ID_QSF_EDITOR_EDIT_TEXT_MARKDOWN, OnUpdateChangeEditorType)

    ON_COMMAND_RANGE(ID_QSF_EDITOR_VIEW_QUESTION, ID_QSF_EDITOR_VIEW_HELP, OnViewQuestionHelpText)
    ON_UPDATE_COMMAND_UI_RANGE(ID_QSF_EDITOR_VIEW_QUESTION, ID_QSF_EDITOR_VIEW_HELP, OnUpdateViewQuestionHelpText)

    ON_COMMAND(IDC_EDIT_LANG, OnLanguageChanged)
    ON_CBN_SELENDOK(IDC_EDIT_LANG, OnLanguageChanged)

END_MESSAGE_MAP()


CQSFEView::CQSFEView(CFormDoc* const pFormDoc)
    :   CFormView(IDD_QSF_EDIT_VIEW),
        m_editors{ &m_htmlEditor, &m_textEditor },
        m_currentEditor(&m_htmlEditor),
        m_textTypeEditing(CapiText::Type::Question),
        m_languageIndex(0)
{
    ASSERT(pFormDoc != nullptr);

    // get the file path for this form file's application
    if( WindowsDesktopMessage::Send(UWM::Designer::GetApplication, &m_application, pFormDoc) == 1 )
    {
        m_applicationFilePath = m_application->GetApplicationFilePath();
    }

    else
    {
        ASSERT(false);
        m_application = nullptr;
        m_applicationFilePath = TC::ToUtf8(pFormDoc->GetPathName());
    }
}


void CQSFEView::DoDataExchange(CDataExchange* const pDX)
{
    __super::DoDataExchange(pDX);

    DDX_Control(pDX, IDC_HTML_EDIT, m_htmlEditor.GetWnd());
    DDX_Control(pDX, IDC_QSF_LOGIC_CONTROL, m_textEditor.GetWnd());
}


void CQSFEView::OnInitialUpdate()
{
    __super::OnInitialUpdate();

    // show the HTML editor by default
    ASSERT(m_currentEditor == &m_htmlEditor),
    m_textEditor.GetWnd().ShowWindow(SW_HIDE);

    for( QuestionTextEditor* const editor : m_editors )
        editor->Initialize(this, m_applicationFilePath);
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
        m_currentEditor->UpdateFillErrorDisplay(m_fillSyntaxCheckResults);
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

    for( QuestionTextEditor* const editor : m_editors )
        editor->Destroy();
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

    for( QuestionTextEditor* const editor : m_editors )
    {
        CWnd& wnd = editor->GetWnd();

        if( wnd.m_hWnd != nullptr )
        {
            wnd.MoveWindow(client_rect);
            wnd.SendMessage(WM_SIZE, nType, MAKELPARAM(client_rect.Width(), client_rect.Height()));
        }
    }
}


void CQSFEView::OnContextMenu(CWnd* /*pWnd*/, const CPoint point)
{
    BCMenu popup_menu;
    popup_menu.CreatePopupMenu();

    popup_menu.AppendMenu(MF_STRING | ( m_currentEditor->CanCut() ? MF_ENABLED : MF_GRAYED ), ID_EDIT_CUT, L"Cu&t\tCtrl+X");
    popup_menu.AppendMenu(MF_STRING | ( m_currentEditor->CanCopy() ? MF_ENABLED : MF_GRAYED ), ID_EDIT_COPY, L"&Copy\tCtrl+C");
    popup_menu.AppendMenu(MF_STRING | ( m_currentEditor->CanPaste() ? MF_ENABLED : MF_GRAYED ), ID_EDIT_PASTE, L"&Paste\tCtrl+V");

    if( IsActiveEditorVisualHtml() )
        popup_menu.AppendMenu(MF_STRING | ( m_currentEditor->CanPaste() ? MF_ENABLED : MF_GRAYED ), ID_EDIT_PASTE_WITHOUT_FORMATTING, L"Paste &Without formatting\tCtrl+Shift+V");

    popup_menu.AppendMenu(MF_SEPARATOR);
    popup_menu.AppendMenu(MF_STRING | ( m_currentEditor->HasContent() ? MF_ENABLED : MF_GRAYED ), ID_EDIT_SELECT_ALL, L"Select &All");
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

    CapiText text = view_model.GetText(m_languageIndex, m_textTypeEditing);
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
        m_currentEditor->UpdateFillErrorDisplay(m_fillSyntaxCheckResults);
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

    m_toolbar.SetStyles(editor_styles);

    m_htmlEditor.GetHtmlEditorCtrl().SetStyles(std::move(editor_styles));
}


bool CQSFEView::IsDirty() const
{
    return m_currentEditor->IsDirty();
}


CFormDoc* CQSFEView::GetFormDoc()
{
    return assert_cast<CFormDoc*>(GetDocument());
}


void CQSFEView::SetCorrectEditor()
{
    QuestionTextEditor* const correct_editor =
        ( m_currentCapiText.GetFormat() == CapiText::Format::Html ) ? static_cast<QuestionTextEditor*>(&m_htmlEditor) :
                                                                      static_cast<QuestionTextEditor*>(&m_textEditor);

    if( m_currentEditor != correct_editor )
    {
        m_currentEditor->GetWnd().ShowWindow(SW_HIDE);

        m_currentEditor = correct_editor;

        CWnd& wnd = m_currentEditor->GetWnd();
        wnd.ShowWindow(SW_SHOW);
        wnd.EnableWindow();
    }

    m_currentEditor->UpdateForFormat(m_application, m_currentCapiText.GetFormat());
}


void CQSFEView::UpdateDisplayText()
{
    CFormDoc* const form_doc = GetFormDoc();
    CapiEditorViewModel& view_model = form_doc->GetCapiEditorViewModel();

    if( view_model.CanHaveText() )
    {
        EnableWindow(TRUE);
        m_currentEditor->GetWnd().EnableWindow(TRUE);

        m_currentCapiText = view_model.GetText(m_languageIndex, m_textTypeEditing);
        SetCorrectEditor();

        if( m_currentCapiText.GetText()->empty() )
        {
            m_currentEditor->ClearContent();
        }

        else
        {
            m_currentEditor->SetContent(m_currentCapiText.GetText().GetString());

            StartIdleTimer();
        }
    }

    else
    {
        EnableWindow(FALSE);
        m_currentEditor->GetWnd().EnableWindow(FALSE);

        m_currentEditor->ClearContent();
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


void CQSFEView::OnSetFocusEditor()
{
    // Make this the active view - this ensures that menu selections (undo, cut, paste...)
    // will be routed to this view
    GetParentFrame()->SetActiveView(this);
    GetDocument()->UpdateAllViews(nullptr, Hint::CapiEditorUpdateStyles);
}


void CQSFEView::OnChangeHtmlEditor()
{
    ASSERT(m_currentEditor == &m_htmlEditor);

    // Text changed in HTML editor - update it in document
    CFormDoc* const form_doc = GetFormDoc();
    CapiEditorViewModel& view_model = form_doc->GetCapiEditorViewModel();

    if( view_model.CanHaveText() )
    {
        m_currentCapiText = CapiText(m_htmlEditor.GetContent(), CapiText::Format::Html);

        view_model.SetText(m_languageIndex, m_textTypeEditing, m_currentCapiText);
    }

    StartIdleTimer();
}


void CQSFEView::OnChangeTextEditor()
{
    ASSERT(m_currentEditor == &m_textEditor);

    // Text changed in text editor - update it in document
    CFormDoc* const form_doc = GetFormDoc();
    CapiEditorViewModel& view_model = form_doc->GetCapiEditorViewModel();

    if( view_model.CanHaveText() )
    {
        m_currentCapiText = CapiText(m_textEditor.GetContent(), m_currentCapiText.GetFormat());

        view_model.SetText(m_languageIndex, m_textTypeEditing, m_currentCapiText);
    }

    StartIdleTimer();
}


bool CQSFEView::IsActiveEditorVisualHtml()
{
    return ( m_currentEditor == &m_htmlEditor &&
             !m_htmlEditor.GetHtmlEditorCtrl().GetCodeViewShowing() );
}


bool CQSFEView::IsActiveEditorAcceptingVisualStyles()
{
    return ( m_currentEditor == &m_textEditor ||
             !m_htmlEditor.GetHtmlEditorCtrl().GetCodeViewShowing() );
}


void CQSFEView::OnUpdateIsActiveEditorVisualHtml(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(IsActiveEditorVisualHtml());
}


void CQSFEView::OnUpdateIsActiveEditorAcceptingVisualStyles(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(IsActiveEditorAcceptingVisualStyles());
}


void CQSFEView::OnEditCopy()
{
    m_currentEditor->Copy();
}


void CQSFEView::OnUpdateEditCopy(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(m_currentEditor->CanCopy());
}


void CQSFEView::OnEditCut()
{
    m_currentEditor->Cut();
}


void CQSFEView::OnUpdateEditCut(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(m_currentEditor->CanCut());
}


void CQSFEView::OnEditPaste()
{
    m_currentEditor->Paste(true);
}


void CQSFEView::OnEditPasteWithoutFormatting()
{
    ASSERT(IsActiveEditorVisualHtml());

    m_currentEditor->Paste(false);
}


void CQSFEView::OnUpdateEditPaste(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(m_currentEditor->CanPaste());
}


void CQSFEView::OnEditSelectAll()
{
    m_currentEditor->SelectAll();
}


void CQSFEView::OnEditUndo()
{
    m_currentEditor->Undo();
}


void CQSFEView::OnEditRedo()
{
    m_currentEditor->Redo();
}


void CQSFEView::OnFormatStyle()
{
    ASSERT(IsActiveEditorVisualHtml());

    HtmlEditorCtrl& html_editor_ctrl = m_htmlEditor.GetHtmlEditorCtrl();
    html_editor_ctrl.ApplyStyle(m_toolbar.GetSelectedStyle());
    html_editor_ctrl.MoveFocus();
}


void CQSFEView::OnUpdateFormatStyle(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(IsActiveEditorVisualHtml());

    if( pCmdUI->m_pOther == &m_toolbar )
    {
        const HtmlEditorCtrl::Style* const style = m_htmlEditor.GetHtmlEditorCtrl().GetStyle();

        if( style != nullptr )
            m_toolbar.SetSelectedStyle(*style);
    }
}


void CQSFEView::OnFormatBold()
{
    ASSERT(IsActiveEditorAcceptingVisualStyles());

    m_currentEditor->Bold();
}


void CQSFEView::OnUpdateFormatBold(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(IsActiveEditorAcceptingVisualStyles());

    if( pCmdUI->m_pOther == &m_toolbar && IsActiveEditorVisualHtml() )
        pCmdUI->SetCheck(m_htmlEditor.GetHtmlEditorCtrl().IsBold());
}


void CQSFEView::OnFormatItalic()
{
    ASSERT(IsActiveEditorAcceptingVisualStyles());

    m_currentEditor->Italic();
}


void CQSFEView::OnUpdateFormatItalic(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(IsActiveEditorAcceptingVisualStyles());

    if( pCmdUI->m_pOther == &m_toolbar && IsActiveEditorVisualHtml() )
        pCmdUI->SetCheck(m_htmlEditor.GetHtmlEditorCtrl().IsItalic());
}


void CQSFEView::OnFormatUnderline()
{
    ASSERT(IsActiveEditorAcceptingVisualStyles());

    m_currentEditor->Underline();
}


void CQSFEView::OnUpdateFormatUnderline(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(IsActiveEditorAcceptingVisualStyles());

    if( pCmdUI->m_pOther == &m_toolbar && IsActiveEditorVisualHtml() )
        pCmdUI->SetCheck(m_htmlEditor.GetHtmlEditorCtrl().IsUnderline());
}


void CQSFEView::OnFormatFontFace()
{
    ASSERT(IsActiveEditorVisualHtml());

    HtmlEditorCtrl& html_editor_ctrl = m_htmlEditor.GetHtmlEditorCtrl();
    html_editor_ctrl.SetFont(m_toolbar.GetFontFace());
    html_editor_ctrl.MoveFocus();
}


void CQSFEView::OnUpdateFormatFontFace(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(IsActiveEditorVisualHtml());

    if( pCmdUI->m_pOther == &m_toolbar )
        m_toolbar.SetFontFace(TC::ToWide(m_htmlEditor.GetHtmlEditorCtrl().GetFontName()));
}


void CQSFEView::OnFormatFontSize()
{
    ASSERT(IsActiveEditorVisualHtml());

    HtmlEditorCtrl& html_editor_ctrl = m_htmlEditor.GetHtmlEditorCtrl();
    html_editor_ctrl.SetFontSize(m_toolbar.GetFontSize());
    html_editor_ctrl.MoveFocus();
}


void CQSFEView::OnUpdateFormatFontSize(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(IsActiveEditorVisualHtml());

    if( pCmdUI->m_pOther == &m_toolbar )
        m_toolbar.SetFontSize(m_htmlEditor.GetHtmlEditorCtrl().GetFontSize());
}


void CQSFEView::OnFormatColor()
{
    ASSERT(IsActiveEditorAcceptingVisualStyles());

    const COLORREF color = m_toolbar.GetForeColor();
    m_currentEditor->SetForeColor(color);
}


template<>
HtmlEditorCtrl::TextAlign CQSFEView::ConvertResourceId(const UINT nID)
{
    switch( nID )
    {
        case ID_FORMAT_ALIGN_LEFT:
            return HtmlEditorCtrl::TextAlign::Left;

        case ID_FORMAT_ALIGN_CENTER:
            return HtmlEditorCtrl::TextAlign::Center;

        default:
            ASSERT(nID == ID_FORMAT_ALIGN_RIGHT);
            return HtmlEditorCtrl::TextAlign::Right;
    }
}


void CQSFEView::OnFormatAlign(const UINT nID)
{
    ASSERT(IsActiveEditorVisualHtml());

    m_htmlEditor.GetHtmlEditorCtrl().Align(ConvertResourceId<HtmlEditorCtrl::TextAlign>(nID));
}


void CQSFEView::OnUpdateFormatAlign(CCmdUI* const pCmdUI)
{
    const bool is_active_editor_visual_html = IsActiveEditorVisualHtml();
    pCmdUI->Enable(is_active_editor_visual_html);

    if( is_active_editor_visual_html && pCmdUI->m_pOther == &m_toolbar )
        pCmdUI->SetCheck(( m_htmlEditor.GetHtmlEditorCtrl().GetTextAlignment() == ConvertResourceId<HtmlEditorCtrl::TextAlign>(pCmdUI->m_nID)) );
}


void CQSFEView::OnEditFormatOutlineBullet()
{
    ASSERT(IsActiveEditorAcceptingVisualStyles());
    m_currentEditor->UnorderedList();
}


void CQSFEView::OnUpdateEditFormatOutlineBullet(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(IsActiveEditorAcceptingVisualStyles());

    if( pCmdUI->m_pOther == &m_toolbar && IsActiveEditorVisualHtml() )
        pCmdUI->SetCheck(( m_htmlEditor.GetHtmlEditorCtrl().GetListStyle() == HtmlEditorCtrl::ListStyle::Unordered) );
}


void CQSFEView::OnEditFormatOutlineNumbering()
{
    ASSERT(IsActiveEditorAcceptingVisualStyles());
    m_currentEditor->OrderedList();
}


void CQSFEView::OnUpdateEditFormatOutlineNumbering(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(IsActiveEditorAcceptingVisualStyles());

    if( pCmdUI->m_pOther == &m_toolbar && IsActiveEditorVisualHtml() )
        pCmdUI->SetCheck(( m_htmlEditor.GetHtmlEditorCtrl().GetListStyle() == HtmlEditorCtrl::ListStyle::Ordered) );
}


void CQSFEView::OnEditInsertImage()
{
    ASSERT(IsActiveEditorAcceptingVisualStyles());

    ImageFileDialog image_file_dlg;

    if( image_file_dlg.DoModal() != IDOK )
        return;

    std::string image_file_path_on_disk = TC::ToUtf8(image_file_dlg.GetPathName());

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

    m_currentEditor->InsertImage(Encoders::ToUri(std::move(relative_path), false));
}


void CQSFEView::OnInsertTable()
{
    ASSERT(IsActiveEditorAcceptingVisualStyles());

    const CSize dimensions = m_toolbar.GetTableDimensions();
    m_currentEditor->InsertTable(dimensions.cx, dimensions.cy);
}


void CQSFEView::OnInsertLink()
{
    ASSERT(IsActiveEditorAcceptingVisualStyles());

    InsertLinkDlg insert_link_dlg(std::string(), std::string(), this);

    if( insert_link_dlg.DoModal() != IDOK )
        return;

    m_currentEditor->InsertLink(insert_link_dlg.GetText(), insert_link_dlg.GetUrl());
}


void CQSFEView::OnChangeTextDirectionRightToLeft()
{
    ASSERT(IsActiveEditorVisualHtml());

    m_htmlEditor.GetHtmlEditorCtrl().RightToLeft();
}


void CQSFEView::OnChangeTextDirectionLeftToRight()
{
    ASSERT(IsActiveEditorVisualHtml());

    m_htmlEditor.GetHtmlEditorCtrl().LeftToRight();
}


void CQSFEView::OnChangeEditorType(const UINT nID)
{
    const bool use_html_code_view = ( nID == ID_QSF_EDITOR_EDIT_HTML_VISUAL_CODE );
    const bool use_html_editor = ( use_html_code_view || nID == ID_QSF_EDITOR_EDIT_HTML_VISUAL );

    // handle the easy case of toggling the HTML editor's code view
    if( use_html_editor && m_currentEditor == &m_htmlEditor )
    {
        HtmlEditorCtrl& html_editor_ctrl = m_htmlEditor.GetHtmlEditorCtrl();

        if( use_html_code_view != html_editor_ctrl.GetCodeViewShowing() )
            html_editor_ctrl.ToggleCodeView();

        return;
    }

    // MARKDOWN_TODO
}


void CQSFEView::OnUpdateChangeEditorType(CCmdUI* const pCmdUI)
{
    bool check = ( pCmdUI->m_pOther == &m_toolbar &&
                   GetFormDoc()->GetCapiEditorViewModel().CanHaveText() );

    if( check )
    {
        switch( pCmdUI->m_nID )
        {
            case ID_QSF_EDITOR_EDIT_HTML_VISUAL:
                check = ( m_currentEditor == &m_htmlEditor && !m_htmlEditor.GetHtmlEditorCtrl().GetCodeViewShowing() );
                break;

            case ID_QSF_EDITOR_EDIT_HTML_VISUAL_CODE:
                check = ( m_currentEditor == &m_htmlEditor && m_htmlEditor.GetHtmlEditorCtrl().GetCodeViewShowing() );
                break;

            case ID_QSF_EDITOR_EDIT_TEXT_HTML:
                check = ( m_currentCapiText.GetFormat() == CapiText::Format::ReportHtml );
                break;

            case ID_QSF_EDITOR_EDIT_TEXT_MARKDOWN:
                check = ( m_currentCapiText.GetFormat() == CapiText::Format::ReportMarkdown );
                break;
        }
    }

    pCmdUI->Enable();
    pCmdUI->SetCheck(check);
}


template<>
CapiText::Type CQSFEView::ConvertResourceId(const UINT nID)
{
    switch( nID )
    {
        case ID_QSF_EDITOR_VIEW_QUESTION:
            return CapiText::Type::Question;

        default:
            ASSERT(nID == ID_QSF_EDITOR_VIEW_HELP);
            return CapiText::Type::Help;
    }
}


void CQSFEView::OnViewQuestionHelpText(const UINT nID)
{
    const CapiText::Type this_text_type = ConvertResourceId<CapiText::Type>(nID);

    if( m_textTypeEditing != this_text_type )
    {
        m_textTypeEditing = this_text_type;
        UpdateDisplayText();
    }
}


void CQSFEView::OnUpdateViewQuestionHelpText(CCmdUI* const pCmdUI)
{
    const bool check = ( pCmdUI->m_pOther == &m_toolbar &&
                         GetFormDoc()->GetCapiEditorViewModel().CanHaveText() &&
                         m_textTypeEditing == ConvertResourceId<CapiText::Type>(pCmdUI->m_nID) );

    pCmdUI->Enable();
    pCmdUI->SetCheck(check);
}


void CQSFEView::OnLanguageChanged()
{
    SetLanguage(m_toolbar.GetLanguageLabel());
    UpdateDisplayText();
}
