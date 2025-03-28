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


IMPLEMENT_DYNAMIC(CQSFEView, CFormView)

BEGIN_MESSAGE_MAP(CQSFEView, CFormView)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
    ON_COMMAND(ID_EDIT_PASTE_WITHOUT_FORMATTING, OnEditPasteWithoutFormatting)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
    ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE_WITHOUT_FORMATTING, OnUpdateEditPaste)
    ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
    ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
    ON_COMMAND(ID_EDIT_CUT, OnEditCut)
    ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
    ON_COMMAND(ID_EDIT_SELECT_ALL, OnEditSelectAll)
    ON_WM_CONTEXTMENU()
    ON_EN_CHANGE(IDC_HTML_EDIT, OnChangeHtmlEdit)
    ON_EN_SETFOCUS(IDC_HTML_EDIT, OnSetfocusHtmlEdit)
    ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
    ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
    ON_COMMAND(ID_VIEW_FORM, OnViewForm)
    ON_COMMAND(ID_VIEW_LOGIC, OnViewLogic)
    ON_COMMAND(ID_VIEW_HIDE, OnViewHide)

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
    ON_COMMAND(ID_TEXT_DIR_RTL, OnChangeTextDirectionRightToLeft)
    ON_UPDATE_COMMAND_UI(ID_TEXT_DIR_RTL, OnUpdateChangeTextDirectionRightToLeft)
    ON_COMMAND(ID_TEXT_DIR_LTR, OnChangeTextDirectionLeftToRight)
    ON_UPDATE_COMMAND_UI(ID_TEXT_DIR_LTR, OnUpdateChangeTextDirectionLeftToRight)
    ON_COMMAND(ID_EDIT_INSERT_IMAGE, OnEditInsertImage)
    ON_UPDATE_COMMAND_UI(ID_EDIT_INSERT_IMAGE, OnUpdateEditInsertImage)
    ON_UPDATE_COMMAND_UI(ID_INSERT_TABLE, OnUpdateInsertTable)
    ON_COMMAND(ID_INSERT_TABLE, OnInsertTable)
    ON_UPDATE_COMMAND_UI(ID_INSERT_LINK, OnUpdateInsertLink)
    ON_COMMAND(ID_INSERT_LINK, OnInsertLink)
    ON_COMMAND(ID_FORMAT_OUTLINE_BULLET, OnEditFormatOutlineBullet)
    ON_UPDATE_COMMAND_UI(ID_FORMAT_OUTLINE_BULLET, OnUpdateEditFormatOutlineBullet)
    ON_COMMAND(ID_FORMAT_OUTLINE_NUMBER, OnEditFormatOutlineNumbering)
    ON_UPDATE_COMMAND_UI(ID_FORMAT_OUTLINE_NUMBER, OnUpdateEditFormatOutlineNumbering)
    ON_COMMAND(ID_QSF_EDITOR_VIEW_CODE, OnToggleViewCode)
    ON_UPDATE_COMMAND_UI(ID_QSF_EDITOR_VIEW_CODE, OnUpdateToggleViewCode)
    ON_COMMAND(IDC_EDIT_LANG, OnLanguageChanged)
    ON_CBN_SELENDOK(IDC_EDIT_LANG, OnLanguageChanged)
    ON_UPDATE_COMMAND_UI(ID_TOGGLE_QN, OnUpdateToggleQuestionHelpText)
    ON_COMMAND(ID_TOGGLE_QN, OnToggleQuestionHelpText)
    ON_WM_TIMER()
END_MESSAGE_MAP()


CQSFEView::CQSFEView(const CString& ent_path)
    :   CFormView(IDD_QSF_EDIT_VIEW),
        m_ent_path(ent_path),
        m_textType(CapiText::Type::Question),
        m_languageIndex(0)
{
    SetupFileServer(ent_path);
    ASSERT(m_questionTextVirtualFileMapping != nullptr);

    m_htmlEditorCtrl.SetUrl(m_questionTextVirtualFileMapping->GetUrl());
}


CQSFEView::~CQSFEView()
{
}


void CQSFEView::DoDataExchange(CDataExchange* pDX)
{
    CFormView::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_HTML_EDIT, m_htmlEditorCtrl);
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
        [&](const auto& language) { return ( language.GetLabel() == language_label_sv ); });

    if( lookup != m_languages.end() )
        SetLanguage(std::distance(m_languages.cbegin(), lookup));
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CQSFEView::Create
//
// note: this has to be here so that the frame can call it at runtime
//
/////////////////////////////////////////////////////////////////////////////
BOOL CQSFEView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext)
{
    return CFormView::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
}

/////////////////////////////////////////////////////////////////////////////
//
//                           CQSFEView::OnCreate
//
/////////////////////////////////////////////////////////////////////////////
int CQSFEView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFormView::OnCreate(lpCreateStruct) == -1)
        return -1;

    auto question_manager = GetFormDoc()->GetCapiQuestionManager();
    SetStyles(question_manager->GetStyles());

    if (!m_toolbar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC, CRect(1,1,1,1), AFX_IDW_TOOLBAR + 50))
        return -1;
    if (!m_toolbar.LoadToolBar(IDR_QSFBAR))
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


void CQSFEView::SetupFileServer(const CString& application_filename)
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
        m_fileServer->CreateVirtualHtmlFile(PortableFunctions::PathGetDirectory(UTF8_TODO::GetUtf8(application_filename)),
        [&]()
        {
            return editor_html;
        }));
}


void CQSFEView::OnEditCopy()
{
    m_htmlEditorCtrl.Copy();
}


void CQSFEView::OnEditSelectAll()
{
    m_htmlEditorCtrl.SelectAll();
}


void CQSFEView::OnUpdateEditCopy(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_htmlEditorCtrl.CanCopy());
}


void CQSFEView::OnEditCut()
{
    m_htmlEditorCtrl.Cut();
}


void CQSFEView::OnUpdateEditCut(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_htmlEditorCtrl.CanCut());
}


void CQSFEView::OnEditPaste()
{
    m_htmlEditorCtrl.Paste(true);
}


void CQSFEView::OnEditPasteWithoutFormatting()
{
    m_htmlEditorCtrl.Paste(false);
}


void CQSFEView::OnUpdateEditPaste(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(m_htmlEditorCtrl.CanPaste());
}


void CQSFEView::OnSize(UINT nType, int cx, int cy)
{
    CFormView::OnSize(nType, cx, cy);

    CRect rcClient;
    GetClientRect(&rcClient);

    m_toolbar.WrapToolBar(rcClient.Width(), rcClient.Height());
    CSize toolbar_size = m_toolbar.CalcSize(FALSE);
    const int border = 8;
    toolbar_size.cy += border;
    m_toolbar.SetWindowPos(NULL, 0, border/2, toolbar_size.cx, toolbar_size.cy,
        SWP_NOACTIVATE | SWP_NOZORDER);

    if (m_htmlEditorCtrl.m_hWnd) {
        rcClient.top += toolbar_size.cy;
        m_htmlEditorCtrl.MoveWindow(rcClient);
        m_htmlEditorCtrl.Resize(nType,cx, cy);
    }
}


void CQSFEView::OnViewHide(void)
{
    CFormChildWnd* pParentFrame = (CFormChildWnd*)GetParentFrame();
    pParentFrame->m_bHideSecondLang = !pParentFrame->m_bHideSecondLang;
    pParentFrame->DisplayActiveMode();
}


void CQSFEView::OnQuestionTextTypeChanged()
{
    UpdateDisplayText();
}


CapiEditorViewModel& CQSFEView::GetViewModel()
{
    return GetFormDoc()->GetCapiEditorViewModel();
}


CFormDoc* CQSFEView::GetFormDoc()
{
    CFormDoc* doc = DYNAMIC_DOWNCAST(CFormDoc, GetDocument());
    ASSERT(doc);
    return doc;
}


void CQSFEView::StartIdleTimer()
{
    m_idle_timer = SetTimer(idleTimerID, 1000, NULL);
}


void CQSFEView::StopIdleTimer()
{
    if (m_idle_timer) {
        KillTimer(idleTimerID);
        m_idle_timer.reset();
    }
}


void CQSFEView::UpdateFillErrorDisplay()
{
    std::map<std::string, std::string> errors;

    for( const auto& [fill, result] : m_fill_syntax_check_results )
    {
        if(std::holds_alternative<CapiEditorViewModel::SyntaxCheckError>(result) )
            errors.try_emplace(UTF8_TODO::GetUtf8(fill), std::get<CapiEditorViewModel::SyntaxCheckError>(result).error_message);
    }

    m_htmlEditorCtrl.SetSyntaxErrors(errors);
}


void CQSFEView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
    BCMenu popMenu;
    popMenu.CreatePopupMenu();

    popMenu.AppendMenu(MF_STRING | (m_htmlEditorCtrl.CanCut() ? 0x0L : MF_GRAYED), ID_EDIT_CUT, _T("Cu&t\tCtrl+X"));
    popMenu.AppendMenu(MF_STRING | (m_htmlEditorCtrl.CanCopy() ? 0x0L : MF_GRAYED), ID_EDIT_COPY, _T("&Copy\tCtrl+C"));
    popMenu.AppendMenu(MF_STRING | (m_htmlEditorCtrl.CanPaste() ? 0x0L : MF_GRAYED), ID_EDIT_PASTE, _T("&Paste\tCtrl+V"));
    popMenu.AppendMenu(MF_STRING | (m_htmlEditorCtrl.CanPaste() ? 0x0L : MF_GRAYED), ID_EDIT_PASTE_WITHOUT_FORMATTING, _T("Paste &Without formatting\tCtrl+Shift+V"));
    popMenu.AppendMenu(MF_SEPARATOR);
    popMenu.AppendMenu(MF_STRING | (m_htmlEditorCtrl.GetText().empty() ? MF_GRAYED : 0x0L), ID_EDIT_SELECT_ALL, _T("Select &All"));
    popMenu.AppendMenu(MF_SEPARATOR);

    CFormChildWnd* pParentFrame = (CFormChildWnd*)GetParentFrame();

    if(pParentFrame->m_bHideSecondLang) {
        popMenu.AppendMenu(MF_STRING, ID_VIEW_HIDE, _T("&Show Second View"));
    }
    else {
        popMenu.AppendMenu(MF_STRING, ID_VIEW_HIDE, _T("&Hide Second View"));
    }

    popMenu.AppendMenu(MF_SEPARATOR);
    popMenu.AppendMenu (MF_STRING, ID_VIEW_FORM, _T("View &Form"));
    popMenu.AppendMenu (MF_STRING, ID_VIEW_LOGIC, _T("View &Logic"));

    popMenu.LoadToolbar(IDR_FORM_FRAME);
    popMenu.TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);
}


void CQSFEView::OnChangeHtmlEdit()
{
    // Text changed in editor - update it in document
    CFormDoc* form_doc = GetFormDoc();
    CapiEditorViewModel& view_model = form_doc->GetCapiEditorViewModel();
    if (view_model.CanHaveText()) {
        view_model.SetText(m_languageIndex, m_textType, WS2CS(m_htmlEditorCtrl.GetText()));
    }

    StartIdleTimer();
}


void CQSFEView::OnUpdate(CView* pSender, LPARAM lHint, CObject* /*pHint*/)
{
    if (pSender != this) {
        if (lHint == Hint::CapiEditorUpdateLanguages) {
            SetLanguages(GetFormDoc()->GetCapiQuestionManager()->GetLanguages());
        }
        if (lHint == Hint::CapiEditorUpdateStyles || lHint == Hint::CapiEditorUpdateQuestionStyles) {
            SetStyles(GetFormDoc()->GetCapiQuestionManager()->GetStyles());
        }
        if (lHint == Hint::CapiEditorUpdateQuestion || lHint == Hint::CapiEditorUpdateQuestionStyles) {
            m_fill_syntax_check_results.clear();
            UpdateFillErrorDisplay();
        }
        UpdateDisplayText();
    }
}


void CQSFEView::SetStyles(const std::vector<CapiStyle>& styles)
{
    std::vector<HtmlEditorCtrl::Style> editorStyles;
    for (const CapiStyle& style : styles) {
        editorStyles.emplace_back(HtmlEditorCtrl::Style{ "span", style.name, style.class_name, style.css });
    }
    m_htmlEditorCtrl.SetStyles(editorStyles);
    m_toolbar.SetStyles(editorStyles);
}


void CQSFEView::OnSetfocusHtmlEdit()
{
    // Make this the active view - this ensures that menu selections (undo, cut, paste...)
    // will be routed to this view
    GetParentFrame()->SetActiveView(this);
    GetDocument()->UpdateAllViews(nullptr, Hint::CapiEditorUpdateStyles);
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CQSFEView::UpdateDisplayText()
//
/////////////////////////////////////////////////////////////////////////////////
void CQSFEView::UpdateDisplayText()
{
    CFormDoc* const form_doc = GetFormDoc();
    CapiEditorViewModel& view_model = form_doc->GetCapiEditorViewModel();

    if( view_model.CanHaveText() )
    {
        EnableWindow(TRUE);
        m_htmlEditorCtrl.EnableWindow(TRUE);

        const CapiText& capi_text = view_model.GetText(m_languageIndex, m_textType);

        if( capi_text.GetText().empty() )
        {
            m_htmlEditorCtrl.Clear();
        }

        else
        {
            std::wstring wide_text = TC::ToWide(capi_text.GetText());

            if( wide_text != m_htmlEditorCtrl.GetText() )
                m_htmlEditorCtrl.SetText(std::move(wide_text));

            StartIdleTimer();
        }
    }

    else
    {
        EnableWindow(FALSE);
        m_htmlEditorCtrl.EnableWindow(FALSE);

        m_htmlEditorCtrl.Clear();
    }

    UpdateToolbar();
}


void CQSFEView::OnEditUndo()
{
    m_htmlEditorCtrl.Undo();
}


void CQSFEView::OnEditRedo()
{
    m_htmlEditorCtrl.Redo();
}


void CQSFEView::OnViewForm()
{
    CFormChildWnd* pParentFrame = (CFormChildWnd*)GetParentFrame();
    pParentFrame->OnViewForm();
}


void CQSFEView::OnViewLogic()
{
    CFormChildWnd* pParentFrame = (CFormChildWnd*)GetParentFrame();
    pParentFrame->OnViewLogic();
}


void CQSFEView::OnFormatStyle()
{
    m_htmlEditorCtrl.ApplyStyle(m_toolbar.GetSelectedStyle());
    m_htmlEditorCtrl.MoveFocus();
}


void CQSFEView::OnUpdateFormatStyle(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
    if (pCmdUI->m_pOther == &m_toolbar) {
        const HtmlEditorCtrl::Style* style = m_htmlEditorCtrl.GetStyle();
        if (style)
            m_toolbar.SetSelectedStyle(*style);
    }
}


void CQSFEView::OnFormatColor()
{
    COLORREF color = m_toolbar.GetForeColor();
    m_htmlEditorCtrl.SetForeColor(color);
}


void CQSFEView::OnUpdateFormatColor(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
}


void CQSFEView::OnFormatAlignLeft()
{
    m_htmlEditorCtrl.AlignLeft();
}


void CQSFEView::OnUpdateFormatAlignLeft(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
    if (pCmdUI->m_pOther == &m_toolbar)
        pCmdUI->SetCheck(m_htmlEditorCtrl.GetTextAlignment() == HtmlEditorCtrl::TextAlign::Left);
}


void CQSFEView::OnFormatAlignCenter()
{
    m_htmlEditorCtrl.AlignCenter();
}


void CQSFEView::OnUpdateFormatAlignCenter(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
    if (pCmdUI->m_pOther == &m_toolbar)
        pCmdUI->SetCheck(m_htmlEditorCtrl.GetTextAlignment() == HtmlEditorCtrl::TextAlign::Center);
}


void CQSFEView::OnFormatAlignRight()
{
    m_htmlEditorCtrl.AlignRight();
}


void CQSFEView::OnUpdateFormatAlignRight(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
    if (pCmdUI->m_pOther == &m_toolbar)
        pCmdUI->SetCheck(m_htmlEditorCtrl.GetTextAlignment() == HtmlEditorCtrl::TextAlign::Right);
}


void CQSFEView::OnChangeTextDirectionRightToLeft()
{
    m_htmlEditorCtrl.RightToLeft();
}


void CQSFEView::OnUpdateChangeTextDirectionRightToLeft(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
}


void CQSFEView::OnChangeTextDirectionLeftToRight()
{
    m_htmlEditorCtrl.LeftToRight();
}


void CQSFEView::OnUpdateChangeTextDirectionLeftToRight(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
}


void CQSFEView::OnEditFormatOutlineBullet()
{
    m_htmlEditorCtrl.UnorderedList();
}


void CQSFEView::OnUpdateEditFormatOutlineBullet(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
    if (pCmdUI->m_pOther == &m_toolbar)
        pCmdUI->SetCheck(m_htmlEditorCtrl.GetListStyle() == HtmlEditorCtrl::ListStyle::Unordered);
}


void CQSFEView::OnEditFormatOutlineNumbering()
{
    m_htmlEditorCtrl.OrderedList();
}


void CQSFEView::OnUpdateEditFormatOutlineNumbering(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
    if (pCmdUI->m_pOther == &m_toolbar)
        pCmdUI->SetCheck(m_htmlEditorCtrl.GetListStyle() == HtmlEditorCtrl::ListStyle::Ordered);
}


void CQSFEView::OnEditInsertImage()
{
    ImageFileDialog dlg;
    if (dlg.DoModal() != IDOK) {
        return;
    }

    CString image_path_on_disk = dlg.GetPathName();
    if (image_path_on_disk.IsEmpty()) {
        return;
    }
    if (PathGetVolume(UTF8_TODO::GetUtf8(image_path_on_disk)) != PathGetVolume(UTF8_TODO::GetUtf8(m_ent_path))) {
        AfxMessageBox(_T("Images must be on the same disk volume as your CSPro application. ")
                      _T("Try copying the file to the folder that contains your application."), MB_ICONEXCLAMATION);
        return;
    }

    std::wstring relative_path = GetRelativeFName(m_ent_path, image_path_on_disk);
    m_htmlEditorCtrl.InsertImage(Encoders::ToUri(UTF8_TODO::GetUtf8(PortableFunctions::PathToForwardSlash(relative_path)), false));
}


void CQSFEView::OnUpdateEditInsertImage(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
}


void CQSFEView::OnInsertTable()
{
    CSize dimensions = m_toolbar.GetTableDimensions();
    m_htmlEditorCtrl.InsertTable(dimensions);
}


void CQSFEView::OnUpdateInsertTable(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
}


void CQSFEView::OnInsertLink()
{
    m_htmlEditorCtrl.ShowInsertLinkDialog();
}


void CQSFEView::OnUpdateInsertLink(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
}


void CQSFEView::OnToggleViewCode()
{
    m_htmlEditorCtrl.ToggleCodeView();
}


void CQSFEView::OnUpdateToggleViewCode(CCmdUI* pCmdUI)
{
    pCmdUI->Enable();
    if (pCmdUI->m_pOther == &m_toolbar)
        pCmdUI->SetCheck(m_htmlEditorCtrl.GetCodeViewShowing());
}


void CQSFEView::OnLanguageChanged()
{
    SetLanguage(m_toolbar.GetLanguageLabel());
    UpdateDisplayText();
}


void CQSFEView::OnUpdateToggleQuestionHelpText(CCmdUI* pCmdUI)
{
    pCmdUI->Enable();
    if (pCmdUI->m_pOther == &m_toolbar)
        pCmdUI->SetCheck(m_textType == CapiText::Type::Help);
}


void CQSFEView::OnToggleQuestionHelpText()
{
    m_textType = ( m_textType == CapiText::Type::Question ) ? CapiText::Type::Help :
                                                              CapiText::Type::Question;
    OnQuestionTextTypeChanged();
}


void CQSFEView::OnTimer(UINT nIDEvent)
{
    if (nIDEvent == idleTimerID) {
        StopIdleTimer();

        CFormDoc* doc = GetFormDoc();
        CapiEditorViewModel& view_model = doc->GetCapiEditorViewModel();
        if (view_model.CanHaveText()) {
            CapiText text = view_model.GetText(m_languageIndex, m_textType);
            bool updated = false;
            for (const CapiFill& fill : text.GetFills()) {
                std::wstring fill_text = CS2WS(fill.GetTextToEvaluate());
                auto syntax_check_result = m_fill_syntax_check_results.find(fill_text);
                if (syntax_check_result == m_fill_syntax_check_results.end()) {
                    m_fill_syntax_check_results[fill_text] = view_model.CheckSyntax(CapiLogicParameters::Type::Fill, UTF8_TODO::GetUtf8(fill_text));
                    updated = true;
                }
            }
            if (updated)
                UpdateFillErrorDisplay();
        }
    }
}


void CQSFEView::OnFormatBold()
{
    m_htmlEditorCtrl.Bold();
}


void CQSFEView::OnUpdateFormatBold(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
    if (pCmdUI->m_pOther == &m_toolbar)
        pCmdUI->SetCheck(m_htmlEditorCtrl.IsBold());
}


void CQSFEView::OnFormatItalic()
{
    m_htmlEditorCtrl.Italic();
}


void CQSFEView::OnUpdateFormatItalic(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
    if (pCmdUI->m_pOther == &m_toolbar)
        pCmdUI->SetCheck(m_htmlEditorCtrl.IsItalic());
}


void CQSFEView::OnFormatUnderline()
{
    m_htmlEditorCtrl.Underline();
}


void CQSFEView::OnUpdateFormatUnderline(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
    if (pCmdUI->m_pOther == &m_toolbar)
        pCmdUI->SetCheck(m_htmlEditorCtrl.IsUnderline());
}


void CQSFEView::OnFormatFontFace()
{
    m_htmlEditorCtrl.SetFont(UTF8_TODO::GetUtf8(m_toolbar.GetFontFace()));
    m_htmlEditorCtrl.MoveFocus();
}


void CQSFEView::OnUpdateFormatFontFace(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
    if (pCmdUI->m_pOther == &m_toolbar)
        m_toolbar.SetFontFace(UTF8_TODO::GetWide(m_htmlEditorCtrl.GetFontName()));
}


void CQSFEView::OnFormatFontSize()
{
    m_htmlEditorCtrl.SetFontSize(m_toolbar.GetFontSize());
    m_htmlEditorCtrl.MoveFocus();
}


void CQSFEView::OnUpdateFormatFontSize(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(!m_htmlEditorCtrl.GetCodeViewShowing());
    if (pCmdUI->m_pOther == &m_toolbar)
        m_toolbar.SetFontSize(m_htmlEditorCtrl.GetFontSize());
}


void CQSFEView::UpdateToolbar()
{
    CCmdUI buttonCmdUI;
    buttonCmdUI.m_nIndexMax = m_toolbar.GetCount();
    buttonCmdUI.m_pOther = &m_toolbar;
    for (int i = 0; i < (int)buttonCmdUI.m_nIndexMax; ++i)
    {
        if (m_toolbar.GetItemID(i)) {
            buttonCmdUI.m_nIndex = i;
            buttonCmdUI.m_nID = m_toolbar.GetItemID(i);
            buttonCmdUI.DoUpdate(this, FALSE);
        }
    }
}
